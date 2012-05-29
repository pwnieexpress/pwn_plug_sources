/*********************************************************************
 *
 * Filename:        cobex_socket.c
 * Original author: Pontus Fuchs <pontus.fuchs@tactel.se>
 *
 *     Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.
 *
 *     This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU Lesser General Public
 *     License as published by the Free Software Foundation; either
 *     version 2 of the License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *     Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public
 *     License along with this library; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *     MA  02111-1307  USA
 *
 ********************************************************************/

/*
 * $Id: obex_socket.c,v 1.6 2002/05/15 13:08:06 kds Exp $
 *
 * OBEX serial port transport functions.
 *
 * Adapted for Affix, Fixes:       Imre Deak <ext-imre.deak@nokia.com>
 * Adapted for Bluez, Fixes marked [REV]:  reverend@unrooted.net
 * Heavily modified for RFCOMM support: Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <openobex/obex.h>

#include "obex_macros.h"
#include "obex_main.h"
#include "obex_socket.h"
#include "obex_sdp.h"


struct cobex_context {
	bdaddr_t btaddr;
	char *addr;
	int devid, chan;
	const char *portname;
	int timeout;
	int rfd, wfd;
	char inputbuf[4096];
	struct termios oldtio, newtio;
};

static void cobex_cleanup(struct cobex_context *gt, int force);
static char *bt_sock_cachefile(char *cfname, int len);
static int bt_sock_cachefile_lookup(FILE * file, const char *btname, bdaddr_t * btaddr);
static int bt_sock_cache_lookup(const char *btname, bdaddr_t * btaddr);
static int bt_sock_cache_add(FILE * file, const char *btname, bdaddr_t const *btaddr);
static char *bt_sock_addr2str(bdaddr_t const *btaddr, char *straddr);
static int bt_sock_str2addr(const char *straddr, bdaddr_t * btaddr);
static int bt_sock_name2bth(int devid, const char *btname, bdaddr_t * btaddr);

static char *bt_sock_cachefile(char *cfname, int len)
{
	char const *env = getenv("HOME");

	if (env)
		snprintf(cfname, len, "%s/.bt-namecache", env);
	else
		snprintf(cfname, len, ".bt-namecache");

	return cfname;
}

static int bt_sock_cachefile_lookup(FILE * file, const char *btname, bdaddr_t * btaddr)
{
	char *name, *addr;
	char buf[512];

	rewind(file);
	while (fgets(buf, sizeof(buf) - 1, file) != NULL) {
		buf[strlen(buf) - 1] = 0;
		name = buf;
		if ((addr = strrchr(name, '\t')) == NULL)
			continue;
		*addr++ = '\0';
		if (bt_sock_str2addr(addr, btaddr) < 0)
			continue;
		if (strcasecmp(name, btname) == 0)
			return 0;
	}

	return -1;
}

static int bt_sock_cache_lookup(const char *btname, bdaddr_t * btaddr)
{
	int err;
	FILE *file;
	char cfname[256];

	bt_sock_cachefile(cfname, sizeof(cfname) - 1);
	if ((file = fopen(cfname, "rt")) == NULL)
		return -1;
	err = bt_sock_cachefile_lookup(file, btname, btaddr);
	fclose(file);

	return err;
}

static int bt_sock_cache_add(FILE * file, const char *btname, bdaddr_t const *btaddr)
{
	bdaddr_t laddr;
	char addr[128];

	if (bt_sock_cachefile_lookup(file, btname, &laddr) < 0) {
		fseek(file, 0, SEEK_END);
		fprintf(file, "%s\t%s\n", btname, bt_sock_addr2str(btaddr, addr));
	}

	return 0;
}

static char *bt_sock_addr2str(bdaddr_t const *btaddr, char *straddr)
{
	int i;

	for (i = 0; i < 6; i++) {
		if (i)
			sprintf(straddr + 2 + (i - 1) * 3, ":%02X", btaddr->b[5 - i]);
		else
			sprintf(straddr, "%02X", btaddr->b[5]);
	}

	return straddr;
}

static int bt_sock_str2addr(const char *straddr, bdaddr_t * btaddr)
{
	int i;
	unsigned int aaddr[6];

	if (sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &aaddr[0], &aaddr[1], &aaddr[2], &aaddr[3], &aaddr[4], &aaddr[5]) != 6)
		return -1;
	for (i = 0; i < 6; i++)
		btaddr->b[5 - i] = (unsigned char) (aaddr[i] & 0xff);

	return 0;
}

static int bt_sock_name2bth(int devid, const char *btname, bdaddr_t * btaddr)
{
	int i, niinf, dd, err = -1;
	inquiry_info *piinf = NULL;
	FILE *file;
	char cfname[256];

	if (bt_sock_cache_lookup(btname, btaddr) == 0)
		return 0;
	bt_sock_cachefile(cfname, sizeof(cfname) - 1);
	if ((file = fopen(cfname, "r+t")) == NULL && (file = fopen(cfname, "w+t")) == NULL)
		return -1;
	fprintf(stderr, "Resolving name '%s' ...\n", btname);
	if ((niinf = hci_inquiry(devid, 32, -1, NULL, &piinf, 0)) < 0) {
		fprintf(stderr, "hci_inquiry error\n");
		fclose(file);
		return -1;
	}
	if ((dd = hci_open_dev(devid)) < 0) {
		fprintf(stderr, "Unable to open HCI device hci%d\n", devid);
		free(piinf);
		fclose(file);
		return -1;
	}
	for (i = 0; i < niinf; i++) {
		char devname[128];

		if (hci_read_remote_name(dd, &piinf[i].bdaddr, sizeof(devname) - 1,
				    devname, 100000) >= 0) {
			if (strcasecmp(devname, btname) == 0) {
				*btaddr = piinf[i].bdaddr;
				err = 0;
			}
			bt_sock_cache_add(file, devname, &piinf[i].bdaddr);
		}
	}
	hci_close_dev(dd);
	free(piinf);
	fclose(file);

	return err;
}

int bt_sockndelay(int sfd, int on)
{

	return on ? fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK):
	fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) & ~O_NONBLOCK);
}

int bt_sock_open(int devid, bdaddr_t const *btaddr, int channel, int timeout)
{
	int sock;
	struct sockaddr_rc laddr, raddr;
	struct hci_dev_info di;
	fd_set wfds;
	struct timeval tv;
	char straddr[64], rstraddr[64];

	if (hci_devinfo(devid, &di) < 0) {
		perror("hci_devinfo");
		return -1;
	}
	laddr.rc_family = AF_BLUETOOTH;
	laddr.rc_bdaddr = di.bdaddr;
	laddr.rc_channel = 0;
	raddr.rc_family = AF_BLUETOOTH;
	raddr.rc_bdaddr = *btaddr;
	raddr.rc_channel = channel;
	if ((sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror("socket");
		return -1;
	}
	if (bind(sock, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
		perror("bind");
		close(sock);
		return -1;
	}
	printf("Local device %s\n", bt_sock_addr2str(&di.bdaddr, straddr));
	printf("Remote device %s (%d)\n", bt_sock_addr2str(btaddr, rstraddr), channel);
	bt_sockndelay(sock, 1);
	if (connect(sock, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
		if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
			perror("connect");
			close(sock);
			return -1;
		}
		FD_ZERO(&wfds);
		FD_SET(sock, &wfds);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if (select(sock + 1, (fd_set *) 0, &wfds, (fd_set *) 0, &tv) == -1) {
			perror("select");
			close(sock);
			return -1;
		}
		if (!FD_ISSET(sock, &wfds)) {
			printf("connect timeout\n");
			close(sock);
			return -1;
		}
	}
	bt_sockndelay(sock, 0);

	return sock;
}

static int cobex_init(struct cobex_context *gt)
{

	if (gt->rfd == -1) {
		if (gt->addr != NULL) {
			if ((gt->rfd = bt_sock_open(gt->devid, &gt->btaddr, gt->chan,
						    gt->timeout)) < 0) {

				return -1;
			}
			gt->wfd = gt->rfd;

		} else if (gt->portname != NULL) {
			if ((gt->rfd = open(gt->portname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0) {
				perror("Can' t open tty");
				return -1;
			}
			gt->wfd = gt->rfd;

			tcgetattr(gt->rfd, &gt->oldtio);
			bzero(&gt->newtio, sizeof(struct termios));
			gt->newtio.c_cflag = B115200 | CS8 | CREAD;
			gt->newtio.c_iflag = IGNPAR;
			gt->newtio.c_oflag = 0;
			tcflush(gt->rfd, TCIFLUSH);
			tcsetattr(gt->rfd, TCSANOW, &gt->newtio);
		} else {
			gt->rfd = 0;
			gt->wfd = 1;
		}
	}

	return 1;
}

static void cobex_cleanup(struct cobex_context *gt, int force)
{

	if (gt->portname != NULL) {
		if (force && gt->rfd != -1) {
			/* Send a break to get out of OBEX-mode */
			if (ioctl(gt->rfd, TCSBRKP, 0) < 0)
				BTERROR("Unable to send break!\n");
		}
	}
	if (gt->rfd != -1)
		close(gt->rfd);
	if (gt->wfd != -1 && gt->wfd != gt->rfd)
		close(gt->wfd);
	gt->rfd = gt->wfd = -1;
}

