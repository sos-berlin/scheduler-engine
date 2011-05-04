// $Id$

#include "zschimmer.h"
#include "z_posix.h"
#include "mutex.h"
#include "threads.h"
#include "log.h"


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


const pthread_cond_t  pthread_cond_initial    = PTHREAD_COND_INITIALIZER;


namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------Event::Event

Event::Event( const string& name )
: 
    _zero_(this+1),
    _pthread_cond(pthread_cond_initial)
{
    _name = name;
    create();
}

//------------------------------------------------------------------------------------Event::~Event

Event::~Event()
{
    close();
}

//-------------------------------------------------------------------------------------Event::close

void Event::close()
{
    if( _created )
    {
        int err = pthread_cond_destroy( &_pthread_cond );
        if( err )  Z_LOG( "pthread_cond_destroy err=" << err << '\n' );

        _created = false;
    }
}

//------------------------------------------------------------------------------------Event::create

void Event::create()
{
    int err = pthread_cond_init( &_pthread_cond, NULL );
    if( err )  throw_errno( err, "pthread_cond_init", name().c_str() );

    _created = true;
}

//-------------------------------------------------------------------------------------Event::signal

void Event::signal( const string& name )
{
    Z_MUTEX( _mutex )
    {
        _signal_name = name;
        _signaled    = true;

        if( _waiting )
        {
            Z_LOG( "pthread_cond_signal " << _name << " \"" << _signal_name << "\"\n" );

            int err = pthread_cond_signal( &_pthread_cond );
            if( err )  throw_errno( err, "pthread_cond_signal", _name.c_str(), name.c_str() );
        }
    }
}

//------------------------------------------------------------------------------Event::async_signal

void Event::async_signal( const char* )
{
    // pthread_mutex_lock:
    // The  mutex  functions  are  not  async-signal  safe.  What  this  means  is  that  they
    // should  not  be  called from  a signal handler. In particular, calling pthread_mutex_lock 
    // or pthread_mutex_unlock from a signal handler may deadlock the calling thread.

    _signaled = true;
}

//------------------------------------------------------------------------Event::signaled_then_reset

bool Event::signaled_then_reset()
{
    if( !_signaled )  return false;
    
    bool signaled = false;

    Z_MUTEX( _mutex )
    {
        signaled = _signaled;
        reset();
    }

    return signaled;
}

//-------------------------------------------------------------------------------------Event::reset

void Event::reset()
{
    Z_MUTEX( _mutex )
    {
      //Z_DEBUG_ONLY( Z_LOG( "Ereignis " << _name << "  reset()\n" ) );

        _signaled = false;
        _signal_name = "";

        //BOOL ok = ResetEvent( _handle );
        //if( !ok )  throw_mswin( "ResetEvent", _name.c_str() );
    }
}

//---------------------------------------------------------------------------------------Event::wait

void Event::wait()
{
    Z_MUTEX( _mutex )
    {
        if( _signaled )  return;

        _waiting++;
        
        Z_DEBUG_ONLY( Z_LOG( "pthread_cond_wait " << _name << "\n" ) );
        int err = pthread_cond_wait( &_pthread_cond, &_mutex._system_mutex );   // _mutex wird hier freigegeben und wieder gesperrt!
        
        _waiting--;

        if( err )
        {
            if( err == EINTR )  Z_LOG( "err=EINTR\n" );
            else throw_errno( err, "pthread_cond_wait", _name.c_str() );
        }
        else
        {
            Z_DEBUG_ONLY( Z_LOG( "pthread_cond_wait: " << as_text() << " signalisiert!\n" ) );
        }
    }
}

//--------------------------------------------------------------------------------------Event::wait

bool Event::wait( double seconds )
{
    Z_MUTEX( _mutex )
    {
        if( _signaled )  { 
            //Z_LOG( "pthread_cond_timedwait() nicht gerufen, weil _signaled==true\n" ); 
            return true; 
        }

        _waiting++;

        struct timeval  now;
        struct timespec t;
        struct timezone tz;
        
        gettimeofday( &now, &tz );

        memset( &t, 0, sizeof t );
        timespec_add( &t, seconds );
        timespec_add( &t, now );

        Z_DEBUG_ONLY( Z_LOG( "pthread_cond_timedwait(" << seconds << ") " << _name << '\n' ) );

        int err = pthread_cond_timedwait( &_pthread_cond, &_mutex._system_mutex, &t );

        _waiting--;

        if( err )  
        {
            if( err == ETIMEDOUT )  return false;
            if( err == EINTR     )  { Z_LOG( "pthread_cond_timedwait() err=EINTR\n" ); return true; }
            throw_errno( err, "pthread_cond_timedwait", _name.c_str() );
        }

        Z_DEBUG_ONLY( Z_LOG( "pthread_cond_timedwait: " << as_text() << " signalisiert!\n" ) );
    }

    return true;
}

