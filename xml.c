#include <assert.h>
#include <string.h>
#include <libxml/parser.h>
#include <glib/gprintf.h>
#include <openssl/evp.h>

#include "xml.h"
#include "encryption.h"
#include "functions.h"

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
	xmlNode* node = xmlFindNode(root_node, name, 0);
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

xmlChar* xmlGetPropBase64(xmlNode* node, const xmlChar* name) {
	char* tmp = (char*)xmlGetProp(node, name);
	gsize data_len;
	return g_base64_decode(tmp, &data_len);
}

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
	hexdump("ivec", ivec, ivec_len);
	evp_cipher(EVP_aes_256_ofb(), data, data_len, passphrase_key, ivec);
	hexdump("data", data, data_len);
	
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
