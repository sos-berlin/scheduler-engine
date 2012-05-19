// $Id: threads_base.h 13322 2008-01-27 15:27:19Z jz $

#ifndef ZSCHIMMER_THREADS_BASE_H
#define ZSCHIMMER_THREADS_BASE_H

namespace zschimmer {


//---------------------------------------------------------------------------------------Event_base
/*
    Bei den Events könnte besser getrennt werden, welche Methoden eine Systemfunktion aufrufen
    und welche Methoden nur das Flag _signaled ändern. 
*/

struct Event_base : Object//, Has_set_signaled
{
    enum Dont_create { dont_create };

                                Event_base                  ()                                      : _zero_(this+1) {}
                                Event_base                  ( const string& name )                  : _zero_(this+1), _name(name) {}
    virtual                    ~Event_base                  ();

    virtual void                close                       () = 0;

    virtual void                set_name                    ( const string& name )                  { _name = name; }
    string                      name                        ()                                      { return _name; }

    virtual bool                valid                       () const = 0;
  //virtual void            set_signal                      () = 0;
    virtual void                signal                      ( const string& name ) = 0;
    virtual bool                signaled                    ()                                      { return _signaled; }   // Nicht const, kann Zustand ändern
    bool                        signaled_flag               () const                                { return _signaled; }   // const
    void                    set_signaled                    ( bool b = true )                       { _signaled = b; }
    void                    set_signaled                    ( const string& name )                  { _signaled = true; _signal_name = name; }
    virtual void                reset                       () = 0;
  //virtual bool                signaled_after_check        ()                                      { return _signaled; }

    void                    set_waiting_thread_id           ( Thread_id id )                        { _waiting_thread_id = id; }  // Wenn nur dieser Thread wartet

    virtual bool                wait                        ( double seconds ) = 0;
    bool                        wait_until_localtime        ( double local_time );                  // Berücksichtigt Sommerzeitumstellung
    bool                        wait_until_gmtime           ( double gm_time );
    virtual string              as_text                     () const;
    friend ostream&             operator <<                 ( ostream& s, const Event_base& e )     { s << e.as_text(); return s; }


    Fill_zero                  _zero_;
    string                     _name;
    string                     _signal_name;
    bool                       _signaled;
    bool                       _waiting;
    Thread_id                  _waiting_thread_id;


  private:
                                Event_base                  ( const Event_base& );                   // Nicht implementiert. CRITICAL_SECTION darf nicht kopiert oder verschoben werden
    void                        operator =                  ( const Event_base& );                   // Nicht implementiert.
};

//--------------------------------------------------------------------------------------Thread_base

struct Thread_base : Object
{
    typedef Thread_id           Id;


                                Thread_base                 ( const string& name )                  : _zero_(this+1), _thread_name(name) {}
    virtual                    ~Thread_base                 ()                                      {}

    virtual void                thread_close                ()                                      = 0;

    void                    set_thread_name                 ( const string& name )                  { _thread_name = name; }
    string                      thread_name                 () const                                { return _thread_name; }

    void                    set_thread_termination_event    ( Event_base* event )                   { _thread_termination_event = event; }

    int                         thread_exit_code            ()                                      { return _thread_exit_code; }
    virtual string              thread_as_text              () const                                { return "Thread"; }
    virtual string              as_text                     () const                                { return thread_as_text(); }

    int                         call_thread_main            ();                                     // Mit thread_call_main() zusammenfassen?

  protected:
    void                        thread_call_main            ();
    virtual int                 thread_main                 ()                                      = 0;
    virtual void                thread_init                 ()                                      = 0;
    virtual void                thread_exit                 ( int exit_code )                       = 0;

    Fill_zero                  _zero_;
    string                     _thread_name;
    int                        _thread_exit_code;
    Event_base*                _thread_termination_event;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
