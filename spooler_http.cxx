// $Id: spooler_http.cxx,v 1.22 2004/12/10 15:19:42 jz Exp $
/*
    Hier sind implementiert

    Http_parser
    Http_request
    Http_response
    Log_chunk_reader
    Html_chunk_reader

    Notdürftige Implementierung von HTTP 1.1
    Siehe http://www.w3.org/Protocols/rfc2616/rfc2616.html
*/


#include "spooler.h"
#include "spooler_version.h"


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
        int rest = _text.length() - ( _body_start + _content_length );
        if( rest > 0 )  
        {
            if( rest == 2  &&  string_ends_with( _text, "\r\n" ) )  {}  // Okay für Firefox
                                                              else  throw_xc( "SPOOLER-HTTP toomuchdata" );
        }

        _http_request->_body.assign( _text.data() + _body_start, _content_length ); 
    }
}

//-------------------------------------------------------------------------Http_parser::is_complete

bool Http_parser::is_complete()
{
    return _reading_body  &&  (    _text.length() == _body_start + _content_length
                                || _text.length() == _body_start + _content_length + 2 );  // Firefox hängt noch ein \r\n an
}

//------------------------------------------------------------------------Http_parser::parse_header

void Http_parser::parse_header()
{
    /*if( z::Log_ptr log = "http" )
    {
        int end = _text.find( '\n' );
        if( end == string::npos )  end = _text.length();   // Vorsichtshalber
                             else  end++;
        log << string( _text, end );
    }*/

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
        if( state == in_path  &&  _next_char[0] == '?' )
        {
            path = word + _next_char[0];
            word = "";
            _next_char++;
            state = in_parameter;
        }
        else
        if( state == in_parameter  &&  _next_char[0] == '&'  ||  (Byte)_next_char[0] <= (Byte)' ' )
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

Http_response::Http_response( Http_request* http_request, Chunk_reader* chunk_reader, const string& content_type )
: 
    _zero_(this+1), 
    _chunk_reader( chunk_reader ),
    _chunked( http_request->is_http_1_1() ),
    _close_connection_at_eof( !http_request->is_http_1_1() ),
    _http_request(http_request)
{ 
    set_content_type(content_type); 
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

    if( _chunked )  _header = "HTTP/1.1 ";
              else  _header = "HTTP/1.0 ";

    if( _status_code )
    {
        _header += as_string( _status_code );
        _header += " ";

        for( int i = 0; i < _status_text.length(); i++ )  if( (uint)_status_text[i] < ' ' )  _status_text[i] = ' ';
        _header += _status_text;
        _header += "\r\n";
    }
    else
    {
        _header += "200 OK\r\n";
    }

    _header += "Content-Type: "  + _content_type + "\r\n"
               "Date: " + string(time_text) + " GMT\r\n"
               "Server: Scheduler " + string(VER_PRODUCTVERSION_STR) + "\r\n";

    //if( _http_request->_header[ "cache-control" ] == "no-cache" )
        _header += "Cache-Control: no-cache\r\n";   // Sonst bleibt z.B. die scheduler.xslt im Browser kleben und ein Wechsel der Datei wirkt nicht.
                                                    // Gut wäre eine Frist, z.B. 10s

    if( _chunked                 )  _header += "Transfer-Encoding: chunked\r\n";
    if( _close_connection_at_eof )  _header += "Connection: close\r\n";

    Z_FOR_EACH( Header_fields, _header_fields, h )  _header += h->first + ": " + h->second + "\r\n";

    _header += "\r\n";

    _chunk_size = _header.length();
    _finished = true;
}

//-------------------------------------------------------------------------------Http_response::eof

bool Http_response::eof()
{
    return _eof;
}

//------------------------------------------------------------------------------Http_response::read

string Http_response::read( int recommended_size )                           
{
    if( !_finished )  finish();


    string result;

    if( _eof )  return result;

    if( _chunk_index == 0  &&  _chunk_offset < _chunk_size )
    {
        //uint length = min( recommended_size, _header.length() - _chunk_offset );
        //uint r      = _chunk_offset;
        //_chunk_offset += length;
        //return _header.substr( r, length );
        _chunk_offset = _chunk_size;
        result = _header;
    }

    if( _chunk_offset == _chunk_size ) 
    {
        result += start_new_chunk();
    }

    if( _chunk_offset < _chunk_size )
    {
        string data =_chunk_reader->read_from_chunk( min( recommended_size, (int)( _chunk_size - _chunk_offset ) ) );
        _chunk_offset += data.length();
        result += data;
    }

    if( _chunk_offset == _chunk_size  &&  !_chunk_eof ) 
    {
        _chunk_eof = true;
        if( _chunked )  result.append( "\r\n" );
    }

    //Z_LOG( __PRETTY_FUNCTION__ << "() ==> " << result << "\n" );

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
    
    //Z_LOG( __PRETTY_FUNCTION__ << "  chunk_size=" << _chunk_size << "\n" );

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

//-------------------------------------------------------------String_chunk_reader::read_from_chunk

string String_chunk_reader::read_from_chunk( int recommended_size )
{ 
    int length = min( recommended_size, (int)_text.length() );

    int offset = _offset;
    _offset += length;

    return _text.substr( offset, length ); 
}

//---------------------------------------------------------------Log_chunk_reader::Log_chunk_reader

Log_chunk_reader::Log_chunk_reader( Prefix_log* log )
: 
    _zero_(this+1), 
    _log(log) 
{
}

//--------------------------------------------------------------Log_chunk_reader::~Log_chunk_reader

Log_chunk_reader::~Log_chunk_reader()
{
    if( _event )  _log->remove_event( _event );
}

//----------------------------------------------------------------------Log_chunk_reader::set_event

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

int Log_chunk_reader::get_next_chunk_size()
{
    if( _file.opened()  &&  !_file_eof )
    {
        uint64 size = _file.length() - _file.tell();
        if( size > 0 )  return size < _recommended_block_size? (int)size : _recommended_block_size;
    }

    return 0;  // eof
}

//----------------------------------------------------------------Log_chunk_reader::read_from_chunk

string Log_chunk_reader::read_from_chunk( int recommended_size )
{ 
    return _file.read_string( recommended_size );
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
        case reading_prefix:  _chunk = _html_prefix;    return true;

        case reading_text:    if( !try_fill_chunk() )   return false;
                              if( _chunk.length() > 0 ) return true;
                              _state = reading_suffix;

        case reading_suffix:  _chunk = _html_suffix;    return true;

        case reading_finished: _chunk = "";             return true;

        default:               return true;
    }
}

//-----------------------------------------------------------Html_chunk_reader::get_next_chunk_size

int Html_chunk_reader::get_next_chunk_size()
{
    return _chunk.length();
}

//----------------------------------------------------------------Html_chunk_reader::try_fill_chunk

bool Html_chunk_reader::try_fill_chunk()
{
    _chunk = "";

    while( _chunk.length() < _recommended_block_size )
    {
        if( _available_net_chunk_size == 0 )
        {
            if( !_chunk_reader->next_chunk_is_ready() )  return _chunk.length() > 0;

            _available_net_chunk_size = _chunk_reader->get_next_chunk_size();
        }

        string text = _chunk_reader->read_from_chunk( _available_net_chunk_size );
        if( text == "" )  return true;  // Fertig, bei _chunk_length() == 0: eof        return _chunk.length() > 0;

        _available_net_chunk_size -= text.length();

        _chunk.reserve( _chunk.length() + text.length() * 2 );

        const char* text_data = text.data();

        for( int i = 0; i < text.length(); i++ )
        {
            int  begin = i;
            char c     = text_data[i];

            if( _awaiting_class )
            {
                while( i < text.length()  &&  c != '<'  &&  c != '>'  &&  c != '&'  &&  c != '\r'  &&  c != ']'  &&  c != '\n' )  c = text_data[ ++i ];
                
                _line_prefix.append( text_data + begin, i - begin );

                if( i == text.length() )  break;

                if( c == ']' )
                {
                    _awaiting_class = false;

                    {
                        int left_bracket  = _line_prefix.find( '[' );
                        int right_bracket = _line_prefix.length();  //_line_prefix.find( ']' );
                        if( left_bracket != string::npos  &&  right_bracket != string::npos  &&  left_bracket < right_bracket )
                        {

                            _chunk += "<span class='log_";
                            _chunk += lcase( _line_prefix.substr( left_bracket + 1, right_bracket - left_bracket - 1 ) );
                            _chunk += "'>";

                            _in_span++;    // </span> nicht vergessen
                        }
                    }


                    /*
                    {
                        int left_parenthesis   = _line_prefix.find( '(' );
                        int right_parenthesis  = _line_prefix.find( ')', left_parenthesis );
                        if( right_parenthesis == string::npos )  right_parenthesis = _line_prefix.length();

                        if( left_parenthesis != string::npos  &&  right_parenthesis != string::npos )
                        {
                            int blank = _line_prefix.find( ' ', left_parenthesis );
                            if( blank == string::npos )  blank = right_parenthesis;

                            _chunk.append( _line_prefix.data(), left_parenthesis );
                            _chunk += "<span class='log_";
                            _chunk += lcase( _line_prefix.substr( left_parenthesis + 1, blank - left_parenthesis - 1 ) );
                            _chunk += "'>";
                            _chunk.append( _line_prefix.data() + left_parenthesis, right_parenthesis + 1 - left_parenthesis );
                            _chunk += "</span>";

                            _line_prefix.erase( 0, right_parenthesis + 1 );
                        }
                    }
                    */

                    _chunk += _line_prefix;
                    _line_prefix = "";
                    //_blank_count = 0;
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

//---------------------------------------------------------------Html_chunk_reader::read_from_chunk

string Html_chunk_reader::read_from_chunk( int recommended_size )
{ 
    switch( _state )
    {
        case reading_prefix:   _state = reading_text;     break;
        case reading_suffix:   _state = reading_finished; break;
        default: ;
    }

    return _chunk;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
