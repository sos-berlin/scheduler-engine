// Mengenoperationen                                    (c) SOS GmbH Berlin
//
// 20. 9.89     Joacim Zschimmer

#ifndef __SOSSET_H
#define __SOSSET_H

#include <assert.h>

#ifndef SYSTEM_GNU
#   include <memory.h>
#endif

namespace sos
{

inline void set_incl (Byte set [], uint4 elem)  {
    set [ (uint) (elem >> 3) ] |= (Byte) (1 << ((Byte) elem & 7));
}


inline void set_excl (Byte set [], uint4 elem)  {
    set [ (uint) (elem >> 3) ] &= ~ (Byte) (1 << ((Byte) elem & 7));
}


inline Bool in_set (const Byte set [], uint4 elem)  {
    return ( set [ (uint) (elem >> 3) ] & (Byte) (1 << ((Byte) elem & 7)) ) != 0;
}



struct Bit_set
{
                                Bit_set                 ( uint4 count );
                                Bit_set                 ( uint4 count, void* bit_map_ptr );
                               ~Bit_set                 ();

    Bool                        operator []             ( uint4 i ) const               { return is_elem( i ); }
    Bit_set&                    operator |=             ( const Bit_set& o )            { unite( o ); return *this; }
    Bit_set&                    operator &=             ( const Bit_set& o )            { cut( o ); return *this; }

    Bool                        valid                   () const;
    void                        clear                   ()                              { memset( _bit_map_ptr, 0, _size ); }
    void                        include                 ( uint4 elem );
    void                        include                 ( uint4 elem, uint4 count );
    void                        exclude                 ( uint4 elem );
    void                        exclude                 ( uint4 elem, uint4 count );
    int4                        first_excluded          () const  { return scan( 0, 0 ); }
    inline Bool                 is_elem                 ( uint4 elem ) const;
    void                        unite                   ( const Bit_set &set2 );
    void                        cut                     ( const Bit_set &set2 );
    void                        invert                  ();
    uint4                       cardinal                () const;
    Bool                        disjunct                ( const Bit_set &set2 ) const;
    int4                        scan                    ( int4 start, Bool value ) const;
    int4                        scan                    ( int4 start, Bool value, int4 anzahl ) const;

  protected:
    const uint                 _size;
    Bool                       _delete;
    Byte*                      _bit_map_ptr;
};


//---------------------------------------------------------------------------Sos_set<TYPE,SIZE>

template< int SIZE >
struct Sos_set : Bit_set
{
                                Sos_set                 ();
                                Sos_set                 ( const Sos_set<SIZE>& o );

    Sos_set<SIZE>&              operator =              ( const Sos_set<SIZE>& o ); //???       { memcpy( _bits, o._bits, sizeof _bits ); _bit_map_ptr = _bits; return *this; }

    Sos_set<SIZE>&              operator |=             ( const Sos_set<SIZE>& o )            { unite( o ); return *this; }
    Sos_set<SIZE>&              operator &=             ( const Sos_set<SIZE>& o )            { cut( o ); return *this; }
    Sos_set<SIZE>               operator |              ( const Sos_set<SIZE>& o )            { Sos_set<SIZE> set; set |= o; return set; }
    Sos_set<SIZE>               operator &              ( const Sos_set<SIZE>& o )            { Sos_set<SIZE> set; set &= o; return set; }
    Sos_set<SIZE>               operator ~              ()                                    { Sos_set<SIZE> set = *this; set.invert(); return set; }

  private:
    Byte                       _bits [ ( SIZE + 7 ) / 8 ];
};

//--------------------------------------------------------------------------Sos_set::Sos_set()

template<int SIZE>
inline Sos_set<SIZE>::Sos_set()
:
    Bit_set ( SIZE, _bits )
{
    memset( _bits, 0, sizeof _bits );
}

//--------------------------------------------------------------------------Sos_set::Sos_set()

template<int SIZE>
inline Sos_set<SIZE>::Sos_set( const Sos_set<SIZE>& o )
:
    Bit_set ( SIZE, _bits )
{
    memcpy( _bits, o._bits, sizeof _bits );
    _bit_map_ptr = _bits;
}

} //namespace sos


#include "sosset.inl"

#endif
