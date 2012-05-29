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
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <getopt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <openobex/obex.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <assert.h>

#include "obexsrv.h"
#include "util.h"
#include "sdp.h"
#include "fs.h"
#include "pathcheck.h"

// define to not fork for debugging
//#define SINGLE

server_context context;
static handler_func handler;

void debug4(char *m1, char *m2, char *m3, char *m4)
{
	if (!context.daemon) fprintf(stderr, m1, m2, m3, m4);
}

void debug(char *m1)
{
	debug4(m1, NULL, NULL, NULL);
}

void debug2(char *m1, char *m2)
{
	debug4(m1, m2, NULL, NULL);
}

void debug3(char *m1, char *m2, char *m3)
{
	debug4(m1, m2, m3, NULL);
}

void UnicodeToChar(uint8_t *c, const uint8_t *uc, int size)
{
	iconv_t dsc = iconv_open(nl_langinfo(CODESET),"UTF16BE");
	size_t isiz,osiz;
	char *ip = (char*)uc;
	char *op = (char*)c;
	isiz = osiz=size;

	if (iconv(dsc,&ip,&isiz,&op,&osiz) == -1) {
		debug2("iconv: error %s\n", strerror(errno));
		OBEX_UnicodeToChar(c,uc,size);
	}
	iconv_close(dsc);
}

void dosyslog(char *m1, char *m2, char *m3)
{
	char log[2048] = {0};
	char ra[20];

	// logging on?
	if (!context.syslog) return;
	
	ba2str(&context.remote_address, ra);

	// print remote address and format string
	snprintf(log, 2048, "[%s] %s", ra, m1);
	// execute format string
	snprintf(log+strlen(log), 2048-strlen(log), m2, m3);
	
	syslog(LOG_INFO, "%s", log);
}

int security(int mode)
{
	int res = 0;
	int opt;
		
	debug3("%s: mode = %d\n", (char*)__func__, (char*)mode);
	
	switch (mode) {
	case 0:
		return(0);
		break;
	case 1:
		// don't trigger security mode-2
		res = 1;
		break;
	case 2:
		opt = RFCOMM_LM_AUTH;
		if (setsockopt(context.client, SOL_RFCOMM, RFCOMM_LM, &opt, sizeof(opt)) < 0) {
			perror("security setsockopt authenticate: \n");
			dosyslog("failed authenticating connection\n", 0,0);
		}
		else {
			res = 1;
			dosyslog("authenticated connection\n", 0,0);
		}
		break;
	case 3:
		opt = RFCOMM_LM_SECURE;
		if (setsockopt(context.client, SOL_RFCOMM, RFCOMM_LM, &opt, sizeof(opt)) < 0) {
			perror("security setsockopt authenticate+enc (secure): \n");
			dosyslog("failed securing connection\n", 0,0);
		}
		else {
			res = 1;
			dosyslog("secured connection\n", 0,0);
		}
		break;
	case 4:
		debug2("%s: wait until connect before security mode is decided\n", (char*)__func__);
		return(1);
		break;
	}

	context.security = mode;
	return(res);
}

/*
 *  build fullpath
 */
char* getfullpath()
{
	char *fullpath = NULL;

	assert(context.filename != NULL);

	if (context.chroot) {
		if (context.path && context.filename[0] != '/') {
			fullpath = (char*) malloc(strlen(context.path) + strlen(context.filename) + 2);
			assert(fullpath != NULL);
			sprintf(fullpath, "%s/%s", context.path, context.filename);
		}
		else {
			fullpath = (char*) malloc(strlen(context.filename) + 2);
			assert(fullpath != NULL);
			sprintf(fullpath, "/%s", context.filename);
		}
	}
	else {
		char *tmp;
		
		if (context.path && context.filename[0] != '/') {
			fullpath = (char*) malloc(strlen(context.rootdir) + strlen(context.path) + strlen(context.filename) + 3);
			assert(fullpath != NULL);
			sprintf(fullpath, "%s/%s/%s", context.rootdir, context.path, context.filename);
		}
		else {
			fullpath = (char*) malloc(strlen(context.rootdir) + strlen(context.filename) + 2);
			assert(fullpath != NULL);
			sprintf(fullpath, "%s/%s", context.rootdir, context.filename);
		}
		
		tmp = resolve_path(fullpath);
		free(fullpath);
		fullpath = tmp;
		if (strncmp(fullpath, context.rootdir, strlen(context.rootdir)) != 0) {
			debug3("%s: path check failed for: %s\n", (char*)__func__, fullpath);
			free(fullpath);
			fullpath = NULL;
		}
	}
	
	return(fullpath);
}

