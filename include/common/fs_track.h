
#ifndef TP2_FS_TRACK_H
#define TP2_FS_TRACK_H

#include <cstdint>

class FS_Track{
private:

public:
    struct RegData{

    };

    uint8_t opcode_opts;
    uint8_t size[3];
    uint64_t id;

    uint8_t fs_track_getOpcode();

    uint8_t fs_track_getOpt();

    uint32_t fs_track_getSize();

    uint64_t fs_track_getId();

    void Reg_send_data();
    RegData Reg_read_data();


};

#endif //TP2_FS_TRACK_H
