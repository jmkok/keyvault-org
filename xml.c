#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <glib/gprintf.h>

#include <string.h>
//~ #include <openssl/x509.h>
#include <openssl/evp.h>
//~ #include <openssl/hmac.h>

#include <assert.h>

#include "main.h"
#include "encryption.h"
#include "functions.h"
#include "xml.h"

#define CIPHER_RC4 (1<<0)
#define CIPHER_AES_128_OFB (1<<1)
#define CIPHER_AES_192_OFB (1<<2)
#define CIPHER_AES_256_OFB (1<<3)

const int default_cipher_list = CIPHER_RC4 | CIPHER_AES_256_OFB;

xmlAttr* xmlNewPropInteger(xmlNode* node, const xmlChar* name, const int value) {
	gchar* tmp = malloc(32);
	g_sprintf(tmp, "%u", value);
	xmlAttr* attrib = xmlNewProp(node, name, BAD_CAST tmp);
	g_free(tmp);
	return attrib;
}

xmlAttr* xmlNewPropBase64(xmlNodePtr node, const xmlChar* name, const void* data, const int size) {
	gchar* tmp = g_base64_encode(data, size);
	xmlAttr* attrib = xmlNewProp(node, name, BAD_CAST tmp);
	g_free(tmp);
	return attrib;
}

// -------------------------------------------------------------------------------
//
// Encrypt an xmlDoc
//

xmlDoc* xml_doc_encrypt(xmlDoc* doc, const char* passphrase) {
	xmlChar* xml_text;
	int xml_size;
	unsigned char* rc4_ivec = (unsigned char*)create_random_password(16);
	shuffle_ivec(rc4_ivec);
	unsigned char* aes_ivec = (unsigned char*)create_random_password(16);
	shuffle_ivec(aes_ivec);
	xmlDocDumpFormatMemory(doc, &xml_text, &xml_size, 1);

	// Create the doc
	xmlDoc* enc_doc = xmlNewDoc(BAD_CAST "1.0"); 

	// Create the root of the doc
	xmlNode* root = xmlNewNode(NULL, BAD_CAST "keyvault");
	xmlNewProp(root, BAD_CAST "encrypted", BAD_CAST "yes");
	xmlDocSetRootElement(enc_doc, root);

	// PBKDF2 the passphrase
	char* pbkdf2_salt = create_random_password(32);
	unsigned int pbkdf2_rounds = 20000 + (random_integer() % 1000);
	u_char encryption_key[32];
	if (pbkdf2_rounds) {
		PKCS5_PBKDF2_HMAC_SHA1(
			passphrase, strlen(passphrase),
			(unsigned char*)pbkdf2_salt, strlen(pbkdf2_salt), 
			pbkdf2_rounds, 32, encryption_key);
	}
	else {
		memset(encryption_key,0,32);
		strncpy((char*)encryption_key,passphrase,32);
	}
	hexdump("encryption_key",encryption_key,32);

	// Store the pbkdf2 setup
	xmlNode* pbkdf2_node = xmlNewChild(root, NULL, BAD_CAST "pbkdf2", NULL);
	xmlNewProp(pbkdf2_node, BAD_CAST "salt", BAD_CAST pbkdf2_salt);
	xmlNewPropInteger(pbkdf2_node, BAD_CAST "rounds", pbkdf2_rounds);

	// Encrypt the data using RC4
	evp_cipher(EVP_rc4(), xml_text, xml_size, encryption_key, rc4_ivec);
	xmlNode* encryption_node = xmlNewChild(root, NULL, BAD_CAST "encryption", NULL);
	xmlNewProp(encryption_node, BAD_CAST "cipher", BAD_CAST "rc4");
	xmlNewPropBase64(encryption_node, BAD_CAST "ivec", rc4_ivec, 16);

	// Encrypt the data using AES_256_OFB
	evp_cipher(EVP_aes_256_ofb(), xml_text, xml_size, encryption_key, aes_ivec);
	encryption_node = xmlNewChild(root, NULL, BAD_CAST "encryption", NULL);
	xmlNewProp(encryption_node, BAD_CAST "cipher", BAD_CAST "aes_256_ofb");
	xmlNewPropBase64(encryption_node, BAD_CAST "ivec", BAD_CAST aes_ivec, 16);

	// Add the encrypted data
	gchar* tmp=g_base64_encode((unsigned char*)xml_text, xml_size);
	xmlNode* data = xmlNewChild(root, NULL, BAD_CAST "data", BAD_CAST tmp);
	g_free(tmp);
	xmlNewPropInteger(data, BAD_CAST "size", xml_size);

	return enc_doc;
}

// -------------------------------------------------------------------------------
//
// Decrypt an xmlDoc
//

