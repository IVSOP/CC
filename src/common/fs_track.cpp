#include <cstring>

#include "fs_track.h"
#include "TCP_socket.h"
#include "convert_types.h"
#include "errors.h"

/* Constructors */

FS_Track::FS_Track(uint8_t opcode){
    this->opcode_opts = ((opcode << 1) + false);
    FS_Track::setSize(0);
    this->hash = 0;
    this->data = nullptr;
    this->dataSize = 0;
}

FS_Track::FS_Track(uint8_t opcode, uint64_t hash) {
    this->opcode_opts = ((opcode << 1) + true);
    FS_Track::setSize(0);
    this->hash = hash;
    this->data = nullptr;
    this->dataSize = 0;
}

FS_Track::FS_Track() {
    this->opcode_opts = 0;
    FS_Track::setSize(0);
    this->hash = 0;
    this->data = nullptr;
    this->dataSize = 0;
}

FS_Track::~FS_Track() {
    if (this->dataSize > 0) delete[] (uint8_t *) this->data;
}

FS_Track::RegUpdateData::RegUpdateData(uint64_t file_id, bitMap block_numbers) {
    this->file_hash = file_id;
    this->block_numbers = bitMap(block_numbers);
}

FS_Track::RegUpdateData::~RegUpdateData() = default;

uint64_t FS_Track::RegUpdateData::getFileHash() {
    return this->file_hash;
}

bitMap FS_Track::RegUpdateData::getBlockNumbers() {
    return bitMap(this->block_numbers);
};

FS_Track::PostFileBlocksData::PostFileBlocksData(struct in_addr ip, bitMap block_numbers) {
    this->ip = in_addr();
    this->ip.s_addr = ip.s_addr;
    this->block_numbers = bitMap(block_numbers);
}

FS_Track::PostFileBlocksData::~PostFileBlocksData() = default;

FS_Track::ErrorMessageData::ErrorMessageData(std::string details) {
    this->details = std::string();

    this->details.assign(details);
}

FS_Track::ErrorMessageData::~ErrorMessageData() = default;

/* Constructors end */

/* Private Functions */

void FS_Track::setSize(uint32_t bytes) {
    this->size[0] = (std::uint8_t) ((bytes >> 16) & 0xFF);
    this->size[1] = (std::uint8_t) ((bytes >> 8) & 0xFF);
    this->size[2] = (std::uint8_t) ((bytes) & 0xFF);
}

/* private Functions end */

void FS_Track::setData(void *buf, uint32_t size) {
    uint32_t newSize = this->dataSize + size;
    uint8_t *newData = new uint8_t[newSize];

    if (this->dataSize > 0) {
        memcpy(newData, this->data, this->dataSize);
        delete[] (uint8_t *) this->data;
    }

    memcpy(&newData[this->dataSize], buf, size);

    this->data = newData;
    this->dataSize = newSize;
}

void FS_Track::fsTrackHeaderReadBuffer(void *buf, ssize_t size) {
    if (size < 4) {
        print_error("No data to read.")

        return;
    }

    uint8_t *buffer = (uint8_t *) buf;

    memcpy(&(this->opcode_opts), buffer, 1);

    memcpy(this->size, &buffer[1], 3);
}

void FS_Track::fsTrackReadHash(void *buf, ssize_t size) {
    if (size < 8) {
        print_error("No data to read.")

        return;
    }

    char *buffer = (char *) buf;

    memcpy(&(this->hash), buffer, 8);
}

std::pair<uint8_t *, uint32_t> FS_Track::fsTrackToBuffer() {
    uint32_t dataSize = this->fsTrackGetSize();
    uint32_t bufferSize = 4 + dataSize;

    bool hasID = this->fsTrackGetOpt() == 1;
    if (hasID) bufferSize += 8;

    uint8_t *buffer = (uint8_t *) new uint8_t[bufferSize];

    uint32_t curPos = 0;

    memcpy(&buffer[curPos], &this->opcode_opts, 1);
    ++curPos;

    memcpy(&buffer[curPos], this->size, 3);
    curPos += 3;

    if (hasID) {
        memcpy(&buffer[curPos], &this->hash, 8);
        curPos += 8;
    }

    if (dataSize > 0) {
        memcpy(&buffer[curPos], this->data, dataSize);
    }

    return std::pair<uint8_t *, uint32_t>(buffer, bufferSize);
}

uint8_t FS_Track::fsTrackGetOpcode() {
    uint8_t opcode = this->opcode_opts;
    return ((opcode & 0xFE) >> 1);
}

uint8_t FS_Track::fsTrackGetOpt() {
    uint8_t opt = this->opcode_opts;
    return (opt & 0x01);
}

uint32_t FS_Track::fsTrackGetSize() {
    return ((this->size[0] << 16) + (this->size[1] << 8) + (this->size[2]));
}

uint64_t FS_Track::fsTrackGetHash() {
    return this->hash;
}

// TODO No update não podiamos usar a mesma estrutura do post?

/**
 * Sets FS_Track data with desired UpdateFileBlocksData structs
 * @param data
 */
