#!/usr/bin/python

#
#  sobexsrv (c) Collin R. Mulliner <collin(AT)betaversion.net>
#               web: http://www.mulliner.org/bluetooth/sobexsrv.php
#
#  license: GPLv2
#
#  The GTK stuff was taken from bluepin (BlueZ)
#

import os, sys, string, popen2, pygtk
pygtk.require('2.0')
import gtk

# *** CONFIGURATION ***

# --- log level ---
# 0 no logging
# 1 transfer log
# 2 debug
LOGLEVEL = 2
LOGFILE = "/tmp/sobexsrv.log"

# --- basepath ---
# this is the default INBOX
BASEPATH = "/tmp/"

# --- OVERWRITE ---
# can files and directorys be overwritten or deleted
# 0 no
# 1 yes
OVERWRITE = 0

# --- SECURITY LEVEL ---
# 0 reject connection 
# 1 no security (everyone can connect)
# 2 authenticate
# 3 auth + encrypt
# 4 decide later
SECURITYLEVEL = 1

# *** END CONFIG ***

# --- HELPERS ---

def log_close():
 logfd.close()

def clean_exit(code):
 log_close()
 sys.exit(code)

def log_open(logfile):
 global logfd
 logfd = open(logfile, "a")
 if logfd == -1:
  print "can't open logfile\n"
  sys.exit(0)

def log(level, stri):
 global remote
 global connectionid
 if level <= LOGLEVEL:
  stri2 = string.rstrip(stri, "\n")
  logfd.write("[" + remote + " " + connectionid + "] " + stri2 + "\n")

def set_display():
	disp = ":0"
	auth = ""
	proc = "-C X -C Xorg -C XFree86"
	ps   = "/bin/ps " + proc + " --format args --no-headers"

	r,w = popen2.popen2(ps)
	arg = string.split(r.read())
	for i in range(1, len(arg)):
		if arg[i][0] != '-' and i==1:
			disp = arg[i]
		elif arg[i] == "-auth":
			auth = arg[i+1]
			break

	os.environ['DISPLAY']    = disp 
	os.environ['XAUTHORITY'] = auth

# Dialog Class
DLG_OK = 1
DLG_CANCEL = 0
class Dialog(gtk.Dialog):
	result = DLG_CANCEL 
	args = {}
	def __init__(self, modal=False, mesg=None, args = {}):
		gtk.Dialog.__init__(self)
		self.args = args
		self.set_modal(modal)
#		self.set_usize(400, 0)
#		self.set_uposition(300,300)
		
		self.connect("destroy", self.quit)
		self.connect("delete_event", self.quit)

		self.action_area.set_border_width(2)

		ok = gtk.Button("Accept")
		ok.connect("clicked", self.ok)
		self.action_area.pack_start(ok, padding = 20)
		ok.show()

		okauth = gtk.Button("Accept+AUTH")
		okauth.connect("clicked", self.okauth)
		self.action_area.pack_start(okauth, padding = 20)
		okauth.show()

		okenc = gtk.Button("Accept+ENC")
		okenc.connect("clicked", self.okenc)
		self.action_area.pack_start(okenc, padding = 20)
		okenc.show()

		cl = gtk.Button("Reject")
		cl.connect("clicked", self.cancel)
		self.action_area.pack_start(cl, padding = 20)
		cl.show()

		if mesg:
			msg = gtk.Label("")
			msg.set_text(mesg)
			self.vbox.pack_start(msg, padding = 10)
			msg.show()

	def ok(self, *args):
		self.result = DLG_OK 
		self.quit()

	def okauth(self, *args):
		self.result = 2 
		self.quit()

	def okenc(self, *args):
		self.result = 3 
		self.quit()

	def cancel(self, *args):
		self.result = DLG_CANCEL 
		self.quit()

	def quit(self, *args):
		self.hide()
		self.destroy()
		gtk.main_quit()

def dialog(title, mesg, args, modal = False):
	dlg = Dialog(args = args, mesg = mesg, modal = modal)
	dlg.set_title(title)
	dlg.show()
	gtk.main()
	return dlg.result

