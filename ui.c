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
//~ #include <glib.h>
//~ #include <gtk/gtklist.h>
//~ #include <mxml.h>
#include <libxml/parser.h>
#include <sys/time.h>
#include <assert.h>

#include "treeview.h"
#include "dialogs.h"
#include "ssh.h"
#include "functions.h"
#include "configuration.h"
#include "encryption.h"
#include "gtk_shortcuts.h"
#include "list.h"
#include "xml.h"

// -----------------------------------------------------------
//
// Global variables
//

struct tGlobal* global;

tKvoFile* active_keyvault;
GtkWidget* popup_menu;

// -----------------------------------------------------------
//
// Global widgets
//

// The main window
GtkWidget* main_window;

// The menus
GtkWidget* open_recent_menu;

// The entries
GtkWidget* title_entry;
GtkWidget* username_entry;
GtkWidget* password_entry;
GtkWidget* url_entry;
GtkWidget* info_text;

// The data tree
struct tTreeData {
	GtkWidget* treeview;
	GtkTreeStore* treestore;
	GtkTreeModel* treefilter;
	char* filter_title;
};
struct tTreeData* treedata;

char* active_passphrase = NULL;
char* active_filename = NULL;

// -----------------------------------------------------------
//
// load_from_file - load a file into the treestore
//

static int load_from_file(const gchar* filename, GtkTreeStore* treestore) {
	// Read the file
	xmlDoc* doc_enc = xmlParseFile(filename);
	//~ xmlDocDump(stdout, doc_enc);puts("");

	// Request the passphrase to decode the file ("secret")
	//~ gchar* passphrase = "secret";
	

	// xml => encrypted-xml
	xmlDoc* doc = NULL;
	while(1) {
		if (!active_passphrase) {
			active_passphrase = dialog_request_password(NULL,"menu_test_passphrase");
			if (!active_passphrase)
				return 0;
		}
		doc = xml_doc_decrypt(doc_enc, active_passphrase);
		if (!doc) {
			free(active_passphrase);
			active_passphrase = NULL;
			continue;
		}
		break;
	}
	//~ xmlDocDump(stdout, doc);puts("");

	// treestore => xml
	import_xml_into_treestore(treestore, doc);
	//~ xmlDoc* doc = export_reestore_to_xml(treestore);
	//~ xmlDocDump(stdout, doc);puts("");
	return 1;
}

// -----------------------------------------------------------
//
// MENU: file -> open
//

