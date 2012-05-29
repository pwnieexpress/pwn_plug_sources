#ifndef MPD_BT_H
#define MPD_BT_H

#include <stdio.h>
#include <bluetooth/bluetooth.h>

int  bt_configure(int, int, char *);
int  bt_setup();
int  bt_rfcomm_config(int);
int  bt_release(int, int);
int  bt_bind(int, int, bdaddr_t *, int);

#endif /* MPD_BT_H */
