#!/bin/bash
# Pwn Plug Patch 1.1.2
# pwnieexpress.com
# Revision 7.16.2012

plug_release=""`grep -o "Release 1.1" /etc/motd.tail`""
echo ""

# Verify we are root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Verify we (probably) have internet access
wget -qO- --timeout=5 http://google.com > /dev/null
if [[ $? -eq 0 ]]; then
        echo "[+] Internet connection detected, continuing."
else
        echo "[-] Internet connection could not be established, exiting..."
        # exit 2
fi

# Verify patch 1.1.3 is not applied
if [ "`grep -o 1.1.3 /etc/motd.tail`" == "1.1.3" ] ; then 
        echo "[-] Patch 1.1.3 already applied. Aborting..."
        echo "[-] If you have questions, please email support@pwnieexpress.com."
        exit 3
fi

# Verify current Pwn Plug release is 1.1 or greater
if [ "$plug_release" == "Release 1.1" ] ; then 
    echo "[+] Pwn Plug is eligible for update."
else
    echo "[-] This patch requires Pwn Plug Release 1.1 or greater. Aborting..."
    echo "[-] You can download the 1.1 upgrade at:                            "
    echo "[-] http://www.pwnieexpress.com/downloads.html                      "
    echo "[-] If you have questions, please email support@pwnieexpress.com.   "
    exit 4
fi

echo ""
echo "       === Pwn Plug Patch 1.1.3 ===         "
echo ""
echo "This update will require internet access to install packages."
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
        mv -v /storage/pentest /pentest
    else
        echo "[-] Unable to copy pentest tools back to NAND disk."
        echo "[-] Please contact support@pwnieexpress.com for a "
        echo "[-] copy of the latest pentest tools."
    fi

    if [ -d /storage/msf3 ]; then
        echo "[+] Removing SVN copy of metasploit."
        rm -rf /storage/msf3
    else
        echo "[-] Warning: Unable to locate svn copy of metasploit."
    fi

    # Clean up the 

else

    if [ "`grep -o mmcblk0p1 /etc/fstab`" == "mmcblk0p1" ] ; then 
        echo "[+] Mount point for SD card already exists!"
    else
        echo "[+] Creating mount point for SD Card"
        # Create mount point for SD card. Unfortunately we can't rely on the SD card being 
        # available, so don't try to mount it. 
        mkdir /storage

        # Add a line to automount the sd card into /storage (see manual for how to format)
        echo "/dev/mmcblk0p1 /storage ext2 rw 0 0" >> /etc/fstab
    fi
   
fi

###
### Attempt to move metasploit to git. Note that this was done for the 1.1.2 patch
### but if we don't exist for some reason, let's go ahead and re-create this.
###
if [ -d /opt/metasploit/msf3/.git ]; then
    echo "[+] Removing /opt/metasploit/msf3..."
    rm -rf /opt/metasploit/msf3
    echo "[+] /opt/metasploit/msf3 removed."

    echo "[+] Installing Git..."
    apt-get -y update
    apt-get -y install git
    echo "[+] Git installed."

    echo "[+] Downloading PX Metasploit fork to /opt/metasploit/msf3..."
    mkdir /opt/metasploit/msf3
    cd /opt/metasploit/msf3
    git clone git://github.com/pwnieexpress/metasploit-framework.git .
    echo "[+] PX Metasploit fork downloaded to /opt/metasploit/msf3."

    echo "alias msfupdate='cd /opt/metasploit/msf3 && git pull origin master'" >> /root/.bashrc
    source /root/.bashrc
    echo "[+] Added msfupdate alias to root's bashrc."
fi

###
### Implement updated Plug UI
###

# Kill the running plug ui
killall -9 ruby

# Remove Old code
rm -rf /var/pwnplug/plugui/*

# Extract & Place updated code 
tar -zxf ./plugui.tar.gz
mv plugui/* /var/pwnplug/plugui

# Clean up
rm -rf ./plugui
rm ./plugui.tar.gz

###
### Update Ruby version
###

# Remove old / outdated Ruby
apt-get -y remove ruby rubygems ruby1.8 rubygems1.8

# Install updated Ruby & Metasploit dependencies
apt-get -y install libffi5 libyaml-0-2 libpq-dev libxslt1-dev libxml2-dev

# Install updated Ruby
dpkg -i ./libruby1.9.1_1.9.3.429pwnix0_armel.deb
dpkg -i ./ruby1.9.1_1.9.3.429pwnix0_armel.deb
dpkg -i ./ruby1.9.1-dev_1.9.3.429pwnix0_armel.deb

# Clean up
rm ./libruby1.9.1_1.9.3.429pwnix0_armel.deb
rm ./ruby1.9.1_1.9.3.429pwnix0_armel.deb
rm ./ruby1.9.1-dev_1.9.3.429pwnix0_armel.deb

# Install Plug UI Dependencies
gem install sinatra --no-ri --no-rdoc

# Restart Plug UI
service plugui start

# Install metasploit bundle now that we have an updated ruby installed 
cd /opt/metasploit/msf3
bundle install

# Update release version for community or regular edition
sed -i 's/.*Release 1.1[.?]c.*$/ Pwn Plug Elite Release 1.1.3 \[June 2013\]/g' /etc/motd.tail

echo "[+] Release version updated to 1.1.3 successfully."

# Done
echo ""
echo "---------------------------------------------------------------"
echo "Pwn Plug Patch 1.1.3 applied successfully! - Note that the SD  "
echo " card is not required in this version. You may now use all     "
echo " functionality of your Pwn Plug without the SD card.           "
echo "---------------------------------------------------------------"
echo ""
