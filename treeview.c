#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>
//~ #include <mxml.h>
#include <libxml/parser.h>

#include "treeview.h"
#include "main.h"
//~ #include "xml.h"
#include "encryption.h"

// -----------------------------------------------------------
//
// Global variables
//

//~ extern GtkTreeStore* treestore;

// -----------------------------------------------------------
//
// export the treestore into an xml text
//

char* export_treestore_to_xml(GtkTreeStore* treestore) {
	// Callback per treenode
	gboolean treenode_to_mxml(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
		char* title;
		char* username;
		char* password;
		char* url;
		char* info;

		gtk_tree_model_get(model, iter, COL_TITLE, &title, COL_USERNAME, &username, COL_PASSWORD, &password, COL_URL, &url, COL_INFO, &info,  -1);

		//~ mxml_node_t* keyvault = data;
		//~ mxml_node_t* group = mxmlNewElement(keyvault, "node");
		xmlNode* keyvault = data;
		xmlNode* group = xmlNewChild(keyvault, NULL, BAD_CAST "node", NULL);
		//~ mxml_node_t* node;
		//~ node = mxmlNewElement(group, "title");		mxmlNewText(node, 0, title);
		//~ node = mxmlNewElement(group, "username");	mxmlNewText(node, 0, username);
		//~ node = mxmlNewElement(group, "password");	mxmlNewText(node, 0, password);
		//~ node = mxmlNewElement(group, "url");			mxmlNewText(node, 0, url);
		//~ node = mxmlNewElement(group, "info");			mxmlNewText(node, 0, info);
		xmlNewChild(group, NULL, BAD_CAST "title", BAD_CAST title);
		xmlNewChild(group, NULL, BAD_CAST "username", BAD_CAST username);
		xmlNewChild(group, NULL, BAD_CAST "password", BAD_CAST password);
		xmlNewChild(group, NULL, BAD_CAST "url", BAD_CAST url);
		xmlNewChild(group, NULL, BAD_CAST "info", BAD_CAST info);
		g_free(title);
		g_free(username);
		g_free(password);
		g_free(url);
		g_free(info);
		return FALSE;
	}

	// Create the xml container
	//~ mxml_node_t* xml = mxmlNewXML("1.0");
	xmlDoc* xml = xmlNewDoc(BAD_CAST "1.0");
	//~ mxml_node_t* keyvault = mxmlNewElement(xml,"keyvault");
	xmlNode* keyvault = xmlNewDocNode(xml, NULL, BAD_CAST "keyvault", NULL);

	// Foreach node call "treenode_to_mxml" and store the node into the <keyvault> element
	gtk_tree_model_foreach(GTK_TREE_MODEL(treestore), treenode_to_mxml, keyvault);

	// Return the xml container as a string
	//~ return mxmlSaveAllocString(xml, whitespace_cb);
	return NULL;
}

// -----------------------------------------------------------
//
// write a while record into the treestore
//

void my_tree_add(GtkTreeStore *treestore ,GtkTreeIter* self, GtkTreeIter* parent, char* title, char* username,char* password,char* url,char* info) {
	gtk_tree_store_append(treestore, self, parent);
	gtk_tree_store_set(treestore, self, COL_TITLE, title, COL_USERNAME, username, COL_PASSWORD, password, COL_URL, url, COL_INFO, info, -1);
}

// -----------------------------------------------------------
//
// import an xml text into the treestore
//

extern xmlNode* xmlFindNode(xmlNode* root_node, xmlChar* name, int depth);

