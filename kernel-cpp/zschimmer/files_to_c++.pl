#! /usr/bin/perl -W
# $Id: scheduler_keyword_to_xml.pl 3305 2005-01-12 09:15:50Z jz $

use File::stat;

$script_name = "files_to_c++.pl";
$string_size = 4096;
$name = "embedded_files";


sub file_to_cxx
{
    my $filename = shift;
    my $name     = shift;
    
    my $hex = "0123456789abcdef";
    
    print "const char $name" . "[] = \n";

    
    my $o_binary = 0x8000;  # Nur Windows
    my $o_rdonly = 0;
    
    if( $gzip )
    {
        open FILE, "gzip <$filename |"  or die "$filename ist nicht lesbar: $!";
    }
    else
    {
        #sysopen(  FILE, "$filename", $o_rdonly | $o_binary )  or die "$filename ist nicht lesbar: $!";
        open FILE, "<$filename"  or die "$filename ist nicht lesbar";
    }
    
    binmode FILE;
    print "    \"";
    my $data;
    my $at_begin_of_line = 1;
        
    
    while( read( FILE, $data, $string_size ) )
    {            
        for( my $i = 0; $i < length( $data ); $i++ )
        {                    
            if( $trim  &&  $at_begin_of_line )
            { 
                if( $soft_trim   &&  substr( $data, $i, 1 ) eq ' ' )  { print " "; }
                while( $i < length( $data )  &&  substr( $data, $i, 1 ) eq ' ' ) { $i++; }
                if( $i < length( $data ) )  { $at_begin_of_line = 0; }
            }    
            
            if( $i < length( $data ) )
            {
                my $c = substr( $data, $i, 1 );
                my $o = ord( $c );
                
                if( $gzip )
                {
                    if( $i > 0  &&  $i % 32 == 0 ) { print "\"\n    \""; }
                    print "\\x" . substr( $hex, $o / 16, 1 ) . substr( $hex, $o % 16, 1 );
                }
                elsif( $o >= 0x20  &&  $o < 0x7F )
                {                      
                    if( $c eq "\"" )
                    {  
                        print "\\\"";
                    }
                    elsif( $c eq "\\" )
                    {  
                        print "\\\\";
                    }  
                    else
                    {
                        print $c;
                    }
                }
                else
                {
                    if(  $c eq "\r" )
                    {
                        #print "\\r";
                    }
                    elsif( $c eq "\n" )
                    {  
                        print "\\n\"\n    \"";  
                        $at_begin_of_line = 1;
                    }
                    else
                    {
                        print "\\x" . substr( $hex, $o / 16, 1 ) . substr( $hex, $o % 16, 1 ) . "\" \"";
                    }
                }
            }
        }
    }

    print "\";\n\n";
    close FILE;
}



print "// \$" . "Id\$\n";
print "// ACHTUNG: NICHT AENDERN! DIESE DATEI IST GENERIERT VON $script_name\n";
print "\n";
print "\n";
print "\n";
print "#include \"../zschimmer/zschimmer.h\"\n";
print "#include \"../zschimmer/embedded_files.h\"\n";
print "\n";

my $rest = "";
$rest .= "\n";
$rest .= "using namespace zschimmer;\n";
$rest .= "\n";
$rest .= "namespace sos {\n";
$rest .= "namespace scheduler {\n";
$rest .= "\n";
$rest .= "static const Embedded_file embedded_files_array[] = \n";
$rest .= "{\n";

foreach my $arg ( @ARGV )
{
    if( $arg eq "-trim" )
    {
        $trim = 1;
    }
    elsif( $arg eq "-soft-trim" )
    {
        $trim = 1;
        $soft_trim = 1;
    }
    elsif( $arg eq "-gzip" )
    {
        $gzip = 1;
    }
    elsif( $arg =~ /^-name=(.*)$/ )
    {
        $name = $1;
    }
    else
    {
        foreach my $filename ( glob( $arg ) )
        {
            $filename =~ s/\\/\//g;         # '\' => '/'
            my $name = $filename;
            $name =~ s/[^a-zA-Z0-9_]/_/g;
            $name = "file_$name";
        
            file_to_cxx( $filename, $name );
            
            my $modified_time = 0; #stat( $filename )->mtime;
            $rest .= "    { \"$filename\", $name, sizeof $name - 1, $modified_time },\n"; 
        }
    }
}                       

$rest .= "    { NULL, NULL, 0 }\n";
$rest .= "};\n";
$rest .= "\n";
$rest .= "extern const Embedded_files $name = { embedded_files_array };\n";
$rest .= "\n";
$rest .= "} //namespace scheduler\n";
$rest .= "} //namespace sos\n";

print $rest;
