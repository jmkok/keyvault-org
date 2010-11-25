#ifndef _gtk_ui_h_
#define _gtk_ui_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "list.h"

// -----------------------------------------------------------
//
// variables
//

int create_main_window(const char* default_filename);

struct tGlobal {
	tList* config_list;
};

extern struct tGlobal* global;


#endif
