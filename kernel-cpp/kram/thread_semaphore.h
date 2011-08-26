// $Id: thread_semaphore.h 11394 2005-04-03 08:30:29Z jz $

// Frei von sos.h!

#if 1

#include "../zschimmer/mutex.h"

using zschimmer::Mutex;
using zschimmer::Mutex_guard;
typedef Mutex                   Thread_semaphore;
#define THREAD_LOCK( MUTEX )    Z_MUTEX( MUTEX )
#define THREAD_LOCK_RETURN(A,B,C) Z_MUTEX_RETURN(A,B,C)




#else

#ifndef __SOS_THREAD_SEMAPHORE_H
#define __SOS_THREAD_SEMAPHORE_H

#ifdef SYSTEM_WIN
#   include <windows.h>
#   undef min
#   undef max
#else
#   include <pthread.h>
#endif


#ifndef _WIN32
#   define __assume(X) 
#endif


namespace sos {

//---------------------------------------------------------------------------Thread_semaphore

struct Thread_semaphore
{
    struct Guard
    {
                                Guard                   ()                                              { _semaphore = NULL; }
                                Guard                   ( Thread_semaphore* s )                         { __assume(s); enter(s); }
                               ~Guard                   ()                                              { if( _semaphore )  _semaphore->leave(); }

        void                    enter                   ( Thread_semaphore* s )                         { s->enter(); _semaphore=s; }
        void                    leave                   ()                                              { _semaphore->leave(); _semaphore = NULL; }
                              operator Thread_semaphore*() const                                        { return _semaphore; }

        Thread_semaphore*      _semaphore;
    };

/*
    struct Guard_with_log : Guard
    {
                                Guard_with_log          ( Thread_semaphore* s, const string& debug );
                               ~Guard_with_log          ();

        string                 _debug;
    };
*/


                                Thread_semaphore        ( const string& dummy_name = "" );
    virtual                    ~Thread_semaphore        ();
    virtual void                enter                   ();
    bool                        try_enter               ();
    virtual void                leave                   ();

#   if defined SYSTEM_WIN
        CRITICAL_SECTION       _semaphore;
#    else
        pthread_mutex_t        _pthread_mutex;
      //pthread_mutexattr_t    _pthread_mutex_attr;
#   endif

    string                     _name;

  private:
                                Thread_semaphore        ( const Thread_semaphore& );        // Nicht implementiert, denn CRITICAL_SECTION darf nicht kopiert 
                                                                                            // oder verschoben werden
    void                        operator =              ( const Thread_semaphore& );        // Nicht implementiert
};

//------------------------------------------------------------------------Thread_semaphore_with_log

struct Thread_semaphore_with_log : Thread_semaphore
{
                                Thread_semaphore_with_log( const string& name )                 : Thread_semaphore(name) {}

    void                        enter                   ();
    void                        leave                   ();
};

//-------------------------------------------------------------------------------------------inline

#if defined SYSTEM_WIN

    inline          Thread_semaphore::Thread_semaphore  ( const string& name )                  : _name(name) { InitializeCriticalSection( &_semaphore ); }
    inline          Thread_semaphore::~Thread_semaphore ()                                      { DeleteCriticalSection( &_semaphore ); }
    inline void     Thread_semaphore::enter             ()                                      { EnterCriticalSection( &_semaphore ); }
    inline void     Thread_semaphore::leave             ()                                      { LeaveCriticalSection( &_semaphore ); }

#else

    inline          Thread_semaphore::Thread_semaphore  ( const string& )                       {}
    inline          Thread_semaphore::~Thread_semaphore ()                                      {}
    inline void     Thread_semaphore::enter             ()                                      {}
    inline void     Thread_semaphore::leave             ()                                      {}

#endif

//--------------------------------------------------------------------------------------THREAD_LOCK

//#if defined __GNUC__ || _MSC_VER >= 1300   // Microsoft Visual C++ 2003

#   define THREAD_LOCK( LOCK )                                                                     \
        if( sos::Thread_semaphore::Guard __guard__ = &LOCK )

/*
#else

#   define THREAD_LOCK( LOCK )                                                                     
        for( sos::Thread_semaphore::Guard __guard__##__LINE__ = &LOCK;                              
            __guard__##__LINE__;                                                                   
            __guard__##__LINE__.leave() )

*/

//#define THREAD_LOCK_LOG( LOCK, DEBUG )  THREAD_LOCK( LOCK )
/*
#define THREAD_LOCK_LOG( LOCK, DEBUG )                                                          \
    for( sos::Thread_semaphore::Guard_with_log __guard__##__LINE__ ( &LOCK, DEBUG );            \
         __guard__##__LINE__;                                                                   \
         __guard__##__LINE__.leave() )
*/
//-------------------------------------------------------------------------------THREAD_LOCK_RETURN

#define THREAD_LOCK_RETURN( LOCK, TYPE, EXPR )                                                  \
{                                                                                               \
    TYPE __result__;                                                                            \
    {                                                                                           \
        sos::Thread_semaphore::Guard __guard__ = &LOCK;                                         \
        __result = EXPR;                                                                        \
    }                                                                                           \
    return __result__;                                                                          \
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
#endif
