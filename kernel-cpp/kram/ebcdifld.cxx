// $Id: ebcdifld.cxx 13579 2008-06-09 08:09:33Z jz $

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

/*
    7.11.99: [SIGN IS] LEADING|TRAILING [SEPARATE CHARACTER] eingebaut.  J. Zschimmer
*/

//#include "../kram/optimize.h"
#include "precomp.h"
//#define MODULE_NAME "ebcdifld"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sosstrng.h"

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosprof.h"
#include "../kram/sosstrea.h"
#include "../kram/decimal.h"
#include "../kram/sosfield.h"
#include "../kram/stdfield.h"
#include "../kram/decimal.h"
#include "../kram/ebcdifld.h"

using namespace std;
namespace sos {

//---------------------------------------------------------------------------------------static

static Bool german_profile_read = false;
static Bool german              = false;

extern char ebc2iso_0_is_blank [256];
extern char ebc2iso_german_0_is_blank [256];
extern char mvs2iso_0_is_blank [256];
extern char mvs2iso_german_0_is_blank [256];
extern Byte iso2mvs [256];
extern Byte iso2mvs_german [256];

Listed_type_info Ebcdic_number_type::_type_info;
Listed_type_info Ebcdic_packed_type::_type_info;
Listed_type_info Ebcdic_text_type::_type_info;

SOS_INIT( ebcdic )
{
    Ebcdic_number_type::_type_info._std_type      = std_type_decimal;
    Ebcdic_number_type::_type_info._name          = "Ebcdic_signed_number";
    Ebcdic_number_type::_type_info._nullable      = true;
    Ebcdic_number_type::_type_info._max_size      = max_ebcdic_number_size;
    Ebcdic_number_type::_type_info._max_precision = max_ebcdic_number_size;
    Ebcdic_number_type::_type_info._radix         = 10;
    Ebcdic_number_type::_type_info._max_scale     = max_ebcdic_number_size;
    Ebcdic_number_type::_type_info.normalize();

    Ebcdic_unsigned_number_type::_type_info._std_type      = std_type_decimal;
    Ebcdic_unsigned_number_type::_type_info._name          = "Ebcdic_unsigned_number";
    Ebcdic_unsigned_number_type::_type_info._nullable      = true;
    Ebcdic_unsigned_number_type::_type_info._unsigned      = true;
    Ebcdic_unsigned_number_type::_type_info._max_size      = max_ebcdic_number_size;
    Ebcdic_unsigned_number_type::_type_info._max_precision = max_ebcdic_number_size;
    Ebcdic_unsigned_number_type::_type_info._radix         = 10;
    Ebcdic_unsigned_number_type::_type_info._max_scale     = max_ebcdic_number_size;
    Ebcdic_unsigned_number_type::_type_info._exact_char_repr = true;
    Ebcdic_unsigned_number_type::_type_info.normalize();

    Ebcdic_packed_type::_type_info._std_type      = std_type_decimal;
    Ebcdic_packed_type::_type_info._name          = "Ebcdic_packed_decimal";
    Ebcdic_packed_type::_type_info._nullable      = true;
    Ebcdic_packed_type::_type_info._max_size      = 16;
    Ebcdic_packed_type::_type_info._max_precision = 2 * max_ebcdic_number_size - 1;
    Ebcdic_packed_type::_type_info._radix         = 10;
    Ebcdic_packed_type::_type_info._max_scale     = 2 * max_ebcdic_number_size - 1;
    Ebcdic_packed_type::_type_info.normalize();

    Ebcdic_text_type::_type_info._std_type      = std_type_char;
    Ebcdic_text_type::_type_info._name          = "Ebcdic_text";
    Ebcdic_text_type::_type_info._nullable      = true;
    Ebcdic_text_type::_type_info._max_size      = 32767;
    Ebcdic_text_type::_type_info._max_precision = 32767;
    Ebcdic_text_type::_type_info._exact_char_repr = true;
    Ebcdic_text_type::_type_info.normalize();
}

//--------------------------------------------------------------------------read_profile_german

static void read_profile_german()
{
    Z_MUTEX( hostware_mutex )
    {
        german_profile_read = true;
        Sos_string code;
        code = read_profile_string( "", "sosfield", "code" );
        german = code == "german" || code == "ebcdic-german";
    }
}

//--------------------------------------------------------------------------------------print_x

static void print_x( ostream* s, char c, int count )
{
    if( count <= 4 )  for( int i = 0; i < count; i++ )  *s << c;
                else  *s << c << '(' << count << ')';
}

//-------------------------------------------------------Ebcdic_number_type::Ebcdic_number_type

Ebcdic_number_type::Ebcdic_number_type( int field_size, Ebcdic_type_flags flags )
:
    Field_type( &_type_info, field_size ),
    _scale(0),
    _unsigned ( false )
{
    positive_sign( 0xC );
    set_flags( flags );
    if( (uint)field_size > max_ebcdic_number_size )  throw_xc( "Ebcdic_number_type", "max 31 Ziffern" );
}

//---------------------------------------------------------------Ebcdic_number_type::set_flags

void Ebcdic_number_type::set_flags( Ebcdic_type_flags flags )
{
    if( flags & ebc_ascii ) 
    {
        _zone  = '0';
        _plus  = '+';
        _minus = '-';
        _blank = ' ';
    } 
    else 
    {
        _zone  = '\xF0';  // EBCIDC C'0'
        _plus  = 0x4E;    // EBCIDC C'+'
        _minus = 0x60;    // EBCDIC C'-'
        _blank = 0x40;    // EBCDIC C' '
    }
}

//---------------------------------------------------------------------Ebcdic_number_type::null

Bool Ebcdic_number_type::null( const Byte* p ) const
{
    // Feld ist vollständig binär 0 oder blank?
    const Byte* p_end = p + field_size();
    if( *p == 0x00 )  while( p < p_end )  { if( *p++ != 0x00   )  return false; }
                else  while( p < p_end )  { if( *p++ != _blank )  return false; }
    return true;
}

//-----------------------------------------------------------------Ebcdic_number_type::set_null

void Ebcdic_number_type::set_null( Byte* p ) const
{
    memset( p, _blank, field_size() );
}

//---------------------------------------------------------------Ebcdic_number_type::write_text

void Ebcdic_number_type::write_text( const Byte* ptr, Area* buffer, const Text_format& format ) const
{
    int         n     = _field_size;
    int         size  = n;
    const Byte* p     = ptr;
    const Byte* p_end = p + n;
    Bool        neg   = negative( p );

    if( _separate_sign ) {
        if( _leading_sign )  p++;  
                       else  p_end--;
        if( !neg )  size--;             // Platz für Vorzeichen entfällt ('+' wird nicht ausgegeben)
        n--;
    } else {
        if( neg )  size++;              // Platz für Vorzeichen
    }

    if( _scale ) {
        size++;                         // Platz für Dezimalzeichen
        if( _scale == n )  size++;      // Platz für Vornull
    }

    buffer->allocate_min( size );

    char* b = buffer->char_ptr();

    if( neg )  *b++ = '-';                                          // Vorzeichen

    int s = n - _scale;         // Anzahl der Vorkommastellen

    if( s == 0 )  *b++ = '0';   // Eine 0 vor dem Komma

    if( !format.raw() )  {
        while( s > 1  &&  ( *p & 0x0F ) == 0 )  { s--; p++; }       // Vornullen unterdrücken
        const Byte* p0 = p_end - _scale;                            // Nachnullen unterdrücken
        while( p_end > p0  &&  ( p_end[-1] & 0x0F ) == 0 )  p_end--;
    }

    while( p < p_end )  {
        if( s-- == 0 )  *b++ = format.decimal_symbol();
        //Byte d = *p++ & 0x0F;
        //if( d > 9 )  goto FEHLER;
        //*b++ = (char) ( '0' + d );
        *b++ = ascii()? (char)*p++ : ebc2iso[ *p++ ];        // jz 15.2.00: Falsche Zeichen erhalten
    }

    if( !_unsigned ) {                                       // jz 15.2.00: Vorzeichen korrigieren
        if( ( p[-1] & 0x0F ) < 10 ) {
            if( ascii() )  b[-1] &= ~0x40, b[-1] |= 0x30;
                     else  b[-1] = ebc2iso[ p[-1] | 0xF0u ];
        }
    }
        
    buffer->length( b - buffer->char_ptr() );
//  return;

//FEHLER:
//  if( null( ptr ) )  return;

//  throw_xc_hex( "SOS-1108", ptr, field_size() );
}

//----------------------------------------------------------------Ebcdic_number_type::read_text

void Ebcdic_number_type::read_text( Byte* ptr, const char* t, const Text_format& format ) const
{
    if( format.text() ) {
        if( ascii() ) {
            Text_type type ( field_size() );
            type.read_text( ptr, t, format );
        } else {
            Ebcdic_text_type type ( field_size(), Ebcdic_type_flags( ebc_mvs | ebc_international ) );
            type.read_text( ptr, t, format );
        }
    }
    else
    {
        Decimal number;

        number.scale( _scale );
        number.length( _field_size );
        number.read_text( t, format );

        int   i = _field_size;
        Byte* p = ptr;
        
        if( _separate_sign )  { 
            if( _leading_sign )  p++; 
            i--; 
        }

        while( i > 0 )  *p++ = _zone + number.digit( --i );
        while( i > number.length() )  if( number.digit( --i ) != 0 )  throw_overflow_error( "SOS-1107", t );

        int sign = number.sign();
        if( _unsigned  &&  sign < 0 )  throw_xc( "SOS-1237", t ); // darf nicht negativ werden

        if( _separate_sign ) {
            //( _leading_sign? ptr[0] : p[0] ) = sign < 0? 0x60 : 0x4E;   // EBCDIC '-', '+'
            ( _leading_sign? ptr[0] : p[0] ) = sign < 0? _minus : _plus;
        } else {
            Byte* s = _leading_sign? ptr : p -1;
            if( ascii() ) {
                if( sign < 0 )  *s = *s & 0x0F | 0x40;            // @ABCDEFGHI
            } else {
                *s &= sign < 0? 0xDF : _positive_sign_mask;
            }
        }
    }
}

//-----------------------------------------------------------------Ebcdic_number_type::negative

Bool Ebcdic_number_type::negative( const Byte* p ) const
{ 
    Byte s = _leading_sign? p[0] : p[ _field_size - 1 ]; 

    if( _separate_sign ) {
      //return s == 0x60;       // '-'
        return s == _minus;     // '-'
    } else {
        if( ascii() ) {
            return ( s & 0x40 ) != 0;               // Bit 6
        } else {
            s &= 0xF0; 
            return s == 0xD0 || s == 0xB0; 
        }
    }
}

//---------------------------------------------------------------Ebcdic_number_type::op_compare

int Ebcdic_number_type::op_compare( const Byte* a, const Byte* b ) const
{
    Byte aa [ 1 + max_ebcdic_number_size ];
    Byte bb [ 1 + max_ebcdic_number_size ];
    int  i;

  //if( aa[ _field_size ] <= _blank )           // NULL
    if( a[ _field_size-1 ] <= _blank )           // NULL
    {            
        memset( aa, 0, 1 + _field_size );
    } 
    else 
    {
        memcpy( aa + 1, a, _field_size );
        aa[ 0 ] = 2; // positiv
        
        if( negative( aa + 1 ) )  
        {
            aa[ 0 ] = 1; // negativ
            for( i = 1; i <= _field_size; i++ )  aa[ i ] = ~aa[ i ];
        }
    }

  //if( bb[ _field_size ] <= _blank )           // NULL
    if( b[ _field_size-1 ] <= _blank )           // NULL
    {           
        memset( aa, 0, 1 + _field_size );
    } 
    else 
    {
        memcpy( bb + 1, b, _field_size );
        bb[ 0 ] = 2; // positiv
        
        if( negative( bb + 1 ) )  
        {
            bb[ 0 ] = 1; // negativ
            for( i = 1; i <= _field_size; i++ )  bb[ i ] = ~bb[ i ];
        }
    }

    return memcmp( aa, bb, 1 + _field_size );
}

//---------------------------------------------------------------Ebcdic_number_type::_get_param

void Ebcdic_number_type::_get_param( Type_param* param ) const
{
    param->_precision    = field_size() - _separate_sign;
    param->_display_size = param->_precision + ( _scale? 2 : 1 )    // Vorzeichen und evtl. Dezimalzeichen
                                             + ( _scale == field_size()? 1 : 0 );  // Vornull
    param->_scale        = _scale;
  //param->_info_ptr     = _info;
}

//--------------------------------------------------------------------Ebcdic_number_type::_obj_print

void Ebcdic_number_type::_obj_print( ostream* s ) const
{
    *s << "PIC ";
    //if( _leading_sign )  *s << 'S';
    if( !_unsigned )  *s << 'S';
    print_x( s, '9', ( field_size() - _separate_sign ) - _scale );
    if( _scale )  *s << 'V';
    print_x( s, '9', _scale );
    if( _leading_sign  ||  _separate_sign ) {
        *s << " SIGN IS";
        *s << ( _leading_sign? " LEADING" : " TRAILING" );
        if( _separate_sign )  *s << " SEPARATE";
    }
    //if( !_unsigned && !_leading_sign )  *s << 'S';
}

//-------------------------------------Ebcdic_unsigned_number_type::Ebcdic_unsigned_number_type

Ebcdic_unsigned_number_type::Ebcdic_unsigned_number_type( int field_size, Ebcdic_type_flags flags )
:
    Ebcdic_number_type( &_type_info, field_size, flags )
{
    scale( 0 );
    _unsigned = true;

  //if( (uint)field_size > 31 )  throw_xc( "Ebcdic_number_type", "max 31 Ziffern" );
    positive_sign( 0xF );
}

//-------------------------------------------------------Ebcdic_unsigned_number_type::negative

Bool Ebcdic_unsigned_number_type::negative( const Byte* p ) const
{ 
    // jz 15.2.00. Ein Buchstabe wird nun als Buchstabe geliefert, nicht mehr als negative Zahl
    return false;     
}

//-----------------------------------------------------Ebcdic_unsigned_number_type::_get_param

void Ebcdic_unsigned_number_type::_get_param( Type_param* param ) const
{
    param->_precision    = field_size();
    param->_display_size = param->_precision + ( _scale? 1 : 0 )                   // Dezimalzeichen
                                             + ( _scale == field_size()? 1 : 0 );  // Vornull
    param->_scale        = _scale;
  //param->_info_ptr     = _info;
}

//------------------------------------------------------Ebcdic_unsigned_number_type::_obj_print
/*
void Ebcdic_unsigned_number_type::_obj_print( ostream* s ) const
{
    *s << "PIC ";
    print_x( s, '9', field_size() - _scale );
    if( _scale )  *s << 'V';
    print_x( s, '9', _scale );
    // *s << "Ebcdic_unsigned_number(" << field_size();
    // if( scale() )  *s <<  ',' << scale();
    // *s << ')';
}
*/
//-------------------------------------------------------Ebcdic_packed_type::Ebcdic_packed_type

Ebcdic_packed_type::Ebcdic_packed_type( int field_size )
:
    Field_type( &_type_info, field_size ),
    _positive_sign( 0x0C ),
    _scale(0)
{
    if( (uint)field_size > max_ebcdic_number_size )  throw_xc( "Ebcdic_packed_type", "max 31 Ziffern" );
}

//---------------------------------------------------------------------Ebcdic_packed_type::null

Bool Ebcdic_packed_type::null( const Byte* p ) const
{
    // Feld ist vollständig binär 0 oder blank?
    const Byte* p1 = p + field_size();
    if( p1[-1] == 0x00 )  while( p1 > p )  { if( *--p1 != 0x00 )  return false; }
    if( p1[-1] == 0x40 )  while( p1 > p )  { if( *--p1 != 0x40 )  return false; }       // Nur EBCDIC-Blanks?
                    else  while( p1 > p )  { if( *--p1 != 0x20 )  return false; }       // Nur ASCII-Blanks?
    return true;
}

//-----------------------------------------------------------------Ebcdic_packed_type::set_null

void Ebcdic_packed_type::set_null( Byte* p ) const
{
    memset( p, 0, field_size() );
}

//---------------------------------------------------------------Ebcdic_packed_type::op_compare

int Ebcdic_packed_type::op_compare( const Byte* a, const Byte* b ) const
{
    Byte aa [ 1 + max_ebcdic_packed_size ];
    Byte bb [ 1 + max_ebcdic_packed_size ];
    int  i;

    if( ( a[ _field_size - 1 ] & 0xF0 ) == 0x00 )       // NULL?
    {
        memset( aa, 0, 1 + _field_size );
    } 
    else 
    {
        memcpy( aa + 1, a, _field_size );
        aa[ 0 ] = 2;  // positiv
        aa[ _field_size ] &= 0xF8;  // Vorzeichen -> 0xX8  NULL -> 0xX0
    
        if( negative( aa + 1 ) ) 
        {
            // -0 < +0 !
            aa[ 0 ] = 1;  // negativ
            for( i = 1; i <= _field_size; i++ )  aa[ i ] = ~aa[ i ];
        }
    }

    if( ( b[ _field_size ] & 0xF0 ) == 0x00 )           // NULL?
    {
        memset( bb, 0, 1 + _field_size );
    } 
    else 
    {
        memcpy( bb, b, _field_size );
        bb[ 0 ] = 2;  // positiv
        bb[ _field_size ] &= 0xF8;  // Vorzeichen -> 0xX8  NULL -> 0xX0
        
        if( negative( bb + 1 ) ) 
        {
            // -0 < +0 !
            bb[ 0 ] = 1;  // negativ
            for( i = 1; i <= _field_size; i++ )  bb[ i ] = ~bb[ i ];
        }
    }

    return memcmp( aa, bb, 1 + _field_size );
}

//---------------------------------------------------------------Ebcdic_packed_type::write_text

void Ebcdic_packed_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    Bool neg = negative( p );
    int  n   = 2 * _field_size - 1;

