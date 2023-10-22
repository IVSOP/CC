#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "errors.h"

// all implementations take the first 32 bits of the answer

// SHA-1 implementation

uint32_t sha1(const void *buf, size_t len) {
	unsigned char hash[SHA_DIGEST_LENGTH]; // == 20

	const unsigned char *msg = reinterpret_cast<const unsigned char *>(buf);

	SHA1(msg, len, hash);

	// get the first 32 bits, ignore the rest
	const uint32_t *magic_ptr = reinterpret_cast<const uint32_t *>(hash);

	return *(magic_ptr);
}

// SHA-2 256 implementation

uint32_t sha2(const void *buf, size_t len) {
	unsigned char hash[SHA256_DIGEST_LENGTH];

	const unsigned char *msg = reinterpret_cast<const unsigned char *>(buf);

	SHA256(msg, len, hash);

	// get the first 32 bits, ignore the rest
	const uint32_t *magic_ptr = reinterpret_cast<const uint32_t *>(hash);

	return *(magic_ptr);
}

// SHA-3 256 implementation

uint32_t sha3(const void *buf, size_t len) {
	const int size = EVP_MD_size(EVP_sha3_256());
	unsigned char *hash = reinterpret_cast<unsigned char *>(alloca(size));

	const unsigned char *msg = reinterpret_cast<const unsigned char *>(buf);

	EVP_MD_CTX *mdctx;

	if((mdctx = EVP_MD_CTX_new()) == NULL) {
		print_error("Error init context");
	}

	if(1 != EVP_DigestInit_ex(mdctx, EVP_sha3_256(), NULL)) {
		print_error("Error init context");
	}

	if(1 != EVP_DigestUpdate(mdctx, msg, len)) {
		print_error("Error digesting");
	}

	if(1 != EVP_DigestFinal_ex(mdctx, hash, NULL)) {
		print_error("Error finalizing");
	}

	EVP_MD_CTX_free(mdctx);

	// get the first 32 bits, ignore the rest
	const uint32_t *magic_ptr = reinterpret_cast<const uint32_t *>(hash);

	return *(magic_ptr);
}


#endif


/*

	std::string result;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        result += hex;
    }

*/
