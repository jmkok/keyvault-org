/*
 * http://library.gnome.org/devel/gtk-tutorial/stable/book1.html
 * http://library.gnome.org/devel/gtk/stable/
 * http://library.gnome.org/devel/gtk/unstable/gtk-Stock-Items.html
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gprintf.h>
#include <libxml/parser.h>
#include <errno.h>
#include <assert.h>

#include "gtk_ui.h"
#include "gtk_treeview.h"
#include "gtk_dialogs.h"
#include "gtk_shortcuts.h"
#include "list.h"
#include "functions.h"
#include "configuration.h"
#include "encryption.h"
#include "xml.h"
#include "file_location.h"

// -----------------------------------------------------------
//
// Global variables
//

struct SETUP* global = NULL;
struct PASSKEY* passkey = NULL;
struct UI* g_ui = NULL;
static struct FILE_LOCATION* active_file_location = NULL;

static void setup_menu_items(struct UI* ui, struct CONFIG* config);

// -----------------------------------------------------------
//
// Set the active file
//

static void set_active_file_location(struct FILE_LOCATION* loc, int modified) {
	struct UI* ui = g_ui;

	/* If an active file location is present, release this first */
	if (active_file_location && active_file_location != loc)
		destroy_file_location(active_file_location);
	active_file_location = loc;

	/* Update the visibility of some menu items */
	gboolean v = (loc ? TRUE : FALSE);
	gtk_widget_set_sensitive(ui->save_menu_item, v);
	gtk_widget_set_sensitive(ui->close_menu_item, v);
	gtk_widget_set_sensitive(ui->export_menu_item, v);
	gtk_widget_set_sensitive(ui->import_menu_item, !v);	/* inverted */

	/* The left side */
	gtk_widget_set_sensitive(ui->filter_entry, v);
	gtk_widget_set_sensitive(ui->treeview_scroll, v);

	/* The right side */
	gtk_widget_set_sensitive(ui->title_entry, v);
	gtk_widget_set_sensitive(ui->username_entry, v);
	gtk_widget_set_sensitive(ui->password_entry, v);
	gtk_widget_set_sensitive(ui->url_entry, v);
	gtk_widget_set_sensitive(ui->group_entry, v);
	gtk_widget_set_sensitive(ui->info_text, v);
	gtk_widget_set_sensitive(ui->random_password_button, v);
	gtk_widget_set_sensitive(ui->launch_button, v);
	gtk_widget_set_sensitive(ui->record_save_button, v);

	/* Update the menu title */
	if (!loc) {
		gtk_window_set_title(GTK_WINDOW(ui->main_window), "Keyvault.org");
		return;
	}

	char* title = NULL;
	if (loc->filename) {
		title = malloc(strlen(loc->filename) + 64);
		sprintf(title, "%s - %s%s", "Keyvault.org", loc->filename, (modified ? "*" : ""));
	}
	else if (loc->title) {
		title = malloc(strlen(loc->title) + 64);
		sprintf(title, "%s - %s%s", "Keyvault.org", loc->title, (modified ? "*" : ""));
	}
	else {
		title = malloc(128);
		sprintf(title, "%s - %s%s", "Keyvault.org", "NEW", (modified ? "*" : ""));
	}
	gtk_window_set_title(GTK_WINDOW(ui->main_window), title);
	free(title);
}

// -----------------------------------------------------------
//
// Ask the user for the passphrase, and calculate the derived keys from it
//

static int gtk_request_passphrase(void) {
	int err=0;

	/* Request the passphrase from the user */
	gchar* passphrase = gtk_dialog_password(NULL, "Enter passphrase");
	if (!passphrase) {
		passkey->valid = 0;
		memset(passkey->data, 0, 32);
		memset(passkey->config, 0, 32);
		memset(passkey->login, 0, 32);
		return -1;
	}

	/* Create the keyvault-data keys */
	pkcs5_pbkdf2_hmac_sha1(passphrase, KEYVAULT_DATA,   KEYVAULT_DATA_ROUNDS,   passkey->data);
	pkcs5_pbkdf2_hmac_sha1(passphrase, KEYVAULT_CONFIG, KEYVAULT_CONFIG_ROUNDS, passkey->config);
	pkcs5_pbkdf2_hmac_sha1(passphrase, KEYVAULT_LOGIN,  KEYVAULT_LOGIN_ROUNDS,  passkey->login);
	passkey->valid = 1;

	/* Cleanup */
	memset(passphrase, 0 ,strlen(passphrase));
	g_free(passphrase);
	return err;
}

// -----------------------------------------------------------
//
// user_decrypt_xml - decrypt an encrypted doc (if needed request passphrase from user)
//