static void menu_file_open(GtkWidget *widget, gpointer ptr)
{
	gchar* filename=dialog_open_file(widget, main_window);
	if (filename) {
		GtkTreeStore* treestore = ptr;
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

void open_kvo_file_real(tKvoFile* kvo) {
	g_printf("open_kvo_file_real()\n");
	g_printf("hostname: %s\n",kvo->hostname);
	g_printf("username: %s\n",kvo->username);
	g_printf("filename: %s\n",kvo->filename);
	if (!kvo->protocol || (strcmp(kvo->protocol,"local") == 0)) {
		trace();
	}
	else if (strcmp(kvo->protocol,"ssh") == 0) {
		ssh_get_file(kvo);
	}
}

void menu_file_open_advanced(GtkWidget *widget, gpointer parent_window)
{
	// Create a new kvo file
	tKvoFile* kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	// Let the use fill in all required fields...
	if (dialog_request_kvo(kvo)) {
		// Store the kvo to the list
		list_add(global->kvo_list, kvo);
		update_recent_list(global->kvo_list);
		// Open the file...
		open_kvo_file_real(kvo);
	}
}

// -----------------------------------------------------------
//
// save_to_file - save the treestore to a file
//

static void save_to_file(const gchar* filename, GtkTreeStore* treestore) {

	// treestore => xml
	xmlDoc* doc = export_treestore_to_xml(treestore);
	//~ xmlDocDump(stdout, doc);puts("");

	// Request the passphrase to encode the file ("secret")
	if (!active_passphrase)
		active_passphrase = dialog_request_password(NULL,"menu_test_passphrase");

	// xml => encrypted-xml
	xmlDoc* enc_doc = xml_doc_encrypt(doc, active_passphrase);
	//~ xmlDocDump(stdout, enc_doc);puts("");

	// encrypted-xml => disk
	FILE* fp = fopen(filename, "w");
	if (fp) {
		xmlDocDump(fp, enc_doc);
		fclose(fp);
	}
}

// -----------------------------------------------------------
//
// MENU: file -> save as
//

static void menu_file_save_as(GtkWidget *widget, gpointer ptr)
{
	gchar* filename = dialog_save_file(widget, main_window);
	if (filename) {
		GtkTreeStore* treestore = ptr;
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
// Launch the URL
//

void click_launch_button(GtkWidget *widget, gpointer ptr) {
	const gchar* url=gtk_entry_get_text(GTK_ENTRY(url_entry));
	printf("Launch: %s\n",url);
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

static void clear_filter(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data) {
	gtk_entry_set_text(entry,"");
}

// -----------------------------------------------------------
//
// Create a default configuration
//

static void create_demo_data(GtkWidget *widget, gpointer ptr) {
	GtkTreeStore* treestore = ptr;
	static const char* demo_names[10]={"Tweakers.net","Google.nl","Mijn postbank","Rabobank","Woopie","Amazon","Marktplaats","Hyves","Facebook","IBM services"};
	static const char* demo_users[10]={"john","james","jack","judy","boris","bas","bruno","berend","boudewijn","jmkok"};
	int i;
	for (i=0;i<10;i++) {
		GtkTreeIter iter;
		// Insert a new record with a random password
		gchar* password = create_random_password(12);
		treestore_add_record(treestore, &iter, NULL, demo_names[i], demo_users[i], password, "http://someserver.com/path/inde.html", "Some info");
		g_free(password);
	}
}

// -----------------------------------------------------------
//
// Create a default configuration
//

static void create_demo_config(GtkWidget *widget, gpointer kvo_list_ptr) {
	tKvoFile* kvo;
	tList* kvo_list=kvo_list_ptr;

	// Remove all items from the kvo_list
	void remove_items(tList* kvo_list,void* data) {
		list_remove(kvo_list, data);
	}
	list_foreach(kvo_list,remove_items);

	// Add jmkok.kvo
	kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	kvo->title = strdup("jmkok.kvo");
	kvo->filename = strdup("jmkok.kvo");
	list_add(kvo_list,kvo);

	// Add ssh @ youcom.nl
	kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	kvo->title = strdup("ssh://jmkok@youcom.nl");
	kvo->protocol = strdup("ssh");
	kvo->hostname = strdup("green.youcom.nl");
	kvo->username = strdup("jmkok");
	kvo->filename = strdup("test.kvo");
	kvo->local_filename = strdup("local.kvo");
	list_add(kvo_list,kvo);

	// Add ssh @ pooh
	kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	kvo->title = strdup("ssh://jmkok@p23.nl");
	kvo->protocol = strdup("ssh");
	kvo->hostname = strdup("pooh.p23.nl");
	kvo->username = strdup("jmkok");
	kvo->filename = strdup("sjaak.kvo");
	kvo->local_filename = strdup("local.kvo");
	list_add(kvo_list,kvo);

	// Update the open->recent
	update_recent_list(kvo_list);
}

// -----------------------------------------------------------
//
// Any changes made to the edit fields should be written into the tree store
//

void write_changes_to_treestore(GtkWidget *widget, gpointer selection) {
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
			COL_INFO, info,
			-1);
	}
}

// -----------------------------------------------------------
//
// Load "save.kvo"
//

static void click_test_load(GtkWidget *widget, gpointer ptr) {
	GtkTreeStore* treestore = ptr;
	load_from_file("save.kvo", treestore);
}

// -----------------------------------------------------------
//
// Save "save.kvo"
//

static void click_test_save(GtkWidget *widget, gpointer ptr) {
	GtkTreeStore* treestore = ptr;
	save_to_file("save.kvo", treestore);
}

// -----------------------------------------------------------
//
// Create a random password
//

static void click_random_password(GtkWidget *widget, gpointer ptr) {
	gchar* password = create_random_password(12);
	gtk_entry_set_text(GTK_ENTRY(password_entry),password);
	g_free(password);
}

// -----------------------------------------------------------
//
// Add a new item
//

static void click_add_item(GtkWidget *widget, gpointer treestore_ptr) {
	GtkTreeIter iter;
	// Insert a new record with a random password
	gchar* password = create_random_password(12);
	gtk_tree_store_append(treedata->treestore, &iter, NULL);
	gtk_tree_store_set(treedata->treestore, &iter, COL_TITLE, "NEW", COL_USERNAME, "", COL_PASSWORD, password, COL_URL, "", COL_INFO, "", -1);
	g_free(password);
	// Set focus...
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treedata->treeview));
	gtk_tree_selection_select_iter(selection, &iter);
}

static void click_copy_username(GtkWidget *widget, gpointer ptr) {
	printf("click_copy_username\n");
}

static void click_copy_password(GtkWidget *widget, gpointer ptr) {
	printf("click_copy_password\n");
}

// -----------------------------------------------------------
//
// Show and hide the contents of the password field
//

static void show_password(GtkWidget *widget, gpointer ptr) {
	//~ GtkWidget*
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), TRUE);
}

