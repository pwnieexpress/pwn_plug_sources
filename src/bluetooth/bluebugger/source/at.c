/*
 * Copyright (C) 2006 by Martin J. Muench <mjm@codito.de>
 *
 * Part of mpd - mobile phone dumper
 *
 * Some code stolen from btxml.c by Andreas Oberritter
 *
 */

#define _GNU_SOURCE /* getline() */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "at.h"
#include "debug.h"

extern FILE *output;

/* 
 * Manufactur list 
 */
static enum {
  MANUF_UNKNOWN,
  MANUF_ERICSSON,
  MANUF_NOKIA
} manuf;

/*
 * Phone Calls
 */

int at_dial(FILE *fp, const char *call_id)
{
  char call_comm[256];

  snprintf(call_comm, sizeof(call_comm), "D%s;", call_id);

  debug(("Dial command: '%s'", call_comm));

  fprintf(output, "Dialing '%s' ....", call_id);
  fflush(stdout);

  at_cmd(fp, NULL, call_comm);

  sleep(10);
  printf("call to '%s' should be active now\n\n", call_id);
  printf("Press <enter> to abort bluetooth connection\n");
  printf("* shows 'cancel call too?'-popup on Nokia 6310i)\n");
  (void)getc(stdin);

  return 0;
}

/*
 * Custom
 */

int at_custom(FILE *fp, const char *custom_command)
{
  char buf[0x1000];

  debug(("Custom command: %s", custom_command));

  if (at_cmd(fp, buf, custom_command) != 1) {
  }

  fprintf(output, "%s: '%s'\n\n", custom_command, buf);

  return 0;
}


/*
 * Specific AT funcitons
 */

/* No echo */
void at_disable_echo(FILE *fp)
{
  at_cmd(fp, NULL, "E0");
}

/* Get all identification data */
int at_parse_identification(FILE *fp)
{
  fprintf(output, "Mobile Identification\n"\
	  "---------------------\n");

  /* at_parse_manufacturer_identification(fp); */
  at_parse_model_identification(fp);
  at_parse_revision_identification(fp);
  at_parse_psn_identification(fp);
  at_parse_capabilities(fp);

  fprintf(output, "\n");
  return 0;
}

/* Get all phonebook entries */
int at_parse_phonebook_list(FILE *fp)
{
  char buf[0x1000];

  if (at_cmd(fp, buf, "+CPBS=?") != 1)
    return -1;

  debug(("Phonebook (req: '+CPBS=?'): '%s'", buf));

  fprintf(output, "Address Book\n"\
         "------------\n");

  return at_parse_brackets(fp, buf, at_parse_phonebook);
}

/* Get all SMS */
int at_parse_message_list(FILE *fp)
{
  char buf[0x1000];

  if(manuf != MANUF_ERICSSON) {
    if (at_cmd(fp, NULL, "+CMGF=1") != 0)
      return -1;
  }

  if (at_cmd(fp, buf, "+CPMS=?") != 1)
    return -1;

  debug(("Messages (req '+CMGF=1' and '+CPMS=?'): '%s'", buf));

  fprintf(output, "SMS Messages\n"\
	 "------------\n");

  return at_parse_brackets(fp, buf, at_parse_message_storage);
}


/* 
 * Generic high-level funcs 
 */


/* Generic at cmd send/recv func */ 
int at_cmd(FILE *fp, char *buf, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  at_send(fp, fmt, ap);
  va_end(ap);

  return at_recv(fp, buf);
}

/* pass data between brackets to func */
int at_parse_brackets(FILE *fp, char *buf, int (*cb)(FILE*, const char*))
{
  char *start, *end, *str;

  if ((!(start = strchr(buf, '('))) || (!(end = strchr(++start, ')'))))
    return -1;

  *end = '\0';

  while ((str = strsep(&start, ",")))
    cb(fp, str);

  return 0;
}


/* 
 * Get device identification
 */

/* manufacturer */
int at_parse_manufacturer_identification(FILE *fp)
{
  char buf[0x1000];

  if (at_cmd(fp, buf, "+GMI") < 1)
    return -1;

  if (strstr(buf, "Ericsson"))
    manuf = MANUF_ERICSSON;
  else if (strstr(buf, "Nokia"))
    manuf = MANUF_NOKIA;
  else
    manuf = MANUF_UNKNOWN;

  fprintf(output, "Manufactor: %s\n\n", buf);

  return 0;
}

/* model */
int at_parse_model_identification(FILE *fp)
{
  char buf[0x1000];

  if (at_cmd(fp, buf, "+GMM") < 1)
    return -1;

  fprintf(output, "Model:      %s\n", buf);
  return 0;
}

/* revision */
int at_parse_revision_identification(FILE *fp)
{
  char buf[0x1000];

  if (at_cmd(fp, buf, "+GMR") < 1)
    return -1;

  fprintf(output, "Revision:   %s\n", buf);
  return 0;
}

/* psn */
int at_parse_psn_identification(FILE *fp)
{
  char buf[0x1000];

  if (at_cmd(fp, buf, "+CGSN") < 1)
    return -1;

  fprintf(output, "PSN/IMEI:   %s\n", buf);

  return 0;
}

/* capabilities */
int at_parse_capabilities(FILE *fp)
{
  char buf[0x1000];
  char *ptr;

  if (at_cmd(fp, buf, "+GCAP") < 1)
    return -1;
  
  ptr=buf;
  ptr+=strlen("+GCAP: ");

  fprintf(output, "Capability: %s\n", ptr);
  
  return 0;
}



