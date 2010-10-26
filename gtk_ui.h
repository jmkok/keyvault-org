#ifndef _gtk_ui_h_
#define _gtk_ui_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "list.h"

#define trace() printf("<%s:%u>\n",__FILE__,__LINE__)
#define todo() printf("TODO <%s:%u>\n",__FILE__,__LINE__)

// -----------------------------------------------------------
//
// variables
//

int create_main_window(const char* filename);

struct tGlobal {
	tList* kvo_list;
};

extern struct tGlobal* global;


#endif
