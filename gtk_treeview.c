#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <string.h>

#include "gtk_treeview.h"
#include "main.h"
#include "xml.h"
#include "functions.h"

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

		xmlNode* keyvault = data;
		xmlNode* group = xmlNewChild(keyvault, NULL, BAD_CAST "node", NULL);
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
// import an xml text into the treestore
//

static void xmlContentsIntoTreestoreColumn(xmlNode* node, char* name, GtkTreeStore* treestore, GtkTreeIter* iter, int col) {
	const char* text = (char*)xmlGetContents(node, BAD_CAST name);
	gtk_tree_store_set(treestore, iter, col, text, -1);
}

void import_xml_into_treestore(GtkTreeStore* treestore, xmlDoc* doc) {
	// Remove all rows
	gtk_tree_store_clear(treestore);

	// Read all <node> items and place them inthe tree store
	GtkTreeIter iter;
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlNode* node = root->children;
	while(node) {
		if ((node->type == XML_ELEMENT_NODE) && xmlStrEqual(node->name, BAD_CAST "node")) {
			gtk_tree_store_append(treestore, &iter, NULL);
			xmlContentsIntoTreestoreColumn(node, "title", treestore, &iter, COL_TITLE);
			xmlContentsIntoTreestoreColumn(node, "username", treestore, &iter, COL_USERNAME);
			xmlContentsIntoTreestoreColumn(node, "password", treestore, &iter, COL_PASSWORD);
			xmlContentsIntoTreestoreColumn(node, "url", treestore, &iter, COL_URL);
			xmlContentsIntoTreestoreColumn(node, "info", treestore, &iter, COL_INFO);
		}
		node = node->next;
	}
}

// -----------------------------------------------------------
//
// write a while record into the treestore
//

void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* title, const char* username, const char* password, const char* url, const char* info) {
	gtk_tree_store_append(treestore, iter, parent);
	gtk_tree_store_set(treestore, iter, COL_TITLE, title, COL_USERNAME, username, COL_PASSWORD, password, COL_URL, url, COL_INFO, info, -1);
}

// -----------------------------------------------------------
//
// export the treestore to an csv file (UNENCRYPTED)
//

void export_treestore_to_csv(GtkTreeStore* treestore, const char* filename) {
	// Callback per treenode
	gboolean treenode_to_csv(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
		FILE* fh = data;
		char* title;
		char* username;
		char* password;
		char* url;
		char* info;

		gtk_tree_model_get(model, iter, COL_TITLE, &title, COL_USERNAME, &username, COL_PASSWORD, &password, COL_URL, &url, COL_INFO, &info,  -1);
		fprintf(fh,"\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",title,username,password,url,info);

		g_free(title);
		g_free(username);
		g_free(password);
		g_free(url);
		g_free(info);
		return FALSE;
	}

	// Create the xml container
	FILE* fh = fopen(filename,"w");
	fprintf(fh,"title,username,password,url,info\n");

	// Foreach node call "treenode_to_xml" and store the node into the <keyvault> element
	gtk_tree_model_foreach(GTK_TREE_MODEL(treestore), treenode_to_csv, fh);
	
	// Close the handle
	fclose(fh);
}

// -----------------------------------------------------------
//
// import the treestore from an csv file (UNENCRYPTED)
//

char** read_csv_line(char* line) {
	line++;
	char** list = mallocz(10 * sizeof(char*));
	int i;
	for (i=0;i<10;i++) {
		char* ptr = strstr(line,"\",\"");
		if (!ptr)
			break;
		list[i] = line;
		*ptr=0;
		line = ptr + 3;
	}
	list[i] = line;
	char* ptr = strchr(line,'"');
	*ptr=0;
	return list;
}

void import_treestore_from_csv(GtkTreeStore* treestore, const char* filename) {
	// Remove all rows
	gtk_tree_store_clear(treestore);

	char* line = malloc(16384);
	FILE* fh = fopen(filename,"r");
	fgets(line, 16384, fh);

	// Read all lines and place them in the tree store
	GtkTreeIter iter;
	while(fgets(line, 16384, fh)) {
		char** array = read_csv_line(line);
		printf("array: %s / %s / %s / %s / %s\n", array[0], array[1], array[2], array[3], array[4]);
		treestore_add_record(treestore, &iter, NULL, array[0], array[1], array[2], array[3], array[4]);
		//~ gtk_tree_store_append(treestore, &iter, NULL);
		//~ gtk_tree_store_set(treestore, &iter, COL_TITLE, title, -1);
		//~ gtk_tree_store_set(treestore, &iter, COL_USERNAME, username, -1);
		//~ gtk_tree_store_set(treestore, &iter, COL_PASSWORD, password, -1);
		//~ gtk_tree_store_set(treestore, &iter, COL_URL, url, -1);
		//~ gtk_tree_store_set(treestore, &iter, COL_INFO, info, -1);
	}
}
