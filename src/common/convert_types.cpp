#include "convert_types.h"

uint32_t vptrToUint32(void *buffer, uint32_t *idx) {
    uint8_t *data = (uint8_t *) buffer;

    uint64_t ans = 0;
    for (int i = 0; i < 4; ++i) {
        ans |= (data[(*idx)++] << (i * 8));
    }

    return ans;
}

uint64_t vptrToUint64(void *buffer, uint32_t *idx) {
    uint8_t *data = (uint8_t *) buffer;

    uint64_t ans = 0;
    for (int i = 0; i < 8; ++i) {
        ans |= static_cast<uint64_t>(data[(*idx)++]) << (i * 8);
    }

    return ans;
}

void pushUint32IntoVectorUint8(std::vector<uint8_t> *vector, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        vector->emplace_back((value >> (i * 8)) & 0xFF);
    }
}

void pushUint64IntoVectorUint8(std::vector<uint8_t> *vector, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        vector->emplace_back((value >> (i * 8)) & 0xFF);
    }
}

void bitmap_serialize(std::vector<uint8_t> *vector, bitMap bit_map, uint32_t total_bits, uint8_t significant_bits){
    uint32_t total_ints = (total_bits - significant_bits) / 8;
    uint32_t idx;
    uint8_t value;

    for(uint32_t i = 0; i < total_ints; i++){
        idx = i * 8;

        value = 0;

        for (int j = 0; j < 8; ++j) {
            value |= ((bit_map.at(idx + j) << (7 - j)) & 0xFF);
        }

        vector->emplace_back(value);
    }

    idx = total_ints * 8;
    value = 0;

    for(uint8_t i = 0; i < significant_bits; i++){
        value |= ((bit_map.at(idx + i) << (7 - i)) & 0xFF);
    }

    vector->emplace_back(value);
}

bitMap bitmap_deserialize(uint8_t *vector, uint32_t* idx, uint32_t total_bytes, uint8_t significant_bits){
    bitMap ans = bitMap();

    uint32_t total_ints = significant_bits == 0 ? total_bytes : total_bytes - 1;
    uint8_t value;

    for(uint32_t i = 0; i < total_ints; i++){
        value = vector[(*idx)++];

        for (int j = 0; j < 8; ++j) {
            ans.push_back((value >> (7-j)) & 0x1);
        }
    }

    value = vector[(*idx)++];

    for(uint8_t i = 0; i < significant_bits; i++){
        ans.push_back((value >> (7-i)) & 0x1);
    }

    return ans;
}
