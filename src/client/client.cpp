#include "client.h"

#define SERVER_IP "0.0.0.0"

Client::Client()
        : socketToServer(SERVER_IP), udpSocket(), blocksPerFile() // por default sockets aceitam tudo
{
    // register
    initUploadLoop();
    commandParser();
}

Client::~Client() {
    udpSocket.closeSocket();
}

// read from socket into a buffer, does nothing else
void Client::readLoop() {
    // very bad, copying memory for no reason
    FS_Transfer_Info info;
    while (true) {
        udpSocket.receiveData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);
        inputBuffer.push(info);
    }
}

// write from buffer into socket, does nothing else
void Client::writeLoop() {
    FS_Transfer_Info info;
    while (true) {
        // very bad, copying memory for no reason
        info = outputBuffer.pop();

        udpSocket.sendData(&info.packet, FS_TRANSFER_PACKET_SIZE, &info.addr);
    }
}

void Client::initUploadLoop() {
    // creates a thread that listens in permanently, and writes the data to a buffer
    // another thread tries permanently to write to another buffer
    readThread = std::thread(&Client::readLoop, this);
    writeThread = std::thread(&Client::writeLoop, this);
}

void Client::commandParser() {

}
