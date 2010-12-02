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
#include <sys/time.h>
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
#include "ssh.h"

// -----------------------------------------------------------
//
// The encryption variables
// The keys are built from the same passphrase, but with different salts and rounds
//

static int passphrase_valid = 0;
static u_char passphrase_data[32];		// This key is used to encrypt/decrypt the container
static u_char passphrase_config[32];	// This key is used to encrypt/decrypt some of the configuration fields
static u_char passphrase_login[32];		// This key is used to login to keyvault.org (todo)

// The salts and rounds for the keys (not secret, they just need to be different)
#define KEYVAULT_DATA   "5NewDdGpLQ0W-keyvault-data"
#define KEYVAULT_CONFIG "n41JFWQAdmcf-keyvault-config"
#define KEYVAULT_LOGIN  "S3ftaw7kgXlc-keyvault-login"
#define KEYVAULT_DATA_ROUNDS   (4*1024)
#define KEYVAULT_CONFIG_ROUNDS (5*1024)
#define KEYVAULT_LOGIN_ROUNDS  (6*1024)

// -----------------------------------------------------------
//
// Global variables
//

struct tGlobal* global;

static char* active_filename = NULL;
//~ static tFileDescription* active_file;
GtkWidget* popup_menu;

// -----------------------------------------------------------
//
// Global widgets
//

// The main window
GtkWidget* main_window;

// The menus
GtkWidget* open_recent_menu;
GtkWidget* save_recent_menu;

// The entries
GtkWidget* title_entry;
GtkWidget* username_entry;
GtkWidget* password_entry;
GtkWidget* url_entry;
GtkWidget* group_entry;
GtkWidget* info_text;
GtkWidget* time_created_label;
GtkWidget* time_modified_label;

// The data tree
struct tTreeData {
	GtkWidget* treeview;
	GtkTreeStore* treestore;
	GtkTreeModel* treefilter;
	char* filter_title;
};
struct tTreeData* treedata;

// -----------------------------------------------------------
//
// static functions (needed as they reference each other)
//

static void update_recent_list(tList* config_list);

// -----------------------------------------------------------
//
// Ask the user for the passphrase, and calculate the derived keys from it
//

static int gtk_request_passphrase(void) {
	int err=0;
	// Request the passphrase from the user
	gchar* passphrase = gtk_dialog_password(NULL, "Enter passphrase");
	if (!passphrase) {
		passphrase_valid = 0;
		memset(passphrase_data, 0, 32);
		memset(passphrase_config, 0, 32);
		memset(passphrase_login, 0, 32);
		return -1;
	}

	// Create the keyvault-data keys
	pkcs5_pbkdf2_hmac_sha1(passphrase, KEYVAULT_DATA,   KEYVAULT_DATA_ROUNDS,   passphrase_data);
	pkcs5_pbkdf2_hmac_sha1(passphrase, KEYVAULT_CONFIG, KEYVAULT_CONFIG_ROUNDS, passphrase_config);
	pkcs5_pbkdf2_hmac_sha1(passphrase, KEYVAULT_LOGIN,  KEYVAULT_LOGIN_ROUNDS,  passphrase_login);
	passphrase_valid = 1;

	// Cleanup
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
		if (!passphrase_valid && (gtk_request_passphrase() != 0))
			break;
		xmlDoc* decrypted_doc = xml_doc_decrypt(encrypted_doc, passphrase_data);
		if (decrypted_doc) {
			//~ xmlDocShow(decrypted_doc);
			return decrypted_doc;
		}
		passphrase_valid = 0;
	}
	return NULL;
}

// -----------------------------------------------------------
//
// user_decrypt_xml - decrypt an encrypted doc (if needed request passphrase from user)
//

static int load_from_file(const gchar* filename, GtkTreeStore* treestore) {
	debugf("load_from_file('%s',%p)\n", filename, treestore);
	// Read the file
	xmlDoc* encrypted_doc = xmlParseFile(filename);

	// Decrypt the doc
	xmlDoc* doc = user_decrypt_xml(encrypted_doc);

	// Move the encrypted xml into the treestore
	import_treestore_from_xml(treestore, doc);
	
	return 1;
}

