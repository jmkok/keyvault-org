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
// Read properties of a node
//

xmlChar* xmlGetPropBase64(xmlNode* node, const xmlChar* name) {
	char* tmp = (char*)xmlGetProp(node, name);
	if (!tmp) return NULL;
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

xmlChar* xmlGetTextContents(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, name, 0);
	if (node)
		return xmlNodeGetContent(node);
	return NULL;
}

xmlChar* xmlGetBase64Contents(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = root_node;
	if (name)
		node = xmlFindNode(root_node, name, 0);
	if (!node)
		return NULL;
	xmlChar* tmp = xmlNodeGetContent(node);
	if (!tmp) return NULL;
	gsize data_len;
	return g_base64_decode((void*)tmp, &data_len);
}

int xmlGetIntegerContents(xmlNode* root_node, const xmlChar* name) {
	// Get the node
	xmlNode* node = xmlFindNode(root_node, name, 0);
	if (!node) return 0;
	// Read the text
	xmlChar* text = xmlNodeGetContent(node);
	if (!text) return 0;
	// Convert to integer
	int retval = atol((char*)text);
	xmlFree(text);
	return retval;
}

// -------------------------------------------------------------------------------
//
// Convert a text string into a specific EVP_CIPHER
//

const EVP_CIPHER* evp_cipher_text(const char* text) {
	debugf("evp_cipher_text('%s')\n", text);
	if (strcmp(text,"null") == 0)					return EVP_enc_null();
	if (strcmp(text,"rc4") == 0)					return EVP_rc4();
	if (strcmp(text,"des_ofb") == 0)			return EVP_des_ofb();
	if (strcmp(text,"aes_128_ofb") == 0)	return EVP_aes_128_ofb();
	if (strcmp(text,"aes_192_ofb") == 0)	return EVP_aes_192_ofb();
	if (strcmp(text,"aes_256_ofb") == 0)	return EVP_aes_256_ofb();
	// No match yet, then we have a serious problem
	return NULL;
}

#define XML_NODE_NAME CONST_BAD_CAST "vault"
#define XML_NODE_NS CONST_BAD_CAST "http://keyvault.org"

// -------------------------------------------------------------------------------
//
// Encrypt an xmlNode (IN PLACE)
//

xmlNode* xmlNodeEncryptInPlace(xmlNode* node, const unsigned char passphrase[32], const char* protocols) {
	debugf("xmlNodeEncryptInPlace(%p,%p,'%s')\n", node, passphrase, protocols);
	xmlNode* node_encrypted = xmlNodeEncrypt(node, passphrase, NULL);
	if (node_encrypted) {
		xmlReplaceNode(node, node_encrypted);
		xmlFreeNode(node);
	}
	return node_encrypted;
}

// -------------------------------------------------------------------------------
//
// Encrypt an xmlNode
//

