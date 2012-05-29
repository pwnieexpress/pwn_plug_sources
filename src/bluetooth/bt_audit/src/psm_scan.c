/**
 *  @file psm_scan.c
 *
 *  PSM scan - a Bluetooth(tm) L2CAP PSM scanner
 *
 *  @author Collin R. Mulliner <collin@betaversion.net>
 *
 *  (c) Collin R. Mulliner
 *
 *  web: www.betaversion.net/btdsd/
 */

/*
 * This file is part of psm_scan
 *
 * psm_scan is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * psm_scan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with psm_scan; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/****
 *
 *  WARNING  UGLY  CODE  AHEAD
 * 
 ****/

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

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>

#include <asm/types.h>
#include <asm/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

/**
 *  @brief scan all PSMs of dst
 *
 *  @param dstAddr  bluetooth address (format XX:XX:XX:XX:XX:XX)
 *  @param s        start PSM
 *  @param e        end PSM
 *  @param onlyopen only show open PSMs
 *  @param conn     plain connect() scan, no raw socket
 *
 *  @returns 0 on success -1 on failure
 */
int psm_scan(char *dstAddr, char *srcAddr,  unsigned short int s,	
	unsigned short int e, int onlyopen, int conn)
{
	struct sockaddr_l2 sa, psa;
	int sock, psock;
	bdaddr_t src, dst;
	unsigned int i;
	unsigned char buf[1024];
	char status_line[1024];
	
	
	baswap(&dst, strtoba(dstAddr));
	baswap(&src, strtoba(srcAddr));

	if (conn == 0) {
		/* SETUP and CONNECT the raw socket */
		sa.l2_bdaddr = src;
		sa.l2_family = AF_BLUETOOTH;
		sa.l2_bdaddr = dst;
		sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP);
	
		if (bacmp(&src, BDADDR_ANY) != 0) {
			if (bind(sock, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
				perror("Can't bind() raw socket");
				return(-1);
			}
		}
		if (connect(sock, (struct sockaddr*) &sa, sizeof(sa)) != 0) {
			perror("Can't connect() raw socket");
			return(-1);
		}
	}
	
	/*
	 *  scan PSMs, only scan odd PSMs
	 */
	for (i = s; i <= e; i+=2) {
	  //sleep(1);	
		/* show last PSM */
		if (i >= (e-2)) onlyopen = 0;
		
		psock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

		if (psock >= 0) {
			psa.l2_family = AF_BLUETOOTH;
			psa.l2_psm = i;
		
			if (bacmp(&src, BDADDR_ANY) != 0) {
				psa.l2_bdaddr = src;
				if (bind(psock, (struct sockaddr *) &psa, sizeof(psa)) < 0) {
					perror("Can't bind()");
					return(-1);
				}
			}
		
			psa.l2_psm = htobs(i);
			psa.l2_bdaddr = dst;

			if (connect(psock, (struct sockaddr*) &psa, sizeof(psa)) != 0) {
				if (conn == 1 && onlyopen == 0) {
					printf("psm: 0x%0.4x (%0.5d) closed\n", i, i);
				}
			}
			else {
				if (conn == 1) {
					printf("psm: 0x%0.4x (%0.5d) open\n", i, i);
				}
			}
			
			if (conn == 0) {		
				l2cap_cmd_hdr *cmd = (l2cap_cmd_hdr*) buf;
				l2cap_conn_rsp *crsp = (l2cap_conn_rsp*) (buf + L2CAP_CMD_HDR_SIZE);

				while (1) {
					struct pollfd pf[1];
					register int err;

					memset(status_line, 0,1024);
					sprintf(status_line, "psm: 0x%0.4x (%0.5d)", i, i);
						
					pf[0].fd = sock;
					pf[0].events = POLLIN|POLLPRI;
					if ((err = poll(pf, 1, 10*10000)) < 0) {
						perror("Poll failed");
						return(-1);
					}
					if (!err) {
						printf("Poll error, exit!\n");
						return(-1);
					}
				
					if ((err = recv(sock, buf, sizeof(buf), 0)) < 0) {
						perror("Recv failed");
						close(sock);
					
						/* SETUP and CONNECT the raw socket */
						sa.l2_bdaddr = src;
						sa.l2_family = AF_BLUETOOTH;
						sa.l2_bdaddr = dst;
						sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP);
	
						if (bacmp(&src, BDADDR_ANY) != 0) {
							if (bind(sock, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
								perror("Can't bind() raw socket");
								return(-1);
							}
						}
						if (connect(sock, (struct sockaddr*) &sa, sizeof(sa)) != 0) {
							perror("Can't connect() raw socket");
							return(-1);
						}
					
						/* retry */
						i -= 2;
						goto retry;
					}
								
					if (!err) {
						printf("Disconnected\n");
						break;
					}
						
					cmd->len = __le16_to_cpu(cmd->len);
					if (cmd->code == L2CAP_COMMAND_REJ) {
						printf("%s target says: command rejected\n", status_line);
					}
					else if (cmd->code == L2CAP_CONN_RSP) {
						switch (crsp->status) {
						case L2CAP_CS_NO_INFO:
							sprintf(status_line+strlen(status_line), " status: L2CAP_CS_NO_INFO    ");
							break;
						case  L2CAP_CS_AUTHEN_PEND:
							sprintf(status_line+strlen(status_line), " status: L2CAP_CS_AUTHEN_PEND");
							break;
						case  L2CAP_CS_AUTHOR_PEND:
							sprintf(status_line+strlen(status_line), " status: L2CAP_CS_AUTHOR_PEND");
							break;
						}
							
						switch (crsp->result) {
						case L2CAP_CR_SUCCESS:
							printf("%s result: L2CAP_CR_SUCCESS\n", status_line);
							break;
						case L2CAP_CR_PEND:
							printf("%s result: L2CAP_CR_PEND\n", status_line);
							continue;
							break;
						case L2CAP_CR_BAD_PSM:
							if (onlyopen == 0) printf("%s result: L2CAP_CR_BAD_PSM\n", status_line);
							break;
						case L2CAP_CR_SEC_BLOCK:
							printf("%s result: L2CAP_CR_SEC_BLOCK\n", status_line);
							break;
						case L2CAP_CR_NO_MEM:
							printf("%s result: L2CAP_CR_NO_MEM\n", status_line);
							break;
						}
						break;
					}
					else if (cmd->code == L2CAP_ECHO_RSP) {
						printf("%s target says: echo responds\n", status_line);
					}
				}
			} /* end: if (conn == 0) */
		}
		else {
			perror("Socket() failed\n");
		}

	retry:
		close(psock);
	}
	
	return(0);
}

