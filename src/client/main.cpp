//
// Created by kard on 19-10-2023.
//

#include "UDP_socket.h"


void test_udp () {
	NodeUDPSocket serverSocket = NodeUDPSocket("127.15.15.15");

	NodeUDPSocket clientSocket = NodeUDPSocket("127.15.15.16");

	if (fork() == 0) {
		char buff[50] = "Hello World";
		sockaddr_in dest;
		memcpy(&dest, &serverSocket.node_addr, sizeof(struct sockaddr_in));
		clientSocket.sendData(buff, 50, &dest);
		serverSocket.closeSocket();
		clientSocket.closeSocket();
		_exit(0);
	} else {
		char buff2[50];
		sockaddr_in ola;
		serverSocket.receiveData(buff2, 50, &ola);
		printf("%s\n", buff2);
		clientSocket.closeSocket();
		serverSocket.closeSocket();
	}

	serverSocket.closeSocket();
	clientSocket.closeSocket();

}

int main() {
	return 0;
}
