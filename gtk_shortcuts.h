#ifndef _gtk_shortcuts_h_
#define _gtk_shortcuts_h_

GtkWidget* gtk_add_labeled_entry(GtkWidget* vbox, gchar* title, gchar* default_val);
GtkWidget* gtk_add_label(GtkWidget* vbox, gchar* title);
GtkWidget* gtk_add_entry(GtkWidget* vbox, gchar* value);

GtkWidget* gtk_add_separator(GtkWidget* parent);
GtkWidget* gtk_add_menu_item(GtkWidget* menu, gchar* title);
GtkWidget* gtk_add_menu_item_clickable(GtkWidget* menu, gchar* title, GCallback, void* ptr);
GtkWidget* gtk_add_menu(GtkWidget* menu, gchar* title);
void gtk_remove_menu_item(GtkWidget* menu_item, GtkWidget* parent);
	
#endif
