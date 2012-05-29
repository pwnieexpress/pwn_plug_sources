/* 
   RedFang - The Bluetooth Device Hunter
   Copyright (C) 2003 @stake inc

   Written 2003 by Ollie Whitehouse <ollie@atstake.com>
 
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

/*
 * $Id: fang.c,v 2.50 2003/10/15 09:00:00 ollie Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include <stdarg.h>

#include <termios.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <netinet/in.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "list.h"

int DEBUG = 0;
typedef __u64 HCIA;

static char *version = { "2.5" };

#define PATH_MAX	200
#define LOGSTDOUT	1
#define LOGTXTFILE	2
#define LOGOTHER	3

static void usage(void);

void log_open(int, char *);
void log_flush(int);
void log_close(int);
void log_write(int, const char *, ...);

struct {
   char filename[200200+1];
   int logtype;
   FILE *log_fd;
} output;

typedef struct {
   int random;
   int btout;
   int discovery;
   int threads;
} options;

options opts;

static void banner(int logtype) {

   log_write(logtype, "redfang - the bluetooth hunter ver %s\n", version);
   log_write(logtype, "(c)2003 @stake Inc\n");
   log_write(logtype, "author:   Ollie Whitehouse <ollie@atstake.com>\n");
   log_write(logtype, "enhanced: threads by Simon Halsall <s.halsall@eris.qinetiq.com>\n");
   log_write(logtype, "enhanced: device info discovery by Stephen Kapp <skapp@atstake.com>\n");

}

void printManf () {
   int i;
   
   printf("Valid manf codes are:\n");
   for (i=0; manf[i].name; i++) {
      printf("\t%-15s %-8s", manf[i].name, manf[i].addr);
      if (manf[i].desc) printf(manf[i].desc);
      printf("\n");
   }
}

char * findManf (char *name) 
{
   int i;

   for (i=0; manf[i].name != NULL; i++) {
      if (strncmp(manf[i].name, name, strlen(manf[i].name))) continue;

      return(manf[i].addr);
   }

   return(NULL);
}

char * hcia2str(HCIA a, char *buf) 
{
   static char string[64];

   if (buf == NULL) {
      buf = string;
   //   printf("Allocating static buffer\n");
   }

   sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", 
		   (unsigned char)((a&0xff0000000000LL)>>40), 
		   (unsigned char)((a&0xff00000000LL)>>32), 
		   (unsigned char)((a&0xff000000)>>24), 
		   (unsigned char)((a&0xff0000)>>16), 
		   (unsigned char)((a&0xff00)>>8), 
		   (unsigned char)((a&0xff)));

   return(buf);
}

void str2hcia(char *str, HCIA *hci) 
{
   char *ptr = str;
   char *ind;
   HCIA tmp;

   *hci = strtoll(str, (char **)NULL, 16);

   while (*ptr != '\0') {
      ptr++;   
   }
}

int hciaInc(unsigned char *a, int n, int l) 
{
   if (a[n] == 255) {
      if (n == l) return(0);
      a[n] = 0;
      return(hciaInc(a, n-1, l));
   } else {
      a[n]++;
   }
   return(1);
}

static void hciReset(int dev_id, int ctl) 
{
   
   int ret;
   struct hci_dev_req dr;
   
   if(ioctl(ctl, HCIDEVDOWN, dev_id) < 0 ) {
      printf("Reset failed hci%d. %s(%d)\n", dev_id, strerror(errno), errno);
      exit(1);
   }

        /* Start HCI device */
   if((ret = ioctl(ctl, HCIDEVUP, dev_id)) < 0 ) {
      if( errno == EALREADY )
	 return;
      printf("Can't init device hci%d. %s(%d)\n", dev_id, strerror(errno), errno);
      exit(1);
   }
   
   dr.dev_id  = dev_id;
   dr.dev_opt = SCAN_DISABLED;
   
   if(ioctl(ctl, HCISETSCAN, (unsigned long)&dr) < 0 ) {
      printf("Can't set scan mode on hci%d. %s(%d)\n", dev_id, strerror(errno), errno);
      exit(1);
   }
}

inquiry_info *info = NULL;
int num_rsp = 100;

