/* 
   SpoofTooph
   Copyright (C) 2009-2012 Shadow Cave LLC

   Written 2009-2012 by JP Dunning (.ronin)
   ronin [ at ] shadowcave [dt] org
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
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <ncurses.h>
#include <pthread.h>

#include "bdaddr.c"
#include "dev_class.c"
#include "namelist.c"
#include "btdev.h"

typedef struct btdev btdev;

// TESTING
//
// int thread_count = 0;
//
 
int numaddrs = 0;		// Number if devices logged
int max_addrs = 10;		// Maximum number of devices in array
int thread_block = 0;		// Blocks running of thread

int scifi_flag = 0;		// Indicates is SciFi names should be used
int u_flag = 0;			// Indicated a interactive delay for reinitializing interface

int num_list = 7;		// Number of devices per page
int hdev = 0;
char * log_file = NULL;		// Loge file
void * run_thread();
btdev * mydev;			// Struct to store device infromation
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;


void onExit( void )
{
//    save_file();
}

// Resize mydev array
int incrementArray()
{
	max_addrs += 10;
	mydev = (btdev *)realloc(mydev, (max_addrs * sizeof(btdev)));

	return 0;
}

// Verify the address is a valid Bluetooth address
int check_addr(char opt[])
{
	// Checking for valid addr

	int i = 0;
	for (i; i < 17; i++)
	{
		if ((((int)opt[i] >= 48) && ((int)opt[i] <= 57)) || (((int)opt[i] >= 65) && ((int)opt[i] <= 70))) {}
		else {
			fprintf(stderr, "Invalid addr: %s\n", opt);
			return(1);
		}
		i++;

		if ((((int)opt[i] >= 48) && ((int)opt[i] <= 57)) || (((int)opt[i] >= 65) && ((int)opt[i] <= 70))) {}
		else {
			fprintf(stderr, "Invalid addr: %s\n", opt);
			return(1);
		}
		i++;

		if (((i == 17)&&((int)opt[i] == 0)) || ((int)opt[i] == 58)) {}
		else {
			fprintf(stderr, "Invalid addr: %s\n", opt);
			return(1);
		}
	}
	
	return 0;
}

// Verify that the input is a valid class
int check_class(char * opt)
{	

	// Checking for valid class
	if ((opt[0] != '0') || (opt[1] != 'x')) {
		fprintf(stderr,"Invalid class: %s\n", opt);
		return(1);
	}
	int i = 2;
	for (i; i < 7; i++)
	{
		if ((((int)opt[i] >= 48) && ((int)opt[i] <= 57)) || (((int)opt[i] >= 97) && ((int)opt[i] <= 122))) {}
		else {
			fprintf(stderr,"Invalid class: %s\n", opt);
			return(1);
		}
	}
	return 0;
}

/* 
Write the device class.

Most of the following function is form 'hciconfig.c' in BlueZ
*/
static int cmd_class(int hdev, char *opt)
{
	if (check_class(opt))
		return 1;
	
	int s = hci_open_dev(hdev);
	if (s < 0) {
		fprintf(stderr,"Can't open device hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		return(1);
	}
	if (opt) {
		uint32_t cod = strtoul(opt, NULL, 16);
		if (hci_write_class_of_dev(s, cod, 2000) < 0) {
			fprintf(stderr,"Can't write local class of device on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
			return(1);
		 }
	}
// 	printf("Class Set: %s\n", opt);
	return(0);
}

/* 
Write the device name.

Most of the following function is form 'hciconfig.c' in BlueZ
*/
static int cmd_name(int hdev, char *opt)
{
	if ((sizeof(opt) < 0) || (sizeof(opt) > 248)) {
		fprintf(stderr,"error: Invalid name: %s\n", opt);
		return(1);
	}

	int dd;

	dd = hci_open_dev(hdev);
	if (dd < 0) {
		fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		return(1);
	}

	if (opt) {
		if (hci_write_local_name(dd, opt, 2000) < 0) {
			fprintf(stderr, "Can't change local name on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
			return(1);
		}
	} 
	
	hci_close_dev(dd);
//	printw("Name Set:  %s\n", opt);
	return(0);
}

// Convert hex into integer
int hextoint(char hex)
{
	switch(hex)
	{
	  case '0':
		return 0;
	  case '1':
		return 1;
	  case '2':
		return 2;
	  case '3':
		return 3;
	  case '4':
		return 4;
	  case '5':
		return 5;
	  case '6':
		return 6;
	  case '7':
		return 7;
	  case '8':
		return 8;
	  case '9':
		return 9;
	  case 'a':
		return 10;
	  case 'b':
		return 11;
	  case 'c':
		return 12;
	  case 'd':
		return 13;
	  case 'e':
		return 14;
	  case 'f':
		return 15;
	  case 'A':
		return 10;
	  case 'B':
		return 11;
	  case 'C':
		return 12;
	  case 'D':
		return 13;
	  case 'E':
		return 14;
	  case 'F':
		return 15;
	}
	
	return -1;
}

int put_class(int dev_index, char * class)
{
  

	int i=2;
	for (i=2; i < 8; i++) {
		if (hextoint(class[i]) == -1) {
			fprintf(stderr,"invalid class data.\n");
			return 1;
		}
	}

	mydev[dev_index].flags = (hextoint(class[2])*16)+(hextoint(class[3]));
	mydev[dev_index].major_class = (hextoint(class[4])*16)+(hextoint(class[5]));
	mydev[dev_index].minor_class = (hextoint(class[6])*16)+(hextoint(class[7]));

	return 0;
}

// Get the Class from the Flags, Major Class, and Minor Class
int get_class(int dev_index, char * class)
{ 
	class[0] = '0';
	class[1] = 'x';
	char class_part1[3] = {0};
	char class_part2[3] = {0};
	char class_part3[3] = {0}; 
	int n1, n2, n3;
	
	n1 = sprintf(class_part1, "%02x", mydev[dev_index].flags);		// Get hex values of flag
	class[2] = class_part1[0];
	class[3] = class_part1[1];
	n2 = sprintf(class_part2, "%02x", mydev[dev_index].major_class);	// Get hex values of major class
	class[4] = class_part2[0];
	class[5] = class_part2[1];
	n3 = sprintf(class_part3, "%02x", mydev[dev_index].minor_class);	// Get hex values of minor class
	class[6] = class_part3[0];
	class[7] = class_part3[1];
	
	return (1);
}

// Make appropriate funtion calls to spoof the device
int spoof(int dev_index, int hdev)
{
  
	cmd_bdaddr(hdev, mydev[dev_index].addr);
//	if (u_flag){
//		printw("Press any key once USB is reinitialized");
//		getchar();
//	}

	sleep(10);

//	cmd_class(hdev, class);
	cmd_class(hdev, mydev[dev_index].class);
	cmd_name(hdev, mydev[dev_index].name);
	return 0;
}

/* 
Write the print the services from the class.

Most of the following function is form 'hciconfig.c' in BlueZ
*/
int print_service(int n)
{
	unsigned int i;
	int first = 1;
	printw("[");
	for (i = 0; i < (sizeof(services) / sizeof(*services)); i++) {
		if (mydev[n].flags & (1 << i)) {
			if (!first) {
				printw(",");
				printw(" %s", services[i]);
				first = 0;
			}
			else {
				first = 0;
				printw(" %s", services[i]);
			}
		}
	}
	printw(" ]");

	return 0;
}
// Display the major and minor class of device
int print_class(int i)
{
	char * minor = get_minor_device_name(mydev[i].major_class, mydev[i].minor_class);
					
	if ((mydev[i].major_class & 0x1f) >= sizeof(major_devices) / sizeof(*major_devices))
		printw("%-40s","Unknown");
	else {
		char classes[50];
		strcpy(classes, major_devices[mydev[i].major_class & 0x1f]);
		strcat(classes, " (");
		strcat(classes,get_minor_device_name(mydev[i].major_class & 0x1f, mydev[i].minor_class));
		strcat(classes, ")");
		printw("%-40s", classes);
	}
	return 0;
}

// Print a list of devices
int choice_list(int page)
{
	// Get current time
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	
	// Print to screen
	clear();
	printw("             _____                    __ _______                _     \n");
	printw("            / ____|                  / _|__   __|              | |    \n");
	printw("           | (___  _ __   ___   ___ | |_   | | ___   ___  _ __ | |__  \n");
	printw("            \\___ \\| '_ \\ / _ \\ / _ \\|  _|  | |/ _ \\ / _ \\| '_ \\| '_ \\ \n");
	printw("            ____) | |_) | (_) | (_) | |    | | (_) | (_) | |_) | | | |\n");
	printw("           |_____/| .__/ \\___/ \\___/|_|    |_|\\___/ \\___/| .__/|_| |_|\n");
	printw("                  | |                                    | |          \n");
	printw("                  |_|                                    |_|          \n");
	printw("\n                        Time: %s\n", asctime (timeinfo));
	printw("   TYPE\t\t\t\t\t   NAME\n");
	printw("   ADDR\t\t\tCLASS\t\t   SERVICES\n\n");

	int i=page*num_list;	// Start index of device to print 
	int n;			// End index of device to print
	
	if ((i+num_list) > numaddrs) {
		n = numaddrs;
	} else {
		n = i+num_list;
	}
	
	// Print device info
	while(i < n)
	{
	  printw("%i) ",i );

	  print_class(i);
	  if (mydev[i].name)
		printw("%s",mydev[i].name );
	  else
		printw("%s", "Unknown");
	  printw("\n");
	  
	  printw("   %-21s0x%02x%02x%02x\t   " ,mydev[i].addr,mydev[i].flags, mydev[i].major_class, mydev[i].minor_class );
//	  printw("   %-21s%s\t   " ,mydev[i].addr,mydev[i].class );

	  if(mydev[i].flags) {
		print_service(i);
	  } else {
		printw("%s", "Unspecified");
	  }
	  
	  printw("\n");
	  i++;
	}

	printw("\nPage %i of %i\n", (page + 1), ((numaddrs-1)/num_list + 1));
	printw("\n's' make selection, 'p' previous page, 'n' next page, 'q' quite: ");

	
//	printw("Number of scans: %i", thread_count);
	return 0;

}

// Assign random info to the Bluetooth interface
int dev_random(int hdev)
{
	srand ( time(NULL) );
	int major = (rand() % 9);

	int minor = (rand() % 6);  // Add more robust verion later
	
	// Randomly add service flags
	int flag = 0;              
	if (rand() % 2) flag += 1;
	if (rand() % 2) flag += 2;
	if (rand() % 2) flag += 4;
	if (rand() % 2) flag += 8;
	if (rand() % 2) flag += 10;
	if (rand() % 2) flag += 20;
	if (rand() % 2) flag += 40;
	if (rand() % 2) flag += 80;

	// Generate random name
	int name_choice = (rand() % 100);
	char dev_name[100] = {0};

	if (scifi_flag)
		strcpy(dev_name, scifi_name(name_choice));
	else
		strcpy(dev_name, rand_name(name_choice));

	strcat(dev_name, "\'s ");
	strcat(dev_name, major_devices[major & 0x1f]);


	// Generale Random Class
	char class[9] = {0};
	sprintf(class, "0x%02x%02x%02x", flag, major, minor);
	char * class_str = class;
  	
	// Generate random address
	char addr_part[3] = {0};
	char addr[18] = {0};
	int i = 0;
	addr[i++] = '0';
	addr[i++] = '0';
	addr[i++] = ':';
	
	while ( i < 18)
	{
		sprintf(addr_part, "%02x", (rand() % 254));	
		addr[i++] = addr_part[0];
		addr[i++] = addr_part[1];
		addr[i++] = ':';
	}

	sprintf(addr_part, "%02x", (rand() % 254));	
	addr[i++] = addr_part[0];
	addr[i++] = addr_part[1];
	
	// Set Bluetooth info
	
	cmd_bdaddr(hdev, addr);
	sleep(10);

	cmd_class(hdev, class_str);
	cmd_name(hdev, dev_name);
}

// Get devices from log file
int load_log(char * log_file)
{
	FILE *file = fopen ( log_file, "r" );

	if ( file != NULL ) {
		
		char line[LENGTH_NAME + LENGTH_ADDR + LENGTH_CLASS] = { 0 };
		
		// Read line by line
		while (fgets( line, sizeof(line), file )) { 

			if (strcmp(line,"\n") != 0) {

				char * pound_index;
				
				// '#' indicates a line is commented out
				pound_index = strchr(line, '#');
				
				if (pound_index == NULL) {
					
					char name [LENGTH_NAME] = { 0 };
					char addr [LENGTH_ADDR] = { 0 };
					char class [LENGTH_CLASS] = { 0 };
					char * add_addr;
					add_addr = strchr(line, '\n');
										
					// Parse line for ADDR, CLASS, and NAME
					sscanf(line, "%[^','],%[^','],%s", addr, class, name);
					
					// Add Name
					strcpy(mydev[numaddrs].name, name);	
					
					// Add Addr
					if (check_addr(addr) == 0) {
						strcpy(mydev[numaddrs].addr, addr);	
					} else {
						fprintf(stderr,"Invalid or improper data from file.\n" ); 
						return 1;
					}
					
					// Add Class
					if (check_class(class) == 0) {
						put_class(numaddrs, class);
					} else {
						fprintf(stderr,"Invalid or improper data from file.\n" ); 
						return 1;
					}
					
					numaddrs++;	// Increment number of devs
				}
			}
		}
		fclose ( file );
	}
	else { 	
		fprintf(stderr,"File not found\n" ); 
	}

}


int save_file()
{
	FILE *fp;
	time_t rawtime;
	struct tm * timeinfo;
	int i = 0;
	
	if(numaddrs >= 1) {
	  
		// Create file
		if((fp = fopen(log_file, "w")) == NULL)
			return 1;

		fprintf(fp, "%s,%s,%s\n", mydev[i].addr, mydev[i].class, mydev[i].name);
		fclose(fp);
		i++;
	}
	
	
	while(i < numaddrs) {
		// Append file
		if((fp = fopen(log_file, "a")) == NULL)
			return 1;

		fprintf(fp, "%s,%s,%s\n", mydev[i].addr, mydev[i].class, mydev[i].name);
		i++;
		
		fclose(fp);
	}
	
	return 0;
}


// Scan for available bluetooth devices
int scan(char * log_file)
{
	inquiry_info *ii = NULL;
	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;
	int i;
	int t_count;
	char addr[18] = { 0 };
	
//	dev_id = hci_get_route(NULL);
	dev_id = hdev;
	sock = hci_open_dev( dev_id );

	// Error with opening socket
	if (dev_id < 0 || sock < 0) {
		fprintf(stderr,"error: opening socket\n");
		close( sock );
		return 1;
	}
	
	len  = 3; //8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
	num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);

	// Check for any devices found
	if( num_rsp < 0 ) {
		close( sock );
		return 1;
	}
	
	// Cycle through all the addresses found
	for (i = 0; i < num_rsp; i++) {

		char name[248] = { 0 };
		ba2str(&(ii+i)->bdaddr, addr);
		memset(name, 0, sizeof(name));

//		// Check for device name
//		if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
//			strcpy(name, "[unknown]");
		
		int num = 0;
		int matchfound = 0;

		// Check to see if a new device has been discovered
		while (num < numaddrs) { 
			if (strcmp (addr, mydev[num].addr) == 0) { 
				matchfound = 1;
		      
				// If the name of the device is found, replace "[unknown]" with the name.
//				if((strcmp(mydev[num].name, "[unknown]") == 0)&&(strcmp(name,"[unknown]") != 0)) {
				if(strcmp(mydev[num].name, "[unknown]") == 0) {
					// Check for device name
					if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
						strcpy(name, "[unknown]");
	
					strcpy(mydev[num].name, name);
				}
			}
			num++;
		}

		// Add newly found device to list
		if (matchfound == 0) {
			
			if ((numaddrs+10) > max_addrs)
				incrementArray();

			// Check for device name
			if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
				strcpy(name, "[unknown]");

			strcpy(mydev[numaddrs].addr, addr);
			strcpy(mydev[numaddrs].name, name);
			mydev[numaddrs].major_class = (ii+i)->dev_class[1];
			mydev[numaddrs].minor_class = (ii+i)->dev_class[0];
			mydev[numaddrs].pscan_rep_mode = (ii+i)->pscan_rep_mode;
			mydev[numaddrs].pscan_mode = (ii+i)->pscan_mode;
			mydev[numaddrs].clock_offset = (ii+i)->clock_offset;
			mydev[numaddrs].flags = (ii+i)->dev_class[2];
			sprintf(mydev[numaddrs].class,"0x%02x%02x%02x",mydev[numaddrs].flags, mydev[numaddrs].major_class, mydev[numaddrs].minor_class);


			// Write to log file
			if (log_file) {
				
				FILE *fp;

				// Create file
				if((fp = fopen(log_file, "a")) == NULL) {
					close( sock );
					return 1;
				}

				time_t rawtime;
				struct tm * timeinfo;
				time (&rawtime);
				timeinfo = localtime (&rawtime);

//				fprintf(fp, "# DEVICE DISCOVERED: %s", asctime (timeinfo));
//				fprintf(fp, "%s\n%s\n0x%02x%02x%02x\n", name, addr, mydev[numaddrs].flags, mydev[numaddrs].major_class, mydev[numaddrs].minor_class);
				fprintf(fp, "%s,%s,%s\n", mydev[i].addr, mydev[i].class, mydev[i].name);

				fclose (fp);
			}

			numaddrs++;
		}
	}

	free( ii );
	close( sock );
	return 0;

}

// Used for thread to scan for Bluetooth devices
void *run_thread()
{
	pthread_mutex_lock( &mutex1 );
	
	scan(log_file);
	
	pthread_mutex_unlock( &mutex1 );

	thread_block = 2;
}

// Print help
void usage()
{
	printf("\nspooftooph v0.5 by JP Dunning (.ronin) \n<www.hackfromacave.com>\n(c) 2009-2012 Shadow Cave LLC.\n\n"\
			"NAME\n\tspooftooph\n"\
 			"\nSYNOPSIS\n\tspooftooph -i dev [-mstu] [-nac]|[-R]|[-r file] [-w file]\n"\
			"\nDESCRIPTION\n"\
			"\t-a <address>\t: Specify new BD_ADDR\n"\
			"\t-b <num_lines>\t: Number of Bluetooth profiles to display per page\n"\
			"\t-c <class>\t: Specify new CLASS\n"\
			"\t-h\t\t: Help\n"\
			"\t-i <dev>\t: Specify interface\n"\
			"\t-m\t\t: Specify multiple interfaces during selection\n"\
			"\t-n <name>\t: Specify new NAME\n"\
			"\t-r <file>\t: Read in CSV logfile\n"\
			"\t-R\t\t: Assign random NAME, CLASS, and ADDR\n"\
			"\t-s\t\t: Scan for devices in local area\n"\
			"\t-t <time>\t: Time interval to clone device in range\n"\
			"\t-u\t\t: USB delay.  Interactive delay for reinitializing interface\n"\
			"\t-w <file>\t: Write to CSV logfile\n"\
			"\t\t\t  (Useful in Virtualized environment when USB must be passed through.)\n\n");
}

int main(int argc, char **argv)
{
	atexit(onExit);

	mydev = (btdev *)malloc(max_addrs * sizeof(btdev));

	int dd = hci_open_dev(hdev);
	char addr[18] = {0};
//	char * name = NULL;
	char name[300] = {0};
	char * class = NULL;
	
	int manual = 0;
	int scaning = 0;
	int load = 0;
	int i = 1;
	int change_addr = 0;
	int scan_iterations = 10;
	int rand = 0;
	int time_delay = 0;
	int timeing = 0;
	int mitm = 0;
	
	if (argc == 1) {
		usage();
		exit(1);
	}

	// Check arguments
	while (i < argc) {		
		if (strcmp(argv[i], "-h") == 0) {
			usage();
			return 0;
		} else if (strcmp(argv[i], "-s") == 0) {
			scaning = 1;
//		} else if (strcmp(argv[i], "-r") == 0) {
//			rand = 1;
		} else if (strcmp(argv[i], "-R") == 0) {
			rand = 1;
//			scifi_flag=1;
		} else if (strcmp(argv[i], "-u") == 0) {
//			rand = 1;
			u_flag=1;
		} else if (strcmp(argv[i], "-t") == 0) {
			timeing = 1;
			if (++i < argc)
				time_delay = atoi(argv[i]);
		} else if (strcmp(argv[i], "-i") == 0) {
			if (++i < argc)
				hdev = atoi(argv[i] + 3);
		} else if (strcmp(argv[i], "-b") == 0) {
			if (++i < argc)
				num_list = atoi(argv[i]);
			if ((num_list > 100) || (num_list < 1)) {
				usage();
				return 1;
			}
		} else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "-w") == 0)) {
			if (++i < argc)
				log_file = argv[i];
		} else if ((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "-r") == 0)){
			if (++i < argc)
				log_file = argv[i];
			load = 1;
		} else if (strcmp(argv[i], "-n") == 0) {
			if (++i < argc)
				strcpy(name,argv[i]);
			manual=1;
		} else if (strcmp(argv[i], "-m") == 0) {
			mitm=1;
		} else if (strcmp(argv[i], "-c") == 0) {
			if (++i < argc){
				class = argv[i];}
			manual=1;
		} else if (strcmp(argv[i], "-a") == 0) {
			if (++i < argc)
				strcpy(addr, argv[i]);
			change_addr=1;
			manual=1;
		} else {
			usage();
			exit(1);
		}
		i++;
	}

	if ((manual + scaning + rand + load + timeing) != 1)
	{
		usage();
		exit(1);
	} else if (rand) {	// Generate random Bluetooth device info
		dev_random(hdev);
	} else if (manual) {	// Change Bluetooth device info to user specified input
		if (change_addr) {
			if (cmd_bdaddr(hdev, addr))
				exit(1);
		}
		
		if (u_flag){
			printf("Press any key once USB is reinitialized ");
			fflush(stdout);
			getchar();
		} else 
		{
			sleep(10);
		}
		
		if (class) {			
			if (cmd_class(hdev, class))
				exit(1);
		} 
		
		if (name) {
			if (cmd_name(hdev, name))
				exit(1);
		} 
	} else if (load) {	// Read in devices from log file
		mydev = (btdev *)malloc(sizeof(btdev)*100);

		if ( load_log(log_file) != 0) {
		      exit(1);
		}

		
		initscr();
		cbreak();
		keypad(stdscr, TRUE);
		noecho();
		

		int page = 0;
		char ch = 'a';
		char string_choice[3];		
		int choice = -1;

		do {

			choice_list(page);
			
			refresh();

			choice = (ch - 48);
			
			
			switch(ch)
			{  	
			  case 's':	// Make selection

				choice_list(page);
				timeout(100000);
				echo();
				
				if (mitm) {
					char hcidev[6] = {0};

					printw("\n\nProvide interface and hit ENTER (Ctrl+C to quite): ");
//					refresh();
					getnstr(hcidev, 6);
					timeout(100000);		
					hdev = atoi(hcidev + 3);

					printw("\n\nMake selection by reference number and hit ENTER (Ctrl+C to quite): ");
//					refresh();
					getnstr(string_choice, 3);
					timeout(100000);		
					choice = atoi(string_choice);

					if((choice >= 0) && (choice < numaddrs))
					{
						clear();
						endwin();

						fflush(stdout);
						printf("\nCloning ...\n\n");
						fflush(stdout);

						spoof(choice, hdev);
						timeout(100000);	
						
						initscr();
						noecho();
						timeout(100);
//						refresh();
						
//						exit(0);
					}
				} else {
					printw("\n\nMake selection by reference number and hit ENTER (Ctrl+C to quite): ");
					refresh();
					getnstr(string_choice, 3);
//						timeout(100);		
					choice = atoi(string_choice);

					if((choice >= 0) && (choice < numaddrs))
					{
						clear();
						endwin();

//						printw("\nCloning ...\n\n");
						fflush(stdout);

						printf("\nCloning ...\n\n");
						fflush(stdout);
						
						spoof(choice, hdev);
						
						exit(0);
					}
				}
			  case 'n':	// Next page
					if (((page+1)*num_list) < numaddrs) {
						page++;
						choice_list(page);
					}
					break;
			  case 'p':	// Previous page
					if (0 <= (page - 1)) {
						page--;
						choice_list(page);
					}
					break;
			}
		} while(((ch = getch()) != 'Q') && (ch != 'q'));
		
		endwin();

	} else if (timeing) {	// Scan for devices in range and clone info at specific intervals
		
		mydev = (btdev *)malloc(sizeof(btdev)*100);
		
		while(1)
		{
			numaddrs = 0;
			scan(NULL);

			if (numaddrs >= 0)
			{
				spoof(0, hdev);
			} else {
				printf("\nNo change.\n");  // No device in range to clone
			}

			sleep(time_delay);
		}
	} else if (scaning) { 	// Scan for devices in range and allow user to select the device info to clone 

		mydev = (btdev *)malloc(sizeof(btdev)*100);

		initscr();
		cbreak();
		keypad(stdscr, TRUE);
		noecho();
		timeout(100);
		
		int ch;

		char string_choice[3];
		
		int choice = -1;
		int rc;
		pthread_t thread;
		int page = 0;
		
		while(((ch = getch()) != 'Q') && (ch != 'q')) 
		{
			if (thread_block == 0)
			{

				choice_list(page);
				refresh();

				thread_block = 1;

//				thread_count++;
				
				if(log_file) {
					if (save_file() == 1) {
						printf("File IO error\n\n");
						exit(1);
					}
				}

				// Create independent threads for scanning. Each of which will execute function
				if( (rc=pthread_create( &thread, NULL, &run_thread, NULL)) )
				{
				    endwin();
				    thread_block=0;
				    fprintf(stderr,"Thread creation failed: %d\n", rc);
				    exit (1);
				}				
			} 
			
			if (thread_block == 2) {
				pthread_join(thread, NULL);
				thread_block = 0;
			}
			
			switch(ch)
			{  	
			  	case 's':	// Make selection
					if(log_file)
						save_file();

					choice_list(page);
					timeout(100000);
					echo();
					
					if (mitm) {
						char hcidev[6] = {0};

						printw("\n\nProvide interface and hit ENTER (Ctrl+C to quite): ");
//						refresh();
						getnstr(hcidev, 6);
						timeout(100000);		
						hdev = atoi(hcidev + 3);

						printw("\n\nMake selection by reference number and hit ENTER (Ctrl+C to quite): ");
//						refresh();
						getnstr(string_choice, 3);
						timeout(100000);		
						choice = atoi(string_choice);

						if((choice >= 0) && (choice < numaddrs))
						{
  							clear();
							endwin();

							fflush(stdout);
							printf("\nCloning ...\n\n");
							fflush(stdout);

							spoof(choice, hdev);
							timeout(100000);	
							
							initscr();
							noecho();
							timeout(100);
//							refresh();
							
//							exit(0);
						}
					} else {
						printw("\n\nMake selection by reference number and hit ENTER (Ctrl+C to quite): ");
						refresh();
						getnstr(string_choice, 3);
//						timeout(100);		
						choice = atoi(string_choice);

						if((choice >= 0) && (choice < numaddrs))
						{
							clear();
							endwin();

							fflush(stdout);
							printf("\nCloning ...\n\n");
							fflush(stdout);

							spoof(choice, hdev);
							
							exit(0);
						}
					}
				case 'n':	// Next page
//					if(log_file)
//						save_file();

					if (((page+1)*num_list) < numaddrs) {
						page++;
						choice_list(page);
					}
					break;
				case 'p':	// Previous page
//					if(log_file)
//						save_file();

					if (0 <= (page - 1)) {
						page--;
						choice_list(page);
					}
					break;					
			}			
		}

		if(log_file) {
			if (save_file() == 1) {
				printf("File IO error\n\n");
				exit(1);
			}
		}
		
		pthread_cancel(thread);		
		endwin();
	} else
	{
		usage();
	}
	exit(0);
}

