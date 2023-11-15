#ifndef CLIENT_H
#define CLIENT_H

#include <unordered_map>
#include <thread>
#include <chrono>
#include <fstream>
#include "TCP_socket.h"
#include "UDP_socket.h"
#include "fs_transfer.h"
#include "bounded_buffer.h"
#include "bitmap.h"
#include "checksum.h"

#define CLIENT_INPUT_BUFFER_SIZE 10
#define CLIENT_OUTPUT_BUFFER_SIZE 10

// struct para que as threads possam trabalhar sobre os dados recebidos
struct FS_Transfer_Info {

	// por default fica neste por e dizemos que tem ipv4
	// assim basta mudar o ip para tudo funcionar
	FS_Transfer_Info() {
		addr.sin_family = AF_INET;
		addr.sin_port = htons(UDP_PORT);
	}
    time_t timestamp;
    struct sockaddr_in addr;
    FS_Transfer_Packet packet;

    constexpr const FS_Transfer_Packet& getTransferPacket() const {
        return packet;
    }

    void setTimestamp(time_t& timsestamp);
    void setAdrr(struct sockaddr_in&) const;
    void setPacket(const FS_Transfer_Packet *, ssize_t size);

};


// 1 thread le do socket, outra pega nisso e realiza trabalho
// 1 thread escreve para socket, main thread faz trabalho em que escreve para o buffer dela
// main thread faz trabalkho relacio
// como buffers ja estao implementados, no futuro podemos usar thread pool

// pode ser otimizado, ha muitas copias de memoria

struct Client {

    using FS_Transfer_Packet_handler = void (Client::*) (FS_Transfer_Info&); //typedef para as funções da dispatch table

    Client();
    Client(char* dir);
	Client(char* dir, const std::string &IPv4); // para set do server IP

    ~Client();

    // main funcs, called in constructor
    void registerWithServer();
    void initUploadLoop();
    void commandParser(const char * dir);
    void assignDispatchTable();

    void readLoop();
    void writeLoop();

    void sendRequest();
	void answerRequestsLoop();

	void sendInfo(FS_Transfer_Info &info);

    //parse opcode funcs
    void ReqBlockData(FS_Transfer_Info& info);
    void RespondBlockData(FS_Transfer_Info& info);

    void regNewFile(const char* dir, const char* fn, size_t size);
    ssize_t getFileBlock(uint64_t fileHash, uint32_t blockID, char * buffer);
    void writeFileBlock(uint64_t fileHash, uint32_t blockN, char * buffer, size_t size);

	void wrongChecksum(const FS_Transfer_Info &info) {
		// ....................
	}

    void regDirectory(char* directory);
    void regFile(const char* dir, char* fn);
    void weightedRoundRobin(uint64_t hash, std::unordered_map<uint32_t , std::vector<uint32_t>>& available_nodes, std::unordered_map<uint32_t, uint32_t>& nodes_requested_blocks);
    uint32_t selectNode(std::vector<uint32_t>& block_nodes, std::unordered_map<uint32_t, uint32_t>& nodes_requested_blocks);

    ClientTCPSocket socketToServer;
    NodeUDPSocket udpSocket; // usamos apenas 1 socket para tudo
    std::thread readThread, writeThread, answererThread; // 1 thread loop read, 1 thread loop write, 1 thread loop responder pedidos (faz upload)

	// consider maling this arrays of pointers to avoid so many copies!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BoundedBuffer<FS_Transfer_Info, CLIENT_INPUT_BUFFER_SIZE> inputBuffer; // meter isto na heap??????????????
    BoundedBuffer<FS_Transfer_Info, CLIENT_OUTPUT_BUFFER_SIZE> outputBuffer; // meter isto na heap??????????????

	// sem controlo de concorrencia por agora, nao planeio usar varias threads aqui
    //track blocks that each file has
	std::unordered_map<uint64_t, bitMap> blocksPerFile;
    // juntar ao array de cima maybe
    std::unordered_map<uint64_t, uint32_t> currentBlocksInEachFile; 
    
    //track file descriptors for each filehash
    std::unordered_map<uint64_t, FILE*> fileDescriptorMap;

    //dispatch table
    std::unordered_map<uint8_t,FS_Transfer_Packet_handler> dispatchTable;

    //inicializados só uma vez, alterados com o decorrer
    FS_Transfer_Info dataFinal;
    FS_Transfer_Packet dataPacket;
    BlockSendData blockSend;
};

#endif
