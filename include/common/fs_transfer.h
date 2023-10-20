//
// Created by mafba on 19/10/2023.
//

#ifndef TP2_FS_TRANSFER_CPP_H
#define TP2_FS_TRANSFER_CPP_H


#include <cstdint>
#include <stddef.h>

#define FS_TRANSFER_DATA_SIZE 1024 // uint32 + 1 kib de dados

struct BlockData {
	uint32_t blockID;
	char data[FS_TRANSFER_DATA_SIZE];
};

struct FS_Transfer {
	FS_Transfer() = default;
	~FS_Transfer() = default;

    uint32_t checksum;
    uint32_t opc_size;
    uint64_t id;
	// void * data;
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
};

	#define FS_TRANSFER_DEBUG
	#ifdef FS_TRANSFER_DEBUG

		static_assert(offsetof(FS_Transfer, data) == sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t), "Error: FS_Transfer has padding which is not accounted for");
		// isto nao da pq??????????????????? static_assert(offsetof(BlockData, data) == sizeof(uint32_t), "Error: BlockData has padding which is not accounted for");
		static_assert(sizeof(BlockData) == sizeof(char) * 1024 + sizeof(uint32_t), "Error: BlockData has padding which is not accounted for");

	#undef FS_TRANSFER_DEBUG // ?????

	#endif


#endif //TP2_FS_TRANSFER_CPP_H
