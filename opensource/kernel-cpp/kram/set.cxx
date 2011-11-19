#define MODULE_NAME "set"
//                                              (c) SOS GmbH Berlin
//                                              Joacim Zschimmer                                
#if 0     /* LÖSCHEN!! */

#include <limits.h>

#include <sos.h>
#include <set.h>

//-----------------------------------------------------------------------------

Bit_set::Bit_set( uint4 count ) :
    _size        ( (uint) ((count + 7) / 8) ),
    _bit_map_ptr ( new Byte [_size] ),
    _delete      ( true )
{
    assert (count + 7 <= (uint4)UINT_MAX * 8);
    if (valid ()) {
        memset (_bit_map_ptr, 0, _size);
    }
}

Bit_set::Bit_set( uint4 count, void* bit_map_ptr ) :
    _size        ( (uint) ((count + 7) / 8) ),
    _bit_map_ptr ( (Byte*)bit_map_ptr ),
    _delete      ( false )
{
    assert (count + 7 <= (uint4)UINT_MAX * 8);
}


void Bit_set::unite (const Bit_set &set2) {
    uint i;
    uint m = min (_size, set2._size);

    for (i = 0; i < m; i++) {
        _bit_map_ptr [i] |= set2._bit_map_ptr [i];
    }
}

void Bit_set::cut (const Bit_set &set2) {
    uint i;
    uint m = min (_size, set2._size);

    for (i = 0; i < m; i++) {
        _bit_map_ptr [i] &= set2._bit_map_ptr [i];
    }
}


uint4 Bit_set::cardinal () const  {
    uint i, c = 0;
    for (i = 0; i < _size; i++) {
        if (is_elem (i)) c++;
    }
    return c;
}


Bool Bit_set::disjunct( const Bit_set &set2 ) const
{
    uint s = set2._size;

    while (s--) {
        if (_bit_map_ptr [s] & set2._bit_map_ptr [s])  return false;
    }
    return true;

} //Bit_set::is_disjunct

//-------------------------------------------------------------Bit_set::include

void Bit_set::include( uint4 elem, uint4 count )
{
    for( int i = elem; i < elem + count; i ++ ) {
        include( i );
    }
}

//-------------------------------------------------------------Bit_set::exclude

void Bit_set::exclude( uint4 elem, uint4 count )
{
    for( int i = elem; i < elem + count; i ++ ) {
        exclude( i );
    }
}

//----------------------------------------------------------------Bit_set::scan

int4 Bit_set::scan( int4 start, int value )  const
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

//----------------------------------------------------------------Bit_set::scan

int4 Bit_set::scan( int4 start, int value, int4 anzahl ) const
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

#endif

