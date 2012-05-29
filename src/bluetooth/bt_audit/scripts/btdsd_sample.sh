#!/bin/sh
#
# do minimal device scan
#
# hardware information (hcitool info)
# SDP public browse group (sdptool browse)
# L2CAP scan (psm_scan)
# RFCOMM scan (rfcomm_scan)
#

if [ "$#" = "2" ];  then
 echo -en "\ngetting data from $1, please wait..."
 date >$2
 echo "" >>$2
 hcitool info $1 >>$2
 echo "" >>$2
 sdptool browse --tree $1 >>$2
 echo "" >>$2
 ../src/psm_scan -o -s 1 -e 1001 $1 >>$2
 echo "" >>$2
 ../src/rfcomm_scan -o -s 1 -e 30 $1 >>$2
 echo -e "done\n"
else
 echo "usage: $0 BD_ADDR out_file"
fi
