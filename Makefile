TARG=keyvault
SRCS=main.c configuration.c xml.c treeview.c dialogs.c ssh.c encryption.c gtk_shortcuts.c functions.c
OBJS=$(SRCS:.c=.o)
LIBS=glib-2.0 gtk+-2.0 openssl mxml
CFLAGS=`pkg-config $(LIBS) --cflags` -Wall -Werror
CLIBS=`pkg-config $(LIBS) --libs` -lssh2

.cc.o:

$(TARG): $(OBJS)
	$(CC) $(CLIBS) $(OBJS) -o $(TARG)

all: $(TARG)

run: $(TARG)
	./$(TARG)

clean:
	rm -f $(TARG) $(OBJS)