struct cobex_context *cobex_open(int devid, const char *port, int timeout)
{
	int n;
	struct cobex_context *gt;
	const char *chan;
	unsigned int channs[4];

	gt = malloc(sizeof(struct cobex_context));
	if (gt == NULL)
		return NULL;

	memset(gt, 0, sizeof(*gt));
	gt->rfd = gt->wfd = -1;
	gt->portname = port;
	gt->addr = NULL;
	gt->chan = -1;
	gt->timeout = timeout;
	gt->devid = devid;
	if ((chan = strchr((const char *) port, '@')) != NULL) {
		gt->addr = strdup((const char *) port);
		gt->addr[chan - (const char *) port] = 0;
		if (bt_sock_str2addr(gt->addr, &gt->btaddr) < 0 &&
		    bt_sock_name2bth(devid, gt->addr, &gt->btaddr) < 0) {
			fprintf(stderr, "Unable to resolve '%s'\n", gt->addr);
			cobex_cleanup(gt, 0);
			return NULL;
		}
		if (strlen(chan + 1) > 0)
			gt->chan = atoi(chan + 1);
		else {
			if ((n = bt_service_channel(devid, &gt->btaddr, OBEX_UUID, channs,
						    sizeof(channs) / sizeof(channs[0]))) <= 0) {
				fprintf(stderr, "Unable to find OBEX service in '%s'\n", gt->addr);
				cobex_cleanup(gt, 0);
				return NULL;
			}
			gt->chan = (int) channs[0];
			BTDEBUG("Remote OBEX channel: %d\n", gt->chan);
		}
	}

