// $Id$

#ifndef ZSCHIMMER_Z_POSIX_THREAD_H
#define ZSCHIMMER_Z_POSIX_THREAD_H

#include "base.h"

#ifdef Z_UNIX

#include <pthread.h>
#include "threads_base.h"
#include "log.h"


namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------------Thread

struct Thread : zschimmer::Thread_base, Non_cloneable
{
  //typedef pthread_t           Id;


                                Thread                      ( const string& name = "" )             : Thread_base(name), _zero_(this+1) {}
                               ~Thread                      ();

    void                        thread_close                ();
    void                        thread_start                ();

  //bool                        thread_wait_for_termination ( double wait_time = INT_MAX );
    void                        thread_wait_for_termination ();

    bool                        thread_is_running           ()                                      { return _thread_is_running; }
    Id                          thread_id                   ()                                      { return _thread_id; }
  //void                    set_thread_id                   ( Id id )                               { _thread_id = id; }            // Nur sinnvoll, wenn das Objekt mal nicht als Thread läuft (z.B. Task_subsystem)
    pthread_t                   thread_handle               ()                                      { return _pthread_handle; }

  //bool                        try_set_thread_priority     ( int pri )                             { return SetThreadPriority( _thread_handle._handle, pri ) != 0; }
  //int                         thread_priority             ()                                      { return GetThreadPriority( _thread_handle._handle ); }
  //virtual string              thread_as_text              () const;

  protected:
    friend void*                thread_function             ( void* );
    virtual int                 thread_main                 () = 0;
    void                        thread_init                 ();
    void                        thread_exit                 ( int exit_code );

  private:
    Fill_zero                  _zero_;
  //pthread_attr_t             _pthread_attr;
    pthread_t                  _pthread_handle;
    Id                         _thread_id;                  // Kopie von _pthread_handle oder von außen gesetzt (dann _pthread_handle == NULL, für Spooler-Threads)
  //Event                      _thread_event;
    bool                       _thread_is_running;
    int                        _thread_pid;
};


inline Thread::Id               current_thread_id           ()                                      { return pthread_self(); }

//-----------------------------------------------------------------------------Thread_specific_data

struct Thread_specific_data      
{
                                Thread_specific_data        ()                                      : _key( (pthread_key_t)-1 ) {}
                               ~Thread_specific_data        ()                                      { free(); }

    void                        allocate                    ();
    void                        free                        ();
    void                        set                         ( void* value ) const                   { pthread_setspecific( _key, value ); }
    void*                       get                         () const                                { return pthread_getspecific( _key ); }

    bool                        is_allocated                () const                                { return _key != (pthread_key_t)-1; }

    pthread_key_t              _key;
};

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
#endif
