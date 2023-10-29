#ifndef TP2_CONVERT_TYPES_H
#define TP2_CONVERT_TYPES_H

#include <cstdint>
#include <vector>

/**
 * Reads data from a buffer and converts it to an uint32_t
 * @param buffer Buffer
 * @param idx Current reading index
 * @return The read uint32_t
 */
uint32_t vptr_to_uint32(void *buffer, uint32_t *idx);

/**
 * Reads data from a buffer and converts it to an uint64_t
 * @param buffer Buffer
 * @param idx Current reading index
 * @return The read uint32_t
 */
uint64_t vptr_to_uint64(void *buffer, uint32_t *idx);

/**
 * Pushes an uint32_t value into an uint8_t vector
 * @param vector uint8_t vector
 * @param value uint32_t value
 */
void push_uint32_into_vector_uint8(std::vector<uint8_t> *vector, uint32_t value);

/**
 * Pushes an uint64_t value into an uint8_t vector
 * @param vector uint8_t vector
 * @param value uint64_t value
 */
void push_uint64_into_vector_uint8(std::vector<uint8_t> *vector, uint64_t value);

#endif //TP2_CONVERT_TYPES_H
