#include "UDP_socket.h"
#include "TCP_socket.h"
#include "fs_transfer.h"
#include "fs_track.h"
#include "fs_track_test.h"
#include "checksum.h"
#include "errors.h"
#include "client.h"
#include "socket_common.h"

int main(int argc, char *argv[]) {
    if(argc != 2 && argc != 4) { // !!!! mudar para 3, 4 e para testar fora do core
        print_error("Not enough arguments");
        return -1;
    }
    if (argc == 2) {
        Client client = Client(argv[1]);
    }
    if (argc == 4) {
        Client client = Client(argv[1], std::string(argv[2]),std::string(argv[3]));
    }
    std::cout << "The end" << std::endl;
}


