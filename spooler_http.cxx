// $Id: spooler_http.cxx,v 1.7 2004/07/26 08:24:40 jz Exp $
/*
    Hier sind implementiert

    Xml_end_finder
    Communication::Channer
    Communication
*/


#include "spooler.h"
#include "spooler_version.h"

using namespace std;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------Http_request::parameter
    
string Http_request::parameter( const string& name ) const
{ 
    map<string,string>::const_iterator it = _parameters.find( name );
    return it == _parameters.end()? "" : it->second;
}

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

    string word;
    string path;
    string parameter_name;
    enum State { in_path, in_parameter }  state = in_path;

    while(1)
    {
        if( _next_char[0] == '&'  ||  (Byte)_next_char[0] <= (Byte)' ' )
        {
            if( state == in_path )  path = word;
                              else  _http_request->_parameters[ parameter_name ] = word;
            state = in_parameter;
            if( (Byte)_next_char[0] <= (Byte)' ' )  break;
            word = "";
            parameter_name = "";
            _next_char++;
        }
        else
        if( _next_char[0] == '=' && state == in_parameter )
        {
            parameter_name = word;
            word = "";
            _next_char++;
        }
        else
        if( _next_char[0] == '%'  &&  _next_char[1] != '\0'  &&  _next_char[2] != '\0' )
        {
            word += (char)hex_as_int32( string( _next_char+1, 2 ) );
            _next_char += 3;
        }
        else
            word += *_next_char++;
    }

    eat_spaces();
    return path;
}

//----------------------------------------------------------------------------Http_response::finish

void Http_response::finish()
{
    time_t      t;
    char        time_text[26];

    ::time( &t );
    memset( time_text, 0, sizeof time_text );

#   ifdef Z_WINDOWS
        strcpy( time_text, asctime( gmtime( &t ) ) );
#    else
        struct tm  tm;
        asctime_r( gmtime_r( &t, &tm ), time_text );
#   endif
    
    time_text[24] = '\0';

    _header = "HTTP/1.1 200 OK\r\n"
              "Content-Type: "  + _content_type + "\r\n"
              "Transfer-Encoding: chunked\r\n"
              "Date: " + string(time_text) + " GMT\r\n"
              "Server: Scheduler " + string(VER_PRODUCTVERSION_STR) + "\r\n"
              "Cache-Control: no-cache\r\n"
              "\r\n";

    _chunk_size = _header.length();
}

//-------------------------------------------------------------------------------Http_response::eof

bool Http_response::eof()
{
    return _eof;
}

//------------------------------------------------------------------------------Http_response::read

string Http_response::read( int recommended_size )                           
{
    if( _chunk_index == 0 )
    {
        if( _chunk_offset < _chunk_size )
        {
            //uint length = min( recommended_size, _header.length() - _chunk_offset );
            //uint r      = _chunk_offset;
            //_chunk_offset += length;
            //return _header.substr( r, length );
            _chunk_offset = _chunk_size;
            return _header;
        }

        return start_new_chunk();
    }
    else
    {
        string result;

        if( _chunk_offset < _chunk_size )
        {
            result = read_chunk( min( recommended_size, (int)( _chunk_size - _chunk_offset ) ) );
            _chunk_offset += result.length();
        }

        if( _chunk_offset == _chunk_size  &&  !_chunk_eof ) 
        {
            _chunk_eof = true;
            result.append( "\r\n" );
        }

        if( _chunk_offset == _chunk_size ) 
        {
            result.append( start_new_chunk() );
        }

        return result;
    }
}

//-------------------------------------------------------------------Http_response::start_new_chunk

string Http_response::start_new_chunk()
{
    if( !next_chunk_is_ready() )  return "";

    _chunk_index++;
    _chunk_offset = 0;
    _chunk_size   = get_next_chunk_size();

    string result = as_hex_string( (int)_chunk_size ) + "\r\n";
    
    if( _chunk_size > 0 )
    {
        _chunk_eof = false;
    }
    else
    {
        _eof = true;
        result += "\r\n";    
    }
    
    return result;
}

