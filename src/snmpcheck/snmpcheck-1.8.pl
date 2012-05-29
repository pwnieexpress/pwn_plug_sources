#!/usr/bin/env perl

#
# Copyright (c) 2005-2010 by Matteo Cantoni (www.nothink.org)
#
# Like to snmpwalk, snmpcheck permits to enumerate information via SNMP protocol.
# It allows enumeration (hardware, software, network) of any devices with SNMP protocol support.
# It could be useful for penetration testing or systems monitoring.
#
# Distributed under GPL license and based on "Athena-2k" script by jshaw. 
# More informations available from http://www.nothink.org.
#
# License: (http://www.gnu.org/licenses/gpl.txt)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

use strict;
use warnings;

$|=1;

use threads;
use Getopt::Std;
use Net::SNMP qw(ticks_to_time OCTET_STRING DEBUG_ALL);
use Number::Bytes::Human qw(format_bytes);
use Time::HiRes qw(gettimeofday tv_interval);

my $name        = "snmpcheck.pl";
my $version     = "v1.8";
my $description = "SNMP enumerator";
my $copyright   = "Copyright (c) 2005-2011";
my $author      = "Matteo Cantoni (www.nothink.org)";

# MIBs Involved 
#
# System 
my $mibDescr                       = "1.3.6.1.2.1.1.1.0";            # System Description
my $mibNTDomain                    = "1.3.6.1.4.1.77.1.4.1.0";       # NT Primary Domain
my $mibUptimeInstance              = "1.3.6.1.2.1.1.3.0";            # Uptime Instance
my $mibSystemUptime                = "1.3.6.1.2.1.25.1.1.0";         # System Uptime
my $mibSystemDate                  = "1.3.6.1.2.1.25.1.2.0";         # System Date 
my $mibContact                     = "1.3.6.1.2.1.1.4.0";            # System Contact
my $mibName                        = "1.3.6.1.2.1.1.5.0";            # System Name
my $mibLocation                    = "1.3.6.1.2.1.1.6.0";            # System Location
my $mibServices                    = "1.3.6.1.4.1.77.1.2.3.1.1";     # Services (add to it)
my $mibAccounts                    = "1.3.6.1.4.1.77.1.2.25";        # User Accounts
my $mibMemSize                     = "1.3.6.1.2.1.25.2.2.0";         # Total System Memory
my $mibMotd                        = "1.3.6.1.4.1.42.3.1.3.0";       # Motd (Solaris)
# Devices
my $mibDevIndex		           = "1.3.6.1.2.1.25.3.2.1.1";	     # Devices Index
my $mibDevType		           = "1.3.6.1.2.1.25.3.2.1.2";	     # Devices Type 
my $mibDevList                     = "1.3.6.1.2.1.25.3.2.1.3";       # Devices List
my $mibDevStatus                   = "1.3.6.1.2.1.25.3.2.1.5";       # Devices Status
# Processs
my $mibProcesses                   = "1.3.6.1.2.1.25.1.6.0";         # System Processes 
my $mibRunIndex                    = "1.3.6.1.2.1.25.4.2.1.1";       # Running PIDs 
my $mibRunName                     = "1.3.6.1.2.1.25.4.2.1.2";       # Running Programs
my $mibRunPath                     = "1.3.6.1.2.1.25.4.2.1.4";       # Processes Path
my $mibRunParameters               = "1.3.6.1.2.1.25.4.2.1.5";       # Processes Parameters
my $mibRunType                     = "1.3.6.1.2.1.25.4.2.1.6";       # Processes Type
my $mibRunStatus                   = "1.3.6.1.2.1.25.4.2.1.7";       # Processes Status
my $mibProName                     = "1.3.6.1.4.1.42.3.12.1.1.10";   # Running Process Name (Solaris)
my $mibProPid                      = "1.3.6.1.4.1.42.3.12.1.1.1";    # Running Process Pid (Solaris)
my $mibProUser                     = "1.3.6.1.4.1.42.3.12.1.1.8";    # Running Process User (Solaris)
# Storage 
my $mibStorIndex                   = "1.3.6.1.2.1.25.2.3.1.1";       # Storage Device Index 
my $mibStorType                    = "1.3.6.1.2.1.25.2.3.1.2";       # Storage Device Type
my $mibStorDescr                   = "1.3.6.1.2.1.25.2.3.1.3";       # Storage Description
my $mibStorUnits                   = "1.3.6.1.2.1.25.2.3.1.4";       # Storage Units 
my $mibStorSize                    = "1.3.6.1.2.1.25.2.3.1.5";       # Storage Total Size
my $mibStorUsed                    = "1.3.6.1.2.1.25.2.3.1.6";       # Storage Used
my $mibPtype                       = "1.3.6.1.2.1.25.3.8.1.4";       # Partition Type
# Network
my $mibInt                         = "1.3.6.1.2.1.2.2.1.2";          # Network Interfaces
my $mibIntMTU                      = "1.3.6.1.2.1.2.2.1.4";          # Net Int MTU Size
my $mibIntSpeed                    = "1.3.6.1.2.1.2.2.1.5";          # Net Int Speed
my $mibIntBytesIn                  = "1.3.6.1.2.1.2.2.1.10";         # Net Int Octets In
my $mibIntBytesOut                 = "1.3.6.1.2.1.2.2.1.16";         # Net Int Octects Out
my $mibIntPhys                     = "1.3.6.1.2.1.2.2.1.6";          # Int MAC addr
my $mibAdminStat                   = "1.3.6.1.2.1.2.2.1.7";          # Int up/down?
my $mibIPForward                   = "1.3.6.1.2.1.4.1.0";            # IP Forwarding?
my $mibIPAddr                      = "1.3.6.1.2.1.4.20.1.1";         # Int IP Address
my $mibNetmask                     = "1.3.6.1.2.1.4.20.1.3";         # Int IP Netmask
# Software
my $mibSoftIndex                   = "1.3.6.1.2.1.25.6.3.1.1";       # Software Index
my $mibSoftName                    = "1.3.6.1.2.1.25.6.3.1.2";       # Software Name 
# Shares
my $mibShareName                   = "1.3.6.1.4.1.77.1.2.27.1.1";    # Reports Share Names
my $mibSharePath                   = "1.3.6.1.4.1.77.1.2.27.1.2";    # Reports Share Path
my $mibShareComm                   = "1.3.6.1.4.1.77.1.2.27.1.3";    # Reports Share Comments
# Routing Info
my $mibRouteDest                   = "1.3.6.1.2.1.4.21.1.1";         # Route Destinations
my $mibRouteMetric                 = "1.3.6.1.2.1.4.21.1.3";         # Route Metric
my $mibRouteNHop                   = "1.3.6.1.2.1.4.21.1.7";         # Route Next Hop 
my $mibRouteMask                   = "1.3.6.1.2.1.4.21.1.11";        # Route Mask
# IP
my $mibDefaultTTL                  = "1.3.6.1.2.1.4.2.0";            # IP Default TTL
my $mibInReceives                  = "1.3.6.1.2.1.4.3.0";            # IP In Receives
my $mibInDelivers                  = "1.3.6.1.2.1.4.9.0";            # IP In Delivers
my $mibOutRequests                 = "1.3.6.1.2.1.4.10.0";           # IP Out Requests
# TCP
my $mibTCPInSegs                   = "1.3.6.1.2.1.6.10.0";           # TCP In Segs
my $mibTCPOutSegs                  = "1.3.6.1.2.1.6.11.0";           # TCP Out Segs
my $mibTCPRetransSegs              = "1.3.6.1.2.1.6.12.0";           # TCP Retrans Segs
my $mibTCPState                    = "1.3.6.1.2.1.6.13.1.1";         # TCP Connect State
my $mibTCPLAddr                    = "1.3.6.1.2.1.6.13.1.2";         # TCP Local Address
my $mibTCPLPort                    = "1.3.6.1.2.1.6.13.1.3";         # TCP Local Port
my $mibTCPRAddr                    = "1.3.6.1.2.1.6.13.1.4";         # TCP Remote Address
my $mibTCPRPort                    = "1.3.6.1.2.1.6.13.1.5";         # TCP Remote Port
# UDP
my $mibUDPLAddr                    = "1.3.6.1.2.1.7.5.1.1";          # UDP Local Address
my $mibUDPLPort                    = "1.3.6.1.2.1.7.5.1.2";          # UDP Local Port
# IIS
my $http_totalBytesSentLowWord     = "1.3.6.1.4.1.311.1.7.3.1.2.0";  # IIStotalBytesSentLowWord
my $http_totalBytesReceivedLowWord = "1.3.6.1.4.1.311.1.7.3.1.4.0";  # IIStotalBytesReceivedLowWord
my $http_totalFilesSent            = "1.3.6.1.4.1.311.1.7.3.1.5.0";  # IIStotalFilesSent
my $http_currentAnonymousUsers     = "1.3.6.1.4.1.311.1.7.3.1.6.0";  # IIScurrentAnonymousUsers
my $http_currentNonAnonymousUsers  = "1.3.6.1.4.1.311.1.7.3.1.7.0";  # IIScurrentNonAnonymousUsers
my $http_totalAnonymousUsers       = "1.3.6.1.4.1.311.1.7.3.1.8.0";  # IIStotalAnonymousUsers
my $http_totalNonAnonymousUsers    = "1.3.6.1.4.1.311.1.7.3.1.9.0";  # IIStotalNonAnonymousUsers
my $http_maxAnonymousUsers         = "1.3.6.1.4.1.311.1.7.3.1.10.0"; # IISmaxAnonymousUsers
my $http_maxNonAnonymousUsers      = "1.3.6.1.4.1.311.1.7.3.1.11.0"; # IISmaxNonAnonymousUsers
my $http_currentConnections        = "1.3.6.1.4.1.311.1.7.3.1.12.0"; # IIScurrentConnections
my $http_maxConnections            = "1.3.6.1.4.1.311.1.7.3.1.13.0"; # IISmaxConnections
my $http_connectionAttempts        = "1.3.6.1.4.1.311.1.7.3.1.14.0"; # IISconnectionAttempts
my $http_logonAttempts	           = "1.3.6.1.4.1.311.1.7.3.1.15.0"; # IISlogonAttempts
my $http_totalGets	           = "1.3.6.1.4.1.311.1.7.3.1.16.0"; # IIStotalGets
my $http_totalPosts	           = "1.3.6.1.4.1.311.1.7.3.1.17.0"; # IIStotalPosts
my $http_totalHeads	           = "1.3.6.1.4.1.311.1.7.3.1.18.0"; # IIStotalHeads
my $http_totalOthers	           = "1.3.6.1.4.1.311.1.7.3.1.19.0"; # IIStotalOthers
my $http_totalCGIRequests          = "1.3.6.1.4.1.311.1.7.3.1.20.0"; # IIStotalCGIRequests
my $http_totalBGIRequests          = "1.3.6.1.4.1.311.1.7.3.1.21.0"; # IIStotalBGIRequests
my $http_totalNotFoundErrors       = "1.3.6.1.4.1.311.1.7.3.1.22.0"; # IIStotalNotFoundErrors

