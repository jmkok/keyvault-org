#ifndef _gtk_dialogs_h_
#define _gtk_dialogs_h_

#include "main.h"
#include "structures.h"

extern gchar* gtk_dialog_password(GtkWindow* parent, const gchar* title);
extern void gtk_dialog_error(const gchar *message);

extern gboolean gtk_dialog_request_config(GtkWidget* parent, tConfigDescription* config);

extern gchar* gtk_dialog_open_file(GtkWindow* parent_window, int filter);
extern gchar* gtk_dialog_save_file(GtkWindow* parent_window, int filter);

#endif
