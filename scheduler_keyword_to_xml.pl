#! /usr/bin/perl -W
# $Id: scheduler_keyword_to_xml.pl,v 1.4 2004/09/13 17:28:02 jz Exp $


my $script_name     = "scheduler_keyword_to_xml.pl";
my $output_filename = "register.xml";
my $output_path     = "../../spooler/doc/$output_filename";


my ( $year, $month, $mday, $hour, $min, $sec ) = (gmtime)[ 5, 4, 3, 2, 1, 0 ];  $year += 1900;  $month++;


open( OUTPUT, ">$output_filename" )  or die "$output_filename: $!";

print OUTPUT '<?xml version="1.0" encoding="utf-8"?>' ."\n";
print OUTPUT '<?xml-stylesheet href="scheduler.xslt" type="text/xsl"?>' ."\n";
print OUTPUT "\n";
print OUTPUT "<!-- ACHTUNG: NICHT AENDERN! DIESE DATEI IST GENENIERT VON $script_name -->\n";
print OUTPUT "\n";
print OUTPUT "<register\n";
print OUTPUT '    author="$' . "Author: $script_name " . '$"' . "\n";
print OUTPUT '    date= "$' . 'Date: ' . "$year/$month/$mday $hour:$min:$sec" . ' $"' . "\n";
print OUTPUT ">\n";

our %keyword_references = ();

while( my $filename = shift )
{
    read_file( $filename )  unless $filename =~ /\/$output_filename$/;
}


my @keys = sort { lc($a) cmp lc($b) } keys %keyword_references;
foreach my $keyword ( @keys )
{
    print OUTPUT "<register_keyword keyword='$keyword'>\n";
    my $last_xml_line = "";
    
    foreach my $xml_line ( sort @{$keyword_references{$keyword}} )
    {
        print OUTPUT "    $xml_line"  unless $xml_line eq $last_xml_line;
        $last_xml_line = $xml_line;
    }
    
    #print OUTPUT @{$keyword_references{$keyword}};
    print OUTPUT "</register_keyword>\n";
}


print OUTPUT "</register>\n";



sub read_file
{
    my $filename     = shift;
    my $file_title   = "XXX";
    my $head_title   = "";
    my $root_element = "";
    my $name         = "";
    
    open( FILE, "<$filename" )  or die "$filename: $!";

    
    while( <FILE> )
    {
        if( $root_element eq ""     &&  /\<([^?! \/\>]+)/ )         { $root_element = $1; }
        if( $name         eq ""     &&  / name *= *"(.*)"/ )        { $name = $1; }
        if( $file_title   eq "XXX"  &&  / title *= *"(.*)"/ )       { $file_title = $1; }
        if( $head_title   eq ""     &&  / head_title *= *"(.*)"/ )  { $head_title = $1; }
        if( $root_element  &&  /[^?-]>/ )  { last; }
    }

    if( $file_title eq "XXX"  &&  $head_title )  { $file_title = $head_title; }

    if( $root_element eq "xml_element" )
    { 
        $file_title = "&lt;$name&gt;";

        my $keyword = $name;
        my $xml_line = "<register_entry register_file='$filename' register_title='$file_title'  register_keyword='$keyword' type='definition'/>\n";
        add_keyword_reference( $keyword, $xml_line );
    }
    #if( $root_element eq "xml_element" )  { $file_title = "XML-Element $file_title" }
        
    
    while( <FILE> )
    {
        if( /\<scheduler_keyword +keyword=("([^"]+)"|'([^']+)')/ )
        {
            my $keyword = $2? $2 : $3;
            my $xml_line = "<register_entry register_file='$filename' register_title='$file_title'  register_keyword='$keyword'/>\n";
            add_keyword_reference( $keyword, $xml_line );
        }

        if( my $element = get_element( "scheduler_ini_entry" ) )
        {
            my $a_file    = get_attribute( $element, "file" );
            my $a_section = get_attribute( $element, "section" );
            my $a_entry   = get_attribute( $element, "entry" );
            my $xml_line  = "<register_ini_entry register_file='$filename' register_title='$file_title' file='$a_file' section='$a_section' entry='$a_entry'/>\n";
            add_keyword_reference( $a_entry, $xml_line );
        }

        if( my $element = get_element( "scheduler_option" ) )
        {
            my $a_name    = get_attribute( $element, "name" );
            my $xml_line  = "<register_option register_file='$filename' register_title='$file_title' name='$a_name'/>\n";
            add_keyword_reference( $a_name, $xml_line );
        }

        if( my $element = get_element( "scheduler_element" ) )
        {
            my $a_name    = get_attribute( $element, "name" );
            #my $a_directory = get_attribute( $element, "directory" );
            my $xml_line  = "<register_element register_file='$filename' register_title='$file_title' name='$a_name'/>\n";  # directory='$directory
            add_keyword_reference( $a_name, $xml_line );
        }
    }
    
    close( FILE );
}


sub add_keyword_reference
{
    my $keyword  = shift;
    my $xml_line = shift;
    
    push( @{$keyword_references{$keyword}}, $xml_line );  # s. Perl Cookbook Seite 140 (5.7)
}


sub get_element
{
    my $element_name = shift;
    
    return /\<$element_name[^>]*\>/? $& : "";
}


sub get_attribute
{
    my $element = shift;
    my $attribute_name = shift;
    
    if( $element =~ / $attribute_name *= *("([^"]*)"|'([^']*)')/ )
    {
        return $2? $2 : $3;
    }
    
    return "$element";
}
