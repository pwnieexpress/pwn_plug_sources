/*
 * UNrooted.net example code
 *
 * Most of these functions are just rips from the Affix Bluetooth project OBEX
 * programs.  There are comments in the code about where the functions came
 * from and what if anything was changed.
 *
 * Throughout these functions I had to change the BTDEBUG and BTERROR macros to
 * printfs.  I could have just defined BTERROR and BTDEBUG, but I didn't for
 * some reason.. who knows.
 *
 * The original header comment specifying the copyright holder and the
 * requirements of the GPL are pasted below.
 */

/*
 * Original comment header block:
 * ----------------------------------------------------------------------
 *
 * Affix - Bluetooth Protocol Stack for Linux
 * Copyright (C) 2001 Nokia Corporation
 * Original Author: Dmitry Kasatkin <dmitry.kasatkin@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id: obex_client.c,v 1.22 2002/06/25 10:16:49 kds Exp $
 *
 * OBEX client lib
 *
 * Fixes:	Dmitry Kasatkin <dmitry.kasatkin@nokia.com>
 *
 * Heavily modified: Davide Libenzi <davidel@xmailserver.org>
 *
 * ----------------------------------------------------------------------
 * End original comment block
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <openobex/obex.h>

#include "obex_macros.h"
#include "obex_main.h"
#include "obex_socket.h"

#define UPUSH_APPNAME "ussp-push v0.11"
#define BT_SERVICE "OBEX"
#define OBEX_PUSH        5


/*
 * prototypes for local functions
 */

obex_t *__obex_connect(int devid, void *addr, int timeout, int *err);
void obex_event(obex_t * handle, obex_object_t * object, int mode, int event,
		int obex_cmd, int obex_rsp);
void obex_free(client_context_t *gt);
int obex_disconnect(obex_t * handle);
uint8_t *easy_readfile(const char *filename, int *file_size);
int get_filesize(const char *filename);
void request_done(obex_t * handle, obex_object_t * object, int obex_cmd, int obex_rsp);



int bt_debug;



/*
 * These two functions are from affix/profiles/obex/obex_io.c
 */
int get_filesize(const char *filename)
{
	struct stat stats;

	stat(filename, &stats);
	return (int) stats.st_size;
}

uint8_t *easy_readfile(const char *filename, int *file_size)
{
	int actual;
	int fd;
	uint8_t *buf;

	fd = open(filename, O_RDONLY, 0);
	if (fd == -1) {
		return NULL;
	}
	*file_size = get_filesize(filename);
	printf("name=%s, size=%d\n", filename, *file_size);
	if (!(buf = malloc(*file_size))) {
		return NULL;
	}

	actual = read(fd, buf, *file_size);
	close(fd);

	*file_size = actual;
	return buf;
}

/*
 * This function comes from affix/profiles/obex/obex_client.c .. All I changed
 * was a BTERROR() macro.  The OBEX_HandleInput() calls inside the loop should
 * result in request_done() getting called eventually.  request_done() sets
 * the clientdone flag which releases the function from its loop
 */
int handle_response(obex_t * handle, char *service)
{
	int err = 0;
	client_context_t *gt = OBEX_GetUserData(handle);

	gt->clientdone = 0;
	while (!gt->clientdone) {
		if ((err = OBEX_HandleInput(handle, 1)) < 0) {
			BTERROR("Error while doing OBEX_HandleInput()\n");
			break;
		}
		err = (gt->rsp == EOBEX_OK) ? 0 : gt->rsp;
	}

	return err;
}

void obex_free(client_context_t *gt)
{

	cobex_close(gt->private);
	free(gt);
}

/*
 * This function comes from affix/profiles/obex/obex_client.c ... I just
 * trimmed out handling of other transport styles.. that stuff was using some
 * globals and I didn't want to pull them in.
 */
int obex_disconnect(obex_t * handle)
{
	int err;
	obex_object_t *oo;
	client_context_t *gt = OBEX_GetUserData(handle);

	oo = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT);
	err = OBEX_Request(handle, oo);
	if (err)
		return err;
	handle_response(handle, BT_SERVICE);
	obex_free(gt);

	return 0;
}

/*
 * This function came from the affix/profiles/obex/obex_client.c file.
 * Initially it did some checks to see what type of link it was working over,
 * supporting a few different transports it looked like.  But the logic around
 * choosing what to do relied on a couple of global variables, so I just pulled
 * the decision paths I didn't want and left the ones I did.
 */
