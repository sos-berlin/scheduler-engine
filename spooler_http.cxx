// $Id: spooler_http.cxx,v 1.13 2004/07/27 08:24:03 jz Exp $
/*
    Hier sind implementiert

    Http_parser
    Http_request
    Http_response
    Log_chunk_reader
    Html_chunk_reader
*/


#include "spooler.h"
#include "spooler_version.h"


using namespace std;

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------------chunk_size

const int chunk_size = 32768;   // Ungefähr, für Html_chunk_reader

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

//--------------------------------------------------------------------------Http_request::parameter
    
string Http_request::parameter( const string& name ) const
{ 
    map<string,string>::const_iterator it = _parameters.find( name );
    return it == _parameters.end()? "" : it->second;
}

//--------------------------------------------------------------------Http_request::keep_connection

bool Http_request::is_http_1_1() const
{
    return _protocol == "HTTP/1.1";
}

//---------------------------------------------------------------------Http_response::Http_response

Http_response::Http_response( const Http_request* http_request, Chunk_reader* chunk_reader, const string& content_type )
: 
    _zero_(this+1), 
    _chunk_reader( chunk_reader ),
    _chunked( http_request->is_http_1_1() ),
    _close_connection_at_eof( !http_request->is_http_1_1() )
{ 
    set_content_type(content_type); 
    finish();
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

    if( _chunked )  _header = "HTTP/1.1 200 OK\r\n";
              else  _header = "HTTP/1.0 200 OK\r\n";

    _header += "Content-Type: "  + _content_type + "\r\n"
               "Date: " + string(time_text) + " GMT\r\n"
               "Server: Scheduler " + string(VER_PRODUCTVERSION_STR) + "\r\n"
               "Cache-Control: no-cache\r\n";

    if( _chunked                 )  _header += "Transfer-Encoding: chunked\r\n";
    if( _close_connection_at_eof )  _header += "Connection: close\r\n";

    _header += "\r\n";

    _chunk_size = _header.length();
}

//-------------------------------------------------------------------------------Http_response::eof

bool Http_response::eof()
{
    return _eof;
}

//------------------------------------------------------------------------------Http_response::read

string Http_response::read( int _recommended_chunk_size )                           
{
    string result;

    if( _eof )  return result;

    if( _chunk_index == 0  &&  _chunk_offset < _chunk_size )
    {
        //uint length = min( _recommended_chunk_size, _header.length() - _chunk_offset );
        //uint r      = _chunk_offset;
        //_chunk_offset += length;
        //return _header.substr( r, length );
        _chunk_offset = _chunk_size;
        result = _header;
    }

    if( _chunk_offset == _chunk_size ) 
    {
        result += start_new_chunk( _recommended_chunk_size );
    }

    if( _chunk_offset < _chunk_size )
    {
        string data =_chunk_reader->read_chunk( min( _recommended_chunk_size, (int)( _chunk_size - _chunk_offset ) ) );
        _chunk_offset += data.length();
        result += data;
    }

    if( _chunk_offset == _chunk_size  &&  !_chunk_eof ) 
    {
        _chunk_eof = true;
        if( _chunked )  result.append( "\r\n" );
    }

    //Z_LOG( __FUNCTION__ << "() ==> " << result << "\n" );

    return result;
}

//-------------------------------------------------------------------Http_response::start_new_chunk

string Http_response::start_new_chunk()
{
    if( !_chunk_reader->next_chunk_is_ready() )  return "";

    _chunk_index++;
    _chunk_offset = 0;
    _chunk_size   = _chunk_reader->get_next_chunk_size();

    string result;
    
    if( _chunked )  result = as_hex_string( (int)_chunk_size ) + "\r\n";
    
    //Z_LOG( __FUNCTION__ << "  chunk_size=" << _chunk_size << "\n" );

    if( _chunk_size > 0 )
    {
        _chunk_eof = false;
    }
    else
    {
        _eof = true;
        if( _chunked )  result += "\r\n";    
    }
    
    return result;
}

