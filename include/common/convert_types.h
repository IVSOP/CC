#ifndef TP2_CONVERT_TYPES_H
#define TP2_CONVERT_TYPES_H

#include <cstdint>
#include <vector>

uint32_t vptr_to_uint32(void* buffer, uint32_t* idx);
uint64_t vptr_to_uint64(void* buffer, uint32_t* idx);
void push_uint32_into_vector_uint8(std::vector<uint8_t>* vector, uint32_t value);
void push_uint64_into_vector_uint8(std::vector<uint8_t>* vector, uint64_t value);

#endif //TP2_CONVERT_TYPES_H
