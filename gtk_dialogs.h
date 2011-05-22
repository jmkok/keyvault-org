#ifndef _gtk_dialogs_h_
#define _gtk_dialogs_h_

#include "main.h"
#include "structures.h"

extern gchar* gtk_dialog_password(GtkWindow* parent, const gchar* title);
extern gint gtk_dialog_ex(const gchar *message, GtkMessageType type);

extern gboolean gtk_dialog_request_config(GtkWidget* parent, tFileDescription* kvo);

extern gchar* gtk_dialog_open_file(GtkWindow* parent_window, int filter);
extern gchar* gtk_dialog_save_file(GtkWindow* parent_window, int filter);

#define gtk_info(msg) gtk_dialog_ex(msg, GTK_MESSAGE_INFO)
#define gtk_warning(msg) gtk_dialog_ex(msg, GTK_MESSAGE_WARNING)
#define gtk_question(msg) gtk_dialog_ex(msg, GTK_MESSAGE_QUESTION)
#define gtk_error(msg) gtk_dialog_ex(msg, GTK_MESSAGE_ERROR)

#endif
