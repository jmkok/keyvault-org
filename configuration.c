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

static xmlDoc* configDoc;

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

// -----------------------------------------------------------
//
// Read the configuration
//

void read_configuration(tList* config_list) {
	printf("read_configuration(%p)\n", config_list);
	configDoc = xmlParseFile("config.xml");
	//~ xmlDocFormatDump(stdout, configDoc, 1);
	if (!configDoc)
		configDoc = new_configuration();
	assert(configDoc);

	// Get the root
	xmlNode* root = xmlDocGetRootElement(configDoc);
	assert(root);
	//~ xmlElemDump(stdout, NULL, root);puts("");

	// Walk all children
	xmlNode* node = root->children;
	while(node) {
		//~ xmlElemDump(stdout, NULL, node);puts("");
		if (node->type == XML_ELEMENT_NODE) {
			xmlChar* title = NULL;
			if (xmlIsNodeEncrypted(node))
				title = xmlGetProp(node, CONST_BAD_CAST "title");
			else if (xmlStrEqual(node->name, CONST_BAD_CAST "kvo_file"))
				title = xmlGetTextContents(node, CONST_BAD_CAST "title");
			if (title) {
				// Create a new kvo_file
				tConfigDescription* config = mallocz(sizeof(tConfigDescription));
				listAdd(config_list, config);
				config->title = (char*)title;
				config->doc = configDoc;
				config->node = node;
				char* tmp = (char*)xmlGetProp(node, CONST_BAD_CAST "check");
				if (tmp) {
					gsize data_len;
					config->passphrase_check = g_base64_decode(tmp, &data_len);
				}
			}
		}
		node = node->next;
	}
}

// -----------------------------------------------------------
//
// get_configuration
//

tFileDescription* node_to_kvo(xmlNode* node) {
	printf("node_to_kvo(%p)\n",node);
	assert(node);
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

void save_configuration(tList* config_list) {
	printf("save_configuration(%p)\n", config_list);

	// Write the configuration to the disk
	FILE* fp = fopen("config.xml", "w");
	if (fp) {
		xmlDocFormatDump(fp, configDoc, 1);
		fclose(fp);
	}
}
