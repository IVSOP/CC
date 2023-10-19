//
// Created by mafba on 19/10/2023.
//

#ifndef TP2_FS_TRANSFER_CPP_H
#define TP2_FS_TRANSFER_CPP_H


#include <cstdint>

class FS_Transfer{
private:

public:
    uint8_t opcode_opts;
    uint8_t size[3];
    uint64_t id;

    uint8_t fs_track_getOpcode(FS_Transfer header);
    uint8_t fs_track_getOpt(FS_Transfer header);
    uint32_t fs_track_getSize(FS_Transfer header);
    uint64_t fs_track_getId(FS_Transfer header);
};

#endif //TP2_FS_TRANSFER_CPP_H
