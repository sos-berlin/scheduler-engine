#!/usr/bin/perl -w

$map = $ARGV[0];  # wieso 0?
#$map = "c:/bin/hostapi.map";

open( MAP, "<" . $map )  or die;

$_ = <MAP>;

### Segementgrößen lesen###

while( defined $_  &&  ! /^ Start         Length Name               Class/ )  { $_ = <MAP>; }
$_ = <MAP>;
while( defined $_  &&  /^ *$/ )                                   { $_ = <MAP>; }

$segsize[0] = 0;  # dummy
$last_seg = "";

while(1) {
    last if !defined $_ || /^ *$/;
    $seg = hex substr( $_, 2, 4 );
    if( substr( $_, 14, 1 ) eq " " ) { $size = hex( substr( $_, 15, 4 ) ) }
                                else { $size = hex( substr( $_, 14, 5 ) ) }
    $segsize[ $seg ] = hex( substr( $_, 7, 4 ) ) + $size;
    $last_seg = $seg;
    $_ = <MAP>;
}


while( defined $_  &&  ! /^  Address         Publics by Value/ )  { $_ = <MAP>; }
$_ = <MAP>;
while( defined $_  &&  /^ *$/ )                                   { $_ = <MAP>; }
while( defined $_  &&  substr( $_, 0, 6 ) eq " 0000:" )           { $_ = <MAP>; }

$seg = "0001";
$n = 0;

seg: while(1)
{
    $last_off = 0;
    $last_name = "";

    off: while(1)
    {
        last seg if !defined $_  ||  /^ *$/;
        if( substr( $_, 1, 4 ) ne $seg ) { $_ = <MAP>; last; }
        $off = hex substr( $_, 6, 5 );
        if( $last_name ne "" ) {
            $size = $off - $last_off;
            if( $size > 0 ) {
                $a[ $n++ ] = substr( "        " . $size, -7 ) . " " . $last_name;
            }
        }
        $last_off  = $off;
        $last_name = substr( $_, 17 );
        $_ = <MAP>;
    }
    if( $last_name ne "" ) {
        $a[ $n++ ] = substr( "        " . ($segsize[hex $seg]-$last_off), -7 ) . " " . $last_name;
    }

    $seg = substr( $_, 1, 4 );
}

@a = sort @a;

for( $i = $n - 1; $i >= 0; $i-- ) {
    print $a[ $i ];
}

######################################################
#
#while( defined $_  &&  ! /^Line numbers for / )  { $_ = <MAP>; }
#
#while( defined $_  &&  /^Line numbers for / )
#{
#    $filename = substr( $_, 17 );
#    $filename = substr( $filename, 0, index( $filename, " " ) );
#    print $filename, "\n";
#    $_ = <MAP>;
#
#    while( defined $_  &&  /^ *$/ )  { $_ = <MAP>; }
#
#    $m = 0;
#    $last_line = 0;
#    $last_off  = 0;
#
#    while(1) {
#        $rest = $_;
#        $last_off = 0;
#        $i = 0;
#        while( $rest !~ /^ *$/ ) {
#            print "rest=", $rest, "\n";
#            #( $x, $line, $x, $seg, $off, $rest ) = $
#            ( $x, $line ) = ( $rest =~ /^ *[0-9]* / );
#            rest =~ /^ *[0-9]* *[0-9A-F]*:[0-9A-F]*.*$]/;
#            print "x=",$x, ",";
#            print "line=",$line, ",";
#            print "seg=",$seg, ",";
#            print "off=",$off, "\n";
#            #$rest = substr $rest, pos();
#            last if !defined $line  ||  $line < $last_line;
#            $size = hex $off - hex $last_off;
#            if( $size >= 0 ) {
#                #$b[ $m++ ] =
#                print substr( " " x 7 . $line, -7 ) . " " . substr( " " x 7 . $size, -7 ) . " " . $filename;
#            }
#            $last_off  = $off;
#            $last_line = $line;
#            $i++;
#        }
#
#        $_ = <MAP>;
#        last if $i < 4;
#    }
#
#    while( defined $_  &&  /^ *$/ )  { $_ = <MAP>; }
#}