void usage()
{
	printf("\npsm_scan v0.4 (c) Collin R. Mulliner <collin@betaversion.net>\nusage:\n\tpsm_scan [-S <bd_addr>] [-o] [-c] "\
			"[-s <psm>] [-e <psm>] <bd_addr>\n\noptions:\n"\
			"\tS\t: source BDADDR (local)\n"\
			"\to\t: only show open PSMs\n\tc\t: simple connect scan (no raw sockets)\n"\
			"\ts\t: start PSM\n\te\t: end PSM\n\n"\
			"Note:\n"\
			"\tPSM range is 1-65535 (0x1 - 0xFFFF) (only odd numbers!)\n"\
			"\tRAW socket scanning can only be done by ROOT\n");
}

int main(int argc, char **argv)
{
	char srcAddr[20];
	int opt;
	int showopen = 0;
	int onlyconn = 0;
	int startpsm = 1;
	int endpsm = 0xffff;
	

	memcpy(srcAddr, "00:00:00:00:00:00\0", 18);
	
	while ((opt = getopt(argc, argv,"S:e:s:oc")) != EOF) {
	
		switch(opt) {
		case 'S':
			strncpy(srcAddr, optarg, 20);
			break;
		case 'o':
			showopen = 1;
			printf("scanning, this will take some time...\n");
			break;
		case 'c':
			onlyconn = 1;
			printf("doing a simple connect() scan...\n");
			break;
		case 's':
			startpsm = atoi(optarg);
			if (startpsm == 0) {
				usage();
				exit(-1);
			}
			break;
		case 'e':
			endpsm = atoi(optarg);
			if (endpsm == 0) {
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

	if (onlyconn == 0) {
		if (geteuid() != 0) {
			usage();
			fprintf(stderr, "\nERROR: need to be ROOT for using raw sockets!\n");
			return(-1);
		}
	}

	if ((startpsm % 2) == 0) startpsm = ((startpsm > 1) ? startpsm-1 : 1);
	exit(psm_scan(argv[optind], srcAddr, startpsm, endpsm, showopen, onlyconn));
}
