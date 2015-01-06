// $Id: z_posix_thread.cxx 13343 2008-01-29 15:56:01Z jz $

#include "zschimmer.h"
#include "z_posix.h"
#include "mutex.h"
#include "threads.h"


#ifdef Z_UNIX

#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>

#ifdef Z_SOLARIS
#   include <stropts.h>
#   include <sys/filio.h>       // ioctl( , FIONBIO )
#endif


namespace zschimmer {
namespace posix {

//----------------------------------------------------------------------------------thread_function

void* thread_function( void* param )
{
    Thread*          thread = (Thread*)param;


//  Ins Hauptprogramm (spooler.cxx)!?
    int              err;
    sigset_t         sigset;
    
    sigemptyset( &sigset );
    sigaddset( &sigset, SIGINT  );
    sigaddset( &sigset, SIGTERM );

    err = pthread_sigmask( SIG_BLOCK, &sigset, NULL );
    if( err )  throw_errno( err, "pthread_sigmask" );


    thread->_thread_pid = getpid();
  //Z_LOG( "Thread " << thread->_thread_name << " startet, pid=" << thread->_thread_pid << "\n" );

    thread->thread_call_main();

    return (void*)(long)thread->_thread_exit_code;
}

//----------------------------------------------------------------------------------Thread::~Thread

Thread::~Thread()
{
    try
    {
        thread_close();
    }
    catch( const exception& x ) { Z_LOG( "Thread::~Thread: " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------------------Thread::thread_close

void Thread::thread_close()
{
    if( _pthread_handle )
    {
        Z_LOG( "pthread_detach " << (void*)_pthread_handle << ' ' << _thread_name << '\n' );
        int err = pthread_detach( _pthread_handle );

//            if( err == 3 )  { Z_LOG( "pthread_detach errno=" << err << " " << strerror(err) << "\n" );  err = 0; }

        if( err )  throw_errno( err, "pthread_detach", _thread_name.c_str() );
    }

    _pthread_handle = 0;
    _thread_id = 0;
}

//-----------------------------------------------------------------------------Thread::thread_start

void Thread::thread_start()
{ 
    //pthread_attr_init( &_pthrad_attr );

    Z_LOG( "pthread_create " << _thread_name << '\n' );

    _thread_id = 0;
    _thread_is_running = true;

    int err = pthread_create( &_pthread_handle, NULL, &thread_function, this );
    if( err ) 
    {
        _thread_is_running = false;
        throw_errno( err, "pthread_create", _thread_name.c_str() );
    }

    _thread_id = _pthread_handle;
}

//------------------------------------------------------------------------------Thread::thread_init

void Thread::thread_init()
{
}

//------------------------------------------------------------------------------Thread::thread_exit

void Thread::thread_exit( int exit_code )
{
    _thread_exit_code = exit_code;
    _thread_is_running = false;
    if( _thread_termination_event )  _thread_termination_event->signal( "Thread " + _thread_name + " terminated" );

    //2008-01-29 "terminate called without an active exception":  pthread_exit( (void*)exit_code );
}

//--------------------------------------------------------------Thread::thread_wait_for_termination

void Thread::thread_wait_for_termination()
{
    if( _pthread_handle )
    {
        void* exit_code;

        Z_LOG( "pthread_join " << (void*)_pthread_handle << " (" << _thread_name << ") ...\n" );

        int err = pthread_join( _pthread_handle, &exit_code );
        if( err )  throw_errno( err, "pthread_join", _thread_name.c_str() );

        Z_LOG( "pthread_join " << (void*)_pthread_handle << " (" << _thread_name << ")  OK\n" );

        _pthread_handle = 0;
        _thread_id = 0;

        _thread_exit_code = (int)(long)exit_code;
    }
}

//-------------------------------------------------------------------Thread_specific_data::allocate

void Thread_specific_data::allocate()
{ 
    Z_LOG( "pthread_key_create()\n" ); 

    pthread_key_t key;

    int err = pthread_key_create( &key, NULL ); 
    if( err )  throw_errno( err, "pthread_key_create" );

    _key = key;
}

//-----------------------------------------------------------------------Thread_specific_data::free

void Thread_specific_data::free()
{ 
    if( _key != (pthread_key_t)-1 )
    {
        Z_LOG( "pthread_key_delete()\n" );
        
        pthread_key_delete( _key );
        
        _key = (pthread_key_t)-1; 
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