static xmlDoc* user_decrypt_xml(xmlDoc* encrypted_doc) {
	while(1) {
		if (!passkey->valid && (gtk_request_passphrase() != 0))
			break;
		xmlDoc* decrypted_doc = xml_doc_decrypt(encrypted_doc, passkey->data);
		if (decrypted_doc) {
			//~ xmlDocShow(decrypted_doc);
			return decrypted_doc;
		}
		passkey->valid = 0;
	}
	return NULL;
}

// -----------------------------------------------------------
//
// Load the data from a file location
//

static void load_from_file_location(struct FILE_LOCATION* loc) {
	debugf("\n%s(%p)\n", __FUNCTION__, loc);
	struct UI* ui = g_ui;
	fl_todo(loc);

	void* data;
	ssize_t len;
	if (read_data(loc,&data,&len) != 0)
		return;

	// Read the data into an xml structure
	xmlDoc* encrypted_doc = xmlReadMemory(data, len, NULL, NULL, XML_PARSE_RECOVER);
	if (!encrypted_doc) {
		gtk_error("Invalid XML document");
		return;
	}

	// Decrypt the doc
	xmlDoc* doc = user_decrypt_xml(encrypted_doc);
	if (doc) {
		store_file_location(global->config, loc);
	}

	// Move the encrypted xml into the treestore
	import_treestore_from_xml(ui->tree->store, doc);

	/* Set the active file location */
	set_active_file_location(loc, 0);

	// Valid passphrase, then store the configuration
	if (passkey->valid) {
		todo();
#if 0
		xmlNode* new_node = kvo_to_node(loc);
		xmlNode* node_encrypted = xmlNodeEncrypt(new_node, passkey->config, NULL);
		if (node_encrypted) {
			xmlNewProp(node_encrypted, XML_CHAR "title", BAD_CAST loc->title);
			xmlReplaceNode(node, node_encrypted);
			//~ xmlFreeNode(config->node); Enabling this will ruin things (what exactly does "xmlReplaceNode" do, free and/or unlink ???)
		}
#endif
	}
}

// -----------------------------------------------------------
//
// MENU: file -> open
//

static void menu_file_open(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer treestore_ptr) {
	struct UI* ui = g_ui;
	gchar* filename = gtk_dialog_open_file(GTK_WINDOW(ui->main_window), 0);
	if (filename) {
		struct FILE_LOCATION* loc = create_file_location_from_uri(filename);
		if (loc) {
			load_from_file_location(loc);
		}
		else {
			gtk_error("Cannot open file");
		}
	}
}

// -----------------------------------------------------------
//
// MENU: file -> open predefined location
//

static void menu_file_open_location(_UNUSED_ GtkWidget *widget, struct FILE_LOCATION* loc) {
	load_from_file_location(loc);
}

// -----------------------------------------------------------
//
// Close the file
//

static void menu_file_close(_UNUSED_ GtkWidget *widget, _UNUSED_ struct FILE_LOCATION* loc) {
	todo(); /* detect unsaved changes */
	set_active_file_location(NULL, 0);
}

// -----------------------------------------------------------
//
// Save (and edit) a recently used KVO file
//

static void save_to_file_location(struct FILE_LOCATION* loc) {
	g_printf("\n%s(...,%p)\n", __FUNCTION__, loc);
	struct UI* ui = g_ui;
	fl_todo(loc);

	// Get a passphrase if not yet available
	if (!passkey->valid) {
		if (gtk_request_passphrase() != 0)
			return;
		passkey->valid = 1;
	}

	// Create an encrypted xml document
	xmlDoc* doc = export_treestore_to_xml(ui->tree->store);
	assert(doc);
	xmlDoc* doc_encrypted = xml_doc_encrypt(doc, passkey->data);
	assert(doc_encrypted);

	/* Write the data to the file */
	xmlChar* data;
	int len;
	xmlDocDumpFormatMemory(doc_encrypted, &data, &len, 1);
	write_data(loc, data, len);
	xmlFree(data);

	// Free the encrypted doc
	xmlFree(doc_encrypted);
	xmlFree(doc);

	// Valid passphrase, then store the configuration
	if (passkey->valid) {
		todo();
#if 0
		xmlNode* new = kvo_to_node(loc);
		xmlNode* enc_new = xmlNodeEncrypt(new, passkey->config, NULL);
		if (enc_new) {
			xmlNewProp(enc_new, XML_CHAR "title", BAD_CAST loc->title);
			xmlReplaceNode(node, enc_new);
			xmlFreeNode(node);
		}
#endif
	}
}

// -----------------------------------------------------------
//
// MENU: file -> save as
//

static void menu_file_save_as(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer treestore_ptr) {
	struct UI* ui = g_ui;
	gchar* filename = gtk_dialog_save_file(GTK_WINDOW(ui->main_window), 0);
	if (filename) {
		struct FILE_LOCATION* loc = create_file_location_from_uri(filename);
		g_free (filename);
		if (loc)
			save_to_file_location(loc);
		else
			gtk_error("Failed to save file");
	}
}

