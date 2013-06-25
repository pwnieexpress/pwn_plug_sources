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

# Verify patch 1.1.2 is not applied
if [ "`grep -o 1.1.3 /etc/motd.tail`" == "1.1.3" ] ; then 
        echo "[-] Patch 1.1.3 already applied. Aborting..."
        echo "[-] If you have questions, please email support@pwnieexpress.com."
        exit 1
fi

# Verify current Pwn Plug release is 1.1 or greater
if [ "$plug_release" == "Release 1.1" ] ; then 
    echo "[+] Pwn Plug is eligible for update."
else
    echo "[-] This patch requires Pwn Plug Release 1.1 or greater. Aborting..."
    echo "[-] You can download the 1.1 upgrade at:                            "
    echo "[-] http://www.pwnieexpress.com/downloads.html                      "
    echo "[-] If you have questions, please email support@pwnieexpress.com.   "
    exit 1
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
    # Create mount point for SD card. Unfortunately we can't rely on the SD card being 
    # available, so don't try to mount it. 
    mkdir /storage

    # Add a line to automount the sd card into /storage (see manual for how to format)
    echo "/dev/mmcblk0p1 /storage ext2 rw 0 0" >> /etc/fstab

fi

###
### Attempt to move metasploit to Git. Note that this was done for the 1.1.2 patch
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
### Clean up ruby version - This is specific to version 1.1.3
###

# Remove old / outdated Ruby
apt-get -y remove ruby rubygems ruby1.8 rubygems1.8

# Download our Ruby packages
wget http://pub.pwnieexpress.com/ruby_arm/libruby1.9.1_1.9.3.429pwnix0_armel.deb
wget http://pub.pwnieexpress.com/ruby_arm/ruby1.9.1_1.9.3.429pwnix0_armel.deb
wget http://pub.pwnieexpress.com/ruby_arm/ruby1.9.1-dev_1.9.3.429pwnix0_armel.deb

# Verify Sha1's as: 
# 8e8277d063d0fb9fd3473a23fb76374e7b2bd917  libruby1.9.1_1.9.3.429pwnix0_armel.deb
# 98d8a0579671aa65bb895fc6dc4060726d74a1a6  ruby1.9.1_1.9.3.429pwnix0_armel.deb
# 4db691fcd5a8c0491feb37df1aba7221b2348536 ruby1.9.1-dev_1.9.3.429pwnix0_armel.deb

apt-get install libffi5 libyaml-0-2
dpkg -i libruby1.9.1_1.9.3.429pwnix0_armel.deb
dpkg -i ruby1.9.1_1.9.3.429pwnix0_armel.deb
dpkg -i ruby1.9.1-dev_1.9.3.429pwnix0_armel.deb

# install postgres dev library for pg gem
apt-get install libpq-dev

# Install metasploit's bundle 
cd /opt/metasploit/msf3
bundle install


# Update release version for community or regular edition
sed -i 's/Release 1.1[.?]*c.*$/Release 1.1.3c \[June 2013\]/g' /etc/motd.tail
sed -i 's/Release 1.1[.?]* .*$/Release 1.1.3 \[June 2013\]/g' /etc/motd.tail

echo "[+] Release version updated successfully."

# Done
echo ""
echo "---------------------------------------------------------------"
echo "Pwn Plug Patch 1.1.3 applied successfully! - Note that the SD  "
echo " card is not required in this version. You may now use all     "
echo " functionality of your Pwn Plug without the SD card.           "
echo "---------------------------------------------------------------"
echo ""
