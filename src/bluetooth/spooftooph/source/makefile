# Makefile for spooftooph

CC = gcc
BT_LIB = -lbluetooth
NCURSES_LIB = -lncurses
PTHREAD = -pthread
BIN = /usr/bin

all: spooftooph

spooftooph:
	$(CC) $(BT_LIB) $(NCURSES_LIB) $(PTHREAD) dev_class.c namelist.c spooftooph.c bdaddr.c oui.c -o spooftooph

install:
	mv spooftooph	$(BIN)

uninstall:

	rm -i $(BIN)/spooftooph
clean:
	rm spooftooph
