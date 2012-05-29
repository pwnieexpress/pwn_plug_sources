# Makefile for pwntooth

CC = gcc
BT_LIB = -lbluetooth
BIN = /bin

all: pwntooth 

pwntooth:
	$(CC) $(BT_LIB) pwntooth.c -o pwntooth 

install:
	cp pwntooth	/usr/bin/
	cp pwntooth.conf /etc/bluetooth
clean:
	rm pwntooth 
