#include <cstring>

#include "fs_track.h"
#include "TCP_socket.h"
#include "convert_types.h"

/* Constructors */

FS_Track::FS_Track(uint8_t opcode, bool opts, uint64_t id) {
    this->opcode_opts = ((opcode << 1) + opts);
    FS_Track::set_Size(0);
    this->id = id;
    this->data = nullptr;
}

FS_Track::~FS_Track(){
    delete[] (char*) this->data;
}

FS_Track::RegData::RegData(char *filename) {
    int len = strlen(filename);

    memcpy(this->filename, filename, len);

    this->filename[len] = '\0';
}

FS_Track::RegData::~RegData() = default;

FS_Track::IDAssignmentData::IDAssignmentData(char *filename, uint64_t file_id) {
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
    this->ip = in_addr();
    this->ip.s_addr = ip.s_addr;
    this->block_numbers = std::vector(block_numbers);
}

FS_Track::PostFileBlocksData::~PostFileBlocksData() = default;

FS_Track::ErrorMessageData::ErrorMessageData(std::string details) {
    this->details = std::string();

    this->details.assign(details);
}

FS_Track::ErrorMessageData::~ErrorMessageData() = default;

/* Constructors end */

/* Private Functions */

void FS_Track::set_Size(uint32_t bytes) {
    this->size[0] = (std::uint8_t) ((bytes >> 16) & 0xFF);
    this->size[1] = (std::uint8_t) ((bytes >> 8) & 0xFF);
    this->size[2] = (std::uint8_t) ((bytes) & 0xFF);
}

void FS_Track::set_data(void* buf, uint32_t size){
    this->data = new char[size];
    memcpy(this->data, buf, size);

    this->set_Size(size);
}

/* private Functions end */

void FS_Track::fs_track_read_buffer(void* buf, ssize_t size){
    if(size < 4){
        perror("No data to read.");

        return;
    }

    char* buffer = (char*) buf;

    uint32_t curPos = 0;

    memcpy(&(this->opcode_opts), buffer, 1);
    ++curPos;

    memcpy(this->size, &buffer[1], 3);
    curPos += 3;

    bool hasID = this->fs_track_getOpt() == 1;
    uint32_t dataSize = this->fs_track_getSize();

    if(hasID){
        memcpy(&(this->id), &buffer[curPos], 8);
        curPos += 8;
    }

    if(dataSize > 0) this->set_data(&buffer[curPos], dataSize);
}

std::pair<char*, uint32_t> FS_Track::fs_track_to_buffer(){
    uint32_t dataSize = this->fs_track_getSize();
    uint32_t bufferSize = 4 + dataSize;

    bool hasID = this->fs_track_getOpt() == 1;
    if(hasID) bufferSize += 8;

    char* buffer = (char*) new char[bufferSize];

    uint32_t curPos = 0;

    memcpy(&buffer[curPos], &this->opcode_opts, 1);
    ++curPos;

    memcpy(&buffer[curPos], this->size, 3);
    curPos += 3;

    if(hasID) {
        memcpy(&buffer[curPos], &this->id, 8);
        curPos += 8;
    }

    if(dataSize > 0){
        memcpy(&buffer[curPos], this->data, dataSize);
    }

    return std::pair<char*, uint32_t>(buffer, bufferSize);
}

uint8_t FS_Track::fs_track_getOpcode() {
    uint8_t opcode = this->opcode_opts;
    return ((opcode & 0xFE) >> 1);
}

uint8_t FS_Track::fs_track_getOpt() {
    uint8_t opt = this->opcode_opts;
    return (opt & 0x01);
}

uint32_t FS_Track::fs_track_getSize() {
    return ((this->size[0] << 16) + (this->size[1] << 8) + (this->size[2]));
}

uint64_t FS_Track::fs_track_getId() {
    return this->id;
}



/**
 * Sets FS_Track data with desired RegData structs
 * @param data
 */
