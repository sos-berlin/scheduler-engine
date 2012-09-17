#include "precomp.h"
//#define MODULE_NAME "sosstrng"
// sosstrng.cxx 		(c) Jörg Schwiemann

#include "../kram/sysdep.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosctype.h"
#include "../kram/sosalloc.h"
#include "../kram/soslimtx.h"

using namespace std;
namespace sos {

extern const Sos_string empty_string = "";

//------------------------------------------------------------------------------Sos_string trim

void ltrim( Sos_string* str_ptr )
{
    const char* p = c_str(*str_ptr);
    while (*p && isspace(*p)) { p++; }

    if ( p != c_str(*str_ptr) ) *str_ptr = p;
}

//-----------------------------------------------------------------------------------------trim

void trim( Sos_string* str_ptr )
{
    ltrim( str_ptr );
    rtrim( str_ptr );
}

//-----------------------------------------------------------------------------------------trim

string trim( const string& str )
{
    return ltrim( rtrim( str ) );
}

//---------------------------------------------------------------------------------right_string

string right_string( const string& str, int len )
{
    if( (uint)len > str.length() )  len = str.length();
    return str.substr( str.length() - len, len );
}

//---------------------------------------------------------------------------------------_empty

Bool _empty( const Sos_string& str )
{
    if( length( str ) == 0 ) {
        return true;
    } else {
        int i;
        for( i = 0; i < (int)length( str ); i++ ) {
            if( str[ i ] != ' ' &&  str[ i ] != '\t' )  break;
        }
        return i == length( str );
    }
}

//------------------------------------------------------------------------------------as_string

Sos_string as_string( Big_int o )
{
    ZERO_RETURN_VALUE( Sos_string );

    char buffer [ 50 ];

    int len = sprintf( buffer, "%" PRINTF_LONG_LONG "d", o );
    return as_string( buffer, len );
}

//------------------------------------------------------------------------------------as_string

Sos_string as_string( Ubig_int o )
{
    ZERO_RETURN_VALUE( Sos_string );

    char buffer [ 50 ];

    int len = sprintf( buffer, "%" PRINTF_LONG_LONG "u", o );
    return as_string( buffer, len );
}

//------------------------------------------------------------------------------------as_string

Sos_string as_string( double o )
{
    Sos_limited_text<100> buffer;
    double_type.write_text( (const Byte*)&o, &buffer, std_text_format );
    return string( buffer.char_ptr(), buffer.length() );
}

//--------------------------------------------------------------------------------as_hex_string

string as_hex_string( Big_int o )
{
    char buffer [ 50 ];

    int len = sprintf( buffer, "%" PRINTF_LONG_LONG "X", o );
    return as_string( buffer, len );
}

//--------------------------------------------------------------------------------as_hex_string

string as_hex_string( int o )
{
    char buffer [ 50 ];

    int len = sprintf( buffer, "%X", o );
    return as_string( buffer, len );
}

//--------------------------------------------------------------------------------as_hex_string

string as_hex_string( char o )
{
    char hex[] = "0123456789ABCDEF";

    char buffer[2];

    buffer[0] = hex[ o >> 4 ];
    buffer[1] = hex[ o & 0x0F ];
    return string( buffer, 2 );
}

//---------------------------------------------------------------------------------hex_to_digit

static int hex_to_digit( Byte hex_char )
{
    if( isdigit( hex_char ) )  return hex_char - '0';
    return toupper( hex_char ) - 'A' + 10;
}

//--------------------------------------------------------------------------------hex_as_uint32

uint32 hex_as_int32( const string& str )
{
    uint32 result = 0;
    const char* p0 = str.c_str();
    const char* p  = p0;

    if( str.length() % 2 == 1 )  p--;

    while( *p )
    {
        Byte h1 = p < p0? 0 : p[0];
        Byte h2 = p[1];

        if( !isxdigit( h1 )  ||  !isxdigit( h2 ) )  throw_xc( "SOS-1426", str );
        if( result & 0xFF000000 )  throw_xc( "SOS-1107", str, "int32" );

        result <<= 8;
        result |= (Byte)( hex_to_digit( h1 ) * 16 + hex_to_digit( h2 ) );

        p += 2;
    }

    return result;
}

//---------------------------------------------------------------------------------------as_int

int as_int( const Sos_string& string )
{
    return as_int( c_str( string ) );
}

//---------------------------------------------------------------------------------------as_int

int as_int( const Sos_string& string, int deflt )
{
    return as_int( c_str( string ), deflt );
}

//--------------------------------------------------------------------------------------as_uint

uint as_uint( const Sos_string& string )
{
    return as_uint( c_str( string ) );
}

//-----------------------------------------------------------------------------------as_big_int

Big_int as_big_int( const Sos_string& string )
{
    return as_big_int( c_str( string ) );
}

//----------------------------------------------------------------------------------as_ubig_int

Ubig_int as_ubig_int( const Sos_string& string )
{
    return as_ubig_int( c_str( string ) );
}

//---------------------------------------------------------------------------------------append

void append( Sos_string* string_ptr, const char* ptr, int length )
{
    *string_ptr += as_string( ptr, length );
}

//--------------------------------------------------------------------------------quoted_string

Sos_string quoted_string( const char* text, char quote1, char quote2, size_t len )
{
    ZERO_RETURN_VALUE( Sos_string );

    const char* p = text;
    const char* q;
    const char* e = p + len;
    Sos_string  result = as_string( quote1 );

    while( e - p ) {
        q = (char*)memchr( p, quote2, e - p );
        if( !q )  break;
        if( q - p )  append( &result, p, q - p );
        result += quote2;
        result += quote2;
        p = q + 1;
    }

    append( &result, p, e - p );
    result += quote1;

    return result;
}

//-----------------------------------------------------------------------------should_be_quoted

int should_be_quoted( const char* text )
{
    const char* p = text;
    while( sos_isalnum( *p )  ||  *p == '_' )  p++;
    return *p != '\0';
}

//---------------------------------------------------------------------------cond_quoted_string

Sos_string cond_quoted_string( const char* text, char quote1, char quote2 )
{
    ZERO_RETURN_VALUE( Sos_string );

    if( should_be_quoted( text ) )  return quoted_string( text, quote1, quote2 );
                              else  return text;
}

//--------------------------------------------------------------------------------append_option

void append_option( Sos_string* string, const char* option, const char* value )
{
    *string += option;

    if( should_be_quoted( value ) )  *string += quoted_string( value );
                               else  *string += value;
}

} //namespace sos
