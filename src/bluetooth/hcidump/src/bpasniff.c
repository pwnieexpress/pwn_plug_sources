/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2004-2011  Marcel Holtmann <marcel@holtmann.org>
 *
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <getopt.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "parser/parser.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"

static volatile sig_atomic_t __io_canceled = 0;

static void sig_hup(int sig)
{
}

static void sig_term(int sig)
{
	__io_canceled = 1;
}

static int read_revision(int dd, char *revision, int size)
{
	struct hci_request rq;
	unsigned char req[] = { 0x07 };
	unsigned char buf[46];

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = 0x000e;
	rq.cparam = req;
	rq.clen   = sizeof(req);
	rq.rparam = &buf;
	rq.rlen   = sizeof(buf);

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	if (buf[0] > 0) {
		errno = EIO;
		return -1;
	}

	if (revision)
		strncpy(revision, (char *) (buf + 1), size);

	return 0;
}

static int enable_sniffer(int dd, uint8_t enable)
{
	struct hci_request rq;
	unsigned char req[] = { 0x00, enable };
	unsigned char buf[1];

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = 0x000e;
	rq.cparam = req;
	rq.clen   = sizeof(req);
	rq.rparam = &buf;
	rq.rlen   = sizeof(buf);

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	if (buf[0] > 0) {
		errno = EIO;
		return -1;
	}

	return 0;
}

static int enable_sync(int dd, uint8_t enable, bdaddr_t *bdaddr)
{
	struct hci_request rq;
	unsigned char req[] = { 0x01, enable,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0xfa, 0x00 };

	memcpy(req + 2, bdaddr, 6);

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = 0x000e;
	rq.cparam = req;
	rq.clen   = sizeof(req);

	hci_send_req(dd, &rq, 1000);

	return 0;
}

static char *type2str(uint8_t type)
{
	switch (type) {
	case 0x00:
		return "NULL";
	case 0x01:
		return "POLL";
	case 0x02:
		return "FHS";
	case 0x03:
		return "DM1";
	case 0x04:
		return "DH1";
	case 0x05:
		return "HV1";
	case 0x06:
		return "HV2";
	case 0x07:
		return "HV3";
	case 0x08:
		return "DV";
	case 0x09:
		return "AUX1";
	case 0x0a:
		return "DM3";
	case 0x0b:
		return "DH3";
	case 0x0c:
		return "EV4";
	case 0x0d:
		return "EV5";
	case 0x0e:
		return "DM5";
	case 0x0f:
		return "DH5";
	case 0xff:
		return "ID";
	default:
		return "UNK";
	}
}

static void decode(unsigned char *buf, int count)
{
	struct frame frm;
	uint8_t id, status, channel;
	uint16_t num, len;
	uint32_t time;
	uint8_t type, addr, temp, hdr;
	uint8_t flow, arqn, seqn, hec, llid, pflow;
	uint16_t plen;

	if (count < 7)
		return;

	id = buf[0];
	num = ntohs(bt_get_unaligned((uint16_t *) (buf + 1)));
	len = btohs(bt_get_unaligned((uint16_t *) (buf + 3)));

	status  = buf[5];
	time    = ntohl(bt_get_unaligned((uint32_t *) (buf + 6)));
	channel = buf[10];

	if (len < 8)
		return;

	type = (len < 7) ? 0xff : bt_get_unaligned((uint8_t *) (buf + 11));

	if (type < 2)
		return;

	p_indent(-1, NULL);

	memset(&frm, 0, sizeof(frm));
	frm.data     = buf + 12;
	frm.data_len = count - 12;
	frm.ptr      = frm.data;
	frm.len      = frm.data_len;
	frm.in       = 0;
	frm.master   = 0;
	frm.handle   = 0;
	frm.flags    = 0;

	p_indent(0, &frm);

	printf("BPA: id %d num %d status 0x%02x time %d channel %2d len %d\n",
		id, num, status, time, channel, len - 6);

	if (type < 3) {
		printf("  %s\n", type2str(type));
		raw_dump(1, &frm);
		return;
	}

	addr = bt_get_unaligned((uint8_t *) (buf + 12));
	temp = bt_get_unaligned((uint8_t *) (buf + 13));
	flow = (temp & 0x04) >> 2;
	arqn = (temp & 0x02) >> 1;
	seqn = (temp & 0x01);
	hec  = bt_get_unaligned((uint8_t *) (buf + 14));

	hdr = bt_get_unaligned((uint8_t *) (buf + 20));
	plen  = ((hdr & 0x10) >> 4) | ((hdr & 0x08) >> 2) | (hdr & 0x04) | ((hdr & 0x02) << 2) | ((hdr & 0x01) << 4);
	pflow = ((hdr & 0x20) >> 5);
	llid = ((hdr & 0x80) >> 7) | ((hdr & 0x40) >> 5);
	hdr = bt_get_unaligned((uint8_t *) (buf + 21));
	plen = plen | ((hdr & 0x80) >> 2) | (hdr & 0x40) | ((hdr & 0x20) << 2) | ((hdr & 0x08) << 4);

	p_indent(0, &frm);

	printf("%s: addr 0x%02x flow %d arqn %d seqn %d hec 0x%02x llid %d pflow %d plen %d\n",
		type2str(type), addr, flow, arqn, seqn, hec, llid, pflow, plen);

	if (type == 0x03 && llid == 3) {
		memset(&frm, 0, sizeof(frm));
		frm.data     = buf + 22;
		frm.data_len = plen;
		frm.ptr      = frm.data;
		frm.len      = frm.data_len;
		frm.in       = 0;
		frm.master   = 1;
		frm.handle   = 0;
		frm.flags    = llid;

		lmp_dump(1, &frm);
		return;
	}

	raw_dump(1, &frm);
}

