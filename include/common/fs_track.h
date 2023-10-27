
#ifndef TP2_FS_TRACK_H
#define TP2_FS_TRACK_H

#include <cstdint>
#include <netinet/in.h>
#include <vector>
#include <string>

#define FILENAME_SIZE 256
#define SIZE_LENGTH 3

class FS_Track{
private:
    void set_Size(uint32_t);
    uint32_t dataSize;
public:
    FS_Track();
    FS_Track(uint8_t, bool, uint64_t);
    ~FS_Track();

    struct RegUpdateData{
        uint64_t file_hash;
        std::vector<uint32_t> block_numbers;

        RegUpdateData(uint64_t, std::vector<uint32_t>);
        ~RegUpdateData();
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

    uint8_t opcode_opts;
    uint8_t size[SIZE_LENGTH];
    uint64_t hash;
    void* data;

    void fs_track_header_read_buffer(void* buf, ssize_t size);
    void fs_track_set_hash(void* buf, ssize_t size);

    std::pair<uint8_t*, uint32_t> fs_track_to_buffer();

    void set_data(void*, uint32_t);

    uint8_t fs_track_getOpcode();

    uint8_t fs_track_getOpt();

    uint32_t fs_track_getSize();

    uint64_t fs_track_getId();

    void RegUpdateData_set_data(const std::vector<RegUpdateData>& data);
    std::vector<RegUpdateData> RegUpdateData_get_data();

    void PostFileBlocks_set_data(const std::vector<PostFileBlocksData>& data);
    std::vector<PostFileBlocksData> PostFileBlocks_get_data();

    void ErrorMessage_set_data(std::string& details);
    ErrorMessageData ErrorMessage_get_data();
};

#endif //TP2_FS_TRACK_H
