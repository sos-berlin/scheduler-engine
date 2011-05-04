#! /usr/bin/perl -W
# $Id$


my $script_name     = "scheduler_keyword_to_xml.pl";
my $output_filename = "register_data.xml";
my $output_path     = "../../spooler/doc/$output_filename";


my ( $year, $month, $mday, $hour, $min, $sec ) = (gmtime)[ 5, 4, 3, 2, 1, 0 ];  $year += 1900;  $month++;


open( OUTPUT, ">$output_filename" )  or die "$output_filename: $!";

print OUTPUT '<?xml version="1.0" encoding="utf-8"?>' ."\n";
print OUTPUT '<?xml-stylesheet href="scheduler.xsl" type="text/xsl"?>' ."\n";
print OUTPUT "\n";
print OUTPUT "<!-- ACHTUNG: NICHT AENDERN! DIESE DATEI IST GENERIERT VON $script_name -->\n";
print OUTPUT "\n";
print OUTPUT "<register_data\n";
print OUTPUT '    author="$' . "Author: $script_name " . '$"' . "\n";
print OUTPUT '    date= "$' . 'Date: ' . "$year/$month/$mday $hour:$min:$sec" . ' $"' . "\n";
print OUTPUT ">\n";

our %keyword_references = ();

while( my $filename = shift )
{
    read_file( $filename )  unless $filename =~ /\/$output_filename$/;
}


my @keys = sort { lc($a) cmp lc($b) } keys %keyword_references;
foreach my $k ( @keys )
{
    $k =~ /^(.*)\t(.*)$/;
    my $keyword = $1;
    my $keyword_display = $2;
    print OUTPUT "<register_keyword keyword='$keyword'>\n";
    print OUTPUT "    <register_keyword_display>$keyword_display</register_keyword_display>\n"    if $keyword_display;
    
    my $last_xml_line = "";
    
    foreach my $xml_line ( sort @{$keyword_references{$k}} )
    {
        print OUTPUT "    $xml_line"  unless $xml_line eq $last_xml_line;
        $last_xml_line = $xml_line;
    }
    
    #print OUTPUT @{$keyword_references{$keyword}};
    print OUTPUT "</register_keyword>\n";
}


print OUTPUT "</register_data>\n";



