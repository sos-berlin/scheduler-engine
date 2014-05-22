//#define MODULE_NAME "sleep"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include <math.h>
#include "../kram/sysdep.h"

#if defined SYSTEM_UNIX
#   include <unistd.h>
#endif
#if defined SYSTEM_WIN32
#   include <windows.h>
#endif

#include "../kram/sos.h"
#include "../kram/sleep.h"

using namespace std;
namespace sos {

void sos_sleep( double seconds )
{
    Z_LOG2("sleep", seconds << "s\n");

    #if defined SYSTEM_WIN32
        Sleep( (uint4)ceil( seconds * 1000 ) );
    #else
        struct timespec t;
        t.tv_sec = (time_t)floor( seconds );
        t.tv_nsec = (time_t)max( 0.0, ( seconds - floor( seconds ) ) * 1e9 );
        nanosleep( &t, NULL );
    #endif
}

} //namespace sos
