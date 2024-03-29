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
	configuration.c ssh.c encryption.c functions.c list.c xml.c \
	file_location.c
OBJS=$(SRCS:.c=.o)
LIBRARIES=glib-2.0 gtk+-2.0 libxml-2.0 openssl libssh2
#~ CROSS=-m32
CFLAGS=$(CROSS) `pkg-config --cflags $(LIBRARIES)` -Wall
CFLAGS += -Wextra -Werror -Wshadow -Wwrite-strings
#~ CFLAGS += -Wcast-qual - this gives an error in "gthread.h" on an 64 bit platform
#~ CFLAGS += -pedantic
LIBS=$(CROSS) `pkg-config --libs $(LIBRARIES)`

# Package setup
DPKG_PATH = dpkg
VERSION = $(shell date +'%Y-%m-%d')
ARCH = $(shell uname -i)

all: $(TARG)

run: $(TARG)
	./$(TARG)

clean:
	rm -f $(TARG) $(OBJS) $(TARG)*.deb
	rm -rf $(DPKG_PATH)

$(TARG): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARG)

install: $(TARG)
	install -s $(TARG) /usr/local/bin/
	cp keyvault-org.desktop /usr/share/applications/

$(TARG).deb: $(TARG)
	@mkdir -p $(DPKG_PATH)/DEBIAN
	@mkdir -p $(DPKG_PATH)/usr/local/bin/
	@mkdir -p $(DPKG_PATH)/usr/share/applications/
	cp dpkg.control $(DPKG_PATH)/DEBIAN/control
	install -s $(TARG) $(DPKG_PATH)/usr/local/bin/
	cp keyvault-org.desktop $(DPKG_PATH)/usr/share/applications/
	@sed -i 's/_VERSION_/$(VERSION)/' $(DPKG_PATH)/DEBIAN/control
	@sed -i 's/_ARCH_/$(ARCH)/' $(DPKG_PATH)/DEBIAN/control
	@fakeroot -- dpkg-deb --build $(DPKG_PATH) $(TARG)_$(VERSION)_$(ARCH).deb