// -----------------------------------------------------------
//
// MENU: file -> save
//

static void menu_file_save(_UNUSED_ GtkWidget *widget, _UNUSED_ gpointer ptr) {
	if (active_file_location) {
		save_to_file_location(active_file_location);
		gtk_info("Files saved");
	}
}

// -----------------------------------------------------------
//
// MENU: edit -> location
// edit a profile
//

static void menu_file_edit_location(_UNUSED_ GtkWidget* widget, struct FILE_LOCATION* loc) {
	g_printf("%s()\n", __FUNCTION__);
	struct UI* ui = g_ui;

	// Let the use fill in all required fields...
	if (gtk_dialog_request_config(ui->main_window, loc)) {
		setup_menu_items(ui, global->config);
		store_file_location(global->config, loc);
	}
}

// -----------------------------------------------------------
//
// MENU: file -> profile -> new
// create a new profile, and start editing this profile
//

static void menu_new_profile(_UNUSED_ GtkWidget* widget, _UNUSED_ struct CONFIG* config) {
	//~ xmlNode* node = new_config_node(doc);
	struct FILE_LOCATION* loc = create_file_location();
	set_active_file_location(loc, 1);
	setup_menu_items(g_ui, global->config);
	//~ menu_edit_profile(NULL, loc);
	//~ store_file_location(config, loc);
	//~ update_profile_menu(config);
}

// -----------------------------------------------------------
//
// MENU: file -> import
//

static void menu_file_import(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr) {
	struct UI* ui = g_ui;
	GtkTreeStore* treestore = treestore_ptr;
	gchar* filename = gtk_dialog_open_file(GTK_WINDOW(ui->main_window), 2);
	if (strstr(filename,".csv")) {
		import_treestore_from_csv(treestore, filename);
	}
	if (strstr(filename,".xml")) {
		xmlDoc* doc = xmlParseFile(filename);
		if (doc) {
			import_treestore_from_xml(treestore, doc);
			xmlFree(doc);
		}
	}
}

// -----------------------------------------------------------
//
// MENU: file -> export
//

static void menu_file_export(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr) {
	struct UI* ui = g_ui;
	GtkTreeStore* treestore = treestore_ptr;
	gtk_warning("You are about to save your information unencrypted");
	gchar* filename = gtk_dialog_save_file(GTK_WINDOW(ui->main_window), 1);
	if (strstr(filename,".csv")) {
		export_treestore_to_csv(treestore, filename);
	}
	if (strstr(filename,".xml")) {
		xmlDoc* doc = export_treestore_to_xml(treestore);
		if (doc) {
			FILE* fp = fopen(filename, "w");
			if (fp) {
				xmlDocFormatDump(fp, doc, 1);
				fclose(fp);
			}
			xmlFree(doc);
		}
	}
}

// -----------------------------------------------------------
//
// Launch the URL
//

void click_launch_button(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	struct UI* ui = g_ui;
	const gchar* url = gtk_entry_get_text(GTK_ENTRY(ui->url_entry));
	printf("Launch: %s\n",url);
	char* cmd = malloc(strlen(url) + 64);
	sprintf(cmd,"/usr/bin/xdg-open %s",url);
	FILE* fp = popen(cmd,"r");
	if (fp)
		fclose(fp);
}

// -----------------------------------------------------------
//
// A user has made a change to the search field
//

static void treemodel_filter_change(GtkWidget *widget, gpointer ptr) {
	struct UI* ui = g_ui;
	GtkTreeModelFilter* treefilter = ptr;
	const char* text=gtk_entry_get_text(GTK_ENTRY(widget));
	strcpy(ui->tree->filter_text, text);
	gtk_tree_model_filter_refilter(treefilter);
}

static void clear_filter(GtkEntry *entry, _UNUSED_ GtkEntryIconPosition icon_pos, _UNUSED_ GdkEvent* event, _UNUSED_ gpointer user_data) {
	gtk_entry_set_text(entry,"");
}

// -----------------------------------------------------------
//
// Any changes made to the edit fields should be written into the tree store
//

