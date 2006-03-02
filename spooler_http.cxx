// $Id$
/*
    Hier sind implementiert

    Parser
    Request
    Response
    Log_chunk_reader
    Html_chunk_reader
    String_chunk_reader

    Notdürftige Implementierung von HTTP 1.1
    Siehe http://www.w3.org/Protocols/rfc2616/rfc2616.html
*/


/*
    Mögliche Verbesserungen:

    HTTP 1.1 ist nur dürftig implementiert.
    Pipelining ist derzeit nicht möglich. Dazu muss ein signalisiertes recv während send() möglich sein --> Async_socket umarbeiten, getrenntes _state für recv() und send()
    "Transfer-Encoding: chunked" nicht, wenn nur ein oder kein Chunk da ist (z.B.String_chunk_reader)
    Mehrere Chunks mit einem send-scattered() senden
    _operation->begin() schon wenn Kopf empfangen. Antworten und Anfrage empfangen gleichzeitig!  Request->get_part()
    "Last-Modified:" der Datei senden

*/


#include "spooler.h"
#include "spooler_version.h"
#include "../zschimmer/charset.h"

using namespace std;

namespace sos {
namespace spooler {
namespace http {

//-------------------------------------------------------------------------------------------static

stdext::hash_map<int,string>  http_status_messages;

//--------------------------------------------------------------------------------------------const

struct Http_status_code_table 
{   
    Status_code                _code; 
    const char*                _text;
};

const Http_status_code_table http_status_code_table[]  =
{
    { status_200_ok                            , "OK" },
    { status_301_moved_permanently             , "Moved Permanently" },
    { status_403_forbidden                     , "Forbidden" },
    { status_404_bad_request                   , "Bad Request" },
    { status_500_internal_server_error         , "Internal Server Error" },
    { status_501_not_implemented               , "Not Implemented" },
    { status_504_gateway_timeout               , "Gateway Timeout" },
    { status_505_http_version_not_supported    , "HTTP Version Not Supported" },
    {}
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( scheduler_http )
{
    for( const Http_status_code_table* p = http_status_code_table; p->_code; p++ )
    {
        http_status_messages[ (int)p->_code ] = p->_text;
    }
}

//--------------------------------------------------------------------------------------date_string

string date_string( time_t t )
{
    char time_text[] = "Sun Jan 01 00:00:00 1900 GMT";
    //                  ^^^^^^^^^^^^^^^^^^^^^^^^^ wird von asctime() überschrieben

#   ifdef Z_WINDOWS
        asctime_s( time_text, sizeof time_text, gmtime( &t ) );
#    else
        struct tm  tm;
        asctime_r( gmtime_r( &t, &tm ), time_text );
#   endif
    
    strcpy( time_text + sizeof time_text - 5, " GMT" );   // asctime() hat das überschrieben
    return time_text;
}

//--------------------------------------------------------------------------get_content_type_parameter

string get_content_type_parameter( const string& content_type, const string& param_name )
{
    const char* p = content_type.c_str();
    
    while(1)
    {
        p = strchr( p, ';' );
        if( !p )  break;

        p++;
        while( isspace( (unsigned char)*p ) )  p++;

        const char* n = param_name.c_str();

        while( *p  &&  *n  &&  tolower( (unsigned char)*p ) == tolower( (unsigned char)*n ) )  p++, n++;
        if( *n == 0 &&  *p == '=' )
        {
            p++;
            string result;
            while( *p  &&  *p != ';' )  result += *p++;
            return rtrim( result );
        }
    }

    return "";
}

//------------------------------------------------------------------------------Headers::operator[]

string Headers::operator[]( const string& name ) const
{
    Map::const_iterator it = _map.find( lcase( name ) );
    return it == _map.end()? "" : it->second._value;
}

//-------------------------------------------------------------------------------------Headers::set

void Headers::set( const string& name, const string& value )
{
    if( name == "" )  z::throw_xc( __FUNCTION__, "Name is empty" );
    if( value.find( '\n' ) != string::npos )  z::throw_xc( __FUNCTION__, "Value has multiple lines" );

    set_unchecked( name, value );
}

//---------------------------------------------------------------------------Headers::set_unchecked

void Headers::set_unchecked( const string& name, const string& value )
{
    string lname = lcase( name );

    if( value != "" )  _map[ lname ] = Entry( name, value );
                 else  _map.erase( lname );
}

//-----------------------------------------------------------------------------Headers::set_default

void Headers::set_default( const string& name, const string& value )
{
    if( value != "" )
    {
        string        lname = lcase( name );
        Map::iterator it    = _map.find( lname );

        if( it == _map.end() )  set( lname, value );
    }
}

//-----------------------------------------------------------------------------------Headers::print

void Headers::print( ostream* s ) const
{
    Z_FOR_EACH_CONST( Map, _map, h )
    {
        *s << h->second._name << ": " << h->second._value << "\r\n";
    }
}

//-----------------------------------------------------------------------------------Headers::print

void Headers::print( ostream* s, const string& header_name ) const
{
    Map::const_iterator it = _map.find( lcase( header_name ) );
    if( it != _map.end() )  print( s, it );
}

//-----------------------------------------------------------------------------------Headers::print

void Headers::print( ostream* s, const Headers::Map::const_iterator& it ) const
{
    if( it != _map.end() )
    {
        *s << it->second._name << ": " << it->second._value << "\r\n";
    }
}

//------------------------------------------------------------------------Headers::print_and_remove

void Headers::print_and_remove( ostream* s, const string& header_name )
{
    Map::iterator it = _map.find( lcase( header_name ) );
    if( it != _map.end() )
    {
        print( s, it );
        _map.erase( it );
    }
}

//-----------------------------------------------------------------------------------Parser::Parser
    
Parser::Parser( Request* request )
:
    _zero_(this+1),
    _request( request )
{
    _text.reserve( 2000 );
}

//------------------------------------------------------------------------------------Parser::close
    
    void Parser::close()
{
    _request = NULL;
}

//---------------------------------------------------------------------------------Parser::add_text
    
void Parser::add_text( const char* text, int len )
{
    _text.append( text, len );

    if( !_reading_body )
    {
        int end_size = 3;
        int header_end = _text.find( "\n\r\n" );  // Leerzeile?
        if( header_end == string::npos )  header_end = _text.find( "\n\n" ), end_size = 2;

        if( header_end != string::npos )
        {
            // Kopf gelesen

            _body_start = header_end + end_size;
            _reading_body = true;

            parse_header();
            string content_length = _request->_headers[ "content-length" ];
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

        _request->_body.assign( _text.data() + _body_start, _content_length ); 
    }
}

//------------------------------------------------------------------------------Parser::is_complete

bool Parser::is_complete()
{
    return _reading_body  &&  (    _text.length() == _body_start + _content_length
                                || _text.length() == _body_start + _content_length + 2 );  // Firefox hängt noch ein \r\n an
}

//-----------------------------------------------------------------------------Parser::parse_header

void Parser::parse_header()
{
    /*if( z::Log_ptr log = "http" )
    {
        int end = _text.find( '\n' );
        if( end == string::npos )  end = _text.length();   // Vorsichtshalber
                             else  end++;
        log << string( _text, end );
    }*/

    _next_char = _text.c_str();

    _request->_http_cmd = eat_word();
    _request->_path     = eat_path();
    _request->_protocol = eat_word();
    eat_line_end();

    while( next_char() > ' ' )
    {
        string name = eat_until( ":" );
                      eat( ":" );
        string value = eat_until( "" );
        _request->_headers.set_unchecked( name, value );
        eat_line_end();
    }

    eat_line_end();
}

//-------------------------------------------------------------------------------Parser::eat_spaces

void Parser::eat_spaces()
{ 
    while( *_next_char == ' ' )  _next_char++; 
}

//--------------------------------------------------------------------------------------Parser::eat

void Parser::eat( const char* what )
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

//-----------------------------------------------------------------------------Parser::eat_line_end

void Parser::eat_line_end()
{
    eat_spaces();

    if( *_next_char == '\r' )  _next_char++;
    eat( "\n" );
}

//---------------------------------------------------------------------------------Parser::eat_word

string Parser::eat_word()
{
    string word;
    while( *_next_char > ' ' )  word += *_next_char++;

    eat_spaces();
    return word;
}

//--------------------------------------------------------------------------------Parser::eat_until

string Parser::eat_until( const char* character_set )
{
    string word;
    while( *_next_char >= ' '  &&  strchr( character_set, *_next_char ) == NULL )  word += *_next_char++;

    eat_spaces();
    return rtrim( word );
}

//---------------------------------------------------------------------------------Parser::eat_path

string Parser::eat_path()
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
                              else  _request->_parameters[ parameter_name ] = word;
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

    if( !string_begins_with( path, "/" ) )  path = "/" + path;

    return path;
}

//-----------------------------------------------------------------------------Operation::Operation

Operation::Operation( Operation_connection* pc )
: 
    Communication::Operation( pc ), 
    _zero_(this+1)
{
    _request  = Z_NEW( Request() );
    _parser   = Z_NEW( Parser( _request ) );
}

//---------------------------------------------------------------------------------Operation::close
    
void Operation::close()
{ 
    if( _web_service_operation )  _web_service_operation->close(),  _web_service_operation = NULL;
    if( _response )  _response->close(),  _response = NULL;
    if( _parser   )  _parser  ->close(),  _parser   = NULL;
    if( _request  )  _request ->close(),  _request  = NULL;

    Communication::Operation::close(); 
}

//---------------------------------------------------------------------------------Operation::begin

void Operation::begin()
{
    Z_LOG2( "scheduler.http", "HTTP: " << _parser->text() << "\n" );    // Wird auch mit "socket.data" protokolliert (default aus)

    _response = Z_NEW( Response( this ) );

    try
    {
        _request->check();

        if( Web_service* web_service = _spooler->_web_services.web_service_by_url_path_or_null( _request->_path ) )
        {
            _web_service_operation = web_service->new_operation( this );
            _web_service_operation->begin();
        }
        else
        {
            Command_processor command_processor ( _spooler, _connection->_security_level, this );
            command_processor.execute_http( this );

            _response->set_ready();
        }
    }
    catch( Http_exception& x )
    {
        _connection->_log.error( x.what() );
        _response->set_status( x._status_code, x.what() );
        _response->set_ready();
    }
    catch( exception& x )
    {
        _connection->_log.error( x.what() );
        _response->set_status( status_500_internal_server_error, x.what() );
        _response->set_ready();
    }


    _response->set_event( &_connection->_socket_event );
    _response->recommend_block_size( 32768 );

    //_parser  = NULL;
    //_request = NULL;
}

//-----------------------------------------------------------------------Operation::async_continue_

bool Operation::async_continue_( Continue_flags flags )
{
    bool something_done = false;

    if( flags & cont_next_gmtime_reached )
    {
        if( _response &&  !_response->is_ready() ) 
        {
            if( _connection )  _connection->_log.error( "HTTP-Operation wird nach Fristablauf abgebrochen" );
            
            if( _response )
            {
                _response->set_status( status_504_gateway_timeout );
                _response->set_ready();
            }

            something_done = true;
        }
    }
    /*
    else
    {
        if( _web_service_operation ) 
        {
            something_done |= _web_service_operation->async_continue( flags );  // Da passiert nix
        }
    }
    */

    return something_done;
}

//----------------------------------------------------------------------Operation::get_response_part

string Operation::get_response_part()
{ 
    return _response->read( _response->recommended_block_size() );
}

//-------------------------------------------------------------------Operation::response_is_complete

bool Operation::response_is_complete()
{ 
    return !_response || _response->eof(); 
}

//---------------------------------------------------------------Operation::should_close_connection

bool Operation::should_close_connection()
{ 
    return _response  &&  _response->close_connection_at_eof(); 
}

//---------------------------------------------------------------------------Operation::dom_element

xml::Element_ptr Operation::dom_element( const xml::Document_ptr& document, const Show_what& what ) const
{
    xml::Element_ptr result = document.createElement( "http_operation" );

    if( _web_service_operation )
    {
        result.appendChild( _web_service_operation->dom_element( document, what ) );
    }

    return result;
}

//-----------------------------------------------------------------------------------Request::close

void Request::close()
{
}

//-------------------------------------------------------------------------------Request::parameter
    
string Request::parameter( const string& name ) const
{ 
    String_map::const_iterator it = _parameters.find( name );
    return it == _parameters.end()? "" : it->second;
}

//-------------------------------------------------------------------------Request::keep_connection

bool Request::is_http_1_1() const
{
    return _protocol == "HTTP/1.1";
}

//-------------------------------------------------------------------------------------Request::url

string Request::url() const
{
    S result;
    result << "http://" << _headers[ "host" ] << url_path();        // Zum Beispiel "Host: hostname:80"
    return result;
}

//----------------------------------------------------------------------------Request::content_type

string Request::content_type() const
{
    string content_type = _headers[ "content-type" ];
    if( content_type == "" )  return "";

    size_t pos = content_type.find( ";" );
    if( pos == string::npos )  pos = content_type.length();

    return rtrim( content_type.substr( 0, pos ) );
}

//----------------------------------------------------------------------Request::charset_name

string Request::charset_name() const
{
    return get_content_type_parameter( _headers[ "content-type" ], "charset" );
}

//-----------------------------------------------------------------------------------Request::check

void Request::check()
{
    if( _protocol != ""  
     && _protocol != "HTTP/1.0"  
     && _protocol != "HTTP/1.1" )  throw Http_exception( status_505_http_version_not_supported );

    if( !string_begins_with( _path, "/" ) )  throw Http_exception( status_404_bad_request );
}

//----------------------------------------------------------------------Request::get_String_content

STDMETHODIMP Request::get_String_content( BSTR* result )
{
    HRESULT hr = S_OK;
    
    try
    {
        hr = Charset::for_name( charset_name() )->Encoded_to_bstr( _body, result );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//----------------------------------------------------------------------Request::get_Binary_content

STDMETHODIMP Request::get_Binary_content( SAFEARRAY** result )
{
    HRESULT hr = S_OK;
    
    try
    {
        Locked_safearray<unsigned char> safearray ( _body.length() );
        memcpy( &safearray[0], _body.data(), _body.length() );
        *result = safearray.take_safearray();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------------------Http_exception::what

const char* Http_exception::what()
{ 
    S result;
    
    result << "HTTP " << (int)_status_code << ' ' << http_status_messages[ _status_code ].c_str();
    if( _error_text != "" )  result << ": " << _error_text;
    
    _what = result;
    return _what.c_str();
}

//-------------------------------------------------------------------------------Response::Response

Response::Response( Operation* operation )
: 
    _zero_(this+1), 
    _chunked( operation->request()->is_http_1_1() ),
    _close_connection_at_eof( !operation->request()->is_http_1_1() ),
    _operation(operation)
{ 
    if( _close_connection_at_eof )  _headers.set_default( "Connection", "close" );

    if( _operation->_request->_headers[ "cache-control" ] == "no-cache" )
        _headers.set( "Cache-Control", "no-cache" );   // Sonst bleibt z.B. die scheduler.xslt im Browser kleben und ein Wechsel der Datei wirkt nicht.
                                                       // Gut wäre eine Frist, z.B. 10s
}

//-----------------------------------------------------------------------------------esponse::close
    
void Response::close()
{
    _operation = NULL;
    _chunk_reader = NULL;
}

//-----------------------------------------------------------------------------Response::set_status
    
void Response::set_status( Status_code code, const string& )
{ 
    _status_code = code; 

    /* Die Fehlermeldung sollte nicht dem Benutzer gezeigt werden. Sie kann Pfadnamen enthalten, z.B. bei ERRNO-2
    if( text != "" )
    {
        S body;
        body << "<html><body><p style='color: red; font-weight: bold;'>";

        for( const char* p = text.c_str(); *p; p++ )
        {
            switch( *p )
            {
                case '&': body << "&amp;";  break;
                case '<': body << "&lt;" ;  break;
                case '>': body << "&gt;" ;  break;
                default: body << *p;
            }
        }

        body << "</p></body></html>";
        
        set_chunk_reader( Z_NEW( String_chunk_reader( body, "text/html" ) ) );
    }
    */
}

//------------------------------------------------------------------------------Response::set_ready

void Response::set_ready()
{ 
    _ready = true; 
    _operation->_connection->signal( "http response is ready" );
}

//-----------------------------------------------------------------------------------Response::send

void Response::send()
{
    if( is_ready() )  throw_xc( "SCHEDULER-247" );

    set_ready();
}

//---------------------------------------------------------------------------------Response::finish

void Response::finish()
{
    if( !_status_code )  _status_code = status_200_ok;

    if( (int)_status_code >= 100  &&  (int)_status_code <= 199  ||  (int)_status_code == 204  || (int)_status_code == 304 )       // RFC 2617 4.4
    {
        if( _chunk_reader )
        {
            Z_LOG( "_chunk_reader entfernt\n" ); 
            _chunk_reader = NULL;
        }
    }

    _chunked = _operation->request()->is_http_1_1();


    // _headers füllen

    if( !_headers.contains( "Date" ) )  _headers.set( "Date", date_string( ::time(NULL) ) );
    if( _chunk_reader )  _headers.set_default( "Content-Type", _chunk_reader->_content_type );


    // _headers_stream schreiben

    _headers_stream << _operation->request()->_protocol << ' ' << _status_code << ' ' << http_status_messages[ _status_code ] << "\r\n";
    _headers.print( &_headers_stream );

    if( _chunked )  _headers_stream << "Transfer-Encoding: chunked\r\n";
          //  else  _headers_stream << "Content-Length: ???\r\n";

    _headers_stream << "Server: Scheduler " VER_PRODUCTVERSION_STR;
    //if( Web_service_operation* wso = _operation->_web_service_operation )
    //    _headers_stream << wso->_web_service->obj_name();
    _headers_stream << "\r\n\r\n";

    _chunk_size = _headers_stream.length();
    _finished = true;
}

//------------------------------------------------------------------------------------Response::eof

bool Response::eof()
{
    return _eof;
}

//-----------------------------------------------------------------------------------Response::read

string Response::read( int recommended_size )                           
{
    if( !_finished )  finish();


    string result;

    if( _eof )  return result;

    if( _chunk_index == 0  &&  _chunk_offset < _chunk_size )
    {
        result = _headers_stream;
        _chunk_offset += _headers_stream.length();
    }

    if( _chunk_offset == _chunk_size ) 
    {
        if( _chunk_reader ) 
        {
            result += start_new_chunk();
        }
        else 
        {
            _eof = true;
            if( _chunked )  result += "0\r\n\r\n";
        }
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

//------------------------------------------------------------------------Response::start_new_chunk

string Response::start_new_chunk()
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

//------------------------------------------------------------------------Response::put_Status_code

STDMETHODIMP Response::put_Status_code( int code )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        set_status( (Status_code)code );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------------------Response::put_Header

STDMETHODIMP Response::put_Header( BSTR name, BSTR value )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        set_header( string_from_bstr( name ), string_from_bstr( value ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------------------Response::get_Header

STDMETHODIMP Response::get_Header( BSTR name, BSTR* result )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        hr = String_to_bstr( header( string_from_bstr( name ) ), result );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------Web_service_response::put_Character_encoding
/*
STDMETHODIMP Web_service_response::put_Character_encoding( BSTR encoding )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        return E_NOTIMPL;
        //_http_response->set_character_encoding( string_from_bstr( encoding ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------Web_service_response::get_Character_encoding

STDMETHODIMP Web_service_response::get_Character_encoding( BSTR* result )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        return E_NOTIMPL;
        //_http_response->set_character_encoding( string_from_bstr( encoding ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------Web_service_response::put_Content_type

STDMETHODIMP Web_service_response::put_Content_type( BSTR content_type )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        //http_response()->set_content_type( string_from_bstr( content_type ) ); 
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------Web_service_response::get_Content_type

STDMETHODIMP Web_service_response::get_Content_type( BSTR* result )
{
    HRESULT hr = S_OK;
    
    try
    {
        return E_NOTIMPL;
        //_http_response->set_content_type( string_from_bstr( content_type ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}
*/
//---------------------------------------------------------------------Response::put_String_content

STDMETHODIMP Response::put_String_content( BSTR content_bstr )
{
    HRESULT hr = S_OK;
    
    try
    {
        const Charset* charset = Charset::for_name( get_content_type_parameter( header( "content-type" ), "charset" ) );

        set_chunk_reader( Z_NEW( String_chunk_reader( charset->encoded_from_bstr( content_bstr ), "" ) ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------Response::put_Binary_content

STDMETHODIMP Response::put_Binary_content( SAFEARRAY* safearray )
{
    HRESULT hr = S_OK;
    
    try
    {
        Locked_safearray<Byte> a ( safearray );

        set_chunk_reader( Z_NEW( Byte_chunk_reader( a.data(), a.count(), "" ) ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------------------------Response::Send

STDMETHODIMP Response::Send() // VARIANT* content, BSTR content_type_bstr )
{
    if( closed() )  return E_FAIL;

    HRESULT hr = S_OK;
    
    //if( !content )  return E_POINTER;

    try
    {
        send();

        /*
        string                  content_type = string_from_bstr( content_type_bstr );
        ptr<http::Chunk_reader> chunk_reader;


        if( content->vt == VT_BSTR )
        {
            const Charset*  charset      = Charset::for_name( http::get_content_type_parameter( content_type, "charset" ) );
            const BSTR      content_bstr = V_BSTR( content );

            chunk_reader = Z_NEW( http::String_chunk_reader( charset->encoded_from_bstr( content_bstr ), content_type ) );
        }
        else
        if( content->vt == VT_ARRAY )
        {
            SAFEARRAY* safearray = V_ARRAY( content );
            VARTYPE    vartype   = 0;

            hr = SafeArrayGetVartype( safearray, &vartype );
            if( FAILED(hr) )  return hr;
            if( vartype != VT_UI1 )  return DISP_E_TYPEMISMATCH;

            Locked_safearray<Byte> a ( safearray );

            chunk_reader = Z_NEW( http::Byte_chunk_reader( &a[0], a.count(), content_type ) );
        }
        else
            return DISP_E_TYPEMISMATCH;


        http_response()->set_chunk_reader( chunk_reader );
        */
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
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
    int length = min( recommended_size, (int)( _text.length() - _offset ) );

    int offset = _offset;
    _offset += length;

    return _text.substr( offset, length ); 
}

//---------------------------------------------------------------Log_chunk_reader::Log_chunk_reader

Log_chunk_reader::Log_chunk_reader( Prefix_log* log )
: 
    Chunk_reader( "text/html; charset=" + scheduler_character_encoding ),
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
OPEN:
    if( !_file.opened() )
    {
        if( !_log->started() )  return false;       // Wenn Log noch nicht gestartet worden ist (z.B. Order in der Warteschlange), dann gibt's noch keine Datei
                                                    // Und wenn die Datei schon geschlossen ist? Kann das passieren?
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
        if( _log->filename() != _file.filename() )  // Dateiname gewechselt (Log.start_new_file)? Dann beenden wir das Protokoll
        {
            _file.close();
            _html_insertion = "<hr size='1'/>";
            goto OPEN;
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

//-----------------------------------------------------------------Log_chunk_reader::html_insertion

string Log_chunk_reader::html_insertion()
{ 
    string result = _html_insertion;
    _html_insertion = "";
    return result;
}

//-------------------------------------------------------------Html_chunk_reader::Html_chunk_reader

Html_chunk_reader::Html_chunk_reader( Chunk_reader* chunk_reader, const string& title )
: 
    Chunk_reader_filter( chunk_reader, "text/html; charset=" + get_content_type_parameter( chunk_reader->content_type(), "charset" ) ),
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

        _chunk.append( _chunk_reader->html_insertion() );   // Z.B. <hr/>

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

string Html_chunk_reader::read_from_chunk( int )
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

} //namespace http
} //namespace spooler
} //namespace sos
