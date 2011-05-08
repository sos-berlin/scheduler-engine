// ibm370.h
//                                                      (c) Joacim Zschimmer


#ifndef __IBM370_H
#define __IBM370_H

#ifndef __EMU370_H
//    #include <emu370h.h>
#endif

#if !defined __AREA_H
    #include "../kram/area.h"
#endif

#if !defined __DECIMAL_H
    #include "../kram/decimal.h"
#endif


namespace sos {

//-----------------------------------------------------------------------------

struct e370_Virt_addr;

//-----------------------------------------------------------------------------

struct Halbwort {
    Byte h [2];

    Halbwort ()  {}
    Halbwort (int i) {
        h [0] = (Byte) (i >> 8);
        h [1] = (Byte) i;
        //h [0] = *significant_byte_ptr (&i, 1);
        //h [1] = *significant_byte_ptr (&i, 0);
    }
    operator int2 () const  { return ((int2) h [0] << 8) | (int2) h [1]; }
    operator Const_area () const  { return Const_area( h, sizeof h ); }
};

struct Uhalbwort {
    Byte h [2];

    Uhalbwort ()  {}
    Uhalbwort (uint i) {
        h [0] = (Byte) (i >> 8);
        h [1] = (Byte) i;
        //h [0] = *significant_byte_ptr (&i, 1);
        //h [1] = *significant_byte_ptr (&i, 0);
    }
    operator uint2 () const  { return ((uint2) h [0] << 8) | (uint2) h [1]; }
    operator Const_area () const  { return Const_area( h, sizeof h ); }
};

struct Uwort3 {
    Byte w [3];

    Uwort3 ()  {}
    Uwort3 (uint4 i) {
        w [0] = *significant_byte_ptr (&i, 2);
        w [1] = *significant_byte_ptr (&i, 1);
        w [2] = *significant_byte_ptr (&i, 0);
    }
    operator uint4 () const {
        uint4 i;
        *significant_byte_ptr (&i, 2) = w [0];
        *significant_byte_ptr (&i, 1) = w [1];
        *significant_byte_ptr (&i, 0) = w [2];
        return i;
    }
    operator Const_area () const  { return Const_area( w, sizeof w ); }
};

struct Wort4
{
    Byte                        w [4];

                                Wort4                   ();
                                Wort4                   ( uint4 );
                                Wort4                   ( Halbwort );
                                Wort4                   ( Uhalbwort );
                                operator uint4          () const;
                                operator Const_area     () const;

    void                        object_store            ( std::ostream& );
    Wort4&                      object_load             ( std::istream& );

};

//istream& operator >> ( istream&, Wort4& );
//ostream& operator << ( ostream&, const Wort4& );

struct Doppelwort8 {
    operator Const_area () const  { return Const_area( b, sizeof b ); }
  private:
    Byte b [8];
};

typedef Wort4   Wort;

//-----------------------------------------------------------------------------
#if !defined SYSTEM_WIN         // Provisorisch
    typedef Wort4 Adr;
    typedef Wort4 Addr;
#else

struct Ibm370
{
    struct Addr : Wort4
    {
        Addr()  {}
        Addr( e370_Virt_addr );
        Addr( int4 );
        operator e370_Virt_addr() const;
    };
}; // Ibm370

typedef Ibm370::Addr Addr;
typedef Ibm370::Addr Adr;

#endif

} //namespace sos

#include "ibm370.inl"

#endif
