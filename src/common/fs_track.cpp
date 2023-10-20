#include <cstring>

#include "fs_track.h"
#include "TCP_socket.h"

/* Constructors */

FS_Track::FS_Track(uint8_t opcode, bool opts, uint8_t size[], uint64_t id, void *data) {
    this->opcode_opts = ((opcode_opts << 1) + opts);
    memcpy(this->size, size, SIZE_LENGTH * sizeof(uint8_t));
    this->id = id;
    this->data = data;
}

FS_Track::~FS_Track() = default;

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

/* private Functions end */

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
    auto *ptr = reinterpret_cast<uint8_t *> (&ans);
    ptr[1] = this->size[0];
    ptr[2] = this->size[1];
    ptr[3] = this->size[2];

    return ans;
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

    int lenSize = 4;
    uint32_t totalBytes = 0;

    // For each struct
    for (const auto &fileData: data) {

        // Filename len
        uint32_t len = (uint32_t) strnlen(fileData.filename, FILENAME_SIZE);

        // Serialize filename len byte by byte
        for (int i = 0; i < lenSize; i++) {
            serializedData->emplace_back((std::uint8_t) ((len >> (i * 8)) & 0xFF));
        }

        // Serialize filename byte by byte
        for (uint32_t i = 0; i < len; i++) {
            serializedData->emplace_back(fileData.filename[i]);
            totalBytes++;
        }

        // Update data's total bytes
        totalBytes += (lenSize + len);
    }

    // Update FS_Track
    FS_Track::set_Size(totalBytes);
    this->data = serializedData;
}

/**
 * Deserialize FS_Track data into RegData structs
 * @return
 */
