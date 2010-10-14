#include <gtk/gtk.h>
#include <libxml/parser.h>

#include "treeview.h"
#include "main.h"
#include "xml.h"

// -----------------------------------------------------------
//
// Global variables
//

// The data tree
extern GtkWidget* data_treeview;
extern GtkTreeStore* data_treestore;
extern GtkTreeModel* data_treemodel;

// -----------------------------------------------------------
//
// export the treestore into an xml text
//

xmlDoc* export_treestore_to_xml(GtkTreeStore* treestore) {
	// Callback per treenode
	gboolean treenode_to_xml(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
		trace();
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
	xmlDoc* doc = xmlNewDoc(BAD_CAST "1.0");

	// Create the root node
	xmlNode* keyvault = xmlNewNode(NULL, BAD_CAST "keyvault");
	xmlDocSetRootElement(doc, keyvault);

	// Foreach node call "treenode_to_xml" and store the node into the <keyvault> element
	gtk_tree_model_foreach(GTK_TREE_MODEL(treestore), treenode_to_xml, keyvault);

	// Return the xml doc
	return doc;
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

//~ extern xmlNode* xmlFindNode(xmlNode* root_node, xmlChar* name, int depth);

void extract_field(xmlNode* node, char* name, GtkTreeStore* treestore, GtkTreeIter* self, int col) {
	const char* text = (char*)xmlGetContents(node, BAD_CAST name);
	gtk_tree_store_set(treestore, self, col, text, -1);
}

void import_xml_into_treestore(GtkTreeStore* treestore, xmlDoc* doc) {
	// Remove all rows
	trace();
	gtk_tree_store_clear(treestore);
	trace();
	exit(1);
	
	GtkTreeIter self;
	//~ mxml_node_t* node=xml;
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlNode* node = root->children;
	//~ while ((node=mxmlFindElement(node,xml,"node",NULL,NULL,MXML_DESCEND))) {
	//~ while ((node=xmlFindNode(node, BAD_CAST "node", 0))) {
	while(node) {
		if (node->type == XML_ELEMENT_NODE) {
			gtk_tree_store_append(treestore, &self, NULL);
			extract_field(node,"title", treestore, &self, COL_TITLE);
			extract_field(node,"username", treestore, &self, COL_USERNAME);
			extract_field(node,"password", treestore, &self, COL_PASSWORD);
			extract_field(node,"url", treestore, &self, COL_URL);
			extract_field(node,"info", treestore, &self, COL_INFO);
		}
		node = node->next;
	}
	//~ my_tree_add(treestore, &toplevel, NULL, "You/Com accounts","","pwd","http://server.com/","info");
	todo();	// gtk_widget_show_all(gui->window);
}
