/*
 * Copyright (C) 2006 by Martin J. Muench <mjm@codito.de>
 *
 * Bluebug utility
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "bt.h"
#include "at.h"
#include "wrap.h"
#include "debug.h"

#define VERSION     "0.1"
#define SAFE_DELETE(x) if(x) free(x);

/* local functions */
static void Usage(const char *);
static void execute_command(FILE *, int, char *[]);
static void sig_handler(int);
     
int   opterr = 0;     /* shutup getopt() */
FILE *output;
char *target_addr=NULL, *name=NULL, *device=NULL;
FILE *rfcomm_fp = NULL;
int sock=-1, dev_id=-1, hci_sock=-1;

/* 
 * Small wrapper to hack mobile phones (for dummies) 
 */

int main(int argc, char *argv[])
{
  int c, timeout=5000, name_lookup=1, channel=17;
  char filename[FILENAME_MAX];
  bdaddr_t bdtmp;
  bdaddr_t *bdaddr;

  /* default output = stdout */
  output=stdout;

  printf("\nbluebugger %s ( MaJoMu | www.codito.de )\n" \
	 "-----------------------------------------\n\n", VERSION);

  if (geteuid() != 0)
    Usage("You need (e)uid 0 (e.g. be root)\n");

  while((c = getopt(argc, argv, "m:a:t:d:i:no:c:")) != -1) {
    switch(c) {
    case 'a':
      target_addr = (char *)Malloc(strlen(optarg)+1);
      strcpy(target_addr, optarg);
      break;
    case 'd':
      device = (char *)Malloc(strlen(optarg)+1);
      strcpy(device, optarg);
      break;
    case 'm':
      name = (char *)Malloc(strlen(optarg)+1);
      strcpy(name, optarg);
      break;
    case 't':
      timeout = atoi(optarg) * 1000;
      break;
    case 'n':
      name_lookup = 0;
      break;
    case 'c':
      channel = atoi(optarg);
      break;
    case 'o':
      if((output = fopen(optarg, "w+")) == NULL) {
	fprintf(stderr, "Cannot open '%s': %s\n", optarg, strerror(errno));
	exit(EXIT_FAILURE);
      }
      break;
    default:
      Usage("Invalid arguments");
    }
  }

  /* get rid of parsed args */
  argv += optind;
  argc -= optind;

  /* Get bt address */
  if(target_addr == NULL)
    Usage("You have to set the target address");

  fprintf(output, "Target Device:\t'%s'\n", target_addr);
  str2ba(target_addr, &bdtmp);
  bdaddr = &bdtmp;

  /* Get device ID */
  if ((dev_id = hci_get_route(NULL)) == -1) {
    fprintf(stderr, "hci_get_route() failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Set signal handler */
  signal(SIGINT, sig_handler);

  /* Setup socket */
  if ((hci_sock = hci_open_dev(dev_id)) == -1) {
    fprintf(stderr, "hci_open_dev() failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Get target device name */
  if(name_lookup) {
    char asc_name[256]="";
    hci_read_remote_name(hci_sock, bdaddr, sizeof(asc_name)-1, asc_name, timeout);
    fprintf(output, "Target Name:  \t'%s'\n\n", asc_name);
  }
  else
    fprintf(output, "\n");


  /* Configure hci socket */
  bt_configure(dev_id, hci_sock, name);

  /* Create rfcomm socket for communication */
  if((sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM)) == -1) {
    fprintf(stderr, "socket() failed: %s\n", strerror(errno));
    hci_close_dev(hci_sock);
    exit(EXIT_SUCCESS);
  }

  /* bind rfcomm socket (create dev) */
  if(bt_bind(sock, dev_id, bdaddr, channel)) {
    fprintf(stderr, "bt_bind() failed: %s\n", strerror(errno));
    hci_close_dev(hci_sock);
    bt_release(sock, dev_id);
    close(sock);
    exit(EXIT_SUCCESS);
  }
  /* wait 1 sec to ensure dev is ready */
  sleep(1);

  /* Use rfcomm device */
  snprintf(filename, FILENAME_MAX, "%s%u", device == NULL ? "/dev/rfcomm" : device, dev_id);
  if (!(rfcomm_fp = fopen(filename, "r+"))) {
    fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
    hci_close_dev(hci_sock);
    bt_release(sock, dev_id);
    close(sock);
    exit(EXIT_FAILURE);
  }

  /* Set rfcomm device options */
  if (bt_rfcomm_config(fileno(rfcomm_fp)) == 0) {

    /* execute main commands */
    execute_command(rfcomm_fp, argc, argv);

  }
  else
    fprintf(stderr, "bt_rfcomm_config() failed\n");

  /* close rfcomm device */
  fclose(rfcomm_fp);

  /* Cleanup */
  bt_release(sock, dev_id);
  close(sock);
  hci_close_dev(hci_sock);

  /* Cleanup */
  SAFE_DELETE(name);
  SAFE_DELETE(device);
  SAFE_DELETE(target_addr);

  /* Close output file */
  if(output != stdout)
    fclose(output);

  printf("...done\n\n");

  exit(EXIT_SUCCESS);
}

static void execute_command(FILE *rfcomm_fp, int argc, char *argv[]) {
  int i;

  /* shut up */
  at_disable_echo(rfcomm_fp);

  /* Default = get info/phonebook/messages */
  if(!argc) {
    at_parse_manufacturer_identification(rfcomm_fp);
    at_parse_identification(rfcomm_fp);
    at_parse_phonebook_list(rfcomm_fp);
    at_parse_message_list(rfcomm_fp);
    return;
  }

  /* Process given commands */
  for(i = 0; i < argc ; i++) {
    debug(("+ Command: AT%s\n\n", argv[i]));

    if(!strcmp(argv[i], "info")) {
      at_parse_manufacturer_identification(rfcomm_fp);
      at_parse_identification(rfcomm_fp);
    }
    else if(!strcmp(argv[i], "phonebook")) {
      at_parse_phonebook_list(rfcomm_fp);
    }
    else if(!strcmp(argv[i], "messages")) {
      at_parse_message_list(rfcomm_fp);
    }
    else if(!strcmp(argv[i], "dial")) {
      if(i + 1 >= argc)
	Usage("You need to specify the number for 'dial'");
      at_dial(rfcomm_fp, argv[++i]);
    }
    /*
    else if(!strcmp(argv[i], "sms")) {
      if(i + 2 >= argc) 
        Usage("You need to specify the number and text for 'sms'");
      at_sms(rfcomm_fp, argv[i + 1], argv[i + 2]);
      i = i + 2;
    }
    */
    else {
      at_custom(rfcomm_fp, argv[i]);
    }

  }

  return;
}


/*
 * Misc funcs
 */

/* Usage */
static void Usage(const char *err_msg)
{
  printf("Usage: bluebugger [OPTIONS] -a <addr> [MODE]                            \n\n" \
         "       -a <addr>     = Bluetooth address of target                      \n\n");

  printf("       Options:                                                           \n" \
	 "       --------                                                           \n");
  printf("       -m <name>     = Name to use when connecting (default: '')          \n" \
         "       -d <device>   = Device to use (default: '/dev/rfcomm')             \n" \
         "       -c <channel>  = Channelto use (default: 17)                        \n" \
	 "       -n            = No device name lookup                              \n" \
         "       -t <timeout>  = Timeout in seconds for name lookup (default: 5)    \n" \
	 "       -o <file>     = Write output to <file>                             \n");
								
  printf("\n");

  printf("       Mode:                                                              \n" \
         "       -----                                                              \n");

  printf("       info                   = Read Phone Info   (default)               \n" \
	 "       phonebook              = Read Phonebook    (default)               \n" \
         "       messages               = Read SMS Messages (default)               \n" \
	 "       dial <num>             = Dial number                               \n" \
	 "       ATCMD                  = Custom Command (e.g. '+GMI')              \n");
  printf("                                                                          \n");
  printf("       Note: Modes can be combined, e.g. 'info phonebook +GMI'            \n");

  printf("\n* %s\n\n", err_msg);

  exit(EXIT_FAILURE);
}

static void
sig_handler(int signo)
{
  /* close rfcomm device */
  if(rfcomm_fp)
    fclose(rfcomm_fp);

  /* Cleanup */
  if(sock != -1 && dev_id != -1) {
    bt_release(sock, dev_id);
    close(sock);
  }

  if(hci_sock != -1)
    hci_close_dev(hci_sock);

  /* Cleanup */
  SAFE_DELETE(name);
  SAFE_DELETE(device);
  SAFE_DELETE(target_addr);

  exit(EXIT_FAILURE);
}
     

/* EOF */
