#include "TCP_socket.h"
#include <unistd.h>
#include <iostream>
#include "fs_track.h"
#include "fs_transfer.h"
#include "fs_track_test.h"

/* Reg Data */

void set_RegData(FS_Track* data){
    std::vector<FS_Track::RegData> dados = std::vector<FS_Track::RegData>();

    char filename[FILENAME_SIZE];

    for(int i = 0; i < 10; i++){
        snprintf(filename, FILENAME_SIZE, "This is file %d.", i);
        dados.emplace_back(filename);
    }

    data->Reg_set_data(dados);
}

void read_RegData(FS_Track* data){
    for(const auto& file : data->Reg_get_data()){
        std::cout << "File with name: " << file.filename << std::endl;
        std::cout << std::endl;
    }
}

void testRegData(){
    testCommunication(set_RegData, read_RegData);
}

/* ID Assignment */

void set_IDAssignment(FS_Track* data){
    std::vector<FS_Track::IDAssignmentData> dados = std::vector<FS_Track::IDAssignmentData>();

    char filename[FILENAME_SIZE];

    for(int i = 0; i < 5; i++){
        snprintf(filename, FILENAME_SIZE, "This is file %d.", i);
        dados.emplace_back(filename, i);
    }

    data->IDAssignment_set_data(dados);
}

void read_IDAssignment(FS_Track* data){
    for(const auto& file : data->IDAssignment_get_data()){
        std::cout << "File with id: " << file.file_id << std::endl;
        std::cout << file.filename << std::endl;
        std::cout << std::endl;
    }
}

void testIDAssignment(){
    testCommunication(set_IDAssignment, read_IDAssignment);
}

/* Update File Blocks */

void set_UpdateFileBlocks(FS_Track* data){
    std::vector<FS_Track::UpdateFileBlocksData> dados = std::vector<FS_Track::UpdateFileBlocksData>();

    for(int i = 0; i < 5; i++){
        dados.emplace_back(i, (2*i)+1);
    }

    data->UpdateFileBlocks_set_data(dados);
}

void read_UpdateFileBlocks(FS_Track* data){
    for(const auto& file : data->UpdateFileBlocks_get_data()){
        std::cout << file.file_id << ": " << file.block_number << std::endl;
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
