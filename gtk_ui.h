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
	struct CONFIG* config;
};

extern struct tGlobal* global;

struct UI {
	// The main window
	GtkWidget* main_window;

	// The menus
	GtkWidget* open_profile_menu;
	GtkWidget* save_profile_menu;
	GtkWidget* edit_profile_menu;
};

#endif
