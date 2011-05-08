// ascii.h

#ifndef __ASCII_H
#define __ASCII_H

struct Ascii_char
{
    Ascii_char( unsigned char a     ) : value ( a )  {}
    Ascii_char( const Ebcdic& );

    operator unsigned char () const  { return value; }

    unsigned char value;
};

#endif
