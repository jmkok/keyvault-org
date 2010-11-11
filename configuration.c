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
// Read the configuration
//

int is_node_encrypted(xmlNode* node) {
	// encrypted="yes"
	int retval = 0;
	xmlChar* encrypted = xmlGetProp(node, BAD_CAST "encrypted");
	if (encrypted) {
		if (xmlStrEqual(encrypted, BAD_CAST "yes"))
			retval = 1;
		free(encrypted);
	}
	return retval;
}

// -----------------------------------------------------------
//
// Read the configuration
//

xmlDoc* new_configuration(void) {
	// Create an xml node that conatins all configuration
	xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
	xmlNode* config = xmlNewNode(NULL, BAD_CAST "config");
	xmlDocSetRootElement(doc, config);
	return doc;
}

void read_configuration(tList* config_list) {
	printf("read_configuration()\n");
	configDoc = xmlParseFile("config.xml");
	if (!configDoc)
		configDoc = new_configuration();
	assert(configDoc);

	xmlNode* root = xmlDocGetRootElement(configDoc);
	if (!root)
		xmlFreeDoc(configDoc);
	//~ xmlElemDump(stdout, NULL, root);puts("");

	xmlNode* node = root->children;
	while(node) {
		if ((node->type == XML_ELEMENT_NODE) && xmlStrEqual(node->name, BAD_CAST "kvo_file")) {
			//~ xmlElemDump(stdout, NULL, node);puts("");
			// Create a new kvo_file
			tConfigDescription* config = mallocz(sizeof(tConfigDescription));
			list_add(config_list, config);
			// Analyze all items for this kvo_file
			config->title = (char*)xmlGetContents(node, BAD_CAST "title");
			config->doc = configDoc;
			config->node = node;
			char* tmp = (char*)xmlGetProp(node, BAD_CAST "check");
			gsize data_len;
			config->passphrase_check = g_base64_decode(tmp, &data_len);
		}
		node = node->next;
	}
}

// -----------------------------------------------------------
//
// get_configuration
//

tFileDescription* get_configuration(tConfigDescription* config, const unsigned char passphrase_key[32]) {
	printf("get_configuration()\n");
	xmlNode* node = config->node;
	if (node) {
		tFileDescription* kvo = mallocz(sizeof(tFileDescription));
		kvo->config = config;
		kvo->protocol = (char*)xmlGetContents(node, BAD_CAST "protocol");
		kvo->hostname = (char*)xmlGetContents(node, BAD_CAST "hostname");
		kvo->port = xmlGetContentsInteger(node, BAD_CAST "port");
		kvo->username = (char*)xmlGetContentsEncrypted(node, BAD_CAST "username", passphrase_key);
		kvo->password = (char*)xmlGetContentsEncrypted(node, BAD_CAST "password", passphrase_key);
		kvo->filename = (char*)xmlGetContentsEncrypted(node, BAD_CAST "filename", passphrase_key);
		gsize size;
		char* fingerprint_base64 = (char*)xmlGetContents(node, BAD_CAST "fingerprint");
		if (fingerprint_base64) {
			kvo->fingerprint = g_base64_decode(fingerprint_base64, &size);
			free(fingerprint_base64);
		}
		return kvo;
	}
	return NULL;
}

// -----------------------------------------------------------
//
// put_configuration
//

void put_configuration(tFileDescription* kvo, const unsigned char passphrase_key[32]) {
	printf("put_configuration()\n");
	assert(kvo);
	assert(kvo->config);
	assert(kvo->config->node);

	//~ tFileDescription* kvo = config_list->data[i];
	xmlNode* kvo_file_node = kvo->config->node;
	xmlNode* node = kvo_file_node->children;
	while(node) {
		xmlNode* unlink_node = node;
		node = node->next;
		xmlUnlinkNode(unlink_node);
		xmlFreeNode(unlink_node);
	}
	//~ xmlNode* kvo_file_node = xmlNewChild(config, NULL, BAD_CAST "kvo_file", NULL);

	xmlNewChild(kvo_file_node, NULL, BAD_CAST "title", BAD_CAST kvo->config->title);
	xmlNewChild(kvo_file_node, NULL, BAD_CAST "protocol", BAD_CAST kvo->protocol);
	if (kvo->hostname)
		xmlNewChild(kvo_file_node, NULL, BAD_CAST "hostname", BAD_CAST kvo->hostname);
	if (kvo->port) {
		char tmp[64];
		sprintf(tmp,"%u",kvo->port);
		xmlNewChild(kvo_file_node, NULL, BAD_CAST "port", BAD_CAST tmp);
	}
	if (kvo->username)
		xmlNewChildEncrypted(kvo_file_node, NULL, BAD_CAST "username", BAD_CAST kvo->username, passphrase_key);
	if (kvo->password)
		xmlNewChildEncrypted(kvo_file_node, NULL, BAD_CAST "password", BAD_CAST kvo->password, passphrase_key);
	if (kvo->filename)
		xmlNewChildEncrypted(kvo_file_node, NULL, BAD_CAST "filename", BAD_CAST kvo->filename, passphrase_key);
	if (kvo->fingerprint)	{
		char* tmp=g_base64_encode(kvo->fingerprint,16);
		xmlNewChild(kvo_file_node, NULL, BAD_CAST "fingerprint", BAD_CAST tmp);
		g_free(tmp);
	}
}

// -----------------------------------------------------------
//
// Save the configuration
//

void save_configuration(tList* config_list) {
	printf("save_configuration()\n");
	// encrypted-xml => disk
	FILE* fp = fopen("config.xml", "w");
	if (fp) {
		xmlDocFormatDump(fp, configDoc, 1);
		fclose(fp);
	}

	// Remove the xml node
	//~ xmlFreeDoc(configDoc);
}