our ($opt_t,$opt_p,$opt_c,$opt_v,$opt_r,$opt_w,$opt_d,$opt_T,$opt_D,$opt_h);
getopts("t:p:c:v:r:wdT:Dh");

my $community = $opt_c || "public";
my $port      = $opt_p || 161;
my $snmpver   = $opt_v || 1;
my $retries   = $opt_r || 0;
my $timeout   = $opt_T || 20;

my $usage = "$name $version - $description\n$copyright by $author\n
 Usage ./$name -t <IP address>\n
\t-t : target host;

\t-p : SNMP port; default port is $port;
\t-c : SNMP community; default is $community;
\t-v : SNMP version (1,2); default is $snmpver;
\t-r : request retries; default is $retries;

\t-w : detect write access (separate action by enumeration);

\t-d : disable 'TCP connections' enumeration!
\t-T : force timeout in seconds; default is $timeout. Max is 60;
\t-D : enable debug;
\t-h : show help menu;\n\n";

die $usage if $opt_h;
die $usage unless $opt_t;

my ($target,$session,$error,$debug,$thr);

if ($opt_t !~ /^([01]?\d\d?|2[0-4]\d|25[0-5])\.([01]?\d\d?|2[0-4]\d|25[0-5])\.([01]?\d\d?|2[0-4]\d|25[0-5])\.([01]?\d\d?|2[0-4]\d|25[0-5])$/){
	die " [*] Error: target is not a valid IP address!\n";
} else {
	$target = $opt_t;
}

