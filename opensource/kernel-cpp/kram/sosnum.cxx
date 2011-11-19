#include <precomp.h>
#if 0

#define MODULE_NAME "sosnum"
#define COPYRIGHT   "(c) SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#pragma implementation

#include <sosstrng.h>

#include <sos.h>
//#include <sosstrea.h>
#include <sosfield.h>
#include <exp10.h>
#include <sosnum.h>


#if defined SOS_NUMBER_BIGINT
    Sos_number_type sos_number_type;
#endif

//----------------------------------------------------------------------------Sos_number::operator=

Sos_number& Sos_number::operator= ( Big_int value )
{
    _number = value;
    _number *= exp10_table[ _scale ];
    return *this;
}

//----------------------------------------------------------------------------Sos_number::operator=
/*
Sos_number& Sos_number::operator*= ( Big_int value )
{
    _number = value;
    _number.exp10( -_scale );
    return *this;
}
*/
//-----------------------------------------------------------------------Sos_number_type::print

void Sos_number_type::print( const Byte* p, ostream* s, const Text_format& f ) const
{
    Big_int_field t ( ((const Sos_number*)p)->_scale );
    t.print( (const Byte*)&((const Sos_number*)p)->_number, s, f );
}

//-----------------------------------------------------------------------Sos_number_type::input

void Sos_number_type::input( Byte* p, istream* s, const Text_format& f ) const
{
    Big_int_field t ( ((const Sos_number*)p)->_scale );
    t.input( (Byte*)&((const Sos_number*)p)->_number, s, f );
}

//--------------------------------------------------------------------------Sos_number::object_load
#if 0 && !defined SOS_NUMBER_BIGINT

void Sos_number::object_load( const Sos_string& str )
{
                                // 12.345,6789
                                // 123456789
    Byte        ziffern [ 40 ];       // [ 0 ]: niederwertigste Ziffer
    Byte*       z         = ziffern;
    int         komma_pos = 0;        // Anzahl der Nachkommastellen in ziffern

    if( length( str ) > sizeof ziffern ) {
        throw_syntax_error( "SOS-1100" );
    }

    memset( ziffern, 0, sizeof ziffern );

    for( int i = length( str ) - 1; i >= 0; i-- )
    {
        const char c = str[ i ];
        if( c >= '0'  &&  c <= '9' ) {
            *z++ = c - '0';
        }
        else
        if( c == '.' ) ;       // Punkt an beliebiger Stelle ignorieren (Tausenderpunkt)
        else
        if( c == ',' ) {
            komma_pos = z - ziffern;
        }
        else throw_syntax_error( "SOS-1105" );
    }

    _number = 0;

    int shift = -_scale - komma_pos;

  /*if( shift < 0 ) {
        z -= shift;             // Niederwertige Stellen abschneiden; RUNDEN NICHT VERGESSEN!
        int rundung_fehlt;
    }*/

    while( z-- > ziffern + max( 0, -shift ) ) {
        //_number.set_digit( i, *y );
        _number *= 10;
        _number += *z;
    }

    if( shift > 0 ) {
//      _number.scale( shift );
    }
}

//-------------------------------------------------------------------------Sos_number::object_store

void Sos_number::object_store( Sos_string* string_ptr ) const
{
    char ziffern [ 40 + 1 ];

    Decimal n = _number;

    char* z = ziffern + sizeof ziffern;
    int   i = _number.no_of_significant_digits();   //length();

    //while( n.digit( --i ) == 0  &&  i > 1 );

    int j = 0;
    *--z = '\0';
    while( j < i )
    {
        *--z = '0' + n.digit( j++ );

        if( j == -_scale ) {
            *--z = ',';
        }
    }

    if( *z == ',' )  *--z = '0';
    if( *z == '\0' ) *--z = '0';

    (*string_ptr) = z;
}

#endif
//-------------------------------------------------------------------------------read_ibm370_packed
#if 0

void read_ibm370_packed( Sos_binary_istream* s, Sos_number* number_ptr, uint length, int komma )
{
    read_ibm370_packed( s, &number_ptr->_number, length );
    number_ptr->_number.scale( -komma - number_ptr->_scale );
}

//------------------------------------------------------------------------------write_ibm370_packed

void write_ibm370_packed( Sos_binary_ostream* s, const Sos_number& number, uint length, int komma )
{
    Decimal decimal ( number._number );
    decimal.exp10( komma - number._scale );
    write_ibm370_packed( s, decimal, length );
}
#endif

#endif
