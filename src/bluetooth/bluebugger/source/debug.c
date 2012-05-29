/* Copyright (C) 2006  Martin J. Muench <mjm@codito.de> */

#include "debug.h"

extern FILE *output;

void 
ic_debug(const char *fmt, ...)
{
  char buffer[4096];
  va_list ap;
  
  memset(buffer, 0, sizeof(buffer));
  va_start(ap, fmt);
   vsnprintf(buffer, sizeof(buffer)-1, fmt, ap);
  va_end(ap);

  fprintf(output, "+ %s\n", buffer);
}