if (($opt_T) and ($opt_T !~ /^(\d+)$/) or ($timeout < 0 or $timeout > 60)){
	die " [*] Error: max timeout value is 60 seconds!\n";
}

if ($opt_v and $opt_v !~ /1|2/){
	die " [*] Error: currently only SNMPv1 and SNMPv2c permitted!\n";
}

if ($opt_D){
	$debug = "0x02";
} else{
	$debug = "0";
}

if (!$opt_T){
	$thr = threads->new(\&WaitProcess,30);
	$thr->detach();
}

$SIG{INT} = sub {
	if ($session){
		print " [*] Closing connection...\n";
		$session->close;
	}

	$thr->kill('SIGUSR1');
	threads->exit();

	exit(1);
};

print "$name $version - $description\n$copyright by $author\n\n";

print " [*] Try to connect to $target\n";

local $SIG{ALRM} = sub {
	if ($session){
		print " [*] Timeout while connecting to host $target\n\n";
		$session->close;
	}
	exit(0);
};

alarm $timeout if $opt_T;

Connection($target);

if (!$opt_T){
	$thr->kill('SIGUSR1');
	threads->exit();
}

print "\n";

exit(0);

sub Connection {
	my $target = shift;

	($session,$error) = Net::SNMP->session(
		Hostname  => $target,
		Community => $community,
		Domain    => 'udp',
		Port      => $port,
		Version   => $snmpver,
		Timeout   => $timeout,
		Retries   => $retries,
		Debug     => $debug,
		Translate => [
			-timeticks => 0x0
		]
	);

	if (!$session){
		printf " [*] Error: %s.\n", $session->error();
		$session->close();
		if (!$opt_T){
			$thr->kill('SIGUSR1');
			threads->exit();
		}
		exit(1);
	}

	my $start_time = [gettimeofday];

	print " [*] Connected to $target\n";

	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
	printf " [*] Starting enumeration at %4d-%02d-%02d %02d:%02d:%02d\n",$year+1900,$mon+1,$mday,$hour,$min,$sec;

	# Write access check 
	if ($opt_w){
	
		my $contact = $session->get_request(
			-varbindlist => [ $mibContact ]
		);
	
		if (!$contact){
			printf " [*] Error: %s.\n", $session->error();
			$session->close();
			if (!$opt_T){
				$thr->kill('SIGUSR1');
				threads->exit();
			}
			exit(1);
		}

		my $check = $session->set_request(
			-varbindlist => [$mibContact, OCTET_STRING ,'testwriteaccess']
		);
		
		if ($check){
			print " [*] Write access enabled!\n";
			$session->set_request(
				-varbindlist => [$mibContact, OCTET_STRING ,$contact]
			);
		} else {
			print " [*] No write access enabled.\n";
		}
		
		$session->close;

		my $end_time = [gettimeofday];
		my $elapsed  = tv_interval($start_time,$end_time);

		printf " [*] Checked $target in %.2f seconds\n",$elapsed;
		
		exit(0);
	}
	
	my $hostname = $session->get_request(
		-varbindlist => [ $mibName ]
	);

	if (!$hostname){
		printf " [*] Error: %s.\n", $session->error();
		$session->close();
		if (!$opt_T){
			$thr->kill('SIGUSR1');
			threads->exit();
		}
		exit(1);
	} else {
		$hostname = $hostname->{$mibName};
	}

	my $descr = SNMPSystem($hostname);

	SNMPDevices()         if $descr !~ /^-|^Cisco|^Fibre|^Sun/; 
	SNMPStorage()         if $descr !~ /^-|^Cisco|^Fibre|^Sun/;
	SNMPAccounts()        if $descr !~ /^-|^Cisco|^Fibre|^Sun|^Linux/;
	SNMPProcesses($descr) if $descr !~ /^-|^Cisco|^Fibre|^Sun/;
	SNMPNetInfo();
	SNMPRoutingInfo();
	SNMPNetServices();
	SNMPTCPConnections()  if !$opt_d;
	SNMPUDPPorts();
	SNMPSoftware()        if $descr !~ /^-|^Cisco|^Fibre|^Sun/;
	SNMPShares()          if $descr !~ /^-|^Cisco|^Fibre|^Linux/;
	SNMPMountPoints()     if $descr !~ /^-|^Cisco|^Fibre|^Hardware/;
	SNMPIIS()             if $descr =~ /^Hardware/;
	
	$session->close;

	my $end_time = [gettimeofday];
	my $elapsed  = tv_interval($start_time,$end_time);

	printf "\n [*] Enumerated $target in %.2f seconds\n",$elapsed;
}

