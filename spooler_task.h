// $Id: spooler_task.h,v 1.123 2003/10/18 21:23:17 jz Exp $

#ifndef __SPOOLER_TASK_H
#define __SPOOLER_TASK_H


namespace sos {
namespace spooler {

//----------------------------------------------------------------------------------------------Task

struct Task : Sos_self_deleting
{
    enum State
    {
        s_none,

        s_loading,
        s_waiting_for_process,  // Prozess aus Prozessklasse wählen, evtl. warten, bis ein Prozess verfügbar ist.
        s_starting,             // load, spooler_init, spooler_open
                                // Bis hier gilt Task::starting() == true
        s_running,              // Läuft (wenn _in_step, dann in step__start() und step__end() muss gerufen werden)
        s_running_delayed,      // spooler_task.delay_spooler_process gesetzt
        s_running_waiting_for_order,
        s_running_process,      // Läuft in einem externen Prozess, auf dessen Ende nur gewartet wird

        s_suspended,            // Angehalten
                                // Ab hier gilt Task::ending() == true
        s_ending,               // spooler_close,  
        s_on_success,           // spooler_on_success
        s_on_error,             // spooler_on_error
        s_exit,                 // spooler_exit
        s_release,              // Release()
        s_ended,                // 
        s_closed,               // close() gerufen, Object Task ist nicht mehr zu gebrauchen
        s__max
    };


    enum Call_state
    {
        c_none,
        c_begin,
        c_step,
        c_end,
    };


                                Task                        ( Job* );
                               ~Task                        ();

    int                         id                          ()                                      { return _id; }

    void                        cmd_end                     ();
    void                        cmd_nice_end                ( Job* for_job = NULL );

    void                        close                       ();
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );

    State                       state                       () const                                { return _state; }
    void                        attach_to_a_thread          ();
    bool                        do_something                ();

    Job*                        job                         ()                                      { return _job; }
    Time                        next_time                   ();
    Spooler_thread*             thread                      ()                                      { return _thread; }
    string                      name                        () const                                { return _obj_name(); }
    virtual string             _obj_name                    () const                                { return "Task " + _job->obj_name() + " " + as_string(_id) ; }

    string                      state_name                  ()                                      { return state_name( state() ); }
    static string               state_name                  ( State );
    State                       state                       ()                                      { return _state; }
    bool                        starting                    ()                                      { return _state > s_none  &&  _state <= s_starting; }
    bool                        ending                      ()                                      { return _end  ||  _state >= s_ending; }
    bool                        is_idle                     ()                                      { return _state == s_running_waiting_for_order  &&  !_end; }

    Time                        last_process_start_time     ()                                      { THREAD_LOCK_RETURN( _lock, Time, _last_process_start_time ); }
    Time                        ending_since                ()                                      { THREAD_LOCK_RETURN( _lock, Time, _ending_since ); }

    void                        signal                      ( const string& signal_name );
    bool                        has_error                   ()                                      { return _error != NULL; }
    void                    set_error_xc_only               ( const Xc& );

    void                    set_order                       ( Order* );
    Order*                      take_order                  ( const Time& now );

  protected:
    void                        remove_order_after_error    ();

    void                        finish                      ();
    void                        load                        ();
    Async_operation*            begin__start                ();
  //bool                        begin__end                  ();
  //void                        end__start                  ();
  //void                        end__end                    ();
  //void                        step__start                 ();
    bool                        step__end                   ();
    bool                        operation__end              ();

    void                        set_mail_defaults           ();
    void                        clear_mail                  ();
    void                        send_collected_log          ();

    void                        set_cause                   ( Start_cause );
    void                        set_history_field           ( const string& name, const Variant& value );
    virtual void                set_close_engine            ( bool )                                {}      // Für Module_task
    bool                        has_parameters              ();
    xml::Document_ptr           parameters_as_dom           ()                                      { return _params->dom(); }


    bool                        check_timeout               ();
    bool                        wait_until_terminated       ( double wait_time = latter_day );
    void                        set_delay_spooler_process   ( Time t )                              { _log.debug("delay_spooler_process=" + t.as_string() ); _next_spooler_process = Time::now() + t; }

