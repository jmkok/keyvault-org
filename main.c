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
#include <mxml.h>
#include <sys/time.h>

#include "treeview.h"
#include "dialogs.h"
#include "ssh.h"
#include "functions.h"
#include "xml.h"
#include "configuration.h"
#include "encryption.h"
#include "gtk_shortcuts.h"

// -----------------------------------------------------------
//
// Global variables
//

struct tGUI* gui;
struct tGlobal* global;

tKvoFile* active_keyvault;

extern void quick_message (GtkWidget *parent,gchar *message);

// -----------------------------------------------------------
//
// MENU: file -> open
//

void menu_file_open(GtkWidget *widget, gpointer parent_window)
{
	gchar* filename=dialog_open_file(widget,parent_window);
	if (filename) {
		// Read file...
		FILE* stream=fopen(filename,"rb");
		fseek(stream,0,SEEK_END);
		size_t filesize=ftell(stream);
		g_printf("File size: %zu\n",filesize);
		//~ fseek(stream,16,SEEK_SET);
		void* buffer=g_malloc(filesize);
		fread(buffer,1,filesize,stream);
		fclose(stream);
		// read aes file
		//~ AES_KEY* key=g_malloc(sizeof(AES_KEY));
		//~ AES_set_decrypt_key((unsigned char*)"test",sizeof(AES_KEY),key);
		//~ hexdump(key,32);g_print("\n");
		//~ hexdump(buffer,32);g_print("\n");
		//~ AES_decrypt(buffer,buffer,key);
		//~ hexdump(buffer,32);g_print("\n");
		//~ aes_decrypt(buffer,filesize,"secret");
		//~ printf("%s",buffer);
		// free
		g_free(buffer);
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
		global->kvo_list = g_list_append(global->kvo_list, kvo);
		update_recent_list(global->kvo_list);
		// Open the file...
		open_kvo_file_real(kvo);
	}
}

// -----------------------------------------------------------
//
// MENU: file -> save
//

void menu_file_save(GtkWidget *widget, gpointer parent_window)
{
	gchar* filename=dialog_save_file(widget,parent_window);
	if (filename) {
		//~ save_to_file (filename);
		g_free (filename);
	}
}

// -----------------------------------------------------------
//
// Launch the URL
//

void click_launch_button(GtkWidget *widget, gpointer ptr) {
	const gchar* url=gtk_entry_get_text(GTK_ENTRY(gui->url_entry));
	printf("Launch: %s\n",url);
}

// -----------------------------------------------------------
//
// Setup the treestore with a default file
//

void load_default_xml() {
	// Read file...
	FILE* stream=fopen("default.xml","rb");
	fseek(stream,0,SEEK_END);
	size_t filesize=ftell(stream);
	printf("File size: %zu\n",filesize);
	char* buffer=g_malloc(filesize+1);

	// Read the file
	fseek(stream,0,SEEK_SET);
	size_t rx=fread(buffer,1,filesize,stream);
	buffer[rx]=0;
	printf("rx: %zu\n",rx);
	fclose(stream);

	import_xml_into_treestore(gui->treestore,buffer);

	g_free(buffer);

}

// -----------------------------------------------------------
//
// A user has made a change to the search field
//

char* filter_title;
void perform_filter(GtkWidget *widget, gpointer ptr) {
	const char* text=gtk_entry_get_text(GTK_ENTRY(widget));
	strcpy(filter_title,text);
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gui->treemodel));
}
void clear_filter(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data) {
	gtk_entry_set_text(entry,"");
}

// -----------------------------------------------------------
//
// Any changes made to the edit fields should be written into the tree store
//