sub SNMPSystem {

	my $hostname       = shift;
	my $descr          = GetRequest($mibDescr);
	my $uptimeinstance = GetRequest($mibUptimeInstance);
	my $systemuptime   = GetRequest($mibSystemUptime);
	my $contact        = GetRequest($mibContact);
	my $location       = GetRequest($mibLocation);
	my $motd           = GetRequest($mibMotd);
	my $ntdomain       = GetRequest($mibNTDomain) if $descr =~ /^Hardware/;

	$descr = Sanitize($descr);
	$motd  = Sanitize($motd);

	PrintTitle("System information");
		
	$uptimeinstance = ticks_to_time($uptimeinstance);
	$systemuptime   = ticks_to_time($systemuptime);

	print " Hostname               : $hostname\n"       if $hostname;
	print " Description            : $descr\n"          if $descr;
	print " Uptime system          : $systemuptime\n"   if $systemuptime;
	print " Uptime SNMP daemon     : $uptimeinstance\n" if $uptimeinstance;
	print " Contact                : $contact\n"        if $contact;
	print " Location               : $location\n"       if $location;
	print " Motd                   : $motd\n"           if $motd;
	print " Domain (NT)            : $ntdomain\n"       if $ntdomain;

	return $descr;
}

sub SNMPStorage {
	
	my @storindex = GetTable($mibStorIndex);
	my @stortype  = GetTable($mibStorType);
	my @stordescr = GetTable($mibStorDescr);
	my @storsize  = GetTable($mibStorSize); 
	my @storused  = GetTable($mibStorUsed); 
	my @storunits = GetTable($mibStorUnits); 
	my @ptype     = GetTable($mibPtype);

	if ($#stordescr > 0){
	
		PrintTitle("Storage information");
	
		my $a = 0;
	
		foreach (@stordescr){

			my %storagetypes = (
				'1.3.6.1.2.1.25.2.1.1'  => 'Other',
				'1.3.6.1.2.1.25.2.1.2'  => 'Ram',
				'1.3.6.1.2.1.25.2.1.3'  => 'Virtual Memory',
				'1.3.6.1.2.1.25.2.1.4'  => 'Fixed Disk',
				'1.3.6.1.2.1.25.2.1.5'  => 'Removable Disk',
				'1.3.6.1.2.1.25.2.1.6'  => 'Floppy Disk',
				'1.3.6.1.2.1.25.2.1.7'  => 'Compact Disc',
				'1.3.6.1.2.1.25.2.1.8'  => 'Ram Disk',
				'1.3.6.1.2.1.25.2.1.9'  => 'Flash Memory',
				'1.3.6.1.2.1.25.2.1.10' => 'Network Disk'
			);

			my %fstypes = (
				'1.3.6.1.2.1.25.3.9.1'  => 'Other',
				'1.3.6.1.2.1.25.3.9.2'  => 'Unknown',
				'1.3.6.1.2.1.25.3.9.3'  => 'BerkeleyFFS',
				'1.3.6.1.2.1.25.3.9.4'  => 'Sys5FS',
				'1.3.6.1.2.1.25.3.9.5'  => 'Fat',
				'1.3.6.1.2.1.25.3.9.6'  => 'HPFS',
				'1.3.6.1.2.1.25.3.9.7'  => 'HFS',
				'1.3.6.1.2.1.25.3.9.8'  => 'MFS',
				'1.3.6.1.2.1.25.3.9.9'  => 'NTFS',
				'1.3.6.1.2.1.25.3.9.10' => 'VNode',
				'1.3.6.1.2.1.25.3.9.11' => 'Journaled',
				'1.3.6.1.2.1.25.3.9.12' => 'iso9660',
				'1.3.6.1.2.1.25.3.9.13' => 'RockRidge',
				'1.3.6.1.2.1.25.3.9.14' => 'NFS',
				'1.3.6.1.2.1.25.3.9.15' => 'Netware',
				'1.3.6.1.2.1.25.3.9.16' => 'AFS',
				'1.3.6.1.2.1.25.3.9.17' => 'DFS',
				'1.3.6.1.2.1.25.3.9.18' => 'Appleshare',
				'1.3.6.1.2.1.25.3.9.19' => 'RFS',
				'1.3.6.1.2.1.25.3.9.20' => 'DGCFS',
				'1.3.6.1.2.1.25.3.9.21' => 'BFS',
				'1.3.6.1.2.1.25.3.9.22' => 'FAT32',
				'1.3.6.1.2.1.25.3.9.23' => 'LinuxExt2'
			);

			print " $_\n";

			print "\tDevice id       : $storindex[$a]\n" if $storindex[$a];
			print "\tDevice type     : $storagetypes{$stortype[$a]}\n" if $storagetypes{$stortype[$a]};

			if ($ptype[$a]){
				if ($fstypes{$ptype[$a]}){
					print "\tFilesystem type : $fstypes{$ptype[$a]}\n";
				} else{
					print "\tFilesystem type : Unknown\n";
				}
			} else{
				print "\tFilesystem type : Unknown\n";
			}

			if ($storunits[$a]){
				print "\tDevice units    : $storunits[$a]\n";

				if ($storsize[$a]){
					$storsize[$a] = ($storsize[$a] * $storunits[$a]);
					my $s = format_bytes($storsize[$a]);
					print "\tMemory size     : $s\n";
				}
			
				if ($storused[$a]){
					$storused[$a] = $storused[$a] * $storunits[$a];
					my $s = format_bytes($storused[$a]);
					print "\tMemory used     : $s\n";
				}
		
				if ($storsize[$a] and $storused[$a]){
					my $free = $storsize[$a] - $storused[$a];
					$free = format_bytes($free);
					print "\tMemory free     : $free\n";
				}
			}

			print "\n";
			
			$a++;
		}
	}
}

