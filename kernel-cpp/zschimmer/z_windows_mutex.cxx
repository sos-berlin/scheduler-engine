// $Id: z_windows_mutex.cxx 11394 2005-04-03 08:30:29Z jz $

#include "zschimmer.h"

#ifdef Z_WINDOWS

#include "z_windows_mutex.h"
#include <process.h>
#include "log.h"

namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------------------------------static

typedef BOOL (WINAPI *Proc)(LPCRITICAL_SECTION);

static volatile Proc      tryentercriticalsection = (Proc)-1;
static volatile HINSTANCE kernel32;

//---------------------------------------------------------------------------------Mutex::try_enter

bool Mutex::try_enter()
{
    bool ok = false;

    if( tryentercriticalsection == (Proc)-1 )
    {
        OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;

        GetVersionEx( &v );
        if( v.dwPlatformId == VER_PLATFORM_WIN32_NT )  // Windows NT oder Windows 2000?
        {
            kernel32 = LoadLibrary( "kernel32.dll" );
            if( kernel32 )  tryentercriticalsection = (Proc)GetProcAddress( kernel32, "TryEnterCriticalSection" );
        }

        if( tryentercriticalsection == (Proc)-1 )  tryentercriticalsection = NULL;
    }

    Proc t = tryentercriticalsection;
    if( t != NULL  &&  t != (Proc)-1 )  
    {
        ok = t( &_system_mutex ) != 0;
    }

    return ok;
}

//-------------------------------------------------------------------------Mutex::locking_thread_id

Thread_id Mutex::locking_thread_id()
{
    //return //Erst ab Windows 2003 Server: GetThreadId( _system_mutex.OwningThread );
    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif

