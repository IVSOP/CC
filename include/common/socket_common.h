#ifndef SOCKET_COMNMON_H
#define SOCKET_COMNMON_H

#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

// sem static nao da, um dia pode ser que perceba
// pq pointer e nao reference?
static void setIPv4(const std::string &ipv4, struct sockaddr_in *addr) {
    if (inet_pton(AF_INET, ipv4.c_str(), &addr->sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        exit(EXIT_FAILURE);
    }
}

struct Ip {
    struct sockaddr_in addr;

    Ip() = default;

    Ip(struct sockaddr_in addr){
        this->addr = addr;
    }

    bool operator==(const struct Ip &o) {
        return addr.sin_addr.s_addr == o.addr.sin_addr.s_addr;
    }

    bool operator<(const struct Ip &o) {
        return addr.sin_addr.s_addr < o.addr.sin_addr.s_addr;
    }
};

#endif
