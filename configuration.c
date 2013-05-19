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
// Read the configuration
//

struct CONFIG* read_configuration(const char* filename) {
	printf("read_configuration('%s')\n", filename);

	/* Create a config structure */
	struct CONFIG* config = calloc(1,sizeof(struct CONFIG));

	/* Read the xml config */
	config->doc = xmlParseFile(filename);

	/* No config found, create an empty config */
	if (!config->doc) {
		config->doc = xmlNewDoc(CONST_BAD_CAST "1.0");
		xmlNode* root = xmlNewNode(NULL, CONST_BAD_CAST "config");
		xmlDocSetRootElement(config->doc, root);
	}

	//~ xmlDocShow(config->doc);
	assert(config->doc);

	return config;
}

// -----------------------------------------------------------
//
// Save the configuration
// All the nodes are/should be encrypted already
//

int save_configuration(const char* filename, struct CONFIG* config) {
	printf("save_configuration('%s',%p)\n", filename, config);
	FILE* fp = fopen(filename, "w");
	if (!fp) {
		fprintf(stderr, "ERROR: Failed to open %s for writing\n", filename);
		xmlDocShow(config->doc);
		return -1;
	}
	xmlDocFormatDump(fp, config->doc, 1);
	fclose(fp);
	return 0;
}
