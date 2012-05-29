/*
 *  $Id$
 */
 
/*
 *  sobexsrv (c) Collin R. Mulliner <collin(AT)betaversion.net>
 *               web: http://www.mulliner.org/bluetooth/sobexsrv.php
 *
 *  license: GPLv2
 *
 */
 
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>

/**
 *  @brief write n bytes to socket
 *
 *  this is taken from: unix network programming by W.R. Stevens
 *  
 *  @param fd   file descriptor
 *  @param vptr bytes to write out
 *  @param n    number of bytes to write
 *
 *  @returns -1 on failure "n" on success
 */
ssize_t nwrite(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	
	
	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR) {
				nwritten = 0;
			}
			else {
				return(-1);
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return(n);
}

/**
 *  @brief read N bytes from file descriptor
 *
 *  @param fd   file descriptor
 *  @param vptr read buffer
 *  @param n    number of bytes to read
 *
 *  @returns number of bytes read or -1 on error
 */
ssize_t cread(int fd, void *vptr, size_t n)
{
	int result;
	int rd = 0;
	
reread:
	if ((result = read(fd, vptr+rd, n-rd)) < 0) {
		if (result == EINTR) {
			result = 0;
			goto reread;
		}
		else {
			return(-1);
		}
	}
	else {
		rd += result;
		if (rd < n) goto reread;
	}
	return(rd);
}

/*
 *  $Log$
 */
