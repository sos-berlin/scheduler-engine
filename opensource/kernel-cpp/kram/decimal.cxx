#include "precomp.h"
//#define MODULE_NAME "decimal"
// decimal.cpp
//                                                      (c) Joacim Zschimmer

//#pragma implementation

#include <limits.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "../kram/sos.h"
#include "../kram/sosfield.h"           // Text_format
#include "../kram/soslimtx.h"
#include "../kram/log.h"
#include "../kram/sosstrea.h"


#include "decimal.h"

using namespace std;
namespace sos {


Type_info Decimal_type::_type_info;

Decimal_type decimal_type;

//-------------------------------------------------------------------------------------SOS_INIT

SOS_INIT( decimal )
{
    Decimal_type::_type_info._std_type      = std_type_decimal;
    Decimal_type::_type_info._name          = "Decimal";
    Decimal_type::_type_info._nullable      = true;
    Decimal_type::_type_info._max_size      = 31;
    Decimal_type::_type_info._max_precision = 31;
    Decimal_type::_type_info._radix         = 10;
    Decimal_type::_type_info._max_scale     = 31;
    Decimal_type::_type_info.normalize();
}

//--------------------------------------------------------------------------------------Decimal

Decimal::Decimal( Big_int value, int length )
:
    _length   (length),
    _overflow (0),
    _sign     (value < 0 ? -1 : value == 0 ? 0 : 1),
    _scale    (0)
{
    memset (_digits, 0, sizeof _digits);

    int i = 0;
    value = abs( value );
    while( value && i < length ) {
        _digits[ i ] = Byte( value % 10 );
        value /= 10;
        i++;
    }
}

//-------------------------------------------------------------Decimal::zero

Bool Decimal::zero() const
{
    return _digits[0] == 0
       &&  (sizeof _digits == 1 ||
            memcmp(_digits,_digits+1,sizeof _digits-1) == 0);
}

//---------------------------------------------------------------Decimal::exp10

void Decimal_31::exp10( int shift_value, Byte round_digit )
{
    if (shift_value < 0) {      // rechts
        shift_value = -shift_value;

        if ( shift_value <= length() ) {
            *this += Decimal( (signed char)round_digit ).as_big_int() << ( shift_value - 1 );
        }

        if (shift_value < length()) {
            memmove (_digits, _digits + shift_value,
                     length() - shift_value);
        } else {
            shift_value = length();
        }
        memset (_digits + length() - shift_value, 0, shift_value );
        if( zero() )  _sign = 0;
    }
    else
    if (shift_value > 0) {      // links
        for (int i = length() - 1; i > length() - shift_value; i--) {
            if (_digits[i] != 0)  _overflow = true;
        }
        if (shift_value < length()) {
            memmove (_digits + shift_value, _digits,
                     length() - shift_value);
        } else {
            shift_value = length();
        }
        memset( _digits, 0, shift_value );
        if (zero())  _sign = 0;
    }
}

//--------------------------------------------------------------------------Decimal::operator+=

Decimal_31& Decimal_31::operator+= ( const Decimal_31 &p )
{
  /*if (sign() == p.sign())  add (p);
    else
    if (sign() >  p.sign())  sub (p);
    else {
        sub (p);
        neg();
    }*/
    if( sign() >= 0  &&  p.sign() >= 0 ) {
        add( p );
    } else {
        sub( p );
        if( sign() < 0 ) {
            neg();
        }
    }
    return *this;
}

//--------------------------------------------------------------------------Decimal::operator*=

Decimal_31& Decimal_31::operator*= ( const Decimal_31& p )
{
    Decimal_31 summe = 0;
    Bool       ueberlauf = false;
    const int  len = no_of_significant_digits();

    for( int i = p.no_of_significant_digits() - 1; i >= 0; i-- ) {
        summe.exp10( 1 );

        if( p._digits[ i ] ) {
        		int		  j;
            Decimal_31 produkt   = 0;      // Multiplikant * Ziffer
            int        uebertrag = 0;

            for( j = 0; j < len; j++ ) {
                int klein = _digits[ j ] * p._digits[ i ] + uebertrag;
                produkt._digits[ j ] = klein % 10;
                uebertrag            = klein / 10;
            }

            if( j >= 0 ) {
                produkt._digits[ j ] = uebertrag;
                uebertrag = 0;
            }

            if( uebertrag )  ueberlauf = true;
            summe += produkt;
        }
    }

    summe._sign = _sign * p._sign;
    *this = summe;
    if( ueberlauf )  _overflow = true;
    return *this;
}

//--------------------------------------------------------------------------Decimal::operator/=

#if 0
Decimal_31& Decimal_31::operator/= ( const Decimal_31& p )
{
    Decimal_31 summe = 0;
    Bool       ueberlauf = false;
    const int  len = no_of_significant_digits();

    for( int i = p.no_of_significant_digits() - 1; i >= 0; i-- ) {
        summe.exp10( 1 );

        if( p._digits[ i ] ) {
            Decimal_31 produkt   = 0;      // Multiplikant * Ziffer
            int        uebertrag = 0;

            for( int j = 0; j < len; j++ ) {
                int klein = _digits[ j ] * p._digits[ i ] + uebertrag;
                produkt._digits[ j ] = klein % 10;
                uebertrag            = klein / 10;
            }

            if( j >= 0 ) {
                produkt._digits[ j ] = uebertrag;
                uebertrag = 0;
            }

            if( uebertrag )  ueberlauf = true;
            summe += produkt;
        }
    }

    summe._sign = _sign * p._sign;
    *this = summe;
    if( ueberlauf )  _overflow = true;
    return *this;
}
#endif

//------------------------------------------------------------------------------Decimal::divide

Decimal_31& Decimal_31::divide( const Decimal_31& divisor, Decimal_31* rest_ptr )
{
    Big_int int_dividend = as_big_int();
    Big_int int_divisor  = divisor.as_big_int();

    *rest_ptr = int_dividend % int_divisor;
    *this     = int_dividend / int_divisor;
    return *this;
}

//--------------------------------------------------------------------------Decimal::as_big_int

Big_int Decimal::as_big_int() const
{
    int i;
    for( i = 0; i < _scale; i++ )  {
        if( _digits[ i ] )  {
            Sos_limited_text<100> text;
            write_text( &text );
            throw_xc( "SOS-1107", "Big_int", c_str( text ) );
        }
    }

    Big_int v = 0;

    for( i = length() - 1; i >= 0; i-- )
    {
        if( v > BIG_INT_MAX / 10 )  throw_overflow_error();
        v = v * 10 + _digits[i];
        if( v < 0 )  throw_overflow_error();
    }

    return v * _sign;
}

//---------------------------------------------------------------------------------Decimal::add

void Decimal_31::add( const Decimal_31& p )
{
    Byte carry = 0;
    Bool zero  = true;

    for (int i = 0; i < length(); i++) {
        Byte sum = _digits[ i ] + p._digits[ i ] + carry;
        carry = 0;
        if (sum >= 10) {
            sum -= 10;
            carry = 1;
        }
        _digits[ i ] = sum;
        if (sum != 0)  zero = false;
    }
    if (zero)  _sign = 0;
    _overflow = carry;
}

//---------------------------------------------------------------------------------Decimal::sub

void Decimal_31::sub( const Decimal_31& p )
{
    Byte borrow = 0;
    Bool zero   = true;

    for (int i = 0; i < length(); i++) {
        signed char diff = _digits[ i ] - p._digits[ i ] - borrow;
        borrow = 0;
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        }
        _digits[ i ] = diff;
        if (diff != 0)  zero = false;
    }

