#include "TCP_socket.h"
#include <unistd.h>
#include <iostream>
#include "fs_track.h"
#include "fs_track_test.h"

/* Reg Update data */

void setRegUpdateData(FS_Track *data) {
    std::vector<FS_Track::RegUpdateData> dados = std::vector<FS_Track::RegUpdateData>();

    for (int i = 0; i < 5; i++) {
        bitMap blocks = bitMap();

        for (int j = 1; j <= (2 * i) + 1; j++) blocks.emplace_back(j % 2 == 0);

        dados.emplace_back(i, blocks);
    }

    data->regUpdateDataSetData(dados);
}

void readRegUpdateData(FS_Track *data) {
    for (const auto &file: data->regUpdateDataGetData()) {
        std::cout << "File: " << file.file_hash << std::endl;

        for (const auto &block: file.block_numbers) {
            std::cout << block << std::endl;
        }

        std::cout << "-----------------------------------" << std::endl;
    }
}

void testRegUpdateData() {
    testCommunication(setRegUpdateData, readRegUpdateData);
}

/* Post File Blocks */

void setPostFileBlocks(FS_Track *data) {
    std::vector<FS_Track::PostFileBlocksData> dados = std::vector<FS_Track::PostFileBlocksData>();

    for (uint32_t i = 0; i < 3; i++) {
        struct in_addr ip;
        ip.s_addr = i;

        bitMap blocks = bitMap();

        for (uint32_t j = 0; j < 2 * i + 1; j++) {
            blocks.emplace_back(j % 2 == 0);
        }

        dados.emplace_back(ip, blocks);
    }

    data->postFileBlocksSetData(dados);
}

void readPostFileBlocks(FS_Track *data) {
    for (const auto &strct: data->postFileBlocksGetData()) {
        std::cout << "IP: " << strct.ip.s_addr << std::endl;
        for (auto i: strct.block_numbers) {
            std::cout << i << std::endl;
        }
        std::cout << "-----------------------" << std::endl;
    }

}

void testPostFileBlocks() {
    testCommunication(setPostFileBlocks, readPostFileBlocks);
}

/* Error Message */

void setErrorMessage(FS_Track *data) {
    std::string ola = "Hello World";

    data->FS_Track::errorMessageSetData(ola);
}

void readErrorMessage(FS_Track *data) {
    std::cout << data->errorMessageGetData().details << std::endl;
}

void testErrorMessage() {
    testCommunication(setErrorMessage, readErrorMessage);
}

void testCommunication(void set(FS_Track *), void get(FS_Track *)) {

    ServerTCPSocket server = ServerTCPSocket();
    server.socketListen();

    ClientTCPSocket client = ClientTCPSocket(inet_ntoa(server.addr.sin_addr));

    if (fork() == 0) { // client
        auto *data = new FS_Track(1, false, 82);

        set(data);

        std::pair<uint8_t *, uint32_t> buf = data->FS_Track::fsTrackToBuffer();

        client.sendData(buf.first, buf.second);

        delete data;

        _exit(0);
    } else { // server
        char buffer[1500];

        FS_Track *data = new FS_Track();

        ServerTCPSocket::SocketInfo new_connection = server.acceptClient();

        uint32_t bytes, remainBytes;

        bytes = new_connection.receiveData(buffer, 4);

        if (bytes < 4) return;

        data->fsTrackHeaderReadBuffer(buffer, bytes);

        if (data->fsTrackGetOpt() == 1) {
            bytes = new_connection.receiveData(buffer, 8);
            data->fsTrackReadHash(buffer, bytes);
        }

        remainBytes = data->fsTrackGetSize();

        while (remainBytes > 0) {
            bytes = new_connection.receiveData(buffer, std::min((uint32_t) 1500, remainBytes));

            data->setData(buffer, bytes);

            remainBytes -= bytes;
        }
        get(data);
    }
}
