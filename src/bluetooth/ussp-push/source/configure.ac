#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([ussp-push],[0.11.1],[Davide Libenzi <davidel@xmailserver.org>])
AC_CONFIG_SRCDIR([src/obex_socket.h])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
## BLUETOOTH
PKG_CHECK_MODULES(BLUETOOTH, bluez)
AC_SUBST(BLUETOOTH_CFLAGS)
AC_SUBST(BLUETOOTH_LIBS)
## OPENOBEX
PKG_CHECK_MODULES(OPENOBEX, openobex)
AC_SUBST(OPENOBEX_CFLAGS)
AC_SUBST(OPENOBEX_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_INT16_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_CHECK_FUNCS([bzero memset select socket strcasecmp strchr strdup strerror strrchr])

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 doc/Makefile])
AC_OUTPUT
