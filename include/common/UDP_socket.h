#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define UDP_PORT 9090

struct NodeUDPSocket {
    int node_fd;
    sockaddr_in node_addr;

    NodeUDPSocket();
    ~NodeUDPSocket();

    ssize_t receiveData(void *buf, size_t len, struct sockaddr_in * from) const;
    ssize_t sendData(const void *buf, size_t len, struct sockaddr_in * from) ;
};

#endif