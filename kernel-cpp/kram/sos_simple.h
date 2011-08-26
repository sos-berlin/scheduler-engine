// $Id: sos_simple.h 11394 2005-04-03 08:30:29Z jz $


#ifndef __SOS_SIMPLE_H
#define __SOS_SIMPLE_H

#include <string.h>

namespace sos {

//-------------------------------------------------------------------------------------Fill_end

struct Fill_end
{
};

//------------------------------------------------------------------------------------Fill_zero

struct Fill_zero
{
    Fill_zero( void* ende )
    {
        memset( this + 1, 0, (char*)ende - (char*)( this+1 ) );
    }

    Fill_zero( const Fill_end& ende )
    {
        int length = (char*)&ende - (char*)( this+1 );
        assert( length >= 0 );
        memset( this + 1, 0, length );
    }
};

//---------------------------------------------------------------------------------------------

} //namespace sos

#endif