int internal_handler(char *command, char *friendlyname)
{
	const unsigned char obex_ftp_target[] = {OBEX_FTP_TARGET};
	const char root_path[] = {"/"};
	char *fullpath = NULL;
	int fp;
	int ret = 0;
	unsigned int fsize = 0;
	
		
	if (strcmp(friendlyname, "get") == 0) {
		// get capabilities
		if (context.type && strcmp(context.type, "x-obex/capability") == 0) {
			context.data_length = sizeof(SOBEXSRV_CAPABILITY_STR)+sizeof(VERSION_STRING);
			if ((context.data = (unsigned char*) malloc(context.data_length+1)) == NULL) {
				perror("internal capability malloc:");
				exit(-1);
			}
			sprintf((char*)context.data, SOBEXSRV_CAPABILITY_STR, VERSION_STRING);
			
			dosyslog("pulling x-obex/capability object\n", 0,0);
			return(1);
		}
		// directory listing
		else if (context.type && strcmp(context.type, "x-obex/folder-listing") == 0) {
			// check target
			if (memcmp(context.target, obex_ftp_target, context.target_length) == 0) {
				char *data = NULL;
				char *path = NULL;
				int ret = 0;
				
				// don't allow folder listing
				if (!context.folder_listing) goto deny_folder_listing;
				
				if (context.chroot) {
					if (!context.path) {
						path = (char*) root_path;
					}
					else {
						path = context.path;
					}
				}
				else {
					if (context.path) {
						if ((path = (char*) malloc(strlen(context.path) + strlen(context.rootdir) + 2)) == NULL) {
							perror("internal_handler pre-lsdir malloc");
							exit(-1);
						}
						sprintf(path, "%s/%s", context.rootdir, context.path);
					}
					else {
						path = context.rootdir;
					}
				}
				
				debug3("%s: lsdir for: \"%s\"\n", (char*)__func__, path);
				
				lsdir(path, context.lsdir_params, &data);
				if (data != NULL) {
					if ((context.data = (unsigned char*) malloc(strlen(data)+sizeof(OBEX_FTP_FOLDER_LISTING_START_PARENT)+sizeof(OBEX_FTP_FOLDER_LISTING_END)+1)) == NULL) {
						perror("internal_handler lsdir malloc");
						exit(-1);
					}
					context.data_length = sprintf((char*)context.data, "%s%s%s", OBEX_FTP_FOLDER_LISTING_START_PARENT, data, OBEX_FTP_FOLDER_LISTING_END);
					free(data);
					
					debug2("%s", (char*)context.data);
					ret = 1;
				}
				else {
					debug2("%s: lsdir didn't return data!\n", (char*)__func__);
					goto out_err_folder_listing;
				}
				
				dosyslog("folder listing for \"%s\"\n", path, 0);
out_err_folder_listing:
				if (context.path && !context.chroot) free(path);				
				return(ret);
			}
			else {
				debug2("%s: target check failed\n", (char*)__func__);
deny_folder_listing:
				return(0);
			}
		}
		// handle generic mime type get
		else if (context.type && !context.filename) {
			debug3("%s: generic mime request for: %s\n", (char*)__func__, context.type);
			
			if (strstr(context.type, "..")) {
				debug2("%s: mime type contains unsafe \"..\"\n", (char*)__func__);
				return(0);
			}
			if (context.mime_type_dir) {			
				if ((fullpath = (char*) malloc(strlen(context.mime_type_dir) + strlen(context.type) + 2)) == NULL) {
					perror("internal handler generic mime malloc:");
					exit(-1);
				}

				bzero(fullpath, strlen(context.mime_type_dir) + strlen(context.type) + 2);
				sprintf(fullpath, "%s/%s", context.mime_type_dir, context.type);
				
				goto generic_mime_type_get;
			}
			else {
				debug2("%s: no mime_type_dir specified\n", (char*)__func__);
				return(0);
			}
		}
		// normal get
		else if (context.filename) {			
			if ((fullpath = getfullpath()) == NULL) {
				debug2("%s: invalid pathname\n", (char*)__func__);
				dosyslog("pull failed (invalid pathname)\n", 0, 0);
				return(0);
			}

			/*
			 *  this is for the generic mime type get
			 *  - THIS IS UGLY, I KNOW 
			 */
			if (1 == 2) {
generic_mime_type_get:
				debug2("%s: get from generic mime type\n", (char*)__func__);
			}
			
			debug3("%s: get for \"%s\"\n", (char*)__func__, fullpath);
		
			if ((fp = open(fullpath, O_RDONLY)) < 0) {
				goto get_out;
			}
			fsize = lseek(fp, 0, SEEK_END);
			lseek(fp, 0, SEEK_SET);
		
			context.data_length = fsize;
			if ((context.data = malloc(fsize)) == NULL) {
				perror("internal handler get malloc:");
				exit(-1);
			}
			if (cread(fp, context.data, fsize) == fsize) {
				ret = 1;
			}
			
get_out:
			if (ret) dosyslog("pulling \"%s\"\n", fullpath, 0);
			else dosyslog("failed pulling \"%s\"\n", fullpath, 0);
			close(fp);
			if (fullpath != NULL) free(fullpath);
			return(ret);
		}
		else {
			debug2("%s: get without filename!\n", (char*)__func__);
			return(0);
		}
	}
	else if (strcmp(friendlyname, "put") == 0) {
		if ((fullpath = getfullpath()) == NULL) {
			debug2("%s: invalid pathname\n", (char*)__func__);
			dosyslog("push failed (invalid pathname)\n", 0, 0);
			return(0);
		}
		
		debug4("%s: put for \"%s\" length=%d\n", (char*)__func__, fullpath, (char*)context.data_length);
		
		if ((fp = open(fullpath, O_WRONLY|O_CREAT, 0666)) < 0) {
			goto put_out;
		}
		if (nwrite(fp, context.data, context.data_length) < context.data_length) {
			goto put_out;
		}
		ret = 1;
		
put_out:
		if (ret) dosyslog("pushing \"%s\"\n", fullpath, 0);
		else dosyslog("failed pushing \"%s\"\n", fullpath, 0);
		if (fullpath != NULL) free(fullpath);
		close(fp);
		return(ret);
	}
	else if (strcmp(friendlyname, "delete") == 0) {
		struct stat di;
		/*
		if (context.path && context.filename) {
			fullpath = (char*) malloc(strlen(context.path) + 257);
			sprintf(fullpath, "%s/%s", context.path, context.filename);
		}
		else {
			return(0);
		}
		*/
		if ((fullpath = getfullpath()) == NULL) {
			debug2("%s: delete: invalid path\n", (char*)__func__);
			dosyslog("delete failed (invalid path)\n", 0, 0);
			return(0);
		}
		
		debug3("%s: delete for \"%s\"\n", (char*)__func__, fullpath);
	
		if (stat(fullpath, &di) == 0) {
			if (S_ISDIR(di.st_mode)) {
				if (rmdir(fullpath) == 0) {
					ret = 1;
				}
				else {
					debug3("%s: deleting directory \"%s\" failed\n", (char*)__func__, fullpath);
				}
			}
			else {
				if (unlink(fullpath) == 0) {
					ret = 1;
				}
				else {
					debug3("%s: deleting file \"%s\" failed\n", (char*)__func__, fullpath);
				}
			}
		}
		
		if (ret) dosyslog("deleting \"%s\"\n", fullpath, 0);
		else dosyslog("failed deleting \"%s\"\n", fullpath, 0);
		if (fullpath != NULL) free(fullpath);
		return(ret);
	}
	else if (strcmp(friendlyname, "setpath") == 0) {
		struct stat di;
		
		/*
		 *  just return ok if the path is null
		 *  this brings you back to the default inbox
		 */
		if (!context.path) {
			return(1);
		}
		
		if ((fullpath = getfullpath()) == NULL) {
			return(0);
		}
		
		/*
		if (context.chroot) {
			if (context.path) {
				fullpath = context.path;
			}
			else {
				context.path = (char*) root_path;
			}
		}
		else {
			if (context.path) {
				fullpath = (char*) malloc(strlen(context.rootdir) + strlen(context.path) + 2);
				sprintf(fullpath, "%s/%s", context.rootdir, context.path);
			}
			else {
				fullpath = (char*) malloc(strlen(context.rootdir) + 1);
				sprintf(fullpath, "%s", context.rootdir);
			}
		}
		*/
		
		debug3("%s: setpath for \"%s\"\n", (char*)__func__, fullpath);
		
		if (stat(fullpath, &di) == 0) {
			if (S_ISDIR(di.st_mode)) {
				ret = 1;
			}
		}
		else {
			if (mkdir(fullpath, 0777) == 0) {
				ret = 1;
				dosyslog("created directory \"%s\"\n", fullpath, 0);
			}
			else {
				debug3("%s: mkdir \"%s\" failed\n", (char*)__func__, fullpath);
				ret = 0;
				dosyslog("failed to create directory \"%s\"\n", fullpath, 0);
			}
		}
		
		if (!context.chroot) free(fullpath);
		return(ret);
	}
	else if (strcmp(friendlyname, "accept") == 0) {
		/*
		 *  return security settings
		 */
		dosyslog("connecting...\n", 0,0);
		return(context.policy);
	}
	else if (strcmp(friendlyname, "connect") == 0) {
		/*
		 *  accept does the security settings
		 */
		dosyslog("connected\n", 0,0);
		return(1);
	}
	
	return(1);
}

