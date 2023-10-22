#include "convert_types.h"

uint32_t vptr_to_uint32(void* buffer, uint32_t* idx) {
    char* data = (char*) buffer;

    uint32_t ans = ((std::uint64_t) data[(*idx)++]);
    ans |= ((std::uint64_t) data[(*idx)++]) << 8;
    ans |= ((std::uint64_t) data[(*idx)++]) << 16;
    ans |= ((std::uint64_t) data[(*idx)++]) << 24;

    return ans;
}

uint64_t vptr_to_uint64(void* buffer, uint32_t* idx) {
    char* data = (char*) buffer;

    uint64_t ans = ((std::uint64_t) data[(*idx)++]);
    ans |= ((std::uint64_t) data[(*idx)++]) << 8;
    ans |= ((std::uint64_t) data[(*idx)++]) << 16;
    ans |= ((std::uint64_t) data[(*idx)++]) << 24;
    ans |= ((std::uint64_t) data[(*idx)++]) << 32;
    ans |= ((std::uint64_t) data[(*idx)++]) << 40;
    ans |= ((std::uint64_t) data[(*idx)++]) << 48;
    ans |= ((std::uint64_t) data[(*idx)++]) << 56;

    return ans;
}

void push_uint32_into_vector_uint8(std::vector<uint8_t>* vector, uint32_t value) {
    vector->emplace_back((std::uint8_t)(value & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 8) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 16) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 24) & 0xFF));
}

void push_uint64_into_vector_uint8(std::vector<uint8_t>* vector, uint64_t value) {
    vector->emplace_back((std::uint8_t)(value & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 8) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 16) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 24) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 32) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 40) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 48) & 0xFF));
    vector->emplace_back((std::uint8_t)((value >> 56) & 0xFF));
}