// -----------------------------------------------------------
//
// MENU: file -> open
//

static void menu_file_open(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr)
{
	gchar* filename=gtk_dialog_open_file(GTK_WINDOW(main_window), 0);
	if (filename) {
		GtkTreeStore* treestore = treestore_ptr;
		if (load_from_file(filename, treestore)) {
			if (active_filename)
				free(active_filename);
			active_filename = strdup(filename);
		}
		g_free (filename);
	}
}


// -----------------------------------------------------------
//
// MENU: file -> open_advanced
//

static void menu_open_recent_file(_UNUSED_ GtkWidget *widget, gpointer config_pointer) {
	debugf("\nmenu_open_recent_file(...,%p)\n", config_pointer);
	assert(config_pointer);
	tConfigDescription* config = config_pointer;
	xmlNodeShow(config->node);
	
	// First we have to decrypt the node...
	xmlNode* config_node = config->node;
	while (xmlIsNodeEncrypted(config->node)) {
		if (passphrase_valid) {
			xmlNode* tmp = xmlNodeDecrypt(config->node, passphrase_config);
			if (tmp) {
				config_node = tmp;
				break;
			}
		}
		if (gtk_request_passphrase() != 0)
			return;
	}

	// Convert the node into an config object
	tFileDescription* kvo = node_to_kvo(config_node);
	if (!kvo)
		return;

	//~ if (!dialog_request_kvo(main_window, kvo))
		//~ return;

	g_printf("hostname: %s\n",kvo->hostname);
	g_printf("username: %s\n",kvo->username);
	g_printf("password: %s\n",kvo->password);
	g_printf("filename: %s\n",kvo->filename);
	if (!kvo->protocol || (strcmp(kvo->protocol,"local") == 0)) {
		trace();
	}
	else if (strcmp(kvo->protocol,"ssh") == 0) {
		void* data;
		ssize_t len;
		if (ssh_get_file(kvo,&data,&len) != 0)
			return;
		
		// Read the data into an xml structure
		xmlDoc* encrypted_doc = xmlReadMemory(data, len, NULL, NULL, XML_PARSE_RECOVER);
		if (!encrypted_doc) {
			gtk_error("Invalid XML document");
			return;
		}

		// Decrypt the doc
		xmlDoc* doc = user_decrypt_xml(encrypted_doc);

		// Move the encrypted xml into the treestore
		import_treestore_from_xml(treedata->treestore, doc);

		// Valid passphrase, then store the configuration
		if (passphrase_valid) {
			xmlNode* new_node = kvo_to_node(kvo);
			xmlNode* node_encrypted = xmlNodeEncrypt(new_node, passphrase_config, NULL);
			if (node_encrypted) {
				xmlNewProp(node_encrypted, XML_CHAR "title", BAD_CAST kvo->title);
				xmlReplaceNode(config->node, node_encrypted);
				//~ xmlFreeNode(config->node); Enabling this will ruin things (what exactly does "xmlReplaceNode" do, free and/or unlink ???)
			}
		}
	}
}

static void menu_file_open_ssh(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	// Create a new kvo file
	tConfigDescription* config = mallocz(sizeof(tConfigDescription));
	// Let the use fill in all required fields...
	if (gtk_dialog_request_config(main_window, config)) {
		// Store the kvo to the list
		listAdd(global->config_list, config);
		update_recent_list(global->config_list);
		// Open the file...
		menu_open_recent_file(NULL, config);
	}
}

// -----------------------------------------------------------
//
// Save (and edit) a recently used KVO file
//