static void hciDiscovery(int dev_id) 
{
	int length, flags;
	bdaddr_t bdaddr;

	char name[248];
	uint16_t handle = 0;
	unsigned char features[8];
	struct hci_version version;
	struct hci_conn_info_req *cr;
	
	int dd, i, cc;

	dd = hci_open_dev(dev_id);
        if (dd < 0) {
	   fprintf(stderr, "HCI device open failed");
	   free(info);
	   exit(1);
	}

	hciReset(dev_id, dd);
	
	log_write(LOGSTDOUT, "Performing Bluetooth Discovery...");

	if(output.logtype == LOGTXTFILE)
	   log_write(LOGTXTFILE, "Performing Bluetooth Discovery...");

	log_flush(LOGSTDOUT);

	length  = 8;  /* ~10 seconds */
	flags = IREQ_CACHE_FLUSH;
	
	num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);
	if(num_rsp < 0) {
	   log_write(LOGSTDOUT, " Inquiry failed.\n");
	   
	   if(output.logtype == LOGTXTFILE)
	      log_write(LOGTXTFILE, " Inquiry failed.\n");
	} else {
	   log_write(LOGSTDOUT, " Completed.\n");
	   
	   if(output.logtype == LOGTXTFILE)
	      log_write(LOGTXTFILE, " Completed.\n");
	}
	
        for(i = 0; i < num_rsp; i++) {
	   memset(name, 0, sizeof(name));
	   if (hci_read_remote_name(dd, &(info+i)->bdaddr, sizeof(name), name, opts.btout) == 0) {
	      baswap(&bdaddr, &(info+i)->bdaddr);
	      
	      log_write(LOGSTDOUT, "Discovered: %s [%s]\n", name, batostr(&bdaddr));
	      log_write(LOGSTDOUT, "Getting Device Information..");
	      
	      if(output.logtype == LOGTXTFILE) {
	        log_write(LOGTXTFILE, "Discovered: %s [%s]\n", name, batostr(&bdaddr));
	      	log_write(LOGTXTFILE, "Getting Device Information..");
	      }

	      handle = 0;
	      if (hci_create_connection(dd, &(info+i)->bdaddr, 0x0008 | 0x0010, 0, 0, &handle, opts.btout) < 0) {
		      cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	              if (cr) {
		              bacpy(&cr->bdaddr, &(info+i)->bdaddr);
		              cr->type = ACL_LINK;
		              if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) >= 0) {
			              handle = cr->conn_info->handle;
		              } 
		      }
	      }	else {
	      	cc = 1;
	      }	
	      
	      if(handle != 0) {
		      log_write(LOGSTDOUT, " Connected.\n");
		      
	      	      if(output.logtype == LOGTXTFILE)
		         log_write(LOGTXTFILE, " Connected.\n");
		      
		      if (hci_read_remote_version(dd, handle, &version, opts.btout-5000) == 0) {
			log_write(LOGSTDOUT, "\tLMP Version: %s (0x%x) LMP Subversion: 0x%x\n"
				"\tManufacturer: %s (%d)\n",
				lmp_vertostr(version.lmp_ver), version.lmp_ver, version.lmp_subver,					                            bt_compidtostr(version.manufacturer), version.manufacturer);
		      	
	      	      	if(output.logtype == LOGTXTFILE)
			   log_write(LOGTXTFILE, "\tLMP Version: %s (0x%x) LMP Subversion: 0x%x\n"
				"\tManufacturer: %s (%d)\n",
				lmp_vertostr(version.lmp_ver), version.lmp_ver, version.lmp_subver,					                            bt_compidtostr(version.manufacturer), version.manufacturer);
		      	
		      }

		      if (hci_read_remote_features(dd, handle, features, opts.btout-5000) == 0) {
		      	log_write(LOGSTDOUT, "\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n%s\n",
				features[0], features[1], features[2], features[3],
				lmp_featurestostr(features, "\t\t", 3));
		      	
	      	      	if(output.logtype == LOGTXTFILE)
		      	   log_write(LOGTXTFILE, "\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n%s\n",
				features[0], features[1], features[2], features[3],
				lmp_featurestostr(features, "\t\t", 3));
		      }
		      if(cc)
      	      	      	hci_disconnect(dd, handle, 0x13, 10000);
	      } else {
		      log_write(LOGSTDOUT, " Failed.\n");
	      	      
		      if(output.logtype == LOGTXTFILE)
		         log_write(LOGTXTFILE, " Failed.\n");
	      }
	   }
	}
	hci_close_dev(dd);
	
}

