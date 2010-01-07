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

extern GtkWidget* create_view_and_model(void);
extern void on_changed(GtkWidget *widget, gpointer statusbar);

char* export_treestore_to_xml(GtkTreeStore* treestore);
void import_xml_into_treestore(GtkTreeStore* treestore,char* xml);

#endif