xmlDoc* xml_doc_decrypt(xmlDoc* doc, const gchar* passphrase) {
	// get the root
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlElemDump(stdout, NULL, root);puts("");

	// Test to see if it is encrypted anyway
	xmlChar* encrypted = xmlGetProp(root, BAD_CAST "encrypted");
	if (!encrypted)
		return doc;
	xmlFree(encrypted);

	// Get the data node
	xmlNode* data_node = xmlFindNode(root, BAD_CAST "data", 0);
	if (!data_node)
		return doc;

	// Read the data for this node
	char* size_text = (char*)xmlGetProp(data_node, BAD_CAST "size");
	int size = atol(size_text);
	printf("size: %u\n", size);

	// Read the data
	gsize data_len;
	char* data_base64 = (char*)xmlNodeGetContent(data_node);
	guchar* data = g_base64_decode(data_base64, &data_len);
	printf("data_len: %u\n", data_len);

	// Read the PBKDF2 proporties
	xmlNode* pbkdf2_node = xmlFindNode(root, BAD_CAST "pbkdf2", 0);
	assert(pbkdf2_node);
	char* pbkdf2_salt = (char*)xmlGetProp(pbkdf2_node, BAD_CAST "salt");
	assert(pbkdf2_salt);
	char* pbkdf2_rounds_text = (char*)xmlGetProp(pbkdf2_node, BAD_CAST "rounds");
	assert(pbkdf2_rounds_text);
	int pbkdf2_rounds = atol(pbkdf2_rounds_text);
	
	// PBKDF2 the passphrase
	u_char encryption_key[32];
	if (pbkdf2_rounds) {
		PKCS5_PBKDF2_HMAC_SHA1(
			passphrase, strlen(passphrase),
			(unsigned char*)pbkdf2_salt, strlen(pbkdf2_salt), 
			pbkdf2_rounds, 32, encryption_key);
	}
	else {
		memset(encryption_key,0,32);
		strncpy((char*)encryption_key,passphrase,32);
	}

	// Decrypt the data
	xmlNode* node = root->children;
	while(node) {
		printf("> %s\n",node->name);
		if (node->type == XML_ELEMENT_NODE && xmlStrEqual(node->name, BAD_CAST "encryption")) {
			xmlChar* cipher = xmlGetProp(node, BAD_CAST "cipher");
			char* ivec_base64 = (char*)xmlGetProp(node, BAD_CAST "ivec");
			gsize ivec_len;
			unsigned char* ivec = g_base64_decode(ivec_base64, &ivec_len);
			hexdump("ivec", ivec, (int)ivec_len);
			if (xmlStrEqual(cipher, BAD_CAST "rc4"))
				evp_cipher(EVP_rc4(), data, data_len, encryption_key, ivec);
			if (xmlStrEqual(cipher, BAD_CAST "aes_256_ofb"))
				evp_cipher(EVP_aes_256_ofb(), data, data_len, encryption_key, ivec);
			xmlFree(cipher);
		}
		node = node->next;
	}

	// Convert into xml
	xmlDoc* doc_dec = xmlParseMemory((char*)data, data_len);

	return doc_dec;
}

// -------------------------------------------------------------------------------
//
// Locate a specific node
//

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth) {
	//~ debugf("soapFindNode('%s',%u) in '%s' \n", name, depth, root_node->name);
	xmlNode* node = root_node->children;
	while (node) {
		//~ debugf("found> %s\n", node->name);fflush(stdout);
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrEqual(node->name, name)) {
				return node;
			}
			if (depth) {
				xmlNode* sub_node = xmlFindNode(node, name, depth-1);
				if (sub_node)
					return sub_node;
			}
		}
		node = node->next;
	}
	return NULL;
}

// -------------------------------------------------------------------------------
//
// helper
//

static int xmlCharFreeAndReturnInteger(xmlChar* text) {
	if (text) {
		int retval = atol((char*)text);
		xmlFree(text);
		return retval;
	}
	return 0;
}

// -------------------------------------------------------------------------------
//
// Extracte and duplicate data from a node
//

xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node)
		return xmlNodeGetContent(node);
	return NULL;
}

int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node) {
	xmlChar* text = xmlNodeGetContent(node);
		return xmlCharFreeAndReturnInteger(text);
	}
	return 0;
}

// OLD

/*
xmlNode* xmlFindNode(xmlNode* root_node, xmlChar* name, int depth) {
	//~ printf("soapFindNode('%s',%u) in '%s' \n", name, depth, root_node->name);
	xmlNode* node = root_node->children;
	while (node) {
		//~ printf("found> %s\n", node->name);fflush(stdout);
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrEqual(node->name, name)) {
				return node;
			}
			if (depth) {
				xmlNode* sub_node = xmlFindNode(node, name, depth-1);
				if (sub_node)
					return sub_node;
			}
		}

		node = node->next;
	}
	return NULL;
}

char* xmlNodeContentsText(xmlNode* root_node, xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node && node->children && node->children->content)
		return strdup((char*)node->children->content);
	return NULL;
}

int xmlNodeContentsInteger(xmlNode* root_node, xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node && node->children && node->children->content)
		return atol((char*)node->children->content);
	return 0;
}
*/
