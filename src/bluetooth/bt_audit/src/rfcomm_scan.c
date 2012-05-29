/**
 *  @file rfcomm_scan.c
 *
 *  RFCOMM scan - a Bluetooth(tm) RFCOMM channel scanner
 *
 *  @author Collin R. Mulliner <collin@betaversion.net>
 *
 *  (c) Collin R. Mulliner
 *
 *  web: www.betaversion.net/btdsd/
 */

/*
 * This file is part of rfcomm_scan
 *
 * rfcomm_scan is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * rfcomm_scan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rfcomm_scan; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>

#include <asm/types.h>
#include <asm/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

static int find_conn(int s, int dev_id, long arg)
{
        struct hci_conn_list_req *cl;
        struct hci_conn_info *ci;
        int i;

        if (!(cl = malloc(10 * sizeof(*ci) + sizeof(*cl)))) {
                perror("Can't allocate memory");
                exit(1);
				}
        cl->dev_id = dev_id;
        cl->conn_num = 10;
        ci = cl->conn_info;

        if (ioctl(s, HCIGETCONNLIST, (void *) cl)) {
               perror("Can't get connection list");
                exit(1);
				}

        for (i = 0; i < cl->conn_num; i++, ci++) {
	        if (!bacmp((bdaddr_t *) arg, &ci->bdaddr)) {
						return 1;
					}
				}

        return 0;
}


/**
 *  @brief scan RFCOMM Channels
 *
 *  @param dstAddr bluetooth address (format XX:XX:XX:XX:XX:XX)
 *  @param s       start RFCOMM Channel
 *  @param e       end RFCOMM Channel
 *  @param o       only show open channels
 *  @param d       don't disconnect ACL after each connect try
 *
 *  @returns 0 on success -1 on failure
 */
int rfcomm_scan(char *dstAddr, char *srcAddr,  unsigned short int s, unsigned short int e, int o, int d)
{
	struct sockaddr_rc sa;
	int sock;
	bdaddr_t src, dst;
	unsigned int i;
	struct hci_conn_info_req *cr;
	bdaddr_t bdaddr;
	int opt, dd, dev_id;

	
	baswap(&dst, strtoba(dstAddr));
	baswap(&src, strtoba(srcAddr));
	str2ba(dstAddr, &bdaddr);
	
	for (i = s; i <= e; i++) {
	
		sock = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

		if (sock >= 0) {
			sa.rc_family = AF_BLUETOOTH;
			sa.rc_channel = 0;
		
			if (bacmp(&src, BDADDR_ANY) != 0) {
				sa.rc_bdaddr = src;
				if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
					perror("bind() failed");
					return(-1);
				}
			}

			sa.rc_channel = i;
			sa.rc_bdaddr = dst;

			if (connect(sock, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
				if (o == 0) {
					printf("rfcomm: %0.2d closed\n", i);
				}
			}
			else {
				printf("rfcomm: %0.2d open\n", i);
			}
		}
		else {
			perror("socket() failed\n");
		}

		close(sock);
	
		/* disconnect ACL */
		if (!d) {
			dev_id = hci_for_each_dev(HCI_UP, find_conn, (long) &bdaddr);
			if (dev_id < 0) {
				fprintf(stderr, "Not connected.\n");
				exit(1);
			}

			dd = hci_open_dev(dev_id);
			if (dd < 0) {
				perror("HCI device open failed");
				exit(1);
			}

			cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
			if (!cr) {
				perror("Can't allocate memory");
				exit(1);
			}

			bacpy(&cr->bdaddr, &bdaddr);
			cr->type = ACL_LINK;
			if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
				perror("Get connection info failed");
				exit(1);
			}

			if (hci_disconnect(dd, htobs(cr->conn_info->handle), HCI_OE_USER_ENDED_CONNECTION, 10000) < 0) {
				perror("Disconnect failed");
			}
			close(dd);
			free(cr);
		}

	}
	
	return(0);
}

void usage()
{
	printf("\nrfcomm_scan v0.1.1 (c) Collin R. Mulliner <collin@betaversion.net>\nusage:\n\trfcomm_scan [-S <bd_addr>] [-o] "\
			"[-s <channel>] [-e <channel>] <bd_addr>\n\noptions:\n"\
			"\tS\t: source BDADDR (local)\n"\
			"\to\t: only show open channels\n"\
			"\ts\t: start channel (1-30)\n\te\t: end channel (1-30)\n"\
			"\td\t: don't disconnect ACL after each rfcomm connection\n\n"\
			"Note:\n"\
			"\tMake sure the host running rfcomm_scan is configured as YOU want, if authentication\n"\
			"\tand/or encryption is on/off active pairings will be used or new pairings will start!\n");
}

int main(int argc, char **argv)
{
	char srcAddr[20];
	int opt;
	int startc = 1;
	int endc = 30;
	int o = 0;
	int d = 0;
		

	memcpy(srcAddr, "00:00:00:00:00:00\0", 18);
	while ((opt = getopt(argc, argv,"S:e:s:od")) != EOF) {
	
		switch(opt) {
		case 'S':
			strncpy(srcAddr, optarg, 20);
			break;
		case 'o':
			o = 1;
			break;
		case 'd':
			d = 1;
			break;
		case 's':
			startc = atoi(optarg);
			if (startc < 1 || startc > 30) {
				usage();
				exit(-1);
			}
			break;
		case 'e':
			endc = atoi(optarg);
			if (endc < 1 || endc > 30) {
				usage();
				exit(-1);
			}
			break;
		default:
			usage();
			exit(-1);
		}
	}

	if (!(argc - optind)) {
		usage();
		exit(-1);
	}

	if (startc > endc) {
		usage();
		exit(-1);
	}
	
	exit(rfcomm_scan(argv[optind], srcAddr, startc, endc, o, d));
}
