#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "gtk_dialogs.h"
#include "gtk_shortcuts.h"
#include "gtk_dialogs.h"
#include "configuration.h"
#include "functions.h"

gchar* gtk_password_dialog(GtkWindow* parent, const gchar* title) {
	/* Create the dialog */
	GtkWidget* dialog = gtk_dialog_new_with_buttons ("Password",
																	 parent,
																	 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
																	 NULL);
	GtkWidget* ok_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	//~ gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	//~ gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);
	//~ gtk_widget_set_can_default(ok_button, TRUE);
	//~ gtk_widget_grab_default(ok_button);
	 
	//~ gtk_window_set_icon_from_file(GTK_WINDOW(dialog), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(dialog), GTK_STOCK_DIALOG_AUTHENTICATION);
	gtk_window_set_default_size (GTK_WINDOW(dialog), 250, -1);

	//~ gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	GtkWidget* content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	/* Add the vbox */
  GtkWidget* vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(content_area), vbox);
	
	/* Add the passphrase */
	GtkWidget* passphrase_entry = gtk_add_labeled_entry(vbox, title, NULL);
	gtk_entry_set_visibility(GTK_ENTRY(passphrase_entry), FALSE);

	/* Attach the enter key to the OK button */
	//~ gtk_signal_connect_object (GTK_OBJECT (passphrase_entry), "focus_in_event", GTK_SIGNAL_FUNC (gtk_widget_grab_default), GTK_OBJECT (ok_button));
	gtk_signal_connect_object (GTK_OBJECT (passphrase_entry), "activate", GTK_SIGNAL_FUNC (gtk_button_clicked), GTK_OBJECT (ok_button));

	/* Show the dialog, wait for the OK button and return the passphrase */
	gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	gchar* retval = NULL;
	if (response == GTK_RESPONSE_OK)
		retval = strdup(gtk_entry_get_text(GTK_ENTRY(passphrase_entry)));
	gtk_widget_destroy(dialog);
	return retval;
}

gboolean dialog_request_config (GtkWidget* parent, tConfigDescription* config) {
	trace();
	if (!config)
		return FALSE;
	todo();
	tFileDescription* kvo = node_to_kvo(config->node);
	gboolean retval=FALSE;
	/* Create the dialog */
	GtkWidget* dialog = gtk_dialog_new_with_buttons ("File configuration",
																	 GTK_WINDOW(parent),
																	 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
																	 NULL);
	GtkWidget* ok_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);	// GTK_RESPONSE_ACCEPT
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);	// GTK_RESPONSE_NONE

	//~ gtk_window_set_icon_from_file(GTK_WINDOW(dialog), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(dialog), GTK_STOCK_DIALOG_AUTHENTICATION);
	gtk_window_set_default_size (GTK_WINDOW(dialog), 300, -1);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	GtkWidget* content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	/* Add the vbox */
  GtkWidget* vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(content_area), vbox);

	/* Add the protocol chhoser */
	GtkWidget* label = gtk_label_new("Protocol");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label , FALSE, TRUE, 1);

	/* Select protocol */
	GtkWidget* combo_box=gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box),"local");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box),"ssh");
  gtk_box_pack_start(GTK_BOX(vbox), combo_box , FALSE, TRUE, 1);
	if (!kvo->protocol)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box),0);
	else if (strcmp(kvo->protocol,"ssh") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box),1);

	/* Add the fields */
	GtkWidget* title_entry = gtk_add_labeled_entry(vbox, "Title",kvo->title);
	GtkWidget* hostname_label = gtk_add_label(vbox,"Hostname");
	GtkWidget* hostname_entry = gtk_add_entry(vbox,kvo->hostname);
	GtkWidget* username_label = gtk_add_label(vbox,"Username");
	GtkWidget* username_entry = gtk_add_entry(vbox,kvo->username);
	GtkWidget* password_label = gtk_add_label(vbox,"Password");
	GtkWidget* password_entry = gtk_add_entry(vbox,kvo->password);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	GtkWidget* filename_entry = gtk_add_labeled_entry(vbox, "Filename",kvo->filename);

	/* Protocol change */
	void change_protocol(GtkWidget *widget, UNUSED gpointer data) {
		gchar* protocol=gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
		if (!protocol || strcmp(protocol,"local") == 0) {
			gtk_widget_hide(hostname_label);
			gtk_widget_hide(hostname_entry);
			gtk_widget_hide(username_label);
			gtk_widget_hide(username_entry);
			gtk_widget_hide(password_label);
			gtk_widget_hide(password_entry);
		}
		else if (strcmp(protocol,"ssh") == 0) {
			gtk_widget_show(hostname_label);
			gtk_widget_show(hostname_entry);
			gtk_widget_show(username_label);
			gtk_widget_show(username_entry);
			gtk_widget_show(password_label);
			gtk_widget_show(password_entry);
		}
		g_free(protocol);
		gint width,height;
		gtk_window_get_size(GTK_WINDOW(dialog),&width,&height);
		gtk_window_resize (GTK_WINDOW(dialog), width, 1);
	}
  g_signal_connect(G_OBJECT (combo_box), "changed", G_CALLBACK(change_protocol), NULL);

	/* Show the dialog, wait for the OK button and return the passphrase */
	gtk_widget_show_all(dialog);
	change_protocol(combo_box,NULL);

	//~ gtk_signal_connect_object (GTK_OBJECT (passphrase_entry), "focus_in_event", GTK_SIGNAL_FUNC (gtk_widget_grab_default), GTK_OBJECT (ok_button));
	gtk_signal_connect_object (GTK_OBJECT (combo_box), "activate", GTK_SIGNAL_FUNC (gtk_button_clicked), GTK_OBJECT (ok_button));

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		// Update the fields in the kvo structure...
		if (kvo->title) free(kvo->title);
		kvo->title = strdup(gtk_entry_get_text(GTK_ENTRY(title_entry)));
		if (kvo->protocol) free(kvo->protocol);
		kvo->protocol=gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo_box));
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box)) >= 1) {
			if (kvo->hostname) free(kvo->hostname);
			kvo->hostname=strdup(gtk_entry_get_text(GTK_ENTRY(hostname_entry)));
			if (kvo->username) free(kvo->username);
			kvo->username=strdup(gtk_entry_get_text(GTK_ENTRY(username_entry)));
			if (kvo->password) free(kvo->password);
			kvo->password=strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
		}
		if (kvo->filename) free(kvo->filename);
		kvo->filename=strdup(gtk_entry_get_text(GTK_ENTRY(filename_entry)));
		retval=TRUE;
	}

	gtk_widget_destroy(dialog);
	return retval;
}

