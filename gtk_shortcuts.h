#ifndef _gtk_shortcuts_h_
#define _gtk_shortcuts_h_

extern GtkWidget* gtk_add_labeled_entry(GtkWidget* vbox, const gchar* title, const gchar* default_val);
extern GtkWidget* gtk_add_label(GtkWidget* vbox, const gchar* title);
extern GtkWidget* gtk_add_entry(GtkWidget* vbox, const gchar* value);

extern GtkWidget* gtk_add_separator(GtkWidget* parent);
extern GtkWidget* gtk_add_menu_item(GtkWidget* menu, const gchar* title);
extern GtkWidget* gtk_add_menu_item_clickable(GtkWidget* menu, const gchar* title, GCallback, void* ptr);
extern GtkWidget* gtk_add_menu(GtkWidget* menu, const gchar* title);
extern void gtk_remove_menu_item(GtkWidget* menu_item, GtkWidget* parent);

extern GtkFileFilter* gtk_add_filter(GtkFileChooser* widget, const char* title, const char* filter, int selected);

extern GtkWidget* gtk_add_scroll_bar_to_treeview(GtkWidget *treeView);

/* This prints an "Assertion failed" message and aborts.  */
extern void __gtk_assert_fail (__const char *__assertion, __const char *__file, unsigned int __line, __const char *__function)
     __THROW __attribute__ ((__noreturn__));


#define gtk_assert(expr)                                                   \
  ((expr)                                                               \
   ? __ASSERT_VOID_CAST (0)                                             \
   : __gtk_assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION))

#undef assert
#define assert gtk_assert

#endif