    void                        set_state                   ( State );

    void                        set_error_xc                ( const Xc& );
    void                        set_error                   ( const Xc& x )                         { set_error_xc( x ); }
    void                        set_error                   ( const z::Xc& x )                      { set_error_xc( x ); }
    void                        set_error                   ( const exception& );
    void                        set_error                   ( const _com_error& );
    Xc_copy                     error                       ()                                      { Xc_copy result; THREAD_LOCK( _lock )  result = _error;  return result; }
    void                        reset_error                 ()                                      { THREAD_LOCK( _lock )  _error = NULL,  _log.reset_highest_level(); }

    void                    set_next_time                   ( const Time& );


    void                        enter_thread                ( Spooler_thread* );
    void                        leave_thread                ();

    Order*                      order                       ()                                      { return _order; }

  
    friend struct               Job;
    friend struct               Object_set;
    friend struct               Com_task;

    friend struct               Task_history;

    virtual void                do_close                    ()                                      {}
    virtual void                do_load                     ()                                      {}
    virtual bool                do_kill                     ()                                      { return false; }
    virtual Async_operation*    do_begin__start             ()                                      { return &dummy_sync_operation; }
    virtual bool                do_begin__end               () = 0;
    virtual Async_operation*    do_end__start               ()                                      { return &dummy_sync_operation; }
    virtual void                do_end__end                 () = 0;
    virtual Async_operation*    do_step__start              ()                                      { return &dummy_sync_operation; }
    virtual bool                do_step__end                () = 0;
    virtual Async_operation*    do_call__start              ( const string& method )                { return &dummy_sync_operation; }
    virtual bool                do_call__end                ()                                      { return true; }           // Default: Nicht implementiert
    virtual Async_operation*    do_release__start           ()                                      { return &dummy_sync_operation; }
    virtual void                do_release__end             ()                                      {}

    virtual bool                loaded                      ()                                      { return true; }
/*
    virtual void                do_close                    ()                                      {}
    virtual bool                do_start                    () = 0;
    virtual void                do_kill                     ()                                      {}
    virtual void                do_end                      () = 0;
    virtual bool                do_step                     () = 0;
    virtual void                do_on_success               () = 0;
    virtual void                do_on_error                 () = 0;
*/
    virtual bool                has_step_count              ()                                      { return true; }

    Fill_zero                  _zero_;

    Z_DEBUG_ONLY( string       _job_name; )
    int                        _id;
    State                      _state;

    Job*                       _job;
    Thread_semaphore           _lock;
    Prefix_log                 _log;
    Spooler*                   _spooler;
    Spooler_thread*            _thread;
    Task_history               _history;
 
    Thread_semaphore           _terminated_events_lock;
    vector<Event*>             _terminated_events;

    Start_cause                _cause;
    double                     _cpu_time;
    int                        _step_count;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _job._period.end() beenden
    bool                       _begin_called;
    bool                       _end;
    bool                       _closed;
    bool                       _signaled;


    Time                       _enqueue_time;
    Time                       _start_at;                   // Zu diesem Zeitpunkt (oder danach) starten
    Time                       _running_since;
    Time                       _ending_since;
    Time                       _last_process_start_time;
    Time                       _last_operation_time;
    Time                       _next_spooler_process;
    Time                       _next_time;
    Time                       _timeout;                   // Frist für eine Operation (oder INT_MAX)
    bool                       _killed;                     // Task abgebrochen (nach do_kill/timeout)
    bool                       _kill_tried;

    ptr<Async_operation>       _operation;
    ptr<Com_variable_set>      _params;
    Variant                    _result;
    string                     _name;
    ptr<Order>                 _order;
    Call_state                 _call_state;
    Xc_copy                    _error;

