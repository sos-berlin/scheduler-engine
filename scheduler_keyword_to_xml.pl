#! /usr/bin/perl -W
# $Id: scheduler_keyword_to_xml.pl,v 1.2 2004/09/11 18:31:06 jz Exp $


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


foreach my $keyword ( sort keys %keyword_references )
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
    my $filename = shift;
    my $file_title = "";
    
    open( FILE, "<$filename" )  or die "$filename: $!";
    
    while( <FILE> )
    {
        if( !$file_title  &&  / title *= *"(.*)"/ )  { $file_title = $1; }
        
        if( /\<scheduler_keyword +keyword=("([^"]+)"|'([^']+)')/ )
        {
            my $keyword = $2? $2 : $3;
            my $xml_line = "<register_entry register_file='$filename' register_title='$file_title'  register_keyword='$keyword'/>\n";
            push( @{$keyword_references{$keyword}}, $xml_line );  # s. Perl Cookbook Seite 140 (5.7)
        }

        if( my $element = get_element( "scheduler_ini_entry" ) )
        {
            my $a_file    = get_attribute( $element, "file" );
            my $a_section = get_attribute( $element, "section" );
            my $a_entry   = get_attribute( $element, "entry" );
            my $xml_line  = "<register_ini_entry register_file='$filename' register_title='$file_title' file='$a_file' section='$a_section' entry='$a_entry'/>\n";
            my $keyword   = $a_entry;
            push( @{$keyword_references{$keyword}}, $xml_line );  # s. Perl Cookbook Seite 140 (5.7)
        }

        if( my $element = get_element( "scheduler_option" ) )
        {
            my $a_name    = get_attribute( $element, "name" );
            my $xml_line  = "<register_option register_file='$filename' register_title='$file_title' name='$a_name'/>\n";
            my $keyword   = $a_name;
            push( @{$keyword_references{$keyword}}, $xml_line );  # s. Perl Cookbook Seite 140 (5.7)
        }

        if( my $element = get_element( "scheduler_element" ) )
        {
            my $a_name    = get_attribute( $element, "name" );
            #my $a_directory = get_attribute( $element, "directory" );
            my $xml_line  = "<register_element register_file='$filename' register_title='$file_title' name='$a_name'/>\n";  # directory='$directory
            my $keyword   = $a_name;
            push( @{$keyword_references{$keyword}}, $xml_line );  # s. Perl Cookbook Seite 140 (5.7)
        }
    }
    
    close( FILE );
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