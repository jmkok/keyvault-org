#ifndef _main_h_
#define _main_h_

#include <stdint.h>
#include <gtk/gtk.h>

#define trace() printf("<%s:%u>\n",__FILE__,__LINE__)
#define todo() printf("TODO <%s:%u>\n",__FILE__,__LINE__)

// -----------------------------------------------------------
//
// variables
//

struct tGUI {
	GtkWidget* window;
	GtkWidget* title_entry;
	GtkWidget* username_entry;
	GtkWidget* password_entry;
	GtkWidget* url_entry;
	GtkWidget* info_text;
	GtkWidget* open_recent_menu;
	// data tree
	GtkWidget* tree_view;
	GtkTreeStore* treestore;
	GtkTreeModel* treemodel;
};

extern struct tGUI* gui;

// -----------------------------------------------------------
//
// functions
//

extern void open_kvo_file(GtkWidget *widget, gpointer kvo_pointer);

#endif