void write_changes_to_treestore(_UNUSED_ GtkWidget* widget, gpointer selection) {
	struct UI* ui = g_ui;
  GtkTreeIter filter_iter;
  GtkTreeModel* filter_model;
	GtkTreeIter child_iter;
	GtkTreeModel* child_model;
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &filter_model, &filter_iter)) {
		GtkTextBuffer* text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->info_text));
		GtkTextIter start,end;
		gtk_text_buffer_get_bounds(text_buffer,&start,&end);
		const gchar* info = gtk_text_buffer_get_text(text_buffer, &start, &end, 0);

		// Migrate filter_iter to child_iter AND filter_model to child_model
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filter_model),	&child_iter,	&filter_iter);
		child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

		// Now update the tree store
		gtk_tree_store_set(GTK_TREE_STORE(child_model), &child_iter,
			COL_TITLE, gtk_entry_get_text(GTK_ENTRY(ui->title_entry)),
			COL_USERNAME, gtk_entry_get_text(GTK_ENTRY(ui->username_entry)),
			COL_PASSWORD, gtk_entry_get_text(GTK_ENTRY(ui->password_entry)),
			COL_URL, gtk_entry_get_text(GTK_ENTRY(ui->url_entry)),
			COL_GROUP, gtk_entry_get_text(GTK_ENTRY(ui->group_entry)),
			COL_INFO, info,
			COL_TIME_MODIFIED, time(0),
			-1);
	}
}

// -----------------------------------------------------------
//
// Create a random password
//

static void click_random_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	struct UI* ui = g_ui;
	gchar* password = create_random_password(12);
	gtk_entry_set_text(GTK_ENTRY(ui->password_entry),password);
	g_free(password);
}

// -----------------------------------------------------------
//
// Add a new item
//

static void click_add_item(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	struct UI* ui = g_ui;
	GtkTreeIter iter;

	/* Insert a new record with a random password */
	gchar* password = create_random_password(12);
	gchar* id = create_random_password(16);
	treestore_add_record(ui->tree->store, &iter, NULL, id, "NEW", "", password, "http://", "", "", time(0), time(0));
	g_free(id);
	g_free(password);

	/* Set focus... */
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui->tree->view));
	gtk_tree_selection_select_iter(selection, &iter);
}

static void click_copy_username(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	printf("%s()\n", __FUNCTION__);
}

static void click_copy_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	printf("%s()\n", __FUNCTION__);
}

// -----------------------------------------------------------
//
// Show and hide the contents of the password field
//

static void focus_in_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	struct UI* ui = g_ui;
	gtk_entry_set_visibility(GTK_ENTRY(ui->password_entry), TRUE);
}

static void focus_out_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	struct UI* ui = g_ui;
	gtk_entry_set_visibility(GTK_ENTRY(ui->password_entry), FALSE);
}

// -----------------------------------------------------------
//
// Popup menu
// http://library.gnome.org/devel/gtk/stable/gtk-migrating-checklist.html#checklist-popup-menu
//

static void do_popup_menu (_UNUSED_ GtkWidget* widget, GdkEventButton *event) {
	struct UI* ui = g_ui;
	int button, event_time;

	if (event) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (ui->tree->popup_menu), NULL, NULL, NULL, NULL, button, event_time);
}

static gboolean my_widget_button_press_event_handler (GtkWidget *widget, GdkEventButton *event) {
	/* Ignore double-clicks and triple-clicks */
	if ((event->button == 3) && (event->type == GDK_BUTTON_PRESS)) {
		do_popup_menu (widget, event);
		return TRUE;
	}
	return FALSE;
}

static gboolean my_widget_popup_menu_handler (GtkWidget *widget) {
	do_popup_menu (widget, NULL);
	return TRUE;
}

// -----------------------------------------------------------
//
// Request the user for a passphrase
//

void menu_test_passphrase(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	gchar* passphrase = gtk_dialog_password(NULL,"Enter passphrase");
	g_printf("passphrase: %s\n",passphrase);
	g_free(passphrase);
}

// -----------------------------------------------------------
//
// Request the user for a new passphrase
//

void menu_edit_change_passphrase(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	int err = gtk_request_passphrase();
	if (err)
		passkey->valid = 0;
	else
		passkey->valid = 1;
}

// -----------------------------------------------------------
//
// Remove menu item
//

static void cb_remove_menu_item(GtkWidget* menu_item, gpointer data) {
	gtk_remove_menu_item(data, menu_item);
}

// -----------------------------------------------------------
//
// Setup the file menu
//