//---------------------------------------------------------String_chunk_reader::get_next_chunk_size

int String_chunk_reader::get_next_chunk_size()
{
    if( _get_next_chunk_size_called )  return 0;
    _get_next_chunk_size_called = true;

    return _text.length();
}

//------------------------------------------------------------------------String_chunk_reader::read

string String_chunk_reader::read_chunk( int _recommended_chunk_size )
{ 
    int length = min( _recommended_chunk_size, (int)_text.length() );

    int offset = _offset;
    _offset += length;

    return _text.substr( offset, length ); 
}

//-------------------------------------------------------------Log_chunk_reader::Log_chunk_reader

Log_chunk_reader::Log_chunk_reader( Prefix_log* log )
: 
    _zero_(this+1), 
    _log(log) 
{
}

//------------------------------------------------------------Log_chunk_reader::~Log_chunk_reader

Log_chunk_reader::~Log_chunk_reader()
{
    if( _event )  _log->remove_event( _event );
}

//---------------------------------------------------------------------Log_chunk_reader::set_event

void Log_chunk_reader::set_event( Event_base* event )
{
    _event = event;
    _log->add_event( _event );
}

//-----------------------------------------------------------Log_chunk_reader::next_chunk_is_ready

bool Log_chunk_reader::next_chunk_is_ready()
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

//-----------------------------------------------------------Log_chunk_reader::get_next_chunk_size

int Log_chunk_reader::get_next_chunk_size( int _recommended_chunk_size )
{
    if( _file.opened()  &&  !_file_eof )
    {
        uint64 size = _file.length() - _file.tell();
        if( size > 0 )  return size < _recommended_chunk_size? (int)size : _recommended_chunk_size;
    }

    return 0;  // eof
}

//--------------------------------------------------------------------Log_chunk_reader::read_chunk

string Log_chunk_reader::read_chunk( int _recommended_chunk_size )
{ 
    return _file.read_string( _recommended_chunk_size );
}

//-------------------------------------------------------------Html_chunk_reader::Html_chunk_reader

Html_chunk_reader::Html_chunk_reader( Chunk_reader* chunk_reader, const string& title )
: 
    Chunk_reader_filter(chunk_reader),
    _zero_(this+1), 
    _state(reading_prefix),
    _awaiting_class(true)
{
    _html_prefix = "<html>\n" 
                        "<head>\n" 
                            "<style type='text/css'>\n"
                                "@import 'scheduler.css';\n"
                                "pre { font-family: Lucida Console, monospace; font-size: 10pt }\n"
                            "</style>\n"
                            "<title>Scheduler log</title>\n"
                        "</head>\n" 

                        "<body class='log'>\n" 

                            "<script type='text/javascript'><!--\n"   
                                "var title=" + quoted_string( title ) + ";\n"
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
                            "<pre class='log'>\n\n";

    _html_suffix =          "\n</pre>\n"
                        "</body>\n"
                    "</html>\n";
}

//------------------------------------------------------------Html_chunk_reader::~Html_chunk_reader

Html_chunk_reader::~Html_chunk_reader()
{
}

//-----------------------------------------------------------Html_chunk_reader::next_chunk_is_ready

bool Html_chunk_reader::next_chunk_is_ready()
{ 
    switch( _state )
    {
        case reading_prefix:  _chunk = _html_prefix;  return true;

        case reading_text:    if( !try_fill_chunk( chunk_size ) )    return false;
                              if( _chunk.length() > 0 )  return true;
                              _state = reading_suffix;

        case reading_suffix:  _chunk = _html_suffix;   return true;

        default:              return true;
    }
}

//-----------------------------------------------------------Html_chunk_reader::get_next_chunk_size

int Html_chunk_reader::get_next_chunk_size( int _recommended_chunk_size )
{
    return _chunk.length();
}

//----------------------------------------------------------------Html_chunk_reader::try_fill_chunk

