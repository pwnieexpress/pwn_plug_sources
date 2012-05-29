# App info
APPNAME = bluelog
VERSION = 1.0.2

# Bluelog-specific, select which CSS to use as default
DEFAULT_CSS = digifail.css

# Debug, build as if on OpenWRT
#TARGET = -DOPENWRT

# Compiler and options
CC = gcc
CFLAGS += -Wall -O2

# Libraries to link
LIBS = -lbluetooth

# Files
SOURCES = bluelog.c
DOCS = ChangeLog COPYING README README.LIVE README.WRT FUTURE

# Targets
# Build
$(APPNAME): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(LIBS) $(TARGET) -o $(APPNAME)

# Build tarball
release: clean
	tar -C ../ -czvf /tmp/$(APPNAME)-$(VERSION).tar.gz $(APPNAME)-$(VERSION)

# Clean for dist
clean:
	rm -rf $(APPNAME) *.o *.txt *.log *.gz

# Install to system
install: $(APPNAME)
	mkdir -p $(DESTDIR)/usr/bin/
	mkdir -p $(DESTDIR)/usr/share/doc/$(APPNAME)-$(VERSION)/
	mkdir -p $(DESTDIR)/usr/share/man/man1
	mkdir -p $(DESTDIR)/var/lib/$(APPNAME)/
	cp $(APPNAME) $(DESTDIR)/usr/bin/
	cp -a $(DOCS) $(DESTDIR)/usr/share/doc/$(APPNAME)-$(VERSION)/
	gzip -c $(APPNAME).1 >> $(APPNAME).1.gz
	cp $(APPNAME).1.gz $(DESTDIR)/usr/share/man/man1/
	cp -a --no-preserve=ownership www/* $(DESTDIR)/var/lib/$(APPNAME)/
	cd $(DESTDIR)/var/lib/$(APPNAME)/ ; ln -sf $(DEFAULT_CSS) style.css

# Upgrade from previous source install
upgrade: removeold install

# Remove current version from system
uninstall:
	rm -rf $(DESTDIR)/usr/share/doc/$(APPNAME)-$(VERSION)/
	rm -f $(DESTDIR)/usr/share/man/man1/$(APPNAME).1.gz
	rm -rf $(DESTDIR)/var/lib/$(APPNAME)/
	rm -f $(DESTDIR)/usr/bin/$(APPNAME)

# Remove older versions
removeold:
	rm -rf $(DESTDIR)/usr/share/doc/$(APPNAME)*
	rm -f $(DESTDIR)/usr/share/man/man1/$(APPNAME).1.gz
	rm -rf $(DESTDIR)/var/lib/$(APPNAME)*
	rm -f $(DESTDIR)/usr/bin/$(APPNAME)
