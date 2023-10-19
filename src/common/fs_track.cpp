#include "fs_track.h"

uint8_t FS_Track::fs_track_getOpcode() {
    uint8_t opcode = this->opcode_opts;
    return ((opcode & 0xFE) >> 1);
}

uint8_t FS_Track::fs_track_getOpt() {
    uint8_t opt = this->opcode_opts;
    return (opt & 0x01);
}
uint32_t FS_Track::fs_track_getSize() {
    uint32_t ans = 0;
    auto* ptr = static_cast<uint8_t *> (this->size);
    ptr[1] = this->size[0];
    ptr[2] = this->size[1];
    ptr[3] = this->size[2];

    return ans;
}
uint64_t FS_Track::fs_track_getId() {
    return this->id;
}
