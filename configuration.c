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
#include "file_location.h"

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
	struct CONFIG* config = calloc(1,sizeof(struct CONFIG));
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
				tConfigDescription* item = calloc(1,sizeof(tConfigDescription));
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
