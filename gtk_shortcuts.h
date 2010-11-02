#ifndef _gtk_shortcuts_h_
#define _gtk_shortcuts_h_

extern GtkWidget* gtk_add_labeled_entry(GtkWidget* vbox, gchar* title, gchar* default_val);
extern GtkWidget* gtk_add_label(GtkWidget* vbox, gchar* title);
extern GtkWidget* gtk_add_entry(GtkWidget* vbox, gchar* value);

extern GtkWidget* gtk_add_separator(GtkWidget* parent);
extern GtkWidget* gtk_add_menu_item(GtkWidget* menu, gchar* title);
extern GtkWidget* gtk_add_menu_item_clickable(GtkWidget* menu, gchar* title, GCallback, void* ptr);
extern GtkWidget* gtk_add_menu(GtkWidget* menu, gchar* title);
extern void gtk_remove_menu_item(GtkWidget* menu_item, GtkWidget* parent);

extern GtkFileFilter* gtk_add_filter(GtkFileChooser* widget, const char* title, const char* filter, int selected);

extern GtkWidget* add_scroll_bar_to_treeview(GtkWidget *treeView);

#endif
