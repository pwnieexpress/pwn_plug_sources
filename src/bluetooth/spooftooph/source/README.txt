  spooftooph

Copyright (C) 2009-2011 Shadow Cave LLC

Written 2009-2011 by JP Dunning (.ronin)
ronin [ at ] shadowcave [dt] org
<www.hackfromacave.com>

 
  License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation;

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY 
CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS 
DISCLAIMED.



  ABOUT

Spooftooph is designed to automate spoofing or cloning Bluetooth device 
Name, Class, and Address.  See 'Usage' for more information of its capabilities.



  INSTALL

1. Run "make" to compile binaries.
2. Run "make install" to install into system.
3. Run "make clean" to delete binaries from spooftooph directory.



  RESOURCES

- BlueZ
- BlueZ-devel
- ncurses
- ncurses-devel 



  USAGE

To modify the Bluetooth adapter, spooftooth must be run with root privileges.  
Spooftooph offers five modes of usage:

 1) Specify NAME, CLASS and ADDR.

> spooftooph -i hci0 -n new_name -a 00:11:22:33:44:55 -c 0x1c010c

 2) Randomly generate NAME, CLASS and ADDR.

> spooftooph -i hci0 -r

 3) Scan for devices in range and select device to clone.  Optionally dump the device 
 information in a specified log file.

> spooftooph -i hci0 -s -d file.log

 4) Load in device info from log file and specify device info to clone.

> spooftooph -i hci0 -l file.log

 5) Clone a random devices info in range every X seconds.

> spooftooph -i hci0 -t 10



  HELP

NAME
        spooftooph

SYNOPSIS
        spooftooph -i dev [-m] [-sd] [-nac] [-r] [-l] [-t]

DESCRIPTION
        -a <address>    : Specify new ADDR
        -b <num_lines>  : Number of Bluetooth profiles to display per page
        -c <class>      : Specify new CLASS
        -d <log>        : Dump scan into log file
        -h              : Help
        -i <dev>        : Specify interface
        -l <log>        : Load a list of Bluetooth profiles to clone from saved log
        -n <name>       : Specify new NAME
        -m              : Specify multiple interfaces durring selection.
        -r              : Assign random NAME, CLASS, and ADDR
        -s              : Scan for devices in local area
        -t <time>       : Time interval to clone device in range