std::vector<FS_Track::RegData> FS_Track::Reg_get_data() {
    // Serialized data
    std::vector<uint8_t> serializedData = *reinterpret_cast<std::vector<uint8_t> *>(this->data);
    uint32_t len = serializedData.size();

    if(len == 0) {
        perror("No data received");

        return {};
    }

    std::vector<FS_Track::RegData> deserializedData = std::vector<FS_Track::RegData>();

    uint32_t filename_len;
    char filename[FILENAME_SIZE];

    // For each byte of data
    for (uint32_t i = 0; i < len;) {
        // Get filename len
        filename_len = ((std::uint32_t) serializedData[i++]);
        filename_len |= ((std::uint32_t) serializedData[i++]) << 8;
        filename_len |= ((std::uint32_t) serializedData[i++]) << 16;
        filename_len |= ((std::uint32_t) serializedData[i++]) << 24;

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

    int lenSize = 4;
    int idSize = 8;
    uint32_t totalBytes = 0;

    // For each struct
    for (const auto &fileData: data) {

        uint64_t fileId = fileData.file_id;

        // Serialize filename fileId byte by byte
        for (int i = 0; i < idSize; i++) {
            serializedData->emplace_back((std::uint8_t) ((fileId >> (i * 8)) & 0xFF));
        }

        // Filename len
        uint32_t len = (uint32_t) strnlen(fileData.filename, FILENAME_SIZE);

        // Serialize filename len byte by byte
        for (int i = 0; i < lenSize; i++) {
            serializedData->emplace_back((std::uint8_t) ((len >> (i * 8)) & 0xFF));
        }

        // Serialize filename byte by byte
        for (uint32_t i = 0; i < len; i++) {
            serializedData->emplace_back(fileData.filename[i]);
            totalBytes++;
        }

        // Update data's total bytes
        totalBytes += (idSize + lenSize + len);
    }

    // Update FS_Track
    FS_Track::set_Size(totalBytes);
    this->data = serializedData;
}

/**
 * Deserialize FS_Track data into IDAssignment_get_data structs
 * @return
 */
std::vector<FS_Track::IDAssignmentData> FS_Track::IDAssignment_get_data() {
    // Serialized data
    std::vector<uint8_t> serializedData = *reinterpret_cast<std::vector<uint8_t> *>(this->data);
    uint32_t len = serializedData.size();

    if(len == 0) {
        perror("No data received");

        return {};
    }

    std::vector<FS_Track::IDAssignmentData> deserializedData = std::vector<FS_Track::IDAssignmentData>();

    uint64_t fileId;
    uint32_t filename_len;
    char filename[FILENAME_SIZE];

    // For each byte of data
    for (uint32_t i = 0; i < len;) {
        // Get file id
        fileId = ((std::uint64_t) serializedData[i++]);
        fileId |= ((std::uint64_t) serializedData[i++]) << 8;
        fileId |= ((std::uint64_t) serializedData[i++]) << 16;
        fileId |= ((std::uint64_t) serializedData[i++]) << 24;
        fileId |= ((std::uint64_t) serializedData[i++]) << 32;
        fileId |= ((std::uint64_t) serializedData[i++]) << 40;
        fileId |= ((std::uint64_t) serializedData[i++]) << 48;
        fileId |= ((std::uint64_t) serializedData[i++]) << 56;

        // Get filename len
        filename_len = ((std::uint32_t) serializedData[i++]);
        filename_len |= ((std::uint32_t) serializedData[i++]) << 8;
        filename_len |= ((std::uint32_t) serializedData[i++]) << 16;
        filename_len |= ((std::uint32_t) serializedData[i++]) << 24;

        // Get file name
        for (uint32_t j = 0; j < filename_len; j++) filename[j] = (char) serializedData[i++];

        // Store recovered data
        deserializedData.emplace_back(filename, fileId);
    }

    return deserializedData;
}

/**
 * Sets FS_Track data with desired UpdateFileBlocksData structs
 * @param data
 */
void FS_Track::UpdateFileBlocks_set_data(const std::vector<UpdateFileBlocksData>& data) {
    // Serialized bytes
    auto *serializedData = new std::vector<uint32_t>();

    uint32_t totalBytes = 0;

    // For each struct
    for(const auto& fileBlock : data){
        // Serialized file Id
        serializedData->emplace_back((std::uint32_t)(fileBlock.file_id & 0xFFFFFFFF));
        serializedData->emplace_back((std::uint32_t)((fileBlock.file_id >> 32) & 0xFFFFFFFF));

        // Serialize block number
        serializedData->emplace_back(fileBlock.block_number);

        totalBytes += 12;
    }

    FS_Track::set_Size(totalBytes);
    this->data = serializedData;
}

/**
 * Deserialize FS_Track data into UpdateFileBlocksData structs
 * @return
 */
std::vector<FS_Track::UpdateFileBlocksData> FS_Track::UpdateFileBlocks_get_data() {
    // Serialized data
    std::vector<uint32_t> serializedData = *reinterpret_cast<std::vector<uint32_t> *>(this->data);
    uint32_t len = serializedData.size();

    if(len == 0) {
        perror("No data received");

        return {};
    }

    std::vector<FS_Track::UpdateFileBlocksData> deserializedData = std::vector<FS_Track::UpdateFileBlocksData>();

    uint64_t file_id;
    uint32_t block_number;

    for(uint32_t i = 0; i < len;){
        // Deserialize file id
        file_id = (std::uint64_t) serializedData[i++];
        file_id |= ((std::uint64_t) serializedData[i++]) << 32;

        // Deserialize block number
        block_number = serializedData[i++];

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
    auto *serializedData = new std::vector<uint32_t>();

    uint32_t totalBytes = 0;

    // For each struct
    for (const auto &node_blocks: data) {
        //Serialize ip and total number of blocks
        serializedData->emplace_back(node_blocks.ip.s_addr);
        serializedData->emplace_back((uint32_t) node_blocks.block_numbers.size());

        // Serialize blocks' number
        for (auto block: node_blocks.block_numbers) serializedData->emplace_back(block);

        totalBytes += (8 + (node_blocks.block_numbers.size() * 4));
    }

    // Update FS_Track
    FS_Track::set_Size(totalBytes);
    this->data = serializedData;
}

/**
 * Deserialize FS_Track data into PostFileBlocks structs
 * @return
 */
std::vector<FS_Track::PostFileBlocksData> FS_Track::PostFileBlocks_get_data() {
    auto deserializedData = std::vector<FS_Track::PostFileBlocksData>();

    std::vector<uint32_t> serializedData = *reinterpret_cast<std::vector<uint32_t> *>(this->data);
    uint32_t len = serializedData.size();

    if(len == 0) {
        perror("No data received");

        return {};
    }

    struct in_addr ip = in_addr();
    uint32_t totalBlocks;

    for (uint32_t i = 0; i < len;) {
        ip.s_addr = serializedData[i++];
        totalBlocks = serializedData[i++];

        std::vector<uint32_t> block_numbers = std::vector<uint32_t>(totalBlocks, 0);

        for (uint32_t j = 0; j < totalBlocks && i < len; j++) {
            block_numbers.emplace_back(serializedData[i++]);
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

    serializedData->emplace_back((std::uint8_t)(len & 0xFF));
    serializedData->emplace_back((std::uint8_t)((len >> 8) & 0xFF));
    serializedData->emplace_back((std::uint8_t)((len >> 16) & 0xFF));
    serializedData->emplace_back((std::uint8_t)((len >> 24) & 0xFF));

    for(uint32_t i = 0; i < len; i++){
        serializedData->emplace_back(details[i++]);
    }

    this->data = serializedData;
}

/**
 * Deserialize FS_Track data into error message
 * @return
 */
FS_Track::ErrorMessageData FS_Track::ErrorMessage_get_data() {
    auto deserializedData = std::string();

    std::vector<uint8_t> serializedData = *reinterpret_cast<std::vector<uint8_t> *>(this->data);
    uint32_t len = serializedData.size();
    uint32_t i = 0;

    if(len == 0) {
        perror("No data received");

        return std::string();
    }

    uint32_t filename_len = ((std::uint32_t) serializedData[i++]);
    filename_len |= ((std::uint32_t) serializedData[i++]) << 8;
    filename_len |= ((std::uint32_t) serializedData[i++]) << 16;
    filename_len |= ((std::uint32_t) serializedData[i++]) << 24;

    // Get file name
    for (uint32_t j = 0; j < filename_len; j++) deserializedData.push_back((char) serializedData[i++]);

    return deserializedData;
}
