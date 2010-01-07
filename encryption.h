#ifndef _encryption_h_
#define _encryption_h_

unsigned char* create_random_array(ssize_t len);
void shuffle_ivec(unsigned char ivec[16]);
void aes_ofb(void* buffer,size_t len,const char* passphrase,const unsigned char* source_ivec);

#endif
