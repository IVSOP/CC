#include "TCP_socket.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "TCP_socket.h"
#include "thread_pool.h"
#include "threadRAII.h"
#include "fs_track.h"
#include "fs_track_test.h"
#include <thread>
#include "server.h"

#define BUFFER_SIZE (uint32_t) 1500

void read_data(FS_Track* data){
    uint8_t OPCode = data->fs_track_getOpcode();

    if(OPCode == 0 || OPCode == 1) read_RegUpdateData(data);

    else if(OPCode == 3) read_PostFileBlocks(data);

    else if(OPCode == 4) read_ErrorMessage(data);
}

void serveClient(ServerTCPSocket::SocketInfo connection){
    uint8_t* buffer = new uint8_t[BUFFER_SIZE];
    uint32_t bytes, remainBytes;
    FS_Track* data;

    while(true) {
        data = new FS_Track();

        bytes = connection.receiveData(buffer, 4);

        if (bytes == 0) break;

        data->fs_track_header_read_buffer(buffer, bytes);

        if (data->fs_track_getOpt() == 1) {
            bytes = connection.receiveData(buffer, 8);
            data->fs_track_set_hash(buffer, bytes);
        }

        remainBytes = data->fs_track_getSize();

        while (remainBytes > 0) {
            bytes = connection.receiveData(buffer, std::min(BUFFER_SIZE, remainBytes));

            data->set_data(buffer, bytes);

            remainBytes -= bytes;
        }

        read_data(data);

        delete data;
    }

    delete[] (char*) buffer;
}

void acceptClients(){
    ServerTCPSocket server = ServerTCPSocket();
    server.socketListen();

    ServerTCPSocket::SocketInfo new_connection;

    while(true){
        new_connection = server.acceptClient();

        ThreadRAII tr(std::thread(serveClient,new_connection), ThreadRAII::DtorAction::join);

    }
}

// int main () {
//     // printf("%lu\n", offsetof(FS_Transfer_Packet, data));
//     // printf("%lu\n", sizeof(FS_Transfer_Packet));
//     // printf("%lu\n", FS_TRANSFER_PACKET_SIZE);
//     // printf("%lu\n", sizeof(FS_Data) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t));

//     //ThreadPool<ServerTCPSocket::SocketInfo, 10, 8>;

//     acceptClients();

//     return 0;
// }

int main () {
    Server s;
    std::vector<uint32_t> josefina = {125,225,3,526};
    FS_Track::RegUpdateData data = FS_Track::RegUpdateData(1240912490,josefina);
    s.add_new_info(124152,data);
    s.print_map();

}
