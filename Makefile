# Needed libraries
# - libglib2.0-dev
# - libgtk2.0-dev
# - libxml2-dev
# - libssh2-1-dev
# Documentation
# - http://library.gnome.org/devel/gtk/stable/

TARG=keyvault
SRCS=main.c ui.c configuration.c treeview.c dialogs.c ssh.c encryption.c gtk_shortcuts.c functions.c list.c xml.c
OBJS=$(SRCS:.c=.o)
LIBS=glib-2.0 gtk+-2.0 openssl libxml-2.0
CFLAGS=`pkg-config $(LIBS) --cflags` -Wall -Werror
CLIBS=`pkg-config $(LIBS) --libs` -lssh2

.cc.o:

all: $(TARG)
	#~ make run

run: $(TARG)
	LANG=C ./$(TARG)

clean:
	rm -f $(TARG) $(OBJS)

$(TARG): $(OBJS)
	$(CC) $(CLIBS) $(OBJS) -o $(TARG)