static void setup_menu_items(struct UI* ui, struct CONFIG* config) {
	printf("%s()\n", __FUNCTION__);

	/* remove all items in the file and edit menu */
	gtk_container_foreach(GTK_CONTAINER(ui->file_menu), cb_remove_menu_item, ui->file_menu);
	gtk_container_foreach(GTK_CONTAINER(ui->edit_menu), cb_remove_menu_item, ui->edit_menu);

	/* FILE: open, save, save as */
	ui->new_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "New", G_CALLBACK(menu_new_profile), NULL);
	ui->open_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "Open file", G_CALLBACK(menu_file_open), ui->tree->store);
	ui->save_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "Save", G_CALLBACK(menu_file_save), ui->tree->store);
	gtk_add_menu_item_clickable(ui->file_menu, "Save as...", G_CALLBACK(menu_file_save_as), ui->tree->store);
	ui->edit_location_item = gtk_add_menu(ui->file_menu, "Edit...");
	gtk_add_separator(ui->file_menu);

	/* EDIT: add, copy */
	GtkWidget* add_item = gtk_add_menu_item_clickable(ui->edit_menu, "Add item", G_CALLBACK(click_add_item), NULL);
	GtkWidget* copy_username_menu_item = gtk_add_menu_item_clickable(ui->edit_menu, "Copy username", G_CALLBACK(click_copy_username), NULL);
	GtkWidget* copy_password_menu_item = gtk_add_menu_item_clickable(ui->edit_menu, "Copy passphrase", G_CALLBACK(click_copy_password), NULL);
	gtk_add_separator(ui->edit_menu);
	gtk_add_menu_item_clickable(ui->edit_menu, "Change passphrase", G_CALLBACK(menu_edit_change_passphrase), NULL);

	/* FILE+EDIT: predefined files */
	if (config) {
		int idx = 0;
		while(1) {
			struct FILE_LOCATION* loc = get_file_location_by_index(config, idx++);
			if (!loc)
				break;
			//~ if (idx == 1)
				//~ gtk_add_separator(ui->edit_menu);
			gtk_add_menu_item_clickable(ui->file_menu, loc->title, G_CALLBACK(menu_file_open_location), loc);
			//~ gtk_add_menu_item_clickable(ui->edit_menu, loc->title, G_CALLBACK(menu_file_edit_location), loc);
			gtk_add_menu_item_clickable(ui->edit_location_item, loc->title, G_CALLBACK(menu_file_edit_location), loc);
		}
		if (idx > 1)
			gtk_add_separator(ui->file_menu);
	}

	/* import/export */
	ui->import_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "Import...", G_CALLBACK(menu_file_import), ui->tree->store);
	ui->export_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "Export...", G_CALLBACK(menu_file_export), ui->tree->store);
	gtk_add_separator(ui->file_menu);

	/* exit */
	ui->close_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "Close", G_CALLBACK(menu_file_close), NULL);
	ui->exit_menu_item = gtk_add_menu_item_clickable(ui->file_menu, "Exit", G_CALLBACK(gtk_main_quit), NULL);

	/* FILE: Set the accelerators */
  gtk_widget_add_accelerator(ui->open_menu_item, "activate", ui->accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(ui->save_menu_item, "activate", ui->accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(ui->exit_menu_item, "activate", ui->accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	/* EDIT: Set the accelerators */
	gtk_widget_add_accelerator(add_item, "activate", ui->accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(copy_username_menu_item, "activate", ui->accel_group, GDK_u, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(copy_password_menu_item, "activate", ui->accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	/* Disable some of the items */
	set_active_file_location(NULL, 0);

	/* Update the application */
	gtk_widget_show_all(ui->file_menu);
	gtk_widget_show_all(ui->edit_menu);
}

// -----------------------------------------------------------
//
// The selection has changed
//

static char* strdup_ctime(time_t xtime) {
	char* tmp=malloc(32);
	ctime_r(&xtime,tmp);
	tmp[strlen(tmp)-1]=0;
	return tmp;
}

static void treeview_selection_changed(GtkWidget *widget, gpointer statusbar) {
	struct UI* ui = g_ui;
  GtkTreeIter iter;
  GtkTreeModel *model;
  char* id;
  char* title;
	char* username;
  char* password;
	char* url;
  char* group;
  char* info;
  time_t time_created;
  time_t time_modified;

  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		// Read the item
    gtk_tree_model_get(model, &iter,
			COL_ID, &id,
			COL_TITLE, &title,
			COL_USERNAME, &username,
			COL_PASSWORD, &password,
			COL_URL, &url,
			COL_GROUP, &group,
			COL_INFO, &info,
			COL_TIME_CREATED, &time_created,
			COL_TIME_MODIFIED, &time_modified, -1);

		// Update the status bar
		char* time_created_text = strdup_ctime(time_created);
		char* time_modified_text = strdup_ctime(time_modified);
    gtk_statusbar_push(GTK_STATUSBAR(statusbar),gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),title), title);

		// Update the input fields
		gtk_entry_set_text(GTK_ENTRY(ui->title_entry),(title?title:""));
		gtk_entry_set_text(GTK_ENTRY(ui->username_entry),(username?username:""));
		gtk_entry_set_text(GTK_ENTRY(ui->password_entry),(password?password:""));
		gtk_entry_set_text(GTK_ENTRY(ui->url_entry),(url?url:""));
		gtk_entry_set_text(GTK_ENTRY(ui->group_entry),(group?group:""));
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->info_text));
		if (info)
			gtk_text_buffer_set_text(buffer,info,-1);
		else
			gtk_text_buffer_set_text(buffer,"",-1);
		gtk_label_set_markup(GTK_LABEL(ui->time_created_label), time_created_text);
		gtk_label_set_markup(GTK_LABEL(ui->time_modified_label), time_modified_text);

    free(time_modified_text);
    free(time_created_text);

		// Free
    g_free(id);
    g_free(title);
    g_free(username);
    g_free(password);
    g_free(url);
    g_free(group);
    g_free(info);
  }
}

