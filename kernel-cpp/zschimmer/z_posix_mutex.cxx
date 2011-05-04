// $Id$

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
#   define PTHREAD_MUTEX_FAST_NP  PTHREAD_MUTEX_RECURSIVE    // Solaris kennt PTHREAD_MUTEX_FAST_NP nicht
#endif

#ifdef Z_HPUX
#   define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_ERRORCHECK    // HP kennt PTHREAD_MUTEX_FAST_NP nicht
#endif

//-------------------------------------------------------------------------------------------------

namespace zschimmer {
namespace posix {

//--------------------------------------------------------------------------------------enter_mutex

void enter_mutex( pthread_mutex_t* mutex )
{
//fprintf(stderr,"pthread_mutex_lock(%p)\n", (void*)mutex );
    int err = pthread_mutex_lock( mutex );
    if( err )  throw_errno( err, "pthread_mutex_lock" );
}

//--------------------------------------------------------------------------------------leave_mutex

void leave_mutex( pthread_mutex_t* mutex )
{
//fprintf(stderr,"pthread_mutex_unlock(%p)\n", (void*)mutex );
    int err = pthread_mutex_unlock( mutex );
    if( err )  throw_errno( err, "pthread_mutex_unlock" );
}

//----------------------------------------------------------ndestroyable_mutex::Undestroyable_mutex

Undestroyable_mutex::Undestroyable_mutex( const string& name, Kind kind )
:
    Mutex_base(name,kind)
{
    int                 err;
    pthread_mutexattr_t attr;

    memset( &_system_mutex, 0, sizeof _system_mutex );

    err = pthread_mutexattr_init( &attr);
    if( err )  throw_xc( "pthread_mutexattr_init", _name.c_str() );


    int k = (int)PTHREAD_MUTEX_RECURSIVE;
    if( kind == kind_nonrecursive )
    {
//#       ifdef Z_DEBUG
            k = (int)PTHREAD_MUTEX_ERRORCHECK;
//#        else
//            kind = (int)PTHREAD_MUTEX_FAST_NP;
//#       endif
    }

    err = pthread_mutexattr_settype( &attr, k );
    if( err )  throw_xc( "pthread_mutexattr_settype", _name.c_str() );

    err = pthread_mutex_init( &_system_mutex, &attr );
    if( err )  throw_xc( "pthread_mutex_init", _name.c_str() );

    err = pthread_mutexattr_destroy( &attr );
    if( err )  throw_xc( "pthread_mutexattr_destroy", _name.c_str() );
}

//---------------------------------------------------------ndestroyable_mutex::~Undestroyable_mutex
    
Undestroyable_mutex::~Undestroyable_mutex()
{
    // Wird nicht zerstört
}

//-----------------------------------------------------------------------Undestroyable_mutex::enter

void Undestroyable_mutex::enter()
{
    //if( !_dont_log )  fprintf(stderr,"pthread_mutex_lock %s (%p)\n", _name.c_str(), (void*)&_system_mutex );
    int err = pthread_mutex_lock( &_system_mutex );
    if( err )  throw_errno( err, "pthread_mutex_lock", _name.c_str() );
}

//-------------------------------------------------------------------Undestroyable_mutex::try_enter

bool Undestroyable_mutex::try_enter()
{
    //if( !_dont_log )  fprintf(stderr,"pthread_mutex_lock try %s (%p)\n", _name.c_str(), (void*)&_system_mutex );
    int err = pthread_mutex_trylock( &_system_mutex );
    if( err ) 
    {
        if( err == EBUSY )
        {
            //if( !_dont_log )  fprintf(stderr,"pthread_mutex_lock BUSY %s (%p)\n", _name.c_str(), (void*)&_system_mutex );
            return false;
        }

        //fprintf(stderr,"pthread_mutex_lock %s (%p) ERROR=%d\n", _name.c_str(), (void*)&_system_mutex, err );
        throw_errno( err, "pthread_mutex_lock", _name.c_str() );
    }

    return true;
}

//-----------------------------------------------------------------------Undestroyable_mutex::leave

void Undestroyable_mutex::leave()
{
    //if( !_dont_log )  fprintf(stderr,"pthread_mutex_unlock %s (%p)\n", _name.c_str(), (void*)&_system_mutex );
    int err = pthread_mutex_unlock( &_system_mutex );
    if( err ) 
    {
        //fprintf(stderr,"pthread_mutex_unlock %s (%p) ERROR=%d\n", _name.c_str(), (void*)&_system_mutex, err );
        throw_errno( err, "pthread_mutex_unlock", _name.c_str() );
    }
}

//-----------------------------------------------------------Undestroyable_mutex::locking_thread_id

Thread_id Undestroyable_mutex::locking_thread_id()
{
    return 0;   // Sollte die Thread-Id des sperrenden Threads sein.
}

//-------------------------------------------------------------------------------------utex::~Mutex

Mutex::~Mutex()
{
    int err = pthread_mutex_destroy( &_system_mutex );
    if( err )  Z_LOG( Z_FUNCTION << " ERROR pthread_mutex_destroy() err=" << err << " " << strerror( err ) << '\n' );
}

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