    if (zero)  _sign = 0;

    if (borrow) {                       // Invertieren: (1)000..000 - nnn..nnn
        borrow = 0;
        for (int i = 0; i < length(); i++) {
            signed char diff = 0 - _digits[ i ] - borrow;
            borrow = 0;
            if (diff < 0) {
                diff += 10;
                borrow = 1;
            }
            _digits[ i ] = diff;
        }
        _sign = -_sign;
    }
}

//---------------------------------------------------------------------------Decimal::operator<

Bool Decimal::operator< ( const Decimal& p ) const
{
    return ( *this - p ).sign() < 0;
}

//--------------------------------------------------------------------------Decimal::operator<=

Bool Decimal::operator<= ( const Decimal& p ) const
{
    return ( *this - p ).sign() <= 0;
}

//--------------------------------------------------------------------------Decimal::operator==

Bool Decimal::operator== ( const Decimal& p ) const
{
    if( _scale == p._scale
     && _sign  == p._sign
     && memcmp( _digits, p._digits, sizeof _digits ) == 0 )  return true;

    return ( *this - p ).zero();
}

//----------------------------------------------------------Decimal::no_of_significant_digits()

int Decimal::no_of_significant_digits() const
{
    int i = length() - 1;
    while( i >= 0  &&  _digits[ i ] == 0 )  i--;

    return i + 1;
}

