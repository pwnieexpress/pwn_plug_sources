/*
 *  $Id$
 */
 
/*
 *  sobexsrv (c) Collin R. Mulliner <collin(AT)betaversion.net>
 *               web: http://www.mulliner.org/bluetooth/sobexsrv.php
 *
 *  license: GPLv2
 *
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <netinet/in.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#ifndef __OBEXSRV_H__
#define __OBEXSRV_H__

typedef int (*handler_func)(char*, char*);

#define OBEX_FTP_TARGET 0xF9,0xEC,0x7B,0xC4,0x95,0x3C,0x11,0xD2,0x98,0x4E,0x52,0x54,0x00,0xDC,0x9E,0x09
#define OBEX_FTP_FOLDER_LISTING_START "<?xml version=\"1.0\"?>\n<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\n<folder-listing version=\"1.0\">\n"
#define OBEX_FTP_FOLDER_LISTING_START_PARENT "<?xml version=\"1.0\"?>\n<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\n<folder-listing version=\"1.0\">\n<parent-folder />\n"
#define OBEX_FTP_FOLDER_LISTING_END "</folder-listing>\n"

#define SOBEXSRV_CAPABILITY_STR ""\
"<?xml version=\"1.0\"?>\n"\
"<!-- OBEX Capability Object -->\n"\
"<!DOCTYPE Capability SYSTEM \"obex-capability.dtd\">\n\n"\
"<Capability Version=\"1.0\">\n"\
" <!-- General Purpose information -->\n"\
" <General>\n"\
"  <Manufacturer>Collin Mulliner (http://www.mulliner.org/bluetooth/sobexsrv.php)</Manufacturer>\n"\
"  <Model>sobexsrv</Model>\n"\
"  <OEM>BlueZ</OEM>\n"\
"  <SW Version=\"%s\" />\n"\
" </General>\n"\
" <!-- Service Object Definitions -->\n"\
" <Service>\n"\
"  <Name>Folder-Browsing</Name>\n"\
"  <UUID>F9EC7BC4-953C-11d2-984E-525400DC9E09</UUID>\n"\
"  <Object>\n"\
"   <Type>x-obex/folder-listing</Type>\n"\
"  </Object>\n"\
"  <Access>\n"\
"   <Protocol>Bluetooth</Protocol>\n"\
"   <Target>F9EC7BC4-953C-11d2-984E-525400DC9E09</Target>\n"\
"  </Access>\n"\
" </Service>\n"\
"</Capability>\n"


/* start OPENOBEX structs - DONT CHANGE! */
typedef union
{
	struct sockaddr_in inet;
	struct sockaddr_rc rfcomm;
} saddr_t;

typedef struct obex_transport
{
	int type;
	int connected;    /* Link connection state */
	unsigned int mtu; /* Tx MTU of the link */
	saddr_t self;     /* Source address */
	saddr_t peer;     /* Destination address */
} obex_transport_t;

typedef struct obex_internal
{
	uint16_t mtu_tx;     /* Maximum OBEX TX packet size */
	uint16_t mtu_rx;     /* Maximum OBEX RX packet size */
	uint16_t mtu_tx_max; /* Maximum TX we can accept */

	int fd;             /* Socket descriptor */
	int serverfd;
	int writefd;        /* write descriptor - only OBEX_TRANS_FD */
	unsigned int state;
        
	int keepserver; /* Keep server alive */
	int filterhint; /* Filter devices based on hint bits */
	int filterias;  /* Filter devices based on IAS entry */

	void *tx_msg; /* Reusable transmit message */
	void *rx_msg; /* Reusable receive message */

	obex_object_t *object; /* Current object being transfered */      
	obex_event_t eventcb;  /* Event-callback */

	obex_transport_t trans; /* Transport being used */
	obex_ctrans_t ctrans;
	void *userdata;         /* For user */
} obex_internal_t;
/* end OPENOBEX structs - DONT CHANGE! */

#define UNNAMED_PUT_FILENAME "unnamed_file"

typedef struct server_context_t
{
	char *handler_script;  /**< handler executable */
	int mode;              /**< 0=script 1=internal */
	char *rootdir;         /**< default inbox */
	int chroot;            /**< do chroot */
	char *base_path;	     /**< default inbox */
	char lsdir_params[10]; /**< folder-list options */
	char *mime_type_dir;   /**< mime-type dir. */
	int folder_listing;    /**< enabled folder listing 1=yes, 0=no */
	int syslog;            /**< use syslog 1=yes, 0=no */
	int daemon;            /**< become daemon 1=yes, 0=no */
	int policy;            /**< security policy 1=public, 2=auth, 3=enc */
	
	bdaddr_t local_address;       /**< BD address to bind to */
	unsigned short int channel;   /**< the RFCOMM channel */
	int sdp_opush;                /**< publish OPUSH record in SDP */
	int sdp_ftp;                  /**< publish OBEXFTP record in SDP */
	
	sdp_record_t *sdp_record[2]; /**< SDP record */
	sdp_session_t *sdp_session;  /**< SDP session */

	int server;	                  /**< server socket */
	struct sockaddr_rc srv_addr;	/**< server address */

	/* connection specific stuff */
	unsigned int connection_id;  /**< connection id */
	bdaddr_t remote_address;     /**< remote address */
	
	char *target;      /**< target */
	int target_length; /**< length of target */
	char *filename;    /**< filename get/put */
	char *path;        /**< path (setpath) */ 
	char *type;        /**< type */
	char *description; /**< description */
	int count;         /**< count */
	
	uint8_t *data;            /**< data */
	int data_type;            /**< type of data 0=nodata, 1=data, 2=empty*/
	unsigned int data_length; /**< length of data */
	
	obex_t *libObex;	/**< obex instance */
	
	int security;    /**< current security setting */
	
	int client;	                 /**< client socket */
	struct sockaddr_rc clt_addr; /**< client address */

	int pid; /**< client process pid */
	
} server_context;

#endif

/*
 *  $Log$
 */
