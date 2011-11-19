// $Id: z_windows_mutex.h 11394 2005-04-03 08:30:29Z jz $

#ifndef ZSCHIMMER_Z_WINDOWS_MUTEX_H
#define ZSCHIMMER_Z_WINDOWS_MUTEX_H

#ifdef Z_WINDOWS

#include "mutex_base.h"

namespace zschimmer {
namespace windows {

//--------------------------------------------------------------------------------------------------

typedef CRITICAL_SECTION        System_mutex;

inline void                     enter_mutex                 ( System_mutex* m )                     { EnterCriticalSection( m ); }
inline void                     leave_mutex                 ( System_mutex* m )                     { LeaveCriticalSection( m ); }

//------------------------------------------------------------------------------------windows::Mutex
// AUFBAU NICHT ÄNDERN, wird mit zur Laufzeit nachgeladenem Modul ausgetauscht!

struct Mutex : Mutex_base
{
                                Mutex                       ( const string& name = "", Kind kind = kind_recursive ) : Mutex_base(name,kind) { InitializeCriticalSection( &_system_mutex ); }
                               ~Mutex                       ()                                      { DeleteCriticalSection( &_system_mutex ); }

    void                        enter                       () throw()                              { enter_mutex( &_system_mutex ); }
    bool                        try_enter                   ();
    void                        leave                       () throw()                              { leave_mutex( &_system_mutex ); }
    Thread_id                   locking_thread_id           ();

    CRITICAL_SECTION           _system_mutex;
};

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
#endif
