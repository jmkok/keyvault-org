#include "gtk_ui.h"
#include "encryption.h"

int main(int argc, char** argv)
{
	// Initialize the gtk
  gtk_init(&argc, &argv);

  // Open the essential random handle (do it here so we can warn the user with a dialog before doing anything else
  random_init();

	// Initialize libxml2
	xmlThrDefIndentTreeOutput(1);
	xmlKeepBlanksDefault(0);
  
	// Load a default file
	char* filename = NULL;
	int i;
	for (i=1;i<argc;i++) {
		filename = argv[i];
	}

	// test
	#include "xml.h"
	unsigned char* pass = malloc_random(16);
	xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");
	xmlNode* config = xmlNewNode(NULL, BAD_CAST "config");
	xmlDocSetRootElement(doc, config);
	for (i=0;i<3;i++) {
		xmlNode* node = xmlNewChild(config, NULL, BAD_CAST "node", NULL);
		xmlNewChild(node, NULL, BAD_CAST "title", BAD_CAST "Title 1");
		xmlNewChild(node, NULL, BAD_CAST "title", BAD_CAST "Title 2");
		if (i==1)
			xmlNodeEncrypt(node, pass, "rc4,aes_128_ofb");
	}
	xmlDocFormatDump(stdout, doc, 1);
	
	xmlNode* node = config->children;
	while(node) {
		xmlNodeDecrypt(node, pass, "rc4,aes_128_ofb");
		node = node->next;
	}
	xmlDocFormatDump(stdout, doc, 1);
	exit(0);

	// Create the main window
	create_main_window(filename);

  return 0;
}
