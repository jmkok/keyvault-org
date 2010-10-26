# Needed libraries
# - libglib2.0-dev
# - libgtk2.0-dev
# - libxml2-dev
# - libssh2-1-dev
# Documentation
# - http://library.gnome.org/devel/gtk/stable/

TARG=keyvault
SRCS=main.c \
	gtk_ui.c gtk_treeview.c gtk_dialogs.c gtk_shortcuts.c \
	configuration.c ssh.c encryption.c functions.c list.c xml.c
OBJS=$(SRCS:.c=.o)
LIBRARIES=glib-2.0 gtk+-2.0 libxml-2.0 openssl libssh2
CFLAGS=`pkg-config --cflags $(LIBRARIES)` -Wall -Werror
LIBS=`pkg-config --libs $(LIBRARIES)`

.cc.o:

all: $(TARG)
	#~ make run

run: $(TARG)
	LANG=C ./$(TARG) xxx.kvo

clean:
	rm -f $(TARG) $(OBJS)

$(TARG): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARG)
