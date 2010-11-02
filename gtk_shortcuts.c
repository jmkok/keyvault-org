#include <gtk/gtk.h>
#include "gtk_shortcuts.h"
#include "functions.h"

GtkWidget* gtk_add_labeled_entry(GtkWidget* vbox, gchar* title, gchar* value) {
	/* 1. Add the label. */
	GtkWidget* label = gtk_label_new(title);
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label , FALSE, TRUE, 1);

	/* 2. Add the entry box. */
	GtkWidget* entry = gtk_entry_new();
	//~ gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	if (value)
		gtk_entry_set_text(GTK_ENTRY(entry),value);
  gtk_box_pack_start(GTK_BOX(vbox), entry , FALSE, TRUE, 1);

	return entry;
}

GtkWidget* gtk_add_label(GtkWidget* vbox,gchar* title) {
	GtkWidget* label = gtk_label_new(title);
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label , FALSE, TRUE, 1);
	return label;
}

GtkWidget* gtk_add_entry(GtkWidget* vbox,gchar* value) {
	GtkWidget* entry = gtk_entry_new();
	if (value)
		gtk_entry_set_text(GTK_ENTRY(entry),value);
  gtk_box_pack_start(GTK_BOX(vbox), entry , FALSE, TRUE, 1);
	return entry;
}

GtkWidget* gtk_add_separator(GtkWidget* parent) {
	GtkWidget* separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),separator);
	return separator;
}

void gtk_remove_menu_item(GtkWidget* menu_item, GtkWidget* parent) {
	todo();
}

GtkWidget* gtk_add_menu_item(GtkWidget* parent, gchar* title) {
	GtkWidget* menu_item = gtk_menu_item_new_with_label(title);
	gtk_menu_item_set_use_underline(GTK_MENU_ITEM(menu_item),TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),menu_item);
	return menu_item;
}

GtkWidget* gtk_add_menu_item_clickable(GtkWidget* parent, gchar* title, GCallback callback_f, void* ptr) {
	GtkWidget* menu_item = gtk_menu_item_new_with_label(title);
	gtk_menu_item_set_use_underline(GTK_MENU_ITEM(menu_item),TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),menu_item);
	g_signal_connect(G_OBJECT (menu_item), "activate", G_CALLBACK(callback_f), ptr);
	return menu_item;
}

GtkWidget* gtk_add_menu(GtkWidget* parent, gchar* title) {
	GtkWidget* menu_item = gtk_menu_item_new_with_label(title);
	gtk_menu_item_set_use_underline(GTK_MENU_ITEM(menu_item),TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),menu_item);
	GtkWidget* menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),menu);
	return menu;
}

GtkFileFilter* gtk_add_filter(GtkFileChooser* parent, const char* title, const char* filter, int selected) {
	GtkFileFilter* widget = gtk_file_filter_new ();
	gtk_file_filter_set_name(widget, title);
	gtk_file_filter_add_pattern (widget, filter);
	gtk_file_chooser_add_filter(parent, widget);
	if (selected)
		gtk_file_chooser_set_filter(parent, widget);
	return widget;
}

/* 
 * Adds a vertical scrollbar to a tree view, returning an hbox holding them both 
 */ 

static void treeview_map_handler(GtkWidget *treeView, gpointer data)
{ 
	GtkWidget *padBox = (GtkWidget*)data; 
	gint x, y; 
	g_assert(GTK_IS_HBOX(padBox)); 

	/* Set the size of the padding above the tree view's vertical scrollbar to the 
	* height of its header_window (i.e. the offset to the top of its bin_window) */ 
	gtk_tree_view_convert_bin_window_to_widget_coords(GTK_TREE_VIEW(treeView), 0, 0, &x, &y); 
	gtk_widget_set_size_request(padBox, -1, y); 
} 

GtkWidget *add_scroll_bar_to_treeview(GtkWidget *treeView) {
	GtkAdjustment *pAdj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(treeView)); 
	GtkWidget *vScroll = gtk_vscrollbar_new(pAdj); 
	GtkWidget *padBox, *hBox, *vBox; 

	hBox = gtk_hbox_new(FALSE, 0); 

	/* First insert the tree view */ 
	gtk_box_pack_start(GTK_BOX(hBox), treeView, TRUE, TRUE, 0); 
	gtk_widget_show(treeView); 

	/* Then, packed up against its right side, add in a vbox containing a 
	 * box for padding at the top and the scrollbar below that */ 
	vBox = gtk_vbox_new(FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(hBox), vBox, FALSE, FALSE, 0); 
	gtk_widget_show(vBox); 
	padBox = gtk_hbox_new(FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(vBox), padBox, FALSE, FALSE, 0); 
	gtk_widget_show(padBox); 
	gtk_box_pack_start(GTK_BOX(vBox), vScroll, TRUE, TRUE, 0); 
	gtk_widget_show(vScroll); 

	/* The padding box above the scroll bar needs to be set to the height of the
	 * tree view's header_window, but we can't do that until it is mapped */ 
	g_signal_connect(G_OBJECT(treeView), "map", G_CALLBACK(treeview_map_handler), padBox); 

	return hBox; 
} 