//--------------------------------------------------------------------------write_ibm370_packed

void write_ibm370_packed( Sos_binary_ostream* s, const Decimal& decimal, uint field_size, Byte positive_sign )
{
    int needed_space = /*12.11.97 Frey ( decimal.sign() < 0 ) +*/ field_size;
    if( (int)s->space_left() < needed_space )  throw_too_long_error( "SOS-1113", needed_space, s->space_left() );

    Byte* p0          = (Byte*) s->space( field_size );
    Byte* p           = p0 + field_size;
    int   digit_count = decimal.no_of_significant_digits();

    if( digit_count > (int)field_size * 2 - 1 )  { Xc x ( "SOS-1176" ); x.insert( decimal.as_big_int() ); x.insert( field_size * 2 - 1 ); throw x; }

    *--p = ( decimal.digit( 0 ) << 4 )  |  ( decimal.sign() >= 0 ? positive_sign : 0x0D );

    int d = 1;
    while( p > p0  &&  d < digit_count ) {
        Byte b = decimal.digit( d++ );
        if( d < digit_count ) {
            b |= decimal.digit( d++ ) << 4;
        }
        *--p = b;
    }

    memset( p0, 0x00, p - p0 );
}

//---------------------------------------------------------------------------read_ibm370_packed

void read_ibm370_packed( Sos_binary_istream* s, Decimal* decimal_ptr, uint field_size )
{
    const Byte* const p0     = (const Byte*)s->read_bytes( field_size );
    const Byte*       p      = p0 + field_size;

    if( field_size * 2 - 1 > decimal_ptr->length() )  { Xc x ( "SOS-1174" ); x.insert_hex( p0, field_size ); x.insert( decimal_ptr->length() ); throw x; }

    //*decimal_ptr = 0;

    int  d          = 0;        // Nummer der zu setzenden Ziffer
    Byte b          = *--p;
    int  ored_bytes = b & 0xF0;
    Byte sign       = b & 0x0F;

    if( sign < 0x0A )  goto FEHLER;
    decimal_ptr->sign( sign == 0x0B || sign == 0x0D? -1 : 1 );

    while(1) {
        Byte digit = b >> 4;  if( digit > 9 )  goto FEHLER;
        decimal_ptr->digit( d++, digit );

        if( p == p0 )  break;

        b = *--p;
        ored_bytes |= b;

        digit = b & 0x0F;
        if( digit > 9 )  goto FEHLER;
        decimal_ptr->digit( d++, digit );
    }

    while( d < decimal_ptr->length() )  decimal_ptr->digit( d++, 0 );
    if( ored_bytes == 0 )  decimal_ptr->sign( 0 );

    return;

  FEHLER:
    Xc x ( "SOS-1173" );
    x.insert_hex( p0, field_size );
    throw x;
    //LOG( "read_ibm370_packed: " << *decimal_ptr << "  " );
    //for( int j = 9; j >= 0; j-- ) { LOG( (char)( '0' + decimal_ptr->digit(j) ) ); }
    //LOG( "\n" );
}

