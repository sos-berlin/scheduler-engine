// $Id$

#ifndef __ZSCHIMMER_WINDOWS_EVENT_H
#define __ZSCHIMMER_WINDOWS_EVENT_H

#ifdef Z_WINDOWS

#include "threads_base.h"
#include "z_windows.h"

namespace zschimmer {
namespace windows {

//--------------------------------------------------------------------------------------------Event

struct Event : zschimmer::Event_base, Handle
{
                                Event                       ( const string& name = "" );
                                Event                       ( Dont_create, const string& name = "" ): Handle(NULL), _zero_(this+1), zschimmer::Event_base(name), _mutex( "Event " + name) {}
                                Event                       ( HANDLE h, const string& name = "" )   : Handle(h   ), _zero_(this+1), zschimmer::Event_base(name), _mutex( "Event " + name) {}
                               ~Event                       ();

    Event&                      operator =                  ( const HANDLE& h )                     { assert( _handle==NULL ); _handle = h; }

    void                        create                      ();
    virtual void                close                       ()                                      { Handle::close(); }

  //void                        set_signal                  ();
    void                        signal                      ( const string& name );
    void                        async_signal                ( const char* name );
    bool                        signaled_then_reset         ();
    void                        reset                       ();
    bool                        wait                        ( double seconds = INT_MAX );
  //void                        lock                        ();
  //void                        unlock                      ();
    bool                        valid                       () const                                { return Handle::valid(); }


    Fill_zero                  _zero_;
    Mutex                      _mutex;
};

//-----------------------------------------------------------------------------------windows::Events

/*
struct Events : Non_cloneable
{
                                Events                      ();
                               ~Events                      ();

    void                        add                         ( Event* );
    void                        remove                      ( Event* );
  //HANDLE                      operator []                 ( int index )                           { return _handles[index]; }
    int                         wait_until_localtime        ( double local_time );
    int                         wait_until_gmtime           ( double time );


    Fill_zero_                 _zero_;
    vector<Event*>             _events;
    pthread_cond_t             _pthread_cond;
    pthread_mutex_t            _pthread_mutex;
};
*/

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
#endif
