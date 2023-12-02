#ifndef SOCKET_COMNMON_H
#define SOCKET_COMNMON_H

#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

// sem static nao da, um dia pode ser que perceba
// pq pointer e nao reference?
void setIPv4(const std::string &ipv4, struct sockaddr_in *addr);

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