// -----------------------------------------------------------
//
// treemodel_visible_func()
// Filter function
// It filters the title column, but potentially could filter other columns
//

static gboolean treemodel_visible_func (GtkTreeModel *model, GtkTreeIter  *iter, _UNUSED_ gpointer data) {
	struct UI* ui = g_ui;
	gchar *str;
	gboolean visible = FALSE;
	gtk_tree_model_get (model, iter, COL_TITLE, &str, -1);
	if (str) {
		lowercase(str);
		if (strstr(str, ui->tree->filter_text))
			visible = TRUE;
		g_free (str);
	}

	return visible;
}

// -----------------------------------------------------------
//
// treestore sort_func()
//

static gint treestore_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, _UNUSED_ gpointer data){
	gchar *str1,*str2;
	gtk_tree_model_get (model, a, COL_TITLE, &str1, -1);
	gtk_tree_model_get (model, b, COL_TITLE, &str2, -1);
	int retval=0;
	if (str1 && str2) {
		lowercase(str1);
		lowercase(str2);
		retval=strcmp(str1,str2);
		g_free (str1);
		g_free (str2);
	}
	return retval;
}

// -----------------------------------------------------------
//
// treestore_click_col1()
//

int sort_order=0;
static void treestore_reverse_sort_order(GtkWidget *widget, gpointer ptr) {
	GtkTreeViewColumn* col1 = (GtkTreeViewColumn*)widget;
	GtkTreeStore* treestore = (GtkTreeStore*)ptr;
	sort_order = 1-sort_order;
	gtk_tree_view_column_set_sort_order(col1, sort_order);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treestore), COL_TITLE, sort_order);
}

// -----------------------------------------------------------
//
// create_view_and_model()
//

static struct TREEVIEW* create_view_and_model(void) {
	struct TREEVIEW* tree = calloc(1,sizeof(struct TREEVIEW));

	// Malloc the filter
	tree->filter_text = calloc(1,1024);

	// Create the treestore (6 strings fields + 2 time fields)
	tree->store = gtk_tree_store_new(NUM_COLS,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_UINT, G_TYPE_UINT
	);

	// Sort on title
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tree->store), COL_TITLE, treestore_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tree->store), COL_TITLE, sort_order);

	// Create the filtered model ("filter_title")
	tree->model = gtk_tree_model_filter_new(GTK_TREE_MODEL(tree->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(tree->model), treemodel_visible_func, NULL, NULL);
	tree->view = gtk_tree_view_new_with_model(tree->model);

	// Create column 1 and set the title
  GtkTreeViewColumn* col1 = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col1, "Titles");
  gtk_tree_view_column_set_clickable(col1, TRUE);
  gtk_tree_view_column_set_sort_indicator(col1, TRUE);
  //~ gtk_tree_view_column_set_sort_column_id(col1, COL_TITLE);	// Is this easier ? It currently only gives me trouble...
  g_signal_connect(G_OBJECT (col1), "clicked", G_CALLBACK(treestore_reverse_sort_order), tree->store);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree->view), col1);

  // Define how column 1 looks like
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col1, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col1, renderer, "text", COL_TITLE);

	// Make the title searchable
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree->view), COL_TITLE);

	// Add scrolling capabilities (noi mouse support...)
	//~ GtkWidget* sw = addScrollBarToTreeView(tree->view);
	//~ gtk_box_pack_start(GTK_BOX(vbox_left), sw , TRUE, TRUE, 1);

	// POPUP MENU
  g_signal_connect(G_OBJECT(tree->view), "button-press-event", G_CALLBACK(my_widget_button_press_event_handler), NULL);
  g_signal_connect(G_OBJECT(tree->view), "popup-menu", G_CALLBACK(my_widget_popup_menu_handler), NULL);

	tree->popup_menu = gtk_menu_new ();
	//~ g_signal_connect (menu, "deactivate",G_CALLBACK(gtk_widget_destroy), NULL);

	// Add the accelerator
  GtkAccelGroup* popup_accel_group = gtk_accel_group_new ();
  gtk_menu_set_accel_group(GTK_MENU (tree->popup_menu), popup_accel_group);

	/* ... add menu items ... */
	gtk_add_menu_item_clickable(tree->popup_menu, "add", G_CALLBACK(click_add_item), NULL);
	//~ gtk_widget_add_accelerator (add_menu_item, "activate", popup_accel_group, GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_add_menu_item_clickable(tree->popup_menu, "quit", G_CALLBACK(gtk_main_quit), NULL);
	gtk_menu_attach_to_widget (GTK_MENU (tree->popup_menu), tree->view, NULL);
	gtk_widget_show_all(tree->popup_menu);

	/* Return the create tree */
	return tree;
}

