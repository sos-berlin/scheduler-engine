/* soscont.h                                            (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

#ifndef SOSCONT_H
#define SOSCONT_H

// Das ist hier sehr provisorisch für winedit2.h:

template< class T >
struct Sos_container< T >
{
    virtual int                 count                   () const                        = 0;
  //virtual T                   operator []             ( int i )                       = 0;
    virtual string              name                    ( int i ) const                 = 0;
};

#endif
