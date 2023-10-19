#include "TCP_socket.h"

#include <unistd.h>
#include <iostream>

ServerTCPSocket::ServerTCPSocket() {
	if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		std::cerr << "Error creating socket" << std::endl;
        exit(EXIT_FAILURE);
	}

    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->addr.sin_port = htons(TCP_PORT);

	int opt = 1;
	// Forcefully attaching socket to the port 9090
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Error setting socket options" << std::endl;
        exit(EXIT_FAILURE);
    }

	if (bind(serverfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        exit(EXIT_FAILURE);
    }	
}

ServerTCPSocket::~ServerTCPSocket() {
	close(serverfd);
}

void ServerTCPSocket::socketListen() const {
	if (listen(serverfd, MAX_CONNECTIONS_IN_QUEUE) < 0) {
        std::cerr << "Error listening for connections" << std::endl;
        exit(EXIT_FAILURE);
    }
}

// returns new socket fd
// NOT responsibility of this class to close new socket
ServerTCPSocket::SocketInfo ServerTCPSocket::acceptClient() const {
	SocketInfo socketInfo;
	int clientSocket = accept(serverfd, &socketInfo.addr, nullptr);
    if (clientSocket < 0) {
        std::cerr << "Error accepting connection" << std::endl;
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








ClientTCPSocket::ClientTCPSocket(const std::string &ipv4) {
	if (inet_pton(AF_INET, ipv4.c_str(), &addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        exit(EXIT_FAILURE);
    }

	this->addr.sin_family = AF_INET;
	this->addr.sin_port = htons(TCP_PORT);

	int status;
	if ((status = connect(clientfd, (struct sockaddr*)&addr, sizeof(addr))) < 0) {
        std::cerr << "Connection Failed" << std::endl;
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
