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

	// The entries
	GtkWidget* title_entry;
	GtkWidget* username_entry;
	GtkWidget* password_entry;
	GtkWidget* url_entry;
	GtkWidget* group_entry;
	GtkWidget* info_text;
	GtkWidget* time_created_label;
	GtkWidget* time_modified_label;

	/* The treeview popup menu */
	GtkWidget* popup_menu;
};

struct PASSKEY {
	int valid;
	u_char data[32];		// This key is used to encrypt/decrypt the container
	u_char config[32];	// This key is used to encrypt/decrypt some of the configuration fields
	u_char login[32];	// This key is used to login to keyvault.org (todo)
};

#endif
