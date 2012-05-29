
AM_MAKEFLAGS = --no-print-directory

library_sources = lib/bluetooth.h lib/bluetooth.c \
			lib/hci_lib.h lib/hci.h lib/hci.c lib/l2cap.h

parser_sources =  parser/parser.h parser/parser.c \
					parser/lmp.c \
					parser/hci.c \
					parser/l2cap.c \
					parser/smp.c \
					parser/att.c \
					parser/sdp.h parser/sdp.c \
					parser/rfcomm.h parser/rfcomm.c \
					parser/bnep.c \
					parser/cmtp.c \
					parser/hidp.c \
					parser/hcrp.c \
					parser/avdtp.c \
					parser/avctp.c \
					parser/avrcp.c \
					parser/obex.c \
					parser/capi.c \
					parser/ppp.c \
					parser/tcpip.c \
					parser/ericsson.c \
					parser/csr.c \
					parser/bpa.c


sbin_PROGRAMS = src/hcidump

src_hcidump_SOURCES = src/hcidump.c $(library_sources) $(parser_sources)


noinst_PROGRAMS = src/bpasniff src/csrsniff

src_bpasniff_SOURCES = src/bpasniff.c $(library_sources) $(parser_sources)

src_csrsniff_SOURCES = src/csrsniff.c $(library_sources) $(parser_sources)


dist_man_MANS = src/hcidump.8

EXTRA_DIST = src/magic.btsnoop

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in \
	depcomp missing install-sh mkinstalldirs
