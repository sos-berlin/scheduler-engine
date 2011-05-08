#include "precomp.h"
//#define MODULE_NAME "pointer"
// pointer.cpp                                        (c) SOS GmbH Berlin
//                                                        Joacim Zschimmer

//#pragma implementation

#include <stdlib.h>

#if defined( SYSTEM_DOS )
#   include <dos.h>
#endif

#include "../kram/sos.h"

#if !defined SYSTEM_GNU

#include "../kram/pointer.h"

using namespace std;
namespace sos {

//---------------------------------------------------------------------------------------valid2

inline Bool valid2( const void* ptr, int size )
// Siehe auch inline valid() in pointer.h
{
#   if defined( SYSTEM_DOS )
        return ( (long) FP_OFF( ptr ) + object_size - 1 <= 0xFFFF
               &&  FP_SEG( ptr ) > 0x0800
               &&  (long) FP_SEG( ptr ) * 16 + FP_OFF( ptr ) < 0xA0000 );
#   else
        return ptr != 0;
#   endif
}

//-------------------------------------------------------------------------------------checked2

const void* checked2( const void* ptr, int object_size, const char* debug_info )
{
    if( !valid2( ptr, object_size )) {
        SHOW_ERR( "*** Invalid Pointer " << (void*)ptr << "in \"" << debug_info << "\" ***" );
        //abort();
        exit( 99 );     // jz 5.7.95
    }
    return ptr;
}

} //namespace sos

#endif