sub SNMPDevices {

	my @devicesindex = GetTable($mibDevIndex);
	my @devicestype  = GetTable($mibDevType);
	my @deviceslist  = GetTable($mibDevList);
	my @devicestatus = GetTable($mibDevStatus);

	if ($#deviceslist > 0){
	
		PrintTitle("Devices information");
		
		my %devicetype = (
			'1.3.6.1.2.1.25.3.1.1'  => 'Other',
			'1.3.6.1.2.1.25.3.1.2'  => 'Unknown',
			'1.3.6.1.2.1.25.3.1.3'  => 'Processor',
			'1.3.6.1.2.1.25.3.1.4'  => 'Network',
			'1.3.6.1.2.1.25.3.1.5'  => 'Printer',
			'1.3.6.1.2.1.25.3.1.6'  => 'Disk Storage',
			'1.3.6.1.2.1.25.3.1.10' => 'Video',
			'1.3.6.1.2.1.25.3.1.11' => 'Audio',
			'1.3.6.1.2.1.25.3.1.12' => 'Coprocessor',
			'1.3.6.1.2.1.25.3.1.13' => 'Keyboard',
			'1.3.6.1.2.1.25.3.1.14' => 'Modem',
			'1.3.6.1.2.1.25.3.1.15' => 'Parallel Port',
			'1.3.6.1.2.1.25.3.1.16' => 'Pointing',
			'1.3.6.1.2.1.25.3.1.17' => 'Serial Port',
			'1.3.6.1.2.1.25.3.1.18' => 'Tape',
			'1.3.6.1.2.1.25.3.1.19' => 'Clock',
			'1.3.6.1.2.1.25.3.1.20' => 'Volatile Memory',
			'1.3.6.1.2.1.25.3.1.21' => 'Non Volatile Memory'
		);

		printf " %5s %20s  %7s  %s\n\n","Id","Type","Status","Description";

		my $a = 0;

		foreach (@deviceslist){

			my $id     = '-';
			my $type   = '-';
			my $status = 'Unknown';
			my $descr  = $_;

			$type = $devicetype{$devicestype[$a]};

			if ($devicestatus[$a]){
				if ($devicestatus[$a] eq '1'){
					$status = "Unknown";
				}elsif($devicestatus[$a] eq '2'){
					$status = "Running";
				}elsif($devicestatus[$a] eq '3'){
					$status = "Warning";
				}elsif($devicestatus[$a] eq '4'){
					$status = "Testing";
				}elsif($devicestatus[$a] eq '5'){
					$status = "Down";
				}
			}

			printf " %5s %20s  %7s  %s\n",$devicesindex[$a],$type,$status,$descr;
			
			$a++;
		}
	}
}

sub SNMPAccounts {

	my @accounts = GetTable($mibAccounts);

	if ($#accounts > 0){
		
		PrintTitle("User accounts");

		@accounts = sort @accounts;
		foreach(@accounts){
			print " $_\n";
		}
	}
}

sub SNMPProcesses {

	my $descr = shift;

	if ($descr =~ /^Sun/){
	
		# Solaris
		my @runproid    = GetTable("1.3.6.1.4.1.42.3.12.1.1.1");  # psProcessID 
		my @runparid    = GetTable("1.3.6.1.4.1.42.3.12.1.1.2");  # psParentProcessID
		my @runprosize  = GetTable("1.3.6.1.4.1.42.3.12.1.1.3");  # psProcessSize
		my @runcputime  = GetTable("1.3.6.1.4.1.42.3.12.1.1.4");  # psProcessCpuTime
		my @runstate    = GetTable("1.3.6.1.4.1.42.3.12.1.1.5");  # psProcessState
		my @runtty      = GetTable("1.3.6.1.4.1.42.3.12.1.1.7");  # psProcessTTY
		my @runusername = GetTable("1.3.6.1.4.1.42.3.12.1.1.8");  # psProcessUserName
		my @runuserid   = GetTable("1.3.6.1.4.1.42.3.12.1.1.9");  # psProcessUserID
		my @runname     = GetTable("1.3.6.1.4.1.42.3.12.1.1.10"); # psProcessName
		my @runstatus   = GetTable("1.3.6.1.4.1.42.3.12.1.1.11"); # psProcessStatus
	
		if ($#runproid > 0){
			
			PrintTitle("Processes");

			print "   Pid    Ppid    Size Cputime  State      TTY     Username   Uid          Name    Status\n\n";

			for (my $a = 0; $a < $#runproid; $a++){
				printf " %6s %6s %6s %6s %6s %10s %10s %6s %15s %6s\n", $runproid[$a],$runparid[$a],$runprosize[$a],$runcputime[$a],$runstate[$a],$runtty[$a],$runusername[$a],$runuserid[$a],$runname[$a],$runstatus[$a];
			}
		}

	} else{
		# Other
		my $processes = GetRequest($mibProcesses);
		my @runindex  = GetTable($mibRunIndex);
		my @runname   = GetTable($mibRunName);
		my @runpath   = GetTable($mibRunPath);
		my @runtype   = GetTable($mibRunType);
		my @runstatus = GetTable($mibRunStatus);
	
		if ($#runindex > 0){
			
			PrintTitle("Processes");

			print " Total processes : $processes\n\n" if $processes;
			print " Process type    : 1 unknown, 2 operating system, 3 device driver, 4 application\n"; 
			print " Process status  : 1 running, 2 runnable, 3 not runnable, 4 invalid\n\n"; 
			
			printf " %10s %25s %13s %15s  Process path\n\n", "Process id","Process name","Process type","Process status";

			for (my $a = 0; $a < $#runindex; $a++){
				if ($runname[$a] ne " System Idle Process"){
					printf " %10s %25s %13s %15s  $runpath[$a]\n", $runindex[$a],$runname[$a],$runtype[$a],$runstatus[$a];
				}
			}
		}
	}
}