# -- READ header --
def do():
 global remote
 global connectionid
 line = "just dot have an empty line"
 path = ""
 remote = "00:00:00:00:00:00"
 connectionid = "0"
 while len(line) > 2:
  line = sys.stdin.readline()
  if len(line) < 2:
   break
  log(2, "header_line: " + line)
  line = string.rstrip(line, "\n")
  tmp = line.split(None, 1)
  if tmp[0].find("command:") == 0:
   command = tmp[1]
  if tmp[0].find("friendly:") == 0:
   friendly = tmp[1]	
  if tmp[0].find("length:") == 0:
   length = tmp[1]
  if tmp[0].find("path:") == 0:
   path = tmp[1]
  if tmp[0].find("target:") == 0:
   target = tmp[1]
  if tmp[0].find("type:") == 0:
   type = tmp[1]
  if tmp[0].find("local:") == 0:
   local = tmp[1]
  if tmp[0].find("remote:") == 0:
   remote = tmp[1]
  if tmp[0].find("filename:") == 0:
   filename = tmp[1]
  if tmp[0].find("description:") == 0:
   description = tmp[1]
  if tmp[0].find("connection_id:") == 0:
   connectionid = tmp[1]
  if tmp[0].find("count:") == 0:
   count = tmp[1]
  if tmp[0].find("data_type:") == 0:
   datatype = tmp[1]
  if tmp[0].find("server_session:") == 0:
   serversession = tmp[1]

 # -- PROCESS commands --

 # -- PUT --
 if command == "put":
  fullpath = BASEPATH + path + "/" + filename
  log(1, "PUT " + fullpath + " " + length + " Bytes\n")
  if len(filename) == 1:
   clean_exit(2)
  # create
  if datatype == "1":
   if os.access(fullpath, os.F_OK) and OVERWRITE == 0:
    log(1, "PUT " + fullpath + " exits and overwrite is disabled\n")
    clean_exit(2)
   fp = open(fullpath, "w")
   if fp == -1:
    log(1, "PUT " + fullpath + " can't be created/overwritten\n")
    clean_exit(2)
   buf = sys.stdin.read(int(length))
   fp.write(buf)
   fp.close()
   log(1, "PUT " + fullpath + " saved\n")
   clean_exit(1)
  # delete file/directory
  else:
   if not os.access(fullpath, os.F_OK) or OVERWRITE == 0:
    log(1, "PUT (delete) " + fullpath + " no such file or no permissions\n")
    clean_exit(2)
   else:
    try:
     os.unlink(fullpath)
     log(1, "PUT (delete)" + fullpath + " deleted\n")
    except OSError:
     os.rmdir(fullpath)
     log(1, "PUT (delete)" + fullpath + " deleted\n")
    clean_exit(1)

 # -- GET --
 elif command == "get":
  # folder listing
  if type == "x-obex/folder-listing":
   log(1, "GET listing of folder: " + BASEPATH + path + "\n")
   if os.access(BASEPATH + path, os.F_OK) == -1:
    log(1, "GET " + BASEPATH + path + " can't be accessed\n")
   buf = "<?xml version=\"1.0\"?>\n" + "<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\n" + "<folder-listing version=\"1.0\">\n"
   lsout = os.popen("ls -la " + BASEPATH + path, "r")
   line = "no empty line"
   while len(line) > 0:
    line = lsout.readline()
    line = string.rstrip(line, "\n")
    parts = line.split(None, 8)
    if len(parts) > 3:
     if parts[0].find("d") == 0:
      if parts[8] == "..":
       buf = buf + "<partenfolder/>\n"
      elif parts[8] != ".":
       buf = buf + "<folder name=\"" + parts[8] + "\" size=\"" + parts[4] + "\"/>\n"
     elif parts[0].find("-") == 0:
      buf = buf + "<file name=\"" + parts[8] + "\" size=\"" + parts[4] + "\"/>\n"
   buf = buf + "</folder-listing>"
   print "length: " + str(len(buf)) + "\n"
   log(2, "GET listing length " + str(len(buf)) + "\n")
   print buf
   log(2, "GET listing: " + buf + "\n");
   clean_exit(1)

  # regular get
  fullpath = BASEPATH + path + "/" + filename
  log(1, "GET " + fullpath + "\n")
  fp = open(fullpath)
  if fp == -1:
   log(1, "GET can't open " + fullpath + "\n")
   clean_exit(2)
  fp.seek(0, 2)
  fpsize = fp.tell()
  fp.seek(0, 0)
  buf = fp.read(fpsize)
  print "length: " + str(fpsize) + "\n"
  log(1, "GET " + fullpath + " " + str(fpsize) + " Bytes\n")
  print buf
  fp.close()
  clean_exit(1)

 # -- CONNECTION STUFF --
 elif command == "connect":
  log(1, "CONNECT\n")
  clean_exit(1)
 elif command == "disconnect":
  log(1, "DISCONNECT\n")
  clean_exit(1)
 elif command == "accept":
  res = dialog("SOBEXSRV", "incoming connection from: " + remote, "")
  log(1, "ACCEPT at security level " + str(res) + "\n")
  clean_exit(res)
 #
 # need to implement mkdir
 #
 elif command == "setpath":
  clean_exit(1)
 
def main():
 # Set X display before initializing GTK
 set_display()
 # start logging
 log_open(LOGFILE)
 
 do()

# execute HANDLER
main()