void do_default_config(GtkWidget *widget, gpointer kvo_list_ptr) {
	tKvoFile* kvo;
	GList* kvo_list=kvo_list_ptr;

	// Remove all items from thge kvo_list
	void remove_items(gpointer data, gpointer user_data) {
		kvo_list=g_list_remove(kvo_list,data);
	}
	g_list_foreach(kvo_list,remove_items,0);

	// Add jmkok.kvo
	kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	kvo->title = strdup("jmkok.kvo");
	kvo->filename = strdup("jmkok.kvo");
	kvo_list = g_list_append(kvo_list,kvo);

	// Add ssh @ youcom.nl
	kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	kvo->title = strdup("ssh://jmkok@youcom.nl");
	kvo->protocol = strdup("ssh");
	kvo->hostname = strdup("green.youcom.nl");
	kvo->username = strdup("jmkok");
	kvo->filename = strdup("test.kvo");
	kvo->local_filename = strdup("local.kvo");
	kvo_list = g_list_append(kvo_list,kvo);

	// Add ssh @ pooh
	kvo=malloc(sizeof(tKvoFile));
	memset(kvo,0,sizeof(tKvoFile));
	kvo->title = strdup("ssh://jmkok@p23.nl");
	kvo->protocol = strdup("ssh");
	kvo->hostname = strdup("pooh.p23.nl");
	kvo->username = strdup("jmkok");
	kvo->filename = strdup("sjaak.kvo");
	kvo->local_filename = strdup("local.kvo");
	kvo_list = g_list_append(kvo_list,kvo);

	update_recent_list(kvo_list);

	// Load default kvo
	load_default_xml();
}

// -----------------------------------------------------------
//
// Any changes made to the edit fields should be written into the tree store
//

void write_changes_to_treestore(GtkWidget *widget, gpointer selection_ptr) {
  GtkTreeIter iter;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection_ptr), &model, &iter)) {
		GtkTextBuffer* text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->info_text));
		GtkTextIter start,end;
		gtk_text_buffer_get_bounds(text_buffer,&start,&end);
		const gchar* info=gtk_text_buffer_get_text(text_buffer,&start,&end,0);

		//~ gtk_tree_store_set(gui->treestore, &iter, COL_TITLE, title, COL_USERNAME, username, COL_PASSWORD, password, COL_URL, url, COL_INFO, info, -1);
		//~ gtk_tree_store_set(gui->treestore, &iter, COL_TITLE, title, -1);
		//~ GtkTreeModel *modelX=gtk_tree_model_filter_get_model((GtkTreeModelFilter*)model);
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 
			COL_TITLE, gtk_entry_get_text(GTK_ENTRY(gui->title_entry)), 
			COL_USERNAME, gtk_entry_get_text(GTK_ENTRY(gui->username_entry)), 
			COL_PASSWORD, gtk_entry_get_text(GTK_ENTRY(gui->password_entry)),
			COL_URL, gtk_entry_get_text(GTK_ENTRY(gui->url_entry)),
			COL_INFO, info,
			-1);
	}
}

// -----------------------------------------------------------
//
// Load an AES file into the treestore
//

