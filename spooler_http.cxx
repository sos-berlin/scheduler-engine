// $Id: spooler_http.cxx,v 1.1 2004/07/18 15:38:02 jz Exp $
/*
    Hier sind implementiert

    Xml_end_finder
    Communication::Channer
    Communication
*/


#include "spooler.h"


using namespace std;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------Http_parser::Http_parser
    
Http_parser::Http_parser( Http_request* http_request )
:
    _zero_(this+1),
    _http_request( http_request )
{
    _text.reserve( 1000 );
}

//----------------------------------------------------------------------------Http_parser::add_text
    
void Http_parser::add_text( const char* text, int len )
{
    _text.append( text, len );

    if( !_reading_body )
    {
        int end_size = 3;
        int header_end = _text.find( "\n\r\n" );
        if( header_end == string::npos )  header_end = _text.find( "\n\n" ), end_size = 2;

        if( header_end != string::npos )
        {
            _body_start = header_end + end_size;
            _reading_body = true;

            parse_header();
            string content_length = _http_request->_header[ "content-length" ];
            if( !content_length.empty() )
            {
                _content_length = as_uint( content_length );
            }
        }
    }

    if( _reading_body  &&  _text.length() >= _body_start + _content_length )
    {
        if( _text.length() > _body_start + _content_length  )  throw_xc( "SPOOLER-HTTP too much data" );
        _http_request->_body.assign( _text.data() + _body_start, _content_length ); 
    }
}

//-------------------------------------------------------------------------Http_parser::is_complete

bool Http_parser::is_complete()
{
    return _text.length() == _body_start + _content_length;
}

//------------------------------------------------------------------------Http_parser::parse_header

void Http_parser::parse_header()
{
    _next_char = _text.c_str();

    _http_request->_http_cmd = eat_word();
    _http_request->_path     = eat_path();
    _http_request->_protocol = eat_word();
                              eat_line_end();


    while( next_char() > ' ' )
    {
        string name = eat_until( ":" );
                      eat( ":" );
        string value = eat_until( "" );
        _http_request->_header[ lcase( name ) ] = value;
        eat_line_end();
    }

    eat_line_end();
}

//--------------------------------------------------------------------------Http_parser::eat_spaces

void Http_parser::eat_spaces()
{ 
    while( *_next_char == ' ' )  _next_char++; 
}

//---------------------------------------------------------------------------------Http_parser::eat

void Http_parser::eat( const char* what )
{
    const char* w = what;
    while( *w  &&  *_next_char == *w )  w++, _next_char++;
    if( *w != '\0' )  
    {
        if( what[0] == '\n' )  throw_xc( "SCHEDULER-213", "Zeilenende" );
                         else  throw_xc( "SCHEDULER-213", what );
    }

    eat_spaces();
}

//------------------------------------------------------------------------Http_parser::eat_line_end

void Http_parser::eat_line_end()
{
    eat_spaces();

    if( *_next_char == '\r' )  _next_char++;
    eat( "\n" );
}

//----------------------------------------------------------------------------Http_parser::eat_word

string Http_parser::eat_word()
{
    string word;
    while( *_next_char > ' ' )  word += *_next_char++;

    eat_spaces();
    return word;
}

//---------------------------------------------------------------------------Http_parser::eat_until

string Http_parser::eat_until( const char* character_set )
{
    string word;
    while( *_next_char >= ' '  &&  strchr( character_set, *_next_char ) == NULL )  word += *_next_char++;

    eat_spaces();
    return rtrim( word );
}

//----------------------------------------------------------------------------Http_parser::eat_path

string Http_parser::eat_path()
{
    eat_spaces();

    string path;

    while( (Byte)_next_char[0] > (Byte)' ' )
    {
        if( _next_char[0] == '%'  &&  _next_char[1] != '\0'  &&  _next_char[2] != '\0' )
        {
            path += (char)hex_as_int32( string( _next_char+1, 2 ) );
            _next_char += 3;
        }
        else
            path += *_next_char++;
    }

    eat_spaces();
    return path;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

