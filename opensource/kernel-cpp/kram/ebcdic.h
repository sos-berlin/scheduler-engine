// ebcdic.h                                             (c) SOS GmbH Berlin

#if 0

#ifndef __EBCDIC_H
#define __EBCDIC_H

#pragma interface

#include <chkarray.h>

struct Ebcdic_char
{
    Ebcdic_char() : value( 0 )  {}
    Ebcdic_char( char a     ) : value ( iso2ebc[ (unsigned char) a ] )  {}
    //Ebcdic_char( int  i = 0 ) : value ( (unsigned char) i )  {}

    operator unsigned char () const  { return ebc2iso[ value ]; }

    unsigned char value;
};

template< int mini, int maxi >
struct Ebcdic_array : Checked_array< Ebcdic_char, mini, maxi >
{
     Ebcdic_array();
     Ebcdic_array( const char* string0 );
};


#include <ebcdic.inl>

#endif
#endif
