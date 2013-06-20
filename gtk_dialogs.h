#ifndef _gtk_dialogs_h_
#define _gtk_dialogs_h_

#include "main.h"
#include "structures.h"

struct FILE_LOCATION;

extern gchar* gtk_dialog_password(GtkWindow* parent, const gchar* title);
extern gint gtk_dialog_ex(const gchar *message, GtkMessageType type);

extern gboolean gtk_dialog_request_config(GtkWidget* parent, struct FILE_LOCATION* kvo);

extern gchar* gtk_dialog_open_file(GtkWindow* parent_window, int filter);
extern gchar* gtk_dialog_save_file(GtkWindow* parent_window, int filter);

int gtk_dialog_printf(GtkMessageType type, const char* fmt, ...);

#define gtk_info(msg...) gtk_dialog_printf(GTK_MESSAGE_INFO, msg)
#define gtk_warning(msg...) gtk_dialog_printf(GTK_MESSAGE_WARNING, msg)
#define gtk_question(msg...) gtk_dialog_printf(GTK_MESSAGE_QUESTION, msg)
#define gtk_error(msg...) gtk_dialog_printf(GTK_MESSAGE_ERROR, msg)

#endif
