#include "TCP_socket.h"
#include <unistd.h>
#include <iostream>
#include "fs_track.h"
#include "fs_track_test.h"

/* Reg Update data */

void set_UpdateFileBlocks(FS_Track* data){
    std::vector<FS_Track::RegUpdateData> dados = std::vector<FS_Track::RegUpdateData>();

    for(int i = 0; i < 5; i++){
        std::vector<uint32_t> blocks = std::vector<uint32_t>();

        for(int j = 1; j <= (2*i)+1; j++) blocks.emplace_back(j);

        dados.emplace_back(i, blocks);
    }

    data->RegUpdateData_set_data(dados);
}

void read_UpdateFileBlocks(FS_Track* data){
    for(const auto& file : data->RegUpdateData_get_data()){
        std::cout << "File: " << file.file_id << std::endl;

        for(const auto& block : file.block_numbers){
            std::cout << block << std::endl;
        }

        std::cout << "-----------------------------------" << std::endl;
    }
}

void testUpdateFileBlocks(){
    testCommunication(set_UpdateFileBlocks, read_UpdateFileBlocks);
}

/* Post File Blocks */

void set_PostFileBlocks(FS_Track* data){
    std::vector<FS_Track::PostFileBlocksData> dados = std::vector<FS_Track::PostFileBlocksData>();

    for(uint32_t i = 0; i < 3; i++){
        struct in_addr ip;
        ip.s_addr = i;

        std::vector<uint32_t> blocks = std::vector<uint32_t>();

        for(uint32_t j = 0; j < 2*i + 1; j++){
            blocks.emplace_back(j);
        }

        dados.emplace_back(ip, blocks);
    }

    data->PostFileBlocks_set_data(dados);
}

void read_PostFileBlocks(FS_Track* data){
    for(const auto& strct : data->PostFileBlocks_get_data()){
        std::cout << "IP: " << strct.ip.s_addr << std::endl;
        for(auto i : strct.block_numbers){
            std::cout << i << std::endl;
        }
        std::cout << "-----------------------" << std::endl;
    }

}

void testPostFileBlocks(){
    testCommunication(set_PostFileBlocks, read_PostFileBlocks);
}

/* Error Message */

void set_ErrorMessage(FS_Track* data){
    std::string ola = "Hello World";

    data->FS_Track::ErrorMessage_set_data(ola);
}

void read_ErrorMessage(FS_Track* data){
    std::cout << data->ErrorMessage_get_data().details << std::endl;
}

void testErrorMessage(){
    testCommunication(set_ErrorMessage, read_ErrorMessage);
}

void testCommunication (void set(FS_Track*), void get(FS_Track*)) {

    ServerTCPSocket server = ServerTCPSocket();
    server.socketListen();

    ClientTCPSocket client = ClientTCPSocket(inet_ntoa(server.addr.sin_addr));

    if (fork() == 0) { // client
        auto* data = new FS_Track(1, false, 82);

        set(data);

        std::pair<char*, uint32_t> buf = data->FS_Track::fs_track_to_buffer();

        client.sendData(buf.first, buf.second);

        delete data;

        _exit(0);
    } else { // server
        char buf[1500];

        FS_Track* data = new FS_Track();

        ServerTCPSocket::SocketInfo new_connection = server.acceptClient();

        ssize_t bytes = new_connection.receiveData(buf, 1500);

        data->fs_track_read_buffer(buf, bytes);

        printf("%d, %d, %u, %lu\n", data->fs_track_getOpcode(), data->fs_track_getOpt() ? 1 : 0, data->fs_track_getSize(), data->fs_track_getId());

        get(data);
    }
}
