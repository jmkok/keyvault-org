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

// -----------------------------------------------------------
//
// functions
//

extern void save_configuration(tList* kvo_list);
extern void read_configuration(tList* kvo_list);

tFileDescription* node_to_kvo(xmlNode* node);
xmlNode* kvo_to_node(tFileDescription* kvo);

#endif
