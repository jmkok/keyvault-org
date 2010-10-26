#ifndef _gtk_dialogs_h_
#define _gtk_dialogs_h_

#include "main.h"
#include "structures.h"

extern gchar* dialog_request_password (GtkWindow* parent, gchar* title);
extern void gtk_error_dialog(const gchar *message);

extern gboolean dialog_request_kvo(tFileDescription* );

extern gchar* dialog_open_file(GtkWidget *widget, gpointer parent_window, int filter);
extern gchar* dialog_save_file(GtkWidget *widget, gpointer parent_window);


#endif
