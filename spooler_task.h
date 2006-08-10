// $Id$

#ifndef __SPOOLER_TASK_H
#define __SPOOLER_TASK_H


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------Start_cause

enum Start_cause
{
    cause_none                  = 0,    // Noch nicht gestartet
    cause_period_once           = 1,    // <run_time once="yes">
    cause_period_single         = 2,    // <run_time single_start="yes">
    cause_period_repeat         = 3,    // <run_time repeat="..">
  //cause_job_repeat            = 4,    // spooler_job.repeat = ..
    cause_queue                 = 5,    // <start_job at="">
    cause_queue_at              = 6,    // <start_job at="..">
    cause_directory             = 7,    // start_when_directory_changed
    cause_signal                = 8,
    cause_delay_after_error     = 9,
    cause_order                 = 10,
    cause_wake                  = 11,   // sc_wake
    cause_min_tasks             = 12
};

string                          start_cause_name            ( Start_cause );

//----------------------------------------------------------------------------------------------Task

struct Task : Object, 
              Scheduler_object
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
        s_ending_waiting_for_subprocesses, // nach spooler_close, Ende der Subprozesse abwarten
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


    struct Registered_pid : z::Object, Non_cloneable
    {
                                Registered_pid              ( Task*, int pid, const Time& timeout, bool wait, bool ignore_error, bool ignore_signal, const string& title );
                               ~Registered_pid              ()                                      { close(); }

        void                    close                       ();
        void                    try_kill                    ();


        Spooler*               _spooler;
        Task* const            _task;
        int                    _pid;
        Time                   _timeout_at;
        bool                   _wait;                       // Auf Ende der Task warten und Exitcode und Signal auswerten (-> Task-Error)
        bool                   _ignore_error;
        bool                   _ignore_signal;
        string                 _title;                      // Kann die Kommandozeile sein
        bool                   _killed;
    };


    typedef stdext::hash_map< int, ptr<Registered_pid> >  Registered_pids;



                                Task                        ( Job* );
                               ~Task                        ();

    int                         id                          ()                                      { return _id; }

    void                        cmd_end                     ( bool kill_immediately = false );
    void                        cmd_nice_end                ( Job* for_job = NULL );

    void                        close                       ();
    void                        job_close                   ();                                     // Setzt _job = NULL
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;
    xml::Document_ptr           dom                         ( const Show_what& ) const;

    State                       state                       () const                                { return _state; }
    void                        attach_to_a_thread          ();
    bool                        do_something                ();

    Job*                        job                         ();
    Prefix_log*                 log                         ()                                      { return _log; }
    Time                        next_time                   ();
    void                        calculate_next_time_after_modified_order_queue();
    Spooler_thread*             thread                      ()                                      { return _thread; }
    string                      name                        () const                                { return obj_name(); }
    virtual string              obj_name                    () const                                { return _obj_name; }

    string                      state_name                  () const                                { return state_name( state() ); }
    static string               state_name                  ( State );
    State                       state                       ()                                      { return _state; }
    bool                        starting                    ()                                      { return _state > s_none  &&  _state <= s_starting; }
    bool                        ending                      ()                                      { return _end  ||  _state >= s_ending; }
    bool                        is_idle                     ()                                      { return _state == s_running_waiting_for_order  &&  !_end; }

    bool                        running_state_reached       () const                                { return _running_state_reached; }
    Time                        last_process_start_time     ()                                      { THREAD_LOCK_RETURN( _lock, Time, _last_process_start_time ); }
    Time                        ending_since                ()                                      { THREAD_LOCK_RETURN( _lock, Time, _ending_since ); }

    void                        signal                      ( const string& signal_name );
    bool                        has_error                   ()                                      { return _error != NULL; }
    void                    set_error_xc_only               ( const Xc& );

    void                    set_exit_code                   ( int exit_code )                       { _exit_code = exit_code; }
    int                         exit_code                   ()                                      { return _exit_code; }

    void                    set_web_service                 ( Web_service* );
    void                    set_web_service                 ( const string& name );
    Web_service*                web_service                 () const;
    Web_service*                web_service_or_null         () const                                { return _web_service; }

    void                    set_order                       ( Order* );
    Order*                      take_order                  ( const Time& now );
    void                        postprocess_order           ( bool spooler_process_result );

    void                        add_pid                     ( int pid, const Time& timeout = latter_day );
    void                        remove_pid                  ( int pid );
    void                        add_subprocess              ( int pid, double timeout, bool ignore_error, bool ignore_signal, const string& title );
    void                        set_subprocess_timeout      ();
    bool                        check_subprocess_timeout    ( const Time& now );
    bool                        shall_wait_for_registered_pid();
    

  protected:
    void                        remove_order_after_error    ();

    void                        finish                      ();
    void                        process_on_exit_commands    ();
    bool                        load                        ();
    Async_operation*            begin__start                ();
  //bool                        begin__end                  ();
  //void                        end__start                  ();
  //void                        end__end                    ();
  //void                        step__start                 ();
    bool                        step__end                   ();
    bool                        operation__end              ();

    void                        set_mail_defaults           ();
  //void                        clear_mail                  ();
    void                        trigger_event               ( Scheduler_event* );
    void                        send_collected_log          ();

    void                        set_cause                   ( Start_cause );
    Start_cause                 cause                       () const                                { return _cause; }
    string                      string_cause                () const                                { return start_cause_name( _cause ); }

    void                        set_history_field           ( const string& name, const Variant& value );
    virtual void                set_close_engine            ( bool )                                {}      // Für Module_task
    bool                        has_parameters              ();
    xml::Document_ptr           parameters_as_dom           ()                                      { return _params->dom( Com_variable_set::xml_element_name(), "variable" ); }


    bool                        check_timeout               ( const Time& now );
    bool                        try_kill                    ();
    
    bool                        wait_until_terminated       ( double wait_time = latter_day );
    void                        set_delay_spooler_process   ( Time t )                              { _log->debug("delay_spooler_process=" + t.as_string() ); _next_spooler_process = Time::now() + t; }

    void                        set_state                   ( State );

    void                        set_error_xc                ( const Xc& );
    void                        set_error                   ( const Xc& x )                         { set_error_xc( x ); }
    void                        set_error                   ( const z::Xc& x )                      { set_error_xc( x ); }
    void                        set_error                   ( const exception& );
    void                        set_error                   ( const _com_error& );
    Xc_copy                     error                       ()                                     { Xc_copy result; THREAD_LOCK( _lock )  result = _error;  return result; }
    void                        reset_error                 ()                                      { THREAD_LOCK( _lock )  _error = NULL,  _non_connection_reset_error = NULL,  _is_connection_reset_error = false,  _log->reset_highest_level(); }

    void                    set_next_time                   ( const Time& );


    void                        enter_thread                ( Spooler_thread* );
    void                        leave_thread                ();

    Order*                      order                       ()                                      { return _order; }

  
    friend struct               Job;
    friend struct               Job::Task_queue;
    friend struct               Object_set;
    friend struct               Com_task;

    friend struct               Task_history;

    virtual bool                do_load                     ()                                      { return true; }
    virtual bool                do_kill                     ()                                      { return false; }
    virtual Async_operation*    do_close__start             ()                                      { return &dummy_sync_operation; }
    virtual void                do_close__end               ()                                      {}
  //virtual void                do_close                    ()                                      {}
    virtual Async_operation*    do_begin__start             ()                                      { return &dummy_sync_operation; }
    virtual bool                do_begin__end               () = 0;
    virtual Async_operation*    do_end__start               ()                                      { return &dummy_sync_operation; }
    virtual void                do_end__end                 () = 0;
    virtual Async_operation*    do_step__start              ()                                      { return &dummy_sync_operation; }
    virtual Variant             do_step__end                () = 0;
    virtual Async_operation*    do_call__start              ( const string& )                       { return &dummy_sync_operation; }
    virtual bool                do_call__end                ()                                      { return true; }           // Default: Nicht implementiert
    virtual Async_operation*    do_release__start           ()                                      { return &dummy_sync_operation; }
    virtual void                do_release__end             ()                                      {}

    virtual bool                loaded                      ()                                      { return true; }