static void menu_save_recent_file(_UNUSED_ GtkWidget* widget, gpointer config_pointer) {
	g_printf("\nmenu_save_recent_file(...,%p)\n", config_pointer);
	assert(config_pointer);
	tConfigDescription* config = config_pointer;
	xmlNodeShow(config->node);

	// First we have to decrypt the node...
	xmlNode* config_node = config->node;
	while (xmlIsNodeEncrypted(config->node)) {
		if (passphrase_valid) {
			xmlNode* tmp = xmlNodeDecrypt(config->node, passphrase_config);
			if (tmp) {
				config_node = tmp;
				break;
			}
		}
		if (gtk_request_passphrase() != 0)
			return;
	}

	// Convert the node into an config object
	tFileDescription* kvo = node_to_kvo(config_node);
	if (!kvo)
		return;

	// Get a passphrase if not yet available
	if (!passphrase_valid) {
		if (gtk_request_passphrase() != 0)
			return;
		passphrase_valid = 1;
	}

	// Create an encrypted xml document
	xmlDoc* doc = export_treestore_to_xml(treedata->treestore);
	assert(doc);
	xmlDoc* doc_encrypted = xml_doc_encrypt(doc, passphrase_data);
	assert(doc_encrypted);

	// Write the data to the file
	if (!kvo->protocol || (strcmp(kvo->protocol,"local") == 0)) {
		FILE* fp = fopen(kvo->filename, "w");
		if (fp) {
			xmlDocFormatDump(fp, doc_encrypted, 1);
			fclose(fp);
		}
	}
	// Write the data to an ssh account
	else if (strcmp(kvo->protocol,"ssh") == 0) {
		xmlChar* data;
		int len;
		xmlDocDumpFormatMemory(doc_encrypted, &data, &len, 1);
		ssh_put_file(kvo, data, len);
	}

	// Free the encrypted doc
	xmlFree(doc_encrypted);
	xmlFree(doc);

	// Valid passphrase, then store the configuration
	if (passphrase_valid) {
		xmlNode* new = kvo_to_node(kvo);
		xmlNode* enc_new = xmlNodeEncrypt(new, passphrase_config, NULL);
		if (enc_new) {
			xmlNewProp(enc_new, XML_CHAR "title", BAD_CAST kvo->title);
			xmlReplaceNode(config->node, enc_new);
			xmlFreeNode(config->node);
		}
	}
}

static void menu_file_save_ssh(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	// Create a new kvo file
	tConfigDescription* config = mallocz(sizeof(tConfigDescription));
	// Let the use fill in all required fields...
	if (gtk_dialog_request_config(main_window, config)) {
		// Store the kvo to the list
		listAdd(global->config_list, config);
		update_recent_list(global->config_list);
		// Open the file...
		menu_save_recent_file(NULL, config);
	}
}

// -----------------------------------------------------------
//
// save_to_file - save the treestore to a file
//

static void save_to_file(const gchar* filename, GtkTreeStore* treestore) {

	// treestore => xml
	xmlDoc* doc = export_treestore_to_xml(treestore);
	//~ xmlDocShow(doc);

	// Request the passphrase to encode the file
	if (!passphrase_valid) {
		if (gtk_request_passphrase() != 0)
			return;
		passphrase_valid = 1;
	}

	// xml => encrypted-xml
	xmlDoc* enc_doc = xml_doc_encrypt(doc, passphrase_data);
	//~ xmlDocShow(enc_doc);

	// encrypted-xml => disk
	FILE* fp = fopen(filename, "w");
	if (fp) {
		xmlDocFormatDump(fp, enc_doc, 1);
		fclose(fp);
	}
}

// -----------------------------------------------------------
//
// MENU: file -> save as
//

static void menu_file_save_as(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr)
{
	gchar* filename = gtk_dialog_save_file(GTK_WINDOW(main_window), 0);
	if (filename) {
		GtkTreeStore* treestore = treestore_ptr;
		save_to_file(filename, treestore);
		g_free (filename);
	}
}

// -----------------------------------------------------------
//
// MENU: file -> save
//

static void menu_file_save(GtkWidget *widget, gpointer ptr)
{
	if (active_filename) {
		GtkTreeStore* treestore = ptr;
		save_to_file(active_filename, treestore);
	}
	else {
		menu_file_save_as(widget, ptr);
	}
}

// -----------------------------------------------------------
//
// MENU: file -> import
//

