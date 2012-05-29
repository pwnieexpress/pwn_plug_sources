BINDIR = $(DESTDIR)/usr/bin
ETCINITDIR = $(DESTDIR)/etc/init.d
MANDIR = $(DESTDIR)/usr/man/man8

all:
	cd src; make

clean:
	cd src; make clean

install: all
	install -d $(BINDIR) $(ETCINITDIR) $(MANDIR)
	install ./src/sobexsrv $(BINDIR)
	install ./scripts/handler.py $(BINDIR)/sobexsrv_handler
	install ./sobexsrv.8 $(MANDIR)
