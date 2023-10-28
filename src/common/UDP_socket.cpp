#include "UDP_socket.h"
#include <unistd.h>
#include "socket_common.h"
#include "errors.h"

#define MAXLINE 1024

NodeUDPSocket::NodeUDPSocket()
        : node_fd(-1) {
    //Create socket fd
    if ((node_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error("Error creating UDP socket");
        exit(EXIT_FAILURE);
    }

    //Fill server info
    this->node_addr.sin_family = AF_INET; // IPv4
    this->node_addr.sin_addr.s_addr = INADDR_ANY;
    this->node_addr.sin_port = htons(UDP_PORT);

    // Bind the socket with the server address 
    if (bind(node_fd, (const struct sockaddr *) &node_addr,
             sizeof(node_addr)) < 0) {
        print_error("Error binding UDP socket");
        exit(EXIT_FAILURE);
    }
}

NodeUDPSocket::NodeUDPSocket(const std::string &ipv4)
        : node_fd(-1) {
    //Create socket fd
    if ((node_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error("Error creating UDP socket");
        exit(EXIT_FAILURE);
    }

    //Fill server info
    this->node_addr.sin_family = AF_INET; // IPv4

    setIPv4(ipv4, &node_addr);

    this->node_addr.sin_port = htons(UDP_PORT);


    // Bind the socket with the server address 
    if (bind(node_fd, (const struct sockaddr *) &node_addr, sizeof(node_addr)) < 0) {
        print_error("Error binding");
        exit(EXIT_FAILURE);
    }
}

NodeUDPSocket::~NodeUDPSocket() {
    close(node_fd);
}

int NodeUDPSocket::closeSocket() {
    return close(node_fd);
}


ssize_t NodeUDPSocket::receiveData(void *buf, size_t len, struct sockaddr_in *from) const {
    socklen_t l = sizeof(struct sockaddr_in);
    return recvfrom(node_fd, buf, len, MSG_WAITALL, (struct sockaddr *) from, &l);
}

//If you're sending an initial request, or sending a datagram in response to some other event (like user input, or a timeout) then you should not set the MSG_CONFIRM flag.
ssize_t NodeUDPSocket::sendData(const void *buf, size_t len, struct sockaddr_in *to) {
    socklen_t l = sizeof(struct sockaddr_in);
    return sendto(node_fd, buf, len, 0, (const struct sockaddr *) to, l);
};

