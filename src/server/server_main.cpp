#include "TCP_socket.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "TCP_socket.h"
#include "fs_track.h"
#include "fs_track_test.h"

#define BUFFER_SIZE 1500

void read_data(FS_Track* data){
    uint8_t OPCode = data->fs_track_getOpt();

    if(OPCode == 0 || OPCode == 1) read_RegUpdateData(data);

    else if(OPCode == 3) read_PostFileBlocks(data);

    else if(OPCode == 4) read_ErrorMessage(data);
}

int main () {
    // printf("%lu\n", offsetof(FS_Transfer_Packet, data));
    // printf("%lu\n", sizeof(FS_Transfer_Packet));
    // printf("%lu\n", FS_TRANSFER_PACKET_SIZE);
    // printf("%lu\n", sizeof(FS_Data) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t));
    ServerTCPSocket server = ServerTCPSocket();
    server.socketListen();

    ServerTCPSocket::SocketInfo new_connection;

    uint8_t* buffer = new uint8_t[BUFFER_SIZE];
    uint32_t bytes;
    bool first;
    FS_Track* data;

    while(true){
        data = new FS_Track();
        first = true;

        new_connection = server.acceptClient();


        while((bytes = new_connection.receiveData(buffer, BUFFER_SIZE)) > 0){
            if(first) {
                data->fs_track_read_buffer(buffer, bytes);
                first = false;
            }
            else data->set_data(buffer, bytes);
        }

        read_data(data);
    }

    delete[] (char*) buffer;

    return 0;
}
