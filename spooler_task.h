// $Id: spooler_task.h,v 1.109 2003/08/29 08:14:04 jz Exp $

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
      //s_pending,              // Warten auf Start
        s_start_task,           // Task aus der Warteschlange genommen, muss noch gestartet werden. 
        s_starting,             // In begin__start() (end__start() muss noch gerufen werden)
        s_running,              // Läuft (wenn _in_step, dann in step__start() und step__end() muss gerufen werden)
        s_running_delayed,      // spooler_task.delay_spooler_process gesetzt
        s_running_waiting_for_order,
        s_running_process,      // Läuft in einem externen Prozess, auf dessen Ende nur gewartet wird
        s_suspended,            // Angehalten
        s_end,                  // Task soll beendet werden
        s_ending,               // end__start() gerufen, also in spooler_close()
        s_ended,                // Task wird gelöscht
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

    void                        cmd_end                     ()                                      { _end = true; signal( "end" ); }

    void                        close                       ();
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );

    State                       state                       () const                                { return _state; }
    void                        attach_to_a_thread          ();
    bool                        do_something                ();

    Job*                        job                         ()                                      { return _job; }
    Time                        next_time                   ()                                      { THREAD_LOCK_RETURN( _lock, Time, _next_time ); }
    Spooler_thread*             thread                      ()                                      { return _thread; }
    string                      name                        () const                                { return _obj_name(); }
    virtual string             _obj_name                    () const                                { return "Task " + _job->obj_name() + " " + as_string(_id) ; }

    string                      state_name                  ()                                      { return state_name( state() ); }
    static string               state_name                  ( State );
    State                       state                       ()                                      { return _state; }
  //string                      state                       ();

    Time                        last_process_start_time     ()                                      { THREAD_LOCK_RETURN( _lock, Time, _last_process_start_time ); }

    void                        signal                      ( const string& signal_name = "" );
    bool                        has_error                   ()                                      { return _error != NULL; }
    void                    set_error_xc_only               ( const Xc& );

  protected:
    void                        remove_order_after_error    ();

    bool                        prepare                     ();
    void                        finish                      ();
    Async_operation*            begin__start                ();
    bool                        begin__end                  ();
  //void                        end__start                  ();
    void                        end__end                    ();
  //void                        step__start                 ();
    bool                        step__end                   ();

    void                        set_mail_defaults           ();
    void                        clear_mail                  ();
    void                        send_collected_log          ();

    void                        set_cause                   ( Start_cause );
    void                        set_history_field           ( const string& name, const Variant& value );
    virtual void                set_close_engine            ( bool )                                {}      // Für Module_task
    bool                        has_parameters              ();
    xml::Document_ptr           parameters_as_dom           ()                                      { return _params->dom(); }


    bool                        wait_until_terminated       ( double wait_time = latter_day );
    void                        set_delay_spooler_process   ( Time t )                              { _log.debug("delay_spooler_process=" + t.as_string() ); _next_spooler_process = Time::now() + t; }

    void                        set_state                   ( State, const Time& wait_until = 0 );

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
    virtual void                do_kill                     ()                                      {}
    virtual Async_operation*    do_begin__start             ()                                      { return NULL; }
    virtual bool                do_begin__end               () = 0;
    virtual Async_operation*    do_end__start               ()                                      { return NULL; }
    virtual void                do_end__end                 () = 0;
    virtual Async_operation*    do_step__start              ()                                      { return NULL; }
    virtual bool                do_step__end                () = 0;
/*
    virtual bool                loaded                      ()                                      { return true; }
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
    bool                       _opened;
    bool                       _end;
    bool                       _on_error_called;
    bool                       _closed;


    Time                       _enqueue_time;
    Time                       _start_at;                   // Zu diesem Zeitpunkt (oder danach) starten
    Time                       _running_since;
    Time                       _last_process_start_time;
    Time                       _next_spooler_process;
    Time                       _next_time;

    ptr<Async_operation>       _operation;
    ptr<Com_variable_set>      _params;
    Variant                    _result;
    string                     _name;
  //bool                       _close_engine;               // Bei einem Fehler in spooler_init()
  //bool                       _close_engine;               // Nach Task-Ende Scripting Engine schließen (für use_engine="job")
    ptr<Order>                 _order;
    State                      _state;
    bool                       _in_step;                    // Für s_running: step() wird gerade ausgeführt, step__end() muss noch gerufen werden
    bool                       _processing;                 // In asynchroner Ausführung (zwischen xxx__begin() und xxx__end())
    Call_state                 _call_state;
    Xc_copy                    _error;
    bool                       _success;                    // true, wenn spooler_on_success() gerufen werden soll,
                                                            // false, wenn spooler_on_error() gerufen werden soll
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

    ptr<Module_instance>       _module_instance;
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

    virtual void                do_load                     ();
  //virtual void                do_close                    ();
    virtual Async_operation*    do_begin__start             ();
    virtual bool                do_begin__end               ();
    virtual Async_operation*    do_end__start               ();
    virtual void                do_end__end                 ();
    virtual Async_operation*    do_step__start              ();
    virtual bool                do_step__end                ();
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
    virtual void                do_kill                     ();
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