void load_treestore(GtkTreeStore* treestore, tKvoFile* keyvault) {
	// Read KVO file
	FILE* fh=fopen(keyvault->local_filename,"rt");
	if (!fh) return;
	mxml_node_t* xml = mxmlLoadFile(NULL, fh, MXML_NO_CALLBACK);
	fclose(fh);

	// Read the ivec
	mxml_node_t* ivec_node = mxmlFindElement(xml,xml,"ivec",NULL,NULL,MXML_DESCEND);
	ivec_node = mxmlWalkNext(ivec_node,ivec_node,MXML_DESCEND);
	if (!ivec_node) {
		printf("ERROR: missing ivec node\n");
		goto error;
	}
	gsize ivec_len;
	keyvault->ivec = g_base64_decode(ivec_node->value.text.string, &ivec_len);
	hexdump("ivec",keyvault->ivec,(int)ivec_len);
	if (!keyvault->ivec){
		printf("ERROR: Could not decode ivec\n");
		goto error;
	}
	if (ivec_len != 16) {
		printf("ERROR: incorrect ivec length\n");
		goto error;
	}

	// Read the size (not needed)
	mxml_node_t* size_node = mxmlFindElement(xml,xml,"size",NULL,NULL,MXML_DESCEND);
	size_node = mxmlWalkNext(size_node,size_node,MXML_DESCEND);
	if (!size_node) {
		printf("ERROR: missing size node\n");
		goto error;
	}
	keyvault->data_len=atol(size_node->value.text.string);
	printf("data_size: %u\n",keyvault->data_len);

	// Read the data
	gsize buffer_len;
	mxml_node_t* data_node = mxmlFindElement(xml,xml,"data",NULL,NULL,MXML_DESCEND);
	data_node = mxmlWalkNext(data_node,data_node,MXML_DESCEND);
	if (!data_node) {
		printf("ERROR: missing data node\n");
		goto error;
	}
	keyvault->data = g_base64_decode(data_node->value.text.string, &buffer_len);
	if (!keyvault->data){
		printf("ERROR: Could not decode buffer\n");
		goto error;
	}
	if (keyvault->data_len != buffer_len) {
		printf("ERROR: Buffersize is incorrect\n");
		printf("read: %zu\n",buffer_len);
		goto error;
	}

	// Request the passphrase to decode the file
	if (!keyvault->passphrase) {
		keyvault->passphrase = dialog_request_password(NULL,"menu_test_passphrase");
	}

	// decode the data
	aes_ofb(keyvault->data, buffer_len, keyvault->passphrase, keyvault->ivec);

	// shuffle the ivec (1 of 2)
	shuffle_ivec(keyvault->ivec);

	// Store the xml data into the treestore
	//~ clear_treestore(treestore);	- performed by import
	import_xml_into_treestore(treestore, keyvault->data);
	
	active_keyvault=keyvault;

error:
	free(keyvault->data);
	keyvault->data=NULL;
}

void click_load(GtkWidget *widget, gpointer ptr) {
	tKvoFile* keyvault=malloc(sizeof(tKvoFile));
	memset(keyvault,0,sizeof(tKvoFile));
	keyvault->local_filename = strdup("keyvault.kvo");
	keyvault->passphrase = strdup("secret");
	load_treestore(gui->treestore,keyvault);
}

// -----------------------------------------------------------
//
// Save a treestore into an an AES file
//

void save_treestore(GtkTreeStore* treestore, tKvoFile* keyvault) {
	char* xml_filename=concat(keyvault->local_filename,".xml");
	char* aes_filename=concat(keyvault->local_filename,".aes");

	keyvault->data = export_treestore_to_xml(treestore);
	keyvault->data_len = strlen(keyvault->data)+1;

	// Print the structure
	//~ printf("%s\n",xml_text);
	printf("keyvault->data_len: %u\n",keyvault->data_len);

	// Save the plain (unprotected) file
	if (1) {
		GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gui->window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "WARNING: saving unprotected");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		FILE* stream=fopen(xml_filename,"wb");
		fwrite(keyvault->data, 1, strlen(keyvault->data), stream);
		fclose(stream);
	}

	// Shuffle the ivec (2 of 2)
	shuffle_ivec(keyvault->ivec);

	// Encrypt the data
	aes_ofb(keyvault->data, keyvault->data_len, keyvault->passphrase, keyvault->ivec);

	// Save the raw encrypted file (not dangerous)
	if (1) {
		FILE* stream=fopen(aes_filename,"wb");
		fwrite(keyvault->data, 1, keyvault->data_len, stream);
		fclose(stream);
	}

	// Save the encrypted data as xml structure
	gchar* tmp;
	mxml_node_t* node;
	mxml_node_t* xml = mxmlNewXML("1.0");
	mxml_node_t* xml_keyvault = mxmlNewElement(xml,"keyvault");
	node = mxmlNewElement(xml_keyvault, "encryptiom");
		mxmlNewText(node, 0, "AES_OFB");
	node = mxmlNewElement(xml_keyvault, "ivec");
		tmp=g_base64_encode(keyvault->ivec,16);
		mxmlNewText(node, 0, tmp);
		g_free(tmp);
	node = mxmlNewElement(xml_keyvault, "size");
		mxmlNewInteger(node, keyvault->data_len);
	node = mxmlNewElement(xml_keyvault, "data");
		tmp=g_base64_encode((unsigned char*)keyvault->data, keyvault->data_len);
		mxmlNewText(node, 0, tmp);
		g_free(tmp);
	FILE* fh=fopen(keyvault->local_filename,"wt");
	if (fh) {
		mxmlSaveFile(xml,fh,whitespace_cb);
		fclose(fh);
	}

	free(keyvault->data);
	keyvault->data=NULL;

	free(xml_filename);
	free(aes_filename);
}