//-----------------------------------------------------------------------Select_event::Select_event
/*

Select_event::Select_event( const string& name )
: 
    _zero_(this+1)
{
    _name = name;
    create();
}

//----------------------------------------------------------------------Select_event::~Select_event

Select_event::~Select_event()
{
    close();
}

//-------------------------------------------------------------------------------Select_event::init

void Select_event::init()
{
    _socket_pair[0] = -1;
    _socket_pair[1] = -1;
}

//------------------------------------------------------------------------------Select_event::close

void Select_event::close()
{
    if( _socket_pair[0] != -1 )
    {
        ::close( _socket_pair[0] );  _socket_pair[0] = -1;
        ::close( _socket_pair[1] );  _socket_pair[1] = -1;
    }
}

//-----------------------------------------------------------------------------Select_event::create

void Select_event::create()
{
    int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, _socket_pair );
    if( ret == -1 )  throw_errno( errno, "Select_event::create() socketpair", name().c_str() );


    for( int i = 0; i <= 1; i++ ) 
    {
        unsigned long on = 1;
        int ret = ioctl( _socket_pair[i], FIONBIO, &on );
        if( ret == -1 )  throw_socket( errno, "Select_event::create() ioctl(FIONBIO)" );
    }
}

//------------------------------------------------------------------------------Select_event::signal

void Select_event::signal( const string& name )
{
    Z_MUTEX( _mutex )
    {
        _signal_name = name;

        if( !_signaled )
        {
            _signaled  = true;

            //if( _waiting )
            {
                int err = ::send( _socket_pair[0], "x", 1, 0 );
                // Auf Linux liefert send() immer EPERM. Warum?
                if( err )  throw_errno( err, "Select_event::signal send", _name.c_str(), name.c_str() );
            }
        }
    }
}

//-----------------------------------------------------------------------Select_event::async_signal

void Select_event::async_signal( const char* )
{
    // pthread_mutex_lock:
    // The  mutex  functions  are  not  async-signal  safe.  What  this  means  is  that  they
    // should  not  be  called from  a signal handler. In particular, calling pthread_mutex_lock 
    // or pthread_mutex_unlock from a signal handler may deadlock the calling thread.

    _signaled = true;
}

//-----------------------------------------------------------------Select_event::signaled_then_reset

bool Select_event::signaled_then_reset()
{
    if( !_signaled )  return false;
    
    bool signaled = false;

    Z_MUTEX( _mutex )
    {
        signaled = _signaled;
        reset();
    }

    return signaled;
}

//------------------------------------------------------------------------------Select_event::reset

void Select_event::reset()
{
    Z_MUTEX( _mutex )
    {
      //Z_DEBUG_ONLY( Z_LOG( "Ereignis " << _name << "  reset()\n" ) );

        if( _signaled )
        {
            _signaled = false;
            _signal_name = "";

            char buffer[10];

            while(1)
            {
                int ret = ::recv( _socket_pair[1], buffer, sizeof buffer, 0 );
                if( ret == -1 )
                {
                    if( errno == EAGAIN )  break;
                    throw_errno( errno, "Select_event::reset() recv", _name.c_str() );
                }
                if( ret < sizeof buffer )  break;
            }
        }
    }
}

//--------------------------------------------------------------------------------Select_event::wait

void Select_event::wait()
{
    Z_MUTEX( _mutex )
    {
        if( _signaled )  return;

      //_waiting++;
        
            #if FD_SETSIZE != Z_FDSETSIZE
            #   error FD_SETSIZE ist geändert worden!
            #endif

            if( _socket_pair[1] >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", _socket_pair[1], FD_SETSIZE, Z_FUNCTION );
            fd_set readfds;  FD_ZERO( &readfds );
            FD_SET( _socket_pair[1], &readfds );

            int ret = ::select( _socket_pair[1] + 1, &readfds, NULL, NULL, NULL );
            if( ret < 0 )  throw_errno( errno, "Select_event::wait() select", _name.c_str() );

            _signaled = true;
        
      //_waiting--;

    }
}

//--------------------------------------------------------------------------------Select_event::wait

bool Select_event::wait( double seconds )
{
    int ret = 0;

    Z_MUTEX( _mutex )
    {
        if( _signaled )  return true;

      //_waiting++;
        
            timeval t = timeval_from_seconds( seconds );

            if( _socket_pair[1] >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", _socket_pair[1], FD_SETSIZE, Z_FUNCTION );
            fd_set readfds;  FD_ZERO( &readfds );
            FD_SET( _socket_pair[1], &readfds );

            Z_LOG( "select(,{" << _socket_pair[1] << "}," << seconds << ")\n" );

            ret = ::select( _socket_pair[1] + 1, &readfds, NULL, NULL, &t );
            if( ret < 0 )  throw_errno( errno, "Select_event::wait() select", _name.c_str() );

            _signaled = ret > 0;
        
      //_waiting--;

    }

    return ret > 0;
}

*/
//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
