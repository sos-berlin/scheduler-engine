// $Id: z_windows_thread.h 13322 2008-01-27 15:27:19Z jz $

#ifndef __ZSCHIMMER_WINDOWS_THREAD_H
#define __ZSCHIMMER_WINDOWS_THREAD_H

#include "base.h"

#ifdef Z_WINDOWS

#include "z_windows_event.h"


namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------------------------------Thread

struct Thread : Thread_base, Non_cloneable
{
  //typedef uint                Id;


                                Thread                      ( const string& name = "" )             : Thread_base(name), _zero_(this+1), _thread_handle( Event::dont_create, name ) {}
                               ~Thread                      ();

    void                        thread_close                ();
    void                        thread_start                ();

    bool                        thread_wait_for_termination ( double wait_time = INT_MAX );

    bool                        thread_is_running           ();
    Id                          thread_id                   ()                                      { return _thread_id; }
  //void                    set_thread_id                   ( Id id )                               { _thread_id = id; }            // Nur sinnvoll, wenn das Objekt mal nicht als Thread läuft (z.B. Task_subsystem)
    HANDLE                      thread_handle               ()                                      { return _thread_handle; }

    void                    set_thread_name                 ( const string& name );

    bool                        try_set_thread_priority     ( int pri )                             { return SetThreadPriority( _thread_handle._handle, pri ) != 0; }
    int                         thread_priority             ()                                      { return GetThreadPriority( _thread_handle._handle ); }
    virtual string              thread_as_text              () const;

  protected:
    friend uint __stdcall       thread_function             ( void* );
    void                        thread_init                 ();
    void                        thread_exit                 ( int exit_code );
  //virtual int                 thread_main                 () = 0;


  public:
    Fill_zero                  _zero_;
    Event                      _thread_handle;              // Trägt den Thread-Handle und signalisiert Ende des Threads
    Id                         _thread_id;
};


inline Thread::Id               current_thread_id           ()                                      { return GetCurrentThreadId(); }

//-----------------------------------------------------------------------------Thread_local_storage

struct Thread_local_storage
{
                                Thread_local_storage        ()                                      : _index(TLS_OUT_OF_INDEXES) {}
                               ~Thread_local_storage        ()                                      { free(); }

    void                        allocate                    ();
    void                        free                        ();
    void                        set                         ( void* value ) const                   { TlsSetValue( _index, value ); }
    void*                       get                         () const                                { return TlsGetValue( _index ); }

    bool                        is_allocated                () const                                { return _index != TLS_OUT_OF_INDEXES; }

    int                        _index;
};

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
#endif
