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
#include <string.h>
#include <stdlib.h>

char* resolve_path(char *path)
{
	char *output = malloc(strlen(path)+1);
	char *p = path;
	char *l = output;
	char *l2 = output;
	char *o = output;
	int dot = 0;
	int i, len;

	bzero(output, strlen(path)+1);

	len = strlen(path);
	for (i = 0; i < len; i++) {
		if (*p == '/') {
			// catch current dir .
			if (dot == 2) {
				o = l;
				*(o+1) = 0;
			}
			// catch .. ... .... what ever
			else if (dot >= 3) {
				o = l2;
				*(o+1) = 0;
				l = o-1;
				while (*l != '/') l--;
			}
			l2 = l;
			l = o;
			dot = 1;
		}
		else if (*p == '.' && dot > 0) {
			dot++;
		}
		else {
			dot = 0;
		}
		*o++ = *p++;
		*o = 0;
		//printf("test: o=%s l=%s\n",output,l);
	}
	*o = 0;
	if (dot == 2) {
		o = l;
		*(o+1) = 0;
	}
	else if (dot >= 3) {
		o = l2;
		*(o+1) = 0;
	}
	return(output);
}

#ifdef PATHTEST
void main(int argc, char **argv)
{
	printf("%s\n",resolve_path(argv[1]));
}
#endif

/*
 *  $Log$
 */
