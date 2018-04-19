#!/usr/bin/perl

if ($#ARGV == -1) {
    chop($hostname = `hostname`);
} else {
    $hostname = $ARGV[0];
}

if (! (($name,$type,$addr) = (gethostbyname($hostname))[0,2,4])) {
    print STDERR "No such host: $hostname\n";
    exit(1);
}
if (! ($qualname = (gethostbyaddr($addr,$type))[0])) {
    $qualname = $name;
}

$qualname =~ tr/A-Z/a-z/;	# lowercase our name for keytab use.
print "$qualname\n";

