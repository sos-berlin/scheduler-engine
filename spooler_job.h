// $Id: spooler_job.h,v 1.13 2003/12/03 08:52:44 jz Exp $

#ifndef __SPOOLER_JOB_H
#define __SPOOLER_JOB_H

namespace sos {
namespace spooler {


typedef int                     Level;
struct                          Task;
struct                          Module_task;

//-----------------------------------------------------------------------------------Level_interval

struct Level_interval
{
                                Level_interval              ()                                      : _low_level(0), _high_level(0) {}
    explicit                    Level_interval              ( const xml::Element_ptr& e )           { set_dom( e ); }

    void                        set_dom                     ( const xml::Element_ptr& );

    bool                        is_in_interval              ( Level level )                         { return level >= _low_level && level < _high_level; }

    Level                      _low_level;
    Level                      _high_level;
};

//---------------------------------------------------------------------------------Object_set_class

struct Object_set_class : Sos_self_deleting
{
                                Object_set_class            ( Spooler* sp, Prefix_log* log )        : _spooler(sp), _module(sp,log) {}
    explicit                    Object_set_class            ( Spooler* sp, Prefix_log* log, const xml::Element_ptr& e, const Time& xml_mod_time )  : _spooler(sp), _module(sp,log) { set_dom( e, xml_mod_time ); }

    void                        set_dom                     ( const xml::Element_ptr&, const Time& xml_mod_time );

    Spooler*                   _spooler;
    string                     _name;
    map<Level,string>          _level_map;
    
    Module                     _module;
    bool                       _object_interface;

  //Time                       _process_timeout;
};

typedef list< Sos_ptr<Object_set_class> >  Object_set_class_list;

//-----------------------------------------------------------------------------------Spooler_object

struct Spooler_object
{
                                Spooler_object              ( const ptr<IDispatch>& dispatch = NULL ) : _idispatch(dispatch) {}

    Spooler_object&             operator =                  ( const ptr<IDispatch>& dispatch )      { _idispatch = dispatch; return *this; }
    Level                       level                       ();
    void                        process                     ( Level output_level );
    bool                        is_null                     ()                                      { return _idispatch == NULL; }

    ptr<IDispatch>             _idispatch;
};

//---------------------------------------------------------------------------------Object_set_descr

struct Object_set_descr : Sos_self_deleting
{
                                Object_set_descr            ()                                      {}
    explicit                    Object_set_descr            ( const xml::Element_ptr& e )           { set_dom( e ); }

    void                        set_dom                     ( const xml::Element_ptr& );

    string                     _class_name;
    Sos_ptr<Object_set_class>  _class;
    Level_interval             _level_interval;
};

//---------------------------------------------------------------------------------------Object_set

struct Object_set : Sos_self_deleting
{
                                Object_set                  ( Spooler*, Module_task*, const Sos_ptr<Object_set_descr>& );
                               ~Object_set                  ();

    bool                        open                        ();
    void                        close                       ();
    Spooler_object              get                         ();
    bool                        step                        ( Level result_level );
    void                        set_in_call                 ( const string& name );

  //Spooler_thread*             thread                      () const;

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Module_task*               _task;
    Sos_ptr<Object_set_descr>  _object_set_descr;
    Object_set_class*          _class;
    ptr<IDispatch>             _idispatch;                  // Zeiger auf ein Object_set des Skripts
};

//--------------------------------------------------------------------------------------Start_cause

enum Start_cause
{
    cause_none                  = 0,    // Kein Start
    cause_period_once           = 1,    // <run_time once="yes">
    cause_period_single         = 2,    // <run_time single_start="yes">
    cause_period_repeat         = 3,    // <run_time repeat="..">
    cause_job_repeat            = 4,    // spooler_job.repeat = ..
    cause_queue                 = 5,    // <start_job at="">
    cause_queue_at              = 6,    // <start_job at="..">
    cause_directory             = 7,    // start_when_directory_changed
    cause_signal                = 8,
    cause_delay_after_error     = 9,
    cause_order                 = 10,
    cause_wake                  = 11    // sc_wake
};

string                          start_cause_name            ( Start_cause );

//----------------------------------------------------------------------------------------------Job

struct Job : Sos_self_deleting
{
    enum State
    {
        s_none,
        s_stopping,             // Wird gestoppt (Zustand, solange noch Tasks laufen, danach s_stopped)
        s_stopped,              // Gestoppt (z.B. wegen Fehler). Keine Task wird gestartet.
        s_read_error,           // Skript kann nicht aus Datei (include) gelesen werden
        s_pending,              // Warten auf Start
        s_running,              // Mindestens eine Task läuft (die Tasks können aber ausgesetzt, also gerade nicht aktiv sein: s_suspended etc.)
        s__max
    };