/*
    virtual bool                do_start                    () = 0;
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
    string                     _obj_name;

    Job*                       _job;
    Thread_semaphore           _lock;
    ptr<Prefix_log>            _log;
    Spooler_thread*            _thread;
    Task_history               _history;
 
    Thread_semaphore           _terminated_events_lock;
    vector<Event*>             _terminated_events;

    Start_cause                _cause;
    double                     _cpu_time;
    int                        _step_count;
    int                        _exit_code;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _job._period.end() beenden
    bool                       _begin_called;
    bool                       _end;
    bool                       _kill_immediately;
    bool                       _closed;
    bool                       _signaled;
    int                        _delayed_after_error_task_id;


    Time                       _enqueue_time;
    Time                       _start_at;                   // Zu diesem Zeitpunkt (oder danach) starten
    Time                       _running_since;
    Time                       _ending_since;
    Time                       _last_process_start_time;
    Time                       _last_operation_time;
    Time                       _next_spooler_process;
    Time                       _next_time;
    Time                       _timeout;                    // Frist für eine Operation (oder INT_MAX)
    Time                       _idle_since;
    Time                       _idle_timeout_at;
    Time                       _subprocess_timeout;

    bool                       _killed;                     // Task abgebrochen (nach do_kill/timeout)
    bool                       _kill_tried;
    bool                       _module_instance_async_error;    // SCHEDULER-202
    bool                       _is_in_database;                   // Datensatz für diese Task ist in der Datenbank
    bool                       _running_state_reached;      // Zustand s_running... erreicht

    ptr<Async_operation>       _operation;
    ptr<Com_variable_set>      _params;
    Variant                    _result;
    string                     _name;
    ptr<Order>                 _order;
    ptr<Order>                 _order_for_task_end;         // Wird später als _order auf NULL gesetzt, damit im Fehlerfall XSLT-eMail mit <order> verschickt wird. Und für <commands on_exit_code=""/>
    string                     _changed_directories;        // Durch Semikolon getrennt

    Registered_pids            _registered_pids;            // Für add_pid() und remove_pid(). kill_task immediately_yes soll auch diese Prozesse abbrechen.
    Subprocess_register        _subprocess_register;        // Fall Task im Scheduler-Prozess läuft
    Call_state                 _call_state;
    Xc_copy                    _error;
    Xc_copy                    _non_connection_reset_error;  // Der Fehler vor einem Verbindungsfehler wird hier aufgehoben, falls er wegen ignore_signals=".." wiederhergestellt werden muss
    bool                       _is_connection_reset_error;

    ptr<Module_instance>       _module_instance;            // Nur für Module_task. Hier, damit wir nicht immer wieder casten müssen.
    ptr<Web_service>           _web_service;
};

//----------------------------------------------------------------------------------------Task_list

typedef list< ptr<Task> >   Task_list;

#define FOR_EACH_TASK( ITERATOR, TASK )  FOR_EACH( Task_list, _task_list, ITERATOR )  if( Task* TASK = *ITERATOR )

//--------------------------------------------------------------------------------------Module_task

struct Module_task : Task       // Oberklasse für Object_set_task und Job_module_task
{
                                Module_task                 ( Job* j )                              : Task(j) {}

    virtual void                set_close_engine            ( bool b )                              { if( _module_instance )  _module_instance->set_close_instance_at_end(b); }

  //virtual void                do_load                     ();
    virtual Async_operation*    do_close__start             ();
    virtual void                do_close__end               ();
  //virtual void                do_kill                     ()                                      {}
/*
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
    virtual void                do_close__end               ();
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
    virtual bool                do_load                     ();
  //virtual void                do_close__end               ();
    virtual Async_operation*    do_begin__start             ();
    virtual bool                do_begin__end               ();
    virtual Async_operation*    do_end__start               ();
    virtual void                do_end__end                 ();
    virtual Async_operation*    do_step__start              ();
    virtual Variant             do_step__end                ();
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

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
