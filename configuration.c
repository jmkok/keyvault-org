#include <stdint.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libxml/parser.h>

#include "configuration.h"
#include "structures.h"
#include "main.h"
#include "gtk_shortcuts.h"
#include "xml.h"

// -----------------------------------------------------------
//
// Read the configuration
//

void read_configuration(tList* kvo_list) {
	printf("read_configuration()\n");
	xmlDoc* doc = xmlParseFile("config.xml");
	if (!doc)
		return;

	xmlNode* root = xmlDocGetRootElement(doc);
	if (!root)
		xmlFreeDoc(doc);
	//~ xmlElemDump(stdout, NULL, root);puts("");

	xmlNode* node = root->children;
	while(node) {
		if ((node->type == XML_ELEMENT_NODE) && xmlStrEqual(node->name, BAD_CAST "kvo_file")) {
			//~ xmlElemDump(stdout, NULL, node);puts("");
			// Create a new kvo_file
			tKvoFile* kvo=malloc(sizeof(tKvoFile));
			memset(kvo,0,sizeof(tKvoFile));
			kvo->local_filename = strdup("local.kvo");
			list_add(kvo_list,kvo);
			// Analyze all items for this kvo_file
			kvo->title = (char*)xmlGetContents(node, BAD_CAST "title");
			kvo->protocol = (char*)xmlGetContents(node, BAD_CAST "protocol");
			kvo->hostname = (char*)xmlGetContents(node, BAD_CAST "hostname");
			kvo->port = xmlGetContentsInteger(node, BAD_CAST "port");
			kvo->username = (char*)xmlGetContents(node, BAD_CAST "username");
			kvo->password = (char*)xmlGetContents(node, BAD_CAST "password");
			kvo->filename = (char*)xmlGetContents(node, BAD_CAST "filename");
			unsigned int size;
			char* fingerprint_base64 = (char*)xmlGetContents(node, BAD_CAST "fingerprint");
			kvo->fingerprint = g_base64_decode(fingerprint_base64, &size);
			free(fingerprint_base64);
		}
		node = node->next;
	}
	return;
}

// -----------------------------------------------------------
//
// Save the configuration
//

void save_configuration(tList* kvo_list) {
	printf("save_configuration()\n");
	// Create an xml node that conatins all configuration
	xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
	xmlNode* config = xmlNewNode(NULL, BAD_CAST "config");
	xmlDocSetRootElement(doc, config);
	
	int i;
	for (i=0;i < kvo_list->count; i++) {
		tKvoFile* kvo = kvo_list->data[i];
		xmlNode* kvo_file_node = xmlNewChild(config, NULL, BAD_CAST "kvo_file", NULL);

		xmlNewChild(kvo_file_node, NULL, BAD_CAST "title", BAD_CAST kvo->title);
		xmlNewChild(kvo_file_node, NULL, BAD_CAST "protocol", BAD_CAST kvo->protocol);
		if (kvo->hostname)
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "hostname", BAD_CAST kvo->hostname);
		if (kvo->port) {
			char tmp[64];
			sprintf(tmp,"%u",kvo->port);
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "port", BAD_CAST tmp);
		}
		if (kvo->username)
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "username", BAD_CAST kvo->username);
		if (kvo->password)
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "password", BAD_CAST kvo->password);
		xmlNewChild(kvo_file_node, NULL, BAD_CAST "filename", BAD_CAST kvo->filename);
		if (kvo->fingerprint)	{
			char* tmp=g_base64_encode(kvo->fingerprint,16);
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "fingerprint", BAD_CAST tmp);
			g_free(tmp);
		}
	}
	
	// encrypted-xml => disk
	FILE* fp = fopen("config_save.xml", "w");
	if (fp) {
		xmlDocDump(fp, doc);
		fclose(fp);
	}

	// Remove the xml node
	xmlFreeDoc(doc);
}
