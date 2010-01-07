#ifndef _configuration_h_
#define _configuration_h_

#include <stdint.h>
#include <gtk/gtk.h>

// -----------------------------------------------------------
//
// variables
//

// -----------------------------------------------------------
//
// functions
//

extern void save_configuration(GList* kvo_list);
extern void read_configuration(GList* kvo_list);

#endif