    enum State_cmd
    {
        sc_none,
        sc_stop,                // s_running || s_suspended  -> s_stopped
        sc_unstop,              // s_stopped                 -> s_pending
        sc_start,               // s_pending                 -> s_running
        sc_wake,                // s_pending || s_running    -> s_running
        sc_end,                 // s_running                 -> s_pending
        sc_suspend,             // s_running                 -> s_suspended
        sc_continue,            // s_suspended               -> s_running
        sc_reread,              // Job::_reread = true
        sc__max
    };



    typedef list< Sos_ptr<Task> >               Task_queue;
    typedef list< Sos_ptr<Task> >               Task_list;
    typedef list< ptr<Directory_watcher> >      Directory_watcher_list;
    typedef map< int, Time >                    Delay_after_error;
    typedef map< int, Time >                    Delay_order_after_setback;

                                Job                         ( Spooler* );
                               ~Job                         (); 

    void                    set_dom                         ( const xml::Element_ptr&, const Time& mod_time );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what, Job_chain* = NULL );

    void                        init0                       ();                                     // Wird vor Spooler-Skript gerufen
    void                        init                        ();                                     // Wird nach Spooler-Skript gerufen, ruft auch init2()
    void                        init2                       ();                                     // Wird nach reread() gerufen

    const string&               name                        () const                                { return _name; }
    State_cmd                   state_cmd                   () const                                { return _state_cmd; }
    State                       state                       () const                                { return _state; }
    Object_set_descr*           object_set_descr            () const                                { return _object_set_descr; }
    int                         priority                    () const                                { return _priority; }
  //Spooler_thread*             thread                      () const                                { return _thread; }
    string                      job_state                   ();
    string                      include_path                () const;
    string                      title                       ()                                      { string title; THREAD_LOCK( _lock )  title = _title;  return title; }
    string                      jobname_as_filename         ();
    string                      profile_section             ();
    bool                        temporary                   () const                                { return _temporary; }
    void                        set_delay_after_error       ( int error_steps, Time delay )         { _log.debug9( "delay_after_error["        +as_string(error_steps)+"]="+delay.as_string() ); _delay_after_error        [error_steps] = delay; }
    void                        set_delay_order_after_setback( int setbacks, Time delay )           { _log.debug9( "delay_order_after_setback["+as_string(setbacks  )+"]="+delay.as_string() ); _delay_order_after_setback[setbacks   ] = delay; }
    Time                        get_delay_order_after_setback( int setback_count );
    void                        set_max_order_setbacks      ( int n )                               { _log.debug9( "max_order_setbacks"+as_string(n) ); _max_order_setbacks = n; }
    int                         max_order_setbacks          () const                                { return _max_order_setbacks; }
    xml::Element_ptr            read_history                ( const xml::Document_ptr& doc, int id, int n, Show_what show ) { return _history.read_tail( doc, id, n, show ); }

    void                        close                       ();

