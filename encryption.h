#ifndef _encryption_h_
#define _encryption_h_

#include <openssl/evp.h>
#include <stdint.h>

void shuffle_ivec(unsigned char ivec[16]);
void evp_cipher(const EVP_CIPHER *type, unsigned char* buffer, size_t len, const unsigned char key[32], const unsigned char iv[16]);

#endif
