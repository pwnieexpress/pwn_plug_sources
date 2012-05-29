#!/usr/bin/perl
# livelog.pl - Simple Perl script to generate Bluelog Live HTML
# Version 1.0

use strict;

my $BLFILE="/tmp/live.log";
my $DATE=localtime();

print <<HEAD;
<html>
<head>
<title>Bluelog Live</title>
<meta content="20" http-equiv="refresh">
</head>
<body>
<a href="http://www.digifail.com/">Bluelog Live</a>
<hr>
Discovered Devices (Updated $DATE)
<table border=1>
<tr>
<th>Time Discovered</th>
<th>MAC Address</th>
<th>Device Name</th>
<th>Device Class</th>
<th>Capabilities</th>
</tr>
HEAD

open(BL,$BLFILE);
while(my $line=<BL>)
{
	print $line;
}
close(BL);

print <<TAIL;
</table>
</body>
</html>
TAIL

