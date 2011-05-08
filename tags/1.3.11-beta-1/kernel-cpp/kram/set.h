// Mengenoperationen                                    (c) SOS GmbH Berlin
//
// 20. 9.89     Joacim Zschimmer

#ifndef __SET_H
#define __SET_H

#include <assert.h>
#include <memory.h>

#ifndef __JZINCL_H
//   #include <jzincl.h>
#endif

namespace sos {

inline void set_incl (Byte set [], uint4 elem)  {
    set [ (uint) (elem >> 3) ] |= (Byte) (1 << ((Byte) elem & 7));
}


inline void set_excl (Byte set [], uint4 elem)  {
    set [ (uint) (elem >> 3) ] &= ~ (Byte) (1 << ((Byte) elem & 7));
}


inline Bool in_set (const Byte set [], uint4 elem)  {
    return ( set [ (uint) (elem >> 3) ] & (Byte) (1 << ((Byte) elem & 7)) ) != 0;
}


#if 0
#define set_union(set_union,set1,set2,bits)     \
                {int i = ((bits) + 7) >> 3;     \
                 while (i--) (set_union) [i] = (set1) [i] | (set2) [i];}

#define set_schnitt(set_schnitt,set1,set2,bits)     \
                {int i = ((bits) + 7) >> 3;         \
                 while (i--) (set_schnitt) [i] = (set1) [i] & (set2) [i];}

extern bool _fastcall set_disjunct (byte set1 [], byte set2 [], int size);

extern int  _fastcall set_cardinal (byte set [], int size);
#endif

//#pragma comment(lib,"jzlib.lib")


struct Bit_set {
    Bit_set( uint4 count );
    Bit_set( uint4 count, void* bit_map_ptr );
    ~Bit_set();

    Bool  valid       () const;
    void  include     ( uint4 elem );
    void  include     ( uint4 elem, uint4 count );
    void  exclude     ( uint4 elem );
    void  exclude     ( uint4 elem, uint4 count );
    int4  first_excluded() const  { return scan( 0, 0 ); }
    Bool  is_elem     ( uint4 elem ) const;
    void  unite       ( const Bit_set &set2 );
    void  cut         ( const Bit_set &set2 );
    uint4 cardinal    () const;
    Bool  disjunct    ( const Bit_set &set2 ) const;
    int4  scan        ( int4 start, int value ) const;
    int4  scan        ( int4 start, int value, int4 anzahl ) const;

  private:
    const uint  _size;
    const Bool  _delete;
    Byte* const _bit_map_ptr;
};

} //namespace sos

#include <set.inl>

#endif
