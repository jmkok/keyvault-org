#ifndef _encryption_h_
#define _encryption_h_

#include <openssl/evp.h>
#include <stdint.h>

unsigned char* create_random_array(int len);
void shuffle_ivec(unsigned char ivec[16]);
//~ void aes_ofb(unsigned char* buffer,size_t len,const unsigned char passphrase[32],const unsigned char source_ivec[16]);
void aes_ofb(unsigned char* buffer, size_t len, const EVP_CIPHER *type, const unsigned char key[32], const unsigned char iv[16]);

#endif