static void hciScan(int dev_id, HCIA from, HCIA to, int *total)
{
   bdaddr_t bdaddr;
   char name[248];
   char buf[248];

   uint16_t handle = 0;
   unsigned char features[8];
   struct hci_version version;
   struct hci_conn_info_req *cr;
   
   int opt, dd, cc = 0;
   HCIA hci = from;
   HCIA thci = 0;
   int i = 0, skip = 0;

   dd = hci_open_dev(dev_id);
   if (dd < 0) {
	   fprintf(stderr,"HCI device open failed");
	   exit(1);
   }

   hciReset(dev_id, dd);
   
   do {
      skip = 0;
      if (!DEBUG)
      log_write(LOGSTDOUT, "Done %d - %s                     \r", *total, hcia2str(hci, buf)); fflush(stdout);

      if ((!DEBUG) && (output.logtype == LOGTXTFILE))
         log_write(LOGTXTFILE, "Done %d - %s\n", *total, hcia2str(hci, buf)); log_flush(LOGTXTFILE);
      
      baswap(&bdaddr, strtoba(hcia2str(hci, buf)));
      //dev_id = hci_get_route(&bdaddr);

      if(opts.discovery)
	 for(i = 0; i < num_rsp; i++) {
            memcpy(&thci, &(info+i)->bdaddr, sizeof(thci));
            if((thci & 0xFFFFFFFFFFFFLL) == hci)
	       skip = 1;
	 }
      
      if(skip)
	 continue;
      
      if (hci_read_remote_name(dd,&bdaddr,sizeof(name), name, opts.btout) == 0) {
	      log_write(LOGSTDOUT, "Found: %s [%s]\n", name, hcia2str(hci, buf));
	      log_write(LOGSTDOUT, "Getting Device Information..");
	      
	      if(output.logtype == LOGTXTFILE) {
	        log_write(LOGTXTFILE, "Found: %s [%s]\n", name, hcia2str(hci, buf));
	      	log_write(LOGTXTFILE, "Getting Device Information..");
	      }

	      handle = 0;
	      if (hci_create_connection(dd, &bdaddr, 0x0008 | 0x0010, 0, 0, &handle, opts.btout) < 0) {
		      cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	              if (cr) {
		              bacpy(&cr->bdaddr, &bdaddr);
		              cr->type = ACL_LINK;
		              if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) >= 0) {
			              handle = cr->conn_info->handle;
		              } 
		      }
	      }	else {
	      	cc = 1;
	      }	
	      
	      if(handle != 0) {
		      log_write(LOGSTDOUT, " Connected.\n");
		      
	      	      if(output.logtype == LOGTXTFILE)
		         log_write(LOGTXTFILE, " Connected.\n");
		      
		      if (hci_read_remote_version(dd, handle, &version, opts.btout-5000) == 0) {
			log_write(LOGSTDOUT, "\tLMP Version: %s (0x%x) LMP Subversion: 0x%x\n"
				"\tManufacturer: %s (%d)\n",
				lmp_vertostr(version.lmp_ver), version.lmp_ver, version.lmp_subver,					                            bt_compidtostr(version.manufacturer), version.manufacturer);
		      	
	      	      	if(output.logtype == LOGTXTFILE)
			   log_write(LOGTXTFILE, "\tLMP Version: %s (0x%x) LMP Subversion: 0x%x\n"
				"\tManufacturer: %s (%d)\n",
				lmp_vertostr(version.lmp_ver), version.lmp_ver, version.lmp_subver,					                            bt_compidtostr(version.manufacturer), version.manufacturer);
		      	
		      }

		      if (hci_read_remote_features(dd, handle, features, opts.btout-5000) == 0) {
		      	log_write(LOGSTDOUT, "\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n%s\n",
				features[0], features[1], features[2], features[3],
				lmp_featurestostr(features, "\t\t", 3));
		      	
	      	      	if(output.logtype == LOGTXTFILE)
		      	   log_write(LOGTXTFILE, "\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n%s\n",
				features[0], features[1], features[2], features[3],
				lmp_featurestostr(features, "\t\t", 3));
		      }
		      if(cc)
      	      	      	hci_disconnect(dd, handle, 0x13, 10000);
	      } else {
		      log_write(LOGSTDOUT, " Failed.\n");
	      	      
		      if(output.logtype == LOGTXTFILE)
		         log_write(LOGTXTFILE, " Failed.\n");
	      }
      }

      if (DEBUG) {
         log_write(LOGSTDOUT, "Done[dev %d][total %d] - ", dev_id, *total);
         log_write(LOGSTDOUT, "%s    \n", hcia2str(hci, buf)); fflush(stdout);
         
	 if(output.logtype == LOGTXTFILE) {
	    log_write(LOGTXTFILE, "Done[dev %d][total %d] - ", dev_id, *total);
            log_write(LOGTXTFILE, "%s    \n", hcia2str(hci, buf)); fflush(stdout);
         }
      }

      fflush(stdout);


      (*total)++;
   } while (++hci <= to);
   
   hci_close_dev(dd);
}

