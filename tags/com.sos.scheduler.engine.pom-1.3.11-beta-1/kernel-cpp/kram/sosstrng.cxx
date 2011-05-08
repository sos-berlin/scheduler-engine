#include "precomp.h"
//#define MODULE_NAME "sosstrng"
// sosstrng.cxx 		(c) Jörg Schwiemann

#include "../kram/sysdep.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if defined SYSTEM_WIN && defined SYSTEM_STARVIEW
#   include <svwin.h>
#endif

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

//----------------------------------------------------------------------------------------rtrim
/*
void rtrim( Sos_string* str_ptr )
{
    int         len = length(*str_ptr);
    const char* p   = c_str(*str_ptr)+len;
    int         l   = len;

    while (*p && isspace(*p)) { p--; l--; }

    if ( l < len ) {
        *str_ptr = as_string( c_str(*str_ptr), l );
    }
}
*/
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

/*
Sos_string as_string( const Const_area& area )
{
    if( area.length() == 0 ) {
        return Sos_string( "" );
    }
#if defined( SYSTEM_BORLAND )
	return( string( area.char_ptr(), 0, area.length() ) );
#endif
#if defined( SYSTEM_SOLARIS )
	return( RWCString( area.char_ptr(), area.length() ) );
#endif
#if defined( __GNU_C__ )
	// Gnu String?
#endif
}
*/


#if 0
void convert_from_string( const Sos_string& str, Area* area_ptr )
{
	if ( area_ptr->size() < str.length() ) raise( "D???", "???" );
	memcpy( area_ptr->char_ptr(), c_str(str), str.length() );
	area_ptr->length( str.length() );

  exceptions
}


#endif

/*
size_t find( const Sos_string& str, const Sos_string& to_find, size_t pos )
{
#if defined( SYSTEM_SOLARIS )
    RWCRegexp regex(c_str(to_find));
	size_t erg = str.index( regex, NULL, pos );
    return ( erg == RW_NPOS ? NO_POS : erg );
#endif
#if defined( SYSTEM_WIN )
	return str.find( to_find, pos );
#endif
#if defined( __GNU_C__ )
	// Gnu String?
#endif
}
*/
/*
Sos_string sub_string( const Sos_string str, size_t pos, size_t length )
{
    size_t real_length;
    if ( length == (size_t)STRING_LENGTH )
    {
        real_length = strlen( c_str( str ) );
    } else real_length = length;
#if defined( SYSTEM_BORLAND )
	return( string( c_str( str ), pos, real_length ) );
#endif
#if defined( SYSTEM_SOLARIS )
	return( RWCString( c_str( str ) + pos , real_length ) );
#endif
#if defined( __GNU_C__ )
	// Gnu String?
#endif
}
*/

#if defined OWN_SOSSTRING

char Sos_string::_null_string [ 1 ] = "";

/*
void Sos_string::size( int s )
{
    delete [] _ptr;
    _ptr = new char [ s ];        assert( _ptr );
    _size = s;
}
*/
#endif

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

/* Microsoft: Operator << ist mehrdeutig
    char buffer [ 40 ];
    ostrstream s ( buffer, sizeof buffer );
    s << o;
    return as_string( buffer, s.pcount() );
*/
    char buffer [ 50 ];

    int len = sprintf( buffer, "%" PRINTF_LONG_LONG "d", o );
    return as_string( buffer, len );
}

//------------------------------------------------------------------------------------as_string

Sos_string as_string( Ubig_int o )
{
    ZERO_RETURN_VALUE( Sos_string );

/* Microsoft: Operator << ist mehrdeutig
    char buffer [ 40 ];
    ostrstream s ( buffer, sizeof buffer );
    s << o;
    return as_string( buffer, s.pcount() );
*/
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

// ---------------------------------------------------------------------------- expanded_string

Sos_string expanded_string( const Area& area,  char c )
{
    ZERO_RETURN_VALUE( Sos_string );

  	return expanded_string( as_string( area ), area.size(), c );
}

//------------------------------------------------------------------------------expanded_string

Sos_string expanded_string( const Const_area& area, int len,  char c )
{
    ZERO_RETURN_VALUE( Sos_string );

  	return expanded_string( as_string( area ), len, c );
}

//---------------------------------------------------------------------------------------append

void append( Sos_string* string_ptr, const char* ptr, int length )
{
    *string_ptr += as_string( ptr, length );
}

//-----------------------------------------------------------------------------------sub_string

Sos_string sub_string( const Sos_string& str, int pos, int len )
{
    ZERO_RETURN_VALUE( Sos_string );

  	if( (uint)pos > length( str ) )  pos = length( str );

  	return as_string( c_str( str ) + pos, ( len == -1? length( str ) : len ) - pos );
}

//--------------------------------------------------------------------------------quoted_string

Sos_string quoted_string( const char* text, char quote1, char quote2, int len )
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


// ---------------------------------------------------------------------------- expanded_string
#if !defined SOS_STRING_STARVIEW

Sos_string expanded_string( const Sos_string& string, int size, char c )
{
    ZERO_RETURN_VALUE( Sos_string );

    int         rest          = size - length( string );
    if( rest <= 0 )  return string;

    const int   max           = 200;
    uint        r             = min( rest, max );
    Sos_string  s             ( string );
    char        einige_blanks [ max+1 ];

    memset( einige_blanks, ' ', r );
    einige_blanks[ r ] = '\0';

    while( r == max ) {
        s += einige_blanks;
        rest -= max;
        r = min( rest, max );
    }

    einige_blanks[ r ] = '\0';
    return s + einige_blanks;
}

#endif

// RWCString besitzt schon <</>>-Operatoren !
#if defined SOS_STRING_STARVIEW
//------------------------------------------------------------------------------------as_string

Sos_string as_string( const char* string, unsigned int length )
{
    ZERO_RETURN_VALUE( Sos_string );

    return String( string, length );
}

//--------------------------------------------------------------------------------------operator >>

istream& operator>> ( istream& s, Sos_string& string )
{
    string = "";

    while(1) {
        char buffer [ 256 ];

        s.read( buffer, sizeof buffer );

        int length = s.gcount();
        if( length == 0 )  break;

        append( &string, buffer, length );

        if( length < sizeof buffer )  break;
    }

    return s;
}

//--------------------------------------------------------------------------------------operator <<

ostream& operator<< ( ostream& s, const Sos_string& string )
{
    s.write( c_str( string ), length( string ) );
    return s;
}
#endif

#if defined SOS_STRING_BORLAND

string as_string( const char* str, unsigned int length )
{
    ZERO_RETURN_VALUE( string );
    return string( str, 0, length );
}

void assign( string* str, const char* o )
{
    *str = o;               // äquivalent zu: str = string( s );  temporäres Objekt und Destruktor!
}

#endif

#if defined OWN_SOSSTRING

//--------------------------------------------------------------------------Sos_string::Sos_string

Sos_string::Sos_string( const Sos_string& string )
:
    _ptr  ( _null_string ),
    _length( 0 ),
    _size ( 1 )
{
    assert( string._length < string._size );

    size( string._size );
    memcpy( _ptr, string._ptr, string._length );
    _length = string._length;
    _ptr[ _length ] = '\0';
}

//--------------------------------------------------------------------------Sos_string::Sos_string

Sos_string::Sos_string( char c )
:
    _ptr  ( _null_string ),
    _length( 0 ),
    _size ( 1 )
{
    size( 1+1 );
	_ptr[ 0 ] = c;
	_ptr[ 1 ] = '\0';
	_length = 1;
}

//--------------------------------------------------------------------------Sos_string::~Sos_string

Sos_string::~Sos_string()
{
    if( _ptr  &&  _ptr != _null_string )  sos_free( _ptr );

	_ptr = NULL;
	_size = 0;
	_length = 0;
}

//--------------------------------------------------------------------------------Sos_string::size

void Sos_string::size( int s )
{
    if( s <= _size )  return;

    char* ptr = (char*)sos_alloc( s, "Sos_string" );

    if( _ptr ) {
        memcpy( ptr, _ptr, _length + 1 );
        if( _ptr != _null_string )   sos_free( _ptr );
    }

    _ptr = ptr;
    _size = s;
}

//------------------------------------------------------------------------------Sos_string::assign

void Sos_string::assign( const char* str, int length )
{
    if( length == 0 ) 
    {
        if( _ptr  &&  _ptr != _null_string )  sos_free( _ptr );
        _ptr    = _null_string;
        _size   = 1;
        _length = 0;
    }
    else
    {
        if( length + 1 > _size )  size( length + 1 );

        memcpy( _ptr, str, length );
        _ptr[ length ] = '\0';
        _length = length;
    }
}

//------------------------------------------------------------------------------Sos_string::append

void Sos_string::append( const char* string, int len )
{
    int l = _length + len;
    if( l + 1 > _size )  size( l + 1 );

    memcpy( _ptr + _length, string, len );
    _length += len;
    _ptr[ _length ] = '\0';
}

//------------------------------------------------------------------------------Sos_string::append

void Sos_string::append( char c )
{
    if( _length + 1 + 1 > _size )  size( _length + 20 );

    _ptr[ _length++ ] = c;
    _ptr[ _length   ] = '\0';
}


#endif


} //namespace sos