/*
 * Get phonebook entries
 */

/* Parse single entry */
int at_parse_phonebook_entry(FILE *fp, size_t num)
{
  char buf[0x1000], *ptr, *start=NULL, *end=NULL;
  char *number_ptr=NULL, *name_ptr=NULL;
  ssize_t number_len=-1, name_len=-1;

  if (at_cmd(fp, buf, "+CPBR=%u", num) != 1)
    return -1;

  debug(("Phonebook (req '+CPBR=%u'): %s", num, buf));

  ptr = buf;
  if (!strncmp(ptr, "+CPBR: ", 7))
    ptr += 7;

  if (((start = strchr(ptr, '\"'))) && (end = strchr(++start, '\"'))) {
    number_ptr = start;
    number_len = end - start;
  }
  else {
    number_ptr = NULL;
  }

  if (((start = strchr(++end, '\"'))) &&
      (end = strrchr(&ptr[strlen(ptr) - 1], '\"'))) {
    name_ptr = ++start;
    name_len = end - start;
  }
  else {
    name_ptr = NULL;
  }

  if ((number_ptr) && (name_ptr)) {
    fprintf(output, "Name:   %.*s\n", name_len, name_ptr);
    fprintf(output, "Number: %.*s\n", number_len, number_ptr);
    fprintf(output, "\n");
  }
  else {
    fprintf(output, "Raw:    %s\n", ptr);
  }
  return 0;
}

/* Parse phonebook */
int at_parse_phonebook(FILE *fp, const char *name)
{
  char buf[0x1000], *ptr;
  size_t start, end, used, size, i, found;

  /* set storage */
  if (at_cmd(fp, NULL, "+CPBS=%s", name) != 0)
    return -1;

  debug(("Phonebook (req '+CPBS=%s')", name));

  /* Get used number storage entries for nokia and unknown */
  if ((manuf == MANUF_NOKIA) || (manuf == MANUF_UNKNOWN)) 
  {
    /* get stats for storage */
    if (at_cmd(fp, buf, "+CPBS?") != 1)
      return -1;
    
    /* ignore storagename */
    if (!(ptr = strchr(buf, ',')))
      return -1;
    
    /* return on error */
    if (sscanf(++ptr, "%u,%u", &used, &size) != 2)
      return -1;
    
    /* empty */
    if (!used)
      return -1;
  }
  /* Get used number of storage entries for Ericsson */
  else {
    /* How to get number of used entries??? */
    /* TODO */
  }

  /* Get possible storage entry nums */
  if (at_cmd(fp, buf, "+CPBR=?") != 1)
    return -1;
  debug(("Phonebook (req '+CPBR=?'): %s", buf));
  
  if (sscanf(buf, "+CPBR: (%u-%u)", &start, &end) != 2)
    return -1;

  /* SONY ERICSSON HACK, see above if/else */
  if(manuf == MANUF_ERICSSON) {
    start = 1;
    end   = 20;
    used  = 20;
  }

  /* parse all entries */
  for (i = start, found = 0; i <= end && found < used; i++) {
    if (!at_parse_phonebook_entry(fp, i))
      found++;
  }

  return 0;
}

/*
 * Get all SMS messages
 */

/* Parse single message */
int at_parse_message(FILE *fp, size_t num)
{
  char buf[0x1000], *ptr;
  static unsigned int counter=0;

  if (at_cmd(fp, buf, "+CMGR=%u", num) < 1)
    return -1;

  debug(("Message (req: '+CMGR=%u'): %s", num, buf));

  ptr = buf;
  if (!strncmp(ptr, "+CMGR: ", 7))
    ptr += 7;

  fprintf(output, "%d: %s\n\n", counter, ptr);
  counter++;
  return 0;
}

/* Parse all messages */
int at_parse_message_storage(FILE *fp, const char *name)
{
  char buf[0x1000];
  size_t i, msgnum, size;

  if (at_cmd(fp, buf, "+CPMS=%s", name) != 1)
    return -1;

  debug(("Messages (req '+CPMS=%s'): %s", name, buf));

  if (sscanf(buf, "+CPMS: %u,%u", &msgnum, &size) != 2)
    return -1;

  for (i = 1; i <= msgnum; i++)
    at_parse_message(fp, i);

  return 0;
}


/*
 * Low-level funcs 
 */

void at_send(FILE *fp, const char *fmt, va_list ap)
{
  fprintf(fp, "AT");
  vfprintf(fp, fmt, ap);
  fprintf(fp, "\r\n");
}


ssize_t at_recv(FILE *fp, char *dest)
{
  char *line = NULL;
  size_t len = 0;
  size_t ret = 0;
  ssize_t read;

  while ((read = getline(&line, &len, fp)) != -1) {
    if (!read)
      continue;

    if ((line[read - 1] == '\n') && (--read == 0))
      continue;
    if ((line[read - 1] == '\r') && (--read == 0))
      continue;

    line[read++] = '\0';
    
    if (!strcmp(line, "OK"))
      break;
    if ((!strcmp(line, "ERROR")) ||
	(!strncmp(line, "+CME ERROR:", 10)) ||
	(!strncmp(line, "+CMS ERROR:", 10))) {
      ret = -1;
      break;
    }

    if (dest) {
      if (ret)
	dest[-1] = ' ';
      memcpy(dest, line, read);
      dest += read;
    }
    ret++;
  }
  
  free(line);
  
  return ret;
}



