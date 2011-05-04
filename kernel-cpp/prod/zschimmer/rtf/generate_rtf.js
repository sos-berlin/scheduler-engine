// $Id$  Joacim Zschimmer 2004

var hw = new ActiveXObject( "hostware.global" );

var rtf_code_h   = hw.open( "-out -binary rtf_code.h" );
var rtf_code_cxx = hw.open( "-out -binary rtf_code.cxx" );

var line = "// $Id$\n" + "// Generiert von generate_rtf.js aus rtf.xml am " + new Date() + "\n";
rtf_code_h.put_line( line );
rtf_code_cxx.put_line( line );

var control_words = new Array();


var dom_document = new ActiveXObject( "MSXML2.DOMDocument" );
var ok = dom_document.load( "rtf.xml" );
if( !ok )  throw new Error( "XML-Dokument ist fehlerhaft:  " + dom_document.parseError.reason );


var cw = dom_document.selectNodes( "rtf/control_words/control_word" );
for( var i = 0; i < cw.length; i++ )  control_words.push( cw[ i ] );

control_words = control_words.sort( rtf_compare );


var first_non_property;

for( var i = 0; i < control_words.length; i++ )
{
    var control_word = control_words[ i ];
    if( !first_non_property  &&  !is_property(control_word) )  first_non_property=i;
}



rtf_code_h.put_line( "    // Eigenschaften\n" );
for( var i = 0; i < control_words.length; i++ )
{
    var control_word = control_words[ i ];

    if( i == first_non_property )
    {
        rtf_code_h.put_line( "    code__properties_max = " + control_words[i-1].getAttribute( "cxx_name" ) + ",\n" );
        rtf_code_h.put_line( "\n" );
        rtf_code_h.put_line( "    // Nicht-Eigenschaften\n" );
    }
    
    rtf_code_h.put_line( "    " + control_word.getAttribute( "cxx_name" ) + ",\n" );
    
    if( i == 0 )  rtf_code_h.put_line( "    code__properties_min = " + control_word.getAttribute( "cxx_name" ) + ",\n" );
}


rtf_code_cxx.put_line( "\n" );
rtf_code_cxx.put_line( "\n" );
rtf_code_cxx.put_line( "    // Eigenschaften\n" );

for( var i = 0; i < control_words.length; i++ )
{
    var control_word = control_words[ i ];
        
    if( i == first_non_property )
    {
        rtf_code_cxx.put_line( "\n" );
        rtf_code_cxx.put_line( "    // Nicht-Eigenschaften\n" );
    }
    
    var line = "    { ";
    line += span( control_word.getAttribute( "cxx_name" ), 23 );
    line += "," + span( control_word.getAttribute( "word_version" )? control_word.getAttribute( "word_version" ) : 0, 2 );
    line += "," + span( "\"" + control_word.getAttribute( "name" ).replace( /\\/, "\\\\" ) + "\"", 20 );
    line += ",kind_" + span( control_word.getAttribute( "type" ), 18 );
    
    var flags = "flag_none";
    if( control_word.getAttribute( "prop_char"     ) )  flags += "|flag_prop_char";
    if( control_word.getAttribute( "prop_para"     ) )  flags += "|flag_prop_para";
    if( control_word.getAttribute( "prop_sect"     ) )  flags += "|flag_prop_sect";
    if( control_word.getAttribute( "prop_doc"      ) )  flags += "|flag_prop_doc";
    if( control_word.getAttribute( "prop_row"      ) )  flags += "|flag_prop_row";
    if( control_word.getAttribute( "prop_tab"      ) )  flags += "|flag_prop_tab";
    if( control_word.getAttribute( "prop_cell"     ) )  flags += "|flag_prop_cell";
    if( control_word.getAttribute( "prop_dest"     ) )  flags += "|flag_prop_dest";
    if( control_word.getAttribute( "prop_border"   ) )  flags += "|flag_prop_border";
    if( control_word.getAttribute( "header"        ) )  flags += "|flag_header";
    if( control_word.getAttribute( "special_char"  ) )  flags += "|flag_special_char";
    if( control_word.getAttribute( "has_text"      ) )  flags += "|flag_has_text";
    if( control_word.getAttribute( "own_prop"      ) )  flags += "|flag_own_prop";
    if( control_word.getAttribute( "star"          ) )  flags += "|flag_star";
    if( control_word.getAttribute( "print_newline" ) )  flags += "|flag_print_newline";
    
    flags = flags.replace( /^flag_none\|/, "" );
    line += "," + span( flags, 70 );        
    
    line += "," + span( control_word.getAttribute( "default" )? control_word.getAttribute( "default" ) : 0, 13 );
    line += "},"
    
    line += "    // " + ( control_word.getAttribute( "section" )? control_word.getAttribute( "section" ) : "" );
    //if( control_word.neu )  line += "  // neu";
    
    rtf_code_cxx.put_line( line + "\n" );
}


rtf_code_h.close();
rtf_code_cxx.close();



function rtf_compare( a, b )
{
    var a_is_property = is_property( a );
    var b_is_property = is_property( b );

    if( a_is_property > b_is_property )  return -1;
    if( a_is_property < b_is_property )  return +1;


    var a_is_extra = ! a.getAttribute( "name" ).match( /^[a-zA-Z]/ );
    var b_is_extra = ! b.getAttribute( "name" ).match( /^[a-zA-Z]/ );
    
    if( a_is_extra > b_is_extra )  return -1;
    if( a_is_extra < b_is_extra )  return +1;
    
    var a_name = a.getAttribute( "name" ).replace( /^\*\\/, "" );         // "*\\cs" --> "cs"
    var b_name = b.getAttribute( "name" ).replace( /^\*\\/, "" );

    if( a_name < b_name ) return -1;
    if( a_name > b_name ) return +1;
    
    return 0;
}




function is_property( control_word )
{
    return control_word.getAttribute( "prop_char" ) == "true"
        || control_word.getAttribute( "prop_para" ) == "true"
        || control_word.getAttribute( "prop_sect" ) == "true"
        || control_word.getAttribute( "prop_doc" ) == "true"
        || control_word.getAttribute( "prop_row" ) == "true"
        || control_word.getAttribute( "prop_tab" ) == "true"
        || control_word.getAttribute( "prop_cell" ) == "true"
        || control_word.getAttribute( "prop_dest" ) == "true"
        || control_word.getAttribute( "prop_border" ) == "true";
}


function span( text, width )
{
    var blanks = "                                                                                      ";
    var text = "" + text;
    return text + blanks.substr( 0, width - text.length );
}
