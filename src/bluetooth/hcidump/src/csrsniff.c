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
#include <unistd.h>
#include <stdlib.h>
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

static struct {
	uint16_t id;
	uint16_t ver;
	char *date;
} firmware_map[] = {
	{  195,  1, "2001-11-27"	},
	{  220,  2, "2002-01-03"	},
	{  269,  3, "2002-02-22"	},
	{  270,  4, "2002-02-26"	},
	{  284,  5, "2002-03-12"	},
	{  292,  6, "2002-03-20"	},
	{  305,  7, "2002-04-12"	},
	{  306,  8, "2002-04-12"	},
	{  343,  9, "2002-05-02"	},
	{  346, 10, "2002-05-03"	},
	{  355, 11, "2002-05-16"	},
	{  256, 11, "2002-05-16"	},
	{  390, 12, "2002-06-26"	},
	{  450, 13, "2002-08-16"	},
	{  451, 13, "2002-08-16"	},
	{  533, 14, "2002-10-11"	},
	{  580, 15, "2002-11-14"	},
	{  623, 16, "2002-12-12"	},
	{  678, 17, "2003-01-29"	},
	{  847, 18, "2003-04-17"	},
	{  876, 19, "2003-06-10"	},
	{  997, 22, "2003-09-05"	},
	{ 1027, 23, "2003-10-03"	},
	{ 1029, 24, "2003-10-03"	},
	{ 1112, 25, "2003-12-03"	},
	{ 1113, 25, "2003-12-03"	},
	{ 1133, 26, "2003-12-18"	},
	{ 1134, 26, "2003-12-18"	},
	{ 1223, 27, "2004-03-08"	},
	{ 1224, 27, "2004-03-08"	},
	{ 1319, 31, "2004-04-22"	},
	{ 1320, 31, "2004-04-22"	},
	{ 1427, 34, "2004-06-16"	},
	{ 1508, 35, "2004-07-19"	},
	{ 1509, 35, "2004-07-19"	},
	{ 1587, 36, "2004-08-18"	},
	{ 1588, 36, "2004-08-18"	},
	{ 1641, 37, "2004-09-16"	},
	{ 1642, 37, "2004-09-16"	},
	{ 1699, 38, "2004-10-07"	},
	{ 1700, 38, "2004-10-07"	},
	{ 1752, 39, "2004-11-02"	},
	{ 1753, 39, "2004-11-02"	},
	{ 1759, 40, "2004-11-03"	},
	{ 1760, 40, "2004-11-03"	},
	{ 1761, 40, "2004-11-03"	},
	{ 2009, 41, "2005-04-06"	},
	{ 2010, 41, "2005-04-06"	},
	{ 2011, 41, "2005-04-06"	},
	{ 2016, 42, "2005-04-11"	},
	{ 2017, 42, "2005-04-11"	},
	{ 2018, 42, "2005-04-11"	},
	{ 2023, 43, "2005-04-14"	},
	{ 2024, 43, "2005-04-14"	},
	{ 2025, 43, "2005-04-14"	},
	{ 2032, 44, "2005-04-18"	},
	{ 2033, 44, "2005-04-18"	},
	{ 2034, 44, "2005-04-18"	},
	{ 2288, 45, "2005-07-08"	},
	{ 2289, 45, "2005-07-08"	},
	{ 2290, 45, "2005-07-08"	},
	{ 2388, 46, "2005-08-17"	},
	{ 2389, 46, "2005-08-17"	},
	{ 2390, 46, "2005-08-17"	},
	{ 2869, 47, "2006-02-15"	},
	{ 2870, 47, "2006-02-15"	},
	{ 2871, 47, "2006-02-15"	},
	{ 3214, 48, "2006-02-16"	},
	{ 3215, 48, "2006-02-16"	},
	{ 3216, 48, "2006-02-16"	},
	{    0, }
};

static int id2ver(uint16_t id)
{
	int i;

	for (i = 0; firmware_map[i].id; i++)
		if (firmware_map[i].id == id)
			return firmware_map[i].ver;

	return -1;
}

static void usage(void)
{
	printf("csrsniff - Utility for the CSR BlueCore sniffers\n\n");
	printf("Usage:\n"
		"\tcsrsniff [-i <dev>] <master-bdaddr> [slave-bdaddr]\n");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
	struct sigaction sa;
	struct hci_dev_info di;
	struct hci_version ver;
	struct hci_filter flt;
	bdaddr_t bdaddr, master, slave;
	int need_raw;
	int dd, opt, dev = 0;

	bacpy(&slave, BDADDR_ANY);

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

	if (argc < 1) {
		usage();
		exit(1);
	}

	str2ba(argv[0], &master);

	if (argc > 1)
		str2ba(argv[1], &slave);

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
		fprintf(stderr, "Can't read version for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
	}

	if (ver.manufacturer != 10 || id2ver(ver.hci_rev) < 0) {
		fprintf(stderr, "Can't find sniffer at hci%d: %s (%d)\n",
						dev, strerror(ENOSYS), ENOSYS);
		hci_close_dev(dd);
		exit(1);
	}

	if (!bacmp(&di.bdaddr, BDADDR_ANY)) {
		if (hci_read_bd_addr(dd, &bdaddr, 1000) < 0) {
			fprintf(stderr, "Can't read address for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
			hci_close_dev(dd);
			exit(1);
		}
	} else
		bacpy(&bdaddr, &di.bdaddr);

	need_raw = !hci_test_bit(HCI_RAW, &di.flags);

	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_ACLDATA_PKT, &flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	hci_filter_set_event(EVT_VENDOR, &flt);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		fprintf(stderr, "Can't set filter for hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
		hci_close_dev(dd);
		exit(1);
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

	if (need_raw) {
		if (ioctl(dd, HCISETRAW, 1) < 0) {
			fprintf(stderr, "Can't set raw mode on hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
			hci_close_dev(dd);
			exit(1);
		}
	}

	printf("CSR sniffer - Bluetooth packet analyzer ver %s\n", VERSION);

	if (need_raw) {
		if (ioctl(dd, HCISETRAW, 0) < 0)
			fprintf(stderr, "Can't clear raw mode on hci%d: %s (%d)\n",
						dev, strerror(errno), errno);
	}

	hci_close_dev(dd);

	return 0;
}
