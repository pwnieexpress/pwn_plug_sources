/*
 * Copyright (C) 2006 by Martin J. Muench <mjm@codito.de>
 *
 * Part of mpd - mobile phone dumper
 *
 * Some code stolen from btxml.c by Andreas Oberritter
 *
 */

#include "wrap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Simple malloc() wrapper */
void *Malloc(size_t size) {
  void *buffer;
  buffer = malloc(size);

  if(buffer == NULL) {
    fprintf(stderr, "malloc() failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  memset(buffer, 0, size);
  return(buffer);
}
