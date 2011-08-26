// $Id: z_md5.h 13199 2007-12-06 14:15:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_Z_MD5_H
#define __ZSCHIMMER_Z_MD5_H

#include "z_io.h"

namespace zschimmer {

const int                       md5_sum_byte_count          = 16;

//----------------------------------------------------------------------------------------------Md5

struct Md5
{
                                Md5                         ()                                      { memset( _bytes, 0, sizeof _bytes ); }

                                operator string             () const                                { return to_string(); }
    bool                        operator ==                 ( const Md5& o ) const                  { return memcmp( _bytes, o._bytes, sizeof _bytes ) == 0; }
    bool                        operator !=                 ( const Md5& o ) const                  { return !( *this == o ); }

    void                    set_hex                         ( const string& hex );
    string                      to_string                   () const                                { return lcase_hex( _bytes, sizeof _bytes ); }

  private:
    friend struct               Md5_calculator;

    Byte                       _bytes[ md5_sum_byte_count ];
};


//-----------------------------------------------------------------------------------Md5_calculator

struct Md5_calculator
{
                                Md5_calculator              ();

    void                        add                         ( const io::Byte_sequence& );
    Md5                         md5                         ();

  private:
    third_party::md5::MD5_CTX  _md5_context;
    bool                       _finished;
};

//-------------------------------------------------------------------------------------------------

Md5                             md5                         ( const io::Byte_sequence& );

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
