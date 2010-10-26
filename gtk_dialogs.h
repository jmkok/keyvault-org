#ifndef _dialogs_h_
#define _dialogs_h_

#include "main.h"
#include "structures.h"

extern gchar* dialog_request_password (GtkWindow* parent, gchar* title);
extern gboolean dialog_request_kvo (tKvoFile* );

extern gchar* dialog_open_file(GtkWidget *widget, gpointer parent_window);
extern gchar* dialog_save_file(GtkWidget *widget, gpointer parent_window);
extern void about_widget(GtkWidget *widget, gpointer parent_window);

#endif
