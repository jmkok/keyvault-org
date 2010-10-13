#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <glib/gprintf.h>

#include "main.h"
#include "encryption.h"
#include "functions.h"
#include "xml.h"

// -------------------------------------------------------------------------------
//
// Encrypt an xmlDoc
//

xmlDoc* xml_doc_encrypt(xmlDoc* doc, const gchar* passphrase) {
	xmlChar* xml_text;
	int xml_size;
	unsigned char ivec[16]="1234567890ABCDEF";
	shuffle_ivec(ivec);
	xmlDocDumpFormatMemory(doc, &xml_text, &xml_size, 1);
	aes_ofb(xml_text, xml_size, passphrase, ivec);

	// Create the doc
	xmlDoc* enc_doc = xmlNewDoc(BAD_CAST "1.0"); 

	// Create the root of the doc
	xmlNode* root = xmlNewNode(NULL, BAD_CAST "keyvault");
	xmlNewProp(root, BAD_CAST "encrypted", BAD_CAST "yes");
	xmlDocSetRootElement(enc_doc, root);

	// Add the encrypted data
	gchar* tmp=g_base64_encode((unsigned char*)xml_text, xml_size);
	xmlNode* data = xmlNewChild(root, NULL, BAD_CAST "data", BAD_CAST tmp);
	g_free(tmp);

	// Store the encryption type
	xmlNewProp(data, BAD_CAST "encryptiom", BAD_CAST "AES_OFB");

	// Store the used ivec
	tmp = g_base64_encode(ivec,16);
	xmlNewProp(data, BAD_CAST "ivec", BAD_CAST tmp);
	g_free(tmp);

	// Store the size
	tmp = malloc(32);
	g_sprintf(tmp, "%u", xml_size);
	xmlNewProp(data, BAD_CAST "size", BAD_CAST tmp);
	g_free(tmp);

	return enc_doc;
}

// -------------------------------------------------------------------------------
//
// Encrypt an xmlDoc
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
	xmlChar* encryption = xmlGetProp(data_node, BAD_CAST "encryption");
	char* ivec_base64 = (char*)xmlGetProp(data_node, BAD_CAST "ivec");
	char* size_text = (char*)xmlGetProp(data_node, BAD_CAST "size");

	// Process the data for this node
	int size = atol(size_text);
	printf("encryption: %s\n", encryption);
	printf("size: %u\n", size);

	gsize ivec_len;
	unsigned char* ivec = g_base64_decode(ivec_base64, &ivec_len);
	hexdump("ivec", ivec, (int)ivec_len);

	gsize data_len;
	char* data_base64 = (char*)xmlNodeGetContent(data_node);
	guchar* data = g_base64_decode(data_base64, &data_len);
	printf("data_len: %u\n", data_len);

	// Decrypt the data
	aes_ofb(data, data_len, passphrase, ivec);

	// Convert into xml
	xmlDoc* doc_dec = xmlParseMemory((char*)data, data_len);

	/*
	xmlChar* xml_text;
	int xml_size;
	unsigned char ivec[16]="1234567890ABCDEF";
	shuffle_ivec(ivec);
	xmlDocDumpFormatMemory(doc, &xml_text, &xml_size, 1);
	aes_ofb(xml_text, xml_size, passphrase, ivec);

	// Create the doc
	xmlDoc* enc_doc = xmlNewDoc(BAD_CAST "1.0"); 

	// Create the root of the doc
	xmlNode* root = xmlNewNode(NULL, BAD_CAST "keyvault");
	xmlNewProp(root, BAD_CAST "encrypted", BAD_CAST "yes");
	xmlDocSetRootElement(enc_doc, root);

	// Add the encrypted data
	gchar* tmp=g_base64_encode((unsigned char*)xml_text, xml_size);
	xmlNode* data = xmlNewChild(root, NULL, BAD_CAST "data", BAD_CAST tmp);
	g_free(tmp);

	// Store the encryption type
	xmlNewProp(data, BAD_CAST "encryptiom", BAD_CAST "AES_OFB");

	// Store the used ivec
	tmp = g_base64_encode(ivec,16);
	xmlNewProp(data, BAD_CAST "ivec", BAD_CAST tmp);
	g_free(tmp);

	// Store the size
	tmp = malloc(32);
	g_sprintf(tmp, "%u", xml_size);
	xmlNewProp(data, BAD_CAST "size", BAD_CAST tmp);
	g_free(tmp);
	*/

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