sub read_file
{
    my $filename     = shift;
    my $file_title   = "XXX";
    my $file         = "";
    my $head_title   = "";
    my $root_element = "";
    my $name         = "";
    my $category     = "";
    my $last_method  = "";
    my $last_call    = "";
    
    open( FILE, "<$filename" )  or die "$filename: $!";

    
    while( <FILE> )
    {
        if( $root_element eq ""     &&  /<([^?! \n\/>]+)/ )         { $root_element = $1; }
        if( $name         eq ""     &&  / name *= *"(.*)"/ )        { $name = $1; }
        if( $category     eq ""     &&  / category *= *"(.*)"/ )    { $category = $1; }
        if( $file_title   eq "XXX"  &&  / title *= *"(.*)"/ )       { $file_title = $1; }
        if( $head_title   eq ""     &&  / head_title *= *"(.*)"/ )  { $head_title = $1; }
        if( $file         eq ""     &&  / file *= *"(.*)"/ )        { $file = $1; }
        if( $root_element  &&  /(^|[^?-])>/ )  { last; }
    }

    if( $file_title eq "XXX"  &&  $head_title )  { $file_title = $head_title; }

    if( $root_element eq "xml_element" )
    { 
        #if( $category )  { $file_title = $category; }
        #           else  { $file_title = "&lt;$name&gt;"; }
        $file_title = "&lt;$name&gt;";
        $file_title .= " ($category)"  if $category;

        my $keyword = $name;
        my $xml_line = "<register_entry register_file='$filename#' register_title='$file_title'  register_keyword='$keyword' type='definition'/>\n";
        add_keyword_reference( $keyword, "<code>&lt;$keyword&gt;</code>", $xml_line );
    }
    #if( $root_element eq "xml_element" )  { $file_title = "XML-Element $file_title" }

    if( $root_element eq "ini_section" )
    { 
        my $section = $name;
        $file_title = "file $file, section [$section]"  if !$file_title || $file_title eq "XXX";
        
        my $xml_line = "<register_entry register_file='$filename#' register_title='$file_title'  register_keyword='$section' type='definition'/>\n";
        add_keyword_reference( $section, "<code>[$section]</code>", $xml_line );
    }
    
    if( $root_element eq "api.class" )
    { 
        if( !( $filename =~ /-/ ) )   # Nur Task.xml, nicht Task-javascript.xml etc.
        {
            my $section = $name;
            $file_title = "$name"  if !$file_title || $file_title eq "XXX";
            
            my $xml_line = "<register_entry register_file='$filename' register_keyword='$name' type='definition'/>\n";
            add_keyword_reference( "$name (class)", "<code>$name</code> (class)", $xml_line );
        }
    }
    
    
        

    while( <FILE> )
    {
        if( /\<scheduler_keyword +keyword=("([^"]+)"|'([^']+)')/ )
        {
            my $keyword = $2? $2 : $3;
            my $xml_line = "<register_entry register_file='$filename' register_title='$file_title'  register_keyword='$keyword'/>\n";
            add_keyword_reference( $keyword, "", $xml_line );
        }

        if( my $element = get_element( "scheduler_ini_entry" ) )
        {
            my $a_file    = get_attribute( $element, "file" );
            my $a_section = get_attribute( $element, "section" );
            my $a_entry   = get_attribute( $element, "entry" );
            
            if( $a_entry )
            {
                my $xml_line  = "<register_ini_entry register_file='$filename' register_title='$file_title' file='$a_file' section='$a_section' entry='$a_entry'/>\n";
                add_keyword_reference( $a_entry, "<code>$a_entry=</code>", $xml_line );
            }
        }

        if( my $element = get_element( "scheduler_option" ) )
        {
            my $a_name    = get_attribute( $element, "name" );
            my $xml_line = "<register_entry register_file='$filename#use_option__$a_name' register_title='$file_title'  register_keyword='$a_name'/>\n";
            #my $xml_line  = "<register_option register_file='$filename' register_title='$file_title' name='$a_name'/>\n";
            add_keyword_reference( $a_name, "<code>-$a_name=</code>", $xml_line );
        }

        if( my $element = get_element( "scheduler_element" ) )
        {
            my $a_name    = get_attribute( $element, "name" );
            #my $a_directory = get_attribute( $element, "directory" );
            my $xml_line = "<register_entry register_file='$filename#use_element__$a_name' register_title='$file_title'  register_keyword='$a_name'/>\n";
            #my $xml_line  = "<register_element register_file='$filename' register_title='$file_title' name='$a_name'/>\n";  # directory='$directory
            add_keyword_reference( $a_name, "<code>&lt;$a_name&gt;</code>", $xml_line );
        }

        if( $root_element eq "command_line" )
        {
            if( my $element = get_element( "command_option" ) )
            { 
                my $a_name   = get_attribute( $element, "name" );
                my $xml_line = "<register_entry register_file='$filename#option_$a_name' register_title='$file_title'  register_keyword='$a_name' type='definition'/>\n";
                add_keyword_reference( $a_name, "<code>-$a_name=</code>", $xml_line );
            }
        }

        if( $root_element eq "ini_section" )
        {
            if( my $element = get_element( "ini_entry" ) )
            { 
                my $a_name   = get_attribute( $element, "name" );
                   $a_name   = get_attribute( $element, "setting" )  unless $a_name;
                my $xml_line = "<register_entry register_file='$filename#entry_$a_name' register_title='$file_title'  register_keyword='$a_name' type='definition'/>\n";
                add_keyword_reference( $a_name, "<code>$a_name=</code>", $xml_line );
            }
        }

        if( $root_element eq "xml_element" )
        {
            if( my $element = get_element( "xml_attribute" ) )
            { 
                my $a_name   = get_attribute( $element, "name" );
                   $a_name   = get_attribute( $element, "setting" )  if !$a_name;
                my $xml_line = "<register_entry register_file='$filename#attribute_$a_name' register_title='$file_title'  register_keyword='$a_name' type='definition'/>\n";
                add_keyword_reference( $a_name, "<code>$a_name=</code>", $xml_line );
            }
        }

        if( $root_element eq "api.class" )
        {
            if( my $element = get_element( "property" ) )
            { 
                my $a_name   = get_attribute( $element, "name" );
                $last_method = $a_name;
                $last_call   = "$name.$a_name";
                my $xml_line = "<register_entry register_file='$filename#method__$a_name' register_title='$name.$a_name' register_keyword='$a_name' type='definition'/>\n";
                add_keyword_reference( "$a_name (property)", "<code>$a_name</code> (property)", $xml_line );
            }
            
            if( my $element = get_element( "method" ) )
            { 
                my $a_name   = get_attribute( $element, "name" );
                $last_method = $a_name;
                $last_call   = "$name.$a_name()";
                my $xml_line = "<register_entry register_file='$filename#method__$a_name' register_title='$name.$a_name()' register_keyword='$a_name()' type='definition'/>\n";
                add_keyword_reference( $a_name, "<code>$a_name()</code>", $xml_line );
            }

            if( my $element = get_element( "com.parameter" ) )
            { 
                if( my $a_name = get_attribute( $element, "name" ) )
                {
                    my $xml_line = "<register_entry register_file='$filename#method__$last_method' register_title='$last_call' register_keyword='$a_name'/>\n";
                    add_keyword_reference( $a_name, "<code>$a_name</code>", $xml_line );
                }
            }

            if( my $element = get_element( "com.type" ) )
            { 
                if( my $a_class = get_attribute( $element, "class" ) )
                {
                    my $xml_line = "<register_entry register_file='$filename#method__$last_method' register_title='$last_call' register_keyword='$a_class'/>\n";
                    add_keyword_reference( "$a_class (class)", "<code>$a_class</code> (class)", $xml_line );
                }
            }
        }
    }
    
    close( FILE );
}


sub add_keyword_reference
{
    my $keyword         = shift;
    my $keyword_display = shift;
    my $xml_line        = shift;

    push( @{$keyword_references{"$keyword\t$keyword_display"}}, $xml_line );  # s. Perl Cookbook Seite 140 (5.7)
}


sub get_element
{
    my $element_name = shift;
    
    return /\<$element_name [^>]*\>/? $& : "";
}


sub get_attribute
{
    my $element = shift;
    my $attribute_name = shift;
    
    if( $element =~ / $attribute_name *= *("([^"]*)"|'([^']*)')/ )
    {
        return $2? $2 : $3;
    }
    
    return "";
}
