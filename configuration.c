#include <stdint.h>
#include <string.h>

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
			tFileDescription* kvo = mallocz(sizeof(tFileDescription));
			list_add(kvo_list,kvo);
			// Analyze all items for this kvo_file
			kvo->title = (char*)xmlGetContents(node, BAD_CAST "title");
			kvo->protocol = (char*)xmlGetContents(node, BAD_CAST "protocol");
			kvo->hostname = (char*)xmlGetContents(node, BAD_CAST "hostname");
			kvo->port = xmlGetContentsInteger(node, BAD_CAST "port");
			xmlNode* username_node = xmlFindNode(node, BAD_CAST "username", 0);
			if (username_node) {
				if (is_node_encrypted(username_node))
					kvo->username_enc = (unsigned char*)xmlGetContents(node, BAD_CAST "username");
				else
					kvo->username = (char*)xmlGetContents(node, BAD_CAST "username");
			}
			xmlNode* password_node = xmlFindNode(node, BAD_CAST "password", 0);
			if (password_node) {
				if (is_node_encrypted(password_node))
					kvo->password_enc = (unsigned char*)xmlGetContents(node, BAD_CAST "password");
				else
					kvo->password = (char*)xmlGetContents(node, BAD_CAST "password");
			}
			kvo->filename = (char*)xmlGetContents(node, BAD_CAST "filename");
			gsize size;
			char* fingerprint_base64 = (char*)xmlGetContents(node, BAD_CAST "fingerprint");
			if (fingerprint_base64) {
				kvo->fingerprint = g_base64_decode(fingerprint_base64, &size);
				free(fingerprint_base64);
			}
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
		tFileDescription* kvo = kvo_list->data[i];
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

		if (kvo->username_enc) {
			//~ xmlNode* username_node = xmlNewChildBase64(kvo_file_node, NULL, BAD_CAST "username", BAD_CAST kvo->username_enc);
			//~ xmlNewProp(username_node, BAD_CAST "encrypted", BAD_CAST "yes");
		}
		else if (kvo->username)
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "username", BAD_CAST kvo->username);

		if (kvo->password_enc) {
			//~ xmlNode* password_node = xmlNewChildBase64(kvo_file_node, NULL, BAD_CAST "password", BAD_CAST kvo->password_enc);
			//~ xmlNewProp(password_node, BAD_CAST "encrypted", BAD_CAST "yes");
		}
		else if (kvo->password)
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "password", BAD_CAST kvo->password);

		xmlNewChild(kvo_file_node, NULL, BAD_CAST "filename", BAD_CAST kvo->filename);
		if (kvo->fingerprint)	{
			char* tmp=g_base64_encode(kvo->fingerprint,16);
			xmlNewChild(kvo_file_node, NULL, BAD_CAST "fingerprint", BAD_CAST tmp);
			g_free(tmp);
		}
	}
	
	// encrypted-xml => disk
	FILE* fp = fopen("config.xml", "w");
	if (fp) {
		xmlDocFormatDump(fp, doc, 1);
		fclose(fp);
	}

	// Remove the xml node
	xmlFreeDoc(doc);
}