    //if( format.raw() )
    {
        int size = n;                   // Die Ziffern
        if( neg )  size++;              // "-"
        if( _scale ) {
            size++;                     // ","
            if( _scale == n )  size++;  // Vornull "0,"
        }
        buffer->allocate_min( size );

        char*       b     = buffer->char_ptr();
        char        d;
        const Byte* q     = p;
        const Byte* q_end = q + _field_size - 1;
        int         s     = n - _scale;

        if( neg )  *b++ = '-';                                          // Vorzeichen
      //if( s == 0 )  { *b++ = '0'; *b++ = format.decimal_symbol(); }

        if( s == 0 ) {
            *b++ = '0';   // Eine 0 vor dem Komma
            *b++ = format.decimal_symbol();
        }
        else
        if( !format.raw() ) {
            while( s > 2  &&  *q == 0x00 )  { s -= 2; q++; }            // Vornullen überspringen
            if( s > 1  &&  ( *q & 0xF0 ) == 0 ) {                       // Eine Vornull und eine Ziffer übrig?
                d = *q++;    if( d > 9 )  goto FEHLER;                      // Ziffer ausgeben
                *b++ = '0' + d;
                s -= 2;
                if( s == 0 )  *b++ = format.decimal_symbol();
            }
        }


        while( q < q_end ) {
            d = *q >> 4;      if( d > 9 )  goto FEHLER;
            *b++ = '0' + d;
            if( --s == 0 )  *b++ = format.decimal_symbol();

            d = *q & 0x0F;    if( d > 9 )  goto FEHLER;
            *b++ = '0' + d;
            if( --s == 0 )  *b++ = format.decimal_symbol();

            q++;
        }

        d = *q >> 4;      if( d > 9 )  goto FEHLER;
        *b++ = (char)( '0' + d );


        if( !format.raw()  &&  _scale ) {       // Nachnullen unterdrücken
            while( b[-1] == '0' )  b--;
            if( b[-1] == format.decimal_symbol() )  b--;
        }

        buffer->length( b - buffer->char_ptr() );
    }
/*
    else
    {
    	if( null( p ) )  return;

        Decimal number;
        number.length( n );
        number.scale( _scale );
        Sos_binary_istream s ( p, _field_size );
        read_ibm370_packed ( &s, &number, _field_size );
        number.write_text( buffer, format );
    }
*/
    return;

