#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "errors.h"

// all implementations take the first 32 bits of the answer

// SHA-1 implementation

uint32_t sha1(const void *buf, size_t len);

// SHA-2 256 implementation

uint32_t sha2(const void *buf, size_t len);

// SHA-3 256 implementation

uint32_t sha3(const void *buf, size_t len);


#endif


/*

	std::string result;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        result += hex;
    }

*/