	return gt;
}

struct cobex_context *cobex_setsocket(const int fd)
{
	struct cobex_context *gt;

	gt = malloc(sizeof(struct cobex_context));
	if (gt == NULL)
		return NULL;

	gt->rfd = gt->wfd = fd;
	gt->portname = NULL;
	return gt;
}

int cobex_getsocket(struct cobex_context *gt)
{
	return gt->rfd;
}

void cobex_close(struct cobex_context *gt)
{
	BTDEBUG("cobex_close\n");
	free(gt->addr);
	free(gt);
}

int cobex_listen(obex_t * handle, void *userdata)
{
	struct cobex_context *gt;

	BTDEBUG("cobex_listen\n");
	gt = userdata;
	if (gt->rfd >= 0) {
		BTDEBUG("fd already exist. Using it\n");
		return 1;

	}
	if (cobex_init(gt) < 0)
		return 0;
	return 1;
}

int cobex_connect(obex_t * handle, void *userdata)
{
	struct cobex_context *gt;

	BTDEBUG("cobex_connect\n");
	gt = userdata;
	if (gt->rfd >= 0) {
		BTDEBUG("fd already exist. Using it\n");
		return 1;

	}
	if (cobex_init(gt) < 0)
		return 0;
	return 1;
}

int cobex_disconnect(obex_t * handle, void *userdata)
{
	struct cobex_context *gt;

	BTDEBUG("cobex_disconnect\n");
	gt = userdata;
	cobex_cleanup(gt, 0);
	return 1;
}

