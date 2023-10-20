//
// Created by kard on 19-10-2023.
//

#include "UDP_socket.h"

int main () {
	NodeUDPSocket serverSocket = NodeUDPSocket("127.15.15.15");

	// manhosidade para testar, saco o IP da socket de cima
	NodeUDPSocket clientSocket = NodeUDPSocket("127.15.15.16");

	if (fork() == 0) {
		char buff[50] = "Hello World";
		sockaddr_in dest;
		memcpy(&dest, &serverSocket.node_addr, sizeof(struct sockaddr_in));
		clientSocket.sendData(buff, 50, &dest);
		_exit(0);
	} else {
		char buff2[50];
		sockaddr_in ola;
		serverSocket.receiveData(buff2, 50, &ola);
		printf("%s\n", buff2);
	}
	return 0;
}
