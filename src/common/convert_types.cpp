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
