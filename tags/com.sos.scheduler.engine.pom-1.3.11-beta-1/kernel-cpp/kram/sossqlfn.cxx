// $Id$

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"


//#define MODULE_NAME "sossqlfn"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosfield.h"
#include "../kram/sosdate.h"
#include "../kram/sosarray.h"
#include "../kram/sosfunc.h"
#include "../kram/sosctype.h"
#include "../kram/sossqlfn.h"

using namespace std;
namespace sos {

void format( Area* buffer, double a, const char* form, char decimal_symbol );  // area.h

//-------------------------------------------------------------------------------------sql_char
// char( int )  liefert das ASCII-Zeichen für int (0..255)

void SOS_FUNC sql_char( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Dynamic_area buffer;
    char         res;

    const Dyn_obj& par1 = params[ 1 ];

    if( par1.null() )  { *result = null_dyn_obj; return; }
    int c = as_int( par1 );
    if( (uint)c > 255 )  throw_overflow_error( "SOS-1107", "unsigned int1" );
    res = (char)c;

    result->assign( &res, 1 );
}

//-------------------------------------------------------------------------------------sql_char
// current_date()  liefert das aktuelle Datum

void SOS_FUNC sql_current_date( Dyn_obj* result, const Sos_dyn_obj_array& )
{
    Sos_date today = Sos_date::today();

    result->assign( &sos_optional_date_type, &today );
}

//-----------------------------------------------------------------------------------sql_format
// format( zahl, format [, komma] ) formatiert zahl zu einem String
// 0: Ziffer (auch nicht relevante)
// 9: relevante Ziffer

void SOS_FUNC sql_format( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Dynamic_area buffer;

    const Dyn_obj& par1 = params[ 1 ];

    if( par1.null() )  { *result = null_dyn_obj; return; }

    double a = as_double( par1 );

    //if( params.count() == 1 )  format( &buffer, a, "999.999.999.999.990,00", ',' );
    //else {
        Sos_string form = as_string( params[ 2 ] );
        //if( params.count() == 2 )  format( &buffer, a, c_str( form ), ',' );
        //else {
            format( &buffer, a, c_str( form ), as_char( params[ 3 ] ) );
        //}
    //}

    result->assign( buffer.char_ptr(), buffer.length() );
}

//-----------------------------------------------------------------------------------sql_ifnull
// ifnull( a, default ) = a is null? default : a
/*
void SOS_FUNC sql_ifnull( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    const Dyn_obj& par1 = params[ 1 ];

    if( par1.null() )  *result = params[ 2 ];
                 else  *result = par1;
}
*/
//------------------------------------------------------------------------------------sql_empty
// empty( a ) = a is null or rtrim( a ) = ''

void SOS_FUNC sql_empty( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    const Dyn_obj& par1 = params[ 1 ];

    if( par1.null() ) {
        *result = true;
    } else {
        Dynamic_area buffer;
        par1.write_text( &buffer );
        *result = (Bool) (    buffer.length() == 0  
                           || length_without_trailing_spaces( buffer.char_ptr(), buffer.length() ) == 0 );
    }
}

//-----------------------------------------------------------------------------------sql_length

void SOS_FUNC sql_length( Dyn_obj* result, const Sos_dyn_obj_array& params )
// length( a ) = Länge in Zeihchen der Textdarstellung von a ohne hängende Blanks

{
    Dynamic_area buffer;

    const Dyn_obj& par1 = params[ 1 ];

    if( par1.null() )  { *result = null_dyn_obj; return; }

    par1.write_text( &buffer );
    *result = (long)length_without_trailing_spaces( buffer.char_ptr(), buffer.length() );
}

//-----------------------------------------------------------------------------------sql_quoted
// quoted( a )  setzt a in Apostrophe und verdoppelte in a enthaltene Apostrophe.

void SOS_FUNC sql_quoted( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Area_stream text;
    params[ 1 ].print( &text, '\'', '\'' );
    text << flush;

    result->assign( text.area()->char_ptr(), text.area()->length() );
}

//----------------------------------------------------------------------------------sql_to_char
// to_char( datum [,format] ) formatiert ein Datum zu Text

void SOS_FUNC sql_to_char( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    const Dyn_obj& param1 = params[ 1 ];

    if( param1.null() )  {
        result->set_null();
    } else {
        Dynamic_area puffer;
        if( params.count() == 2 ) {   // Datum?
            Sos_string        pattern   = as_string( params[ 2 ] );
            Sos_optional_date date_type ( as_string( param1 ) );

            date_type.write_text( &puffer, c_str( pattern ) );
            result->assign( puffer.char_ptr(), puffer.length() );
        } else {
            params[1].write_text( &puffer );
            result->assign( puffer.char_ptr(), puffer.length() );
        }
    }
}

//----------------------------------------------------------------------------------sql_to_date
// to_date( textdatum [, format] )  interpretiert textdatum als Datum

void SOS_FUNC sql_to_date( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    const Dyn_obj& param1 = params[ 1 ];

    if( param1.null() )  {
        result->set_null();
    } else {
        Sos_string date_string = as_string( param1 );
        if( empty( date_string ) )  {
            result->set_null();
        } else {
            Sos_string pattern;
            if( params.count() == 2 )  pattern = as_string( params[ 2 ] );

            // Vornullen setzten (heikel):  jz 26.5.97
            int date_size = 0;
            const char* p = c_str( pattern );
            while(1) {
                if( *p == 'y'  ||  *p == 'm'  ||  *p == 'd' )  p++;  // yy, mm, dd zählen
                else
                if( sos_isdigit( *p ) )  ;//ok  Schwelle für Jahrhundertwechsel, zählt nicht
                else { date_size = 0; break; }  // Alles andere ohne Vornullen
                p++;
            }

            while( length( date_string ) < date_size )  date_string = "0" + date_string;

            Sos_optional_date      date;
            Sos_optional_date_type date_type ( c_str( pattern ) );

            date_type.read_text( (Byte*)&date, c_str( date_string ) );

            result->assign( &sos_optional_date_type, (const Byte*)&date );
        }
    }
}

//-----------------------------------------------------------------------------sql_to_timestamp
// to_timestamp( textdatum [, format] )  interpretiert textdatum als Datum

void SOS_FUNC sql_to_timestamp( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    const Dyn_obj& param1 = params[ 1 ];

    if( param1.null() )  {
        result->set_null();
    } else {
        Sos_string date_string = as_string( param1 );
        if( empty( date_string ) )  {
            result->set_null();
        } else {
            Sos_string pattern;
            if( params.count() == 2 )  pattern = as_string( params[ 2 ] );

            // Vornullen setzten (heikel):  jz 26.5.97
            int date_size = 0;
            const char* p = c_str( pattern );
            while(1) {
                if( *p == 'y'  ||  *p == 'm'  ||  *p == 'd' )  p++;  // yy, mm, dd zählen
                else
                if( sos_isdigit( *p ) )  ;//ok  Schwelle für Jahrhundertwechsel, zählt nicht
                else { date_size = 0; break; }  // Alles andere ohne Vornullen
                p++;
            }

            while( length( date_string ) < date_size )  date_string = "0" + date_string;

            Sos_optional_date_time      date;
            Sos_optional_date_time_type date_type ( c_str( pattern ) );

            date_type.read_text( (Byte*)&date, c_str( date_string ) );

            result->assign( &sos_optional_date_time_type, (const Byte*)&date );
        }
    }
}

//---------------------------------------------------------------------------------sql_last_day
// last_day( datum ) liefert das Datum des letzten Tags des Monats

void SOS_FUNC sql_last_day( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    const Dyn_obj&          param1 = params[ 1 ];
    Sos_optional_date_time  date_time;

    if( !param1.null() )
    {
        Sos_string  date_string;

        date_string = as_string( param1 );
        Sos_optional_date_type().read_text( (Byte*)&date_time, c_str( date_string ) );

        date_time = ultimo( date_time );
    }

    result->assign( &sos_optional_date_time_type, (const Byte*)&date_time );
}

//-----------------------------------------------------------------------------------sql_substr

void SOS_FUNC sql_substr( Dyn_obj* result, const Sos_dyn_obj_array& params )
// substr( string, anfang, laenge )
{
    Dynamic_area string;
    Dyn_obj par1 = params[ 1 ];

    par1.write_text( &string );

    int start = as_int( params[ 2 ] );
    int len;

    if( start < 1 )  start = 1;   //? throw_xc( "SOS-SQL-59", start );
    start--;
    if( start > length( string ) )  start = length( string );

    if( params.count() == 2 ) {
        len = length( string ) - start;
        if( len < 0 )  len = 0;
    } else {
        len = as_int( params[ 3 ] );
        if( len < 0 )  len = 0;
        if( start + len > length( string ) )  len = length( string ) - start;
    }

    result->assign( string.char_ptr() + start, len );
}

//-----------------------------------------------------------------------------------sql_trim_2
// intern

void SOS_FUNC sql_trim_2( Dyn_obj* result, const Sos_dyn_obj_array& params, int mode )
{
    Dynamic_area string;
    params[1].write_text( &string );

    const char* p = string.char_ptr();
    const char* q = p + string.length();

    if( mode & 2 )  while( p < q  &&  p[0]  == ' ' )  p++;                  // ltrim
    if( mode & 1 )  q = p + length_without_trailing_spaces( p, q - p );     // rtrim

    result->assign( p, q - p );
}

//-------------------------------------------------------------------------------------sql_trim
// trim(string) schneidet links und rechts blanks ab.

void SOS_FUNC sql_trim( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    sql_trim_2( result, params, 1 | 2 );
}

//------------------------------------------------------------------------------------sql_ltrim
// ltrim(string) schneidet links blanks ab.

void SOS_FUNC sql_ltrim( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    sql_trim_2( result, params, 2 );
}

//------------------------------------------------------------------------------------sql_rtrim
// rtrim(string) schneidet rechts blanks ab.

void SOS_FUNC sql_rtrim( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    sql_trim_2( result, params, 1 );
}

//-----------------------------------------------------------------------------------sql_upper
// upper(string) setzt kleine Buchstaben in große um (auch Umlaute etc.)

void SOS_FUNC sql_upper( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
/*
    result->assign_as_string0( params[ 1 ] );
    params[ 1 ]->write..;
    xlat( result->_ptr, result->_ptr, tabucase, len );

    Dyn_obj hat kein Dynamic_area!
*/
    Dynamic_area string;

    params[ 1 ].write_text( &string );

    string.upper_case();

    result->assign( string.char_ptr(), string.length() );
}

//-----------------------------------------------------------------------------------sql_lower
// upper(string) setzt große Buchstaben in kleine um (auch Umlaute etc.)

void SOS_FUNC sql_lower( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Dynamic_area string;

    params[ 1 ].write_text( &string );

    string.lower_case();

    result->assign( string.char_ptr(), string.length() );
}

//--------------------------------------------------------------------------------sql_to_number
// to_number(string) liefert double

void SOS_FUNC sql_to_number( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    *result = as_double( params[ 1 ] );
}

//------------------------------------------------------------------------------------sql_floor

void SOS_FUNC sql_floor( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    *result = floor(as_double( params[ 1 ] ) );
}

//--------------------------------------------------------------------------------sql_translate
// translate( string, altezeichen, neuezeichen )
// translate( "4711", "1234567890", "abcdefghij" ) = "dgaa"

void SOS_FUNC sql_translate( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
/*
    result->assign_as_string0( params[ 1 ] );
    params[ 1 ]->write..;
    xlat( result->_ptr, result->_ptr, tabucase, len );

    Dyn_obj hat kein Dynamic_area!
*/
    Dynamic_area string;
    Dynamic_area alt;
    Dynamic_area neu;

    if( params[1].null() )  { *result = null_dyn_obj; return; }

    params[ 1 ].write_text( &string );
    params[ 2 ].write_text( &alt );
    params[ 3 ].write_text( &neu );

    if( length( alt ) != length( neu ) ) {
        alt += '\0';
        neu += '\0';
        throw_xc( "SOS-SQL-84", alt.char_ptr(), neu.char_ptr() );
    }

    char* p     = string.char_ptr();
    char* p_end = p + string.length();

    while( p < p_end ) {
        const char* q = (const char*)memchr( alt.ptr(), *p, alt.length() );
        if( q )  *p = *neu.char_ptr( q - alt.char_ptr() );
        p++;
    }

    result->assign( string.char_ptr(), string.length() );
}

//------------------------------------------------------------------------------------sql_instr
// instr( string, such [,anfang ] ) liefert Position oder 0
// Kopie von sql_locate() mit vertauschten Parametern

void SOS_FUNC sql_instr( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Sos_string   str;
    int          start = 1;
    int          res   = 0;   // result

    if( params[1].null()
     || params[2].null() )  { *result = null_dyn_obj; return; }

    str = as_string( params[1] );

    if( params.last_index() >= 3 )  start = as_int( params[3] );

    start--;
    if( start < length( str ) )  {
        if( start < 0 )  start = 0;
        Sos_string such = as_string( params[2] );
        const char* p = strstr( c_str( str ) + start, c_str( such ) );
        if( p )  res = p - c_str( str ) + 1;
    }

    *result = res;
}

//-----------------------------------------------------------------------------------sql_locate
// locate( such, string [,anfang ] ) liefert Position oder 0
// sql_instr() ist Kopie mit vertauschten Parametern. SYNCHRON ÄNDERN!

void SOS_FUNC sql_locate( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Sos_string   str;
    int          start = 1;
    int          res   = 0;   // result

    if( params[1].null()
     || params[2].null() )  { *result = null_dyn_obj; return; }

    str = as_string( params[2] );

    if( params.last_index() >= 3 )  start = as_int( params[3] );

    start--;
    if( start < length( str ) )  {
        if( start < 0 )  start = 0;
        Sos_string such = as_string( params[1] );
        const char* p = strstr( c_str( str ) + start, c_str( such ) );
        if( p )  res = p - c_str( str ) + 1;
    }

    *result = res;
}

//----------------------------------------------------------------------------------sql_replace
// replace( string, alt, neu ) ersetzt jedes alt in neu.

void SOS_FUNC sql_replace( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Sos_string   str;
    int          start = 1;
    Dynamic_area buffer;  // result
    Sos_string   such;
    Sos_string   ersatz;

    if( params[1].null()
     || params[2].null()
     || params[3].null() )  { *result = null_dyn_obj; return; }

    str = as_string( params[1] );

    //if( params.last_index() >= 3 )  start = as_int( params[3] );

    start--;
    if( start < 0 )  start = 0;

    such = as_string( params[2] );
    if( length( such ) == 0 )  { *result = null_dyn_obj; return; }

    ersatz = as_string( params[3] );

    const char* p0     = c_str( str ) + start;
    const char* p_end  = c_str( str ) + length( str );

    buffer.allocate_min( 1024 );

    while( p0 < p_end ) {
        const char* p = strstr( p0, c_str( such ) );
        if( !p )  p = p_end;
        buffer.append( p0, p - p0 );
        if( p == p_end ) break;
        buffer.append( c_str( ersatz ), length( ersatz ) );
        p0 = p + length( such );
    }

    result->assign( buffer.char_ptr(), buffer.length() );
}

//------------------------------------------------------------------------------------sql_error
// error(string) Meldet Fehler
void SOS_FUNC sql_error( Dyn_obj*, const Sos_dyn_obj_array& params )
{
    Sos_string text = as_string( params[ 1 ] );
    throw_xc( "SOS-SQL-1000", c_str( text ) );
}

//-----------------------------------------------------------------------------------sql_printf
// printf( format, parameter, ... )
// format: ...% [+- ] [width] [.prec] [l] [dfeg]

void SOS_FUNC sql_printf( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Sos_string   format0;
    double       par [ 20 ];
    int          i, j;
    Dynamic_area format ( 1024 );
    Dynamic_area buffer ( 1024 );

    if( params[1].null() )  { *result = null_dyn_obj; return; }

    format0 = as_string( params[1] );

    const char* p = c_str( format0 );

    i = 0;
    j = 2;

    while( *p )
    {
        if( *p == '%' ) {
            format += *p++;         // [+- #]* [width] [.prec] [FNhlL] typechar
            Bool long_modifier = false;
            while( *p == '+'  ||  *p == '-'  || *p == ' ' )  format += *p++;
            while( sos_isdigit( *p ) )  format += *p++;
            if( *p == '.' ) {
                format += *p++;
                while( sos_isdigit( *p ) )  format += *p++;
            }
            if( *p == 'l' ) { format += *p++; long_modifier = true; }

            switch( *p )
            {
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X': if( !long_modifier )  format += 'l';
                      format += *p++;
                      *(long*)( &par[ i++ ] ) = as_long( params[ j++ ] );
                      break;
          //case 'u': if( !long_modifier )  format += 'l';
          //          format += *p++;
          //          *(ulong*)( &par[ i++ ] ) = as_ulong( params[ j ] );
          //          break;
            case 'f':
            case 'e':
            case 'E':
            case 'g': if( !long_modifier )  format += 'l';
                      format += *p++;
                      par[ i++ ] = as_double( params[ j++ ] );
                      break;
            default: throw_xc( "PRINTF-FORMAT" );
            }
        }
        else format += *p++;
    }

    format += '\0';

    sprintf( buffer.char_ptr(), format.char_ptr(),
             par[0], par[1], par[2], par[3], par[4], par[5], par[6], par[7], par[8], par[9],
             par[10], par[11], par[12], par[13], par[14], par[15], par[16], par[17], par[18], par[19] );

    *result = buffer.char_ptr();
}

//----------------------------------------------------------------------------------sql_truncate
// truncate( zahl, scale )

void SOS_FUNC sql_truncate( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    Sos_string   str;
    Dynamic_area buffer;  // result
    Sos_string   such;
    Sos_string   ersatz;

    if( params[ 1 ].null()  ||  params[2].null() ) {
        *result = null_dyn_obj;
        return;
    } 

    double value = as_double( params[1] );
    int    scale = as_int( params[2] );

    if( scale == 0 ) {
        value = floor( value );
        if( value >= -LONG_MAX  &&  value <= LONG_MAX )  *result = (long)( value + 0.1 );
                                                   else  *result = value;
    } else {
        double p = pow( 10.0, scale );
        *result = floor( value * p ) / p;
    }

    if( params[1].null()
     || params[2].null()
     || params[3].null() )  { *result = null_dyn_obj; return; }
}


//----------------------------------------------------------------------------------sql_str_to_hex

void SOS_FUNC sql_str_to_hex( Dyn_obj* result, const Sos_dyn_obj_array& params )
// str_to_hex( string )
{
    Dynamic_area string;
    Dynamic_area buf;

    params[ 1 ].write_text( &string );
    buf.allocate_min( length(string)*2 );

    ostrstream s( buf.char_ptr(), buf.size() );

    s << hex << string;
    buf.length( s.pcount() );

    //LOG( "sql_str_to_hex: '" << string << "' => '" << buf << "'\n" );
    result->assign( buf.char_ptr(), length(buf) );
}

//----------------------------------------------------------------------------------sql_hex_to_str

inline int1 get_hex( char hex_char )
{
    int1 n;

    if ( hex_char >= '0' && hex_char <= '9' ) {
        n = hex_char - '0';
    } else if ( hex_char >= 'A' && hex_char <= 'F' ) {
        n = hex_char - 'A' + 10;
    } else if ( hex_char >= 'a' && hex_char <= 'f' ) {
        n = hex_char - 'a' + 10;
    } else {
        char str [ 2 ];
        str[0] = hex_char;
        str[1] = '\0';
        throw_xc( "SOS-1392", str );
    }

    return n;
}

void SOS_FUNC sql_hex_to_str( Dyn_obj* result, const Sos_dyn_obj_array& params )
// hex_to_str( string )
{
    Dynamic_area string;
    Dynamic_area buf;

    params[ 1 ].write_text( &string );
    buf.allocate_min( length(string)/2+1 );
    //LOG( "sql_hex_to_str: '" << string << "' => " );

          char* p       = string.char_ptr();
    const char* p_end   = p + length(string);

    while ( p+1 < p_end ) {
        char c;
        int1 n;
        int1 n2;

        try {
            n  = get_hex((char)*p);
            p++;
            n2 = get_hex((char)*(p));
        } catch ( Xc& x ) {
            x.insert( (int1)(p-(char*)string.char_ptr()) );
            throw;
        }

        c = (char) ((int1) ( 16*n + n2 ) );
        buf.append(c);
        p += 1;
    }

    //LOG( "'" << buf << "'\n" );
    result->assign( buf.char_ptr(), length(buf) );
}


//-------------------------------------------------------------------sos_sql_register_functions

void sos_sql_register_functions()
{
    Sos_function_register* function_register = Sos_function_register::init();

    if( !function_register->function_or_0( "to_char", 2 ) )
    {
        function_register->add( &sql_char     , "char"     , 1 );
        function_register->add( &sql_current_date, "current_date", 0 );
        function_register->add( &sql_empty    , "empty"    , 1 );
        function_register->add( &sql_error    , "error"    , 1 );
      //function_register->add( &sql_format   , "format"   , 2 );
        function_register->add( &sql_format   , "format"   , 3 );
      //function_register->add( &sql_ifnull   , "ifnull"   , 2 );
        function_register->add( &sql_instr    , "instr"    , 2 );
        function_register->add( &sql_instr    , "instr"    , 3 );
        function_register->add( &sql_floor    , "floor"    , 1 );
        function_register->add( &sql_ltrim    , "format"   , 2 );
        function_register->add( &sql_last_day , "last_day" , 1 );
        function_register->add( &sql_length   , "length"   , 1 );
        function_register->add( &sql_instr    , "locate"   , 2 );
        function_register->add( &sql_instr    , "locate"   , 3 );
        function_register->add( &sql_lower    , "lower"    , 1 );
        function_register->add( &sql_lower    , "lcase"    , 1 );
        function_register->add( &sql_ltrim    , "ltrim"    , 1 );
        function_register->add( &sql_printf   , "printf"   , 2 );
        function_register->add( &sql_printf   , "printf"   , 3 );
        function_register->add( &sql_printf   , "printf"   , 4 );
        function_register->add( &sql_printf   , "printf"   , 5 );
        function_register->add( &sql_printf   , "printf"   , 6 );
        function_register->add( &sql_printf   , "printf"   , 7 );
        function_register->add( &sql_printf   , "printf"   , 8 );
        function_register->add( &sql_printf   , "printf"   , 9 );
        function_register->add( &sql_printf   , "printf"   , 10 );
        function_register->add( &sql_quoted   , "quoted"   , 1 );
        function_register->add( &sql_rtrim    , "rtrim"    , 1 );
        function_register->add( &sql_replace  , "replace"  , 3 );
        function_register->add( &sql_substr   , "substr"   , 2 );
        function_register->add( &sql_substr   , "substr"   , 3 );
        function_register->add( &sql_to_char  , "to_char"  , 1 );
        function_register->add( &sql_to_char  , "to_char"  , 2 );
        function_register->add( &sql_to_date  , "to_date"  , 1 );
        function_register->add( &sql_to_date  , "to_date"  , 2 );
        function_register->add( &sql_to_timestamp, "to_timestamp"  , 1 );
        function_register->add( &sql_to_timestamp, "to_timestamp"  , 2 );
        function_register->add( &sql_to_number, "to_number", 1 );
        function_register->add( &sql_translate, "translate", 3 );
        function_register->add( &sql_trim     , "trim"     , 1 );
        function_register->add( &sql_truncate , "truncate" , 2 );
        function_register->add( &sql_upper    , "upper"    , 1 );
        function_register->add( &sql_upper    , "ucase"    , 1 );
        function_register->add( NULL          , "warning"  , 2 );    // Dummy
        function_register->add( &sql_str_to_hex, "str_to_hex", 1 );
        function_register->add( &sql_hex_to_str, "hex_to_str", 1 );
    }
}



/*

Die mit "*" markierten Funktionen sind nicht implementiert.
  
Aus ODBC 3.0: odbc.hlp


String-Funktionen

Function	Description

*ASCII(string_exp)
(ODBC 1.0) 	Returns the ASCII code value of the leftmost character of string_exp as an integer.

*BIT_LENGTH(string_exp)
(ODBC 3.0)	Returns the length in bits of the string expression.

CHAR(code)
(ODBC 1.0)
	Returns the character that has the ASCII code value specified by code. The value of code should be between 0 and 255; otherwise, the return value is data source–dependent.

*CHAR_LENGTH(string_exp)
(ODBC 3.0)	Returns the length in characters of the string expression, if the string expression is of a character data type; otherwise, returns the length in bytes of the string expression (the smallest integer not less than the number of bits divided by 8). (This function is the same as the CHARACTER_LENGTH function.)

*CHARACTER_LENGTH
(string_exp)
(ODBC 3.0)	Returns the length in characters of the string expression, if the string expression is of a character data type; otherwise, returns the length in bytes of the string expression (the smallest integer not less than the number of bits divided by 8). (This function is the same as the CHAR_LENGTH function.)

*CONCAT(string_exp1, string_exp2)
(ODBC 1.0)
	Returns a character string that is the result of concatenating string_exp2 to string_exp1. The resulting string is DBMS-dependent. For example, if the column represented by string_exp1 contained a NULL value, DB2 would return NULL, but SQL Server would return the non-NULL string.

*DIFFERENCE(string_exp1, string_exp2)
(ODBC 2.0)
	Returns an integer value that indicates the difference between the values returned by the SOUNDEX function for string_exp1 and string_exp2.

*INSERT(string_exp1, start,length, string_exp2)
(ODBC 1.0)
	Returns a character string where length characters have been deleted from string_exp1 beginning at start and where string_exp2 has been inserted into string_exp, beginning at start.

*LCASE(string_exp)
(ODBC 1.0)	Returns a string equal to that in string_exp with all uppercase characters converted to lowercase.

*LEFT(string_exp, count)
(ODBC 1.0)	Returns the leftmost count characters of string_exp.

LENGTH(string_exp)
(ODBC 1.0)	Returns the number of characters in string_exp, excluding trailing blanks.

LOCATE(string_exp1, string_exp2[, start])
(ODBC 1.0)	Returns the starting position of the first occurrence of string_exp1 within string_exp2. The search for the first occurrence of string_exp1 begins with the first character position in string_exp2 unless the optional argument, start, is specified. If start is specified, the search begins with the character position indicated by the value of start. The first character position in string_exp2 is indicated by the value 1. If string_exp1 is not found within string_exp2, the value 0 is returned.If an application can call the LOCATE scalar function with the string_exp1, string_exp2, and start arguments, then the driver returns SQL_FN_STR_LOCATE when SQLGetInfo is called with an Option of SQL_STRING_FUNCTIONS. If the application can call the LOCATE scalar function with only the string_exp1 and string_exp2 arguments, then the driver returns SQL_FN_STR_LOCATE_2 when SQLGetInfo is called with an Option of SQL_STRING_FUNCTIONS. Drivers that support calling the LOCATE function with either two or three arguments

LTRIM(string_exp)
(ODBC 1.0)	Returns the characters of string_exp, with leading blanks removed.

*OCTET_LENGTH(string_exp)
(ODBC 3.0)	Returns the length in bytes of the string expression. The result is the smallest integer not less than the number of bits divided by 8.

*POSITION(character_exp IN character_exp)
(ODBC 3.0)	Returns the position of the first character expression in the second character expression. The result is an exact numeric with an implementation-defined precision and a scale of 0.

*REPEAT(string_exp, count) 
(ODBC 1.0)	Returns a character string composed of string_exp repeated count times.

REPLACE(string_exp1, string_exp2, string_exp3) 
(ODBC 1.0)	Search string_exp1 for occurrences of string_exp2 and replace with string_exp3.

*RIGHT(string_exp, count) 
(ODBC 1.0)	Returns the rightmost count characters of string_exp.

RTRIM(string_exp) 
(ODBC 1.0)	Returns the characters of string_exp with trailing blanks removed.

*SOUNDEX(string_exp) 
(ODBC 2.0)	Returns a data source–dependent character string representing the sound of the words in string_exp. For example, SQL Server returns a 4-digit SOUNDEX code; Oracle returns a phonetic representation of each word.

*SPACE(count) 
(ODBC 2.0)	Returns a character string consisting of count spaces.

*SUBSTRING(string_exp, start, length) 
(ODBC 1.0)	Returns a character string that is derived from string_exp beginning at the character position specified by start for length characters.

*UCASE(string_exp) 
(ODBC 1.0)	Returns a string equal to that in string_exp with all lowercase characters converted to uppercase.


Numerische Funktionen:

*ABS(numeric_exp) 
(ODBC 1.0)	Returns the absolute value of numeric_exp.

*ACOS(float_exp) 
(ODBC 1.0)	Returns the arccosine of float_exp as an angle, expressed in radians.

*ASIN(float_exp) 
(ODBC 1.0)	Returns the arcsine of float_exp as an angle, expressed in radians.

*ATAN(float_exp) 
(ODBC 1.0)	Returns the arctangent of float_exp as an angle, expressed in radians.

*ATAN2(float_exp1, float_exp2) 
(ODBC 2.0)	Returns the arctangent of the x and y coordinates, specified by float_exp1 and float_exp2, respectively, as an angle, expressed in radians.

*CEILING(numeric_exp) 
(ODBC 1.0)	Returns the smallest integer greater than or equal to numeric_exp.

*COS(float_exp) 
(ODBC 1.0)	Returns the cosine of float_exp, where float_exp is an angle expressed in radians.

*COT(float_exp) 
(ODBC 1.0)	Returns the cotangent of float_exp, where float_exp is an angle expressed in radians.

*DEGREES(numeric_exp) 
(ODBC 2.0)	Returns the number of degrees converted from numeric_exp radians.

*EXP(float_exp) 
(ODBC 1.0)	Returns the exponential value of float_exp.

*FLOOR(numeric_exp) 
(ODBC 1.0)	Returns the largest integer less than or equal to numeric_exp.

*LOG(float_exp) 
(ODBC 1.0)	Returns the natural logarithm of float_exp.

*LOG10(float_exp) 
(ODBC 2.0)	Returns the base 10 logarithm of float_exp.

*MOD(integer_exp1, integer_exp2) 
(ODBC 1.0)	Returns the remainder (modulus) of integer_exp1 divided by integer_exp2.

*PI( )
(ODBC 1.0)	Returns the constant value of pi as a floating point value.

*POWER(numeric_exp, integer_exp) 
(ODBC 2.0)	Returns the value of numeric_exp to the power of integer_exp.

*RADIANS(numeric_exp) 
(ODBC 2.0)	Returns the number of radians converted from numeric_exp degrees.

*RAND([integer_exp]) 
(ODBC 1.0)	Returns a random floating point value using integer_exp as the optional seed value.

*ROUND(numeric_exp, integer_exp) 
(ODBC 2.0)	Returns numeric_exp rounded to integer_exp places right of the decimal point. If integer_exp is negative, numeric_exp is rounded to |integer_exp| places to the left of the decimal point.

*SIGN(numeric_exp) 
(ODBC 1.0)	Returns an indicator of the sign of numeric_exp. If numeric_exp is less than zero, –1 is returned. If numeric_exp equals zero, 0 is returned. If numeric_exp is greater than zero, 1 is returned.

*SIN(float_exp) 
(ODBC 1.0)	Returns the sine of float_exp, where float_exp is an angle expressed in radians.

*SQRT(float_exp) 
(ODBC 1.0)	Returns the square root of float_exp.

*TAN(float_exp) 
(ODBC 1.0)	Returns the tangent of float_exp, where float_exp is an angle expressed in radians.

*TRUNCATE(numeric_
exp, integer_exp) 
(ODBC 2.0)	Returns numeric_exp truncated to integer_exp places right of the decimal point. If integer_exp is negative, numeric_exp is truncated to |integer_exp| places to the left of the decimal point.


Zeit-, Datum- und Intervallfunktionen:

*CURRENT_DATE( ) 
(ODBC 3.0)	Returns the current date.

*CURRENT_TIME[(time-
precision)] 
(ODBC 3.0)	Returns the current local time. The time-precision argument determines the seconds precision of the returned value.

*CURRENT_TIMESTAMP
[(timestamp-precision)]
(ODBC 3.0)	Returns the current local date and local time as a timestamp value. The timestamp-precision argument determines the seconds precision of the returned timestamp.

*CURDATE( ) 
(ODBC 1.0)	Returns the current date.

*CURTIME( ) 
(ODBC 1.0)	Returns the current local time.

*DAYNAME(date_exp) 
(ODBC 2.0)	Returns a character string containing the data source–specific name of the day (for example, Sunday, through Saturday or Sun. through Sat. for a data source that uses English, or Sonntag through Samstag for a data source that uses German) for the day portion of date_exp.

*DAYOFMONTH(date_exp) 
(ODBC 1.0)	Returns the day of the month based on the month field in date_exp as an integer value in the range of 1–31.

*DAYOFWEEK(date_exp) 
(ODBC 1.0)	Returns the day of the week based on the week field in date_exp as an integer value in the range of 1–7, where 1 represents Sunday.

*DAYOFYEAR(date_exp) 
(ODBC 1.0)	Returns the day of the year based on the year field in date_exp as an integer value in the range of 1–366.

*EXTRACT(extract-field FROM extract-source)
(ODBC 3.0)	Returns the extract-field portion of the extract-source. The extract-source argument is a datetime or interval expression. The extract-field argument can be one of the following keywords:YEAR

*MONTH

*DAY

*HOUR

*MINUTE

*SECONDThe precision of the returned value is implementation-defined. The scale is 0 unless SECOND is specified, in which case the scale is not less than the fractional seconds precision of the extract-source field.

*HOUR(time_exp) 
(ODBC 1.0)	Returns the hour based on the hour field in time_exp as an integer value in the range of 0 –23.

*MINUTE(time_exp) 
(ODBC 1.0)	Returns the minute based on the minute field in time_exp as an integer value in the range of 0 –59.

*MONTH(date_exp) 
(ODBC 1.0)	Returns the month based on the month field in date_exp as an integer value in the range of 1–12.

*MONTHNAME(date_exp) 
(ODBC 2.0)	Returns a character string containing the data source–specific name of the month (for example, January through December or Jan. through Dec. for a data source that uses English, or Januar through Dezember for a data source that uses German) for the month portion of date_exp.

*NOW( ) 
(ODBC 1.0)	Returns current date and time as a timestamp value.

*QUARTER(date_exp)
(ODBC 1.0)	Returns the quarter in date_exp as an integer value in the range of 1– 4, where 1 represents January 1 through March 31.

*SECOND(time_exp) 
(ODBC 1.0)	Returns the second based on the second field in time_exp as an integer value in the range of 0 –59.

*TIMESTAMPADD(interval, integer_exp, timestamp_exp) 
(ODBC 2.0)	Returns the timestamp calculated by adding integer_exp intervals of type interval to timestamp_exp. Valid values of interval are the following keywords:SQL_TSI_FRAC_SECOND

*SQL_TSI_SECOND

*SQL_TSI_MINUTE

*SQL_TSI_HOUR

*SQL_TSI_DAY

*SQL_TSI_WEEK

*SQL_TSI_MONTH

*SQL_TSI_QUARTER

*SQL_TSI_YEARwhere fractional seconds are expressed in billionths of a second. For example, the following SQL statement returns the name of each employee and his or her one-year anniversary date:SELECT NAME, {fn 
TIMESTAMPADD(SQL_TSI_YEAR,
1, HIRE_DATE)} FROM
EMPLOYEESIf timestamp_exp is a time value and interval specifies days, weeks, months, quarters, or years, the date portion of timestamp_exp is set to the current date before calculating the resulting timestamp.If timestamp_exp is a date value and interval specifies fractional seconds, seconds, minutes, or hours, the time portion of timestamp_exp is set to 0 before calculating the resulting timestamp.An application determines which intervals a data source supports by calling SQLGetInfo with the SQL_TIMEDATE_ADD_INTERVALS option.
TIMESTAMPDIFF(interval, timestamp_exp1, timestamp_exp2) 
(ODBC 2.0)	Returns the integer number of intervals of type interval by which timestamp_exp2 is greater than timestamp_exp1. Valid values of interval are the following keywords:SQL_TSI_FRAC_SECOND

*SQL_TSI_SECOND

*SQL_TSI_MINUTE

*SQL_TSI_HOUR

*SQL_TSI_DAY

*SQL_TSI_WEEK

*SQL_TSI_MONTH

*SQL_TSI_QUARTER

*SQL_TSI_YEARwhere fractional seconds are expressed in billionths of a second. For example, the following SQL statement returns the name of each employee and the number of years he or she has been employed.SELECT NAME, {fn 
TIMESTAMPDIFF(SQL_TSI_YEAR,
{fn CURDATE()}, HIRE_DATE)}
FROM EMPLOYEESIf either timestamp expression is a time value and interval specifies days, weeks, months, quarters, or years, the date portion of that timestamp is set to the current date before calculating the difference between the timestamps.If either timestamp expression is a date value and interval specifies fractional seconds, seconds, minutes, or hours, the time portion of that timestamp is set to 0 before calculating the difference between the timestamps.An application determines which intervals a data source supports by calling SQLGetInfo with the SQL_TIMEDATE_DIFF_INTERVALS option.

*WEEK(date_exp) 
(ODBC 1.0)	Returns the week of the year based on the week field in date_exp as an integer value in the range of 1–53.

*YEAR(date_exp) 
(ODBC 1.0)	Returns the year based on the year field in date_exp as an integer value. The range is data source–dependent.


System-Funktionen:

*DATABASE( ) 
(ODBC 1.0)	Returns the name of the database corresponding to the connection handle. (The name of the database is also available by calling SQLGetConnectOption with the SQL_CURRENT_QUALIFIER connection option.)

IFNULL(exp, value) 
(ODBC 1.0)	If exp is null, value is returned. If exp is not null, exp is returned. The possible data type or types of value must be compatible with the data type of exp.

*USER( ) 
(ODBC 1.0)	Returns the user name in the DBMS. (The user name is also available by way of SQLGetInfo by specifying the information type: SQL_USER_NAME.) This may be different than the login name.
*/

} //namespace sos
