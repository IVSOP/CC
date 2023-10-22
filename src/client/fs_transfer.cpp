#include "fs_transfer.h"

#include "checksum.h"

// !!!!! pointer comeca no opc_size, senao checksum incluia o proprio campo do checksum
uint32_t FS_Transfer_Packet::calculateChecksum() const {
	return sha3(reinterpret_cast<const void *>(&this->opc_size), FS_TRANSFER_PACKET_SIZE - sizeof(checksum));
}
