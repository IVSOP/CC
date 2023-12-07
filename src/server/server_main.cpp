#include "TCP_socket.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "TCP_socket.h"
#include "thread_pool.h"
#include "threadRAII.h"
#include "fs_track.h"
#include "fs_track_test.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netdb.h>
#include "server.h"
#include "errors.h"

#define BUFFER_SIZE (uint32_t)1500

void read_data(Server &server, ServerTCPSocket::SocketInfo &connection, FS_Track message)
{
    uint8_t OPCode = message.fsTrackGetOpcode();
    std::string errorDetails;
    std::vector<FS_Track::PostFileBlocksData> data;
    std::string node;
    char hostname[LEN_HOSTNAME];
    int status;

    switch (OPCode)
    {
    // Cases 0 and 1 are the same
    // Register node
    case 0:

    // Update node
    case 1:
        status = getnameinfo((struct sockaddr *)&connection.addr, sizeof(struct sockaddr_in), hostname, LEN_HOSTNAME, NULL, 0, 0);
        if (status != 0)
        {
            print_error("There has been an error aquiring the host name");
        }

        node = std::string(hostname);

        printf("Received register/update message from node %s\n", hostname);

        server.registerUpdateNode(node, message.regUpdateDataGetData());
        break;

    // Get Message
    case 2:

        printf("Received get message from node %s\n", inet_ntoa(connection.addr.sin_addr));
        data = server.getNodesWithFile(message.fsTrackGetHash());
        FS_Track::sendPostMessage(connection, message.fsTrackGetHash(), data);
        break;

    // Post Message
    case 3:
        errorDetails = "Only a server can send a Post Message";
        FS_Track::sendErrorMessage(connection, errorDetails);
        break;

    // Error Message
    case 4:
        // TODO what to do with error messages
        break;

    // ByeBye Message
    case 5:
        status = getnameinfo((struct sockaddr *)&connection.addr, sizeof(struct sockaddr_in), hostname, LEN_HOSTNAME, NULL, 0, 0);
        if (status != 0)
        {
            print_error("There has been an error aquiring the host name");
        }

        node = std::string(hostname);

        printf("Received bye bye message from node %s\n", hostname);

        server.deleteNode(node);
        break;
    default:
        errorDetails = "Message has invalid OPCode";
        FS_Track::sendErrorMessage(connection, errorDetails);
        break;
    }
}

void serveClient(ServerTCPSocket::SocketInfo connection, Server &serverData, std::mutex &mtx, std::condition_variable &cdt)
{
    FS_Track message = FS_Track();
    uint8_t *buffer = new uint8_t[BUFFER_SIZE];

    puts("Connection initiated.");

    while (FS_Track::readMessage(message, buffer, BUFFER_SIZE, connection))
    {
        read_data(serverData, connection, message);
        message = FS_Track();
    }

    puts("Connection terminated.");

    message = FS_Track(5);
    read_data(serverData, connection, message);

    delete[] (uint8_t *)buffer;

    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);
    cdt.notify_one();
}

void acceptClients(Server &serverData, std::vector<ThreadRAII> &threadGraveyard, std::mutex &mtx, std::condition_variable &cdt)
{
    ServerTCPSocket serverSocket = ServerTCPSocket();
    serverSocket.socketListen();

    ServerTCPSocket::SocketInfo new_connection;

    while (true)
    {
        new_connection = serverSocket.acceptClient();

        threadGraveyard.emplace_back(std::thread(serveClient, new_connection, std::ref(serverData), std::ref(mtx), std::ref(cdt)), ThreadRAII::DtorAction::join);
    }
}

void cleanUpThreads(std::vector<ThreadRAII> &threadGraveyard, std::mutex &mtx, std::condition_variable &cdt)
{
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);
    while (true)
    {
        cdt.wait(lock);

        threadGraveyard.erase(threadGraveyard.begin(), threadGraveyard.end());
    }
}

int main()
{
    Server server = Server();

    std::mutex mtx = std::mutex();
    std::condition_variable cdt = std::condition_variable();

    std::vector<ThreadRAII> threadGraveyard = std::vector<ThreadRAII>();
    ThreadRAII cleanUp(std::thread([&]()
                                   { cleanUpThreads(threadGraveyard, mtx, cdt); /*Lambda Function*/ }),
                       ThreadRAII::DtorAction::join);

    acceptClients(server, threadGraveyard, mtx, cdt);
}