void quick_message (GtkWidget *parent, gchar *message) {
	/* Create the widgets */
	GtkWidget* dialog = gtk_dialog_new_with_buttons ("Popup message",
																	 GTK_WINDOW(parent),	//main_application_window,
																	 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
																	 GTK_STOCK_OK, GTK_RESPONSE_NONE,
																	 NULL);
	//~ gtk_window_set_icon_from_file(GTK_WINDOW(dialog), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(dialog), GTK_STOCK_DIALOG_AUTHENTICATION);
	GtkWidget* content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	/* Ensure that the dialog box is destroyed when the user responds. */
	g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);

	/* Add the text view, and show everything we've added to the dialog. */
	GtkWidget* info_text = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(info_text),GTK_WRAP_WORD);
	gtk_text_view_set_editable (GTK_TEXT_VIEW(info_text),FALSE);
	GtkTextBuffer* buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text));
	gtk_text_buffer_set_text(buffer,message,-1);

	GtkWidget* scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll),info_text);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (content_area), scroll);
	gtk_widget_set_size_request (dialog, 400, 300);
	gtk_widget_show_all (dialog);
}

gchar* dialog_open_file(GtkWindow* parent_window, int filter)
{
	gchar* filename=NULL;
	GtkWidget* dialog = gtk_file_chooser_dialog_new ("Open File", parent_window, GTK_FILE_CHOOSER_ACTION_OPEN, 
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
								GTK_STOCK_OPEN, GTK_RESPONSE_OK, 
								NULL);
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "Keyvault files", "*.kvo", (filter == 0));
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "Comma separated files", "*.csv", (filter == 1));
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "XML files", "*.xml", (filter == 2));
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "All files", "*.*", 0);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	}
	gtk_widget_destroy (dialog);
	return filename;
}

gchar* dialog_save_file(GtkWindow* parent_window, int filter)
{
	gchar* filename=NULL;
	GtkWidget* dialog = gtk_file_chooser_dialog_new ("Save File", parent_window, GTK_FILE_CHOOSER_ACTION_SAVE,
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								GTK_STOCK_SAVE, GTK_RESPONSE_OK,
								NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "Keyvault files", "*.kvo", (filter == 0));
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "Comma separated files", "*.csv", (filter == 1));
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "XML files", "*.xml", (filter == 2));
	gtk_add_filter(GTK_FILE_CHOOSER(dialog), "All files", "*.*", 0);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	}
	gtk_widget_destroy (dialog);
	return filename;
}

void gtk_error_dialog(const gchar* message) {
	GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message, NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
