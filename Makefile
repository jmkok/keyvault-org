# Needed libraries
# - libglib2.0-dev
# - libgtk2.0-dev
# - libxml2-dev
# - libssh2-1-dev
# Documentation
# - http://library.gnome.org/devel/gtk/stable/
# - http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html

TARG=keyvault
SRCS=main.c \
	gtk_ui.c gtk_treeview.c gtk_dialogs.c gtk_shortcuts.c \
	configuration.c ssh.c encryption.c functions.c list.c xml.c
OBJS=$(SRCS:.c=.o)
LIBRARIES=glib-2.0 gtk+-2.0 libxml-2.0 openssl libssh2
#~ CROSS=-m32
CFLAGS=$(CROSS) `pkg-config --cflags $(LIBRARIES)` -Wall
CFLAGS+=-Wextra -Werror -Wshadow -Wcast-qual -Wwrite-strings
#~ CFLAGS+=-pedantic
LIBS=$(CROSS) `pkg-config --libs $(LIBRARIES)`

all: $(TARG)

run: $(TARG)
	LANG=C ./$(TARG)

clean:
	rm -f $(TARG) $(OBJS)

$(TARG): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARG)
