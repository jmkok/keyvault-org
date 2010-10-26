#ifndef _treeview_h_
#define _treeview_h_

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

xmlDoc* export_treestore_to_xml(GtkTreeStore* treestore);
void import_xml_into_treestore(GtkTreeStore* treestore,xmlDoc* doc);

void treestore_add_record(GtkTreeStore* treestore, GtkTreeIter* iter, GtkTreeIter* parent, const char* title, const char* username, const char* password, const char* url, const char* info);

#endif
