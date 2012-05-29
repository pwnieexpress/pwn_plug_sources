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

#define GNU_SOURCE
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define LINE_LENGTH 2048

/**
 *  @brief generate XML folder listing
 *
 *  only regular files and directories are shown!!!
 *
 *  @param path fullpath to list
 *  @param params print parameters (what information to include) see below
 *  @param out_list pointer to char pointer, for the returned listing
 *
 *  @returns number of items
 *
 *  params has the following format: ONPTD
 *  O = print owner (user and group)
 *  N = print numeric UID and GID only
 *  P = print permissions
 *  T = print m,c,a times
 *  D = print parent dir ".."
 *
 *  params can be NULL to just print everything!
 */
int lsdir(const char *path, const char *params, char **out_list)
{
	DIR *d;
	struct dirent *e = NULL;
	unsigned int num = 0;
	char *fp;
	int path_len;
	struct stat fi;
	struct passwd *ui;
	struct group *gi;
	struct tm ttime;
	char item[LINE_LENGTH];
	unsigned int line_leng;
	unsigned out_leng;
	int s_gu = 1;
	int s_gunum = 0;
	int s_perms = 1;
	int s_times = 1;
	int s_dotdot = 1;
	char newname[LINE_LENGTH];
	int newname_len;
	int i;
	

	assert(out_list != NULL);
	assert(*out_list == NULL);

	
	if ((d = opendir(path)) == NULL) {
		return(0);
	}
	
	path_len = strlen(path);
	if ((fp = (char*) malloc(path_len + 257)) == NULL) {
		closedir(d);
		return(0);
	}
	
	if (params != NULL) {
		s_gu = (int) strchr(params, 'O');
		s_gunum = (int) strchr(params, 'N');
		s_perms = (int) strchr(params, 'P');
		s_times = (int) strchr(params, 'T');
		s_dotdot = (int) strchr(params, 'D');
	}
	
	e = readdir(d);
	
	while (e != NULL) {
		if (!s_dotdot) {
			if (strncmp(e->d_name, "..", (strlen(e->d_name) > 2) ? strlen(e->d_name) : 2) == 0 ||
					strncmp(e->d_name, ".", (strlen(e->d_name) > 1) ? strlen(e->d_name) : 1) == 0) {
				e = readdir(d);
				// only one ".." so we can turn of the check and don't do strncmp every time
				s_dotdot = 0;
				continue;
			}
		}
		num++;
	
		bzero(item, LINE_LENGTH);
		line_leng = 0;
		snprintf(fp, path_len + 257, "%s/%s", path, e->d_name);
		if (stat(fp, &fi) == 0) {			
			if (S_ISREG(fi.st_mode)) {
				line_leng += snprintf(item, LINE_LENGTH, "<file ");
			}
			else if (S_ISDIR(fi.st_mode)) {
				line_leng += snprintf(item, LINE_LENGTH, "<folder ");
			}
			else {
				// ignore everything else
				num--;
				e = readdir(d);
				continue;
			}
	
			// stupid XML escaping
			newname_len = 0;
			bzero(newname, LINE_LENGTH);
			for (i = 0;;i++) {
				if (e->d_name[i] == 0) break;
				else if (e->d_name[i] == '&') {
					strcat(newname, "&amp;");
					newname_len += 5;
				}
				else if (e->d_name[i] == '<') {
					strcat(newname, "&lt;");
					newname_len += 4;
				}
				else if (e->d_name[i] == '>') {
					strcat(newname, "&gt;");
					newname_len += 4;
				}
				else if (e->d_name[i] == '"') {
					strcat(newname, "&quot;");
					newname_len += 6;
				}
				else if (e->d_name[i] == '\'') {
					strcat(newname, "&apos;");
					newname_len += 6;
				}
				else {
					newname[newname_len] = e->d_name[i];
					newname_len++;
				}
			}
			
			line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "name=\"%s\" ", newname);
			line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "size=\"%d\" ", (unsigned int)fi.st_size);
			
			if (s_gu && (!s_gunum)) {
				ui = getpwuid(fi.st_uid);
				gi = getgrgid(fi.st_gid);
			
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "owner=\"%s\" ", ui->pw_name);
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "group=\"%s\" ", gi->gr_name);
			}
			else if (s_gu && s_gunum) {
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "owner=\"%d\" ", fi.st_uid);
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "group=\"%d\" ", fi.st_gid);
			}
			
			if (s_perms) {
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "user-perm=\"");
				if (fi.st_mode & S_IRUSR) {
					line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "R");
				}
				if (fi.st_mode & S_IWUSR) {
					line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "WD");
				}
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "\" ");
			
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "group-perm=\"");
				if (fi.st_mode & S_IRGRP) {
					line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "R");
				}
				if (fi.st_mode & S_IWGRP) {
					line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "WD");
				}
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "\" ");
			
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "other-perm=\"");
				if (fi.st_mode & S_IROTH) {
					line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "R");
				}
				if (fi.st_mode & S_IWOTH) {
					line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "WD");
				}
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "\" ");
			}
			
			if (s_times) {
				gmtime_r(&fi.st_ctime, &ttime);
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "created=\"%4d%02d%02dT%02d%02d%02dZ\" ", 1900+ttime.tm_year, ttime.tm_mon+1, ttime.tm_mday, ttime.tm_hour, ttime.tm_min, ttime.tm_sec);
				gmtime_r(&fi.st_mtime, &ttime);
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "modified=\"%4d%02d%02dT%02d%02d%02dZ\" ", 1900+ttime.tm_year, ttime.tm_mon+1, ttime.tm_mday, ttime.tm_hour, ttime.tm_min, ttime.tm_sec);
				gmtime_r(&fi.st_atime, &ttime);
				line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "accessed=\"%4d%02d%02dT%02d%02d%02dZ\" ", 1900+ttime.tm_year, ttime.tm_mon+1, ttime.tm_mday, ttime.tm_hour, ttime.tm_min, ttime.tm_sec);
			}
				
			line_leng += snprintf(item+line_leng, LINE_LENGTH-line_leng-1, "/>\n");

			if (*out_list == NULL) {
				out_leng = LINE_LENGTH;
				if ((*out_list = (char*) malloc(LINE_LENGTH)) == NULL) {
					perror("lsdir malloc:");
					exit(-1);
				}
				bzero(*out_list, LINE_LENGTH);
			}
			else {
				if (out_leng-strlen(*out_list) <= line_leng) {
					char *tmp = *out_list;
					if ((*out_list = (char*) malloc(out_leng + LINE_LENGTH)) == NULL) {
						perror("lsdir malloc:");
						exit(-1);
					}
					out_leng += LINE_LENGTH;
					bzero(*out_list, out_leng);
					strcpy(*out_list, tmp);
					free(tmp);
				}
			}
			strcat(*out_list, item);
		}
		
		e = readdir(d);
	}
	closedir(d);
	free(fp);
	
	return(num);
}

#ifdef LSDIR_BIN
main(int argc, char **argv)
{
	char *list = NULL;
	lsdir(argv[1], argv[2], &list);
	printf("%s", list);
	free(list);
}
#endif


/*
 *  $Log$
 */
