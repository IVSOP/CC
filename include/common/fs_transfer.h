//
// Created by mafba on 19/10/2023.
//

#ifndef TP2_FS_TRANSFER_CPP_H
#define TP2_FS_TRANSFER_CPP_H


#include <cstdint>
#include <stddef.h>

#define BLOCK_SIZE 1024 // 1 kib de dados

#define MAX_BLOCKS BLOCK_SIZE / sizeof(uint32_t) + 1 // + 1 porque sobra espaco, nao identificamos o numero de blocos porque podemos usar o size do pacote para isso

// deve ser 1044
// sizeof direto daria 1048
#define FS_TRANSFER_PACKET_SIZE sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(FS_Data)

// enviar dados de um bloco
struct BlockData {
	uint32_t blockID;
	char data[BLOCK_SIZE];
};

// pedir blocos
struct BlockRequestData {
	uint32_t blockID[MAX_BLOCKS];
};

union FS_Data {
	BlockData blockData;
	BlockRequestData blockRequestData;
};

struct FS_Transfer_Packet {
	FS_Transfer_Packet() = default;
	~FS_Transfer_Packet() = default;

    uint32_t checksum;
    uint32_t opc_size;
    uint64_t id;
	BlockData data;

    constexpr uint32_t getChecksum() const {
		return checksum;
	}

    constexpr uint8_t getOpcode() const {
		uint8_t fstByte = static_cast<uint8_t> (this->opc_size);
		fstByte = (fstByte & 0xC0) >> 6;
		return fstByte;
	}

    constexpr uint32_t getSize() const {
		uint32_t size = this->opc_size;
		size = (size & 0x3F);
		return size;
	}

    constexpr uint64_t getId() const {
		return id;
	}

	constexpr void *getData() {
		return static_cast<void *>(&data);
	}

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

		static_assert(offsetof(FS_Transfer_Packet, data) == sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t), "Error: FS_Transfer has padding which is not accounted for");
		// static_assert(sizeof(FS_Transfer_Packet) == sizeof(FS_Data) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t), "Error: BlockData has padding which is not accounted for");

		static_assert(sizeof(BlockData) == sizeof(char) * 1024 + sizeof(uint32_t), "Error: BlockData has padding which is not accounted for");
		
		static_assert(sizeof(FS_Data) == sizeof(BlockData), "Error: union FS_Data has unexpected size");
		static_assert(sizeof(FS_Data) == sizeof(BlockRequestData), "Error: union FS_Data has unexpected size");

	#undef FS_TRANSFER_DEBUG // ?????

	#endif


#endif //TP2_FS_TRANSFER_CPP_H
