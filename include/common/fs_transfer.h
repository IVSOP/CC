//
// Created by mafba on 19/10/2023.
//

#ifndef TP2_FS_TRANSFER_CPP_H
#define TP2_FS_TRANSFER_CPP_H


#include <cstdint>

class FS_Transfer{
private:

public:

    uint32_t checksum;
    uint32_t opc_size;
    uint64_t id;

    uint32_t fs_track_getChecksum();
    uint8_t fs_track_getOpcode();
    uint32_t fs_track_getSize();
    uint64_t fs_track_getId();
    
};

#endif //TP2_FS_TRANSFER_CPP_H
