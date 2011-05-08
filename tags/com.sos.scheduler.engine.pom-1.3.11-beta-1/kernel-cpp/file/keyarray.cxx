//#define MODULE_NAME "keyarray"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
#include "precomp.h"

#if 0

#include <sos.h>
#include <area.h>
#include <sosfield.h>
#include <keyarray.h>

//-----------------------------------------------Fixed_length_key_array::Fixed_length_key_array

Fixed_length_key_array::Fixed_length_key_array( const Sos_ptr<Field_type>& type )
:
    _zero_ (this+1),
    _type ( type )
{
    _array.obj_const_name( "Fixed_length_key_array._array" );
    int a = type->alignment();
    _entry_size = ( type->field_size() + a - 1 ) / a * a;
    //_allocation_increment = 100 * _entry_size;
}

//----------------------------------------------Fixed_length_key_array::~Fixed_length_key_array

Fixed_length_key_array::~Fixed_length_key_array()
{
    for( int i = _array.last_index(); i >= _array.first_index(); i-- ) {
        _type->destruct( _array[ i ] );
        SOS_DELETE( _array[ i ] );
    }
/*
    Byte* p0 = _buffer.byte_ptr();
    Byte* p  = p0 + _buffer.length();

    while( p > p0 ) {
        p -= _entry_size;
        _type->destruct( p );
    }
*/
}

//-----------------------------------------------------------Fixed_length_key_array::last_index
/*
void Fixed_length_key_array::last_index( int last )
{
    _buffer.resize_min( ( last - _first_index ) * _entry_size );
}
*/
//------------------------------------------------------------------Fixed_length_key_array::add

void Fixed_length_key_array::add( const Byte* p )
{
    Byte** e = _array.add_empty();

    *e = new Byte [ _entry_size ];          // ausgerichtet, wie von type->alignment() gefordert?
    if( !*e )  throw No_memory_error();
    _type->field_copy( *e, p );
/*
    if( _buffer.length() + _entry_size > _buffer.size() ) {
        _buffer.resize_min( _buffer.length() + 4096 );
    }

    _buffer.length( _buffer.length() + _entry_size );
    _type->field_copy( _buffer.byte_ptr() + _buffer.length(), p );
*/
}

//-----------------------------------------------------------Fixed_length_key_array::operator[]

int Fixed_length_key_array::operator[] ( const Byte* key )
{
    for( int i = _array.first_index(); i <= _array.last_index(); i++ ) {
        if( _type->field_equal( _array[ i ], key ) )  return i;
    }
/*
    char key_text [ 100+1 ];    key_text[ sizeof key_text - 1 ] = '\0';
    char text     [ 100+1 ];    text[ sizeof text - 1 ] = '\0';
    Text_format format;

    format.string_quote( 0 );
    format.char_quote( 0 );
    format.separator( 0 );

    ostrstream s1 ( key_text, sizeof key_text - 1 );
    _type->print( key, &s, format );
    *s << '\0';

    for( int i = _array.first_index(); i <= _array.last_index(); i++ ) {
        _key.pürint
    }
*/
/*
    const Byte* p     = _buffer.byte_ptr();
    const Byte* p_end = _buffer.byte_ptr() + _buffer.length();

    while( p < p_end ) {
        if( memcmp( key, p, _entry_size ) == 0 )  return _first_index + ( p - _buffer.byte_ptr() ) / _entry_size;
        p += _entry_size;
    }
*/

    throw_not_found_error();
}

#endif
