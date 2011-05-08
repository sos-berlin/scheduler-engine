// ibm370.inl

namespace sos {

//------------------------------------------------------------------------Wort4

inline Wort4::Wort4 ()
{
}

inline Wort4::Wort4( uint4 i )
{
    w [0] = *significant_byte_ptr (&i, 3);
    w [1] = *significant_byte_ptr (&i, 2);
    w [2] = *significant_byte_ptr (&i, 1);
    w [3] = *significant_byte_ptr (&i, 0);
}

inline Wort4::Wort4( Halbwort h )
{
    w[ 0 ] = (signed char) h.h[ 0 ] >> 7;
    w[ 1 ] = w[ 0 ];
    w[ 2 ] = h.h[ 0 ];
    w[ 3 ] = h.h[ 1 ];
}

inline Wort4::Wort4( Uhalbwort h )
{
    w[ 0 ] = 0;
    w[ 1 ] = 0;
    w[ 2 ] = h.h[ 0 ];
    w[ 3 ] = h.h[ 1 ];
}

inline Wort4::operator uint4 () const
{
    uint4 i;
    *significant_byte_ptr (&i, 3) = w [0];
    *significant_byte_ptr (&i, 2) = w [1];
    *significant_byte_ptr (&i, 1) = w [2];
    *significant_byte_ptr (&i, 0) = w [3];
    return i;
    //return ((uint4) w [0] << 24)  |  ((uint4) w [1] << 16) |
    //       ((uint4) w [2] <<  8)  |  ((uint4) w [3]      );
}

inline Wort4::operator Const_area () const
{
    return Const_area( w, sizeof w );
}


#if defined SYSTEM_WIN

inline Ibm370::Addr::Addr( int4 value )
 : Wort4( value )
{}



#endif

} //namespace sos
