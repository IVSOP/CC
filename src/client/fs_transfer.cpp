#include "fs_transfer.h"
#include "checksum.h"

//construtores

FS_Transfer_Packet::FS_Transfer_Packet() = default;

FS_Transfer_Packet::FS_Transfer_Packet(uint8_t opc, uint64_t id, const BlockRequestData *data, uint32_t size) {
    this->setOpcode(opc);
    this->setId(id);
    this->setData(data, size);
}

FS_Transfer_Packet::FS_Transfer_Packet(uint8_t opc, uint64_t id, sys_nanoseconds& time,  BlockRequestData *data, uint32_t size) {
    this->setOpcode(opc);
    this->setId(id);
    this->setTimestamp(time);
    this->setData(data, size);
}

FS_Transfer_Packet::FS_Transfer_Packet(uint8_t opc, uint64_t id, const BlockSendData *data, uint32_t size) {
    this->setOpcode(opc);
    this->setId(id);
    this->setData(data, size);
}

FS_Transfer_Packet::~FS_Transfer_Packet() = default;

FS_Data::FS_Data() {}

FS_Data::~FS_Data() {}

// ler Fs_Transfer_Packet da socket, diretamente porque já vem alinhado
// reutilizar um fs_transfer_packet prévio
void FS_Transfer_Packet::fs_transfer_read_buffer(const void *buffer, ssize_t size) {
    if (buffer != nullptr) {
        memcpy(static_cast <void *> (this), buffer, size); // assumir FS_TRANSFER_PACKET_SIZE?
    } else {
        print_error("Buffer is null");
    }
}

//setters

void FS_Transfer_Packet::setOpcode(uint8_t opc) {
    this->opc_size &= 0x3FFFFFFF;
    opc_size |= ((static_cast<uint32_t>(opc) & 0x3) << 30);
}

void FS_Transfer_Packet::setSize(uint32_t size) {
    this->opc_size = (this->opc_size & 0xC0000000) | (size & 0x3FFFFFFF);
}

void FS_Transfer_Packet::setId(uint64_t id) {
    this->id = id;
}

void FS_Transfer_Packet::setTimestamp(uint64_t timestamp) {
    this->timestamp = timestamp;
}

void FS_Transfer_Packet::setTimestamp(sys_nanoseconds time) {
    sys_nano_diff nanoDiff = time.time_since_epoch();
    uint64_t timeUint64 = static_cast<uint64_t>(nanoDiff.count());
    this->timestamp = timeUint64;
}

//já atualiza size e checksum
void FS_Transfer_Packet::setData(const void *data, ssize_t size) {
    memcpy(static_cast <void *> (&this->data), data, size);
    this->setSize(size);
    this->checksum = FS_Transfer_Packet::calculateChecksum();
}

//blockSendData

BlockSendData::BlockSendData() = default;

BlockSendData::BlockSendData(uint32_t blockID, const char *data, ssize_t size) {
    this->setId(blockID);
    BlockSendData::setData(data, size);
}

void BlockSendData::setId(uint32_t id) {
    this->blockID = id;
}

void BlockSendData::setData(const void *data, ssize_t size) {
    if ((unsigned long) size > BLOCK_SIZE) {
        print_error("size too big for array");
    }
    memcpy(this->data, data, size); // devia ser sempre BLOCK_SIZE? // faz deep copy aqui, e volta a fazer no setData??
}

//blockRequestData
BlockRequestData::BlockRequestData(const uint32_t *ids, ssize_t size) {
    this->setData(ids, size);// faz deep copy aqui, e volta a fazer no setData??
}

void BlockRequestData::setData(const __uint32_t *ids, ssize_t size) {
    if ((unsigned long) size > BLOCK_SIZE) {
        print_error("size too big for array");
    }
    memcpy(this->blockID, ids, size); // devia ser sempre MAX_BLOCKS?
}


// outros

// !!!!! pointer comeca no opc_size, senao checksum incluia o proprio campo do checksum
// !!!!! so calcula tendo em conta o size, garantir que esta inicializado
// solucao esta muito dependente do formato da struct, cuidado
uint32_t FS_Transfer_Packet::calculateChecksum() const {
    // return sha3_32(reinterpret_cast<const void *>(&this->opc_size), FS_TRANSFER_PACKET_SIZE - sizeof(checksum));
	return sha3_32(reinterpret_cast<const void *>(&this->opc_size), this->getSize() + sizeof(opc_size) + sizeof(id));
}

