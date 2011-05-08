// integer8.h                                    (c) SOS GmbH Berlin
//                                                   Joacim Zschimmer

#ifndef __INTEGER8_H
#define __INTEGER8_H

namespace sos
{

struct Sos_binary_istream;

struct Integer8
{
                                Integer8                ( int4 );

    friend void                 read_integer8           ( Sos_binary_istream*, Integer8* );
    friend void                 write_integer8          ( Sos_binary_ostream*, const Integer8& );

  private:
    int4                       _high4;
    uint4                      _low4;
};

//==========================================================================================inlines

inline Integer8::Integer8( int4 i )
:
    _high4 ( i < 0? -1 : 0  ),
    _low4  ( (uint4)i )
{
}

#if defined __SOSSTREA_H

inline void read_integer8( Sos_binary_istream* s, Integer8* integer8_ptr )
{
    read_int4 ( s, &integer8_ptr->_high4 );
    read_uint4( s, &integer8_ptr->_low4  );
}


inline void write_integer8( Sos_binary_ostream* s, const Integer8& integer8 )
{
    write_int4 ( s, integer8._high4 );
    write_uint4( s, integer8._low4  );
}

#endif

} //namespace sos

#endif
