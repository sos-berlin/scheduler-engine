// $Id: sosset.cxx 11394 2005-04-03 08:30:29Z jz $

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

#include "precomp.h"
//#define MODULE_NAME "set"
//                                              (c) SOS GmbH Berlin
//                                              Joacim Zschimmer                                

#include <limits.h>

#include "sos.h"
#include "sosset.h"

using namespace std;
namespace sos {

//-----------------------------------------------------------------------------Bit_set::Bit_set

Bit_set::Bit_set( uint4 count ) 
:
    _size        ( (uint) ((count + 7) / 8) ),
    _bit_map_ptr ( (Byte*)sos_alloc( _size, "Bit_set" ) ),
    _delete      ( true )
{
#   if defined SYSTEM_WIN16
        assert (count + 7 <= (uint4)UINT_MAX * 8);
#   endif

    if (valid ()) {
        memset (_bit_map_ptr, 0, _size);
    }
}

//-----------------------------------------------------------------------------Bit_set::Bit_set

Bit_set::Bit_set( uint4 count, void* bit_map_ptr ) 
:
    _size        ( (uint) ((count + 7) / 8) ),
    _bit_map_ptr ( (Byte*)bit_map_ptr ),
    _delete      ( false )
{
#   if defined SYSTEM_WIN16
        assert (count + 7 <= (uint4)UINT_MAX * 8);
#   endif
}

//-------------------------------------------------------------------------------Bit_set::unite

void Bit_set::unite(const Bit_set &set2) 
{
    uint i;
    uint m = min (_size, set2._size);

    for (i = 0; i < m; i++) {
        _bit_map_ptr [i] |= set2._bit_map_ptr [i];
    }
}

//---------------------------------------------------------------------------------Bit_set::cut

void Bit_set::cut(const Bit_set &set2) 
{
    uint i;
    uint m = min (_size, set2._size);

    for (i = 0; i < m; i++) {
        _bit_map_ptr [i] &= set2._bit_map_ptr [i];
    }
}

//------------------------------------------------------------------------------Bit_set::invert

void Bit_set::invert() 
{
    for( uint i = 0; i < _size; i++ )  _bit_map_ptr[ i ] = ~_bit_map_ptr[ i ];
}

//-----------------------------------------------------------------------------Bit_set::cardinal

uint4 Bit_set::cardinal() const  
{
    uint i, c = 0;
    for (i = 0; i < _size; i++) {
        if (is_elem (i)) c++;
    }
    return c;
}

//-----------------------------------------------------------------------------Bit_set::disjunct

Bool Bit_set::disjunct( const Bit_set &set2 ) const
{
    uint s = set2._size;

    while (s--) {
        if (_bit_map_ptr [s] & set2._bit_map_ptr [s])  return false;
    }
    return true;

}

//------------------------------------------------------------------------------Bit_set::include

void Bit_set::include( uint4 first_elem, uint4 count )
{
    if( first_elem == 0 ) 
    {
        int bytes = count / 8;
        int bits  = count % 8;
    
        memset( _bit_map_ptr, 0xFF, bytes );
        if( bits )  _bit_map_ptr[ bytes ] = (Byte) ~( 0xFF << bits );
    } 
    else 
    {
        for( int i = first_elem; i < first_elem + count; i++ )  include( i );
    }
}

//------------------------------------------------------------------------------Bit_set::exclude

void Bit_set::exclude( uint4 elem, uint4 count )
{
    for( int i = elem; i < elem + count; i ++ ) {
        exclude( i );
    }
}

//---------------------------------------------------------------------------------Bit_set::scan

int4 Bit_set::scan( int4 start, Bool value )  const
{
    int4 i;
# if 0                  // Arbeitet wortweise, muá aber noch getestet werden:
    int not_value_8 = value - 1;

    for( i = start; i & ~((int4)-1 << (8*sizeof(int))); i++ ) {
        if( is_elem( i ) == value )  return i;
    }
    for( i = i / 8 / sizeof(int); i < _size; i += sizeof(int) ) {
        if( *(int*)(_bit_map_ptr + i) != not_value_8 )  break;
    }
    for( i = i * 8 * sizeof(int); i < _size * 8; i++ ) {
        if( is_elem( i ) == value )  return i;
    }
# else
    for( i = start; i < _size * 8; i++ ) {
        if( ( is_elem( i ) != 0 ) == value )  return i;
    }
# endif
    return -1;
}

//--------------------------------------------------------------------------------Bit_set::scan

int4 Bit_set::scan( int4 start, Bool value, int4 anzahl ) const
{
    int4 i = start;
    int4 j;

    while( i < _size * 8 ) {
        i = scan( i, value );
        if( i == -1 )  return -1;

        for( j = i + 1; j < i + anzahl; j++ ) {
            if( ( is_elem( j ) != 0 ) != value )  break;
        }
        if( j == i + anzahl )  return i;
        i = j + 1;
    }
    return -1;
}



} //namespace sos
