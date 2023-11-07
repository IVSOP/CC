#ifndef TP2_CONVERT_TYPES_H
#define TP2_CONVERT_TYPES_H

#include <cstdint>
#include <vector>
#include "bitmap.h"

/**
 * Reads data from a buffer and converts it to an uint32_t
 * @param buffer Buffer
 * @param idx Current reading index
 * @return The read uint32_t
 */
uint32_t vptrToUint32(void *buffer, uint32_t *idx);

/**
 * Reads data from a buffer and converts it to an uint64_t
 * @param buffer Buffer
 * @param idx Current reading index
 * @return The read uint32_t
 */
uint64_t vptrToUint64(void *buffer, uint32_t *idx);

/**
 * Pushes an uint32_t value into an uint8_t vector
 * @param vector uint8_t vector
 * @param value uint32_t value
 */
void pushUint32IntoVectorUint8(std::vector<uint8_t> *vector, uint32_t value);

/**
 * Pushes an uint64_t value into an uint8_t vector
 * @param vector uint8_t vector
 * @param value uint64_t value
 */
void pushUint64IntoVectorUint8(std::vector<uint8_t> *vector, uint64_t value);

void bitmap_serialize(std::vector<uint8_t> *vector, bitMap bit_map, uint32_t total_bits, uint8_t significant_bits);

bitMap bitmap_deserialize(uint8_t *vector, uint32_t* idx, uint32_t total_bytes, uint8_t significant_bits);

#endif //TP2_CONVERT_TYPES_H
