#ifndef _gtk_ui_h_
#define _gtk_ui_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "main.h"
#include "list.h"

// -----------------------------------------------------------
//
// variables
//

int create_main_window(struct SETUP* setup);

struct TREEVIEW {
	GtkWidget* view;				/* The view */
	GtkTreeStore* store;		/* The store */
	GtkTreeModel* model;		/* The model */
	GtkWidget* popup_menu;	/* The popup menu */
	char* filter_text;			/* The filter text */
};

struct UI {
	/* The main window */
	GtkWidget* main_window;
	GtkAccelGroup* accel_group;

	/* The file menu + items */
	GtkWidget* file_menu;
	GtkWidget* new_menu_item;
	GtkWidget* open_menu_item;
	GtkWidget* save_menu_item;
	GtkWidget* edit_location_item;
	GtkWidget* close_menu_item;
	GtkWidget* exit_menu_item;
	GtkWidget* import_menu_item;
	GtkWidget* export_menu_item;

	/* The edit menu */
	GtkWidget* edit_menu;

	// The menus
	//~ GtkWidget* open_profile_menu;
	//~ GtkWidget* save_profile_menu;
	//~ GtkWidget* edit_profile_menu;

	/* The left side */
	GtkWidget* filter_entry;
	GtkWidget* treeview_scroll;

	/* The right side */
	GtkWidget* title_entry;
	GtkWidget* username_entry;
	GtkWidget* password_entry;
	GtkWidget* url_entry;
	GtkWidget* group_entry;
	GtkWidget* info_text;
	GtkWidget* time_created_label;
	GtkWidget* time_modified_label;
	GtkWidget* random_password_button;
	GtkWidget* launch_button;
	GtkWidget* record_save_button;

	/* The status bar */
	GtkWidget* statusbar;

	/* The treeview and its proporties */
	struct TREEVIEW* tree;
};

struct PASSKEY {
	int valid;
	u_char data[32];		// This key is used to encrypt/decrypt the container
	u_char config[32];	// This key is used to encrypt/decrypt some of the configuration fields
	u_char login[32];	// This key is used to login to keyvault.org (todo)
};

#endif
