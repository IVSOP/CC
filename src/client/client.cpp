#include "client.h"

#define SERVER_IP "0.0.0.0"

Client::Client()
: socketToServer(SERVER_IP), udpSocket(), thread() // por default sockets aceitam tudo
{
	// register
	initUploadLoop();
	commandParser();
}

Client::~Client() {
	udpSocket.closeSocket();
}

void Client::uploadLoop() {

}

void Client::initUploadLoop() {
	// creates a thread that listens in permanently, and writes the data to the struct
	// another thread could then be called to handle the request, while reads keep going,
	// but we want to reuse the same data struct for now, for simplicity
	// if this is too slow, will change
	thread = std::thread(&Client::uploadLoop, this);
}

void Client::commandParser() {

}