xmlNode* xmlNodeEncrypt(xmlNode* node, const unsigned char passphrase[32], const char* protocols) {
	debugf("xmlNodeEncrypt(%p,%p,'%s')\n", node, passphrase, protocols);
	
	// Get the raw data...
	xmlBuffer* xbuf = xmlBufferCreate();
	xmlNodeDump(xbuf, NULL, node, 0, 1);
	if (!protocols)
		protocols = "rc4,aes_128_ofb";

	// Create the "vault" node
	xmlNode* enode = xmlNewNode(NULL, XML_NODE_NAME);
	xmlNs* ns = xmlNewNs(enode, XML_NODE_NS, NULL);

	// Copy the passphrase key into the encryption key
	u_char encryption_key[32];
	memcpy(encryption_key, passphrase, 32);

	// PBKDF2 the passphrase
	char* pbkdf2_salt = malloc_random(32);
	unsigned int pbkdf2_rounds = 20000 + random_integer(1000);
	if (pbkdf2_rounds) {
		PKCS5_PBKDF2_HMAC_SHA1(
			(const char*)passphrase, 32,
			(unsigned char*)pbkdf2_salt, 32,
			pbkdf2_rounds, 32, encryption_key);
	}
	//~ hexdump(encryption_key, 32);

	// Store the pbkdf2 setup
	xmlNode* pbkdf2_node = xmlNewChild(enode, ns, CONST_BAD_CAST "pbkdf2", NULL);
	xmlNewBase64Prop(pbkdf2_node, CONST_BAD_CAST "salt", BAD_CAST pbkdf2_salt, 32);
	xmlNewIntegerProp(pbkdf2_node, CONST_BAD_CAST "rounds", pbkdf2_rounds);

	// The cipher functions
	void evp_cipher_protocol(tList* UNUSED(list), void* data) {
		char* protocol = data;
		const EVP_CIPHER* cipher = evp_cipher_text(protocol);
		unsigned char* ivec = malloc_random(16);
		evp_cipher(cipher, xbuf->content, xbuf->use+1, encryption_key, ivec);
		xmlNode* encryption_node = xmlNewChild(enode, ns, CONST_BAD_CAST "encryption", NULL);
		xmlNewProp(encryption_node, CONST_BAD_CAST "cipher", BAD_CAST protocol);
		xmlNewBase64Prop(encryption_node, CONST_BAD_CAST "ivec", ivec, 16);
	}

	// Call the cipher function for each requested cipher
	tList* proto_list = listExplode(protocols,",");
	listForeach(proto_list, evp_cipher_protocol);
	listDestroy(proto_list);

	// Add the encrypted data
	gchar* tmp=g_base64_encode((unsigned char*)xbuf->content, xbuf->use+1);
	xmlNode* data_node = xmlNewTextChild(enode, ns, CONST_BAD_CAST "data", BAD_CAST tmp);
	g_free(tmp);
	xmlNewIntegerProp(data_node, CONST_BAD_CAST "size", xbuf->use+1);

	// Return the encrypted node
	return enode;
}

// -------------------------------------------------------------------------------
//
// Decrypt an xmlNode
//

xmlNode* xmlNodeDecrypt(xmlNode* root, const unsigned char passphrase[32]) {
	debugf("xmlNodeDecrypt(%p,%p)\n", root, passphrase);

	// Test to see if it is a keyvault.org node
	if (!xmlIsNodeEncrypted(root))
		return NULL;

	// <pbkdf2>
	u_char encryption_key[32];
	memcpy(encryption_key, passphrase, 32);
	xmlNode* node = root->children;
	while(node) {
		if (xmlStrEqual(node->name, CONST_BAD_CAST "pbkdf2")) {
			unsigned char* pbkdf2_salt = xmlGetPropBase64(node, CONST_BAD_CAST "salt");
			unsigned int pbkdf2_rounds = xmlGetPropInteger(node, CONST_BAD_CAST "rounds");
			if (pbkdf2_rounds) {
				PKCS5_PBKDF2_HMAC_SHA1(
					(const char*)passphrase, 32,
					pbkdf2_salt, 32, 
					pbkdf2_rounds, 32, encryption_key);
			}
			free(pbkdf2_salt);
			//~ hexdump(encryption_key, 32);
		}
		node = node->next;
	}

	// <data>
	void* data = NULL;
	int size = 0;
	node = root->children;
	while(node) {
		if (xmlStrEqual(node->name, CONST_BAD_CAST "data")) {
			//~ xmlElemDump(stdout, NULL, node);
			size = xmlGetPropInteger(node, CONST_BAD_CAST "size");
			data = xmlGetBase64Contents(node, NULL);
		}
		node = node->next;
	}

	// walk all <encryption> nodes
	node = root->children;
	while(node) {
		if (xmlStrEqual(node->name, CONST_BAD_CAST "encryption")) {
			//~ xmlElemDump(stdout, NULL, node);
			char* protocol = (char*) xmlGetProp(node, CONST_BAD_CAST "cipher");
			unsigned char* ivec = xmlGetPropBase64(node, CONST_BAD_CAST "ivec");
			const EVP_CIPHER* cipher = evp_cipher_text(protocol);
			evp_cipher(cipher, data, size, encryption_key, ivec);
		}
		node = node->next;
	}

	// Convert the xml text into an xmlNode object
	xmlDoc* doc = xmlReadDoc(data,NULL,NULL,0);
	//~ xmlDocFormatDump(stdout, doc, 1);
	xmlNode* xnode = xmlDocGetRootElement(doc);
	xmlUnlinkNode(xnode);
	xmlFreeDoc(doc);
	return xnode;
}

