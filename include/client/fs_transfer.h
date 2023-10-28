//
// Created by mafba on 19/10/2023.
//

#ifndef TP2_FS_TRANSFER_CPP_H
#define TP2_FS_TRANSFER_CPP_H


#include <cstdint>
#include <stddef.h>
#include <unistd.h>
#include <cstring>

#define BLOCK_SIZE 1024 // 1 kib de dados

#define MAX_BLOCKS BLOCK_SIZE / sizeof(uint32_t) + 1 // + 1 porque sobra espaco, nao identificamos o numero de blocos porque podemos usar o size do pacote para isso

// deve ser 1044
// sizeof direto daria 1048
#define FS_TRANSFER_PACKET_SIZE sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(FS_Data)

// enviar dados de um bloco
struct BlockSendData {
    uint32_t blockID;
    char data[BLOCK_SIZE];

    BlockSendData(uint32_t block, const char *data, ssize_t size); //char data[BLOCK_SIZE]);

    constexpr uint32_t getId() const {
        return this->blockID;
    }

    char *getData() {
        return this->data;
    }

    void setId(uint32_t id);

    void setData(const void *data, ssize_t size);

};

// pedir blocos
struct BlockRequestData {
    uint32_t blockID[MAX_BLOCKS];

    BlockRequestData(const uint32_t *blockID, ssize_t size); // uint32_t blockID[MAX_BLOCKS]);

    uint32_t *getData() {
        return this->blockID;
    }

    void setData(const __uint32_t *ids, ssize_t size);
};

union FS_Data {
    BlockSendData blockData;
    BlockRequestData blockRequestData;

    FS_Data();

    ~FS_Data();
};

struct FS_Transfer_Packet {
    FS_Transfer_Packet();

    FS_Transfer_Packet(uint8_t opc, uint64_t id, const BlockRequestData *data, uint32_t size);

    FS_Transfer_Packet(uint8_t opc, uint64_t id, const BlockSendData *data, uint32_t size);

    ~FS_Transfer_Packet();

    uint32_t checksum;
    uint32_t opc_size;
    uint64_t id;
    FS_Data data;


    // getters
    constexpr uint32_t getChecksum() const {
        return checksum;
    }

    constexpr uint8_t getOpcode() const {
        return static_cast<uint8_t>((opc_size >> 30) & 0x03);
    }

    constexpr uint32_t getSize() const {
        uint32_t size = this->opc_size;
        return (size & 0x3FFFFFFF);
    }

    constexpr uint64_t getId() const {
        return this->id;
    }

    constexpr void *getData() {
        return static_cast<void *>(&data);
    }

    uint32_t calculateChecksum() const;

    bool checkErrors() const {
        return (calculateChecksum() == checksum);
    }
    // end getters

    // setters
    void setOpcode(uint8_t value);

    void setSize(uint32_t value);

    void setId(uint64_t value);

    void setData(const void *data, ssize_t size);
    // end setters

    //buffer related
    void fs_transfer_read_buffer(const void *buf, ssize_t size);

}; // __attribute__((packed));



/*NOTA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



Struct nao deve ter padding em elementos, e (muito) provavelmente nao tem
Contudo, ela tem 1044B, e isso nao fica alinhado num array de structs, entao o compilador faz padding para 1048 colocando bytes inuteis no fim
Ao fazer memcpy etc, esses bytes vao ser ignorados, e o tamanho 1044 ja esta definido em FS_TRANSFER_PACKET_SIZE
Checks abaixo verificam que esta tudo alinhado, ignorando os trailing bytes na struct principal

!!!!!!!!!!!!!!!!!!!!!!!!!!!! Nunca usar sizeof da struct em memcpy e assim, senao andamos a mandar os bytes inuteis para a socket

Assim, nao tem de haver serialization nem nada porque ja ta tudo feito


*/









#define FS_TRANSFER_DEBUG
#ifdef FS_TRANSFER_DEBUG

static_assert(offsetof(FS_Transfer_Packet, data) == sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t),
              "Error: FS_Transfer has padding which is not accounted for");
// static_assert(sizeof(FS_Transfer_Packet) == sizeof(FS_Data) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t), "Error: BlockData has padding which is not accounted for");

static_assert(sizeof(BlockSendData) == sizeof(char) * 1024 + sizeof(uint32_t),
              "Error: BlockData has padding which is not accounted for");

static_assert(sizeof(FS_Data) == sizeof(BlockSendData), "Error: union FS_Data has unexpected size");
static_assert(sizeof(FS_Data) == sizeof(BlockRequestData), "Error: union FS_Data has unexpected size");

#endif


#endif //TP2_FS_TRANSFER_CPP_H
