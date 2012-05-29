/*
 *  ussp-push by Davide Libenzi ( OBEX file pusher )
 *  Copyright (C) 2006  Davide Libenzi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <netinet/in.h>

#include "obex_macros.h"
#include "obex_main.h"
#include "obex_sdp.h"



static int bt_fill_channs(sdp_data_t *p, unsigned int *channs, int nchanns);
static void bt_find_class(void *value, void *priv);
static int bt_sdp_search(bdaddr_t const *iface, bdaddr_t const *bdaddr,
			 uuid_t *uidsearch, unsigned int *channs, int nchanns);



static int bt_fill_channs(sdp_data_t *p, unsigned int *channs, int nchanns)
{
	int n, proto = 0;

	for (n = 0; p; p = p->next) {
		switch (p->dtd) {
		case SDP_UUID16:
		case SDP_UUID32:
		case SDP_UUID128:
			proto = sdp_uuid_to_proto(&p->val.uuid);
			break;
		case SDP_UINT8:
			if (proto == RFCOMM_UUID) {
				if (n < nchanns) {
					BTDEBUG("Found channel %u\n",
						(unsigned int) p->val.uint8);
					channs[n++] = p->val.uint8;
				}
			}
			break;
		}
	}

	return n;
}

static void bt_find_class(void *value, void *priv)
{
	char uuid_str[MAX_LEN_UUID_STR];
	char svc_uuid_str[MAX_LEN_SERVICECLASS_UUID_STR];
	uuid_t *uuid = (uuid_t *) value;

	sdp_uuid2strn(uuid, uuid_str, MAX_LEN_UUID_STR);
	sdp_svclass_uuid2strn(uuid, svc_uuid_str, MAX_LEN_SERVICECLASS_UUID_STR);
	BTDEBUG("\tFound \"%s\" (0x%s)\n", svc_uuid_str, uuid_str);
	if (sdp_uuid_to_proto(uuid) == OBEX_OBJPUSH_SVCLASS_ID)
		*(int *) priv = 1;
}

static int bt_sdp_search(bdaddr_t const *iface, bdaddr_t const *bdaddr,
			 uuid_t *uidsearch, unsigned int *channs, int nchanns)
{
	int n, s;
	uint32_t range = 0x0000ffff;
	sdp_list_t *attrid, *search, *seq, *next;
	sdp_session_t *sess;
	char str[32];

	sess = sdp_connect(iface, bdaddr, SDP_RETRY_IF_BUSY);
	ba2str(bdaddr, str);
	if (!sess) {
		fprintf(stderr, "Failed to connect to SDP server on %s: %s\n",
			str, strerror(errno));
		return -1;
	}
	attrid = sdp_list_append(0, &range);
	search = sdp_list_append(0, uidsearch);
	if (sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq)) {
		fprintf(stderr, "Service search failed: %s\n", strerror(errno));
		sdp_close(sess);
		return -1;
	}
	sdp_list_free(attrid, 0);
	sdp_list_free(search, 0);
	for (n = 0; seq; seq = next) {
		sdp_record_t *rec = (sdp_record_t *) seq->data;
		sdp_list_t *proto = NULL, *desc;
		uuid_t suidsearch;
		sdp_list_t *classes = NULL;

		sdp_get_service_classes(rec, &classes);
		if (classes) {
			int cfound = 0;

			sdp_list_foreach(classes, bt_find_class, &cfound);
			if (cfound) {
				if (sdp_get_access_protos(rec, &proto) == 0) {
					for (; proto; proto = proto->next) {
						desc = (sdp_list_t *) proto->data;
						for (; desc; desc = desc->next) {
							s = bt_fill_channs((sdp_data_t *) desc->data,
									   channs + n, nchanns - n);
							if (s > 0)
								n += s;
						}
					}
				}
				if (sdp_get_group_id(rec, &suidsearch) != -1) {
					if (suidsearch.value.uuid16 != uidsearch->value.uuid16) {
						s = bt_sdp_search(iface, bdaddr, &suidsearch, channs + n,
								  nchanns - n);
						if (s > 0)
							n += s;
					}
				}
			}
			sdp_list_free(classes, free);
		}

		next = seq->next;
		free(seq);
		sdp_record_free(rec);
	}
	sdp_close(sess);

	return n;
}

int bt_service_channel(int devid, bdaddr_t const *bdaddr, unsigned int service,
		       unsigned int *channs, int nchanns)
{
	bdaddr_t iface;
	uuid_t uidsearch;

	bacpy(&iface, BDADDR_ANY);
	if (devid >= 0)
		hci_devba(devid, &iface);
	sdp_uuid16_create(&uidsearch, service);

	return bt_sdp_search(&iface, bdaddr, &uidsearch, channs, nchanns);
}

