#!/usr/local/bin/perl
eval "exec /usr/local/bin/perl -S $0 $*"
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_]+=)(.*)/ && shift;
			# process any FOO=bar switches

$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

print

  '# Dieses Makefile ist mit b2gmake aus einem Borland-Makefile generiert worden.';
print '# DO NOT EDIT THIS FILE!';
print ' ';
print 'GNUMAKE=1';
print ' ';

while (<>) {
    chop;	# strip record separator

    if ($_ =~ /^!/) {
	print substr($_, 2, 999999);
    }
    elsif ($_ =~ /^[a-zA-Z_]* *=/) {
	$X = $_;

	$s = '=', $X =~ s/$s/:=/;

	print $X;
    }
    elsif ($_ =~ /^\.PATH./) {
	$X = substr($_, 7, 999999);
	$s = '=', $X =~ s/$s/ /g;
	print 'vpath %.' . $X;
    }
    elsif ($_ =~ /^\.[a-z]*\.[a-z]*:/) {
	$M = substr($_, 2, 999999) =~ "\\." &&

	  ($RLENGTH = length($&), $RSTART = length($`)+1);
	$depend = substr($_, 2, $M - 1);
	$target = substr($_, $M + 2, 999999);
	$s = '[ :]', $target =~ s/$s//g;
	print (('%.' . $target . ': %.' . $depend));
    }
    elsif ($_ =~ /^ /) {
	print "\t" . substr($_, 2, 999999);
    }
    else {
	print $_;
    }
}