#include <stdint.h>
#include <mxml.h>
#include <gtk/gtk.h>

#include "configuration.h"
#include "structures.h"
#include "xml.h"
#include "main.h"
#include "gtk_shortcuts.h"

GList* kvo_list=NULL;

// -----------------------------------------------------------
//
// Create the list of recent files
//

void update_recent_list(void) {
	// Remove all items in the recent menu
	void cb_remove_menu_item(GtkWidget* menu_item, gpointer unused) {
		gtk_remove_menu_item(menu_item, gui->open_recent_menu);
	}
	gtk_container_foreach(GTK_CONTAINER(gui->open_recent_menu), cb_remove_menu_item, 0);
	// Add all items in the kvo_list to the recent menu
	void add_to_menu(gpointer data, gpointer user_data) {
		tKvoFile* kvo = data;
		GtkWidget* account_menu_item = gtk_add_menu_item(kvo->title,gui->open_recent_menu);
		g_signal_connect(G_OBJECT (account_menu_item), "activate", G_CALLBACK(open_kvo_file), kvo);
	}
	g_list_foreach(kvo_list,add_to_menu,0);
	// Show the recent menu
	gtk_widget_show_all(gui->open_recent_menu);
}

// -----------------------------------------------------------
//
// Save the configuration
//

void save_configuration(void) {
	printf("save_configuration()\n");
	// Create an xml node that conatins all configuration
	mxml_node_t* xml = mxmlNewXML("1.0");
	mxml_node_t* config = mxmlNewElement(xml,"config");
	int i;
	for (i=0;i < g_list_length(kvo_list); i++) {
		tKvoFile* kvo = g_list_nth_data(kvo_list, i);
		mxml_node_t* kvo_file = mxmlNewElement(config, "kvo_file");
		mxml_node_t* node;
		// title
		node = mxmlNewElement(kvo_file, "title");			
		mxmlNewText(node, 0, kvo->title);
		// protocol
		node = mxmlNewElement(kvo_file, "protocol");	
		mxmlNewText(node, 0, kvo->protocol);
		// hostname
		if (kvo->hostname) {
			node = mxmlNewElement(kvo_file, "hostname");	
			mxmlNewText(node, 0, kvo->hostname);
		}
		// port
		if (kvo->port) {
			node = mxmlNewElement(kvo_file, "port");			
			mxmlNewInteger(node, kvo->port);
		}
		// username
		if (kvo->username) {
			node = mxmlNewElement(kvo_file, "username");	
			mxmlNewText(node, 0, kvo->username);
		}
		// password
		if (kvo->password) {
			node = mxmlNewElement(kvo_file, "password");	
			mxmlNewText(node, 0, kvo->password);
		}
		// filename
		node = mxmlNewElement(kvo_file, "filename");	
		mxmlNewText(node, 0, kvo->filename);
		// fingerprint (ssh)
		if (kvo->fingerprint)	{
			node = mxmlNewElement(kvo_file, "fingerprint");	
			char* tmp=g_base64_encode(kvo->fingerprint,16);
			mxmlNewText(node, 0, tmp);
			g_free(tmp);
		}
	}
	// Save the xml node
	FILE* fh=fopen("config.xml","wt");
	if (fh) {		
		mxmlSaveFile(xml,fh,whitespace_cb);
		fclose(fh);
	}
	// Remove the xml node
	mxmlDelete(xml);
}

// -----------------------------------------------------------
//
// Read the configuration
//

void read_configuration(void) {
	printf("read_configuration()\n");
	FILE* config=fopen("config.xml","rt");
	if (config) {
		mxml_node_t* xml = mxmlLoadFile (NULL,config,MXML_NO_CALLBACK);
		fclose(config);
		//~ mxmlSaveFile(xml,stdout,whitespace_cb);
		mxml_node_t* config_node;
		if ((config_node = mxmlFindElement(xml,xml,"config",NULL,NULL,MXML_DESCEND))) {
			//~ mxmlSaveFile(config_node,stdout,whitespace_cb);
			mxml_node_t* keyvault_node=config_node;
			gsize size;
			while ((keyvault_node=mxmlFindElement(keyvault_node,config_node,"kvo_file",NULL,NULL,MXML_DESCEND))) {
				// Create a new kvo_file
				tKvoFile* kvo=malloc(sizeof(tKvoFile));
				memset(kvo,0,sizeof(tKvoFile));
				kvo->local_filename = strdup("local.kvo");
				kvo_list = g_list_append(kvo_list,kvo);
				// Analyze all items for this kvo_file
				mxml_node_t* child=keyvault_node;
				while ((child=mxmlWalkNext(child,keyvault_node,MXML_DESCEND))) {
					if (child->type == MXML_ELEMENT) {
						mxml_node_t* text=child;
						while ((text=mxmlWalkNext(text,child,MXML_DESCEND))) {
							if (text->type == MXML_TEXT) {
								if (strcmp(child->value.element.name,"title") == 0)
									kvo->title = strdup(text->value.text.string);
								if (strcmp(child->value.element.name,"protocol") == 0)
									kvo->protocol = strdup(text->value.text.string);
								if (strcmp(child->value.element.name,"hostname") == 0)
									kvo->hostname = strdup(text->value.text.string);
								if (strcmp(child->value.element.name,"port") == 0)
									kvo->port = text->value.integer;
								if (strcmp(child->value.element.name,"username") == 0)
									kvo->username = strdup(text->value.text.string);
								if (strcmp(child->value.element.name,"password") == 0)
									kvo->password = strdup(text->value.text.string);
								if (strcmp(child->value.element.name,"filename") == 0)
									kvo->filename = strdup(text->value.text.string);
								if (strcmp(child->value.element.name,"fingerprint") == 0)
									kvo->fingerprint = g_base64_decode(text->value.text.string, &size);
									
							}
						}
						//~ printf("\n");
					}
				}
			}
		}
	}
	
	// Update the list of recent files...
	update_recent_list();
	
	return;
}

