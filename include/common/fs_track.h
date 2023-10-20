
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
public:
    FS_Track(uint8_t, bool, uint8_t[], uint64_t, void*);
    ~FS_Track();

    struct RegData{
        char filename[FILENAME_SIZE];

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

    uint8_t opcode_opts;
    uint8_t size[SIZE_LENGTH];
    uint64_t id;
    void* data;


    uint8_t fs_track_getOpcode();

    uint8_t fs_track_getOpt();

    uint32_t fs_track_getSize();

    uint64_t fs_track_getId();

    void Reg_set_data(const std::vector<RegData>& files_name);
    std::vector<FS_Track::RegData> Reg_get_data();

    void IDAssignment_set_data(const std::vector<IDAssignmentData>& data);
    std::vector<IDAssignmentData> IDAssignment_get_data();

    void UpdateFileBlocks_set_data(const std::vector<UpdateFileBlocksData>& data);
    std::vector<UpdateFileBlocksData> UpdateFileBlocks_get_data();

    void PostFileBlocks_set_data(const std::vector<PostFileBlocksData>& data);
    std::vector<PostFileBlocksData> PostFileBlocks_get_data();

    void ErrorMessage_set_data(std::string& details);
    ErrorMessageData ErrorMessage_get_data();
};

#endif //TP2_FS_TRACK_H
