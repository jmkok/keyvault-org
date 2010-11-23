#ifndef _treeview_h_
#define _treeview_h_

#include <gtk/gtk.h>
#include <libxml/parser.h>

enum {
  COL_ID = 0,
  COL_TITLE,
  COL_USERNAME,
  COL_PASSWORD,
  COL_URL,
  COL_GROUP,
  COL_INFO,
  COL_TIME_CREATED,
  COL_TIME_MODIFIED,
  NUM_COLS
};

// Import/export from XML
void import_treestore_from_xml(GtkTreeStore* treestore,xmlDoc* doc);
xmlDoc* export_treestore_to_xml(GtkTreeStore* treestore);

// Import/export from CSV
extern void import_treestore_from_csv(GtkTreeStore* treestore, const char* filename);
extern void export_treestore_to_csv(GtkTreeStore* treestore, const char* filename);

//~ void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* title, const char* username, const char* password, const char* url, const char* info);
//~ void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* id, const char* title, const char* username, const char* password, const char* url, const char* info, const char* time_created, const char* time_modified);
void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* id, const char* title, const char* username, const char* password, const char* url, const char* group, const char* info, time_t time_created, time_t time_modified);

#endif