    ptr<Module_instance>       _module_instance;            // Nur für Module_task. Hier, damit wir nicht immer wieder casten müssen.
};

//----------------------------------------------------------------------------------------Task_list

typedef list< Sos_ptr<Task> >   Task_list;

#define FOR_EACH_TASK( ITERATOR, TASK )  FOR_EACH( Task_list, _task_list, ITERATOR )  if( Task* TASK = *ITERATOR )

//--------------------------------------------------------------------------------------Module_task

struct Module_task : Task       // Oberklasse für Object_set_task und Job_module_task
{
                                Module_task                 ( Job* j )                              : Task(j) {}

    virtual void                set_close_engine            ( bool b )                              { if( _module_instance )  _module_instance->set_close_instance_at_end(b); }

  //virtual void                do_load                     ();
    virtual void                do_close                    ();
  //virtual void                do_kill                     ()                                      {}
/*
    void                        do_close                    ();
  //bool                        do_start                    ();
  //void                        do_end                      ();
  //bool                        do_step                     ();
    void                        do_on_success               ();
    void                        do_on_error                 ();
*/
    bool                        loaded                      ()                                      { return _module_instance && _module_instance->loaded(); }
  //bool                        load_module_instance        ();
  //void                        close_engine                ();
};

//----------------------------------------------------------------------------------Object_set_task
/*
struct Object_set_task : Module_task
{
                                Object_set_task             ( Job* j )                              : Module_task(j) {}

  //virtual void                do_load                     ();
    virtual void                do_close                    ();
    virtual void                do_begin__start             ();
    virtual bool                do_begin__end               ();
    virtual void                do_end__start               ();
    virtual void                do_end__end                 ();
    virtual void                do_step__start              ();
    virtual bool                do_step__end                ();
    bool                        loaded                      ()                                      { return _object_set && Module_task::loaded(); }


    Sos_ptr<Object_set>        _object_set;
    ptr<Com_object_set>        _com_object_set;
};
*/
//----------------------------------------------------------------------------------Job_module_task

struct Job_module_task : Module_task
{
                                Job_module_task             ( Job* j )                              : Module_task(j) {}

    virtual bool                do_kill                     ();
    virtual void                do_load                     ();
  //virtual void                do_close                    ();
    virtual Async_operation*    do_begin__start             ();
    virtual bool                do_begin__end               ();
    virtual Async_operation*    do_end__start               ();
    virtual void                do_end__end                 ();
    virtual Async_operation*    do_step__start              ();
    virtual bool                do_step__end                ();
    virtual Async_operation*    do_call__start              ( const string& method );
    virtual bool                do_call__end                ();
    virtual Async_operation*    do_release__start           ();
    virtual void                do_release__end             ();
/*
  //virtual bool                loaded                      ();
    bool                        do_start                    ();
    void                        do_end                      ();
    bool                        do_step                     ();
  //void                        do_on_success               ();
  //void                        do_on_error                 ();
*/
};

//------------------------------------------------------------------------------------Process_event
#ifndef Z_WINDOWS

struct Process_event : Event
{
    virtual void                close                       ();
    virtual bool                wait                        ( double seconds );

    int                        _pid;
    int                        _process_signaled;
    int                        _process_exit_code;
};

#endif
//-------------------------------------------------------------------------------------Process_task

struct Process_task : Task      // Job ist irgendein Prozess (z.B. durch ein Shell-Skript implementiert)
{
    Fill_zero _zero_;

                                Process_task                ( Job* j );

    virtual void                do_close                    ();
    virtual bool                do_kill                     ();
  //virtual void                do_begin__start             ();
    virtual bool                do_begin__end               ();
  //virtual void                do_end__start               ();
    virtual void                do_end__end                 ();
  //virtual void                do_step__start              ();
    virtual bool                do_step__end                ();
/*        
  //virtual bool                loaded                      ();
    void                        do_close                    ();
    bool                        do_start                    ();
    void                        do_kill                     ();
    void                        do_end                      ();
    bool                        do_step                     ();
    void                        do_on_success               ()                                      {}
    void                        do_on_error                 ()                                      {}
*/

    virtual bool                has_step_count              ()                                      { return false; }
    bool                        signaled                    ();

#   ifdef Z_WINDOWS
        Process_id             _process_id;
        Event                  _process_handle;
#    else
        Process_event          _process_handle;
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
