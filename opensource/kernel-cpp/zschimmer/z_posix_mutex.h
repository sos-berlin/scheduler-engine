// $Id: z_posix_mutex.h 12662 2007-03-09 09:17:51Z jz $

#ifndef ZSCHIMMER_Z_POSIX_MUTEX_H
#define ZSCHIMMER_Z_POSIX_MUTEX_H

#ifdef Z_UNIX

#include <pthread.h>

namespace zschimmer {
namespace posix {


typedef pthread_mutex_t         System_mutex;

void                            enter_mutex                 ( System_mutex* );
void                            leave_mutex                 ( System_mutex* );

//------------------------------------------------------------------------------Undestroyable_mutex
// AUFBAU NICHT ÄNDERN, wird mit zur Laufzeit nachgeladenem Modul ausgetauscht!

struct Undestroyable_mutex : zschimmer::Mutex_base
{
                                Undestroyable_mutex         ( const string& name = "", Kind = kind_recursive );
    virtual                    ~Undestroyable_mutex         ();

    void                        enter                       ();
    bool                        try_enter                   ();
    void                        leave                       ();
    Thread_id                   locking_thread_id           ();

    pthread_mutex_t            _system_mutex;
};

//--------------------------------------------------------------------------------------------Mutex

struct Mutex : Undestroyable_mutex
{
                                Mutex                       ( const string& name = "", Kind kind = kind_recursive ) : Undestroyable_mutex( name, kind ) {}
                               ~Mutex                       ();
};

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
#endif
