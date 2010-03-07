#ifndef _configuration_h_
#define _configuration_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "list.h"

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

#endif
