#include <stdint.h>
#include <string.h>

#include <assert.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>

#include "configuration.h"
#include "functions.h"
#include "structures.h"
#include "main.h"
#include "gtk_shortcuts.h"
#include "xml.h"

// -----------------------------------------------------------
//
// Create a new and empty configuration
//

static xmlDoc* new_configuration(void) {
	// Create an xml node that conatins all configuration
	xmlDoc* doc = xmlNewDoc(CONST_BAD_CAST "1.0");
	xmlNode* config = xmlNewNode(NULL, CONST_BAD_CAST "config");
	xmlDocSetRootElement(doc, config);
	return doc;
}

xmlNode* new_config_node(xmlDoc* doc) {
	return xmlNewDocNode(doc, NULL, CONST_BAD_CAST "kvo_file", NULL);
}

xmlNode* get_config_node(xmlDoc* doc, int idx) {
	printf("node_to_kvo(%p,%i)\n", doc, idx);
	assert(doc);

	// Get the root
	xmlNode* root = xmlDocGetRootElement(doc);
	assert(root);

	// Walk all children
	xmlNode* node = root->children;
	while(node) {
		trace();
		if (node->type == XML_ELEMENT_NODE) {
			if (idx-- == 0)
				return node;
		}
		node = node->next;
	}
	return NULL;
}

char* get_config_title(xmlNode* node) {
	assert(node);
	if (xmlIsNodeEncrypted(node))
		return (char*)xmlGetProp(node, CONST_BAD_CAST "title");
	else if (xmlStrEqual(node->name, CONST_BAD_CAST "kvo_file"))
		return (char*)xmlGetTextContents(node, CONST_BAD_CAST "title");
	return NULL;
}

// -----------------------------------------------------------
//
// Read the configuration
//

struct CONFIG* read_configuration(const char* filename) {
	printf("read_configuration('%s')\n", filename);

	// Create a config list
	struct CONFIG* config = mallocz(sizeof(struct CONFIG));
	//~ config->config_list = listCreate();

	// Read the xml config
	config->doc = xmlParseFile(filename);
	//~ xmlDocShow(configDoc);
	if (!config->doc)
		config->doc = new_configuration();
	assert(config->doc);

/*
	// Get the root
	xmlNode* root = xmlDocGetRootElement(config->doc);
	assert(root);
	//~ xmlNodeShow(root);

	// Walk all children
	xmlNode* node = root->children;
	while(node) {
		//~ xmlNodeShow(node);
		if (node->type == XML_ELEMENT_NODE) {
			xmlChar* title = NULL;
			if (xmlIsNodeEncrypted(node))
				title = xmlGetProp(node, CONST_BAD_CAST "title");
			else if (xmlStrEqual(node->name, CONST_BAD_CAST "kvo_file"))
				title = xmlGetTextContents(node, CONST_BAD_CAST "title");
			if (title) {
				// Create a new kvo_file
				tConfigDescription* item = mallocz(sizeof(tConfigDescription));
				listAdd(config->config_list, item);
				item->title = (char*)title;
				//~ item->doc = config->doc;
				item->node = node;
				char* tmp = (char*)xmlGetProp(node, CONST_BAD_CAST "check");
				if (tmp) {
					gsize data_len;
					item->passphrase_check = g_base64_decode(tmp, &data_len);
				}
			}
		}
		node = node->next;
	}
*/
	return config;
}

// -----------------------------------------------------------
//
// get_configuration
//

tFileDescription* node_to_kvo(xmlNode* node) {
	printf("node_to_kvo(%p)\n",node);
	assert(node);
	xmlNodeShow(node);
	tFileDescription* kvo = mallocz(sizeof(tFileDescription));
	kvo->title = (char*)xmlGetTextContents(node, CONST_BAD_CAST "title");
	kvo->protocol = (char*)xmlGetTextContents(node, CONST_BAD_CAST "protocol");
	kvo->hostname = (char*)xmlGetTextContents(node, CONST_BAD_CAST "hostname");
	kvo->port = xmlGetIntegerContents(node, CONST_BAD_CAST "port");
	kvo->username = (char*)xmlGetTextContents(node, CONST_BAD_CAST "username");
	kvo->password = (char*)xmlGetTextContents(node, CONST_BAD_CAST "password");
	kvo->filename = (char*)xmlGetTextContents(node, CONST_BAD_CAST "filename");
	kvo->fingerprint = (char*)xmlGetBase64Contents(node, CONST_BAD_CAST "fingerprint");
	return kvo;
}

// -----------------------------------------------------------
//
// put_configuration
//

xmlNode* kvo_to_node(tFileDescription* kvo) {
	printf("kvo_to_node(%p)\n", kvo);
	assert(kvo);
	assert(kvo->title);
	xmlNode* root = xmlNewNode(NULL, CONST_BAD_CAST "kvo_file");
	assert(root);
	xmlNewTextChild(root, NULL, CONST_BAD_CAST "title", BAD_CAST kvo->title);
	if (kvo->protocol)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "protocol", BAD_CAST kvo->protocol);
	if (kvo->hostname)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "hostname", BAD_CAST kvo->hostname);
	if (kvo->port)
		xmlNewIntegerChild(root, NULL, CONST_BAD_CAST "port", kvo->port);
	if (kvo->username)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "username", BAD_CAST kvo->username);
	if (kvo->password)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "password", BAD_CAST kvo->password);
	if (kvo->filename)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "filename", BAD_CAST kvo->filename);
	if (kvo->fingerprint)
		xmlNewBase64Child(root, NULL, CONST_BAD_CAST "fingerprint", BAD_CAST kvo->fingerprint, 16);
	return root;
}

// -----------------------------------------------------------
//
// Save the configuration
// All the nodes are/should be encrypted already
//

void save_configuration(const char* filename, struct CONFIG* config) {
	printf("save_configuration('%s',%p)\n", filename, config);

	// Write the configuration to the disk
	FILE* fp = fopen(filename, "w");
	if (fp) {
		xmlDocFormatDump(fp, config->doc, 1);
		fclose(fp);
	}
}
