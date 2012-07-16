#!/bin/bash
# pwnieexpress.com
# Revision 6.11.2012

plug_release=""`grep -o "Release 1.1" /etc/motd`""

echo ""

# Verify we are root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Verify patch is not already applied
if [ "`grep -o 1.1.1 /etc/motd.tail`" == "1.1.1" ] ; then 
        echo "[-] Patch already applied. Aborting..."
        echo "[-] If you have questions, please email support@pwnieexpress.com.   "
        exit 1
fi

# Verify current Pwn Plug release (1.1 required)
if [ "$plug_release" == "Release 1.1" ] ; then 
        echo "[+] Pwn Plug is Release 1.1"
else
        echo "[-] This patch requires Pwn Plug Release 1.1 or greater. Aborting..."
        echo "[-] You can download the 1.1 upgrade at:                            "
        echo "[-] http://www.pwnieexpress.com/downloads.html                      "
        echo "[-] If you have questions, please email support@pwnieexpress.com.   "
        exit 1
fi

# Verify presence of SD card
if [ -e "/dev/mmcblk0p1" ]; then
        echo "[+] SD card is present, continuing..."
else
        echo "[-] SD card not detected at /dev/mmcblk0p1. Aborting...    "
        echo "[-] Please insert a SD Card of size 1GB (16GB recommended) "
        echo "[-] or greater to continue. If you have questions, please  "
        echo "[-] email support@pwnieexpress.com."
        exit 1
fi

echo ""
echo "                 === Pwn Plug Patch 1.1.1 ===                   "
echo "----------------------------------------------------------------"
echo "                      !!!! WARNING !!!!                         "
echo "         The currently-inserted SD card is about to be          "
echo "       reformatted! All data on the SD card will be lost.       "
echo "                                                                " 
echo " This patch will format the currently-inserted SD card and move "
echo " /opt/metasploit/msf3 and /pentest to the card due to a recent  "
echo " spike in the size of MSF. You will be required to keep the SD  "
echo " card inserted in the Pwn Plug when utilizing these tools. The  "
echo " Plug UI will continue to function without the SD card. If you  "
echo " have any questions, please email support@pwnieexpress.com.     "
echo "----------------------------------------------------------------"
echo ""
echo "Press ENTER to continue, CTRL+C to abort."
read INPUT
echo ""

# Reformat SD card as ext3
echo "[+] Now reformatting SD card as ext3 (will take a minute...)"
mkfs.ext3 /dev/mmcblk0p1
echo "[+] SD card reformat complete."

# Create mount point & mount SD card
mkdir /storage
mount /dev/mmcblk0p1 /storage
echo "[+] SD card mounted to /storage"

# Mount SD card to /storage at system startup
echo "/dev/mmcblk0p1 /storage ext3 rw 0 0" >> /etc/fstab
echo "[+] SD card will be automounted to /storage at system startup"

# Move the /pentest directory to SD card
echo "[+] Moving pentest tools to SD card (will take a minute...)"
mv /pentest /storage
echo "[+] /pentest folder move complete"

# Create symlink for the pentest directory
ln -s /storage/pentest /pentest
echo "[+] Created symlink for /pentest"

# Move MSF3 directory to SD card
echo "[+] Now moving msf3 folder to SD card (will take a few minutes...)"
mv /opt/metasploit/msf3 /storage
echo "[+] msf3 folder move complete"

# Create symlink for msf3
ln -s /storage/msf3 /opt/metasploit/msf3
echo "[+] Created symlink for /opt/metasploit/msf3"

# Update release version
if [ "`grep -o 1.1c /etc/motd.tail`" == "1.1c" ] ; then
        sed -i 's/Release 1.1c \[May 2012\]/Release 1.1.1c \[June 2012\]/g' /etc/motd.tail
else
        sed -i 's/Release 1.1 \[May 2012\]/Release 1.1.1 \[June 2012\]/g' /etc/motd.tail
fi

# Done
echo ""
echo "---------------------------------------------------------------"
echo "Pwn Plug Patch 1.1.1 applied successfully!"
echo "---------------------------------------------------------------"
echo ""
echo "Metasploit can continue to be updated as usual (msfupdate)."
echo "Press ENTER to update Metasploit now, otherwise press CTRL+C to exit."
read INPUT

# Update Metasploit, warn users about svn
echo ""
msfupdate
echo "[+] If you experienced a PROPFIND error, please run 'msfupdate' again to continue updating Metasploit."
echo "[+] Done! If you have questions, please email support@pwnieexpress.com."