obex_t *__obex_connect(int devid, void *addr, int timeout, int *err)
{
	obex_t *handle;
	obex_object_t *oo;
	client_context_t *gt;
	obex_ctrans_t custfunc;

	gt = malloc(sizeof(client_context_t));
	if (gt == NULL)
		return NULL;
	BTDEBUG("__obex_connect: client_context_t = %p\n", gt);
	memset(gt, 0, sizeof(client_context_t));
	gt->private = cobex_open(devid, addr, timeout);
	if (gt->private == NULL) {
		BTERROR("cobex_open() failed\n");
		free(gt);
		*err = -1;
		return NULL;
	}
	if (!(handle = OBEX_Init(OBEX_TRANS_CUST, obex_event, 0))) {
		BTERROR("OBEX_Init failed: %s\n", strerror(errno));
		obex_free(gt);
		*err = -1;
		return NULL;
	}

	memset(&custfunc, 0, sizeof(custfunc));
	custfunc.customdata = gt->private;
	custfunc.connect = cobex_connect;
	custfunc.disconnect = cobex_disconnect;
	custfunc.write = cobex_write;
	custfunc.handleinput = cobex_handle_input;
	custfunc.listen = cobex_listen;
	if (OBEX_RegisterCTransport(handle, &custfunc) < 0) {
		BTERROR("Custom transport callback-registration failed\n");
		obex_free(gt);
		*err = -1;
		return NULL;
	}
	BTDEBUG("Registered transport\n");
	OBEX_SetUserData(handle, gt);
	BTDEBUG("Set user data\n");

	/* create new object */
	oo = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT);
	BTDEBUG("Created new objext\n");
	*err = OBEX_Request(handle, oo);
	BTDEBUG("Started a new request\n");
	if (*err || gt->rsp) {
		*err = *err ? *err: gt->rsp;
		obex_free(gt);
		goto exit;
	}
	*err = handle_response(handle, BT_SERVICE);
	BTDEBUG("Connection return code: %d, id: %d\n", *err, gt->con_id);
	if (*err || gt->rsp) {
		*err = *err ? *err: gt->rsp;
		goto exit_conn;
	}
	printf("Connection established\n");
	return handle;
	exit_conn:
	obex_disconnect(handle);
	exit:
	BTERROR("__obex_connect: error=%d\n", *err);
	return NULL;
}

/*
 * These next two functions come from affix/profiles/obex/obex_client.c
 * All they do are set a few flags in the structs here or disconnect on error.
 * The obex_event() function is called by the obex library when it has an event
 * to deliver to us.. as simple as this is this means just setting that
 * clientdone flag in the user data struct.  Yea... really, that's it.
 */
void request_done(obex_t * handle, obex_object_t * object, int obex_cmd, int obex_rsp)
{
	client_context_t *gt = OBEX_GetUserData(handle);

	BTDEBUG("Command (%02x) has now finished, rsp: %02x\n", obex_cmd, obex_rsp);

	switch (obex_cmd) {
	case OBEX_CMD_DISCONNECT:
		BTDEBUG("Disconnect done!\n");
		OBEX_TransportDisconnect(handle);
		gt->clientdone = 1;
		break;
	case OBEX_CMD_CONNECT:
		BTDEBUG("Connected!\n");
		/* connect_client(handle, object, obex_rsp); */
		gt->clientdone = 1;
		break;
	case OBEX_CMD_GET:
		BTDEBUG("\n\n*** Warning, getclient commented out\n");
		/* get_client(handle, object, obex_rsp); */
		gt->clientdone = 1;
		break;
	case OBEX_CMD_PUT:
		/* put_client(handle, object, obex_rsp); */
		gt->clientdone = 1;
		break;
	case OBEX_CMD_SETPATH:
		BTDEBUG("\n\n*** Warning, setpath_cleitn commented out\n");
		/* setpath_client(handle, object, obex_rsp); */
		gt->clientdone = 1;
		break;
	default:
		BTDEBUG("Command (%02x) has now finished\n", obex_cmd);
		break;
	}
}

