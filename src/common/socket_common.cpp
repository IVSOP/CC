#include "socket_common.h"

void setIPv4(const std::string &ipv4, struct sockaddr_in *addr) {
    if (inet_pton(AF_INET, ipv4.c_str(), &addr->sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        exit(EXIT_FAILURE);
    }
}