void import_xml_into_treestore(GtkTreeStore* treestore, char* xml_text) {
	//~ mxml_node_t* xml = mxmlLoadString (NULL,xml_text,MXML_NO_CALLBACK);
	xmlDoc* xml = xmlReadMemory(xml_text, strlen(xml_text), NULL, NULL, 0);
	
	// Show the xml structure
	//~ mxmlSaveFile(xml,stdout,whitespace_cb);

	// Remove all rows
	gtk_tree_store_clear(treestore);
	
	//~ GtkTreeIter self;
	//~ mxml_node_t* node=xml;
	xmlNode* node = xmlDocGetRootElement(xml);
	//~ while ((node=mxmlFindElement(node,xml,"node",NULL,NULL,MXML_DESCEND))) {
	while ((node=xmlFindNode(node, BAD_CAST "node", 0))) {
/*
		if (node->type == MXML_ELEMENT) {
			//~ printf("MXML_ELEMENT: %s\n",node->value.element.name);
			mxml_node_t* child=node;
			gtk_tree_store_append(treestore, &self, NULL);
			while ((child=mxmlWalkNext(child,node,MXML_DESCEND))) {
				if (child->type == MXML_ELEMENT) {
					//~ printf("\tMXML_ELEMENT: %s\n",child->value.element.name);
					char* name=child->value.element.name;
					child=mxmlWalkNext(child,node,MXML_DESCEND);
					if (child->type == MXML_TEXT) {
						//~ printf("\tMXML_TEXT: %s\n",child->value.text.string);
						char* value=child->value.text.string;
						if (strcmp(name,"title") == 0)		gtk_tree_store_set(treestore, &self, COL_TITLE, value, -1);
						if (strcmp(name,"username") == 0)	gtk_tree_store_set(treestore, &self, COL_USERNAME, value, -1);
						if (strcmp(name,"password") == 0)	gtk_tree_store_set(treestore, &self, COL_PASSWORD, value, -1);
						if (strcmp(name,"url") == 0)			gtk_tree_store_set(treestore, &self, COL_URL, value, -1);
						if (strcmp(name,"info") == 0)			gtk_tree_store_set(treestore, &self, COL_INFO, value, -1);
					}
				}
				else if (child->type == MXML_TEXT) {
					//~ printf("\tMXML_TEXT: %s\n",child->value.text.string);
				}
				else {
					//~ printf("\tMXML_OTHER\n");
				}
			}
			//~ my_tree_add(treestore, &toplevel, NULL, "You/Com accounts","","pwd","http://server.com/","info");
		}
*/
	}
	gtk_widget_show_all(gui->window);
/**/
}

void on_changed(GtkWidget *widget, gpointer statusbar)
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
		gtk_entry_set_text(GTK_ENTRY(gui->title_entry),title);
		gtk_entry_set_text(GTK_ENTRY(gui->username_entry),name);
		gtk_entry_set_text(GTK_ENTRY(gui->password_entry),password);
		gtk_entry_set_text(GTK_ENTRY(gui->url_entry),url);
		GtkTextBuffer* buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->info_text));
		gtk_text_buffer_set_text(buffer,info,-1);
    g_free(title);
    g_free(name);
    g_free(password);
    g_free(url);
    g_free(info);
  }
}

void lowercase(char string[])
{
   int  i = 0;
   while ( string[i] ) {
      string[i] = tolower(string[i]);
      i++;
   }
   return;
}

// Filter function
// It filters the title column, but potentially could filter other columns
extern char* filter_title;
static gboolean visible_func (GtkTreeModel *model, GtkTreeIter  *iter, gpointer data)
{
	gchar *str;
	gboolean visible = FALSE;
	gtk_tree_model_get (model, iter, COL_TITLE, &str, -1);
	if (str) {
		lowercase(str);
		if (strstr(str,filter_title))
			visible = TRUE;
		g_free (str);
	}

	return visible;
}

// Sort function
gint sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data){
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

// Reverse the sort order
int sort_order=0;
void click_col1(GtkWidget *widget, gpointer ptr) {
	GtkTreeViewColumn* col1 = (GtkTreeViewColumn*)widget;
	sort_order = 1-sort_order;
	gtk_tree_view_column_set_sort_order(col1, sort_order);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gui->treestore), COL_TITLE, sort_order);
}

GtkWidget* create_view_and_model(void)
{
	// Create the treestore (8 strings fields)
	gui->treestore = gtk_tree_store_new(NUM_COLS,
		G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,
		G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING
	);

	// Sort on title
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(gui->treestore), COL_TITLE, sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gui->treestore), COL_TITLE, sort_order);

	// Create the filtered model ("filter_title")
	gui->treemodel=gtk_tree_model_filter_new(GTK_TREE_MODEL(gui->treestore), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(gui->treemodel), visible_func, NULL, NULL);

	// TODO: howto get "gtk_tree_model_filter_new" AND "gtk_tree_selection_get_selected" working together...
	// When enabling the line below:
	// - I can then correctly filter
	// - I CANNOT correctly use "write_changes_to_treestore" anymore
	todo();
	gui->treemodel=GTK_TREE_MODEL(gui->treestore);

	// Create the treeview
  GtkWidget* view = gtk_tree_view_new_with_model (gui->treemodel);

	// Create column 1 and set the title
  GtkTreeViewColumn* col1 = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col1, "Titles");
  gtk_tree_view_column_set_clickable(col1, TRUE);
  gtk_tree_view_column_set_sort_indicator(col1, TRUE);
  //~ gtk_tree_view_column_set_sort_column_id(col1, COL_TITLE);	// Is this easier ? It currently only gives me trouble...
  g_signal_connect(G_OBJECT (col1), "clicked", G_CALLBACK(click_col1), NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col1);
  
  // Define how column 1 looks like
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col1, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col1, renderer, "text", COL_TITLE);

	// Make the title searchable
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), COL_TITLE);

  return view;
}
