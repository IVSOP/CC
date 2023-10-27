#ifndef CLIENT_H
#define CLIENT_H

#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_transfer.h"
#include <unordered_map> 
#include <thread>
#include "bounded_buffer.h"

struct Client {

	Client();
	~Client();

	// main funcs, called in constructor
	void registerWithServer();
	void initUploadLoop();
	void commandParser();

	void uploadLoop();


	ClientTCPSocket socketToServer;
	NodeUDPSocket udpSocket; // usamos apenas 1 socket para tudo
	std::thread thread;
};

#endif