static void hide_password(GtkWidget *widget, gpointer ptr) {
	//~ GtkWidget*
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
}

// -----------------------------------------------------------
//
// Popup menu
// http://library.gnome.org/devel/gtk/stable/gtk-migrating-checklist.html#checklist-popup-menu
//

static void do_popup_menu (GtkWidget *my_widget, GdkEventButton *event) {
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

void menu_test_passphrase(GtkWidget *widget, gpointer tree_view) {
	gchar* passphrase = dialog_request_password(NULL,"menu_test_passphrase");
	g_printf("passphrase: %s\n",passphrase);
	g_free(passphrase);
}

// -----------------------------------------------------------
//
// Open (and edit) a recently used KVO file
//

void open_recent_kvo_file(GtkWidget *widget, gpointer kvo_pointer) {
	tKvoFile* kvo=kvo_pointer;
	if (dialog_request_kvo(kvo)) {
		open_kvo_file_real(kvo);
	}
}

// -----------------------------------------------------------
//
// Update the list of recent files
//

void update_recent_list(tList* kvo_list) {
	// First remove all items in the recent menu
	void cb_remove_menu_item(GtkWidget* menu_item, gpointer unused) {
		gtk_remove_menu_item(open_recent_menu, menu_item);
	}
	gtk_container_foreach(GTK_CONTAINER(open_recent_menu), cb_remove_menu_item, 0);

	// Then add all items in the kvo_list to the recent menu
	void add_to_menu(tList* kvo_list, void* data) {
		tKvoFile* kvo = data;
		GtkWidget* account_menu_item = gtk_add_menu_item(open_recent_menu, kvo->title);
		g_signal_connect(G_OBJECT (account_menu_item), "activate", G_CALLBACK(open_recent_kvo_file), kvo);
	}
	list_foreach(kvo_list,add_to_menu);

	// Show the recent menu
	gtk_widget_show_all(open_recent_menu);
}

// -----------------------------------------------------------
//
// The selection has changed
//

static void treeview_selection_changed(GtkWidget *widget, gpointer statusbar)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char* title;
	char* name;
  char* password;
	char* url;
  char* info;

  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
    gtk_tree_model_get(model, &iter, COL_TITLE, &title, COL_USERNAME, &name, COL_PASSWORD, &password, COL_URL, &url, COL_INFO, &info,  -1);
    gtk_statusbar_push(GTK_STATUSBAR(statusbar),gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),title), title);
		gtk_entry_set_text(GTK_ENTRY(title_entry),title);
		gtk_entry_set_text(GTK_ENTRY(username_entry),name);
		gtk_entry_set_text(GTK_ENTRY(password_entry),password);
		gtk_entry_set_text(GTK_ENTRY(url_entry),url);
		GtkTextBuffer* buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text));
		gtk_text_buffer_set_text(buffer,info,-1);
    g_free(title);
    g_free(name);
    g_free(password);
    g_free(url);
    g_free(info);
  }
}

// -----------------------------------------------------------
//
// treemodel_visible_func()
// Filter function
// It filters the title column, but potentially could filter other columns
//

static gboolean treemodel_visible_func (GtkTreeModel *model, GtkTreeIter  *iter, gpointer data)
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