  FEHLER:
    if( !null( p ) )  throw_xc_hex( "SOS-1173", p, _field_size );
}

//----------------------------------------------------------------Ebcdic_packed_type::read_text

void Ebcdic_packed_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    Decimal number;

    number.scale( _scale );
    number.length( 2 * _field_size - 1 );
    number.read_text( t, format );

    Sos_binary_ostream s ( p, _field_size );
    write_ibm370_packed ( &s, number, _field_size, positive_sign() );
}

//---------------------------------------------------------------Ebcdic_packed_type::_get_param

void Ebcdic_packed_type::_get_param( Type_param* param ) const
{
    param->_precision    = field_size() * 2 - 1;
    param->_display_size = param->_precision + ( _scale? 2 : 1 )    // Vorzeichen und evtl. Dezimalzeichen
                                             + ( _scale == field_size()? 1 : 0 );  // Vornull
    param->_scale        = _scale;
  //param->_info_ptr     = &_type_info;
  //LOG( "Ebcdic_packed_type: prec=" << param->_precision << "\n" );
}

//---------------------------------------------------------------Ebcdic_packed_type::_obj_print

void Ebcdic_packed_type::_obj_print( ostream* s ) const
{
    *s << "PIC ";
    if( positive_sign() == 0x0C )  *s << 'S';
    print_x( s, '9', ( 2 * field_size() - 1 ) - _scale );
    if( _scale )  *s << 'V';
    print_x( s, '9', _scale );
    *s << " PACKED-DECIMAL";
    //*s << "Ebcdic_packed(" << ( field_size() * 2 - 1 );
    //if( _scale )  *s << ',' << _scale;
    //*s << ')';
}

