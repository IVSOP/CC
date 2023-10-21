//
// Created by kard on 19-10-2023.
//

#include "TCP_socket.h"
#include <unistd.h>
#include "fs_transfer.h"

int main () {
	printf("%lu\n", offsetof(FS_Transfer_Packet, data));
	printf("%lu\n", sizeof(FS_Transfer_Packet));
	printf("%lu\n", sizeof(FS_Data) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t));
	ServerTCPSocket server = ServerTCPSocket();
	server.socketListen();

	ClientTCPSocket client = ClientTCPSocket(inet_ntoa(server.addr.sin_addr));

	if (fork() == 0) { // client
		char buff[50] = "Hello World";
		
		client.sendData(buff, 50);
		_exit(0);
	} else { // server
		char buff2[50];
		
		ServerTCPSocket::SocketInfo new_connection = server.acceptClient();

		new_connection.receiveData(buff2, 50);

		printf("%s\n", buff2);
	}
	return 0;
}