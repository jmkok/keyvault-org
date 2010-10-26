#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/evp.h>

#include "main.h"
#include "encryption.h"
#include "functions.h"
#include "gtk_dialogs.h"

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
// Random functions
//

static FILE* random_handle;

void random_init(void) {
 	random_handle = fopen("/dev/urandom","r");
 	if (!random_handle) {
		gtk_error_dialog(NULL, "Could not open the handle to the random file.\nYou are still capable of opening the file.\nSaving the file and creating password is insecure !!!", "Error initializing random number generator");
		srand(time(NULL));
	}
}

static void read_random(void* ptr, int len) {
	if (random_handle) {
		fread(ptr, 1, len, random_handle);
	}
	else {
		int i;
		for (i=0;i<len;i++) {
			((char*)ptr)[i] = rand();
		}
	}
}

// -----------------------------------------------------------
//
// Create a random password
//

gchar* create_random_password(int len) {
	char random_charset[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	gchar* password=g_malloc(len+1);
	memset(password,0,len+1);
	int i;
	for (i=0;i<len;i++) {
		int idx = random_integer(strlen(random_charset));
		password[i] = random_charset[idx];
	}
	return password;
}

// -----------------------------------------------------------
//
// Shuffle some bytes
//

//~ static void shuffle_data(void* data, int len) {
	//~ unsigned char* xor = malloc(len);
	//~ read_random(xor,len);
	//~ int i;
	//~ for (i=0;i<16;i++) {
		//~ ((unsigned char*)data)[i] ^= xor[i];
	//~ }
//~ }

// -----------------------------------------------------------
//
// Allocate random memory
//

void* malloc_random(int len) {
	void* data = malloc(len);
	read_random(data,len);
	return data;
}

// -----------------------------------------------------------
//
// Create an random integer
//

unsigned int random_integer(unsigned int max) {
	unsigned int x;
	read_random(&x,sizeof(x));
	return x % max;
}