sub SNMPNetInfo {

	PrintTitle("Network information");

	my $ipforward      = GetRequest($mibIPForward);
	my $DefaultTTL     = GetRequest($mibDefaultTTL);
	my $TCPInSegs      = GetRequest($mibTCPInSegs);
	my $TCPOutSegs     = GetRequest($mibTCPOutSegs);
	my $TCPRetransSegs = GetRequest($mibTCPRetransSegs);
	my $InReceives     = GetRequest($mibInReceives);
	my $InDelivers     = GetRequest($mibInDelivers);
	my $OutRequests    = GetRequest($mibOutRequests);

	if ($ipforward eq "0" || $ipforward eq "2") { $ipforward = "no"; }

	print " IP forwarding enabled   : $ipforward\n"; 
	print " Default TTL             : $DefaultTTL\n";
	print " TCP segments received   : $TCPInSegs\n";
	print " TCP segments sent       : $TCPOutSegs\n";
	print " TCP segments retrans.   : $TCPRetransSegs\n"; 
	print " Input datagrams         : $InReceives\n"; 
	print " Delivered datagrams     : $InDelivers\n"; 
	print " Output datagrams        : $OutRequests\n"; 

	my @int         = GetTable($mibInt);
	my @mtu         = GetTable($mibIntMTU);
	my @intspeed    = GetTable($mibIntSpeed);
	my @intbytesin  = GetTable($mibIntBytesIn);
	my @intbytesout = GetTable($mibIntBytesOut);
	my @intphys     = GetTable($mibIntPhys);
	my @ipaddr      = GetTable($mibIPAddr);
	my @netmask     = GetTable($mibNetmask);
	my @adminstat   = GetTable($mibAdminStat);

	if ($#int > 0){ 
		
		PrintTitle("Network interfaces");
		
		$#int++;
		for (my $a = 0; $a < $#int; $a++){

			chomp $int[$a];

			if (@adminstat){
				if ($adminstat[$a] eq "0"){
					$adminstat[$a] = "down";
				} else {
					$adminstat[$a] = "up";
				}
			} else {
				$adminstat[$a] = '-';
			}

			if ($intspeed[$a] !~ /-/){

				$intspeed[$a] = $intspeed[$a] / 1000000;

				if ($intphys[$a]){
					$intphys[$a] =~ s/\A..//xms;  # remove the 0x
					$intphys[$a] =~ s/(..)/$1:/g; # add a colon between bytes
					$intphys[$a] =~ s/:$//;       # remove the trailing :
				}

				print " Interface               : [ $adminstat[$a] ] $int[$a]\n\n";
				print "\tHardware Address : $intphys[$a]\n"       if $intphys[$a];
				print "\tInterface Speed  : $intspeed[$a] Mbps\n" if $intspeed[$a];
				print "\tIP Address       : $ipaddr[$a]\n"        if $ipaddr[$a];
				print "\tNetmask          : $netmask[$a]\n"       if $ipaddr[$a];
				print "\tMTU              : $mtu[$a]\n"           if $mtu[$a];
				
				if ($intbytesin[$a]){
					print "\tBytes In         : $intbytesin[$a]";
					$intbytesin[$a] = format_bytes($intbytesin[$a]);
					print " ($intbytesin[$a])\n"; 
				}

				if ($intbytesout[$a]){
					print "\tBytes Out        : $intbytesout[$a]";
					if ($intbytesout[$a] =~ /(\d+)/){
						$intbytesout[$a] = format_bytes($intbytesout[$a]);
					}
					print " ($intbytesout[$a])\n";
				}
	
				print "\n";
			}
		}
	}
}

sub SNMPNetServices {

	my @services = GetTable($mibServices);

	if ($#services > 0){
		
		PrintTitle("Network services");

		@services = sort @services;
		foreach(@services){
			print " $_\n";
		}
	}
}
	
sub SNMPRoutingInfo {

	my @routedest	= GetTable($mibRouteDest);
	my @routenhop	= GetTable($mibRouteNHop);
	my @routemask	= GetTable($mibRouteMask);
	my @routemetric	= GetTable($mibRouteMetric);

	if ($#routedest > 0){
		
		PrintTitle("Routing information");
		
		print "      Destination\t  Next Hop\t       Mask\tMetric\n\n";

		for (my $a = 0; $a < $#routedest; $a++){

			$routedest[$a]   = '-' if !$routedest[$a];
			$routenhop[$a]   = '-' if !$routenhop[$a];
			$routemask[$a]   = '-' if !$routemask[$a];
			$routemetric[$a] = '-' if !$routemetric[$a];

			printf "%17s%17s%17s%9s\n", $routedest[$a], $routenhop[$a], $routemask[$a], $routemetric[$a];
		}
	}
}

