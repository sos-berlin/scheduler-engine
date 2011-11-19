// $Id: z_windows_thread.cxx 13786 2009-04-28 08:03:45Z jz $

#include "zschimmer.h"

#ifdef Z_WINDOWS

#include <process.h>
#include "threads.h"
#include "log.h"

namespace zschimmer {
namespace windows {


//------------------------------------------------------------------------------------SetThreadName

typedef struct tagTHREADNAME_INFO
{
    DWORD   dwType;         // must be 0x1000
    LPCSTR  szName;         // pointer to name (in user addr space)
    DWORD   dwThreadID;     // thread ID (-1=caller thread)
    DWORD   dwFlags;        // reserved for future use, must be zero
} THREADNAME_INFO;


static void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName )
{
#   ifdef _DEBUG        
        THREADNAME_INFO info;

        info.dwType     = 0x1000;
        info.szName     = szThreadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags    = 0;

        __try
        {
            RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );     // Übergibt den Namen an den Debugger
        }
        __except(EXCEPTION_CONTINUE_EXECUTION)
        {
        }
#   endif
}

//----------------------------------------------------------------------------------thread_function

uint __stdcall thread_function( void* param )
{
    Thread* thread = (Thread*)param;

    thread->thread_call_main();
    return thread->_thread_exit_code;
}

//----------------------------------------------------------------------------------Thread::~Thread

Thread::~Thread()
{
    thread_close();
}

//-----------------------------------------------------------------------------Thread::thread_close

void Thread::thread_close()
{
    if( _thread_handle )  
    {
        Z_LOG( "TerminateThread " << _thread_name << "\n" );
        TerminateThread( _thread_handle, 99 );
    }

    _thread_handle.close();
}

//-----------------------------------------------------------------------------Thread::thread_start

void Thread::thread_start()
{ 
    Z_LOG( "_beginthreadex()  CreateThread " << _thread_name << "\n" );
    _thread_handle._handle = convert_to_noninheritable_handle( (HANDLE)_beginthreadex( NULL, 0, thread_function, this, 0, &_thread_id ) );
    if( !_thread_handle._handle )  throw_mswin( "CreateThread", _thread_name.c_str() ); 

    //_event.set_handle( _thread_handle );
}

//------------------------------------------------------------------------------Thread::thread_init

void Thread::thread_init()
{
    if( !_thread_name.empty() )  SetThreadName( -1, _thread_name.c_str() );
}

//------------------------------------------------------------------------------Thread::thread_exit

void Thread::thread_exit( int exit_code )
{
    assert( current_thread_id() == _thread_id );

    _thread_exit_code = exit_code;
    //if( _thread_termination_event )  _thread_termination_event->signal( "Thread " + _thread_name + " terminated" );

    Z_LOG( "_endtreadex(" << exit_code << ")\n" );
    _endthreadex( exit_code );
}

//-------------------------------------------------------------Thread::thread_wait_for_termination 

bool Thread::thread_wait_for_termination( double wait_time )
{
    if( !_thread_handle )  return true;

    return _thread_handle.wait( wait_time );
}

//------------------------------------------------------------------------Thread::thread_is_running

bool Thread::thread_is_running()
{
    if( !_thread_handle.valid() )  return false;
    
    DWORD exit_code;

    BOOL ok = GetExitCodeThread( _thread_handle, &exit_code );
    if( !ok )  throw_mswin( "GetExitCodeThread", _thread_name.c_str() );

    return exit_code == STILL_ACTIVE;
}

//--------------------------------------------------------------------------Thread::set_thread_name

void Thread::set_thread_name( const string& name )
{ 
    assert( !_thread_id );

    Thread_base::set_thread_name( name );

    //if( _thread_id )  SetThreadName( _thread_id, name.c_str() );
}

//---------------------------------------------------------------------------Thread::thread_as_text

string Thread::thread_as_text() const
{
    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME kernel_time;
    FILETIME user_time;
    DWORD    exit_code;
    BOOL     ok;
    char     buffer [200];
    string   result;


    ok = GetExitCodeThread( _thread_handle, &exit_code );
    if( ok )  if( exit_code == STILL_ACTIVE )  result += "active";
                                         else  result = "terminated with exit code " + as_string(exit_code);
        else  result = "Unbekannter Thread " + printf_string( "%X", (size_t)_thread_handle.handle() );

    ok = GetThreadTimes( _thread_handle, &creation_time, &exit_time, &kernel_time, &user_time );
    if( ok )
    {
        z_snprintf( buffer, sizeof buffer, ", user_time=%-0.10g, kernel_time=%-0.10g", 
                    (double)windows::int64_from_filetime( user_time   ) / 1E7,
                    (double)windows::int64_from_filetime( kernel_time ) / 1E7 );

        result += buffer;
    }

    return result;
}

//-------------------------------------------------------------------Thread_local_storage::allocate

void Thread_local_storage::allocate()
{ 
    //Z_LOG( "TlsAlloc()\n" ); 

    _index = TlsAlloc(); 

    if( _index == TLS_OUT_OF_INDEXES )  throw_mswin( "TlsAlloc" );
}

//-----------------------------------------------------------------------Thread_local_storage::free

void Thread_local_storage::free()
{ 
    if( _index != TLS_OUT_OF_INDEXES )  
    {
        //Z_LOG( "TlsFree()\n" ); 
    
        TlsFree( _index );
        
        _index = TLS_OUT_OF_INDEXES; 
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif

