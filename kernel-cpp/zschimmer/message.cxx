// $Id: message.cxx 14183 2011-01-18 17:43:56Z rb $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com


#include "zschimmer.h"
#include "message.h"
#include "log.h"


using namespace std;


namespace zschimmer {

//--------------------------------------------------------------------------------------------const
    
const static size_t             max_insertion_length_const = 500;

//----------------------------------------------------------------------------Source_pos::to_string

string Source_pos::to_string() const
{
    return "in " + _source + " " + as_string( _line ) + ":" + as_string( _col );
}

//-------------------------------------------------------------------Message_string::Message_string

Message_string::Message_string( const string& code )
:
    _max_insertion_length( (size_t)-1 )
{
    set_code( code );
}

//-------------------------------------------------------------------Message_string::Message_string

Message_string::Message_string( const char* code )
:
    _max_insertion_length( (size_t)-1 )
{
    set_code( code );
}

//------------------------------------------------------------------Message_string::~Message_string
    
Message_string::~Message_string()
{
}

//-------------------------------------------------------------Message_string::max_insertion_length
    
size_t Message_string::max_insertion_length()
{
    return _max_insertion_length == (size_t)-1? max_insertion_length_const : _max_insertion_length;
}

//-------------------------------------------------------------------------Message_string::set_code
    
void Message_string::set_code( const string& code )
{
    set_code( code.c_str() );
}

//-------------------------------------------------------------------------Message_string::set_code
    
void Message_string::set_code( const char* code )
{
    _code = code;

    _string = code;
    _string += "  ";

    if( strncmp( code, "MSWIN-", 6 ) == 0 
     || strncmp( code, "OLE-"  , 4 ) == 0 
     || strncmp( code, "COM-"  , 4 ) == 0 )
    {
        const char* p = strchr( code, '-' ) + 1;

#       ifdef Z_WINDOWS
            unsigned long error = 0;
            while( isxdigit( (unsigned char)*p ) )  error = ( error << 4 ) + hex_to_digit( *p++ );
            _string.append( get_mswin_msg_text( error ) );
#        else
            string what = get_error_text( string("COM-") + p );       // MSWIN-xx, OLE-xx => COM-xx
            _string += what;
#       endif
    }
    else
    if( strncmp( code, "ERRNO-", 6 ) == 0 )
    {
        _string += z_strerror( atoi( code+6 ) );
    }
    else
        _string += get_error_text( code );
}

//--------------------------------------------------------------------Message_string::insert_string

void Message_string::insert_string( int index, const string& value ) throw()
{
    insert_string( index, value.c_str() );
}

//--------------------------------------------------------------------Message_string::insert_string

void Message_string::insert_string( int index, const char* value ) throw()
{
    if( value == NULL )  return;     // $1 stehen lassen!

    //if( *value )  Z_LOG( "[xc.insert " << index << ", \"" << value << "\"]\n" );

    char var[3] = "$i"; var[1] = (char)index + '0';

    bool   truncated = false;
    size_t end = strlen(value);
    if( end > max_insertion_length() )  end = max_insertion_length(),  truncated = true;
    
    while( end > 0  &&  isspace( (unsigned char)value[ end - 1 ] ) )  end--;

    string text ( value, end );
    if( truncated )  text += "...";

    size_t pos = _string.find( var );
    if( pos != string::npos )
    {
        _string = _string.substr( 0, pos ) + text + _string.substr( pos+2 );
    }
    else
    {
        _string.append( " [" );
        _string.append( text );
        _string.append( "]" );
    }
}

//------------------------------------------------------------------------------insert_into_message
// Für gcc
void insert_into_message( Message_string* m, int index, const char* s ) throw()    
{ 
    m->insert_string( index, s ); 
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* message_string, int index, int64 value ) throw()
{
    message_string->insert( index, as_string( value ) );
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* message_string, int index, double value ) throw()
{
    char buffer [ 100 ];
    z_snprintf( buffer, sizeof buffer, "%-.15g" , value );
    message_string->insert( index, buffer );
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* message_string, int index, const exception& x ) throw()
{
    message_string->insert( index, x.what() );
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* message_string, int index, const Object& object ) throw()
{
    message_string->insert( index, object.obj_name() );
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* message_string, int index, const Object* object ) throw()
{
    message_string->insert( index, object? object->obj_name() : "NULL" );
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* message_string, int index, const String_stream& s ) throw()
{
    message_string->insert( index, s.to_string() );
}

//-------------------------------------------------------------------------------get_mswin_msg_text

string get_mswin_msg_text( int code )
{
#   ifdef Z_WINDOWS

        char text[ 500+1 ];

        int len = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 code,
                                 MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                 text, sizeof text, 
                                 NULL );

