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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>
#include <openobex/obex.h>

#include "obexsrv.h"

extern server_context context;

extern void debug(char*);
extern void debug2(char*, void*);

/*
 * mostly  taken from sdptool of bluez-utils (www.bluez.org)
 */
int add_opush()
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, opush_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	uint8_t chan = context.channel;
	sdp_data_t *channel;
	uint8_t formats[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xFF };
	void *dtds[sizeof(formats)], *values[sizeof(formats)];
	int i;
	uint8_t dtd = SDP_UINT8;
	sdp_data_t *sflist;
	sdp_session_t *session;


	if (!context.sdp_session) {
		debug2("%s: sdp_session invalid\n", (char*)__func__);
		exit(-1);
	}
	session = context.sdp_session;

	context.sdp_record[0] = sdp_record_alloc();
	if (!context.sdp_record[0]) {
		perror("add_opush sdp_record_alloc: ");
		exit(-1);
	}

	memset((void *)context.sdp_record[0], 0, sizeof(sdp_record_t));
	context.sdp_record[0]->handle = 0xffffffff;
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(context.sdp_record[0], root);

	sdp_uuid16_create(&opush_uuid, OBEX_OBJPUSH_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &opush_uuid);
	sdp_set_service_classes(context.sdp_record[0], svclass_id);

	sdp_uuid16_create(&profile[0].uuid, OBEX_OBJPUSH_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, profile);
	sdp_set_profile_descs(context.sdp_record[0], pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &chan);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto[2] = sdp_list_append(0, &obex_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(context.sdp_record[0], aproto);

	for (i = 0; i < sizeof(formats); i++) {
		dtds[i] = &dtd;
		values[i] = &formats[i];
	}
	sflist = sdp_seq_alloc(dtds, values, sizeof(formats));
	sdp_attr_add(context.sdp_record[0], SDP_ATTR_SUPPORTED_FORMATS_LIST, sflist);

	sdp_set_info_attr(context.sdp_record[0], "OBEX Object Push", 0, 0);

	if (sdp_record_register(session, context.sdp_record[0], 0) < 0) {
		debug2("%s: OBEX Object Push Service Record registration failed\n", (char*)__func__);
		exit(-1);
	}
	
	return(1);
}

/*
 *  taken from sdptool of bluez-utils (www.bluez.org)
 */
int add_ftp()
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, ftrn_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	uint8_t u8 = context.channel;
	sdp_data_t *channel;
	sdp_session_t *session;


	if (!context.sdp_session) {
		debug2("%s: sdp_session invalid\n", (char*)__func__);
		exit(-1);
	}
	session = context.sdp_session;

	context.sdp_record[1] = sdp_record_alloc();
	if (!context.sdp_record[1]) {
		perror("add_ftp sdp_record_alloc: ");
		exit(-1);
	}

	memset((void *)context.sdp_record[1], 0, sizeof(sdp_record_t));
	context.sdp_record[1]->handle = 0xffffffff;
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(context.sdp_record[1], root);

	sdp_uuid16_create(&ftrn_uuid, OBEX_FILETRANS_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &ftrn_uuid);
	sdp_set_service_classes(context.sdp_record[1], svclass_id);

	sdp_uuid16_create(&profile[0].uuid, OBEX_FILETRANS_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(context.sdp_record[1], pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &u8);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	sdp_uuid16_create(&obex_uuid, OBEX_UUID);
	proto[2] = sdp_list_append(0, &obex_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(context.sdp_record[1], aproto);

	sdp_set_info_attr(context.sdp_record[1], "OBEX File Transfer", 0, 0);

	if (sdp_record_register(session, context.sdp_record[1], 0) < 0) {
		debug2("%s: OBEX File Transfer Service Record registration failed\n", (char*)__func__);
		exit(-1);
	}
	
	return(1);
}

void remove_sdp()
{
	if (context.sdp_ftp && sdp_record_unregister(context.sdp_session, context.sdp_record[1])) {
		debug2("%s: OBEX File Transfer Service record unregistration failed.\n", (char*)__func__);
	}
	if (context.sdp_opush && sdp_record_unregister(context.sdp_session, context.sdp_record[0])) {
		debug2("%s: OBEX Object Push Service record unregistration failed.\n", (char*)__func__);
	}
	
	sdp_close(context.sdp_session);
}

void open_sdp()
{
	if (!context.sdp_session) {
		context.sdp_session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, 0);
	}
	if (!context.sdp_session) {
		debug2("%s: sdp_session invalid\n", (char*)__func__);
		exit(-1);
	}
}

/*
 *  $Log$
 */