void FS_Track::Reg_set_data(const std::vector<RegData> &data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;

    push_uint32_into_vector_uint8(serializedData, data.size());

    // For each struct
    for (const auto &fileData: data) {

        // Filename len
        uint32_t len = (uint32_t) strnlen(fileData.filename, FILENAME_SIZE);
        push_uint32_into_vector_uint8(serializedData, len);

        // Serialize filename byte by byte
        for (uint32_t i = 0; i < len; i++) {
            serializedData->emplace_back(fileData.filename[i]);
        }

        // Update data's total bytes
        totalBytes += (4 + len);
    }

    // Update FS_Track
    FS_Track::set_data(serializedData->data(), totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into RegData structs
 * @return
 */
std::vector<FS_Track::RegData> FS_Track::Reg_get_data() {
    // Serialized data
    char* serializedData = (char*) this->data;

    uint32_t i = 0;
    uint32_t len = vptr_to_uint32(serializedData, &i);

    if(len == 0) {
        perror("No data received");

        return {};
    }

    std::vector<FS_Track::RegData> deserializedData = std::vector<FS_Track::RegData>();

    uint32_t filename_len;
    char filename[FILENAME_SIZE];

    // For each byte of data
    for (uint32_t k = 0; k < len; k++) {
        // Get filename len
        filename_len = vptr_to_uint32(serializedData, &i);

        // Get file name
        for (uint32_t j = 0; j < filename_len; j++) filename[j] = (char) serializedData[i++];

        // Store recovered data
        deserializedData.emplace_back(filename);
    }

    return deserializedData;
}

/**
 * Sets FS_Track data with desired IDAssignmentData structs
 * @param data
 */
void FS_Track::IDAssignment_set_data(const std::vector<IDAssignmentData> &data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;

    push_uint32_into_vector_uint8(serializedData, data.size());

    // For each struct
    for (const auto &fileData: data) {

        push_uint64_into_vector_uint8(serializedData, fileData.file_id);

        // Filename len
        uint32_t len = (uint32_t) strnlen(fileData.filename, FILENAME_SIZE);
        push_uint32_into_vector_uint8(serializedData, len);

        // Serialize filename byte by byte
        for (uint32_t i = 0; i < len; i++) {
            serializedData->emplace_back(fileData.filename[i]);
        }

        // Update data's total bytes
        totalBytes += (8 + 4 + len);
    }

    // Update FS_Track
    this->set_data(serializedData->data(), totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into IDAssignment_get_data structs
 * @return
 */
std::vector<FS_Track::IDAssignmentData> FS_Track::IDAssignment_get_data() {
    // Serialized data
    char* serializedData = (char*) this->data;

    uint32_t i = 0;
    uint32_t len = vptr_to_uint32(serializedData, &i);

    if(len == 0) {
        perror("No data received");

        return {};
    }

    std::vector<FS_Track::IDAssignmentData> deserializedData = std::vector<FS_Track::IDAssignmentData>();

    uint64_t fileId;
    uint32_t filename_len;
    char filename[FILENAME_SIZE];

    // For each byte of data
    for (uint32_t k = 0; k < len; k++) {
        // Get file id
        fileId = vptr_to_uint64(serializedData, &i);

        // Get filename len
        filename_len = vptr_to_uint32(serializedData, &i);

        // Get file name
        for (uint32_t j = 0; j < filename_len; j++) filename[j] = (char) serializedData[i++];

        // Store recovered data
        deserializedData.emplace_back(filename, fileId);
    }

    return deserializedData;
}

// TODO No update não podiamos usar a mesma estrutura do post?

/**
 * Sets FS_Track data with desired UpdateFileBlocksData structs
 * @param data
 */
void FS_Track::UpdateFileBlocks_set_data(const std::vector<UpdateFileBlocksData>& data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;
    push_uint32_into_vector_uint8(serializedData, (uint32_t) data.size());

    // For each struct
    for(const auto& fileBlock : data){
        // Serialized file Id
        push_uint64_into_vector_uint8(serializedData, fileBlock.file_id);

        // Serialize block number
        push_uint32_into_vector_uint8(serializedData, fileBlock.block_number);

        totalBytes += 12;
    }

    this->set_data(serializedData->data(), totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into UpdateFileBlocksData structs
 * @return
 */
std::vector<FS_Track::UpdateFileBlocksData> FS_Track::UpdateFileBlocks_get_data() {
    // Serialized data
    char* serializedData = (char*) this->data;

    uint32_t i = 0;
    uint32_t len = vptr_to_uint32(serializedData, &i);

    if(len == 0) {
        perror("No data received");

        return {};
    }

    std::vector<FS_Track::UpdateFileBlocksData> deserializedData = std::vector<FS_Track::UpdateFileBlocksData>();

    uint64_t file_id;
    uint32_t block_number;

    for(uint32_t j = 0; j < len; j++){
        // Deserialize file id
        file_id = vptr_to_uint64(serializedData, &i);

        // Deserialize block number
        block_number = vptr_to_uint32(serializedData, &i);

        deserializedData.emplace_back(file_id, block_number);
    }

    return deserializedData;
}

/**
 * Sets FS_Track data with desired PostFileBlocksData structs
 * @param data
 */
void FS_Track::PostFileBlocks_set_data(const std::vector<FS_Track::PostFileBlocksData> &data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;
    uint32_t dataSize = (uint32_t) data.size();

    push_uint32_into_vector_uint8(serializedData, dataSize);

    // For each struct
    for (const auto &node_blocks: data) {
        //Serialize ip and total number of blocks
        push_uint32_into_vector_uint8(serializedData, node_blocks.ip.s_addr);

        uint32_t totalBlocks = node_blocks.block_numbers.size();

        push_uint32_into_vector_uint8(serializedData, totalBlocks);

        // Serialize blocks' number
        for (auto block: node_blocks.block_numbers) {
            push_uint32_into_vector_uint8(serializedData, block);
        }

        totalBytes += (8 + (node_blocks.block_numbers.size() * 4));
    }

    // Update FS_Track
    this->set_data(serializedData->data(), totalBytes);

    delete serializedData;
}

/**
 * Deserialize FS_Track data into PostFileBlocks structs
 * @return
 */
std::vector<FS_Track::PostFileBlocksData> FS_Track::PostFileBlocks_get_data() {
    auto deserializedData = std::vector<FS_Track::PostFileBlocksData>();

    char* serializedData = (char*) this->data;
    uint32_t i = 0;

    // Calculate total number of structs sent
    uint32_t len = vptr_to_uint32(serializedData, &i);

    if(len == 0) {
        perror("No data received");

        return {};
    }

    struct in_addr ip = in_addr();
    uint32_t totalBlocks;

    // For each struct sent
    for (uint32_t k = 0; k < len;k++) {
        // Get Node ip
        ip.s_addr = vptr_to_uint32(serializedData, &i);

        // Calculate total number of blocks sent
        totalBlocks = vptr_to_uint32(serializedData, &i);

        std::vector<uint32_t> block_numbers = std::vector<uint32_t>();

        // For each block sent
        for (uint32_t j = 0; j < totalBlocks; j++) {
            // Get block number
            uint32_t block_number = vptr_to_uint32(serializedData, &i);

            block_numbers.emplace_back(block_number);
        }

        deserializedData.emplace_back(ip, block_numbers);
    }

    return deserializedData;
}

/**
 * Sets FS_Track data with desired error message
 * @param data
 */
void FS_Track::ErrorMessage_set_data(std::string &details) {
    auto* serializedData = new std::vector<uint8_t>();
    uint32_t len = details.size();
    uint32_t totalBytes = 4 + len;

    serializedData->reserve(totalBytes);

    push_uint32_into_vector_uint8(serializedData, len);


    for(uint32_t i = 0; i < len; i++){
        serializedData->emplace_back(details[i]);
    }

    this->set_data(serializedData->data(), totalBytes);

    delete serializedData;
}

/**
 * Deserialize FS_Track data into error message
 * @return
 */
FS_Track::ErrorMessageData FS_Track::ErrorMessage_get_data() {
    auto deserializedData = std::string();

    char* serializedData = (char*) this->data;
    uint32_t i = 0;

    uint32_t filename_len = vptr_to_uint32(serializedData, &i);

    if(filename_len == 0) {
        perror("No data received");

        return std::string();
    }

    // Get file name
    for (uint32_t j = 0; j < filename_len; j++) deserializedData.push_back((char) serializedData[i++]);

    return FS_Track::ErrorMessageData(deserializedData);
}
