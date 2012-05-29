/* 
   Search and Destroy Bluetooth 
   Copyright (C) 2009 Shadow Cave LLC

   Written 2009 by JP Dunning (.ronin)
   ronin@shadowcave.org
   <www.hackfromacave.com>
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY 
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
   OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS 
   DISCLAIMED.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

char ** addrs; 
int numaddrs = 0;

void onExit( void )
{
	free(addrs);
}

// Read in config file and exicuting commands
int run_config(char addr[19], char * log_file, char * filename)
{
	FILE *file = fopen ( filename, "r" );

	if ( file != NULL ) {
		
		char line[236] = { 0 };
		
		// Read line by line
		while (fgets( line, sizeof(line), file )) { 

			if (strcmp(line,"\n") != 0) {

				char * pound_index;

				// '#' indicates a line is commented out
				pound_index = strchr(line, '#');

				if (pound_index == NULL) {

					char cmd [255] = { 0 };
					char * add_addr;

					add_addr = strchr(line, '*');
					
					// Replace '*' with Bluetooth address
					if (add_addr != NULL) {
						strncpy(cmd, line, add_addr-line);
						strcat(cmd, addr);
						strcat(cmd, add_addr + 2);

					}
					
					// Redirect command output to file
					if (strcmp(log_file, "") != 0) {
						strcat(cmd, " >> ");
						strcat(cmd, log_file);
					}
					
					// Display command
					printf("\n> ");
					printf(cmd);
					printf("\n\n");
				
					// Run command
					system(cmd);
					printf("\n");

				}
			}
		}
		fclose ( file );
	}
	else { 	
		perror ( "File not found" ); 
	}

	return 0;
}


// Print a timestamp for each new device discovered
int time_stamp(char addr[19], char name[248], char * log_file)
{
	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	char echo_header [255] = {0};
	strncpy(echo_header, "echo \"\n---------------------------------------------\n", 60);
	strcat(echo_header, "Time:    ");
	strcat(echo_header, asctime (timeinfo));
	strcat(echo_header, "Name:    ");
	strcat(echo_header, name);
	strcat(echo_header, "\nAddress: ");
	strcat(echo_header, addr);
	strcat(echo_header, "\n---------------------------------------------\n\"");

	system(echo_header);

	// Redirect command output to file
	if (strcmp(log_file, "") != 0) {
		strcat(echo_header, " >> ");
		strcat(echo_header, log_file);
		system(echo_header);
	}

	return 0;

}

// Scan for available bluetooth devices
int scan(char * log_file, char * filename)
{
	inquiry_info *ii = NULL;
	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;
	int i;
	int t_count;
	char addr[19] = { 0 };

	dev_id = hci_get_route(NULL);
	sock = hci_open_dev( dev_id );

	// Error with opening socket
	if (dev_id < 0 || sock < 0) {
		perror("error: opening socket\n");
		return 1;
	}
	
	len  = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
	num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);

	// Check for any devices found
	if( num_rsp < 0 ) {
		return 1;
	}
	
	// Cycle through all the addresses found
	for (i = 0; i < num_rsp; i++) {

		char name[248] = { 0 };
		ba2str(&(ii+i)->bdaddr, addr);
		memset(name, 0, sizeof(name));

		// Check for device name
		if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0) 
			strcpy(name, "[unknown]");

//		while (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0) {
//			// Requesting device name
//		}

		int num = 0;
		int matchfound = 0;

		// Check to see if a new device has been discovered
		while (num < numaddrs) { 
			if (strcmp (addr, addrs[num]) == 0) { 
				matchfound = 1;
			}
			num++;
		}

		// Run commands for newly found device address
		if (matchfound == 0) {

			time_stamp(addr, name, log_file);	
			run_config(addr, log_file, filename);

			// Add address to list of discovered devices			
			addrs[numaddrs] = malloc(sizeof addr + 1);
			strcpy(addrs[numaddrs], addr);
			numaddrs++;
		}	
	}
	
	free( ii );
	close( sock );
	return 0;

}

// Print help for software
void usage()
{
	printf("pwntooth v0.2.1 by John P Dunning <ronin@shadowcave.org>\n(c) Shadow Cave LLC.\n\n"\
			"NAME\n\tpwntooth\n"\
 			"\nSYNOPSIS\n\tpwntooth [-l logfile] [-s script] [-t addr]\n"\
			"\nDESCRIPTION\n"\
			"\t-h\t: help\n"\
			"\t-l\t: file to log results\n"\
			"\t-n\t: number of scans before halting\n"\
			"\t-s\t: location and name of script. Default: /etc/bluetooth/pwntooth.conf\n"\
			"\t-t\t: single target device address. Format: XX:XX:XX:XX:XX:XX\n\n");
}

int main(int argc, char **argv)
{
	atexit(onExit);

	int count = 0;
	addrs = (char**)calloc(sizeof(char**),100);
	char * log_file = "";
	char * script_file = "/etc/bluetooth/pwntooth.conf";
	char * addr = "";
	int scan_iterations = 0;
	int bool_single_target = 0;
	int i = 1;

	// Check arguments
	while (i < argc) {		
		if (strcmp(argv[i], "-h") == 0) {
			usage();
			return 0;
		} else if (strcmp(argv[i], "-l") == 0) {
			if (++i < argc)
				log_file = argv[i];
		} else if (strcmp(argv[i], "-n") == 0) {
			if (++i < argc) {
				scan_iterations = atoi(argv[i]);
			}
			// Input validation
			if ((scan_iterations < 1)||(scan_iterations > 1000000)) { 
				perror("incorrect number of iterations.  choose a number between 1 and 1,000,000.\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-s") == 0) {
			if (++i < argc)
				script_file = argv[i];
		} else if (strcmp(argv[i], "-t") == 0) {
			if (++i < argc) {
				bool_single_target = 1;
				addr = argv[i];
				// Input validation
				if (strlen(addr) != 17) { 
					perror("incorrect addr format.\n");
					return 1;
				}
			}
		} else {
			usage();
			return 1;
		}
		i++;
	}

	if (bool_single_target) { 
		run_config(addr, log_file, script_file);
	}
	else {
		// Run for a specified number of iterations
		while(count < scan_iterations)
		{
			printf("Scanning ...\n");
			scan(log_file, script_file); 
			count++;
		}	
	}
	return 0;
}