void FS_Track:: regUpdateDataSetData(const std::vector<RegUpdateData> &data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;
    pushUint32IntoVectorUint8(serializedData, (uint32_t) data.size());

    // For each struct
    for (const auto &fileBlock: data) {
        // Serialized file Id
        pushUint64IntoVectorUint8(serializedData, fileBlock.file_hash);

        uint32_t blocks_size = fileBlock.block_numbers.size();
        uint8_t significant_bits = blocks_size % 8;

        uint32_t blocks_len = (blocks_size / 8) + (significant_bits == 0 ? 0 : 1);

        // Serialize block_numbers length
        pushUint32IntoVectorUint8(serializedData, blocks_len);
        serializedData->emplace_back(significant_bits);

        bitmap_serialize(serializedData, fileBlock.block_numbers, blocks_size, significant_bits);

        totalBytes += (13 + blocks_len);
    }

    this->setData(serializedData->data(), totalBytes);
    this->setSize(totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into UpdateFileBlocksData structs
 * @return
 */
std::vector<FS_Track::RegUpdateData> FS_Track::regUpdateDataGetData() {
    // Serialized data
    uint8_t *serializedData = (uint8_t *) this->data;

    uint32_t i = 0;
    uint32_t len = vptrToUint32(serializedData, &i);

    std::vector<FS_Track::RegUpdateData> deserializedData = std::vector<FS_Track::RegUpdateData>();

    if (len == 0) return deserializedData;

    uint64_t file_hash;

    for (uint32_t j = 0; j < len; j++) {
        // Deserialize file hash
        file_hash = vptrToUint64(serializedData, &i);

        // Deserialize total bytes
        uint32_t blocks_len = vptrToUint32(serializedData, &i);
        uint8_t significant_bits = serializedData[i++];

        deserializedData.emplace_back(file_hash, bitmap_deserialize(serializedData, &i, blocks_len, significant_bits));
    }

    return deserializedData;
}

/**
 * Sets FS_Track data with desired PostFileBlocksData structs
 * @param data
 */
void FS_Track::postFileBlocksSetData(const std::vector<FS_Track::PostFileBlocksData> &data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint8_t>();

    uint32_t totalBytes = 4;
    uint32_t dataSize = (uint32_t) data.size();

    pushUint32IntoVectorUint8(serializedData, dataSize);

    // For each struct
    for (const auto &node_blocks: data) {
        //Serialize Ip and total number of blocks
        pushUint32IntoVectorUint8(serializedData, node_blocks.ip.s_addr);

        uint32_t blocks_size = node_blocks.block_numbers.size();
        uint8_t significant_bits = blocks_size % 8;

        uint32_t blocks_len = (blocks_size / 8) + (significant_bits == 0 ? 0 : 1);

        // Serialize block_numbers length
        pushUint32IntoVectorUint8(serializedData, blocks_len);
        serializedData->emplace_back(significant_bits);

        bitmap_serialize(serializedData, node_blocks.block_numbers, blocks_size, significant_bits);

        totalBytes += (9 + blocks_len);
    }

    // Update FS_Track
    this->setData(serializedData->data(), totalBytes);
    this->setSize(totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into PostFileBlocks structs
 * @return
 */
std::vector<FS_Track::PostFileBlocksData> FS_Track::postFileBlocksGetData() {
    auto deserializedData = std::vector<FS_Track::PostFileBlocksData>();

    if(this->fsTrackGetSize() == 0) return deserializedData;

    uint8_t *serializedData = (uint8_t *) this->data;
    uint32_t i = 0;

    // Calculate total number of structs sent
    uint32_t len = vptrToUint32(serializedData, &i);

    if (len == 0) return deserializedData;

    struct in_addr ip = in_addr();

    // For each struct sent
    for (uint32_t k = 0; k < len; k++) {
        // Get Node Ip
        ip.s_addr = vptrToUint32(serializedData, &i);

        uint32_t blocks_len = vptrToUint32(serializedData, &i);
        uint8_t significant_bits = serializedData[i++];

        deserializedData.emplace_back(ip, bitmap_deserialize(serializedData, &i, blocks_len, significant_bits));
    }

    return deserializedData;
}

/**
 * Sets FS_Track data with desired error message
 * @param data
 */
void FS_Track::errorMessageSetData(std::string &details) {
    auto *serializedData = new std::vector<uint8_t>();
    uint32_t len = details.size();
    uint32_t totalBytes = 4 + len;

    serializedData->reserve(totalBytes);

    pushUint32IntoVectorUint8(serializedData, len);


    for (uint32_t i = 0; i < len; i++) {
        serializedData->emplace_back(details[i]);
    }

    this->setData(serializedData->data(), totalBytes);
    this->setSize(totalBytes);
    delete serializedData;
}

/**
 * Deserialize FS_Track data into error message
 * @return
 */
FS_Track::ErrorMessageData FS_Track::errorMessageGetData() {
    auto deserializedData = std::string();

    uint8_t *serializedData = (uint8_t *) this->data;
    uint32_t i = 0;

    uint32_t filename_len = vptrToUint32(serializedData, &i);

    if (filename_len == 0) {
        print_error("No data received");

        return std::string();
    }

    // Get file name
    for (uint32_t j = 0; j < filename_len; j++) deserializedData.push_back((char) serializedData[i++]);

    return FS_Track::ErrorMessageData(deserializedData);
}
