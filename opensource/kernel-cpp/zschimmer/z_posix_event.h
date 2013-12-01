// $Id: z_posix_event.h 11394 2005-04-03 08:30:29Z jz $

#ifndef ZSCHIMMER_Z_POSIX_EVENT_H
#define ZSCHIMMER_Z_POSIX_EVENT_H

#include "base.h"

#ifdef Z_UNIX

#include <pthread.h>
#include "threads_base.h"


namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------posix::Event

struct Event : zschimmer::Event_base
{
                                Event                       ( const string& name = "" );
                                Event                       ( Dont_create, const string& name = "" ) : _zero_(this+1) {}
                               ~Event                       ();

    void                        create                      ();
    virtual void                close                       ();

    void                        signal                      ( const string& name );
    void                        async_signal                ( const char* name );
    bool                        signaled_then_reset         ();
    void                        reset                       ();
    void                        wait                        ();
    bool                        wait                        ( double seconds );
    bool                        valid                       () const                                { return _created; }


    Fill_zero                  _zero_;
    pthread_cond_t             _pthread_cond;
    bool                       _created;
    int                        _waiting;
    Mutex                      _mutex;
};

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
#endif
