#include <openssl/aes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
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

void aes_ofb(unsigned char* buffer,unsigned long length, const char* passphrase,const unsigned char* const_ivec)
{
	// Setup the user key
	unsigned char* userkey=malloc(32);
	memset(userkey,0,32);
	strncpy((char*)userkey,passphrase,32);

	// Setup the aes key
	AES_KEY key;
	AES_set_encrypt_key(userkey,256,&key);

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
	free(userkey);
}