    void                        start                       ( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time = 0 );
    Sos_ptr<Task>               start_without_lock          ( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time = 0, bool log = false );
    void                        start_when_directory_changed( const string& directory_name, const string& filename_pattern );
    void                        clear_when_directory_changed();
    void                        interrupt_script            ();
    void                        select_period               ( Time = Time::now() );
    bool                        is_in_period                ( Time = Time::now() );
    bool                        queue_filled                ()                                      { return !_task_queue.empty(); }

    Sos_ptr<Task>               create_task                 ( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time = latter_day );
    void                        enqueue_task                ( const Sos_ptr<Task>& );
    Sos_ptr<Task>               get_task_from_queue         ( Time now );
    void                        run_task                    ( const Sos_ptr<Task>&  );

    void                        remove_from_task_queue      ( Task*, Log_level );
    void                        remove_running_task         ( Task* );
  //void                        close_task                  ();
    bool                        read_script                 ();
  //void                        end                         ();
    void                        stop                        ( bool end_all_tasks );
    void                        set_next_start_time         ( Time now, bool repeat = false );
    void                        set_next_time               ( Time );
    void                        calculate_next_time         ( Time now = Time::now() );

    Time                        next_time                   ()                                      { THREAD_LOCK_RETURN( _lock, Time, _next_time ); }

    bool                        execute_state_cmd           ();
    void                        reread                      ();
    Sos_ptr<Task>               task_to_start               ();
    bool                        do_something                ();
    bool                        should_removed              ()                                      { return _temporary && _state == s_stopped; }

    void                    set_repeat                      ( double seconds )                      { THREAD_LOCK( _lock )  _log.debug( "repeat=" + as_string(seconds) ),  _repeat = seconds; }
    Time                        repeat                      ()                                      { THREAD_LOCK_RETURN( _lock, Time, _repeat ); }

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );
    void                        kill_task                   ( int id, bool immediately = false );

    string                      state_name                  ()                                      { return state_name( _state ); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    string                      state_cmd_name              ()                                      { return state_cmd_name( _state_cmd ); }
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );

    void                        set_state_text              ( const string& text )                  { THREAD_LOCK( _lock )  _state_text = text, _log.debug9( "state_text = " + text ); }

    void                        set_error_xc                ( const Xc& );
    void                        set_error_xc_only           ( const Xc& );
    void                        set_error                   ( const exception& );
    void                        reset_error                 ()                                      { THREAD_LOCK( _lock )  _error = NULL,  _log.reset_highest_level(); }

    void                        signal                      ( const string& signal_name );
    void                        notify_a_process_is_idle    ();                                     // Vielleicht wird bald ein Prozess frei?
    void                        remove_waiting_job_from_process_list();

    ptr<Com_job>&               com_job                     ()                                      { return _com_job; }
    void                        signal_object               ( const string& object_set_class_name, const Level& );

    Order_queue*                order_queue                 () const                                { return _order_queue; }
    bool                        order_controlled            () const                                { return _order_queue != NULL; }

    void                        set_job_chain_priority      ( int pri )                             { THREAD_LOCK(_lock) if( _job_chain_priority < pri )  _job_chain_priority = pri; }
    static bool                 higher_job_chain_priority   ( const Job* a, const Job* b )          { return a->_job_chain_priority > b->_job_chain_priority; }

    ptr<Module_instance>        create_module_instance      ();
    Module_instance*            get_free_module_instance    ( Task* );
    void                        release_module_instance     ( Module_instance* );

    void                        increment_running_tasks     ()                                      { InterlockedIncrement( &_running_tasks_count ); }
    void                        decrement_running_tasks     ()                                      { InterlockedDecrement( &_running_tasks_count ); }

    void                        count_task                  ()                                      { InterlockedIncrement( &_tasks_count ); }
    void                        count_step                  ()                                      { InterlockedIncrement( &_step_count ); }

    virtual string             _obj_name                    () const                                { return "Job " + _name; }


    friend struct               Object_set;
    friend struct               Task;
    friend struct               Module_task;
    friend struct               Job_module_task;
    friend struct               Object_set_task;
    friend struct               Process_task;
    friend struct               Com_job;
    friend struct               Spooler_thread;


