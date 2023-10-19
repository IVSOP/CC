#include "fs_transfer.h"

//constexpr 
uint32_t FS_Transfer::fs_track_getChecksum() {
    return this ->checksum;
}

uint8_t FS_Transfer::fs_track_getOpcode() {
    uint8_t fstByte = static_cast<uint8_t> (this->opc_size);
    fstByte = (fstByte & 0xC0) >> 6;
    return fstByte;
}

uint32_t FS_Transfer::fs_track_getSize() {
    uint32_t size = this->opc_size;
    size = (size & 0x3F);
    return size;
}

uint64_t FS_Transfer::fs_track_getId() {
    return this->id;
}

