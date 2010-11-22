#include <assert.h>
#include <string.h>
#include <libxml/parser.h>
#include <glib/gprintf.h>
#include <openssl/evp.h>

#include "xml.h"
#include "encryption.h"
#include "functions.h"
#include "list.h"

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
// Read proiporties of a node
//

xmlChar* xmlGetPropBase64(xmlNode* node, const xmlChar* name) {
	char* tmp = (char*)xmlGetProp(node, name);
	gsize data_len;
	return g_base64_decode(tmp, &data_len);
}

int xmlGetPropInteger(xmlNode* node, const xmlChar* name) {
	char* tmp = (char*)xmlGetProp(node, name);
	if (!tmp) 
		return 0;
	int retval = atoi(tmp);
	free(tmp);
	return retval;
}

// -------------------------------------------------------------------------------
//
// Read the contents of a node
//

xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, name, 0);
	if (node)
		return xmlNodeGetContent(node);
	return NULL;
}

xmlChar* xmlGetContentsBase64(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = root_node;
	if (name)
		node = xmlFindNode(root_node, name, 0);
	if (!node)
		return NULL;
	xmlChar* tmp = xmlNodeGetContent(node);
	gsize data_len;
	return g_base64_decode((void*)tmp, &data_len);
}


int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node) {
	xmlChar* text = xmlNodeGetContent(node);
		return xmlCharFreeAndReturnInteger(text);
	}
	return 0;
}

// -------------------------------------------------------------------------------
//
// Convert a text string into a specific EVP_CIPHER
//

const EVP_CIPHER* evp_cipher_text(const char* text) {
	if (strcmp(text,"null") == 0)					return EVP_enc_null();
	if (strcmp(text,"rc4") == 0)					return EVP_rc4();
	if (strcmp(text,"des_ofb") == 0)			return EVP_des_ofb();
	if (strcmp(text,"aes_128_ofb") == 0)	return EVP_aes_128_ofb();
	if (strcmp(text,"aes_192_ofb") == 0)	return EVP_aes_192_ofb();
	if (strcmp(text,"aes_256_ofb") == 0)	return EVP_aes_256_ofb();
	// No match yet, then we have a serious problem
	return NULL;
}

#define NODE_NAME BAD_CAST "vault"
#define NODE_NS BAD_CAST "http://keyvault.org"


// -------------------------------------------------------------------------------
//
// Encrypt an xmlNode
//

xmlNode* xmlNodeEncrypt(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols) {
	// Get the raw data...
	xmlBuffer* xbuf = xmlBufferCreate();
	xmlNodeDump(xbuf, NULL, node, 0, 1);
	if (!protocols)
		protocols = "rc4,aes_128_ofb";

	//~ xmlDoc* doc = xmlReadDoc(xbuf->content,NULL,NULL,0);
	//~ xmlDocFormatDump(stdout, doc, 1);

	// Create the "vault" node
	xmlNode* enode = xmlNewNode(NULL, NODE_NAME);
	xmlNs* ns = xmlNewNs(enode, NODE_NS, NULL);

	// Copy the passphrase key into the encryption key
	u_char encryption_key[32];
	memcpy(encryption_key, passphrase_key, 32);

	// PBKDF2 the passphrase
	char* pbkdf2_salt = malloc_random(32);
	unsigned int pbkdf2_rounds = 20000 + random_integer(1000);
	if (pbkdf2_rounds) {
		PKCS5_PBKDF2_HMAC_SHA1(
			(char*)passphrase_key, 32,
			(unsigned char*)pbkdf2_salt, 32,
			pbkdf2_rounds, 32, encryption_key);
	}
	hexdump(encryption_key, 32);

	// Store the pbkdf2 setup
	xmlNode* pbkdf2_node = xmlNewChild(enode, ns, BAD_CAST "pbkdf2", NULL);
	xmlNewPropBase64(pbkdf2_node, BAD_CAST "salt", BAD_CAST pbkdf2_salt, 32);
	xmlNewPropInteger(pbkdf2_node, BAD_CAST "rounds", pbkdf2_rounds);

	void evp_cipher_protocol(tList* list, void* data) {
		char* protocol = data;
		const EVP_CIPHER* cipher = evp_cipher_text(protocol);
		unsigned char* ivec = malloc_random(16);
		evp_cipher(cipher, xbuf->content, xbuf->use+1, encryption_key, ivec);
		xmlNode* encryption_node = xmlNewChild(enode, ns, BAD_CAST "encryption", NULL);
		xmlNewProp(encryption_node, BAD_CAST "cipher", BAD_CAST protocol);
		xmlNewPropBase64(encryption_node, BAD_CAST "ivec", ivec, 16);
	}

	tList* proto_list = listExplode(protocols,",");
	listForeach(proto_list, evp_cipher_protocol);
	listDestroy(proto_list);

	// Add the encrypted data
	gchar* tmp=g_base64_encode((unsigned char*)xbuf->content, xbuf->use+1);
	xmlNode* data_node = xmlNewChild(enode, ns, BAD_CAST "data", BAD_CAST tmp);
	g_free(tmp);
	xmlNewPropInteger(data_node, BAD_CAST "size", xbuf->use+1);

	// Return the encrypted node
	return enode;
}

