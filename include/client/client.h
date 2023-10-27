#ifndef CLIENT_H
#define CLIENT_H

#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_transfer.h"
#include <unordered_map> 
#include <thread>
#include "bounded_buffer.h"
#include <chrono>
#include "bitmap.h"

#define CLIENT_INPUT_BUFFER_SIZE 10
#define CLIENT_OUTPUT_BUFFER_SIZE 10

typedef std::chrono::system_clock::duration timestamp_t;

// struct para que as threads possam trabalhar sobre os dados recebidos
struct FS_Transfer_Info {
	timestamp_t timestamp;
	struct sockaddr_in addr;
	FS_Transfer_Packet packet;
};

struct Client {

	Client();
	~Client();

	// main funcs, called in constructor
	void registerWithServer();
	void initUploadLoop();
	void commandParser();

	void readLoop();
	void writeLoop();

	void handleRequest();
	void sendRequest();


	ClientTCPSocket socketToServer;
	NodeUDPSocket udpSocket; // usamos apenas 1 socket para tudo
	std::thread readThread, writeThread;

	BoundedBuffer<FS_Transfer_Info, CLIENT_INPUT_BUFFER_SIZE> inputBuffer; // meter isto na heap??????????????
	BoundedBuffer<FS_Transfer_Info, CLIENT_OUTPUT_BUFFER_SIZE> outputBuffer; // meter isto na heap??????????????

	std::unordered_map<uint64_t, bitMap> blocksPerFile;
};

#endif
