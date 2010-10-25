#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/evp.h>

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
// Symetrical encryption
//

void evp_cipher(const EVP_CIPHER *type, unsigned char* buffer,size_t length, const unsigned char key[32], const unsigned char iv[16])
{
	unsigned char* outbuf=malloc(length);
	int outlen, tmplen;

	// Initialize the engine
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);
	EVP_EncryptInit_ex(&ctx, type, NULL, key, iv);

	// Encrypt the data
	if(EVP_EncryptUpdate(&ctx, outbuf, &outlen, buffer, length) == 0)
		exit(1);
	if(EVP_EncryptFinal_ex(&ctx, outbuf + outlen, &tmplen) == 0)
		exit(1);

	// Cleanup
	EVP_CIPHER_CTX_cleanup(&ctx);

	// Store the encrypted data
	memcpy(buffer, outbuf, length);
	free(outbuf);
}

// -----------------------------------------------------------
//
// Symetrical encryption
//

void read_random(void* ptr, int len) {
	static FILE* fp = NULL;
	if (!fp) fp=fopen("/dev/urandom","r");
	fread(ptr,1,len,fp);
}

unsigned int random_integer() {
	unsigned int x;
	read_random(&x,sizeof(x));
	return x;
}

// -----------------------------------------------------------
//
// create a random password
//

gchar* create_random_password(int len) {
	char random_charset[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	gchar* password=g_malloc(len+1);
	memset(password,0,len+1);
	FILE* fp=fopen("/dev/urandom","r");
	int i,r;
	for (i=0;i<len;i++) {
		if (fp)
			fread(&r,1,sizeof(r),fp);
		else
			r=rand();
		char p=random_charset[r%strlen(random_charset)];
		password[i]=p;
	}
	if (fp)
		fclose(fp);
	return password;
}
