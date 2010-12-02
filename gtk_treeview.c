#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <string.h>
#include <assert.h>

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
// static functions (needed as they reference each other)
//

static long int time_to_int(const char* text) {
	if (!text || !*text)
		return time(0);
	if (strchr(text,':') || strchr(text,'-')) {
		struct tm* xtime = mallocz(sizeof(struct tm));
		int retval = sscanf(text,"%d-%d-%d %d:%d:%d",&xtime->tm_mday, &xtime->tm_mon, &xtime->tm_year, &xtime->tm_hour, &xtime->tm_min, &xtime->tm_sec);
		if ((retval == 3) || (retval == 6)) {
			xtime->tm_mon--;
			xtime->tm_year-=1900;
			//~ printf("date: %02d-%02d-%04d\n", xtime->tm_mday, xtime->tm_mon+1, xtime->tm_year+1900);
			//~ printf("time: %02d:%02d:%02d\n", xtime->tm_hour, xtime->tm_min, xtime->tm_sec);
			//~ char tmp[32];
			//~ asctime_r(xtime,tmp);
			//~ printf("asctime(): %s\n", tmp);
			time_t unix_time = mktime(xtime);
			//~ printf("unix_time: %ld\n", unix_time);
			return unix_time;
		}
		return 0;
	}
	else {
		return atol(text);
	}
}

// -----------------------------------------------------------
//
// export the treestore into an xml text
//

xmlDoc* export_treestore_to_xml(GtkTreeStore* treestore) {
	debugf("export_treestore_to_xml(%p)\n", treestore);
	// Callback per treenode
	gboolean treenode_to_xml(GtkTreeModel *model, _UNUSED_ GtkTreePath* path, GtkTreeIter *iter, gpointer data) {
		char* id;
		char* title;
		char* username;
		char* password;
		char* url;
		char* group;
		char* info;
		int time_created;
		int time_modified;

		gtk_tree_model_get(model, iter, 
			COL_ID, &id, 
			COL_TITLE, &title, 
			COL_USERNAME, &username, 
			COL_PASSWORD, &password, 
			COL_URL, &url, 
			COL_GROUP, &group,
			COL_INFO, &info,
			COL_TIME_CREATED, &time_created,
			COL_TIME_MODIFIED, &time_modified,
			-1);

		xmlNode* keyvault = data;
		xmlNode* node = xmlNewChild(keyvault, NULL, XML_CHAR "node", NULL);
		xmlNewTextChild(node, NULL, XML_CHAR "id", BAD_CAST id);
		xmlNewTextChild(node, NULL, XML_CHAR "title", BAD_CAST title);
		xmlNewTextChild(node, NULL, XML_CHAR "username", BAD_CAST username);
		xmlNewTextChild(node, NULL, XML_CHAR "password", BAD_CAST password);
		if (url)
			xmlNewTextChild(node, NULL, XML_CHAR "url", BAD_CAST url);
		if (info)
			xmlNewTextChild(node, NULL, XML_CHAR "info", BAD_CAST info);
		if (group)
			xmlNewTextChild(node, NULL, XML_CHAR "group", BAD_CAST group);
		xmlNewIntegerChild(node, NULL, XML_CHAR "time-created", time_created);
		xmlNewIntegerChild(node, NULL, XML_CHAR "time-modified", time_modified);

		g_free(id);
		g_free(title);
		g_free(username);
		g_free(password);
		g_free(url);
		g_free(group);
		g_free(info);
		return FALSE;
	}

	// Create the xml container
	xmlDoc* doc = xmlNewDoc(XML_CHAR "1.0");

	// Create the root node
	xmlNode* keyvault = xmlNewNode(NULL, XML_CHAR "keyvault");
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

void xml_contents_into_treestore_column(xmlNode* node, const char* name, GtkTreeStore* treestore, GtkTreeIter* iter, int col) {
	xmlChar* text = xmlGetTextContents(node, CONST_BAD_CAST name);
	if ((col == COL_TIME_CREATED) || (col == COL_TIME_MODIFIED))
		gtk_tree_store_set(treestore, iter, col, time_to_int((char*)text), -1);
	else
		gtk_tree_store_set(treestore, iter, col, text, -1);
	xmlFree(text);
}

void import_treestore_from_xml(GtkTreeStore* treestore, xmlDoc* doc) {
	debugf("import_treestore_from_xml(%p,%p)\n", treestore, doc);
	assert(doc);
	
	// Remove all rows
	gtk_tree_store_clear(treestore);

	// Get the root
	xmlNode* root = xmlDocGetRootElement(doc);
	assert(root);
	//~ xmlNodeShow(root);exit(0);

	// Read all <node> items and place them inthe tree store
	xmlNode* node = root->children;
	while(node) {
		if ((node->type == XML_ELEMENT_NODE) && xmlStrEqual(node->name, XML_CHAR "node")) {
			GtkTreeIter iter;
			gtk_tree_store_append(treestore, &iter, NULL);
			xml_contents_into_treestore_column(node, "id", treestore, &iter, COL_ID);
			xml_contents_into_treestore_column(node, "title", treestore, &iter, COL_TITLE);
			xml_contents_into_treestore_column(node, "username", treestore, &iter, COL_USERNAME);
			xml_contents_into_treestore_column(node, "password", treestore, &iter, COL_PASSWORD);
			xml_contents_into_treestore_column(node, "url", treestore, &iter, COL_URL);
			xml_contents_into_treestore_column(node, "group", treestore, &iter, COL_GROUP);
			xml_contents_into_treestore_column(node, "info", treestore, &iter, COL_INFO);
			xml_contents_into_treestore_column(node, "time-created", treestore, &iter, COL_TIME_CREATED);
			xml_contents_into_treestore_column(node, "time-modified", treestore, &iter, COL_TIME_MODIFIED);
		}
		else {
			printf("NODE: %s\n",node->name);
			//~ xmlNodeShow(node);
		}
		node = node->next;
	}
}

// -----------------------------------------------------------
//
// write a while record into the treestore
//

void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* id, const char* title, const char* username, const char* password, const char* url, const char* group, const char* info, time_t time_created, time_t time_modified) {
	gtk_tree_store_append(treestore, iter, parent);
	gtk_tree_store_set(treestore, iter,
		COL_ID, id, 
		COL_TITLE, title, 
		COL_USERNAME, username, 
		COL_PASSWORD, password, 
		COL_URL, url, 
		COL_GROUP, group,
		COL_INFO, info, 
		COL_TIME_CREATED, time_created,
		COL_TIME_MODIFIED, time_modified,
		-1);
}