static void menu_file_import(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr)
{
	GtkTreeStore* treestore = treestore_ptr;
	gchar* filename = gtk_dialog_open_file(GTK_WINDOW(main_window), 2);
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

static void menu_file_export(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr)
{
	GtkTreeStore* treestore = treestore_ptr;
	gtk_warning("You are about to save your information unencrypted");
	gchar* filename = gtk_dialog_save_file(GTK_WINDOW(main_window), 1);
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
	const gchar* url=gtk_entry_get_text(GTK_ENTRY(url_entry));
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
	GtkTreeModelFilter* treefilter = ptr;
	const char* text=gtk_entry_get_text(GTK_ENTRY(widget));
	strcpy(treedata->filter_title, text);
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
  GtkTreeIter filter_iter;
  GtkTreeModel* filter_model;
	GtkTreeIter child_iter;
	GtkTreeModel* child_model;
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &filter_model, &filter_iter)) {
		GtkTextBuffer* text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text));
		GtkTextIter start,end;
		gtk_text_buffer_get_bounds(text_buffer,&start,&end);
		const gchar* info = gtk_text_buffer_get_text(text_buffer, &start, &end, 0);

		// Migrate filter_iter to child_iter AND filter_model to child_model
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filter_model),	&child_iter,	&filter_iter);
		child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

		// Now update the tree store
		gtk_tree_store_set(GTK_TREE_STORE(child_model), &child_iter, 
			COL_TITLE, gtk_entry_get_text(GTK_ENTRY(title_entry)), 
			COL_USERNAME, gtk_entry_get_text(GTK_ENTRY(username_entry)), 
			COL_PASSWORD, gtk_entry_get_text(GTK_ENTRY(password_entry)),
			COL_URL, gtk_entry_get_text(GTK_ENTRY(url_entry)),
			COL_GROUP, gtk_entry_get_text(GTK_ENTRY(group_entry)),
			COL_INFO, info,
			COL_TIME_MODIFIED, time(0),
			-1);
	}
}

// -----------------------------------------------------------
//
// Load "save.kvo"
//

static void menu_test_quick_load(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr) {
	GtkTreeStore* treestore = treestore_ptr;
	load_from_file("test.kvo", treestore);
}

// -----------------------------------------------------------
//
// Save "save.kvo"
//

static void menu_test_quick_save(_UNUSED_ GtkWidget* widget, gpointer treestore_ptr) {
	GtkTreeStore* treestore = treestore_ptr;
	save_to_file("test.kvo", treestore);
}

// -----------------------------------------------------------
//
// Create a random password
//

static void click_random_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	gchar* password = create_random_password(12);
	gtk_entry_set_text(GTK_ENTRY(password_entry),password);
	g_free(password);
}

// -----------------------------------------------------------
//
// Add a new item
//

static void click_add_item(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	GtkTreeIter iter;
	// Insert a new record with a random password
	gchar* password = create_random_password(12);
	gchar* id = create_random_password(16);
	treestore_add_record(treedata->treestore, &iter, NULL, id, "NEW", "", password, "http://", "", "", time(0), time(0));
	g_free(id);
	g_free(password);
	// Set focus...
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treedata->treeview));
	gtk_tree_selection_select_iter(selection, &iter);
}

static void click_copy_username(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	printf("click_copy_username\n");
}

static void click_copy_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	printf("click_copy_password\n");
}

// -----------------------------------------------------------
//
// Show and hide the contents of the password field
//

static void focus_in_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	//~ GtkWidget*
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), TRUE);
}

static void focus_out_password(_UNUSED_ GtkWidget* widget, _UNUSED_ gpointer data) {
	//~ GtkWidget*
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
}

// -----------------------------------------------------------
//
// Popup menu
// http://library.gnome.org/devel/gtk/stable/gtk-migrating-checklist.html#checklist-popup-menu
//

static void do_popup_menu (_UNUSED_ GtkWidget* widget, GdkEventButton *event) {
	int button, event_time;

	if (event) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL, NULL, NULL, button, event_time);
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
		passphrase_valid = 0;
	else
		passphrase_valid = 1;
}

// -----------------------------------------------------------
//
// Update the list of recent files
//