int scripting_handler(char *command, char *friendlyname)
{
	#define BUFSIZE 4096
	char *buf = NULL;
	char local[20];
	char remote[20];
	int handler_res;
	int pid;
	int rd[2];
	int wr[2];
	int i;
	

	if ((buf = malloc(BUFSIZE)) == NULL) {
		debug2("%s: can't alloc memory\n", (char*)__func__);
		exit(-1);
	}
	
	ba2str(&context.local_address, local);
	ba2str(&context.remote_address, remote);
	
	snprintf(buf, BUFSIZE, "command: %s\nfriendly: %s\nlocal: %s\nremote: %s\n",
		friendlyname, command, local, remote);
	if (context.filename != NULL) {
		snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "filename: %s\n",
			context.filename);
	}
	if (context.type != NULL) {
		snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "type: %s\n", context.type);
	}
	if (context.description != NULL) {
		snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "description: %s\n",
			context.description);
	}
	if (context.path != NULL) {
		snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "path: %s\n",
			context.path);
	}
	if (context.target != NULL) {
		/*  
		 *  figure out howto print the target name this is rather fuzzy
		 *  currently we only detect hex or ascii
		 */
		if ((unsigned char)context.target[0] > 'z') {
			snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "target: ");
			for (i = 0; i < context.target_length; i++) {
				snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "%02X",
					(unsigned char)context.target[i]);
			}
			snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "\n");
		}
		else {
			snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "target: %s\n",
				context.target);
		}
	}
	snprintf(buf+strlen(buf), BUFSIZE-strlen(buf), "length: %u\ncount: %u\n"\
		"connection_id: %u\ndata_type: %d\nserver_session: %d\n\n", context.data_length,
		context.count, context.connection_id, context.data_type, context.pid);
		
	if (pipe(rd) != 0 || pipe(wr) != 0) {
		perror("handler: can't create pipes\n");
		exit(-1);
	}
	
	// child process
	if ((pid = fork()) == 0) {
		
		close(rd[0]);
		close(wr[1]);
		close(0);
		close(1);
		dup(wr[0]);
		dup(rd[1]);
		if (execl((const char*)context.handler_script, (const char*)
				context.handler_script, NULL) == -1) {
			perror("handler failed at execl: ");
			exit(-1);
		}
	}
	
	// mother process
	close(rd[1]);
	close(wr[0]);
	
	debug2("handler: header buf size %d\n", (char*)strlen(buf));
	if (nwrite(wr[1], buf, strlen(buf)) <= 0) {
		debug("handler: write failed\n");
		exit(-1);
	}
	free(buf);
	buf = NULL;
	
	if (strcmp(command, "put") == 0) {
		if (context.data_type == 1) {
			if (context.data == NULL) {
				debug("handler: put with no body data\n");
				exit(-1);
			}
			if (strcmp(friendlyname, "puthint") == 0) {
				// only hint client about put
			}
			else {
				if (nwrite(wr[1], context.data, context.data_length) <= 0) {
					debug("handler: put body write failed\n");
					exit(-1);
				}
			}
		}
	}
	else if (strcmp(command, "get") == 0) {
		int i = 0;
		char *p;
		
		buf = malloc(1024);
		bzero(buf, 1024);
		
		do {
			if (cread(rd[0], &buf[i], 1) <= 0) {
				debug("handler: get read failed\n");
				break;
			}
			if (i > 2) {
				if (buf[i-1] == '\n' && buf[i] == '\n') break;
			}
			i++;
		} while (i < 1023);
		
		if ((p = strstr(buf, "length:")) != NULL) {
			context.data_length = strtol(p+7, NULL, 10);
			if (context.data_length < 0) {
				debug2("handler: get length error (%d)\n", (char*)context.data_length);
				exit(-1);
			}
			debug2("handler: get length %d\n", (char*)context.data_length);
			
			if (context.data_length > 0) {
				if ((context.data = malloc(context.data_length)) == NULL) {
					debug("handler: get data malloc failed\n");
					exit(-1);
				}
				if ((cread(rd[0], context.data, context.data_length)) < context.data_length) {
					debug("handler: get data read failed\n");
					exit(-1);
				}
			}
		}
		else {
			debug("handler: get result parse error\n");
			exit(-1);
		}
	}

	// close descriptors and wait for child
	close(wr[1]);
	close(rd[0]);
	do {
		waitpid(pid, &handler_res, 0); // , WNOHANG);
	} while (!WIFEXITED(handler_res));
	
	debug2("handler: returned with exit code %d\n", (char*)WEXITSTATUS(handler_res));
	return(WEXITSTATUS(handler_res));
}