sub SNMPTCPConnections {

	my @tcpstate = GetTable($mibTCPState);
	my @tcpladdr = GetTable($mibTCPLAddr); 
	my @tcplport = GetTable($mibTCPLPort);
	my @tcpraddr = GetTable($mibTCPRAddr);
	my @tcprport = GetTable($mibTCPRPort);

	if ($#tcpstate > 0){
		
		PrintTitle("Listening TCP ports and connections");
		
		print "   Local Address   Port      Remote Address   Port       State\n\n";

		for (my $a = 0; $a < $#tcpstate; $a++){
			if ($tcpstate[$a] eq "1")  { $tcpstate[$a] = "Closed";       }
			if ($tcpstate[$a] eq "2")  { $tcpstate[$a] = "Listening";    }
			if ($tcpstate[$a] eq "3")  { $tcpstate[$a] = "SYN sent";     }
			if ($tcpstate[$a] eq "4")  { $tcpstate[$a] = "SYN received"; }
			if ($tcpstate[$a] eq "5")  { $tcpstate[$a] = "Established";  }
			if ($tcpstate[$a] eq "6")  { $tcpstate[$a] = "FIN wait1";    }
			if ($tcpstate[$a] eq "7")  { $tcpstate[$a] = "FIN wait2";    }
			if ($tcpstate[$a] eq "8")  { $tcpstate[$a] = "Close wait";   }
			if ($tcpstate[$a] eq "9")  { $tcpstate[$a] = "Last ack";     }
			if ($tcpstate[$a] eq "10") { $tcpstate[$a] = "Closing";      } 
			if ($tcpstate[$a] eq "11") { $tcpstate[$a] = "Time wait";    }
			if ($tcpstate[$a] eq "12") { $tcpstate[$a] = "Delete tcb";   }

			$tcpladdr[$a] = '-' if !$tcpladdr[$a];
			$tcplport[$a] = '-' if !$tcplport[$a];
			$tcpraddr[$a] = '-' if !$tcpraddr[$a];
			$tcprport[$a] = '-' if !$tcprport[$a];
			$tcpstate[$a] = '-' if !$tcpstate[$a];

			printf " %15s %6s   %17s %6s %15s\n", $tcpladdr[$a], $tcplport[$a], $tcpraddr[$a], $tcprport[$a], $tcpstate[$a];
		}
	}
}

sub SNMPUDPPorts {

	my @udpladdr = GetTable($mibUDPLAddr);
	my @udplport = GetTable($mibUDPLPort);

	if ($#udpladdr > 0){
		
		PrintTitle("Listening UDP ports");
		
		print "   Local Address   Port\n\n";

		for (my $a = 0; $a < $#udpladdr; $a++){
			$udpladdr[$a] = '-' if !$udpladdr[$a];
			$udplport[$a] = '-' if !$udplport[$a];

			printf " %15s %6s\n", $udpladdr[$a], $udplport[$a];
		}
	}
}

sub SNMPSoftware {

	my @softindex = GetTable($mibSoftIndex);
	my @softname  = GetTable($mibSoftName);

	if ($#softname > 0){

		PrintTitle("Software components");
		
		my @soft;
		for (my $a = 0; $a < $#softname; $a++){
			push @soft,"$softindex[$a]. $softname[$a]";
		}

		@soft = sort @soft;
		foreach(@soft){
			print " $_\n";
		}
	}
}

sub SNMPIIS {
		
	my $http_totalBytesSentLowWord     = GetRequest($http_totalBytesSentLowWord); 
	my $http_totalBytesReceivedLowWord = GetRequest($http_totalBytesReceivedLowWord); 
	my $http_totalFilesSent            = GetRequest($http_totalFilesSent);
	my $http_currentAnonymousUsers     = GetRequest($http_currentAnonymousUsers);
	my $http_currentNonAnonymousUsers  = GetRequest($http_currentNonAnonymousUsers);
	my $http_totalAnonymousUsers       = GetRequest($http_totalAnonymousUsers);
	my $http_totalNonAnonymousUsers    = GetRequest($http_totalNonAnonymousUsers);
	my $http_maxAnonymousUsers         = GetRequest($http_maxAnonymousUsers);
	my $http_maxNonAnonymousUsers      = GetRequest($http_maxNonAnonymousUsers);
	my $http_currentConnections        = GetRequest($http_currentConnections);
	my $http_maxConnections            = GetRequest($http_maxConnections);
	my $http_connectionAttempts        = GetRequest($http_connectionAttempts);
	my $http_logonAttempts             = GetRequest($http_logonAttempts);
	my $http_totalGets                 = GetRequest($http_totalGets);
	my $http_totalPosts                = GetRequest($http_totalPosts);
	my $http_totalHeads                = GetRequest($http_totalHeads);
	my $http_totalOthers               = GetRequest($http_totalOthers);
	my $http_totalCGIRequests          = GetRequest($http_totalCGIRequests);
	my $http_totalBGIRequests          = GetRequest($http_totalBGIRequests );
	my $http_totalNotFoundErrors       = GetRequest($http_totalNotFoundErrors);

	if ($http_totalFilesSent){
		PrintTitle("Web server information");
	}

	if ($http_totalBytesSentLowWord){
		if ($http_totalBytesSentLowWord =~ /\d+/){
			$http_totalBytesSentLowWord = format_bytes($http_totalBytesSentLowWord);
			print " Total bytes sent low word     : $http_totalBytesSentLowWord\n";
		} else {
			print " Total bytes sent low word     : -\n";
		}
	}
			
	if ($http_totalBytesReceivedLowWord){
		if ($http_totalBytesReceivedLowWord =~ /\d+/){
			$http_totalBytesReceivedLowWord = format_bytes($http_totalBytesReceivedLowWord);
			print " Total bytes received low word : $http_totalBytesReceivedLowWord\n";
		} else {
			print " Total bytes received low word : -\n";
		}
	}

	print " Total files sent              : $http_totalFilesSent\n"           if $http_totalFilesSent;
	print " Current anonymous users       : $http_currentAnonymousUsers\n"    if $http_currentAnonymousUsers;
	print " Current non anonymous users   : $http_currentNonAnonymousUsers\n" if $http_currentNonAnonymousUsers;
	print " Total anonymous users         : $http_totalAnonymousUsers\n"      if $http_totalAnonymousUsers;
	print " Total non anonymous users     : $http_totalNonAnonymousUsers\n"   if $http_totalNonAnonymousUsers;
	print " Max anonymous users           : $http_maxAnonymousUsers\n"        if $http_maxAnonymousUsers;
	print " Max non anonymous users       : $http_maxNonAnonymousUsers\n"     if $http_maxNonAnonymousUsers;
	print " Current connections           : $http_currentConnections\n"       if $http_currentConnections;
	print " Max connections               : $http_maxConnections\n"           if $http_maxConnections;
	print " Connection attempts           : $http_connectionAttempts\n"       if $http_connectionAttempts;
	print " Logon attempts                : $http_logonAttempts\n"            if $http_logonAttempts;
	print " Total gets                    : $http_totalGets\n"                if $http_totalGets;
	print " Total posts                   : $http_totalPosts\n"               if $http_totalPosts;
	print " Total heads                   : $http_totalHeads\n"               if $http_totalHeads;
	print " Total others                  : $http_totalOthers\n"              if $http_totalOthers;
	print " Total CGI requests            : $http_totalCGIRequests\n"         if $http_totalCGIRequests;
	print " Total BGI requests            : $http_totalBGIRequests\n"         if $http_totalBGIRequests;
	print " Total not found errors        : $http_totalNotFoundErrors\n"      if $http_totalNotFoundErrors;
}

