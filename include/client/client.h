#ifndef CLIENT_H
#define CLIENT_H

#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_transfer.h"
#include <unordered_map> 
#include <thread>

struct Client {

	Client();
	~Client();

	// main funcs, called in constructor
	void registerWithServer();
	void initUploadLoop();
	void commandParser();

	void uploadLoop();


	ClientTCPSocket socketToServer;
	// 1 socket para receber e outra para enviar
	// isto e porque os receives bloqueiam
	// outra solucao seria 2 threads na mesma socket, mas e mais confuso
	NodeUDPSocket inputSocket;
	NodeUDPSocket outputSocket;
	std::thread thread;
};

#endif
