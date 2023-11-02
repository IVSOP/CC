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
#include "server.h"

#define BUFFER_SIZE (uint32_t) 1500

void read_data(Server& server, in_addr& ip, FS_Track *message) {
    uint8_t OPCode = message->fsTrackGetOpcode();
    std::string errorDetails;
    ClientTCPSocket clientResponse = ClientTCPSocket(inet_ntoa(ip)); // TODO
    std::vector<FS_Track::PostFileBlocksData> data;

    switch (OPCode) {
        // Cases 0 and 1 are the same
        // Register node
        case 0:

        // Update node
        case 1:
            server.registerUpdateNode(ip.s_addr, message->regUpdateDataGetData());
            break;

        // Get Message
        case 2:
            data = (server.getNodesWithFile(message->fsTrackGetHash()));
            FS_Track::sendPostMessage(clientResponse, message->fsTrackGetHash(), data);
            break;

        // Post Message
        case 3:
            errorDetails = "Only a server can send a Post Message";
            FS_Track::sendErrorMessage(clientResponse, errorDetails);
            break;

        // Error Message
        case 4:
            // TODO what to do with error messages
            break;

        // ByeBye Message
        case 5:
            server.deleteNode(ip.s_addr);
            break;
        default:
            errorDetails = "Message has invalid OPCode";
            FS_Track::sendErrorMessage(clientResponse, errorDetails);
            break;
    }
}

void serveClient(ServerTCPSocket::SocketInfo connection, Server& serverData, std::mutex& mtx, std::condition_variable& cdt) {
    uint8_t *buffer = new uint8_t[BUFFER_SIZE];
    uint32_t bytes, remainBytes;
    FS_Track *message;

    while (true) {
        message = new FS_Track();

        bytes = connection.receiveData(buffer, 4);

        if (bytes == 0) break;

        message->fsTrackHeaderReadBuffer(buffer, bytes);

        if (message->fsTrackGetOpt() == 1) {
            bytes = connection.receiveData(buffer, 8);
            message->fsTrackReadHash(buffer, bytes);
        }

        remainBytes = message->fsTrackGetSize();

        while (remainBytes > 0) {
            bytes = connection.receiveData(buffer, std::min(BUFFER_SIZE, remainBytes));

            message->setData(buffer, bytes);

            remainBytes -= bytes;
        }

        read_data(serverData, connection.addr.sin_addr, message);

        delete message;
    }

    delete[] (char *) buffer;

    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);
    cdt.notify_one();
}

void acceptClients(Server& serverData, std::vector<ThreadRAII>& threadGraveyard, std::mutex& mtx, std::condition_variable& cdt) {
    ServerTCPSocket serverSocket = ServerTCPSocket();
    serverSocket.socketListen();

    ServerTCPSocket::SocketInfo new_connection;

    while (true) {
        new_connection = serverSocket.acceptClient();

        threadGraveyard.emplace_back(std::thread(serveClient, new_connection, std::ref(serverData), std::ref(mtx), std::ref(cdt)), ThreadRAII::DtorAction::join);
    }
}

void cleanUpThreads(std::vector<ThreadRAII>& threadGraveyard, std::mutex& mtx, std::condition_variable& cdt){
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);
    while(true){
        cdt.wait(lock);

        threadGraveyard.erase(threadGraveyard.begin(), threadGraveyard.end());
    }
}

// int main () {
//     // printf("%lu\n", offsetof(FS_Transfer_Packet, data));
//     // printf("%lu\n", sizeof(FS_Transfer_Packet));
//     // printf("%lu\n", FS_TRANSFER_PACKET_SIZE);
//     // printf("%lu\n", sizeof(FS_Data) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t));

//     //ThreadPool<ServerTCPSocket::SocketInfo, 10, 8>;

//     acceptClients();

//     return 0;
// }

int main() {
    /*
    Server s;
    std::vector<uint32_t> josefina = {125, 225, 3, 526};
    FS_Track::RegUpdateData data = FS_Track::RegUpdateData(1240912490, josefina);
    s.addNewInfo(124152, data);
    s.printMap();
    */

    Server server = Server();

    std::mutex mtx = std::mutex();
    std::condition_variable cdt = std::condition_variable();

    std::vector<ThreadRAII> threadGraveyard = std::vector<ThreadRAII>();
    ThreadRAII cleanUp(std::thread([&](){ cleanUpThreads(threadGraveyard, mtx, cdt); /*Lambda Function*/}), ThreadRAII::DtorAction::join);

    acceptClients(server, threadGraveyard, mtx, cdt);
}