static void update_recent_list(tList* config_list) {
	// First remove all items in the recent menu
	void cb_remove_menu_item(GtkWidget* menu_item, _UNUSED_ gpointer data) {
		gtk_remove_menu_item(open_recent_menu, menu_item);
	}
	gtk_container_foreach(GTK_CONTAINER(open_recent_menu), cb_remove_menu_item, 0);
	gtk_container_foreach(GTK_CONTAINER(save_recent_menu), cb_remove_menu_item, 0);

	// Then add all items in the config_list to the recent menu
	void add_to_open_menu(_UNUSED_ tList* list, void* data) {
		tConfigDescription* config = data;
		gtk_add_menu_item_clickable(open_recent_menu, config->title, G_CALLBACK(menu_open_recent_file), config);
	}
	gtk_add_menu_item_clickable(open_recent_menu, "SSH...", G_CALLBACK(menu_file_open_ssh), NULL);
	gtk_add_separator(open_recent_menu);
	listForeach(config_list, add_to_open_menu);
	gtk_widget_show_all(open_recent_menu);

	// Then add all items in the config_list to the recent menu
	void add_to_save_menu(_UNUSED_ tList* list, void* data) {
		tConfigDescription* config = data;
		gtk_add_menu_item_clickable(save_recent_menu, config->title, G_CALLBACK(menu_save_recent_file), config);
	}
	gtk_add_menu_item_clickable(save_recent_menu, "SSH...", G_CALLBACK(menu_file_save_ssh), NULL);
	gtk_add_separator(save_recent_menu);
	listForeach(config_list, add_to_save_menu);
	gtk_widget_show_all(save_recent_menu);
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

static void treeview_selection_changed(GtkWidget *widget, gpointer statusbar)
{
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
		gtk_entry_set_text(GTK_ENTRY(title_entry),(title?title:""));
		gtk_entry_set_text(GTK_ENTRY(username_entry),(username?username:""));
		gtk_entry_set_text(GTK_ENTRY(password_entry),(password?password:""));
		gtk_entry_set_text(GTK_ENTRY(url_entry),(url?url:""));
		gtk_entry_set_text(GTK_ENTRY(group_entry),(group?group:""));
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text));
		if (info)
			gtk_text_buffer_set_text(buffer,info,-1);
		else
			gtk_text_buffer_set_text(buffer,"",-1);
		gtk_label_set_markup(GTK_LABEL(time_created_label), time_created_text);
		gtk_label_set_markup(GTK_LABEL(time_modified_label), time_modified_text);

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

static gboolean treemodel_visible_func (GtkTreeModel *model, GtkTreeIter  *iter, _UNUSED_ gpointer data)
{
	gchar *str;
	gboolean visible = FALSE;
	gtk_tree_model_get (model, iter, COL_TITLE, &str, -1);
	if (str) {
		lowercase(str);
		if (strstr(str, treedata->filter_title))
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

static struct tTreeData* create_view_and_model(void) {
	struct tTreeData* td = mallocz(sizeof(struct tTreeData));
	treedata = td;

	// Malloc the filter
	td->filter_title=malloc(1024);
	*td->filter_title=0;

	// Create the treestore (6 strings fields + 2 time fields)
	td->treestore = gtk_tree_store_new(NUM_COLS,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
		G_TYPE_UINT, G_TYPE_UINT
	);

	// Sort on title
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(td->treestore), COL_TITLE, treestore_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(td->treestore), COL_TITLE, sort_order);

	// Create the filtered model ("filter_title")
	td->treefilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(td->treestore), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(td->treefilter), treemodel_visible_func, NULL, NULL);
	td->treeview = gtk_tree_view_new_with_model(td->treefilter);

	// Create column 1 and set the title
  GtkTreeViewColumn* col1 = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col1, "Titles");
  gtk_tree_view_column_set_clickable(col1, TRUE);
  gtk_tree_view_column_set_sort_indicator(col1, TRUE);
  //~ gtk_tree_view_column_set_sort_column_id(col1, COL_TITLE);	// Is this easier ? It currently only gives me trouble...
  g_signal_connect(G_OBJECT (col1), "clicked", G_CALLBACK(treestore_reverse_sort_order), td->treestore);
  gtk_tree_view_append_column(GTK_TREE_VIEW(td->treeview), col1);
  
  // Define how column 1 looks like
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col1, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col1, renderer, "text", COL_TITLE);

	// Make the title searchable
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(td->treeview), COL_TITLE);

	// Return the tree data
  return td;
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
		"website", "http://www.keyvault.org",
		//~ "website-label", "http://www.keyvault.org",
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

