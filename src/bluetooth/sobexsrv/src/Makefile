BINDIR = $(DESTDIR)/usr/bin
ETCINITDIR = $(DESTDIR)/etc/init.d
MANDIR = $(DESTDIR)/usr/man/man8

VERSION = `cat ../VERSION`
HANDLER = "\"$(DESTDIR)/usr/bin/sobexsrv_handler\""
CHROOT  = "\"/tmp\""
MIMEDIR = "\"/tmp/mime\""

INC =
LIB = -lbluetooth -lopenobex
#CFLAGS = -Wall -g -DVERSION_STRING=$(VERSION) -DHANDLER=$(HANDLER) -DSOBEXSRV_CHROOT=$(CHROOT)
CFLAGS = -Wall -DVERSION_STRING=\"$(VERSION)\" -DHANDLER=$(HANDLER) -DSOBEXSRV_CHROOT=$(CHROOT)
CC = gcc

OBJ = obexsrv.o util.o sdp.o fs.o pathcheck.o
EXEC = sobexsrv

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ) $(LIB)

clean:
	rm -f $(OBJ) $(EXEC)

#install: all
#	install -d $(BINDIR) $(ETCINITDIR)
#	install ./sobexsrv $(BINDIR)
#	install ../scripts/handler.py $(BINDIR)/sobexsrv_handler
#	install ../sobexsrv.8 $(MANDIR)
