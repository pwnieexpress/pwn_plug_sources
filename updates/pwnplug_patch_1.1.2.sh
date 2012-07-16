#!/bin/bash
# Pwn Plug Patch 1.1.2
# pwnieexpress.com
# Revision 7.16.2012

plug_release=""`grep -o "Release 1.1" /etc/motd`""

echo ""

# Verify we are root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Verify patch 1.1.2 is not applied
if [ "`grep -o 1.1.2 /etc/motd.tail`" == "1.1.2" ] ; then 
        echo "[-] Patch 1.1.2 already applied. Aborting..."
        echo "[-] If you have questions, please email support@pwnieexpress.com.   "
        exit 1
fi

# Verify current Pwn Plug release is 1.1 or greater
if [ "$plug_release" == "Release 1.1" ] ; then 
    echo "[+] Pwn Plug is Release 1.1"
else
    echo "[-] This patch requires Pwn Plug Release 1.1 or greater. Aborting..."
    echo "[-] You can download the 1.1 upgrade at:                            "
    echo "[-] http://www.pwnieexpress.com/downloads.html                      "
    echo "[-] If you have questions, please email support@pwnieexpress.com.   "
    exit 1
fi

echo ""
echo "       === Pwn Plug Patch 1.1.2 ===         "
echo "----------------------------------------------------------------"
echo ""
echo "Press ENTER to continue, CTRL+C to abort."
read INPUT
echo ""

# If patch 1.1.1 is applied, undo the applicable 1.1.1 modifications
if [ "`grep -o 1.1.1 /etc/motd.tail`" == "1.1.1" ] ; then 
    
    # move the pentest directory back over to the NAND disk 
    
    rm /pentest

    if [ -d /storage/pentest ]; then
        echo "[+] Moving pentest tools back to internal NAND disk."
        mv /storage/pentest /pentest
    else
        echo "[-] Unable to copy pentest tools back to NAND disk."
        echo "[-] Please contact support@pwnieexpress.com for a "
        echo "[-] copy of the latest pentest tools."
    fi
else
    # Create mount point for SD card. Unfortunately we can't rely on the SD card being 
    # available, so don't try to  
    mkdir /storage
fi

echo "[+] Removing /opt/metasploit/msf3..."
rm -rf /opt/metasploit/msf3
echo "[+] /opt/metasploit/msf3 removed."

echo "[+] Installing Git..."
apt-get -y update
apt-get -y install git
echo "[+] Git installed."

echo "[+] Downloading PX Slimline Metasploit fork to /opt/metasploit/msf3..."
mkdir /opt/metasploit/msf3
cd /opt/metasploit/msf3
git clone https://github.com/pwnieexpress/metasploit-framework.git .
echo "[+] PX Slimline Metasploit fork downloaded to /opt/metasploit/msf3."

echo "alias msfupdate='cd /opt/metasploit/msf3 && git pull origin master'" >> /root/.bashrc
source /root/.bashrc
echo "[+] Added msfupdate alias to root's bashrc."

# Update release version
sed -i 's/Release 1.1 \[May 2012\]/Release 1.1.2 \[July 2012\]/g' /etc/motd.tail
echo "[+] Release version updated successfully."

# Done
echo ""
echo "---------------------------------------------------------------"
echo "Pwn Plug Patch 1.1.2 applied successfully! - Note that the SD  "
echo " card is not required in this version. You may now use all     "
echo " functionality of your Pwn Plug without the SD card.           "
echo "---------------------------------------------------------------"
echo ""

cd /root
rm pwnplug_patch*

