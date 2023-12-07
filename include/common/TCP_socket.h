#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#define TCP_PORT 9090
#define MAX_CONNECTIONS_IN_QUEUE 10


struct ServerTCPSocket {
    int serverfd;
    sockaddr_in addr;

    ServerTCPSocket();

    ~ServerTCPSocket();

    void socketListen() const;

    // devolvida depois de um accept
    struct SocketInfo {
        int sockfd = -1;
        struct sockaddr_in addr;

        ssize_t receiveData(void *buf, size_t len) const;

        ssize_t sendData(const void *buf, size_t len);
    };

    // sq fazer variante que faz copia, recebe SocketInfo*
    SocketInfo acceptClient() const;
};

struct ClientTCPSocket {
    int clientfd;
    sockaddr_in addr;

    ClientTCPSocket() = delete;

    ClientTCPSocket(const std::string &ipv4);
	ClientTCPSocket(struct sockaddr_in ipv4);

    ~ClientTCPSocket();

    ssize_t receiveData(void *buf, size_t len) const;

    ssize_t sendData(const void *buf, size_t len);
};

#endif