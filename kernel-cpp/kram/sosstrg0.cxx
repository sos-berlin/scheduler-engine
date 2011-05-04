// $Id$

//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "sosstrg0"
//#define COPYRIGHT   "© 1885 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>         // MAX_LONG

#if !defined _MSC_VER
#   include <values.h>         // MAXINT
#endif

#include "../kram/sosstrng.h"       // MSC++ definiert MAXSHORT 
#include "../kram/sosstrg0.h"
#include "../kram/sos.h"
#include "../kram/soslimtx.h"
#include "../kram/tabucase.h"

using namespace std;
namespace sos {

//--------------------------------------------------------------------------------------as_bool

Bool as_bool( const char* str )
{
    check_string0_pointer( str, "as_bool" );

    if( strcmp ( str, "0" )     == 0 )  return false;
    if( strcmp ( str, "1" )     == 0 )  return true;

    Sos_limited_text<10> buffer;

    int len = length_without_trailing_spaces( str, strlen( str ) );
    if( len > buffer.size() )  goto FEHLER;
    buffer.assign( str, len );
    buffer.lower_case();
    buffer[ len ] = '\0';

    if( memcmp( buffer.char_ptr(), "true" , 5 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "false", 6 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "yes"  , 4 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "no"   , 3 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "y"    , 2 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "n"    , 2 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "j"    , 2 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "n"    , 2 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "on"   , 3 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "off"  , 4 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "ja"   , 3 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "nein" , 5 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "wahr"  , 5 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "falsch", 6 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "an"   , 3 ) == 0 )  return true;
    if( memcmp( buffer.char_ptr(), "aus"  , 4 ) == 0 )  return false;

    if( memcmp( buffer.char_ptr(), "-1"   , 2 ) == 0 )  return true;        // OLE (VARIANT_TRUE)

  FEHLER:
    throw_conversion_error( "SOS-1240", str );
    return false;
}

//--------------------------------------------------------------------------------------as_bool

Bool as_bool( const char* str, Bool deflt )
{
    return empty( str )? deflt
                       : as_bool ( str );
}

//---------------------------------------------------------------------------------------_empty

Bool _empty( const char* str )
{
    check_string0_pointer( str, "empty" );

    if( !str )  return true;
    const char* p = str;
    while( *p == ' '  ||  *p == '\t' )  p++;
    return *p == '\0';
}

//----------------------------------------------------------------------------------as_uint64
#if defined SYSTEM_INT64

uint64 as_uint64( const char* str )
{
    uint64      n = 0;
    const char* p = str;

    if( p[0] == '0'  &&  ( p[1] == 'x' || p[1] == 'x' ) )       // 0xHEX
    {
        p += 2;

        while(1)
        {
            uint digit = *p - '0';
            if( digit > 9 ) {
                digit = sos_toupper( (Byte)*p ) - 'A';
                if( digit > 6 )  break;
                digit += 10;
            }

            if( n > UINT64_MAX / 0x10 )  goto OVERFLW;
            n *= 0x10;
            if( n > UINT64_MAX - digit )  goto OVERFLW;
            n += digit;
            p++;
        }
    }
    else
    {
        while( 1 )
        {
            uint digit = *p - '0';
            if( digit > 9 )  break;

            if( n > UINT64_MAX / 10 )  goto OVERFLW;
            n *= 10;
            if( n > UINT64_MAX - digit )  goto OVERFLW;
            n += digit;
            p++;
        }
    }

    if( !empty( p )  ||  p == str )  {
        if( empty( str ) )  throw_null_error( "SOS-1270", "integer" );
                      else  throw_conversion_error( "SOS-1435", str );
    }

    return n;

  OVERFLW:
  	throw_overflow_error( "SOS-1107", "uint64", str );
    return 0;
}

//-----------------------------------------------------------------------------------as_int64

int64 as_int64( const char* str )
{
    const char* p    = str;
    int         sign = 1;

    if( *p == '+' )  p++;
    if( *p == '-' )  { sign = -1; p++; }

    uint64 n = as_uint64( p );
    if( n > (uint64)INT64_MAX + ( sign == -1? 1 : 0 ) )  throw_overflow_error( "SOS-1107", "int64", str );
    return sign * n;
}

#endif
//-----------------------------------------------------------------------------------as_big_int

Big_int as_big_int( const char* str )
{
#   ifdef SYSTEM_INT64
        return as_int64( str );
#   else
        return as_int32( str );
#   endif
}

//----------------------------------------------------------------------------------as_ubig_int

Ubig_int as_ubig_int( const char* str )
{
#   ifdef SYSTEM_INT64
        return as_uint64( str );
#   else
        return as_uint32( str );
#   endif
}

//--------------------------------------------------------------------------------------as_long

long as_long( const char* str )
{
    Big_int n = as_big_int( str );
    if( n > LONG_MAX  ||  n < -LONG_MAX - 1 )  throw_overflow_error( "SOS-1107", "long", str );
    return (long)n;
}

//-------------------------------------------------------------------------------------as_ulong

unsigned long as_ulong( const char* str )
{
    Ubig_int n = as_ubig_int( str );
    if( n > ULONG_MAX )  throw_overflow_error( "SOS-1107", "unsigned long", str );
    return (unsigned long)n;
}

//---------------------------------------------------------------------------------------as_int

int as_int( const char* str )
{
    Big_int n = as_big_int( str );
    if( n > MAXINT  ||  n < -MAXINT - 1 )  throw_overflow_error( "SOS-1107", "int", str );
    return (int)n;
}

//---------------------------------------------------------------------------------------as_int

int as_int( const char* str, int deflt )
{
    if( empty(str) )  return deflt;

    Big_int n = as_big_int( str );
    if( n > MAXINT  ||  n < -MAXINT - 1 )  throw_overflow_error( "SOS-1107", "int", str );
    return (int)n;
}

//--------------------------------------------------------------------------------------as_uint

uint as_uint( const char* str )
{
    Ubig_int n = as_ubig_int( str );
    if( n > UINT_MAX )  throw_overflow_error( "SOS-1107", "unsigned int", str );
    return (uint)n;
}

//-------------------------------------------------------------------------------------as_short

short as_short( const char* str )
{
    Big_int n = as_big_int( str );
    if( n > SHORT_MAX  ||  n < -SHORT_MAX - 1 )  throw_overflow_error( "SOS-1107", "short", str );
    return (short)n;
}

//------------------------------------------------------------------------------------as_ushort

unsigned short as_ushort( const char* str )
{
    Ubig_int n = as_big_int( str );
    if( n > USHRT_MAX )  throw_overflow_error( "SOS-1107", "unsigned short", str );
    return (unsigned short)n;
}

//-------------------------------------------------------------------------------------as_int32

int32 as_int32( const char* str )
{
    Big_int n = as_big_int( str );
    if( n > (Big_int)INT32_MAX  ||  n < -(Big_int)INT32_MAX - 1 )  throw_overflow_error( "SOS-1107", "int32", str );
    return (int32)n;
}

//------------------------------------------------------------------------------------as_uint32

uint32 as_uint32( const char* str )
{
    Ubig_int n = as_ubig_int( str );
    if( n > (Ubig_int)UINT32_MAX )  throw_overflow_error( "SOS-1107", "unsigned int32", str );
    return (uint32)n;
}

//-------------------------------------------------------------------------------------as_int16

int16 as_int16( const char* str )
{
    Big_int n = as_big_int( str );
    if( n > (Big_int)INT16_MAX  ||  n < -(Big_int)INT16_MAX - 1 )  throw_overflow_error( "SOS-1107", "int16", str );
    return (int16)n;
}

//------------------------------------------------------------------------------------as_uint16

uint16 as_uint16( const char* str )
{
    Ubig_int n = as_ubig_int( str );
    if( n > UINT16_MAX )  throw_overflow_error( "SOS-1107", "unsigned int16", str );
    return (uint16)n;
}

//-------------------------------------------------------------------------------------as_uintK

int as_uintK( const char* str )
{
    char  buffer [ 20+1 ];
    char* q = buffer;
    const char* p = str;

    if( empty( str ) )  throw_null_error( "SOS-1270", "integer" );

    while( isdigit( *p )  &&  q < buffer + sizeof buffer - 1 )  *q++ = *p++;
    *q++ = '\0';

    if( p == str )  throw_conversion_error( "SOS-1435", str );

    uint n = as_uint( buffer );

    if( *p == 'K' ) {
        p++;
        if( n > INT_MAX / 1024 ) throw_overflow_error( "SOS-1107", "uint", str );
        n *= 1024;
    }
    else 
    if( *p == 'M' ) {
        p++;
        if( n > INT_MAX / 1024*1024 ) throw_overflow_error( "SOS-1107", "uint", str );
        n *= 1024*1024;
    }

    if( !empty( p )  ||  p == str )  throw_conversion_error( "SOS-1435", str );

    return (int)n;
}

//------------------------------------------------------------------------------c_str_as_double

double c_str_as_double( const char* str )
{
    char* t;
    double a = strtod( str, &t );

    while( *t == ' ' )  t++;
    if( t == str || !empty( t ) )  {
        if( empty( str ) )  throw_null_error( "SOS-1270", "double" );
                      else  if( *t == ',' )  throw_conversion_error( "SOS-1436", str );
                                       else  throw_conversion_error( "SOS-1140", str );
    }

    return a;
}

//--------------------------------------------------------------------------position_in_string0

unsigned int position_in_string0( const char* str, char to_find, unsigned int pos )
{
    check_string0_pointer( str, "position_in_string0" );

    uint length = strlen( str );
    if( pos > length )  return length;
    const char* p = (const char*) memchr( str + pos, to_find, length - pos );
    return p? p - str : length;
}

//-------------------------------------------------------------------------------------position

uint position( const char* string, const char* to_find, unsigned int pos )
{
    check_string0_pointer( string, "position" );

    uint        to_find_length = length( to_find );
    const char* s              = string + pos;
    const char* s_end          = s + length( s ) - to_find_length + 1;

    while( s < s_end ) {
        if( memcmp( s, to_find, to_find_length ) == 0 )  break;
        s++;
    }

    return s - string;  // s == s_end? string + length( string ) - s    ?????
}

//---------------------------------------------------------------length_without_trailing_spaces
/*
uint length_without_trailing_spaces( const char* text, uint len )
{
    // Der String wird Wortweise (int32) geprüft.
    // Dabei werden bis drei Bytes vor dem String gelesen, die müssen aber zugreifbar sein, weil sie im selben Speicherwort liegen.

    static const int32 blanks = ( (int32) ' ' << 24 ) | ( (int32) ' ' << 16 ) | ( (int32) ' ' << 8 ) | ' ';
    
    const char* p = text + len;

    if( *(int32*)( (long)text & (long)~3 ) == blanks )
    {
        const char* p0 = (const char*)( (long)p & (long)~3 );
        if( p0 < text )  p0 = text;
        while( p > p0  &&  p[-1] == ' ' )  p--;     // Bis p ausgerichtet ist

        p -= 4;
        while( p > text  &&  *(int32*)p == blanks )  p -= 4;
    } 
    else 
    {
        while( ( (int)p & 3 )  &&  p[-1] == ' ' )  p--;     // Bis p ausgerichtet ist
    
        p -= 4;
        while( *(int32*)p == blanks )  p -= 4;
    }

    p += 4 - 1;

    while( p > text  &&  *p == ' ' )  p--;  

    return p + 1 - text;
}
*/
//-----------------------------------------------------------------length_without_trailing_char

uint length_without_trailing_char( const char* text, uint len, char c )
{
    // Der String wird Wortweise (int32) geprüft.
    // Dabei werden bis drei Bytes vor dem String gelesen, die müssen aber zugreifbar sein, weil sie im selben Speicherwort liegen.

    check_pointer( text, len, "length_without_trailing_char" );

    int32 cccc;
    
    cccc = ( (int32) c << 8 ) | c;
    cccc |= cccc << 16;
    
    const char* p  = text + len;
    const char* p0 = (const char*)( (long)p & (long)~3 );   // Auf int32-Grenze abrunden
    if( p0 < text )  p0 = text;
    while( p > p0  &&  p[-1] == c )  p--;  

    if( p > text  &&  p[-1] == c ) 
    {
        if( p == p0 ) 
        {
            // p ist ausgerichtet, p >= text
            const char* text0 = (const char*)( (long)text & (long)~3 );  // Auf int32-Grenze abrunden
            if( *(int32*)text0 == cccc )       // (p0 < text) muss geprüft werden?
            {
                p -= 4;
                while( p > text  &&  *(int32*)p == cccc )  p -= 4;
                p += 4;
            } 
            else 
            {
                p -= 4;
                if( p >= text0 )  while( *(int32*)p == cccc )  p -= 4;
                p += 4;
            }
        }

        while( p > text  &&  p[-1] == c )  p--;  
    }

    return p - text;
}

//---------------------------------------------------------------length_without_trailing_spaces
/* Konventionelle Version

uint length_without_trailing_spaces( const char* text, uint len )
{
    const char* p = text + len;

    if( text[0] == ' ' )  {
        while( p > text  &&  p[-1] == ' ' )  p--;    // Blanks abschneiden
    } else {
        p--;
        while( *p == ' ' )  p--;                     // Blanks abschneiden
        p++;
    }

    return p - text;
}
*/
//---------------------------------------------------------------------------------------_ltrim

void _ltrim( char* string )
{
    check_string0_pointer( string, "ltrim" );

    if( string[0] == ' ' ) {
        char* p = string;
        while( *p == ' ' )  p++;
        memmove( string, p,  strlen( p ) + 1 );
    }
}

//----------------------------------------------------------------------------------------rtrim

void rtrim( char* string )
{
    check_string0_pointer( string, "rtrim" );

    char* q = string + strlen( string );
    while( q > string  &&  q[-1] == ' ' )  q--;
    *q = '\0';
}

//-----------------------------------------------------------------------------------------trim

void trim( char* string )
{
    check_string0_pointer( string, "trim" );

    char* q = string + strlen( string );
    while( q > string  &&  q[-1] == ' ' )  q--;
    *q = '\0';

    char* p = string;
    if( *p == ' ' ) {
        while( *p == ' ' )  p++;
        memmove( string, p, q - p + 1 );
    }
}


} //namespace sos