// Sort function
static gint treestore_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data){
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

// Reverse the sort order
int sort_order=0;
static void treestore_click_col1(GtkWidget *widget, gpointer ptr) {
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
	struct tTreeData* td = malloc(sizeof(struct tTreeData));
	memset(td, 0, sizeof(struct tTreeData));
	treedata = td;

	// Malloc the filter
	td->filter_title=malloc(1024);
	*td->filter_title=0;

	// Create the treestore (8 strings fields)
	td->treestore = gtk_tree_store_new(NUM_COLS,
		G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,
		G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING
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
  g_signal_connect(G_OBJECT (col1), "clicked", G_CALLBACK(treestore_click_col1), td->treestore);
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

	// GTK_STOCK_CANCEL
	// GTK_STOCK_DELETE
	// GTK_STOCK_CLEAR
	// GTK_STOCK_DIALOG_AUTHENTICATION

int create_main_window(void) {
	// Initialize random generator
	struct timeval tv;
	gettimeofday(&tv,0);
	srand(tv.tv_sec ^ tv.tv_usec);

	// Initialize the generic global component
	global=malloc(sizeof(struct tGlobal));
	memset(global,0,sizeof(struct tGlobal));
	global->kvo_list=list_create();


	GtkWidget* label;

	// Initialize gtk
  gtk_window_set_default_icon_name(GTK_STOCK_DIALOG_AUTHENTICATION);

	// Create the primary window
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(main_window), "Keyvault.org");
	//~ gtk_window_set_icon_from_file(GTK_WINDOW(main_window), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(main_window), GTK_STOCK_DIALOG_AUTHENTICATION);
  gtk_widget_set_size_request (main_window, 500, 600);

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
	GtkWidget* view_menu = gtk_add_menu(menu_bar,"View");
	GtkWidget* help_menu = gtk_add_menu(menu_bar,"Help");

	// File menu
	GtkWidget* open_menu_item = gtk_add_menu_item_clickable(file_menu, "Open file", G_CALLBACK(menu_file_open), td->treestore);
	gtk_add_menu_item_clickable(file_menu, "Open special", G_CALLBACK(menu_file_open_advanced), td->treestore);
	open_recent_menu = gtk_add_menu(file_menu, "Open recent");
	GtkWidget* save_menu_item = gtk_add_menu_item_clickable(file_menu, "Save", G_CALLBACK(menu_file_save), td->treestore);
	gtk_add_menu_item_clickable(file_menu, "Save as...", G_CALLBACK(menu_file_save_as), td->treestore);
	gtk_add_separator(file_menu);
	//~ GtkWidget* read_configuration_menu_item = gtk_add_menu_item("Read configuration",file_menu);
	//~ GtkWidget* save_configuration_menu_item = gtk_add_menu_item("Save configuration",file_menu);
	//~ gtk_add_separator(file_menu);
	GtkWidget* exit_menu_item = gtk_add_menu_item_clickable(file_menu, "Exit", G_CALLBACK(gtk_main_quit), NULL);

	// Edit menu
	gtk_add_menu_item_clickable(edit_menu, "Add item", G_CALLBACK(click_add_item), NULL);
	GtkWidget* copy_username_menu_item = gtk_add_menu_item_clickable(edit_menu, "Copy username", G_CALLBACK(click_copy_username), NULL);
	GtkWidget* copy_password_menu_item = gtk_add_menu_item_clickable(edit_menu, "Copy passphrase", G_CALLBACK(click_copy_password), NULL);

	// Test menu
	gtk_add_menu_item(test_menu, "SSH");
	gtk_add_menu_item_clickable(test_menu, "passphrase", G_CALLBACK(menu_test_passphrase), main_window);
	gtk_add_menu_item_clickable(test_menu, "demo data", G_CALLBACK(create_demo_data), td->treestore);
	gtk_add_menu_item_clickable(test_menu, "demo config", G_CALLBACK(create_demo_config), global->kvo_list);
	gtk_add_menu_item_clickable(test_menu, "load", G_CALLBACK(click_test_load), td->treestore);
	gtk_add_menu_item_clickable(test_menu, "save", G_CALLBACK(click_test_save), td->treestore);

	// Help menu
	gtk_add_menu_item_clickable(help_menu, "About", G_CALLBACK(about_widget), main_window);

	// Prevent warnings...
	if (view_menu);

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

	// The treeview
  gtk_box_pack_start(GTK_BOX(vbox_left), td->treeview , TRUE, TRUE, 1);

	// POPUP MENU
  g_signal_connect(G_OBJECT (td->treeview), "button-press-event", G_CALLBACK(my_widget_button_press_event_handler), NULL);
  g_signal_connect(G_OBJECT (td->treeview), "popup-menu", G_CALLBACK(my_widget_popup_menu_handler), NULL);

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
	g_signal_connect(G_OBJECT (password_entry), "focus-in-event", G_CALLBACK(show_password), NULL);
	g_signal_connect(G_OBJECT (password_entry), "focus-out-event", G_CALLBACK(hide_password), NULL);

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

  //~ g_signal_connect(G_OBJECT (save_configuration_menu_item), "activate", G_CALLBACK(save_configuration), window);
  //~ g_signal_connect(G_OBJECT (read_configuration_menu_item), "activate", G_CALLBACK(read_configuration), window);

	// Read the configuration...
	read_configuration(global->kvo_list);
	update_recent_list(global->kvo_list);

	// Run the app
  gtk_widget_show_all(main_window);
  gtk_main();

	// Save the configuration...
	save_configuration(global->kvo_list);

  return 0;
}
