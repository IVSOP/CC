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

// TEMPORARIO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
typedef int timestamp_t;

// struct para que as threads possam trabalhar sobre os dados recebidos
struct FS_Transfer_Info {

	// por default fica neste por e dizemos que tem ipv4
	// assim basta mudar o ip para tudo funcionar
	FS_Transfer_Info() {
		addr.sin_family = AF_INET;
		addr.sin_port = htons(UDP_PORT);
	}
    timestamp_t timestamp;
    struct sockaddr_in addr;
    FS_Transfer_Packet packet;
};


// 1 thread le do socket, outra pega nisso e realiza trabalho
// 1 thread escreve para socket, main thread faz trabalho em que escreve para o buffer dela
// main thread faz trabalkho relacio
// como buffers ja estao implementados, no futuro podemos usar thread pool

// pode ser otimizado, ha muitas copias de memoria

struct Client {

    Client();
	Client(const std::string &IPv4); // para debug, set da socket udp interna

    ~Client();

    // main funcs, called in constructor
    void registerWithServer();
    void initUploadLoop();
    void commandParser();

    void readLoop();
    void writeLoop();

    void sendRequest();
	void answerRequestsLoop();

	void sendInfo(const FS_Transfer_Info &info);

    ClientTCPSocket socketToServer;
    NodeUDPSocket udpSocket; // usamos apenas 1 socket para tudo
    std::thread readThread, writeThread, answererThread; // 1 thread loop read, 1 thread loop write, 1 thread loop responder pedidos (faz upload)

	// consider maling this arrays of pointers to avoid so many copies!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BoundedBuffer<FS_Transfer_Info, CLIENT_INPUT_BUFFER_SIZE> inputBuffer; // meter isto na heap??????????????
    BoundedBuffer<FS_Transfer_Info, CLIENT_OUTPUT_BUFFER_SIZE> outputBuffer; // meter isto na heap??????????????

	// sem controlo de concorrencia por agora, nao planeio usar varias threads aqui
	std::unordered_map<uint64_t, bitMap> blocksPerFile;
};

#endif