void context_clear(int all)
{
	if (context.filename) {
		free(context.filename);
		context.filename = NULL;
	}
	if (context.type) {
		free(context.type);
		context.type = NULL;
	}
	if (context.description) {
		free(context.description);
		context.description = NULL;
	}
	if (context.data) {
		free(context.data);
		context.data = NULL;
	}
	context.data_length = 0;
	context.data_type = 0;
	context.count = 0;
	
	if (all) {
		if (context.target) {
			free(context.target);
			context.target = NULL;
			context.target_length = 0;
		}
		if (context.path) {
			free(context.path);
			context.path = NULL;
		}
		context.connection_id = 0;
		bzero(&context.remote_address, sizeof(context.remote_address));
	}
}

int cmd_connect(obex_t *handle, obex_object_t *oo)
{
	obex_headerdata_t hv;
	uint8_t hi;
	unsigned int hlen;
	int i;
	int handler_res = 0;
	int ret = 0;
	

	debug("CONNECT start\n");

	while (OBEX_ObjectGetNextHeader(handle, oo, &hi, &hv, &hlen))	{
		switch (hi) {
		case OBEX_HDR_CONNECTION:
			context.connection_id = hv.bq4;
			
			debug2("CONNECT connection id: %d\n", (char*)hv.bq4);
			break;
		case OBEX_HDR_TARGET:
			if ((context.target = malloc(hlen+1))) {
				bzero(context.target, hlen+1);
				memcpy(context.target, hv.bs, hlen);
				context.target_length = hlen;
				
				debug("CONNECT target ");
				if ((unsigned char)context.target[0] > 'z') {
					for (i = 0; i < hlen; i++) {
						debug2("%0.2X", (char*)((int)hv.bs[i]));
					}
					debug("\n");
				}
				else {
					debug2("%s\n", context.target);
				}
			}
			else {
				debug("CONNECT target malloc error\n");
			}
			break;
		default:
			debug2("CONNECT skipped header %02x\n", (char*)((int)hi));
			break;
		}
	}

	handler_res = handler("connect", "connect");
	
	switch (handler_res) {
	case -1:
		debug("CONNECT handler error\n");
		exit(-1);
		break;
	case 1:
	case 2:
	case 3:
		debug2("CONNECT ok, result = %d\n", (char*)handler_res);
		if (context.security == 4 || context.security == 0) {
			if (security(handler_res)) {
				OBEX_ObjectSetRsp(oo, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
				ret = 1;
			}
			else {
				// invoking security mode failed
				goto err;
			}
		}
		else {
			ret = 1;
			// security was already invoked
			OBEX_ObjectSetRsp(oo, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		}
		break;
	default:
		debug("CONNECT deny (fix me!)\n");
		// FIX ME
err:
		OBEX_ObjectSetRsp(oo, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
		break;
	}

	debug("CONNECT end\n");

	return(ret);
}

int cmd_disconnect(obex_t *handle, obex_object_t *oo)
{
	debug("DISCONNECT start\n");

	OBEX_ObjectSetRsp(oo, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
	
	if (context.mode) dosyslog("disconnecting...\n", 0,0);
	
	debug("DISCONNECT end\n");
	
	return(1);
}

int cmd_setpath(obex_t *handle, obex_object_t *oo)
{
	obex_headerdata_t hv;
	uint8_t hi;
	unsigned int hlen;
	int ispath = 0;
	int ret;

	
	debug("SETPATH start\n");

	while (OBEX_ObjectGetNextHeader(handle, oo, &hi, &hv, &hlen))	{
		switch (hi) {
		case OBEX_HDR_NAME:
			ispath = 1;
			if (hlen > 0) {
				char *tmp;
				
				if (hlen == 2) {
					// some clients send no-zero length names with just 0
					if (hv.bs[0] == 0 && hv.bs[1] == 0) {
						goto nonamehdr;
					}
				}
				// absolute path
				if (hv.bs[1] == '/') {
					if (context.path) {
						free(context.path);
						context.path = NULL;
					}
					if ((context.path = (char*) malloc(hlen + 1))) {
						bzero(context.path, hlen + 1);
						UnicodeToChar((unsigned char*)context.path, hv.bs, hlen);
					}
					else {
						perror("cmd_setpath malloc:");
						exit(-1);
					}
				}
				// relative path
				else {
					if (context.path) {
						if ((tmp = (char*) malloc(strlen(context.path) + hlen + 1))) {
							bzero(tmp, strlen(context.path) + hlen + 1);
							sprintf(tmp, "%s/", context.path);
							UnicodeToChar((unsigned char*)(tmp + strlen(tmp)), hv.bs, hlen);
							free(context.path);
							context.path = tmp;
						}
						else {
							perror("cmd_setpath malloc:");
							exit(-1);
						}
					}
					else {
						if ((tmp = (char*) malloc(hlen + 2))) {
							bzero(tmp, hlen + 2);
							strcpy(tmp, "/");
							UnicodeToChar((unsigned char*)(tmp + strlen(tmp)), hv.bs, hlen);
							context.path = tmp;
						}
						else {
							perror("cmd_setpath malloc:");
							exit(-1);
						}
					}
				}
				// clean path
				tmp = resolve_path(context.path);
				free(context.path);
				context.path = tmp;
			}
			else {
				if (context.path) free(context.path);
				context.path = NULL;
				debug("SETPATH with empty name\n");
			}
			debug2("SETPATH path %s\n", context.path);
			break;
		default:
			debug2("SETPATH skipped header %02x\n", (char*)((int)hi));
			break;
		}
	}

	/*
	 *  treat no name_hdr as empty name_hdr
	 */
	if (!ispath) {
		debug("SETPATH no name header\n");
nonamehdr:
		if (context.path) free(context.path);
		context.path = NULL;
	}

	ret = handler("setpath", "setpath");
	
	switch (ret) {
	case -1:
		debug("SETPATH handler error\n");
		exit(-1);
		break;
	case 1:
		debug("SETPATH ok\n");
		OBEX_ObjectSetRsp(oo, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		//hv.bq4 = context.connection_id;
		//OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), 0);
		break;
	default:
		debug("SETPATH deny (fix me!)\n");
		// FIX ME
		OBEX_ObjectSetRsp(oo, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
		break;
	}

	debug("SETPATH end\n");
	context_clear(0);
	return(1);
}

int cmd_get(obex_t *handle, obex_object_t *oo)
{
	obex_headerdata_t hv;
	uint8_t hi;
	unsigned int hlen;

	
	debug("GET start\n");

	while (OBEX_ObjectGetNextHeader(handle, oo, &hi, &hv, &hlen))	{
		switch (hi) {
		case OBEX_HDR_NAME:
			if ((context.filename = (char*) malloc(hlen + 1))) {
				bzero(context.filename, hlen + 1);
				UnicodeToChar((unsigned char*)context.filename, hv.bs, hlen);
				debug2("GET filename: %s\n", context.filename);
			}
			else {
				perror("cmd_get malloc:");
				exit(-1);
			}
			break;
		case OBEX_HDR_TYPE:
		if ((context.type = (char*) malloc(hlen + 1))) {
				bzero(context.type, hlen + 1);
				memcpy(context.type, hv.bs, hlen);	
				debug2("GET type: %s\n", context.type);
			}
			else {
				perror("cmd_get malloc:");
				exit(-1);
			}
			break;
		case OBEX_HDR_CONNECTION:
			debug2("GET connection %d\n", (char*)((int)hv.bq4));
			break;
		default:
			debug2("GET skipped header %02x\n", (char*)((int)hi));
			break;
		}
	}

	switch (handler("get", "get")) {
	case -1:
		debug("GET handler error\n");
		exit(-1);
		break;
	case 1:
		debug("GET ok\n");
		if (context.data_length > 0) {
			// FIX ME (more header fields?)
			OBEX_ObjectSetRsp(oo, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			hv.bs = context.data;
			OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_BODY, hv, context.data_length, 0);
			hv.bq4 = context.data_length;
			OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);
		}
		else {
			debug("GET ok (body length <= 0)\n");
			OBEX_ObjectSetRsp(oo, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			hv.bq4 = context.data_length;
			OBEX_ObjectAddHeader(handle, oo, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);
			// FIX ME
		}
		break;
	default:
		debug("GET deny (fix me!)\n");
		// FIX ME
		OBEX_ObjectSetRsp(oo, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
		break;
	}

	context_clear(0);
	return(1);
}

int cmd_put(obex_t *handle, obex_object_t *oo)
{
	obex_headerdata_t hv;
	uint8_t hi;
	unsigned int hlen;


	debug("PUT start\n");

	while (OBEX_ObjectGetNextHeader(handle, oo, &hi, &hv, &hlen))	{
		switch (hi) {
		case OBEX_HDR_BODY:
			context.data = (unsigned char*)hv.bs;
			context.data_length = hlen;
			if (hlen > 0) context.data_type = 1;
			else context.data_type = 2;
			
			debug2("PUT body length: %d\n", (char*)hlen);
			break;
		case OBEX_HDR_NAME:
			if ((context.filename = (char*) malloc(hlen + 1)))	{
				bzero(context.filename, hlen + 1);
				UnicodeToChar((unsigned char*)context.filename, hv.bs, hlen);

				debug2("PUT name: %s\n", context.filename);
			}
			else {
				perror("cmd_put malloc:");
				exit(-1);
			}
			break;
		case OBEX_HDR_TYPE:
			if ((context.type = (char*) malloc(hlen + 1))) {
				bzero(context.type, hlen + 1);
				memcpy(context.type, hv.bs, hlen);
								
				debug2("PUT type: %s\n", context.type);
			}
			else {
				perror("cmd_put malloc:");
				exit(-1);
			}
			break;
		case OBEX_HDR_LENGTH:
			if (context.data_length == 0) {
				context.data_length = hv.bq4;
			}
			else {
				debug3("PUT: length_header=%d body_length=%d WTF?\n", (char*)hv.bq4, (char*)context.data_length);
			}
			
			debug2("PUT length: %d\n", (char*)context.data_length);
			break;
		case OBEX_HDR_TIME:
			debug2("PUT time: %u (fix me!)\n", (char*)((int)hv.bq4));
			break;
		case OBEX_HDR_DESCRIPTION:
			if ((context.description = (char*) malloc(hlen + 1)))	{
				bzero(context.description, hlen + 1);
				UnicodeToChar((unsigned char*)context.description, hv.bs, hlen);
				debug2("PUT description: %s\n", context.description);
			}
			else {
				perror("cmd_put malloc:");
				exit(-1);
			}
			break;
		case OBEX_HDR_COUNT:
			context.count = hv.bq4;
			debug2("PUT count: %d\n", (char*)context.count);
			break;
		default:
			debug2("skipped header %02x\n", (char*)((int)hi));
			break;
		}
	}

	if (context.filename == NULL)	{
		debug("PUT warning - no name!\n");
	}

	debug2("PUT data_type %d\n", (char*)context.data_type);

	switch(handler("put", (context.data_length == 0) ? "delete" : "put")) {
	case -1:
		debug("PUT handler error\n");
		exit(-1);
		break;
	case 1:
		OBEX_ObjectSetRsp(oo, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		debug("PUT ok\n");
		break;
	default:
		// FIX ME
		OBEX_ObjectSetRsp(oo, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
		debug("PUT deny - fix me!\n");
		break;
	}

	debug("PUT end\n");
	/* 
	 *  set to null so clear context doesn't touch it
	 *  obexlib will free this memory!
	 */
	context.data = NULL;
	context_clear(0);
	return(1);
}

void eventhandler(obex_t *handle, obex_object_t *oo, int mode, int event, int obex_cmd, int obex_rsp)
{
	switch (event) {
	case OBEX_EV_PROGRESS:
		debug("PROGRESS\n");
		break;

	case OBEX_EV_REQDONE:
		switch (obex_cmd) {
		case OBEX_CMD_DISCONNECT:
			OBEX_TransportDisconnect(handle);
			break;
		default:
			debug("REQDONE\n");
			break;
		}
		break;
		
	case OBEX_EV_REQHINT:
		debug("REQHINT - add handler for this!\n");
		OBEX_ObjectSetRsp(oo, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;

	case OBEX_EV_REQ:
		switch (obex_cmd) {
		case OBEX_CMD_DISCONNECT:
			cmd_disconnect(handle, oo);
			break;
		case OBEX_CMD_CONNECT:
			if (!cmd_connect(handle, oo)) {
				debug2("%s: connect failed\b", (char*)__func__);
				goto disconnect;
			}
			break;
		case OBEX_CMD_SETPATH:
			if (context.security == 0 || context.security == 4) {
				if (!security(1))
					goto disconnect;
			}
			cmd_setpath(handle, oo);
			break;
		case OBEX_CMD_GET:
			if (context.security == 0 || context.security == 4) {
				if (!security(1))
					goto disconnect;
			}
			cmd_get(handle, oo);
			break;
		case OBEX_CMD_PUT:
			if (context.security == 0 || context.security == 4) {
				if (!security(1))
					goto disconnect;
			}
			cmd_put(handle, oo);
			break;
		default:
			if (context.security == 0 || context.security == 4) {
				if (!security(1))
					goto disconnect;
			}
			debug("REQ unknown\n");
			break;
		}
		break;
		
	case OBEX_EV_LINKERR:
		debug("LINKERR\n");
		break;
	
	case OBEX_EV_UNEXPECTED:
		// comes together with EV_PROGRESS ...
		break;
	
	case OBEX_EV_ABORT:
		debug("CLIENT ABORT\n");
		cmd_disconnect(handle, oo);
		break;
	
	default:
		printf("unknown event %d\n", event);
		break;
	}
	
	// end function here
	return;
	
disconnect:
	close(context.client);
	exit(1);	
}

void signal_handler(int sig)
{
	int st;
	
	switch (sig) {
	case SIGCHLD:
		while (wait3(&st, WNOHANG, NULL) >=0);
		break;
	case SIGTERM:
	case SIGINT:
		close(context.server);
		remove_sdp();
		if (context.syslog) closelog();
		exit(0);
		break;
	}
}

void usage(char *progname)
{
	fprintf(stderr, "syntax: %s -icHIrRofFdmpslSvh\n"\
		"-i [hciX|bdaddr] hci device or bdaddr to bind to\n"\
		"-c channel       rfcomm channel (default is first available)\n"\
		"-H handler       path to handler executable\n"\
		"-I               internal mode (don't use handler script!)\n"\
		"-r path          default directory for internal mode\n"\
		"-R               chroot to default directory (-r)\n"\
		"-o               don't register OPUSH\n"\
		"-f               don't register OBEXFTP\n"\
		"-F               disable folder-listing\n"\
		"-d               debug (don't become a daemon)\n"\
		"-m path          mime type path\n"\
		"-p               print used rfcomm channel\n"\
		"-s [1|2|3]       security mode-2\n"\
		"-l [ONPTD]       options for directory listing\n"\
		"-S               enable transfer log using syslog\n"\
		"-v               print version\n"\
		"-h               print this help message\n"\
		"\n"\
		"example: sobexsrv -l ONPTD -s 2 -iRS -r /tmp\n"\
		"", progname);
}

int main(int argc, char **argv)
{
	int opt;
	int print_channel = 0;
	unsigned int size;
	int handler_res;
	
	
	setlocale(LC_ALL,"");
	
	signal(SIGCHLD, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	memset(&context, 0, sizeof(server_context));
	context.sdp_opush = 1;
	context.sdp_ftp = 1;
	context.mode = 0; // scripting
	context.daemon = 1;
	strcpy(context.lsdir_params, "OPDT");
	context.policy = 1; // insecure
	context.rootdir = SOBEXSRV_CHROOT;
	context.chroot = 0;
	context.folder_listing = 1;
	
	handler = scripting_handler;
	
	while ((opt = getopt(argc, argv, "c:H:r:l:s:m:i:ofdpvIRhFS")) != EOF) {
		switch (opt) {
		case 'i':
			if (!strncmp(optarg, "hci", 3)) {
				if (hci_devba(atoi(optarg + 3), &context.local_address)) {
					fprintf(stderr, "%s is not a valid hci device name\n", optarg);
					exit(-1);
				}
			}
			else {
				if (str2ba(optarg, &context.local_address)) {
					fprintf(stderr, "%s is not a valid address\n", optarg);
					exit(-1);
				}
			}
			break;
		case 'c':
			context.channel = atoi(optarg);
			break;
		case 'm':
			context.mime_type_dir = optarg;
			break;
		case 'H':
			context.handler_script = optarg;
			break;
		case 'o':
			context.sdp_opush = 0;
			break;
		case 'f':
			context.sdp_ftp = 0;
			break;
		case 'd':
			context.daemon = 0;
			break;
		case 'F':
			context.folder_listing = 0;
			break;
		case 'S':
			context.syslog = 1;
			break;
		case 'I':
			context.mode = 1;
			handler = internal_handler;
			break;
		case 'p':
			print_channel = 1;
			break;
		case 'l':
			strncpy(context.lsdir_params, optarg, 10);
			break;
		case 's':
			context.policy = atoi(optarg);
			if (context.policy < 1 || context.policy > 3) {
				fprintf(stderr, "invalid security mode-2 setting: %s\n", optarg);
				exit(-1);
			}
			break;
		case 'v':
			fprintf(stderr, "sobexsrv version %s\n", VERSION_STRING);
			exit(1);
			break;
		case 'r':
			context.rootdir = optarg;
			break;
		case 'R':
			context.chroot = 1;
			break;
		case 'h':
			usage(argv[0]);
			exit(1);
		default:
			usage(argv[0]);
			exit(-1);
		}
	}
	
	if (optind < argc) {
		usage(argv[0]);
		exit(-1);
	}
	
	// connect to sdp server
	open_sdp();
	
	// chdir
	if (chdir(context.rootdir) != 0) {
		perror("chdir:");
		exit(-1);
	}
	
	if (context.syslog) openlog("sobexsrv", LOG_NDELAY, LOG_USER);
	
	// chroot und setuid(>0)
	if (geteuid() == 0 && getuid() != 0) {
		if (context.chroot && chroot(context.rootdir) != 0) {
			perror("chroot:");
			exit(-1);
		}
		else {
			if (context.chroot) debug2("doing chroot(%s)\n", context.rootdir);
			if (setuid(getuid()) < 0) {
				perror("setuid(getuid()) failed: ");
				exit(-1);
			}
			else {
				debug2("dropping privileges setuid(%d)\n", (char*)getuid());
			}
		}
	}
	else if (geteuid() != 0 && getuid() != 0) {
		// not suid and not run as root!
	}
//	else {
//		fprintf(stderr, "don't run as root just make it suid so we can drop privileges, please read man page!\n");
//		exit(-1);
//	}
	
	// select handler script
	if (context.handler_script == NULL) {
		context.handler_script = HANDLER;
	}

	// check handler script
	if (!context.mode && access(context.handler_script, X_OK) != 0) {
		fprintf(stderr, "can't use given handler: %s\n", context.handler_script);
		usage(argv[0]);
		exit(-1);
	}
		
	// setup server socket
	if ((context.server = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror("server socket: ");
		exit(-1);
	}
	
	// bind to address
	context.srv_addr.rc_family = AF_BLUETOOTH;
	context.srv_addr.rc_channel = context.channel;
	bacpy(&context.srv_addr.rc_bdaddr, &context.local_address);
	if (bind(context.server, (struct sockaddr*) &context.srv_addr, sizeof(context.srv_addr)) < 0) {
		perror("server bind: ");
		exit(-1);
	}
	
	// listen socket
	if (listen(context.server, 1) < 0) {
		perror("server listen: ");
		exit(-1);
	}

	// get channel for dynamic channel allocation
	if (context.channel == 0) {
		struct sockaddr_rc sa = {0};
		unsigned int sa_len = sizeof(struct sockaddr_rc);
		getsockname(context.server, (struct sockaddr*)&sa, &sa_len);
		context.channel = sa.rc_channel;
	}
	
	if (print_channel) printf("%d\n", context.channel);

	// register services with SDPd
	if (context.sdp_opush) add_opush();
	if (context.sdp_ftp) add_ftp();

#ifdef SINGLE
	context.daemon = 0;
#endif

	// detach and become daemon
	if (context.daemon) {
		int fd;
		if (fork()) exit(0);
		fd = open("/dev/null", O_RDWR);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		setsid();
	}
	
	// handle connections
	for (;;) {
		int pid;
		struct sockaddr_rc tmp_addr;
		
		size = sizeof(struct sockaddr_rc);
		context.client = accept(context.server, (struct sockaddr*)&context.clt_addr, &size);
		
		// client child
#ifndef SINGLE		
		if ((pid = fork()) == 0) {
			close(context.server);
#endif			
			// the pid will also be a unique connection id for the handler
			context.pid = getpid();
			
			// get connection information
			bacpy(&context.remote_address, &context.clt_addr.rc_bdaddr);
			size = sizeof(struct sockaddr_rc);
			getsockname(context.client, (struct sockaddr*) &tmp_addr, &size);
			bacpy(&context.local_address, &tmp_addr.rc_bdaddr);

			// invoke accept handler	
			handler_res = handler("accept", "accept");
			if (handler_res == -1) {
				debug2("%s: accept handler failed\n", (char*)__func__);
				goto disconnect;
			}
			
			// trigger security mode-2
			if (!security(handler_res)) {
				debug2("%s: security mode-2 failed or rejected\n", (char*)__func__);
				goto disconnect;
			}
			
			// setup obex session
			context.libObex = OBEX_Init(OBEX_TRANS_FD, eventhandler, 0);
			if (context.libObex == NULL) {
				debug("OBEX can't initialize\n");
				goto disconnect;
			}
			OBEX_SetCustomData(context.libObex, (void*)&context);

			if (FdOBEX_TransportSetup(context.libObex, context.client, context.client, OBEX_DEFAULT_MTU) < 0) {
				debug("OBEX can't setup transport\n");
				goto disconnect;
			}

			// handle client
			while (OBEX_HandleInput(context.libObex, 1) != -1);

disconnect:
			close(context.client);
			if (context.mode) dosyslog("disconnected\n", 0,0);
#ifndef SINGLE
			exit(0);
		}
		
		// mother process
		close(context.client);
#endif
	}

	return(1);
}

/*
 *  $Log$
 */
