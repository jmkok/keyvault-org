#include <openssl/aes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "encryption.h"
#include "functions.h"

// -----------------------------------------------------------
//
// Generate a 16 character random initialization vector
//

void shuffle_ivec(unsigned char ivec[16]) {
	FILE* fp=fopen("/dev/urandom","r");
	int i,r;
	for (i=0;i<16;i++) {
		if (fp)
			fread(&r,1,sizeof(r),fp);
		else
			r=rand();
		ivec[i]^=r;
	}
	if (fp)
		fclose(fp);
}

// -----------------------------------------------------------
//
// AES encrypt/decrypt (symetrical)
//

void aes_ofb_old(unsigned char* buffer,size_t length, const unsigned char passphrase[32],const unsigned char const_ivec[16])
{
	// Setup the user key
	//~ unsigned char* userkey=malloc(32);
	//~ memset(userkey,0,32);
	//~ strncpy((char*)userkey,passphrase,32);

	// Setup the aes key
	AES_KEY key;
	AES_set_encrypt_key(passphrase,256,&key);

	// Setup the iv
	unsigned char* ivec=malloc(16);
	memset(ivec,0,16);
	if (const_ivec)
		memcpy(ivec,const_ivec,16);

	// Do the encryption
	unsigned int n = 0;
	while (length--) {
		if (n == 0)
			AES_encrypt(ivec, ivec, &key);
		*buffer = *buffer ^ ivec[n];
		buffer++;
		n = (n+1) % AES_BLOCK_SIZE;
	}

	// Cleanup
	free(ivec);
	//~ free(userkey);
}

// -----------------------------------------------------------
//
// AES encrypt/decrypt (symetrical)
//

#include <openssl/evp.h>

void aes_ofb(unsigned char* buffer,size_t length, const EVP_CIPHER *type, const unsigned char key[32], const unsigned char iv[16])
{
	unsigned char* outbuf=malloc(length);
	int outlen, tmplen;

	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);

	//~ EVP_EncryptInit_ex(&ctx, EVP_bf_cbc(), NULL, key, iv);
	EVP_EncryptInit_ex(&ctx, EVP_aes_256_ofb(), NULL, key, iv);

	if(EVP_EncryptUpdate(&ctx, outbuf, &outlen, buffer, length) == 0) {
		exit(1);
	}
	/* Buffer passed to EVP_EncryptFinal() must be after data just
	* encrypted to avoid overwriting it.
	*/
	if(EVP_EncryptFinal_ex(&ctx, outbuf + outlen, &tmplen) == 0) {
		exit(1);
	}
	outlen += tmplen;
	EVP_CIPHER_CTX_cleanup(&ctx);
	
	memcpy(buffer,outbuf,length);
}