void obex_event(obex_t * handle, obex_object_t * object, int mode, int event,
		int obex_cmd, int obex_rsp)
{
	client_context_t *gt;

	gt = OBEX_GetUserData(handle);
	BTDEBUG("obex_event: client_context_t = %p\n", gt);
	switch (event) {
	case OBEX_EV_PROGRESS:
		BTDEBUG("Made some progress...\n");
		break;
	case OBEX_EV_ABORT:
		BTDEBUG("Request aborted!\n");
		gt->rsp = -EOBEX_ABORT;
		break;
	case OBEX_EV_REQDONE:
		BTDEBUG("ReqDone\n");
		request_done(handle, object, obex_cmd, obex_rsp);
		/* server_done(handle, object, obex_cmd, obex_rsp); */
		break;
	case OBEX_EV_REQHINT:
		/* Accept any command. Not rellay good, but this is a test-program :
		 * ) */
		/* OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS); */
		BTDEBUG("SetResp\n");
		break;
	case OBEX_EV_REQ:
		/* server_request(handle, object, event, obex_cmd); */
		BTDEBUG("Server request\n");
		break;
	case OBEX_EV_LINKERR:
		OBEX_TransportDisconnect(handle);
		BTDEBUG("Link broken!\n");
		gt->rsp = -EOBEX_HUP;
		break;
	case OBEX_EV_PARSEERR:
		BTDEBUG("Parse error!\n");
		gt->rsp = -EOBEX_PARSE;
		break;
	default:
		BTDEBUG("Unknown event %02x!\n", event);
		break;
	}
}

/*
 * This function also comes directly from affix/profiles/obex/obex_client.c
 * It calls __obex_connect() to connect to the device, and then uses the
 * OpenOBEX call interface to form a PUT request to send the file to the
 * device. Originally this used the basename() of path as the name to put as,
 * but I added a remote parameter so that I didn't have to rename files before
 * moving them over.  Essentially nothing is different.
 */
int obex_push(int devid, void *addr, char *path, char *remote, int timeout)
{
	int err;
	obex_object_t *oo;
	obex_headerdata_t hv;
	obex_t *handle;
	uint8_t *buf;
	int file_size;
	char *namebuf;
	int name_len;
	char *name, *pathc;
	client_context_t *gt;

	pathc = strdup(path);
	name = remote;

	name_len = (strlen(name) + 1) << 1;
	if ((namebuf = malloc(name_len))) {
		OBEX_CharToUnicode((uint8_t *) namebuf, (uint8_t *) name, name_len);
	}
	buf = easy_readfile(path, &file_size);
	if (buf == NULL) {
		BTERROR("Can't find file %s\n", name);
		return -ENOENT;
	}

	handle = __obex_connect(devid, addr, timeout, &err);
	if (handle == NULL) {
		BTERROR("Unable to connect to the server\n");
		free(buf);
		return err;
	}
	BTDEBUG("Connected to server\n");
	gt = OBEX_GetUserData(handle);
	BTDEBUG("obex_push: client_context_t = %p\n", gt);
	gt->opcode = OBEX_PUSH;

	BTDEBUG("Sending file: %s, path: %s, size: %d\n", name, path, file_size);
	oo = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	hv.bs = (uint8_t *) namebuf;
	OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_NAME, hv, name_len, 0);
	hv.bq4 = file_size;
	OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);
	hv.bs = buf;
	OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_BODY, hv, file_size, 0);

	err = OBEX_Request(handle, oo);
	if (err)
		return err;
	err = handle_response(handle, BT_SERVICE);

	obex_disconnect(handle);
	free(buf);
	free(pathc);

	return err;
}

int main(int argc, char **argv)
{
	int i, devid = 0, timeout = 60;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--dev") == 0) {
			if (++i < argc)
				devid = atoi(argv[i]);
		} else if (strcmp(argv[i], "--timeo") == 0) {
			if (++i < argc)
				timeout = atoi(argv[i]);
		} else if (strcmp(argv[i], "--debug") == 0) {
			bt_debug = 1;
		} else
			break;
	}
	if ((argc - i) != 3) {
		BTERROR("%s\n\n"
			"Usage: %s [--dev DEVID] [--timeo TIMEO] [--debug] {DEVICE, BTADDR@[BTCHAN]} LFILE RFILE\n\n"
			"\tDEVICE          = RFCOMM TTY device file\n"
			"\tTIMEO           = RFCOMM connect timeout in seconds\n"
			"\tBTADDR@[BTCHAN] = BlueTooth address/name and OBEX channel. If the OBEX channel\n"
			"\t                  is not specified, and SDP quesry is used to retrieve it\n"
			"\tLFILE           = Local file path\n"
			"\tRFILE           = Remote file name\n\n", UPUSH_APPNAME, argv[0]);
		return 1;
	}
	BTDEBUG("pushing file %s\n", argv[i + 1]);
	if (obex_push(devid, argv[i], argv[i + 1], argv[i + 2], timeout) != 0) {
		BTERROR("Error\n");
		return 2;
	}
	BTDEBUG("Pushed!!\n");

	return 0;
}

