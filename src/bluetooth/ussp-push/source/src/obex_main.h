/*
 *  ussp-push by Davide Libenzi ( OBEX file pusher )
 *  Copyright (C) 2005  Davide Libenzi
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#if !defined(_OBEX_MAIN_H)
#define _OBEX_MAIN_H

#include <stdint.h>
#include <openobex/obex.h>


#define EOBEX_OK 0
#define EOBEX_ABORT 1
#define EOBEX_HUP 2
#define EOBEX_PARSE 3


/*
 * This struct came from the affix/include/affix/obex.h file, it holds all the
 * information for a client connection like what operation is currently being
 * carried out and the openobex connection handle.
 */
typedef struct client_context {
	int serverdone;
	int clientdone;
	char *get_name;	/* Name of last get-request */
	int rsp;		/* error code */
	int opcode;
	char *arg;		/* response storage place */
	uint32_t con_id;		/* connection ide */
	void *private;
} client_context_t;



#endif

