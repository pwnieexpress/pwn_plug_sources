/*
 * Copyright (C) 2006 by Martin J. Muench <mjm@codito.de>
 *
 * Part of mpd - mobile phone dumper
 *
 * Some code stolen from btxml.c by Andreas Oberritter
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "bt.h"

/* setup bt socket */
int bt_configure(int dev_id, int s, char *name)
{
  struct hci_dev_req dr;
  change_local_name_cp cp;

  dr.dev_id = dev_id;
  dr.dev_opt = AUTH_DISABLED;

  if (ioctl(s, HCISETAUTH, &dr) != 0) {
    fprintf(stderr, "HCISETAUTH failed: %s\n", strerror(errno));
    return 1;
  }

  dr.dev_opt = ENCRYPT_DISABLED;

  if (ioctl(s, HCISETENCRYPT, &dr) != 0) {
    fprintf(stderr, "HCISETAUTH failed: %s\n", strerror(errno));
    return 1;
  }

  memset(cp.name, ' ', CHANGE_LOCAL_NAME_CP_SIZE);
  if(name != NULL)
    memcpy(cp.name, name, CHANGE_LOCAL_NAME_CP_SIZE);


  if(hci_send_cmd(s, OGF_HOST_CTL, OCF_CHANGE_LOCAL_NAME,
		  CHANGE_LOCAL_NAME_CP_SIZE, (void *) &cp) < 0) {
    fprintf(stderr, "OCF_CHANGE_LOCAL_NAME failed: %s\n", strerror(errno));
    return 1;
  }

  return 0;
}

/* bind bt socket */
int bt_bind(int sock, int dev_id, bdaddr_t *bdaddr, int channel)
{
  struct rfcomm_dev_req req;
  int ret=0;

  req.dev_id = dev_id;
  req.flags = 0;
  bacpy(&req.src, BDADDR_ANY);
  bacpy(&req.dst, bdaddr);
  req.channel = channel;

  if (ioctl(sock, RFCOMMCREATEDEV, &req) == 0)
    return 0;

  if (errno != EADDRINUSE)
    fprintf(stderr, "RFCOMMCREATEDEV failed: %s\n", strerror(errno));
  else if ((ret = bt_release(sock, dev_id)))
    ;
  else if ((ret = ioctl(sock, RFCOMMCREATEDEV, &req)))
    fprintf(stderr, "RFCOMMCREATEDEV failed: %s\n", strerror(errno));

  return ret;
}

/* set rfcomm options */
int bt_rfcomm_config(int fd)
{
  struct termios t;
  int ret;

  if ((ret = tcgetattr(fd, &t)))
    fprintf(stderr, "tcgetattr failed: %s\n", strerror(errno));
  else {
    t.c_iflag = IGNBRK;
    t.c_oflag = 0;
    t.c_cflag = CLOCAL | CREAD | CS8 | B115200;
    t.c_lflag = 0;
    t.c_line = 0;
    t.c_ispeed = B115200;
    t.c_ospeed = B115200;

    if ((ret = tcsetattr(fd, TCSADRAIN, &t)))
      fprintf(stderr, "tcsetattr failed: %s\n", strerror(errno));
  }

  return ret;
}

/* deinit rfcomm */
int bt_release(int sock, int dev_id)
{
  struct rfcomm_dev_req req;
  int ret;

  req.dev_id = dev_id;
  req.flags = 0;
  bacpy(&req.src, BDADDR_ANY);
  bacpy(&req.dst, BDADDR_ANY);
  req.channel = 0;

  if ((ret = ioctl(sock, RFCOMMRELEASEDEV, &req)))
    fprintf(stderr, "RFCOMMRELEASEDEV failed: %s\n", strerror(errno));

  return ret;
}

