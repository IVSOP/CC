#include "client.h"
#include "fs_track.h"

#define SERVER_IP "0.0.0.0"

Client::Client() // por default sockets aceitam tudo
: socketToServer(SERVER_IP), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile()
{
    // register
    initUploadLoop();
    commandParser();
}

Client::Client(const std::string &IPv4)
: socketToServer(IPv4), udpSocket(), inputBuffer(), outputBuffer(), blocksPerFile()
{
    // register
    initUploadLoop();
    commandParser();
}

Client::~Client() {
    udpSocket.closeSocket();
}

void printPacket(FS_Transfer_Info& info) {
	printf("checksum:%u (it is %s), opc: %u size: %u id: %lu\ndata as string: %s\n", info.packet.checksum,
		info.packet.calculateChecksum() == info.packet.checksum ? "correct" : "wrong",
		info.packet.getOpcode(), info.packet.getSize(), info.packet.id, reinterpret_cast<char *>(&info.packet.data));
}

// read from socket into a buffer, does nothing else
void Client::readLoop() {
	puts("Init reading thread");
	// very bad, copying memory for no reason
	FS_Transfer_Info info;
	while (true) {
		udpSocket.receiveData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);
		// falta timestamp!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		inputBuffer.push(info);

		puts("packet received");
	}
}

// write from buffer into socket, does nothing else
void Client::writeLoop() {
	puts("Init writing thread");
	FS_Transfer_Info info;
	while (true) {
		// very bad, copying memory for no reason
		info = outputBuffer.pop();

        udpSocket.sendData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);

		puts("packet sent");
    }
}

// loops trying to answer a request
void Client::answerRequestsLoop() {
	FS_Transfer_Info info;
	while (true) {
		info = inputBuffer.pop();

		// do work.......
		printPacket(info);
	}
}

void Client::sendInfo(const FS_Transfer_Info &info) {
	outputBuffer.push(info);
}

void Client::initUploadLoop() {
    // creates a thread that listens in permanently, and writes the data to a buffer
    // another thread tries permanently to write to another buffer
	// another thread works on the things read into the input buffer

	// meter threads num inicializador em vez de dar detach??
    readThread = std::thread(&Client::readLoop, this);
	readThread.detach();
    writeThread = std::thread(&Client::writeLoop, this);
	writeThread.detach();
	answererThread = std::thread(&Client::answerRequestsLoop, this);
	answererThread.detach();
}

void Client::commandParser() {

}

void Client::registerWithServer(ClientTCPSocket socket){
    std::vector<FS_Track::RegUpdateData> data = std::vector<FS_Track::RegUpdateData>();
    std::vector<uint32_t> blocks = std::vector<uint32_t>();

    for(const auto& pair: this->blocksPerFile){
        blocks.clear();
        for (uint32_t i = 0; i < pair.second.size(); i++){
            if(pair.second.at(i)) blocks.emplace_back(i);
        }

        data.emplace_back(pair.first, blocks);
    }

    FS_Track::send_reg_message(socket, data);
}
