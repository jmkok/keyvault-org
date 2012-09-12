#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <openssl/evp.h>
#include <libxml/parser.h>

#include "encryption.h"
#include "functions.h"
#include "xml.h"
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
		die("EVP_EncryptUpdate");
	if(EVP_EncryptFinal_ex(&ctx, outbuf + outlen, &tmplen) == 0)
		die("EVP_EncryptFinal_ex");

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

static FILE* __random_handle__;

void random_init(void) {
	__random_handle__ = fopen("/dev/urandom","r");
	if (!__random_handle__) {
		gtk_error("Could not open the handle to the random file.\nYou are still capable of opening the file.\nSaving the file and creating passwords is insecure !!!");
	}

	// Initialize random generator
	struct timeval tv;
	gettimeofday(&tv,0);
	srand(tv.tv_sec ^ tv.tv_usec);
}

static void read_random(void* ptr, int len) {
	if (__random_handle__) {
		fread(ptr, 1, len, __random_handle__);
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
	gchar* password = g_malloc(len+1);
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

// -------------------------------------------------------------------------------
//
// Encrypt an xmlDoc
//

xmlDoc* xml_doc_encrypt(xmlDoc* doc, const unsigned char passphrase_key[32]) {
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlNode* encrypted_node = xmlNodeEncrypt(root, passphrase_key, "rc4,aes_256_ofb");
	if (!encrypted_node) {
		gtk_error("Could not encrypt file");
		return NULL;
	}
	// Create the doc
	xmlDoc* encrypted_doc = xmlNewDoc(CONST_BAD_CAST "1.0"); 
	xmlDocSetRootElement(encrypted_doc, encrypted_node);
	return encrypted_doc;
}

// -------------------------------------------------------------------------------
//
// Encrypt an xmlDoc (OLD)
//

xmlDoc* xml_doc_encrypt_old(xmlDoc* doc, const unsigned char passphrase_key[32]) {
	xmlChar* xml_text;
	int xml_size;
	unsigned char* rc4_ivec = malloc_random(16);
	unsigned char* aes_ivec = malloc_random(16);
	xmlDocDumpFormatMemory(doc, &xml_text, &xml_size, 1);

	// Create the doc
	xmlDoc* enc_doc = xmlNewDoc(CONST_BAD_CAST "1.0"); 

	// Create the root of the doc
	xmlNode* root = xmlNewNode(NULL, CONST_BAD_CAST "keyvault");
	xmlNewProp(root, CONST_BAD_CAST "encrypted", CONST_BAD_CAST "yes");
	xmlDocSetRootElement(enc_doc, root);

	// PBKDF2 the passphrase
	char* pbkdf2_salt = create_random_password(32);
	unsigned int pbkdf2_rounds = 20000 + random_integer(1000);
	u_char encryption_key[32];
	if (pbkdf2_rounds) {
		PKCS5_PBKDF2_HMAC_SHA1(
			(const char*)passphrase_key, 32,
			(unsigned char*)pbkdf2_salt, strlen(pbkdf2_salt), 
			pbkdf2_rounds, 32, encryption_key);
	}
	else {
		memcpy(encryption_key, passphrase_key, 32);
	}
	printf("encryption_key >");
	hexdump(encryption_key,32);

	// Store the pbkdf2 setup
	xmlNode* pbkdf2_node = xmlNewChild(root, NULL, CONST_BAD_CAST "pbkdf2", NULL);
	xmlNewProp(pbkdf2_node, CONST_BAD_CAST "salt", BAD_CAST pbkdf2_salt);
	xmlNewIntegerProp(pbkdf2_node, CONST_BAD_CAST "rounds", pbkdf2_rounds);

	// Encrypt the data using RC4
	evp_cipher(EVP_rc4(), xml_text, xml_size, encryption_key, rc4_ivec);
	xmlNode* encryption_node = xmlNewChild(root, NULL, CONST_BAD_CAST "encryption", NULL);
	xmlNewProp(encryption_node, CONST_BAD_CAST "cipher", CONST_BAD_CAST "rc4");
	xmlNewBase64Prop(encryption_node, CONST_BAD_CAST "ivec", rc4_ivec, 16);

	// Encrypt the data using AES_256_OFB
	evp_cipher(EVP_aes_256_ofb(), xml_text, xml_size, encryption_key, aes_ivec);
	encryption_node = xmlNewChild(root, NULL, CONST_BAD_CAST "encryption", NULL);
	xmlNewProp(encryption_node, CONST_BAD_CAST "cipher", CONST_BAD_CAST "aes_256_ofb");
	xmlNewBase64Prop(encryption_node, CONST_BAD_CAST "ivec", BAD_CAST aes_ivec, 16);

	// Add the encrypted data
	gchar* tmp=g_base64_encode((unsigned char*)xml_text, xml_size);
	xmlNode* data = xmlNewTextChild(root, NULL, CONST_BAD_CAST "data", BAD_CAST tmp);
	g_free(tmp);
	xmlNewIntegerProp(data, CONST_BAD_CAST "size", xml_size);

	return enc_doc;
}

// -------------------------------------------------------------------------------
//
// Decrypt an xmlDoc
//

xmlDoc* xml_doc_decrypt(xmlDoc* doc, const unsigned char passphrase_key[32]) {
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlNode* decrypted_node = xmlNodeDecrypt(root, passphrase_key);
	if (!decrypted_node) {
		gtk_error("Could not decrypt file");
		return NULL;
	}
	// Create the doc
	xmlDoc* decrypted_doc = xmlNewDoc(CONST_BAD_CAST "1.0"); 
	xmlDocSetRootElement(decrypted_doc, decrypted_node);
	return decrypted_doc;
}

// -------------------------------------------------------------------------------
//
// Decrypt an xmlDoc (OLD)
//

xmlDoc* xml_doc_decrypt_old(xmlDoc* doc, const unsigned char passphrase_key[32]) {
	// get the root
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlNodeShow(root);

	// Test to see if it is encrypted anyway
	xmlChar* encrypted = xmlGetProp(root, CONST_BAD_CAST "encrypted");
	if (!encrypted)
		return doc;
	xmlFree(encrypted);

	// Get the data node
	xmlNode* data_node = xmlFindNode(root, CONST_BAD_CAST "data", 0);
	if (!data_node)
		return doc;

	// Read the data for this node
	char* size_text = (char*)xmlGetProp(data_node, CONST_BAD_CAST "size");
	int size = atol(size_text);
	printf("size: %u\n", size);

	// Read the data
	gsize data_len;
	char* data_base64 = (char*)xmlNodeGetContent(data_node);
	guchar* data = g_base64_decode(data_base64, &data_len);
	printf("data_len: %i\n", (int)data_len);	// Casting to (int) prevents warnings on 32/64 systems

	// Read the PBKDF2 proporties
	xmlNode* pbkdf2_node = xmlFindNode(root, CONST_BAD_CAST "pbkdf2", 0);
	assert(pbkdf2_node);
	char* pbkdf2_salt = (char*)xmlGetProp(pbkdf2_node, CONST_BAD_CAST "salt");
	assert(pbkdf2_salt);
	char* pbkdf2_rounds_text = (char*)xmlGetProp(pbkdf2_node, CONST_BAD_CAST "rounds");
	assert(pbkdf2_rounds_text);
	int pbkdf2_rounds = atol(pbkdf2_rounds_text);
	
	// PBKDF2 the passphrase
	u_char encryption_key[32];
	if (pbkdf2_rounds) {
		PKCS5_PBKDF2_HMAC_SHA1(
			(const char*)passphrase_key, 32,
			(unsigned char*)pbkdf2_salt, strlen(pbkdf2_salt), 
			pbkdf2_rounds, 32, encryption_key);
	}
	else {
		memcpy(encryption_key, passphrase_key, 32);
	}

	// Decrypt the data
	xmlNode* node = root->children;
	while(node) {
		printf("> %s\n",node->name);
		if (node->type == XML_ELEMENT_NODE && xmlStrEqual(node->name, CONST_BAD_CAST "encryption")) {
			xmlChar* cipher = xmlGetProp(node, CONST_BAD_CAST "cipher");
			char* ivec_base64 = (char*)xmlGetProp(node, CONST_BAD_CAST "ivec");
			gsize ivec_len;
			unsigned char* ivec = g_base64_decode(ivec_base64, &ivec_len);
			printf("ivec"); hexdump(ivec, (int)ivec_len);
			if (xmlStrEqual(cipher, CONST_BAD_CAST "rc4"))
				evp_cipher(EVP_rc4(), data, data_len, encryption_key, ivec);
			if (xmlStrEqual(cipher, CONST_BAD_CAST "aes_256_ofb"))
				evp_cipher(EVP_aes_256_ofb(), data, data_len, encryption_key, ivec);
			xmlFree(cipher);
		}
		node = node->next;
	}

	// Convert into xml
	xmlDoc* doc_dec = xmlParseMemory((char*)data, data_len);
	xmlDocShow(doc_dec);

	return doc_dec;
}

extern void pkcs5_pbkdf2_hmac_sha1(const char* passphrase, const char* salt, int rounds, unsigned char key[32]) {
	PKCS5_PBKDF2_HMAC_SHA1(
		passphrase, strlen(passphrase),
		(const unsigned char*)salt, strlen(salt), 
		rounds, 32, key);
}
