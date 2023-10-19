
#ifndef TP2_FS_TRACK_H
#define TP2_FS_TRACK_H

#include <cstdint>
#include <netinet/in.h>
#include <vector>
#include <string>

#define FILENAME_SIZE 256

class FS_Track{
private:

public:
    FS_Track();
    ~FS_Track();

    struct RegData{
        char filename[FILENAME_SIZE]{};

        RegData(char*);
        ~RegData();
    };

    struct IDAssignmentData{
        char filename[FILENAME_SIZE];
        uint64_t file_id;

        IDAssignmentData(char*, uint64_t);
        ~IDAssignmentData();
    };

    struct UpdateFileBlocksData{
        uint64_t file_id;
        uint32_t block_number;

        UpdateFileBlocksData(uint64_t, uint32_t);
        ~UpdateFileBlocksData();
    };

    struct PostFileBlocksData{
        struct in_addr ip;
        std::vector<uint32_t> block_numbers;

        PostFileBlocksData(struct in_addr, std::vector<uint32_t>);
        ~PostFileBlocksData();
    };

    struct ErrorMessageData{
        std::string details;

        ErrorMessageData(std::string);
        ~ErrorMessageData();
    };

    uint8_t opcode_opts{};
    uint8_t size[3]{};
    uint64_t id{};

    uint8_t fs_track_getOpcode();

    uint8_t fs_track_getOpt();

    uint32_t fs_track_getSize();

    uint64_t fs_track_getId();

    void Reg_send_data(char* filename);
    RegData Reg_read_data();

    void IDAssignment_send_data(char* filename, uint64_t id);
    IDAssignmentData IDAssignment_read_data();

    void UpdateFileBlocks_send_data(uint64_t id, uint32_t block_number);
    UpdateFileBlocksData UpdateFileBlocks_read_data();

    void PostFileBlocks_send_data(struct in_addr ip, std::vector<uint32_t>& block_numbers);
    PostFileBlocksData PostFileBlocks_read_data();

    void ErrorMessage_send_data(std::string& details);
    ErrorMessageData ErrorMessage_read_data();
};

#endif //TP2_FS_TRACK_H