bool Html_chunk_reader::try_fill_chunk( int _recommended_chunk_size )
{
    _chunk = "";

    while( _chunk.length() < _recommended_chunk_size )
    {
        if( _available_net_chunk_size == 0 )
        {
            if( !_chunk_reader->next_chunk_is_ready() )  return _chunk.length() > 0;

            _available_net_chunk_size = _chunk_reader->get_next_chunk_size( _recommended_chunk_size );
        }

        string text = _chunk_reader->read_chunk( _available_net_chunk_size );
        if( text == "" )  return _chunk.length() > 0;

        _available_net_chunk_size -= text.length();

        _chunk.reserve( text.length() * 2 );

        const char* text_data = text.data();

        for( int i = 0; i < text.length(); i++ )
        {
            int  begin = i;
            char c     = text_data[i];

            if( _awaiting_class )
            {
                while( i < text.length()  &&  c != '<'  &&  c != '>'  &&  c != '&'  &&  c != '\r'  &&  c != '\n' )
                {
                    if( c == ' '  &&  ++_blank_count == 4 )  break;
                    c = text_data[ ++i ];
                }
                
                _line_prefix.append( text_data + begin, i - begin );

                if( i == text.length() )  break;

                if( c == ' ' )
                {
                    _awaiting_class = false;

                    /* Stylesheet funktionier nicht wie gedacht. Also lassen wir es
                    {
                        string type = "scheduler";
                        
                        int left_parenthesis  = _line_prefix.find( '(' );
                        
                        if( left_parenthesis != string::npos  )
                        {
                            type = lcase( _line_prefix.substr( left_parenthesis + 1, _line_prefix.length() - left_parenthesis - 1 ) );
                        }

                        _chunk += "<span class='log_";
                        _chunk += type;
                        _chunk += "'>";

                        _in_span++;    // </span> nicht vergessen
                    }
                    */

                    {
                        int left_bracket  = _line_prefix.find( '[' );
                        int right_bracket = _line_prefix.find( ']' );
                        if( left_bracket != string::npos  &&  right_bracket != string::npos  &&  left_bracket < right_bracket )
                        {

                            _chunk += "<span class='log_";
                            _chunk += lcase( _line_prefix.substr( left_bracket + 1, right_bracket - left_bracket - 1 ) );
                            _chunk += "'>";

                            _in_span++;    // </span> nicht vergessen
                        }
                    }

                    _chunk += _line_prefix;
                    _line_prefix = "";
                    _blank_count = 0;
                }
                else
                {
                    _chunk += _line_prefix;      // War nix. 
                    _line_prefix = "";
                }
            }
            else
            {
                while( i < text.length()  &&  c != '<'  &&  c != '>'  &&  c != '&'  &&  c != '\r'  &&  c != '\n' )  c = text_data[ ++i ];
                _chunk.append( text_data + begin, i - begin );
                if( i == text.length() )  break;
            }

            switch( c )
            {
                case '<' : _chunk += "&lt;";   break;
                case '>' : _chunk += "&gt;";   break;
                case '&' : _chunk += "&amp;";  break;
                case '\r': break;

                case '\n': while( _in_span )  _chunk += "</span>", _in_span--;
                           _chunk += c;        
                           _awaiting_class = true;      // Wir erwarten [info] [error] und dergleichen
                           break;   

                default : _chunk += c;
            }
        }
    }

    return true;  // eof
}

//--------------------------------------------------------------------Html_chunk_reader::read_chunk

string Html_chunk_reader::read_chunk( int _recommended_chunk_size )
{ 
    switch( _state )
    {
        case reading_prefix: _state = reading_text;     break;
        case reading_suffix: _state = reading_finished; break;
        default: ;
    }

    return _chunk;
/*
    switch( _state )
    {
        case reading_prefix:
        {
            _state = reading_text;
            return _html_prefix;
        }

        case reading_text:
        {
            return _chunk;
        }

        case reading_suffix:
        {
            _state = reading_finished;
            return _html_suffix;
        }

        default: throw_xc( __FUNCTION__ );
    }
*/
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