int cobex_write(obex_t * handle, void *userdata, uint8_t * buffer, int length)
{
	struct cobex_context *gt;
	int err, actual = 0, pos = 0;

	BTDEBUG("cobex_write\n");
	gt = userdata;

	/*
	 * [REV]
	 * For some reason the init function wasn't getting called before this call
	 * to write.  Probably my own fault, something I ripped out must have in
	 * the initial cobex_connect() callback from OpenOBEX.  Fortunately for me
	 * it works fine if I just call the init from the first write.  The init
	 * sets the wfd field so that this call gets skipped on all writes after
	 * the first.
	 */
	if (gt->wfd < 0) {
		if ((err = cobex_init(gt)) < 0) {
			BTDEBUG("cobex_init returned %d\n", err);
			return 0;
		}
	}

	while (actual < length) {
		int frag;
		frag = write(gt->wfd, &buffer[pos], length - pos);
		if (frag < 0) {
			if (errno == EAGAIN) {
				frag = 0;
				BTDEBUG("Temp. no resource avail.\n");
			} else {
				BTERROR("Write error: %s\n", strerror(errno));
				return frag;
			}
		}
		actual += frag;
		pos += frag;
		BTDEBUG("Wrote %d fragment\n", frag);
	}
	BTDEBUG("Wrote %d bytes (expected %d)\n", actual, length);
	return actual;
}

int cobex_handle_input(obex_t * handle, void *userdata, int timeout)
{
	int actual;
	struct cobex_context *gt;
	struct timeval time;
	fd_set fdset;
	int ret;
	int16_t expectlen;

	BTDEBUG("cobex_handle_input\n");
	gt = userdata;

	/* Return if no fd */
	if (gt->rfd < 0)
		return -1;

	time.tv_sec = timeout;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(gt->rfd, &fdset);

	actual = 0;
	expectlen = 0;
	do {
		int len;
		fd_set fdset1 = fdset;

		ret = select(gt->rfd + 1, &fdset1, NULL, NULL, &time);

		/* Check if this is a timeout (0) or error (-1) */
		if (ret < 0) {
			BTERROR("Error (%d)\n", ret);
			return -1;
		}
		if (ret == 0)
			len = 0;
		else {
			len = read(gt->rfd, &gt->inputbuf[actual], sizeof(gt->inputbuf) - actual);
			if (len < 0) {
				if (errno == -EAGAIN) {
					BTDEBUG("Temp. no resource available.\n");
					len = 0;
				} else {
					BTDEBUG("read error (%d)\n", len);
					return len;
				}
			} else if (len == 0) {
				BTDEBUG("Connection terminated.\n");
				return -1;
			}
		}
		actual += len;
		if ((expectlen == 0) && (actual >= 3)) {
			memcpy(&expectlen, gt->inputbuf + 1, 2);
			expectlen = ntohs(expectlen);
			BTDEBUG("expect %d bytes\n", expectlen);
			if (expectlen > sizeof(gt->inputbuf)) {
				BTDEBUG("cobex_context buffer size is too small!!!\n");
				exit(1);
			}
		}
	} while ((actual < expectlen) || (expectlen == 0));
	BTDEBUG("Read %d bytes\n", actual);
	OBEX_CustomDataFeed(handle, (uint8_t *) gt->inputbuf, actual);

	return actual;
}