        char* p = text + len;
        if( p > text  &&  p[-1] == '\n' )  p--;  
        if( p > text  &&  p[-1] == '\r' )  p--;

        return string( text, p - text );

#    else

        return "";

#   endif
}

//---------------------------------------------------------------------------operator ostream << Xc

ostream& operator << ( ostream& s, const Xc& x )
{
    s << x._what;
    return s;
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc()  
:
    _return_code(0)
{ 
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const string& code )  
:
    _return_code(0)
{ 
    set_code( code );
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const char* code )  
:
    _return_code(0)
{ 
    set_code( code );
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const exception& x )  
{
    set( x );
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const Message_string& m )  
:
    _code( m.code() ),
    _what( m.as_string() ),
    _return_code(0)
{ 
    if( _what != "" )  Z_LOG( "[ERROR " << what() << "]\n" );
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const char* code, const string& text, const Source_pos& pos )
:
  //_text ( text ),
    _return_code(0)
{
    set_code( code );

    insert( 1, text );

    _what += " (";
    if( !pos._source.empty() )  _what += pos._source + ", ";
    if( pos._line > 0 )  _what += "Zeile " + as_string( pos._line );
    if( pos._col > 0 )  _what += ", Spalte " + as_string( pos._col + 1 );
    _what += ')';

    if( _what != "" )  Z_LOG( "[ERROR " << what() << "]\n" );
}
    
//------------------------------------------------------------------------------------------Xc::~Xc

Xc::~Xc() throw()
{
}

//-------------------------------------------------------------------------------------Xc::set_code

void Xc::set( const exception& x )
{
    if( const Xc* xc = dynamic_cast<const Xc*>( &x ) )
    {
        set_xc( *xc );
    }
    else
    {
        _name        = "exception";
        _code        = "exception";
        _what        = x.what();
        _return_code = 0;

        if( _what != "" )  Z_LOG( "[ERROR " << _what << "]\n" );
    }
}

//-------------------------------------------------------------------------------------Xc::set_code

void Xc::set_xc( const Xc& x )
{
    _name        = x._name;
    _code        = x._code;
    _what        = x._what;
    _return_code = x._return_code;

    if( _what != "" )  Z_LOG( "[ERROR " << _what << "]\n" );
}

//-------------------------------------------------------------------------------------Xc::set_code

void Xc::set_code( const char* code )
{
    if( strncmp( code, "Z-", 2 ) == 0 )
    {
        _code = Z_ERROR_PREFIX;
        _code += code + 1;
    }
    else
    {
        _code = code;
    }

    set_what_by_code( code );
}

//-------------------------------------------------------------------------------------Xc::set_what

void Xc::set_what( const string& text )
{
    _what = text;
    if( _what != "" )  Z_LOG( "[ERROR " << what() << "]\n" );
}

//-----------------------------------------------------------------------------------------Xc::what

const char* Xc::what() const throw()
{
    return _what.c_str();
}

//-----------------------------------------------------------------------------------------Xc::what2

const string Xc::what2() const throw()
{
    return _what;
}

//-----------------------------------------------------------------------------Xc::set_what_by_code

void Xc::set_what_by_code( const char* code )
{
    _what = _code;
    if( !_what.empty() )  _what += "  ";

    if( strncmp( code, "MSWIN-", 6 ) == 0 
     || strncmp( code, "OLE-"  , 4 ) == 0 
     || strncmp( code, "COM-"  , 4 ) == 0 )
    {
        const char* p = strchr( code, '-' ) + 1;

#       ifdef Z_WINDOWS
            unsigned long error = 0;
            while( isxdigit( (unsigned char)*p ) )  error = ( error << 4 ) + hex_to_digit( *p++ );
            _what.append( get_mswin_msg_text( error ) );
#        else
            string what = get_error_text( string("COM-") + p );       // MSWIN-xx, OLE-xx => COM-xx
            _what += what;
#       endif
    }
    else
    if( strncmp( code, "ERRNO-", 6 ) == 0 )
    {
        _what += z_strerror( atoi( code+6 ) );
    }
    else
        _what += get_error_text( code );
}

//---------------------------------------------------------------------------------------Xc::insert    

void Xc::insert( int nr, const char* text_par )
{
    if( text_par == NULL )  return;     // $1 stehen lassen!

    if( *text_par )  Z_LOG( "[xc.insert " << nr << ", \"" << text_par << "\"]\n" );

    char var[3] = "$i"; var[1] = (char)nr + '0';

    bool   truncated = false;
    size_t end = strlen( text_par );
    if( end > max_insertion_length_const )  end = max_insertion_length_const, truncated = true;

    while( end > 0  &&  isspace( (unsigned char)text_par[ end - 1 ] ) )  end--;

    string text ( text_par, end );
    if( truncated )  text += "...";

    size_t pos = _what.find( var );
    if( pos != string::npos )
    {
        _what = _what.substr( 0, pos ) + text + _what.substr( pos+2 );
    }
    else
    {
        _what.append( " [" );
        _what.append( text );
        _what.append( "]" );
    }
}

//---------------------------------------------------------------------------------------Xc::insert    

void Xc::insert( int nr, int i )
{
    insert( nr, as_string( i ) );
}

//----------------------------------------------------------------------------------Xc::append_text

void Xc::append_text( const char* text )
{
    if( text && text[0] )
    {
        Z_LOG( "[xc.append \"" << text << "\"]\n" );

        _what.append( " / " );
        _what.append( rtrim( string( text, min( strlen(text), max_insertion_length_const ) ) ) );
    }
}

//-----------------------------------------------------------------------------------------throw_xc

Z_NORETURN void throw_xc( const string& code ) 
{
    Message_string message ( code );
    message.throw_xc();
}

//-----------------------------------------------------------------------------------------throw_xc

Z_NORETURN void throw_xc( const char* code ) 
{
    Message_string message ( code );
    message.throw_xc();
}

//-----------------------------------------------------------------------------------------throw_xc

void throw_xc( const Xc& x )
{   
    throw x; 
}

//-----------------------------------------------------------------------------------------throw_xc

void throw_xc( const Message_string& m )
{
    throw_xc( Xc( m ) );
}

//-----------------------------------------------------------------------------------------throw_xc

void throw_xc( const char* error_code, const string& text, const Source_pos& pos )
{
    string text2;
    text2 += " (line ";
    text2 += as_string( pos._line );
    text2 += ", column ";
    text2 += as_string( pos._col + 1 );
    text2 += ')';

    throw_xc( error_code, text.c_str(), text2.c_str() );
}

//----------------------------------------------------------------------------------------throw_xc

void throw_xc( const char* error_code, const char* text, const Source_pos& pos )
{
    throw Xc( error_code, string(text), pos );
}

//----------------------------------------------------------------------------------------throw_eof

//void throw_eof()
//{
//    Eof_error x ( "EOF" );
//    throw x;
//}

//--------------------------------------------------------------------------------------throw_errno

void throw_errno( int errn, const char* text1, const char* text2, const char* text3 )
{
    string code = "ERRNO-" + as_string( errn );

    throw Xc( code.c_str(), text1, text2, text3 );
}

//---------------------------------------------------------------------throw_null_pointer_exception

void throw_null_pointer_exception()
{
    throw Null_pointer_exception();
}

//-------------------------------------------------------------------------------------throw_socket

void throw_socket( int errn, const char* text1, const char* text2, const char* text3 )
{
    if( errn < 10000 )  
    {
        throw_errno( errn, text1, text2, text3 );
    }
    else
    {
        string code = "WINSOCK-" + as_string( errn );

        throw Xc( code.c_str(), text1, text2, text3 );
    }
}

//------------------------------------------------------------------------------------throw_pattern

void throw_pattern( const char* pattern, int error, const char* function_name, const char* text1, const char* text2 )
{
    char   code[ 20 ];
    string ins;

    z_snprintf( code, sizeof code, pattern, (long)error );

    if( function_name && function_name[0] )
    {
        ins = function_name;
        if( text1 && text1[0] )  ins += string(", ") + text1;
        if( text2 && text2[0] )  ins += string(", ") + text2;
    }
    else
    {
        if( text1 && text1[0] )  ins +=                text1;
        if( text2 && text2[0] )  ins += string(", ") + text2;
    }

    throw_xc( code, ins );
}

//-------------------------------------------------------------------------------------SetLastError
/*
#ifndef Z_WINDOWS

void SetLastError( int error )
{
    thread_data->_mswin_last_error = error;
}

//-------------------------------------------------------------------------------------GetLastError

int GetLastError()
{
    return thread_data->_mswin_last_error;
}

#endif
*/
//--------------------------------------------------------------------------------------throw_mswin

void throw_mswin( int error, const char* function_name, const char* text1, const char* text2 )
{
    throw_pattern( "MSWIN-%08lX", error, function_name, text1, text2 );
}

//--------------------------------------------------------------------------------------throw_mswin

void throw_mswin( const char* function_name, const char* ins1, const char* ins2 )
{
    throw_mswin( GetLastError(), function_name, ins1, ins2 ); 
}

//----------------------------------------------------------------------------------------throw_com
/*
void throw_com( int error, const char* function_name, const char* text )
{
    throw_pattern( "COM-%08lX", error, function_name, text );
}
*/
//---------------------------------------------------------------------------------------errno_code

string errno_code( int errn )
{
    return "ERRNO-" + as_string( errn );
}

//-------------------------------------------------------------------------------------------------



} //namespace zschimmer
