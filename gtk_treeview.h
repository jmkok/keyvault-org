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
  COL_INFO,
  COL_CREATED,
  COL_MODIFIED,
  NUM_COLS
};

void import_xml_into_treestore(GtkTreeStore* treestore,xmlDoc* doc);
xmlDoc* export_treestore_to_xml(GtkTreeStore* treestore);

extern void import_treestore_from_csv(GtkTreeStore* treestore, const char* filename);
extern void export_treestore_to_csv(GtkTreeStore* treestore, const char* filename);

void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* title, const char* username, const char* password, const char* url, const char* info);

#endif
