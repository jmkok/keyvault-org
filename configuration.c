#include <stdint.h>
#include <mxml.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>

#include "configuration.h"
#include "structures.h"
//~ #include "xml.h"
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
	mxml_node_t* xml = mxmlNewXML("1.0");
	mxml_node_t* config = mxmlNewElement(xml,"config");
	int i;
	for (i=0;i < kvo_list->count; i++) {
		tKvoFile* kvo = kvo_list->data[i];
		mxml_node_t* kvo_file = mxmlNewElement(config, "kvo_file");
		mxml_node_t* node;
		// title
		node = mxmlNewElement(kvo_file, "title");			
		mxmlNewText(node, 0, kvo->title);
		// protocol
		node = mxmlNewElement(kvo_file, "protocol");	
		mxmlNewText(node, 0, kvo->protocol);
		// hostname
		if (kvo->hostname) {
			node = mxmlNewElement(kvo_file, "hostname");	
			mxmlNewText(node, 0, kvo->hostname);
		}
		// port
		if (kvo->port) {
			node = mxmlNewElement(kvo_file, "port");			
			mxmlNewInteger(node, kvo->port);
		}
		// username
		if (kvo->username) {
			node = mxmlNewElement(kvo_file, "username");	
			mxmlNewText(node, 0, kvo->username);
		}
		// password
		if (kvo->password) {
			node = mxmlNewElement(kvo_file, "password");	
			mxmlNewText(node, 0, kvo->password);
		}
		// filename
		node = mxmlNewElement(kvo_file, "filename");	
		mxmlNewText(node, 0, kvo->filename);
		// fingerprint (ssh)
		if (kvo->fingerprint)	{
			node = mxmlNewElement(kvo_file, "fingerprint");	
			char* tmp=g_base64_encode(kvo->fingerprint,16);
			mxmlNewText(node, 0, tmp);
			g_free(tmp);
		}
	}
	// Save the xml node
	FILE* fh=fopen("config_save.xml","wt");
	if (fh) {		
		//~ mxmlSaveFile(xml,fh,whitespace_cb);
		fclose(fh);
	}
	// Remove the xml node
	mxmlDelete(xml);
}