// -----------------------------------------------------------
//
// main()
//

static void click_about(_UNUSED_ GtkWidget* widget, gpointer parent_window)
{
	const char* authors[]={"Jelle Martijn Kok <jmkok@youcom.nl>",NULL};
	gtk_show_about_dialog (parent_window,
		"program-name", "KeyVault.org",
		"title", "About KeyVault.org",
		"logo-icon-name", GTK_STOCK_DIALOG_AUTHENTICATION,
		"authors", authors,
		"website", "http://www.keyvault.org/",
		NULL);
}

// -----------------------------------------------------------
//
// main()
//

	// GTK_STOCK_CANCEL
	// GTK_STOCK_DELETE
	// GTK_STOCK_CLEAR
	// GTK_STOCK_DIALOG_AUTHENTICATION

int create_main_window(struct SETUP* setup) {
	// Initialize the generic global component
	global = setup;
	passkey = calloc(1,sizeof(struct PASSKEY));
	struct UI* ui = calloc(1,sizeof(struct UI));
	g_ui = ui;

	GtkWidget* label;

	// Initialize gtk
  gtk_window_set_default_icon_name(GTK_STOCK_DIALOG_AUTHENTICATION);

	// Create the primary window
  ui->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ui->main_window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(ui->main_window), "Keyvault.org");
	//~ gtk_window_set_icon_from_file(GTK_WINDOW(main_window), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(ui->main_window), GTK_STOCK_DIALOG_AUTHENTICATION);
  gtk_widget_set_size_request (ui->main_window, 700, 600);

	/* Create the accel group */
  ui->accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (ui->main_window), ui->accel_group);

	// Create the treeview (do not place yet)
  ui->tree = create_view_and_model();

	// Make the vbox and put it in the main window
  GtkWidget* vbox = gtk_vbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(ui->main_window), vbox);

	// Menu bar
	GtkWidget* menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 1);
	ui->file_menu = gtk_add_menu(menu_bar,"File");
	ui->edit_menu = gtk_add_menu(menu_bar,"Edit");
	GtkWidget* test_menu = gtk_add_menu(menu_bar,"Test");
	//~ GtkWidget* view_menu = gtk_add_menu(menu_bar,"View");
	GtkWidget* help_menu = gtk_add_menu(menu_bar,"Help");

	// Test menu
	gtk_add_menu_item(test_menu, "SSH");
	gtk_add_menu_item_clickable(test_menu, "passphrase", G_CALLBACK(menu_test_passphrase), ui->main_window);

	// Help menu
	gtk_add_menu_item_clickable(help_menu, "About", G_CALLBACK(click_about), ui->main_window);

	// This is the container for the treeview AND the record info
	// Make the hbox and put it inside the vbox
	//~ GtkWidget* hbox = gtk_hbox_new(FALSE, 2);
	GtkWidget* hbox = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hbox),200);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);

	/* The left side (treeview) */
	GtkWidget* vbox_left = gtk_vbox_new(FALSE, 2);
	gtk_paned_add1(GTK_PANED(hbox), vbox_left);

	/* Treeview filter entry */
	ui->filter_entry = gtk_entry_new();
	gtk_entry_set_icon_from_stock(GTK_ENTRY(ui->filter_entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	gtk_box_pack_start(GTK_BOX(vbox_left), ui->filter_entry, FALSE, TRUE, 1);

	/* Place the tree view in a scrollable widget */
	ui->treeview_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ui->treeview_scroll), ui->tree->view);
	gtk_box_pack_start(GTK_BOX(vbox_left), ui->treeview_scroll , TRUE, TRUE, 1);

	/* The right side (record info) */
	GtkWidget* vbox_right = gtk_vbox_new(FALSE, 2);
	gtk_paned_add2(GTK_PANED(hbox), vbox_right);

	/* Add the title and username input entry */
	ui->title_entry = gtk_add_labeled_entry(vbox_right, "Title", NULL);
	ui->username_entry = gtk_add_labeled_entry(vbox_right, "Username", NULL);

	/* Add the password entry and "random" button */
	label = gtk_label_new("Password");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	GtkWidget* box = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		ui->password_entry = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(ui->password_entry), FALSE);
		gtk_box_pack_start(GTK_BOX(box), ui->password_entry , TRUE, TRUE, 0);
		ui->random_password_button = gtk_button_new_with_label("Random");
		gtk_box_pack_start(GTK_BOX(box), ui->random_password_button , FALSE, TRUE, 0);

	/* Add the url entry and and "launch" button */
	label = gtk_label_new("Url");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	box = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		ui->url_entry = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(box), ui->url_entry , TRUE, TRUE, 0);
		ui->launch_button = gtk_button_new_with_label("Launch");
		gtk_box_pack_start(GTK_BOX(box), ui->launch_button , FALSE, TRUE, 0);

	/* Add the group entry */
	ui->group_entry = gtk_add_labeled_entry(vbox_right, "Group", NULL);

	/* Add the information label */
	label = gtk_label_new("Information");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);

	/* Add the information entry */
	ui->info_text = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ui->info_text),GTK_WRAP_WORD);
	GtkWidget* scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll),ui->info_text);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox_right), scroll , TRUE, TRUE, 1);

	/* Add the created and modified time label */
	box = gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		// Add the created time label
		ui->time_created_label = gtk_label_new("Created...");
		gtk_misc_set_alignment (GTK_MISC(ui->time_created_label), 0, 0);
		gtk_box_pack_start(GTK_BOX(box), ui->time_created_label , FALSE, TRUE, 1);
		// Add the modified time label
		ui->time_modified_label = gtk_label_new("Modified...");
		gtk_misc_set_alignment (GTK_MISC(ui->time_modified_label), 0, 0);
		gtk_box_pack_start(GTK_BOX(box), ui->time_modified_label , FALSE, TRUE, 1);
		// Set the texts to be grey
		GdkColor grey_color;
		gdk_color_parse ("grey", &grey_color);
		gtk_widget_modify_fg (ui->time_created_label, GTK_STATE_NORMAL, &grey_color);
		gtk_widget_modify_fg (ui->time_modified_label, GTK_STATE_NORMAL, &grey_color);

	/* The record save button */
  ui->record_save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
  gtk_box_pack_start(GTK_BOX(vbox_right), ui->record_save_button , FALSE, TRUE, 1);

	/* Status bar... */
  GtkWidget* statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

	/* Connects... */
  g_signal_connect(G_OBJECT (ui->main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT (ui->launch_button), "clicked", G_CALLBACK(click_launch_button), ui->tree->view);
  g_signal_connect(G_OBJECT (ui->random_password_button), "clicked", G_CALLBACK(click_random_password), ui->main_window);

	/* Connect the password entry */
	g_signal_connect(G_OBJECT (ui->password_entry), "focus-in-event", G_CALLBACK(focus_in_password), NULL);
	g_signal_connect(G_OBJECT (ui->password_entry), "focus-out-event", G_CALLBACK(focus_out_password), NULL);

	/* Connect to any changes made to the treeview */
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui->tree->view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect(selection, "changed", G_CALLBACK(treeview_selection_changed), statusbar);

	/* Connect to any changes made to the input fields */
	//~ g_signal_connect(G_OBJECT (title_entry), "changed", G_CALLBACK(write_changes_to_ui->tree->store), selection);
	//~ g_signal_connect(G_OBJECT (username_entry), "changed", G_CALLBACK(write_changes_to_ui->tree->store), selection);
	//~ g_signal_connect(G_OBJECT (password_entry), "changed", G_CALLBACK(write_changes_to_ui->tree->store), selection);
	//~ g_signal_connect(G_OBJECT (url_entry), "changed", G_CALLBACK(write_changes_to_ui->tree->store), selection);
	//~ g_signal_connect(G_OBJECT (info_text), "focus-out-event", G_CALLBACK(write_changes_to_treestore), selection);

	/* Connect to the "save" button */
	g_signal_connect(G_OBJECT (ui->record_save_button), "clicked", G_CALLBACK(write_changes_to_treestore), selection);

	g_signal_connect(G_OBJECT (ui->filter_entry), "changed", G_CALLBACK(treemodel_filter_change), ui->tree->model);
	g_signal_connect(G_OBJECT (ui->filter_entry), "icon-press", G_CALLBACK(clear_filter), NULL);

	/* Read the configuration... */
	setup->config = read_configuration("config.xml");

	/* File menu */
	setup_menu_items(ui, setup->config);

	/* Show the application */
  gtk_widget_show_all(ui->main_window);

	/* Is a default filename given, then read that file */
	if (setup->default_filename) {
		struct FILE_LOCATION* loc = create_file_location_from_uri(setup->default_filename);
		if (loc)
			load_from_file_location(loc);
	}

	/* Run the app */
  gtk_main();

	/* Save the configuration... */
	save_configuration("config.xml", setup->config);

  return 0;
}