void click_save(GtkWidget *widget, gpointer ptr) {
	if (active_keyvault) {
		save_treestore(gui->treestore, active_keyvault);
	}
}

// -----------------------------------------------------------
//
// Create a random password
//

void click_random_password(GtkWidget *widget, gpointer ptr) {
	char random_charset[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	gchar* password=g_malloc(16);
	memset(password,0,16);
	FILE* fp=fopen("/dev/urandom","r");
	int i,r;
	for (i=0;i<12;i++) {
		if (fp)
			fread(&r,1,sizeof(r),fp);
		else
			r=rand();
		char p=random_charset[r%strlen(random_charset)];
		password[i]=p;
	}
	if (fp)
		fclose(fp);
	gtk_entry_set_text(GTK_ENTRY(gui->password_entry),password);
	g_free(password);
}

void show_password(GtkWidget *widget, gpointer entry_ptr) {
	//~ GtkWidget*
	gtk_entry_set_visibility(GTK_ENTRY(gui->password_entry), TRUE);
}

void hide_password(GtkWidget *widget, gpointer entry_ptr) {
	//~ GtkWidget*
	gtk_entry_set_visibility(GTK_ENTRY(gui->password_entry), FALSE);
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

void update_recent_list(GList* kvo_list) {
	// First remove all items in the recent menu
	void cb_remove_menu_item(GtkWidget* menu_item, gpointer unused) {
		gtk_remove_menu_item(menu_item, gui->open_recent_menu);
	}
	gtk_container_foreach(GTK_CONTAINER(gui->open_recent_menu), cb_remove_menu_item, 0);

	// Then add all items in the kvo_list to the recent menu
	void add_to_menu(gpointer data, gpointer user_data) {
		tKvoFile* kvo = data;
		GtkWidget* account_menu_item = gtk_add_menu_item(kvo->title,gui->open_recent_menu);
		g_signal_connect(G_OBJECT (account_menu_item), "activate", G_CALLBACK(open_recent_kvo_file), kvo);
	}
	g_list_foreach(kvo_list,add_to_menu,0);

	// Show the recent menu
	gtk_widget_show_all(gui->open_recent_menu);
}

// -----------------------------------------------------------
//
// main()
//

	// GTK_STOCK_CANCEL
	// GTK_STOCK_DELETE
	// GTK_STOCK_CLEAR
	// GTK_STOCK_DIALOG_AUTHENTICATION

int main(int argc, char** argv)
{
	filter_title=malloc(1024);
	*filter_title=0;

	// Initialize random generator
	struct timeval tv;
	gettimeofday(&tv,0);
	srand(tv.tv_sec ^ tv.tv_usec);

	// Initialize the generic global component
	global=malloc(sizeof(struct tGlobal));
	memset(global,0,sizeof(struct tGlobal));

	// Initialize the global gui component
	gui=malloc(sizeof(struct tGUI));
	memset(gui,0,sizeof(struct tGUI));

	GtkWidget* label;

	// Initialize gtk
  gtk_init(&argc, &argv);
  gtk_window_set_default_icon_name(GTK_STOCK_DIALOG_AUTHENTICATION);

	// Create the primary window
  gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(gui->window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(gui->window), "Keyvault.org");
	//~ gtk_window_set_icon_from_file(GTK_WINDOW(gui->window), "/usr/share/pixmaps/apple-red.png", NULL);
	gtk_window_set_icon_name(GTK_WINDOW(gui->window), GTK_STOCK_DIALOG_AUTHENTICATION);
  gtk_widget_set_size_request (gui->window, 500, 600);

	// Make the vbox and put it in the main window
  GtkWidget* vbox = gtk_vbox_new(FALSE, 3);
  gtk_container_add(GTK_CONTAINER(gui->window), vbox);

	// Menu bar
	GtkWidget* menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 1);
	GtkWidget* file_menu = gtk_add_menu("File",menu_bar);
	GtkWidget* edit_menu = gtk_add_menu("Edit",menu_bar);
	GtkWidget* test_menu = gtk_add_menu("Test",menu_bar);
	GtkWidget* view_menu = gtk_add_menu("View",menu_bar);
	GtkWidget* help_menu = gtk_add_menu("Help",menu_bar);

	// File menu
	GtkWidget* open_menu_item = gtk_add_menu_item("Open file",file_menu);
	GtkWidget* open_advanced_menu_item = gtk_add_menu_item("Open special",file_menu);
	gui->open_recent_menu = gtk_add_menu("Open recent",file_menu);
	GtkWidget* save_menu_item = gtk_add_menu_item("Save",file_menu);
	gtk_add_separator(file_menu);
	//~ GtkWidget* read_configuration_menu_item = gtk_add_menu_item("Read configuration",file_menu);
	//~ GtkWidget* save_configuration_menu_item = gtk_add_menu_item("Save configuration",file_menu);
	//~ gtk_add_separator(file_menu);
	GtkWidget* exit_menu_item = gtk_add_menu_item("Exit",file_menu);

	// Test menu
	GtkWidget* ssh_menu_item = gtk_add_menu_item("SSH",test_menu);
	GtkWidget* passphrase_menu_item = gtk_add_menu_item("passphrase",test_menu);
	GtkWidget* default_config_menu_item = gtk_add_menu_item("default config",test_menu);
	GtkWidget* test_load_menu_item = gtk_add_menu_item("load",test_menu);
	GtkWidget* test_save_menu_item = gtk_add_menu_item("save",test_menu);

	// Help menu
	GtkWidget* about_menu_item = gtk_add_menu_item("About",help_menu);

	// Prevent warnings...
	if (edit_menu && view_menu && ssh_menu_item);

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
  gui->tree_view = create_view_and_model();
  gtk_box_pack_start(GTK_BOX(vbox_left), gui->tree_view , TRUE, TRUE, 1);

	// The right side (record info)
	GtkWidget* vbox_right = gtk_vbox_new(FALSE, 2);
	gtk_paned_add2(GTK_PANED(hbox), vbox_right);

	gui->title_entry = gtk_add_labeled_entry("Title",NULL,vbox_right);
	gui->username_entry = gtk_add_labeled_entry("Username",NULL,vbox_right);

	label = gtk_label_new("Password");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	GtkWidget* box = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		gui->password_entry = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(gui->password_entry), FALSE);
		gtk_box_pack_start(GTK_BOX(box), gui->password_entry , TRUE, TRUE, 0);
		GtkWidget* random_password_button = gtk_button_new_with_label("Random");
		gtk_box_pack_start(GTK_BOX(box), random_password_button , FALSE, TRUE, 0);

	label = gtk_label_new("Url");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	box = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox_right), box, FALSE, TRUE, 1);
		gui->url_entry = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(box), gui->url_entry , TRUE, TRUE, 0);
		GtkWidget* launch_button = gtk_button_new_with_label("Launch");
		gtk_box_pack_start(GTK_BOX(box), launch_button , FALSE, TRUE, 0);

	label = gtk_label_new("Information");
	gtk_misc_set_alignment (GTK_MISC(label), 0, 0);
  gtk_box_pack_start(GTK_BOX(vbox_right), label , FALSE, TRUE, 1);
	//~ text = gtk_text_view_new();
  //~ gtk_box_pack_start(GTK_BOX(vbox_right), text , FALSE, TRUE, 1);

	// The info text
	gui->info_text = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(gui->info_text),GTK_WRAP_WORD);
	GtkWidget* scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll),gui->info_text);
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
  gtk_window_add_accel_group (GTK_WINDOW (gui->window), accel_group);
  gtk_widget_add_accelerator (open_menu_item, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (save_menu_item, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (exit_menu_item, "activate", accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	// Connects...
  g_signal_connect(G_OBJECT (gui->window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT (launch_button), "clicked", G_CALLBACK(click_launch_button), gui->tree_view);
  g_signal_connect(G_OBJECT (random_password_button), "clicked", G_CALLBACK(click_random_password), gui->window);
  g_signal_connect(G_OBJECT (open_menu_item), "activate", G_CALLBACK(menu_file_open), gui->window);
  g_signal_connect(G_OBJECT (open_advanced_menu_item), "activate", G_CALLBACK(menu_file_open_advanced), gui->window);
  g_signal_connect(G_OBJECT (save_menu_item), "activate", G_CALLBACK(menu_file_save), gui->window);
	g_signal_connect(G_OBJECT (about_menu_item), "activate", G_CALLBACK(about_widget), gui->window);
	g_signal_connect(G_OBJECT (exit_menu_item), "activate", G_CALLBACK(gtk_main_quit), NULL);

	// Test menu...
  g_signal_connect(G_OBJECT (passphrase_menu_item), "activate", G_CALLBACK(menu_test_passphrase), gui->window);
  g_signal_connect(G_OBJECT (default_config_menu_item), "activate", G_CALLBACK(do_default_config), global->kvo_list);
  g_signal_connect(G_OBJECT (test_load_menu_item), "activate", G_CALLBACK(click_load), NULL);
  g_signal_connect(G_OBJECT (test_save_menu_item), "activate", G_CALLBACK(click_save), NULL);

	g_signal_connect(G_OBJECT (gui->password_entry), "focus-in-event", G_CALLBACK(show_password), NULL);
	g_signal_connect(G_OBJECT (gui->password_entry), "focus-out-event", G_CALLBACK(hide_password), NULL);

	// Any changes made to the treeview
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gui->tree_view));
  g_signal_connect(selection, "changed", G_CALLBACK(on_changed), statusbar);

	// Any changes made to the input field are written into the tree store
	//~ g_signal_connect(G_OBJECT (gui->title_entry), "changed", G_CALLBACK(write_changes_to_treestore), selection);
	//~ g_signal_connect(G_OBJECT (gui->username_entry), "changed", G_CALLBACK(write_changes_to_treestore), selection);
	//~ g_signal_connect(G_OBJECT (gui->password_entry), "changed", G_CALLBACK(write_changes_to_treestore), selection);
	//~ g_signal_connect(G_OBJECT (gui->url_entry), "changed", G_CALLBACK(write_changes_to_treestore), selection);
	//~ g_signal_connect(G_OBJECT (gui->info_text), "focus-out-event", G_CALLBACK(write_changes_to_treestore), selection);
	g_signal_connect(G_OBJECT (record_save_button), "clicked", G_CALLBACK(write_changes_to_treestore), selection);

	g_signal_connect(G_OBJECT (filter_entry), "changed", G_CALLBACK(perform_filter), NULL);
	g_signal_connect(G_OBJECT (filter_entry), "icon-press", G_CALLBACK(clear_filter), NULL);

  //~ g_signal_connect(G_OBJECT (save_configuration_menu_item), "activate", G_CALLBACK(save_configuration), window);
  //~ g_signal_connect(G_OBJECT (read_configuration_menu_item), "activate", G_CALLBACK(read_configuration), window);

	// Read the configuration...
	read_configuration(global->kvo_list);
	update_recent_list(global->kvo_list);

	// Run the app
  gtk_widget_show_all(gui->window);
  gtk_main();

	// Save the configuration...
	save_configuration(global->kvo_list);

  return 0;
}
