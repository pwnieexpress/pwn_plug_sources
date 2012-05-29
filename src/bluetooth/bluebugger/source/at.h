#ifndef MPD_AT_H
#define MPD_AT_H

#include <stdio.h>

/* Phone calls */
int at_dial(FILE *, const char *);

/* Custom */
int at_custom(FILE *, const char *);

/* No echo */
void at_disable_echo(FILE *);

/* Identification */
int at_parse_identification(FILE *);
int at_parse_manufacturer_identification(FILE *);
int at_parse_model_identification(FILE *);
int at_parse_revision_identification(FILE *);
int at_parse_psn_identification(FILE *);
int at_parse_capabilities(FILE *);

/* Phone book */
int at_parse_phonebook_list(FILE *);
int at_parse_phonebook_entry(FILE *, size_t);
int at_parse_phonebook(FILE *, const char *);

/* SMS */
int at_parse_message_list(FILE *fp);
int at_parse_message_storage(FILE *, const char *);
int at_parse_message(FILE *fp, size_t num);

/* High-level functions */
int  at_cmd(FILE *, char *, const char *, ...);
int at_parse_brackets(FILE *, char *, int (*cb)(FILE*, const char*));

/* Low-level funcs */
void    at_send(FILE *, const char *, va_list);
ssize_t at_recv(FILE *, char *);

#endif /* MPD_AT_H */