// -------------------------------------------------------------------------------
//
// xmlIsNodeEncrypted
// The node should look like <vault xmlns="http://keyvault.org">
// Note that it may contain additional attributes (like a title or something alike)
//

int xmlIsNodeEncrypted(xmlNode* root) {
	// Test to see if it is a keyvault.org node
	assert(root);
	//~ printf("root->name: %s\n",root->name);
	if (!root->name || !xmlStrEqual(root->name, XML_NODE_NAME))
		return 0;
	//~ printf("root->nsDef->href: %s\n",root->nsDef->href);
	if (!root->nsDef || !root->nsDef->href || !xmlStrEqual(root->nsDef->href, XML_NODE_NS))
		return 0;
	return 1;
}

// -------------------------------------------------------------------------------
//
// 
//

xmlNode* xmlNewChildEncrypted_DEPRECATED(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const xmlChar* value, const unsigned char passphrase_key[32]) {
	unsigned char* ivec = malloc_random(16);
	int len = (xmlStrlen(value) & ~63) + 64;
	unsigned char* enlarged_value = mallocz(len+1);
	strcpy((char*)enlarged_value, (const char*)value);
	evp_cipher(EVP_aes_256_ofb(), enlarged_value, len, passphrase_key, ivec);
	gchar* tmp = g_base64_encode(enlarged_value, len);
	xmlNode* node = xmlNewTextChild(parent, ns, name, BAD_CAST tmp);
	xmlNewProp(node, CONST_BAD_CAST "encrypted", CONST_BAD_CAST "yes");
	xmlNewBase64Prop(node, CONST_BAD_CAST "ivec", ivec, 16);
	g_free(tmp);
	free(enlarged_value);
	return node;
}

xmlNode* xmlNewIntegerChild(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const int value) {
	gchar* tmp = malloc(32);
	g_sprintf(tmp, "%u", value);
	xmlNode* node = xmlNewTextChild(parent, ns, name, BAD_CAST tmp);
	g_free(tmp);
	return node;
}

xmlNode* xmlNewBase64Child(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const void* data, const int size) {
	gchar* tmp = g_base64_encode(data, size);
	xmlNode* node = xmlNewTextChild(parent, ns, name, BAD_CAST tmp);
	g_free(tmp);
	return node;
}

xmlAttr* xmlNewIntegerProp(xmlNode* node, const xmlChar* name, const int value) {
	gchar* tmp = malloc(32);
	g_sprintf(tmp, "%u", value);
	xmlAttr* attrib = xmlNewProp(node, name, BAD_CAST tmp);
	g_free(tmp);
	return attrib;
}

xmlAttr* xmlNewBase64Prop(xmlNodePtr node, const xmlChar* name, const void* data, const int size) {
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

xmlChar* xmlGetContentsEncrypted_DEPRECATED(xmlNode* root_node, const xmlChar* name, const unsigned char passphrase_key[32]) {
	xmlNode* node = xmlFindNode(root_node, name, 0);
	if (!node) return NULL;

	xmlChar* rawdata = xmlNodeGetContent(node);
	if (!rawdata) return NULL;

	xmlChar* encrypted = xmlGetProp(node, CONST_BAD_CAST "encrypted");
	if (!encrypted) return rawdata;
	gsize data_len;
	unsigned char* data = g_base64_decode((char*)rawdata, &data_len);
	xmlFree(encrypted);
	xmlFree(rawdata);
	//~ #undef rawdata
	//~ rawdata=malloc(10);

	xmlChar* rawivec = xmlGetProp(node, CONST_BAD_CAST "ivec");
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
	xmlDoc* doc = xmlNewDoc(CONST_BAD_CAST "1.0");
	xmlNode* config = xmlNewNode(NULL, CONST_BAD_CAST "config");
	xmlDocSetRootElement(doc, config);
	int i;
	for (i=0;i<3;i++) {
		xmlNode* node = xmlNewChild(config, NULL, CONST_BAD_CAST "node", NULL);
		xmlNewChild(node, NULL, CONST_BAD_CAST "title", CONST_BAD_CAST "Title 1");
		xmlNewChild(node, NULL, CONST_BAD_CAST "title", CONST_BAD_CAST "Title 2");
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
