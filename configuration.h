#ifndef _configuration_h_
#define _configuration_h_

#include <stdint.h>
#include <gtk/gtk.h>

// -----------------------------------------------------------
//
// variables
//

extern GList* kvo_list;

// -----------------------------------------------------------
//
// functions
//

void update_recent_list(void);
void save_configuration(void);
void read_configuration(void);

#endif
