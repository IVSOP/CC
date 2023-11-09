#include <iostream>
#include "convert_types.h"

uint32_t vptrToUint32(void *buffer, uint32_t *idx) {
    uint8_t *data = (uint8_t *) buffer;

    uint32_t ans = 0;
    for (int i = 0; i < 4; ++i) {
        ans |= static_cast<uint32_t>(data[(*idx)++] << (i * 8));
    }

    return ans;
}

uint64_t vptrToUint64(void *buffer, uint32_t *idx) {
    uint8_t* data = (uint8_t *) buffer;

    uint64_t ans = 0;
    for (int i = 0; i < 8; ++i) {
        uint64_t val = data[(*idx)++];

        ans |= (val << (i * 8));
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
        uint64_t val = (value >> (i * 8)) & 0xFF;

        vector->emplace_back(val);
    }
}

void bitmap_serialize(std::vector<uint8_t> *serializedData, bitMap bit_map, uint32_t total_bits, uint8_t significant_bits){
    uint32_t total_ints = (total_bits - significant_bits) / 8;
    uint32_t idx = 0;
    uint8_t value;

    for(uint32_t i = 0; i < total_ints; i++){
        value = 0;

        for (int j = 0; j < 8; ++j) {
            value |= ((bit_map.at(idx++) << (7 - j)) & 0xFF);
        }

        serializedData->emplace_back(value);
    }

    if(significant_bits > 0) {
        value = 0;

        for (uint8_t i = 0; i < significant_bits; i++) {
            value |= ((bit_map.at(idx++) << (7 - i)) & 0xFF);
        }

        serializedData->emplace_back(value);
    }
}

bitMap bitmap_deserialize(uint8_t *serializedData, uint32_t* idx, uint32_t total_bytes, uint8_t significant_bits){
    bitMap ans = bitMap();

    uint32_t total_ints = significant_bits == 0 ? total_bytes : total_bytes - 1;
    uint8_t value;

    for(uint32_t i = 0; i < total_ints; i++){
        value = serializedData[(*idx)++];

        for (int j = 0; j < 8; ++j) {
            ans.push_back((value >> (7-j)) & 0x1);
        }
    }

    if(significant_bits > 0){
        value = serializedData[(*idx)++];

        for(uint8_t i = 0; i < significant_bits; i++){
            ans.push_back((value >> (7-i)) & 0x1);
        }
    }

    return ans;
}
