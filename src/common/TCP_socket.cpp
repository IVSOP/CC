#include "TCP_socket.h"
#include "errors.h"
#include <unistd.h>
#include "socket_common.h"

ServerTCPSocket::ServerTCPSocket()
        : serverfd(-1) {
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        print_error("Error creating socket");
        exit(EXIT_FAILURE);
    }

    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->addr.sin_port = htons(TCP_PORT);

    int opt = 1;
    // Forcefully attaching socket to the port 9090
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        print_error("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    //Enable SO_REUSEPORT option
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        print_error("setsockopt SO_REUSEPORT");
    }

    if (bind(serverfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        print_error("Error binding socket");
        exit(EXIT_FAILURE);
    }
}

ServerTCPSocket::~ServerTCPSocket() {
    close(serverfd);
}

void ServerTCPSocket::socketListen() const {
    if (listen(serverfd, MAX_CONNECTIONS_IN_QUEUE) < 0) {
        print_error("Error listening for connections");
        exit(EXIT_FAILURE);
    }
}

// returns new socket fd
// NOT responsibility of this class to close new socket
ServerTCPSocket::SocketInfo ServerTCPSocket::acceptClient() const {
    SocketInfo socketInfo;
    socklen_t addrlen = sizeof(socketInfo.addr);
    int clientSocket = accept(serverfd, reinterpret_cast<struct sockaddr *>(&socketInfo.addr), &addrlen);
    if (clientSocket < 0) {
        print_error("Error accepting connection");
        exit(EXIT_FAILURE);
    }
    socketInfo.sockfd = clientSocket;
    return socketInfo;
}

ssize_t ServerTCPSocket::SocketInfo::receiveData(void *buf, size_t len) const {
    return recv(sockfd, buf, len, 0);
}

ssize_t ServerTCPSocket::SocketInfo::sendData(const void *buf, size_t len) {
    return send(sockfd, buf, len, 0);
}










////////////////////////////////////////////////////////








ClientTCPSocket::ClientTCPSocket(const std::string &ipv4)
        : clientfd(-1) {
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        print_error("Error creating socket");
        exit(EXIT_FAILURE);
    }

    setIPv4(ipv4, &addr);

    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(TCP_PORT);

    // int opt = 1;
    // if (setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    //     print_error("Error setting socket options");
    //     exit(EXIT_FAILURE);
    // }

    int status;
    if ((status = connect(clientfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr))) < 0) {
        print_error("Connection Failed");
        exit(EXIT_FAILURE);
    }
}

// impossivel isto ter ficado mais manhoso
ClientTCPSocket::ClientTCPSocket(struct sockaddr_in ipv4)
        : clientfd(-1) {
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        print_error("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // int opt = 1;
    // if (setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    //     print_error("Error setting socket options");
    //     exit(EXIT_FAILURE);
    // }

    int status;
    if ((status = connect(clientfd, reinterpret_cast<struct sockaddr *>(&ipv4), sizeof(&ipv4))) < 0) {
        print_error("Connection Failed");
        exit(EXIT_FAILURE);
    }
}

ClientTCPSocket::~ClientTCPSocket() {
    close(clientfd);
}

ssize_t ClientTCPSocket::receiveData(void *buf, size_t len) const {
    return recv(clientfd, buf, len, 0);
}

ssize_t ClientTCPSocket::sendData(const void *buf, size_t len) {
    return send(clientfd, buf, len, 0);
}