//--------------------------------------------------------------------------Decimal::write_text

void Decimal::write_text( Area* buffer, const Text_format& format ) const
{
    int n = _length;

    buffer->allocate_min( n + 1/*minus*/ + 1/*komma*/ );
    char* b = buffer->char_ptr();
    if( _sign < 0 )  *b++ = '-';

    int i = decimal_digits;
    while( i > _scale + 1  &&  _digits[ i - 1 ] == 0 )  i--;    // Vornullen

    while( i > _scale )  *b++ = _digits[ --i ] + '0';

    int j = 0;
    while( i > j  &&  ( _digits[ j ] == 0 ) )  j++;  // Nachnullen

    if( i > j ) {
        *b++ = format.decimal_symbol();
        while( i > j )  *b++ = (char) ( '0' + _digits[ --i ] );
    }

    buffer->length( b - buffer->char_ptr() );
}

//---------------------------------------------------------------------------Decimal::read_text

void Decimal::read_text( const char* ptr, const Text_format& format, Bool adjust_scale )
{
    const char* p = ptr;
    int         i;

    memset( _digits, 0, sizeof _digits );
    _sign = +1;

    if( adjust_scale ) {
        const char* p = strchr( ptr, '.' );
        if( !p )  p = strchr( ptr, ',' );
        if( p ) {
            int s = 0;
            p++;
            while( isdigit( *p ) )  { p++; s++; }
            if( s > decimal_digits )  throw_xc( "SOS-1107", &decimal_type, ptr );
            if( s > _scale )  _scale = s;
        }
    }

    while( *p == ' ' )  p++;                                // Blanks am Anfang abschneiden

    if( *p == '-' )  { p++; _sign = -1; }                   // Vorzeichen
    else
    if( *p == '+' )  p++;


    while( *p == '0' )  p++;                                // Vornullen

    const char* p0 = p;                                     // Erste Ziffer
    while( isdigit( *p ) )  p++;                            // Vorkommaanteil
    const char* r = p;  i = _scale;
    while( r > p0  &&  i < _length )  _digits[ i++ ] = *--r - '0';
    if( r != p0 )  goto THROW_1107;

    if( format.decimal_symbol()  &&  *p == format.decimal_symbol() )  p++;  // Komma
    i = _scale;
    while( i > 0 &&  isdigit( *p ) )  _digits[ --i ] = *p++ - '0';
    while( *p == '0' )  p++;                                // Nachnullen
    while( *p == ' ' )  p++;                                // Blanks
    if( *p )  goto THROW_1107;

    for( i = 0; i < _length; i++ )  if( _digits[ i ] )  break;
    if( i == _length )  _sign = 0;

    return;

  THROW_1107:
    throw_xc( "SOS-1107", &decimal_type, ptr );
}

//-----------------------------------------------------------------------------------as_decimal

Decimal as_decimal( const char* str )
{
    Decimal decimal;
    decimal.read_text( str, std_text_format, true );
    return decimal;
}

//---------------------------------------------------------------------------Decimal_type::null

Bool Decimal_type::null( const Byte* p ) const
{
    return ((const Decimal*)p)->null();
}

//-----------------------------------------------------------------------Decimal_type::set_null

void Decimal_type::set_null( Byte* p ) const
{
    ((Decimal*)p)->set_null();
}

//---------------------------------------------------------------------Decimal_type::write_text

void Decimal_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    ((const Decimal*)p)->write_text( buffer, format );
}

//----------------------------------------------------------------------Decimal_type::read_text

void Decimal_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    ((Decimal*)p)->read_text( t, format, true );
}

} //namespace sos
