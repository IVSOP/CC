#include <cstring>

#include "fs_track.h"
#include "TCP_socket.h"
#include "convert_types.h"
#include "errors.h"

/* Constructors */

FS_Track::FS_Track(uint8_t opcode, bool opts, uint64_t hash) {
    this->opcode_opts = ((opcode << 1) + opts);
    FS_Track::set_Size(0);
    this->hash = hash;
    this->data = nullptr;
    this->dataSize = 0;
}

FS_Track::FS_Track() {
    this->opcode_opts = 0;
    FS_Track::set_Size(0);
    this->hash = 0;
    this->data = nullptr;
    this->dataSize = 0;
}

FS_Track::~FS_Track() {
    if(this->dataSize > 0) delete [] (uint8_t*) this->data;
}

FS_Track::RegUpdateData::RegUpdateData(uint64_t file_id, std::vector<uint32_t> block_numbers) {
    this->file_hash = file_id;
    this->block_numbers = std::vector<uint32_t>(block_numbers);
}

FS_Track::RegUpdateData::~RegUpdateData() = default;

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

/* private Functions end */

void FS_Track::set_data(void* buf, uint32_t size){
    uint32_t newSize = this->dataSize + size;
    uint8_t* newData = new uint8_t [newSize];

    if(this->dataSize > 0){
        memcpy(newData, this->data, this->dataSize);
        delete [] (uint8_t*) this->data;
    }

    memcpy(&newData[this->dataSize], buf, size);

    this->data = newData;
    this->dataSize = newSize;
}

void FS_Track::fs_track_read_buffer(void* buf, ssize_t size){
    if(size < 4){
        print_error("No data to read.")

        return;
    }

    char* buffer = (char*) buf;

    uint32_t curPos = 0;

    memcpy(&(this->opcode_opts), buffer, 1);
    ++curPos;

    memcpy(this->size, &buffer[1], 3);
    curPos += 3;

    bool hasID = this->fs_track_getOpt() == 1;

    if(hasID){
        memcpy(&(this->hash), &buffer[curPos], 8);
        curPos += 8;
    }

    uint32_t dataSize = size - curPos;
    if(dataSize > 0) this->set_data(&buffer[curPos], dataSize);
}

std::pair<uint8_t *, uint32_t> FS_Track::fs_track_to_buffer(){
    uint32_t dataSize = this->fs_track_getSize();
    uint32_t bufferSize = 4 + dataSize;

    bool hasID = this->fs_track_getOpt() == 1;
    if(hasID) bufferSize += 8;

    uint8_t* buffer = (uint8_t*) new uint8_t[bufferSize];

    uint32_t curPos = 0;

    memcpy(&buffer[curPos], &this->opcode_opts, 1);
    ++curPos;

    memcpy(&buffer[curPos], this->size, 3);
    curPos += 3;

    if(hasID) {
        memcpy(&buffer[curPos], &this->hash, 8);
        curPos += 8;
    }

    if(dataSize > 0){
        memcpy(&buffer[curPos], this->data, dataSize);
    }

    return std::pair<uint8_t *, uint32_t>(buffer, bufferSize);
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
    return this->hash;
}

// TODO No update não podiamos usar a mesma estrutura do post?

/**
 * Sets FS_Track data with desired UpdateFileBlocksData structs
 * @param data
 */
void FS_Track::RegUpdateData_set_data(const std::vector<RegUpdateData>& data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;
    push_uint32_into_vector_uint8(serializedData, (uint32_t) data.size());

    // For each struct
    for(const auto& fileBlock : data){
        // Serialized file Id
        push_uint64_into_vector_uint8(serializedData, fileBlock.file_hash);

        /*
        if(fileBlock.file_hash == 64){
            printf("Ola\n");
        }
         */

        uint32_t blocks_len = fileBlock.block_numbers.size();
        // Serialize block_numbers length
        push_uint32_into_vector_uint8(serializedData, blocks_len);

        for (const auto& block : fileBlock.block_numbers){
            // Serialize block number
            push_uint32_into_vector_uint8(serializedData, block);
        }

        totalBytes += (12 + 4 * blocks_len);
    }

    this->set_data(serializedData->data(), totalBytes);
    this->set_Size(totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into UpdateFileBlocksData structs
 * @return
 */
std::vector<FS_Track::RegUpdateData> FS_Track::RegUpdateData_get_data() {
    // Serialized data
    uint8_t * serializedData = (uint8_t *) this->data;

    uint32_t i = 0;
    uint32_t len = vptr_to_uint32(serializedData, &i);

    if(len == 0) {
        print_error("No data received");

        return {};
    }

    std::vector<FS_Track::RegUpdateData> deserializedData = std::vector<FS_Track::RegUpdateData>();

    uint64_t file_id;

    for(uint32_t j = 0; j < len; j++){
        // Deserialize file hash
        file_id = vptr_to_uint64(serializedData, &i);

        // Deserialize block number
        uint32_t blocks_len = vptr_to_uint32(serializedData, &i);

        // TODO Ivan, otimiza esta merda !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        std::vector<uint32_t> block_numbers = std::vector<uint32_t>();

        for(uint64_t k = 0; k < blocks_len; k++){
            block_numbers.emplace_back(vptr_to_uint32(serializedData, &i));
        }

        deserializedData.emplace_back(file_id, block_numbers);
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
    this->set_Size(totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into PostFileBlocks structs
 * @return
 */
std::vector<FS_Track::PostFileBlocksData> FS_Track::PostFileBlocks_get_data() {
    auto deserializedData = std::vector<FS_Track::PostFileBlocksData>();

    uint8_t * serializedData = (uint8_t *) this->data;
    uint32_t i = 0;

    // Calculate total number of structs sent
    uint32_t len = vptr_to_uint32(serializedData, &i);

    if(len == 0) {
        print_error("No data received");

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
    this->set_Size(totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into error message
 * @return
 */
FS_Track::ErrorMessageData FS_Track::ErrorMessage_get_data() {
    auto deserializedData = std::string();

    uint8_t* serializedData = (uint8_t*) this->data;
    uint32_t i = 0;

    uint32_t filename_len = vptr_to_uint32(serializedData, &i);

    if(filename_len == 0) {
        print_error("No data received");

        return std::string();
    }

    // Get file name
    for (uint32_t j = 0; j < filename_len; j++) deserializedData.push_back((char) serializedData[i++]);

    return FS_Track::ErrorMessageData(deserializedData);
}
