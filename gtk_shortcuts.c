#include <gtk/gtk.h>
#include "main.h"

GtkWidget* gtk_add_labeled_entry(gchar* title,gchar* value,GtkWidget* vbox) {
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

void gtk_add_separator(GtkWidget* parent) {
	GtkWidget* separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),separator);
}

void gtk_remove_menu_item(GtkWidget* menu_item, GtkWidget* parent) {
	todo();
}

GtkWidget* gtk_add_menu_item(gchar* title,GtkWidget* parent) {
	GtkWidget* menu_item = gtk_menu_item_new_with_label(title);
	gtk_menu_item_set_use_underline(GTK_MENU_ITEM(menu_item),TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),menu_item);
	return menu_item;
}

GtkWidget* gtk_add_menu(gchar* title,GtkWidget* parent) {
	GtkWidget* menu_item = gtk_menu_item_new_with_label(title);
	gtk_menu_item_set_use_underline(GTK_MENU_ITEM(menu_item),TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(parent),menu_item);
	GtkWidget* menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),menu);
	return menu;
}
