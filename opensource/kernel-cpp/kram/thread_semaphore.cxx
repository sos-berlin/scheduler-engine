// $Id: thread_semaphore.cxx 11394 2005-04-03 08:30:29Z jz $

#include "precomp.h"
#include "sos.h"
#include "thread_semaphore.h"
#include "log.h"
#include "msec.h"

#if 0

namespace sos {

//-------------------------------------------------------------------------------------------static
#ifdef SYSTEM_WIN

typedef BOOL (WINAPI *Proc)(LPCRITICAL_SECTION);

static volatile Proc      tryentercriticalsection = (Proc)-1;
static volatile HINSTANCE kernel32;

#endif    
//-------------------------------------------------------------------Thread_semaphore::Guard::Guard
/*
Thread_semaphore::Guard_with_log::Guard_with_log( Thread_semaphore* s, const string& debug )    
: 
    Guard(s),
    _debug(debug) 
{
    LOG( "Semaphore " << debug << " entered\n" ); 
    log_indent(+1);
}

//------------------------------------------------------------------Thread_semaphore::Guard::~Guard

Thread_semaphore::Guard_with_log::~Guard_with_log()    
{ 
    log_indent(-1);
    LOG( "Semaphore " << _debug << " leaved\n" ); 
}
*/
//-------------------------------------------------------------------------------------------------

void Thread_semaphore_with_log::enter()
{
    int ok = false;

    if( log_ptr )
    {
        ok = Thread_semaphore::try_enter();
        if( !ok )  LOG( "Semaphore " << _name << " locked. Waiting ...\n" );
    }
    
    string suffix;

    if( !ok )  
    {
        int start = elapsed_msec();
        
        Thread_semaphore::enter();
     
        int wait_msec = elapsed_msec() - start;
        if( wait_msec > 1 )  suffix = " after " + as_string(wait_msec/1000.0) + "s";
    }

    LOG( "Semaphore " << _name << " entered" << suffix << '\n' ); 
    log_indent(+1);
}

//-------------------------------------------------------------------------------------------------

void Thread_semaphore_with_log::leave()
{
    log_indent(-1);
    LOG( "Semaphore " << _name << " leave\n" ); 
    Thread_semaphore::leave();
}

//----------------------------------------------------------------------Thread_semaphore::try_enter
#ifdef SYSTEM_WIN

bool Thread_semaphore::try_enter()
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
        ok = t( &_semaphore ) != 0;
    }

    return ok;
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace sos
#endif