// -------------------------------------------------------------------------------
//
// Decrypt an xmlNode
//

xmlNode* xmlNodeDecrypt(xmlNode* root, const unsigned char passphrase_key[32]) {
	// Test to see if it is a keyvault.org node
	printf("root->name: %s\n",root->name);
	if (!xmlStrEqual(root->name, NODE_NAME))
		return NULL;
	if (!root || !root->nsDef || !root->nsDef->href)
		return NULL;
	printf("root->nsDef->href: %s\n",root->nsDef->href);
	if (!xmlStrEqual(root->nsDef->href, NODE_NS))
		return NULL;

	// <pbkdf2>
	u_char encryption_key[32];
	memcpy(encryption_key, passphrase_key, 32);
	xmlNode* node = root->children;
	while(node) {
		if (xmlStrEqual(node->name, BAD_CAST "pbkdf2")) {
			unsigned char* pbkdf2_salt = xmlGetPropBase64(node, BAD_CAST "salt");
			unsigned int pbkdf2_rounds = xmlGetPropInteger(node, BAD_CAST "rounds");
			if (pbkdf2_rounds) {
				PKCS5_PBKDF2_HMAC_SHA1(
					(char*)passphrase_key, 32,
					pbkdf2_salt, 32, 
					pbkdf2_rounds, 32, encryption_key);
			}
			free(pbkdf2_salt);
			hexdump(encryption_key, 32);
		}
		node = node->next;
	}

	// <data>
	void* data = NULL;
	int size = 0;
	node = root->children;
	while(node) {
		if (xmlStrEqual(node->name, BAD_CAST "data")) {
			xmlElemDump(stdout, NULL, node);
			size = xmlGetPropInteger(node, BAD_CAST "size");
			data = xmlGetContentsBase64(node, NULL);
		}
		node = node->next;
	}

	// walk all <encryption> nodes
	node = root->children;
	while(node) {
		if (xmlStrEqual(node->name, BAD_CAST "encryption")) {
			xmlElemDump(stdout, NULL, node);
			char* protocol = (char*) xmlGetProp(node, BAD_CAST "cipher");
			unsigned char* ivec = xmlGetPropBase64(node, BAD_CAST "ivec");
			const EVP_CIPHER* cipher = evp_cipher_text(protocol);
			evp_cipher(cipher, data, size, encryption_key, ivec);
		}
		node = node->next;
	}

	// Convert the xml text into an xmlNode object
	xmlDoc* doc = xmlReadDoc(data,NULL,NULL,0);
	xmlDocFormatDump(stdout, doc, 1);
	xmlNode* xnode = xmlDocGetRootElement(doc);
	xmlUnlinkNode(xnode);
	xmlFreeDoc(doc);
	return xnode;
}

// -------------------------------------------------------------------------------
//
// 
//

int xmlIsNodeEncrypted(xmlNode* root) {
	// Test to see if it is a keyvault.org node
	printf("root->name: %s\n",root->name);
	if (!xmlStrEqual(root->name, NODE_NAME))
		return 0;
	if (!root || !root->nsDef || !root->nsDef->href)
		return 0;
	printf("root->nsDef->href: %s\n",root->nsDef->href);
	if (!xmlStrEqual(root->nsDef->href, NODE_NS))
		return 0;
	return 1;
}

// -------------------------------------------------------------------------------
//
// 
//

xmlNode* xmlNewChildEncrypted(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const xmlChar* value, const unsigned char passphrase_key[32]) {
	unsigned char* ivec = malloc_random(16);
	int len = (xmlStrlen(value) & ~63) + 64;
	unsigned char* enlarged_value = mallocz(len+1);
	strcpy((char*)enlarged_value, (char*)value);
	evp_cipher(EVP_aes_256_ofb(), enlarged_value, len, passphrase_key, ivec);
	gchar* tmp = g_base64_encode(enlarged_value, len);
	xmlNode* node = xmlNewChild(parent, ns, name, BAD_CAST tmp);
	xmlNewProp(node, BAD_CAST "encrypted", BAD_CAST "yes");
	xmlNewPropBase64(node, BAD_CAST "ivec", ivec, 16);
	g_free(tmp);
	free(enlarged_value);
	return node;
}

xmlNode* xmlNewChildInteger(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const int value) {
	gchar* tmp = malloc(32);
	g_sprintf(tmp, "%u", value);
	xmlNode* node = xmlNewChild(parent, ns, name, BAD_CAST tmp);
	g_free(tmp);
	return node;
}

xmlNode* xmlNewChildBase64(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const void* data, const int size) {
	gchar* tmp = g_base64_encode(data, size);
	xmlNode* node = xmlNewChild(parent, ns, name, BAD_CAST tmp);
	g_free(tmp);
	return node;
}

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
// Extracte and duplicate data from a node
//

xmlChar* xmlGetContentsEncrypted(xmlNode* root_node, const xmlChar* name, const unsigned char passphrase_key[32]) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (!node) return NULL;

	xmlChar* rawdata = xmlNodeGetContent(node);
	if (!rawdata) return NULL;

	xmlChar* encrypted = xmlGetProp(node, BAD_CAST "encrypted");
	if (!encrypted) return rawdata;
	gsize data_len;
	unsigned char* data = g_base64_decode((char*)rawdata, &data_len);
	xmlFree(encrypted);
	xmlFree(rawdata);
	//~ #undef rawdata
	//~ rawdata=malloc(10);

	xmlChar* rawivec = xmlGetProp(node, BAD_CAST "ivec");
	gsize ivec_len;
	unsigned char* ivec = g_base64_decode((char*)rawivec, &ivec_len);
	assert(ivec_len == 16);
	printf("ivec");hexdump(ivec, ivec_len);
	evp_cipher(EVP_aes_256_ofb(), data, data_len, passphrase_key, ivec);
	printf("data");hexdump(data, data_len);
	
	return data;
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

void encryption_test(void) {
	unsigned char* pass = malloc_random(16);
	xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
	xmlNode* config = xmlNewNode(NULL, BAD_CAST "config");
	xmlDocSetRootElement(doc, config);
	int i;
	for (i=0;i<3;i++) {
		xmlNode* node = xmlNewChild(config, NULL, BAD_CAST "node", NULL);
		xmlNewChild(node, NULL, BAD_CAST "title", BAD_CAST "Title 1");
		xmlNewChild(node, NULL, BAD_CAST "title", BAD_CAST "Title 2");
		if (i==1) {
			xmlNode* new = xmlNodeEncrypt(node, pass, "rc4,aes_128_ofb");
			xmlReplaceNode(node, new);
			xmlFreeNode(node);
		}
	}
	puts("--- AFTER ENCRYPTION ---");
	xmlDocFormatDump(stdout, doc, 1);puts("");

	// Decrypt
	xmlNode* node = config->children;
	while(node) {
		xmlNode* new = xmlNodeDecrypt(node, pass);
		if (new) {
			xmlReplaceNode(node, new);
			xmlFreeNode(node);
		}
		node = node->next;
	}
	puts("--- AFTER DECRYPTION ---");
	xmlDocFormatDump(stdout, doc, 1);puts("");
}
