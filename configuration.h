#ifndef _configuration_h_
#define _configuration_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "list.h"
#include "structures.h"

// -----------------------------------------------------------
//
// variables
//

struct CONFIG {
	xmlDoc* doc;
};

// -----------------------------------------------------------
//
// functions
//

extern void save_configuration(const char* filename, struct CONFIG* config);
extern struct CONFIG* read_configuration(const char* filename) __attribute__ ((warn_unused_result));

xmlNode* new_config_node(xmlDoc* doc);
xmlNode* get_config_node(xmlDoc* doc, int idx);
char* get_config_title(xmlNode*);

tFileDescription* node_to_kvo(xmlNode* node);
xmlNode* kvo_to_node(tFileDescription* kvo);

#endif