int create_main_window(const char* default_filename) {
	// Initialize random generator
	struct timeval tv;
	gettimeofday(&tv,0);
	srand(tv.tv_sec ^ tv.tv_usec);

	// Initialize the generic global component
	global = mallocz(sizeof(struct tGlobal));
	global->config_list=listCreate();

	GtkWidget* label;

	// Initialize gtk
  gtk_window_set_default_icon_name(GTK_STOCK_DIALOG_AUTHENTICATION);

	// Create the primary window
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(main_window), "Keyvault.org");
	//~ gtk_window_set_icon_from_file(GTK_WINDOW(main_window), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(main_window), GTK_STOCK_DIALOG_AUTHENTICATION);
  gtk_widget_set_size_request (main_window, 700, 600);

	// Create the treeview (do not place yet)
  struct tTreeData* td = create_view_and_model();

	// Make the vbox and put it in the main window
  GtkWidget* vbox = gtk_vbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(main_window), vbox);

	// Menu bar
	GtkWidget* menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 1);
	GtkWidget* file_menu = gtk_add_menu(menu_bar,"File");
	GtkWidget* edit_menu = gtk_add_menu(menu_bar,"Edit");
	GtkWidget* test_menu = gtk_add_menu(menu_bar,"Test");
	//~ GtkWidget* view_menu = gtk_add_menu(menu_bar,"View");
	GtkWidget* help_menu = gtk_add_menu(menu_bar,"Help");

	// File menu
	GtkWidget* open_menu_item = gtk_add_menu_item_clickable(file_menu, "Open file", G_CALLBACK(menu_file_open), td->treestore);
	open_recent_menu = gtk_add_menu(file_menu, "Open recent");
	GtkWidget* save_menu_item = gtk_add_menu_item_clickable(file_menu, "Save", G_CALLBACK(menu_file_save), td->treestore);
	save_recent_menu = gtk_add_menu(file_menu, "Save recent");
	gtk_add_menu_item_clickable(file_menu, "Save as...", G_CALLBACK(menu_file_save_as), td->treestore);
	gtk_add_separator(file_menu);
	gtk_add_menu_item_clickable(file_menu, "Import...", G_CALLBACK(menu_file_import), td->treestore);
	gtk_add_menu_item_clickable(file_menu, "Export...", G_CALLBACK(menu_file_export), td->treestore);
	gtk_add_separator(file_menu);
	//~ gtk_add_menu_item_clickable(file_menu, "Read configuration", G_CALLBACK(save_configuration), NULL);
	//~ gtk_add_menu_item_clickable(file_menu, "Save configuration", G_CALLBACK(read_configuration), NULL);
	//~ gtk_add_separator(file_menu);
	GtkWidget* exit_menu_item = gtk_add_menu_item_clickable(file_menu, "Exit", G_CALLBACK(gtk_main_quit), NULL);

	// Edit menu
	gtk_add_menu_item_clickable(edit_menu, "Add item", G_CALLBACK(click_add_item), NULL);
	GtkWidget* copy_username_menu_item = gtk_add_menu_item_clickable(edit_menu, "Copy username", G_CALLBACK(click_copy_username), NULL);
	GtkWidget* copy_password_menu_item = gtk_add_menu_item_clickable(edit_menu, "Copy passphrase", G_CALLBACK(click_copy_password), NULL);
	gtk_add_separator(edit_menu);
	gtk_add_menu_item_clickable(edit_menu, "Change passphrase", G_CALLBACK(menu_edit_change_passphrase), NULL);

	// Test menu
	gtk_add_menu_item(test_menu, "SSH");
	gtk_add_menu_item_clickable(test_menu, "passphrase", G_CALLBACK(menu_test_passphrase), main_window);
	gtk_add_menu_item_clickable(test_menu, "quick load", G_CALLBACK(menu_test_quick_load), td->treestore);
	gtk_add_menu_item_clickable(test_menu, "quick save", G_CALLBACK(menu_test_quick_save), td->treestore);

	// Help menu
	gtk_add_menu_item_clickable(help_menu, "About", G_CALLBACK(click_about), main_window);

	// This is the container for the treeview AND the record info
	// Make the hbox and put it inside the vbox
	//~ GtkWidget* hbox = gtk_hbox_new(FALSE, 2);
	GtkWidget* hbox = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hbox),200);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);

	// The left side (treeview)
	GtkWidget* vbox_left = gtk_vbox_new(FALSE, 2);
	gtk_paned_add1(GTK_PANED(hbox), vbox_left);

	// Treeview filter entry
	GtkWidget* filter_entry = gtk_entry_new();
	gtk_entry_set_icon_from_stock(GTK_ENTRY(filter_entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	gtk_box_pack_start(GTK_BOX(vbox_left), filter_entry, FALSE, TRUE, 1);

	// Add scrolling capabilities (noi mouse support...)
	//~ GtkWidget* sw = addScrollBarToTreeView(td->treeview);
	//~ gtk_box_pack_start(GTK_BOX(vbox_left), sw , TRUE, TRUE, 1);
	
	// Add scrolling capabilities
	GtkWidget* treeview_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox_left), treeview_scroll , TRUE, TRUE, 1);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(treeview_scroll), td->treeview);
	
	// POPUP MENU
  g_signal_connect(G_OBJECT(td->treeview), "button-press-event", G_CALLBACK(my_widget_button_press_event_handler), NULL);
  g_signal_connect(G_OBJECT(td->treeview), "popup-menu", G_CALLBACK(my_widget_popup_menu_handler), NULL);

	popup_menu = gtk_menu_new ();
	//~ g_signal_connect (menu, "deactivate",G_CALLBACK(gtk_widget_destroy), NULL);

	// Add the accelerator
  GtkAccelGroup* popup_accel_group = gtk_accel_group_new ();
  gtk_menu_set_accel_group(GTK_MENU (popup_menu), popup_accel_group);

	/* ... add menu items ... */
	gtk_add_menu_item_clickable(popup_menu, "add", G_CALLBACK(click_add_item), NULL);
	//~ gtk_widget_add_accelerator (add_menu_item, "activate", popup_accel_group, GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_add_menu_item_clickable(popup_menu, "quit", G_CALLBACK(gtk_main_quit), NULL);
	gtk_menu_attach_to_widget (GTK_MENU (popup_menu), td->treeview, NULL);
	gtk_widget_show_all(popup_menu);

	// The right side (record info)
	GtkWidget* vbox_right = gtk_vbox_new(FALSE, 2);
	gtk_paned_add2(GTK_PANED(hbox), vbox_right);

	title_entry = gtk_add_labeled_entry(vbox_right, "Title", NULL);
	username_entry = gtk_add_labeled_entry(vbox_right, "Username", NULL);

	// Add the password entry and button
	label = gtk_label_new("Password");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	GtkWidget* box = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		password_entry = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
		gtk_box_pack_start(GTK_BOX(box), password_entry , TRUE, TRUE, 0);
		GtkWidget* random_password_button = gtk_button_new_with_label("Random");
		gtk_box_pack_start(GTK_BOX(box), random_password_button , FALSE, TRUE, 0);

	// Add the url entry and button
	label = gtk_label_new("Url");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	box = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		url_entry = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(box), url_entry , TRUE, TRUE, 0);
		GtkWidget* launch_button = gtk_button_new_with_label("Launch");
		gtk_box_pack_start(GTK_BOX(box), launch_button , FALSE, TRUE, 0);

	// Add the group entry
	group_entry = gtk_add_labeled_entry(vbox_right, "Group", NULL);

	// Add the information text area
	label = gtk_label_new("Information");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	//~ text = gtk_text_view_new();
  //~ gtk_box_pack_start(GTK_BOX(vbox_right), text , FALSE, TRUE, 1);

	// The info text
	info_text = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(info_text),GTK_WRAP_WORD);
	GtkWidget* scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll),info_text);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox_right), scroll , TRUE, TRUE, 1);

	// Add the created and modified time label
	box = gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		// Add the created time label
		time_created_label = gtk_label_new("Created...");
		gtk_misc_set_alignment (GTK_MISC(time_created_label), 0, 0);
		gtk_box_pack_start(GTK_BOX(box), time_created_label , FALSE, TRUE, 1);
		// Add the modified time label
		time_modified_label = gtk_label_new("Modified...");
		gtk_misc_set_alignment (GTK_MISC(time_modified_label), 0, 0);
		gtk_box_pack_start(GTK_BOX(box), time_modified_label , FALSE, TRUE, 1);
		// Set the texts to be grey
		GdkColor grey_color;
		gdk_color_parse ("grey", &grey_color);
		gtk_widget_modify_fg (time_created_label, GTK_STATE_NORMAL, &grey_color);
		gtk_widget_modify_fg (time_modified_label, GTK_STATE_NORMAL, &grey_color);

	// The record save button
  GtkWidget* record_save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
  gtk_box_pack_start(GTK_BOX(vbox_right), record_save_button , FALSE, TRUE, 1);

	// Status bar...
  GtkWidget* statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

  /* Create a GtkAccelGroup and add it to the window. */
  GtkAccelGroup* accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
  gtk_widget_add_accelerator (open_menu_item, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (save_menu_item, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (exit_menu_item, "activate", accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  //~ gtk_widget_add_accelerator (add_menu_item, "activate", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (copy_username_menu_item, "activate", accel_group, GDK_u, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (copy_password_menu_item, "activate", accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	// Connects...
  g_signal_connect(G_OBJECT (main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT (launch_button), "clicked", G_CALLBACK(click_launch_button), td->treeview);
  g_signal_connect(G_OBJECT (random_password_button), "clicked", G_CALLBACK(click_random_password), main_window);

	// Password entry
	g_signal_connect(G_OBJECT (password_entry), "focus-in-event", G_CALLBACK(focus_in_password), NULL);
	g_signal_connect(G_OBJECT (password_entry), "focus-out-event", G_CALLBACK(focus_out_password), NULL);

	// Any changes made to the treeview
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(td->treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect(selection, "changed", G_CALLBACK(treeview_selection_changed), statusbar);

	// Any changes made to the input field are written into the tree store
	//~ g_signal_connect(G_OBJECT (title_entry), "changed", G_CALLBACK(write_changes_to_td->treestore), selection);
	//~ g_signal_connect(G_OBJECT (username_entry), "changed", G_CALLBACK(write_changes_to_td->treestore), selection);
	//~ g_signal_connect(G_OBJECT (password_entry), "changed", G_CALLBACK(write_changes_to_td->treestore), selection);
	//~ g_signal_connect(G_OBJECT (url_entry), "changed", G_CALLBACK(write_changes_to_td->treestore), selection);
	//~ g_signal_connect(G_OBJECT (info_text), "focus-out-event", G_CALLBACK(write_changes_to_treestore), selection);
	g_signal_connect(G_OBJECT (record_save_button), "clicked", G_CALLBACK(write_changes_to_treestore), selection);

	g_signal_connect(G_OBJECT (filter_entry), "changed", G_CALLBACK(treemodel_filter_change), td->treefilter);
	g_signal_connect(G_OBJECT (filter_entry), "icon-press", G_CALLBACK(clear_filter), NULL);

	// Read the configuration...
	read_configuration(global->config_list);
	update_recent_list(global->config_list);

	// Show the application
  gtk_widget_show_all(main_window);

	// Is a default filename given, then read that file
	if (default_filename)
		load_from_file(default_filename, td->treestore);

	// Warn once
	FILE* once = fopen(".keyvault-warning.once","r");
	if (!once) {
		gtk_warning("This software is very much in development.\nKeep an plain text version around somehwere\nFor example in a truecrypt drive\nThis warning will not be shown again");
		once = fopen(".keyvault-warning.once","w");
		if (once)
			fclose(once);
	}
	else {
		fclose(once);
	}

	// Run the app
  gtk_main();

	// Save the configuration...
	save_configuration(global->config_list);

  return 0;
}