static void hciScanThread(void **args)
{
   int *dev_id = (int *)args[0];
   HCIA *from = (HCIA *)args[1];
   HCIA *to = (HCIA *)args[2];

   hciScan(*dev_id, *from, *to, args[3]);
}


static void usage(void)
{
	
   printf("usage:\n"
          "   fang [options]\n\n");
   printf("options:\n"
          "   -r\trange      i.e. 00803789EE76-00803789EEff\n"
          "   -o\tfilename   Output Scan to Text Logfile\n"
	  "     \t           An address can also be manf+nnnnnn, where manf\n"
	  "     \t           is listed with the -l option and nnnnnn is the\n"
	  "     \t           tail of the address. All addresses must be 12\n"
	  "     \t           characters long\n"
          "   -t\ttimeout    The connect timeout, this is 10000 by default\n"
	  "     \t           Which is quick and yields results, increase for\n"
	  "     \t           reliability\n"
          "   -n\tnum        The number of dongles\n"
          "   -d\t           Show debug information\n"            
          "   -s\t           Perform Bluetooth Discovery\n"            
          "   -l\t           Show device manufacturer codes\n"            
          "\n   -h              Display help\n\n"
          "The devices are assumed to be hci0 to hci(n) where (n) is the number\n"
	  "of threads -1, this is currently not configurable but maybe at a\n"
	  "later date\n");
}

char * expand(char *addr) 
{
   static char ret[24];
   char buf[24], tail[24], head[24];
   char *ptr = buf;
   char *ptr2 = NULL;

   strncpy(buf, addr, sizeof(buf));
   strncpy(head, addr, sizeof(head));
   tail[0] = '\0';

   while(*ptr != '\0') {
      if (*ptr == '+') {
         *ptr = '\0';
	 ptr2 = findManf(buf);
	 if (ptr2 == NULL) {
	    printManf();
	    printf("\nManf not found %s\n", buf);
	    exit(-1);
	 }
	 strncpy(head, ptr2, sizeof(head));
	 strncpy(tail, ptr+1, sizeof(tail));
      }
      ptr++;
   }
   
   snprintf(ret, sizeof(ret), "%s%s", head, tail);

   if (strlen(ret) != 12) {
      printf("Address `%s' to short, should be 12 chars long\n", ret);
      exit(-1);
   }
   return(ret);
}

int splitRange(char *range, HCIA *from, HCIA *to) 
{
   char *ptr = range;

   while (*ptr != '\0') {
      if (*ptr == '-') {
         *ptr = '\0';
	 ptr++;
	 break;
      }
      ptr++;
   }

   str2hcia(expand(range), from);
   str2hcia(expand(ptr), to);
   //str2hcia(range, from);
   //str2hcia(ptr, to);
}

void log_open(int logtype, char *filename) {

   if(logtype != LOGTXTFILE) {
      return;
   }
  
   log_write(LOGSTDOUT, "Opening logfile %s.\n", filename);
   
   output.log_fd = fopen(filename, "w");

   if(!output.log_fd) {
	   perror("Unable to open Log file: ");
	   exit(-1);
   }

   strncpy(output.filename, filename, PATH_MAX);
   output.logtype = logtype;
}


void log_close(int logtype) {

	fclose(output.log_fd);
}

void log_flush(int logtype) {
	switch(logtype) {
	case LOGSTDOUT:
	   fflush(stdout);
	   break;
	case LOGTXTFILE:
	   fflush(output.log_fd);
	   break;
	default:
	   break;
	}
}

