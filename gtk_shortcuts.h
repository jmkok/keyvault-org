#ifndef _gtk_shortcuts_h_
#define _gtk_shortcuts_h_

extern GtkWidget* gtk_add_labeled_entry(gchar* title,gchar* default_val,GtkWidget* vbox);
extern GtkWidget* gtk_add_label(GtkWidget* vbox,gchar* title);
extern GtkWidget* gtk_add_entry(GtkWidget* vbox,gchar* value);

extern GtkWidget* gtk_add_separator(GtkWidget* parent);
extern GtkWidget* gtk_add_menu_item(gchar* title,GtkWidget* menu);
extern GtkWidget* gtk_add_menu(gchar* title,GtkWidget* menu);
extern void gtk_remove_menu_item(GtkWidget* menu_item, GtkWidget* parent);
	
#endif