//-----------------------------------------------------------read_field(Ebcdic_packed_type,...)

void read_field_packed ( const Ebcdic_packed_type& type, const Byte* ptr, Big_int* object_ptr )
{
    Decimal             decimal;
    Sos_binary_istream  s ( ptr, type.field_size() );

    read_ibm370_packed( &s, &decimal, type.field_size() );

    *object_ptr = decimal.as_big_int();

    if( Decimal( *object_ptr ) != decimal )  throw_xc( "SOS-1112" );     // Überlauf
}

//-------------------------------------------------------------------write_field(Ebcdic_packed_type,...)

void write_field_packed( const Ebcdic_packed_type& type, Byte* ptr, Big_int object )
{
    Sos_binary_ostream  s ( ptr, type.field_size() );

    write_ibm370_packed( &s, Decimal( object ), type.field_size(), type.positive_sign() );
}

//-----------------------------------------------------------Ebcdic_text_type::Ebcdic_text_type

Ebcdic_text_type::Ebcdic_text_type( int field_size, Ebcdic_type_flags flags )
:
    Xlat_text_type( field_size, 0, 0 )
{
    if( !( flags & ( ebc_german | ebc_international ) ) ) 
    {
        if( !german_profile_read ) {
            read_profile_german();
        }
        if( german )  flags = Ebcdic_type_flags( flags | ebc_german );
    }

    write_table( flags & ebc_mvs? flags & ebc_german? mvs2iso_german_0_is_blank 
                                                    : mvs2iso_0_is_blank  
                                : flags & ebc_german? ebc2iso_german_0_is_blank 
                                                    : ebc2iso_0_is_blank );

    read_table( flags & ebc_mvs? flags & ebc_german? iso2mvs_german 
                                                   : iso2mvs 
                               : flags & ebc_german? iso2ebc_german 
                                                   : iso2ebc );

/*
#	if defined __BORLANDC__  &&  defined __WIN32__ // BC++ 5.00
	if( !german ) {
	    write_table( ebc2iso );// Borland hat oben ebc2iso mit &german[1] verwechselt
    }
#	endif
*/
}

//--------------------------------------------------------------------------Ebcdic_text_type::null

Bool Ebcdic_text_type::null( const Byte* p ) const
{
    // Feld ist vollständig binär 0?
    const Byte* p_end = p + field_size();
    while( p < p_end )  if( *p++ )  return false;
    return true;
}

//-------------------------------------------------------------------Ebcdic_text_type::set_null

void Ebcdic_text_type::set_null( Byte* p ) const
{
    memset( p, 0, field_size() );
}

//-----------------------------------------------------------------Ebcdic_text_type::_obj_print

void Ebcdic_text_type::_obj_print( ostream* s ) const
{
    *s << "PIC ";
    print_x( s, 'X', field_size() );
    //*s << "Ebcdic_packed(" << ( field_size() * 2 - 1 );
    //if( _scale )  *s << ',' << _scale;
    //*s << ')';
}

//-----------------------------------------------------------------Ebcdic_text_type::_get_param
/*
void Ebcdic_text_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
}
*/

} //namespace sos