sub SNMPMountPoints {

	my @StorDescr = GetTable($mibStorDescr);

	if ($#StorDescr > 0){
		
		PrintTitle("Mountpoints");
	
		my $x = 0;
		
		for (my $a = 0; $a < $#StorDescr; $a++){
			if ($StorDescr[$a] =~ /\//){
				print " $StorDescr[$a]\n";
				$x++;
			}
		}

		if ($x == 0){
			print " Not available\n";
		}
	}
}

sub SNMPShares {

	my @ShareName = GetTable($mibShareName);
	my @SharePath = GetTable($mibSharePath);
	my @ShareComm = GetTable($mibShareComm);

	if ($#ShareName > 0){
		
		PrintTitle("Non-administrative shares");

		for (my $a = 0; $a < $#ShareName; $a++){
			print " Share Name : $ShareName[$a]\n";
			print " Path       : $SharePath[$a]\n";
			print " Comments   : $ShareComm[$a]\n\n";
		}
	}
}

sub GetRequest {

	my $oid = shift;

	my $result = $session->get_request(
		-varbindlist => [ $oid ]
	);

	if ($result){
		return $result->{$oid};
	} else {
		return '-';
	} 
}

sub GetTable {

	my $oid = shift;

	my @return;
	my $response = '';
	my $x = 0;

	if ($response = $session->get_table(-baseoid => $oid)){
		foreach my $key (sort keys %$response){
			if ($$response{$key} ne " Virtual Memory"){
				$return[$x] = $$response{$key};
				$x++;
			}
		}

		return @return;
	} else{
		return '-';
	}
}

sub PrintTitle {
	
	my $title = shift;

	print "\n [*] $title\n";
	print " "; print "-" x 95; print "\n\n";
}

sub Sanitize {

	my $string = shift;

	if ($string){
		chomp $string;
		$string =~ s/^\s+$|^\s+$|\n+|\r+//g;
	}

	return $string;
}

sub WaitProcess {

	my $timeout = shift;

	while(1){
		sleep $timeout;
		print " [*] Wait...don't stop $name...\n";
	}
}

__END__

=head1 NAME

snmpcheck - Perl script to enumerate information via SNMP protocol 

=head1 SYNOPSIS

./snmpcheck.pl -t <IP address>

-t : target host;

-p : SNMP port; default port is 161;

-c : SNMP community; default is public;

-v : SNMP version (1,2); default is 1;

-r : request retries; default is 2;

-w : detect write access (separate action by enumeration);

-d : disable 'TCP connections' enumeration!

-T : force timeout in seconds; default is 60. Max is 60;

-D : enable debug;

-h : show help menu;

=head1 DESCRIPTION

Like to snmpwalk, snmpcheck permits to enumerate information via SNMP protocol.
It allows enumeration (hardware, software, network) of any devices with SNMP protocol support.
It could be useful for penetration testing or systems monitoring.

Tested on GNU/Linux, *BSD and Windows (Cygwin and ActivePerl) systems.
Distributed under GPL license and based on "Athena-2k" script by jshaw. 

=head1 NOTE

"TCP connections enumeration" can be very long. Use -d flag to disable it.

=head1 BUGS

You can help sending bug reports or writing patches.

=head1 SEE ALSO

http://en.wikipedia.org/wiki/Simple_Network_Management_Protocol
http://net-snmp.sourceforge.net/docs/man/snmpwalk.html

=head1 AUTHOR

Matteo Cantoni, E<lt>matteo.cantoni@nothink.org<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2005-2010 by Matteo Cantoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

=cut