// -----------------------------------------------------------
//
// export the treestore to an csv file (UNENCRYPTED)
//

void export_treestore_to_csv(GtkTreeStore* treestore, const char* filename) {
	debugf("export_treestore_to_csv(%p,'%s')\n", treestore, filename);
	// Callback per treenode
	gboolean treenode_to_csv(GtkTreeModel *model, _UNUSED_ GtkTreePath* path, GtkTreeIter *iter, gpointer data) {
		FILE* fh = data;
		char* id;
		char* title;
		char* username;
		char* password;
		char* url;
		char* group;
		char* info;
		int time_created;
		int time_modified;

		gtk_tree_model_get(model, iter, 
			COL_ID, &id, 
			COL_TITLE, &title, 
			COL_USERNAME, &username, 
			COL_PASSWORD, &password, 
			COL_URL, &url, 
			COL_GROUP, &group,
			COL_INFO, &info,
			COL_TIME_CREATED, &time_created,
			COL_TIME_MODIFIED, &time_modified,
			-1);
		fprintf(fh,"\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%u\",\"%u\"\n", id, title, username, password, url, group, info, time_created, time_modified);

		g_free(id);
		g_free(title);
		g_free(username);
		g_free(password);
		g_free(url);
		g_free(group);
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
	debugf("import_treestore_from_csv(%p,'%s')\n", treestore, filename);
	// Remove all rows
	gtk_tree_store_clear(treestore);

	char* line = malloc(16384);
	FILE* fh = fopen(filename,"r");
	fgets(line, 16384, fh);

	// Read all lines and place them in the tree store
	GtkTreeIter iter;
	while(fgets(line, 16384, fh)) {
		char** array = read_csv_line(line);
		printf("array: %s / %s / %s / %s / %s / %s / %s / %s / %s\n", array[0], array[1], array[2], array[3], array[4], array[5], array[6], array[7], array[8]);
		int time_created = time_to_int(array[7]);
		int time_modified = time_to_int(array[8]);
		treestore_add_record(treestore, &iter, NULL, array[0], array[1], array[2], array[3], array[4], array[5], array[6], time_created, time_modified);
	}
}