    Fill_zero                  _zero_;
    string                     _name;
    Thread_semaphore           _lock;
    Spooler*                   _spooler;
    Prefix_log                 _log;
    bool                       _waiting_for_process;        // Task kann nicht gestartet werden, weil kein Prozess in der Prozessklasse verfügbar ist
    bool                       _waiting_for_process_try_again;  

  protected:
    friend struct               Job_history;

    string                     _title;                      // <job title="">
    string                     _description;                // <description>
    string                     _state_text;                 // spooler_job.state_text = "..."

    string                     _process_filename;           // Job ist ein externes Programm
    string                     _process_param;              // Parameter für das Programm
    string                     _process_log_filename;
    bool                       _process_ingore_error;

    long                       _tasks_count;                // Anzahl gestarteter Tasks seit Spooler-Start
    long                       _step_count;                 // Anzahl spooler_process() aller Tasks

    State                      _state;
    State_cmd                  _state_cmd;
    bool                       _reread;                     // <script> neu einlesen, also <include> erneut ausführen
    Time                       _delay_until;                // Nach Fehler verzögern
    Time                       _next_start_time;
    Time                       _next_time;                  // Für Spooler_thread::wait(): Um diese Zeit soll Job::do_something() gerufen werden.
    Time                       _next_single_start;
    Time                       _repeat;                     // spooler_task.repeat
    Time                       _task_timeout;               // Frist für einen Schritt einer Task
    Time                       _idle_timeout;               // Frist für den Zustand Task::s_running_waiting_for_order
    int                        _priority;
    bool                       _temporary;                  // Job nach einem Lauf entfernen
    bool                       _start_once;                 // <run_time start_once="">, wird false nach Start

    bool                       _log_append;                 // Jobprotokoll fortschreiben <job log_append=(yes|no)>

    Run_time                   _run_time;
    Period                     _period;                     // Derzeitige oder nächste Period
    Delay_after_error          _delay_after_error;
    long                       _error_steps;                // Zahl aufeinanderfolgender Fehler

    Directory_watcher_list     _directory_watcher_list;
    Xc_copy                    _error;

    ptr<Com_variable_set>      _default_params;


    Module                     _module;                     // Job hat ein eigenes Skript
    xml::Element_ptr           _script_element;             // <script> (mit <include>) für <modify_job cmd="reload"/>

    xml::Document_ptr          _module_xml_document;
    xml::Element_ptr           _module_xml_element;         // <script> aus <config>
    Time                       _module_xml_mod_time;
    Module*                    _module_ptr;
    typedef vector< ptr<Module_instance> >  Module_instance_vector;
    Module_instance_vector     _module_instances;
    ptr<Com_job>               _com_job;

    Sos_ptr<Object_set_descr>  _object_set_descr;           // Job nutzt eine Objektemengeklasse
    Level                      _output_level;
    
    Task_queue                 _task_queue;                 // Warteschlange der nächsten zu startenden Tasks
    Task_list                  _running_tasks;              // Alle laufenden Tasks (auch die gestarteten, aber wartenden, z.B. s_running_waiting_for_order)
    long                       _running_tasks_count;        // Anzahl der Tasks, die tatsächlich laufen (und nicht gerade warten)
    int                        _max_tasks;                  // Max. Anzahl gleichzeitig laufender Tasks. _running_tasks.size() <= _max_tasks!

    Job_history                _history;

    ptr<Order_queue>           _order_queue;
    int                        _job_chain_priority;         // Maximum der Prioritäten aller Jobkettenknoten mit diesem Job. 

    Delay_order_after_setback  _delay_order_after_setback;
    int                        _max_order_setbacks;
};

//------------------------------------------------------------------------------------------Job_list

typedef list< Sos_ptr<Job> >    Job_list;

#define FOR_EACH_JOB( ITERATOR )  FOR_EACH( Job_list, _job_list, ITERATOR )

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