void log_write(int logtype, const char *fmt, ...) {
	va_list ap;
	char *buf;
	int bufsiz = 4096;
	int buf_alloced = 0, rc = 0, writen = 0;

	va_start(ap, fmt);

	if(logtype == LOGSTDOUT) {
		vfprintf(stdout, fmt, ap);
	}

	if(logtype == LOGTXTFILE) {
		buf = (char *) calloc(bufsiz, sizeof(char));
		buf_alloced = 1;
		
		do {
		   rc = vsnprintf(buf,bufsiz, fmt, ap);
		   if( rc>= 0 && rc < bufsiz) 
			   break;
		   bufsiz = (rc > bufsiz) ? rc + 1 : bufsiz * 2;
		   buf = (char *) realloc(buf, bufsiz);
		} while(1);
	
		writen = fwrite(buf, 1, strlen(buf), output.log_fd);
	}
	
	va_end(ap);

	if(buf_alloced)
		free(buf);
}

int main(int argc, char **argv)
{
   int opt, i;
   char logfilename[PATH_MAX+1];
   HCIA from = 0;
   HCIA to   = 0xffffffffffffLL;
   static int total = 0;
   extern int optind,opterr,optopt;
   extern char *optarg;
   int loghandle;

   output.logtype = 0;
  
   opts.btout = 25000;
   opts.threads = 1;
   opts.random = 0;
   
   
   banner(LOGSTDOUT);
   
   while ((opt=getopt(argc, argv, "Rldhsr:n:t:o:")) != -1) {
      switch(opt) {
	 case 'o':
	    log_open(LOGTXTFILE, optarg);
	    break;
         case 't':
            opts.btout = atoi(optarg);
            break;
	 case 'R':
	    opts.random = 1;
	    break;
         case 'l':
	    printManf();
	    exit(0);
	    break;
         case 'd':
	    DEBUG = 1;
	    break;
         case 'n':
	    opts.threads = atoi(optarg);
            break;
         case 'r':
	    splitRange(optarg, &from, &to);
            break;
	 case 's':
	    opts.discovery = 1;
	    break;
         case 'h':
         default:
            usage();
            exit(0);
      }
   }

   if (from > to) {
      HCIA tmp = to;
      to = from;
      from = tmp;
   }

   log_write(LOGSTDOUT, "Scanning %llu address(es)\n",(unsigned long long)(to - from + 1));
   log_write(LOGSTDOUT, "Address range %s", hcia2str(from, NULL));
   log_write(LOGSTDOUT, " -> %s\n", hcia2str(to, NULL));

   if(output.logtype == LOGTXTFILE) {
	banner(LOGTXTFILE);
   	log_write(LOGTXTFILE, "Scanning %llu address(es)\n",(unsigned long long)(to - from + 1));
   	log_write(LOGTXTFILE, "Address range %s", hcia2str(from, NULL));
   	log_write(LOGTXTFILE, " -> %s\n", hcia2str(to, NULL));
   	
	log_flush(LOGTXTFILE);
   }
   
   if(opts.discovery)
      hciDiscovery(0);
   
   if (opts.threads > 1) {
      __u64 num = to - from + 1;
      int split;
      int count;
      int ret;
      int threads = opts.threads;
      pthread_t tid[opts.threads];
      HCIA last = from;

      if (threads > num) {
	 threads = opts.threads = num;
         log_write(LOGSTDOUT, "Reducing to %d threads\n", threads);
      }

      split = ((num)/threads);

      log_write(LOGSTDOUT, "Using %d threads with approx %d addresses each\n", threads, split);

      log_flush(LOGSTDOUT);
      
      for(count = 0; count < threads; count++) {
	 HCIA lfrom = last;
	 HCIA lto;
	 void **args = malloc(5*sizeof(void *));
	 char hcidev[8];

         num = to - last + 1;

         split = ((num)/(threads - count));

	 lto = lfrom + split -1;

	 if ((lto > to) || (count == (threads-1))) lto = to;

	 last = lto + 1;

         args[0] = malloc(sizeof(int));
         args[1] = malloc(sizeof(HCIA));
         args[2] = malloc(sizeof(HCIA));

	 memcpy(args[0], &count, sizeof(int));
	 memcpy(args[1], &lfrom, sizeof(HCIA));
	 memcpy(args[2], &lto, sizeof(HCIA));
	 args[3] = &total;

	 ret  = pthread_create(&tid[count], NULL, (void *)&(hciScanThread), args);
	 if (ret != 0) {
            printf("Failed to create thread %d\n", count+1);
	    perror("pthread_create");
	    exit(-1);
	 }
      }

      for (count = 0; count < threads; count++) {
         ret = pthread_join(tid[count], NULL);
      }
	 
   } else {
      bdaddr_t ba;

      hciScan(0, from, to, &total);
   }

   if(output.logtype == LOGTXTFILE)
	   log_close(LOGTXTFILE);
}
