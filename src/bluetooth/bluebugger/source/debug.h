/* Copyright (C) 2006  Martin J. Muench <mjm@codito.de> */

#ifndef	MPD_DEBUG_H
#define MPD_DEBUG_H

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#define debug(x) ic_debug x;
#else 
#define debug(x) do { } while(1!=1);
#endif

void ic_debug(const char *fmt, ...);

#endif /* MPD_DEBUG_H */