static void process_frames(int dev)
{
	struct sigaction sa;
	struct hci_filter flt;
	unsigned char *buf;
	int dd, size = 2048;

	buf = malloc(size);
	if (!buf) {
		fprintf(stderr, "Can't allocate buffer for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		return;
	}

	dd = hci_open_dev(dev);
	if (dd < 0) {
		fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		free(buf);
		return;
	}

	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_VENDOR_PKT, &flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	hci_filter_set_event(EVT_VENDOR, &flt);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		fprintf(stderr, "Can't set filter for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		free(buf);
		return;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_NOCLDSTOP;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	sa.sa_handler = sig_hup;
	sigaction(SIGHUP, &sa, NULL);

	while (!__io_canceled) {
		int len;

		len = read(dd, buf, size);
		if (len < 0)
			break;
		if (len < 2)
			continue;

		if (buf[0] == 0x04 && buf[1] == 0xff) {
			if (buf[3] == 0x02) {
				switch (buf[4]) {
				case 0x00:
					printf("Waiting for synchronization...\n");
					break;
				case 0x08:
					printf("Synchronization lost\n");
					__io_canceled = 1;
					break;
				default:
					printf("Unknown event 0x%02x\n", buf[4]);
					break;
				}
			}
		}

		if (buf[0] != 0xff)
			continue;

		decode(buf + 1, len - 1);
	}

	hci_close_dev(dd);

	free(buf);
}

static void usage(void)
{
	printf("bpasniff - Utility for the BPA 100/105 sniffers\n\n");
	printf("Usage:\n"
		"\tbpasniff [-i <dev>] <master-bdaddr>\n");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
	struct hci_dev_info di;
	struct hci_version ver;
	char rev[46];
	bdaddr_t bdaddr;
	int dd, opt, dev = 0;

	bacpy(&bdaddr, BDADDR_ANY);

	while ((opt=getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1) {
		switch (opt) {
		case 'i':
			dev = hci_devid(optarg);
			if (dev < 0) {
				perror("Invalid device");
				exit(1);
			}
			break;

		case 'h':
		default:
			usage();
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;
	optind = 0;

	argc -= optind;
	argv += optind;
	optind = 0;

	if (argc < 1) {
		usage();
		exit(1);
	}

	str2ba(argv[0], &bdaddr);

	dd = hci_open_dev(dev);
	if (dd < 0) {
		fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		exit(1);
	}

	if (hci_devinfo(dev, &di) < 0) {
		fprintf(stderr, "Can't get device info for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
	}

	if (hci_read_local_version(dd, &ver, 1000) < 0) {
		fprintf(stderr, "Can't read version info for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
	}

	if (ver.manufacturer != 12) {
		fprintf(stderr, "Can't find sniffer at hci%d: %s (%d)\n",
						dev, strerror(ENOSYS), ENOSYS);
		hci_close_dev(dd);
		exit(1);
	}

	if (read_revision(dd, rev, sizeof(rev)) < 0) {
		fprintf(stderr, "Can't read revision info for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
	}

	printf("%s\n", rev);

	if (enable_sniffer(dd, 0x01) < 0) {
		fprintf(stderr, "Can't enable sniffer for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
	}

	if (enable_sync(dd, 0x01, &bdaddr) < 0) {
		fprintf(stderr, "Can't enable sync for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		enable_sniffer(dd, 0x00);
		hci_close_dev(dd);
		exit(1);
	}

	init_parser(DUMP_EXT | DUMP_VERBOSE, ~0L, 0, DEFAULT_COMPID, -1, -1);

	process_frames(dev);

	if (enable_sync(dd, 0x00, &bdaddr) < 0) {
		fprintf(stderr, "Can't disable sync for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		enable_sniffer(dd, 0x00);
		hci_close_dev(dd);
		exit(1);
	}

	if (enable_sniffer(dd, 0x00) < 0) {
		fprintf(stderr, "Can't disable sniffer for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
	}

	hci_close_dev(dd);

	return 0;
}
