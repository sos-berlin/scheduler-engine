// $Id: z_posix.cxx 13257 2008-01-04 14:47:23Z jz $

#include "zschimmer.h"
#include "z_posix.h"
#include "mutex.h"


#ifdef Z_UNIX

#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>

#ifdef Z_SOLARIS
#   include <stropts.h>
#   include <sys/filio.h>       // ioctl( , FIONBIO )
#endif

using namespace std;
using namespace zschimmer;

//-------------------------------------------------------------------------------------------static

static posix::Undestroyable_mutex interlocked_increment_mutex ( "InterlockedIncrement", Mutex::kind_nonrecursive );    // Für InterlockedIncrement() und InterlockedDecrement()

//-----------------------------------------------------------------------------InterlockedIncrement

long32 InterlockedIncrement( long32* i )  
{ 
    long32 result = 0;

    //Z_MUTEX( interlocked_increment_mutex )
    {
        interlocked_increment_mutex.enter();
        (*i)++; 
        result = *i;
        interlocked_increment_mutex.leave();
    }

    return result;
}

//-----------------------------------------------------------------------------InterlockedDecrement

long32 InterlockedDecrement( long32* i )  
{ 
    long result = 0;

    //Z_MUTEX( interlocked_increment_mutex )
    {
        interlocked_increment_mutex.enter();
        (*i)--; 
        result = *i; 
        interlocked_increment_mutex.leave();
    }

    return result;
}

//---------------------------------------------------------------------------------------strerror_s

errno_t strerror_s( char* buffer, size_t buffer_size, errno_t errn )
{
    errno_t result = 0;

    if( buffer_size > 0 )  buffer[0] = '\0';

#   if defined Z_AIX || defined Z_SOLARIS || defined Z_HPUX
        strncpy( buffer, strerror( errn ), buffer_size );
#   elif 0  // Warum liefert strerror_r() nicht int?
        int ret = strerror_r( errn, buffer, buffer_size );
        result = ret == 0? 0 : errno;
#   else
        const char* p = strerror_r( errn, buffer, buffer_size );
        if( p && p != buffer )  strncpy( buffer, p, buffer_size );
        result = p == NULL? 0 : errno;
#   endif

    if( buffer_size > 0 )  buffer[ buffer_size - 1 ] = '\0';
    
    return result;
}

//-------------------------------------------------------------------------------------------------

namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------------------

void timespec_add( timespec* ts, double seconds )
{
    ts->tv_sec = (time_t)floor( seconds );
    ts->tv_nsec = (time_t)max( 0.0, ( seconds - floor( seconds ) ) * 1e9 );
}

//-------------------------------------------------------------------------------------------------

void timespec_add( timespec* ts, const timeval& tv )
{
    int u = ts->tv_nsec / 1000 + tv.tv_usec;
    ts->tv_nsec = u % 1000000 * 1000  + ts->tv_nsec % 1000;
    ts->tv_sec += tv.tv_sec + u / 1000000;
}

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
