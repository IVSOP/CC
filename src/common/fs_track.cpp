#include <string.h>
#include "fs_track.h"

/* Constructors */

FS_Track::FS_Track() {
    this->opcode_opts;
    this->size;
    this->id;
}

FS_Track::~FS_Track() = default;

FS_Track::RegData::RegData(char* filename) {
    int len = strlen(filename);

    memcpy(this->filename, filename, len);

    this->filename[len] = '\0';
}

FS_Track::RegData::~RegData() = default;

FS_Track::IDAssignmentData::IDAssignmentData(char* filename, uint64_t file_id) {
    int len = strlen(filename);

    memcpy(this->filename, filename, len);

    this->filename[len] = '\0';

    this->file_id = file_id;
}

FS_Track::IDAssignmentData::~IDAssignmentData() = default;

FS_Track::UpdateFileBlocksData::UpdateFileBlocksData(uint64_t file_id, uint32_t block_number) {
    this->file_id = file_id;
    this->block_number = block_number;
}

FS_Track::UpdateFileBlocksData::~UpdateFileBlocksData() = default;

FS_Track::PostFileBlocksData::PostFileBlocksData(struct in_addr ip, std::vector<uint32_t> block_numbers) {
    this->ip = ip;
    this->block_numbers = std::vector(block_numbers);
}

FS_Track::PostFileBlocksData::~PostFileBlocksData() = default;

FS_Track::ErrorMessageData::ErrorMessageData(std::string details) {
    this->details = std::string();

    this->details.assign(details);
}

FS_Track::ErrorMessageData::~ErrorMessageData() = default;

/* Constructors end */

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

void FS_Track::Reg_send_data(char* filename) {
    FS_Track::RegData data = FS_Track::RegData(filename);

    // TODO Send data trough socket
}

FS_Track::RegData FS_Track::Reg_read_data() {
    // TODO from socket, read data to struct

    char* filename;

    return FS_Track::RegData(filename);
}

void FS_Track::IDAssignment_send_data(char* filename, uint64_t file_id) {
    FS_Track::IDAssignmentData data = FS_Track::IDAssignmentData(filename, file_id);

    // TODO Send data trough socket
}

FS_Track::IDAssignmentData FS_Track::IDAssignment_read_data() {
    // TODO from socket, read data to struct

    char* filename;
    uint64_t file_id;

    return FS_Track::IDAssignmentData(filename, file_id);
}

void FS_Track::UpdateFileBlocks_send_data(uint64_t file_id, uint32_t block_number) {
    FS_Track::UpdateFileBlocksData data = FS_Track::UpdateFileBlocksData(file_id, block_number);

    // TODO Send data trough socket
}

FS_Track::UpdateFileBlocksData FS_Track::UpdateFileBlocks_read_data() {
    // TODO from socket, read data to struct

    uint64_t file_id;
    uint32_t block_number;

    return FS_Track::UpdateFileBlocksData(file_id, block_number);
}

void FS_Track::PostFileBlocks_send_data(struct in_addr ip, std::vector<uint32_t>& block_numbers) {
    FS_Track::PostFileBlocksData data = FS_Track::PostFileBlocksData(ip, block_numbers);

    // TODO Send data trough socket
}

FS_Track::PostFileBlocksData FS_Track::PostFileBlocks_read_data() {
    // TODO from socket, read data to struct

    struct in_addr ip;
    std::vector<uint32_t> block_numbers;

    return FS_Track::PostFileBlocksData(ip, block_numbers);
}

void FS_Track::ErrorMessage_send_data(std::string& details) {
    FS_Track::ErrorMessageData data = FS_Track::ErrorMessageData(details);

    // TODO Send data trough socket
}

FS_Track::ErrorMessageData FS_Track::ErrorMessage_read_data() {
    // TODO from socket, read data to struct

    std::string details = std::string();

    return FS_Track::ErrorMessageData(details);
}