//--------------------------------------------------------tring_http_response::String_http_response

String_http_response::String_http_response( const string& text, string content_type )
: 
    _zero_(this+1), 
    _text(text) 
{ 
    set_content_type( content_type ); 
    finish();
}

//--------------------------------------------------------String_http_response::get_next_chunk_size

uint String_http_response::get_next_chunk_size()
{
    return _chunk_index == 1? _text.length() : 0;
}

//-----------------------------------------------------------------------String_http_response::read

string String_http_response::read_chunk( int recommended_size )
{ 
    int length = min( recommended_size, (int)_text.length() );
    return _text.substr( _chunk_offset, length ); 
}

//-------------------------------------------------------------Log_http_response::Log_http_response

Log_http_response::Log_http_response( Prefix_log* log, string content_type )
: 
    _zero_(this+1), 
    _log(log) 
{
    set_content_type( content_type );
    finish();

    _html_prefix = "<html>\n" 
                        "<head>\n" 
                            "<style type='text/css'>\n"
                                "@import 'scheduler.css';\n"
                                "pre { font-family: Lucida Console, monospace; font-size: 10pt }\n"
                            "</style>\n"
                            "<title>Scheduler log</title>\n"
                        "</head>\n" 
                        "<body>\n" 

                            "<script type='text/javascript'><!--\n"   
                                "var title=" + quoted_string( _log->title() ) + ";\n"
                            "--></script>\n"

                            "<script type='text/javascript' src='show_log.js'></script>\n"
/*
                            // Wirkt nicht. Wenn der Scheduler abbricht (abort_immediately), löscht ie6 das Fenster 
                            // und zeigt stattdessen eine unsinnige Fehlermeldung.
                            "<script type='text/javascript' for='window' event='onerror'><!--\n"   
                                //"document.write( '<br/><br/>(load error)' );\n"
                                "return true;\n"
                            "--></script>\n"
*/
                            // onsize wirkt auch nicht. Soll die jeweils letzten Zeilen zeigen.
                            //"<pre class='log' onresize='alert(1);event.srcElement.scrollBy(0,999999999)'>\n";
                            "<pre class='log'>\n";

    _html_suffix =          "</pre>\n"
                        "</body>\n"
                    "</html>\n";
}

//------------------------------------------------------------Log_http_response::~Log_http_response

Log_http_response::~Log_http_response()
{
    if( _event )  _log->remove_event( _event );
}

//---------------------------------------------------------------------Log_http_response::set_event

void Log_http_response::set_event( Event_base* event )
{
    _event = event;
    _log->add_event( _event );
}

//-----------------------------------------------------------Log_http_response::next_chunk_is_ready

bool Log_http_response::next_chunk_is_ready()
{ 
    if( !_file.opened() )
    {
        _file.open( _log->filename(), "rb" );
    }

    if( _file.tell() == _file.length() )
    {
        if( _log->closed() )
        {
            _file_eof = true;
            return true;
        }
        else
            return false;
    }                                       

    return true;
}

//-----------------------------------------------------------Log_http_response::get_next_chunk_size

uint Log_http_response::get_next_chunk_size()
{
    if( _html_prefix.length() > 0 )  return _html_prefix.length();

    if( _file.opened()  &&  !_file_eof )
    {
        uint64 size = _file.length() - _file.tell();
        if( size > 0 )  return size < INT_MAX? (uint)size : INT_MAX;
    }

    if( _html_suffix.length() > 0 )  return _html_suffix.length();

    return 0;    // eof
}

//--------------------------------------------------------------------Log_http_response::read_chunk

string Log_http_response::read_chunk( int recommended_size )
{ 
    string result;

    if( _html_prefix != "" )
    {
        result = _html_prefix;
        _html_prefix = "";
    }
    else
    if( _file.opened()  &&  !_file_eof )
    {
        return _file.read_string( recommended_size );
    }
    else
    if( _html_suffix != "" )
    {
        result = _html_suffix;
        _html_suffix = "";
    }
    else
        throw_xc( __FUNCTION__ );

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
