// $Id$

#include "zschimmer.h"
#include "xml.h"
#include "z_io.h"
#include "xml_libxml2.h"


namespace zschimmer {
namespace xml {

//-------------------------------------------------------------------------------------------------

const char allowed_xml_ascii_characters[ 256 ] =
{
    // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f   
       0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,  // 00   \t, \n, \r  (\r von XML nur geduldet, aber nicht gelesen)
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 10
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 20
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 30
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 40
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 50
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 60
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  // 70
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 80
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 90
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // a0
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // b0
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // c0
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // d0
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // e0
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // f0
};

const char allowed_xml_latin1_characters[ 256 ] =
{
    // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f   
       0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,  // 00   \t, \n, \r  (\r von XML nur geduldet, aber nicht gelesen)
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 10
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 20
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 30
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 40
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 50
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 60
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  // 70
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 80
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 90
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // a0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // b0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // c0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // d0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // e0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // f0
};

const char unicode_substition_character = '¿';

//------------------------------------------------------------non_xml_latin1_characters_substituted

string non_xml_latin1_characters_substituted( const io::Char_sequence& seq )
{
    string result;

    result.resize( seq.length() );

    for( size_t i = 0; i < seq.length(); i++ )
    {
        char c = seq[ i ];
        result[ i ] = is_valid_xml_latin1_character( c )? c : unicode_substition_character;
    }

    return result;
}

//--------------------------------------------------------------------------------------encode_text
    
string encode_text( const string& value, bool quote )
{
    // Für UTF-8 umstellen!

    string result;
    int reserve = value.length() + 10 + value.length() / 10;

    int last = -1;

    for( uint i = 0; i < value.length(); i++ )
    {
        switch( value[i] )
        {
            case '<': 
                if( result.length() == 0 )  result.reserve( reserve );
                result.append( value, last+1, i - (last+1) );
                result += "&lt;";
                last = i;
                break;

            case '>': 
                if( result.length() == 0  
                 || result.length() >= 1  &&  result[ result.length() - 1 ] == '-'  
                 || result.length() >= 2  &&  result[ result.length() - 2 ] == '-' )
                {
                    if( result.length() == 0 )  result.reserve( reserve );
                    result.append( value, last+1, i - (last+1) );
                    result.append( "&gt;" );
                    last = i;
                }
                break;

            case '&':
                if( result.length() == 0 )  result.reserve( reserve );
                result.append( value, last+1, i - (last+1) );
                result.append( "&amp;" );
                last = i;
                break;

            case '"':
                if( quote )
                {
                    if( result.length() == 0 )  result.reserve( reserve );
                    result.append( value, last+1, i - (last+1) );
                    result.append( "&quote;" );
                    last = i;
                }
                break;

            default: ;
        }
    }

    result.append( value, last+1, value.length() - (last+1) );

    return result;
}

//---------------------------------------------------------------------------Xml_writer::Xml_writer

Xml_writer::Xml_writer()
: 
    _is_tag_open(false)
{
}

//---------------------------------------------------------------------------Xml_writer::Xml_writer

Xml_writer::Xml_writer( Writer* writer )
: 
    Filter_writer(writer),
    _is_tag_open(false)
{
    assert( writer );

    set_encoding( xml::libxml2::default_character_encoding );
}

//-------------------------------------------------------------------------Xml_writer::write_prolog
    
void Xml_writer::write_prolog()
{
    write_through( "<?xml version=\"1.0\"" );
    if( _encoding != "" )  set_attribute( "encoding", _encoding );
    write_through( "?>\n" );
}

//----------------------------------------------------------------------------Xml_writer::close_tag
    
void Xml_writer::close_tag()
{
    if( _is_tag_open )
    {
        _writer->write( '>' );
        _is_tag_open = false;
    }
}

//------------------------------------------------------------------------Xml_writer::begin_element

void Xml_writer::begin_element( const string& element_name )
{
    if( _is_tag_open )  close_tag();

    _writer->write( '<' );
    write_through( element_name );
    _is_tag_open = true;
}

//------------------------------------------------------------------------Xml_writer::set_attribute

void Xml_writer::set_attribute( const string& name, int64 value )
{
    set_attribute( name, as_string( value ) );
}

//------------------------------------------------------------------------Xml_writer::set_attribute

void Xml_writer::set_attribute( const string& name, const string& value )
{
    _writer->write( ' ' );
    write_through( name );
    _writer->write( '=' );
    _writer->write( '"' );
    
    for( const char* p = value.c_str(); *p; p++ )
    {
        if( *p == '<' )  _writer->write( "&lt;" );
        else
        if( *p == '>' )  _writer->write( "&gt;" );
        else
        if( *p == '&' )  _writer->write( "&amp;" );
        else
        if( *p == '"' )  _writer->write( "&quot;" );
        else
            write_through( io::Char_sequence( p, 1 ) );
    }

    _writer->write( '"' );
}

//---------------------------------------------------------------Xml_writer::set_attribute_optional

void Xml_writer::set_attribute_optional( const string& name, const string& value )
{
    if( value != "" )  set_attribute( name, value );
}

//--------------------------------------------------------------------------Xml_writer::end_element

void Xml_writer::end_element( const string& element_name )
{
    if( _is_tag_open )  
    {
        _writer->write( '/' );
        _writer->write( '>' );
        _is_tag_open = false;
    }
    else
    {
        _writer->write( '<' );
        _writer->write( '/' );
        _writer->write( element_name );
        _writer->write( '>' );
    }
}

//--------------------------------------------------------------------------------Xml_writer::write

void Xml_writer::write( const io::Char_sequence& seq )
{
    if( seq.length() > 0 )
    {
        if( _is_tag_open )  close_tag();

        const char* p_end = seq.ptr() + seq.length();

        for( const char* p = seq.ptr(); p < p_end; p++ )
        {
            if( *p == '<' )  _writer->write( "&lt;" );
            else
            if( *p == '>' )  _writer->write( "&gt;" );
            else
            if( *p == '&' )  _writer->write( "&amp;" );
            else
            {
                const char* p0 = p;
                
                while( p < p_end  &&  *p != '<'  &&  *p != '&' )
                {
                    if( p[0] == '>' )
                    {
                        if( p < p0+1  ||  p[-1] == ']' )  break;    // ]>
                        if( p < p0+2  ||  p[-2] == ']' )  break;    // ]]>
                    }

                    p++;
                }

                write_through( io::Char_sequence( p0, p - p0 ) );
            }
        }
    }
}

//------------------------------------------------------------------------Xml_writer::write_through

void Xml_writer::write_through( const io::Char_sequence& seq )
{
    //Erstmal nicht:  if( !is_valid_latin1( seq ) )  z::throw_xc( "XML-NO-LATIN1", seq );
    _writer->write( seq );
}

//------------------------------------------------------------------------Xml_writer::write_element

void Xml_writer::write_element( const libxml2::Element_ptr& element )
{
    if( !element )  z::throw_xc( Z_FUNCTION );

    close_tag();
    write_through( element.xml_without_prolog( libxml2::default_character_encoding ) );
}

//--------------------------------------------------------------------------------Xml_writer::flush

void Xml_writer::flush()
{
}

//--------------------------------------------------------------ml_string_writer::Xml_string_writer

Xml_string_writer::Xml_string_writer()
: 
    Xml_writer(), 
    _string_writer( Z_NEW( io::String_writer() ) )  
{ 
    _writer = _string_writer; 
}

//-------------------------------------------------------------------------Xml_string_writer::flush
    
void Xml_string_writer::flush()
{ 
    Xml_writer::flush(); 
    if( _writer )  _writer->flush(); 
}

//---------------------------------------------------------------------Xml_string_writer::to_string

string Xml_string_writer::to_string()
{ 
    flush();
    return _string_writer->to_string(); 
}

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace zschimmer
