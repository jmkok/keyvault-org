#ifndef _encryption_h_
#define _encryption_h_

#include <stdint.h>
#include <openssl/evp.h>
#include <libxml/parser.h>

extern void evp_cipher(const EVP_CIPHER *type, unsigned char* buffer, size_t len, const unsigned char key[32], const unsigned char iv[16]);

extern void random_init(void);
extern void* malloc_random(int len);
extern unsigned int random_integer(unsigned int max);

extern gchar* create_random_password(int len);

extern xmlDoc* xml_doc_encrypt(xmlDoc* doc, const gchar* passphrase);
extern xmlDoc* xml_doc_decrypt(xmlDoc* doc, const gchar* passphrase);

//~ extern void shuffle_data(void* data, int len);
//~ extern void read_random(void* ptr, int len);

#endif
