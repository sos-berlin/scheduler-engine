// $Id: spooler_task.h,v 1.60 2002/09/12 19:30:35 jz Exp $

#ifndef __SPOOLER_TASK_H
#define __SPOOLER_TASK_H


namespace sos {
namespace spooler {


typedef int                     Level;
struct                          Task;

//-----------------------------------------------------------------------------------Level_interval

struct Level_interval
{
                                Level_interval              ()                              : _low_level(0), _high_level(0) {}
    explicit                    Level_interval              ( const xml::Element_ptr& e )   { set_xml( e ); }

    void                        set_xml                     ( const xml::Element_ptr& );

    bool                        is_in_interval              ( Level level )                 { return level >= _low_level && level < _high_level; }

    Level                      _low_level;
    Level                      _high_level;
};

//---------------------------------------------------------------------------------Object_set_class

struct Object_set_class : Sos_self_deleting
{
                                Object_set_class            ( Spooler* sp )                              : _spooler(sp), _script(sp) {}
    explicit                    Object_set_class            ( Spooler* sp,  const xml::Element_ptr& e )  : _spooler(sp), _script(sp) { set_xml( e ); }

    void                        set_xml                     ( const xml::Element_ptr& );

    Spooler*                   _spooler;
    string                     _name;
    map<Level,string>          _level_map;
    
    Script                     _script;
    bool                       _object_interface;

  //Time                       _process_timeout;
};

typedef list< Sos_ptr<Object_set_class> >  Object_set_class_list;

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
    cause_order                 = 10
};

string                          start_cause_name            ( Start_cause );

//-----------------------------------------------------------------------------------Spooler_object

struct Spooler_object
{
                                Spooler_object              ( const CComPtr<IDispatch>& dispatch = NULL ) : _idispatch(dispatch) {}

    Spooler_object&             operator =                  ( const CComPtr<IDispatch>& dispatch ) { _idispatch = dispatch; return *this; }
    Level                       level                       ();
    void                        process                     ( Level output_level );
    bool                        is_null                     ()                              { return _idispatch == NULL; }

    CComPtr<IDispatch>         _idispatch;
};

//---------------------------------------------------------------------------------Object_set_descr

struct Object_set_descr : Sos_self_deleting
{
                                Object_set_descr            ()                              {}
    explicit                    Object_set_descr            ( const xml::Element_ptr& e )   { set_xml( e ); }

    void                        set_xml                     ( const xml::Element_ptr& );

    string                     _class_name;
    Sos_ptr<Object_set_class>  _class;
    Level_interval             _level_interval;
};

//---------------------------------------------------------------------------------------Object_set

struct Object_set : Sos_self_deleting
{
                                Object_set                  ( Spooler*, Task*, const Sos_ptr<Object_set_descr>& );
                               ~Object_set                  ();

    bool                        open                        ();
    void                        close                       ();
    Spooler_object              get                         ();
    bool                        step                        ( Level result_level );
    void                        set_in_call                 ( const string& name );

    Thread*                     thread                      () const;

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Task*                      _task;
    Sos_ptr<Object_set_descr>  _object_set_descr;
    Object_set_class*          _class;
    CComPtr<IDispatch>         _idispatch;                  // Zeiger auf ein Object_set des Skripts
};

//----------------------------------------------------------------------------------------------Job

struct Job : Sos_self_deleting
{
    enum State
    {
        s_none,
        s_stopped,              // Gestoppt (z.B. wegen Fehler)
        s_read_error,           // Skript kann nicht aus Datei (include) gelesen werden
        s_pending,              // Warten auf Start
        s_start_task,           // Task aus der Warteschlange genommen, muss noch gestartet werden. 
        s_starting,             //
        s_loaded,               // Skript geladen (mit spooler_init), aber nicht gestartet (spooler_open)
        s_running,              // Läuft
        s_running_delayed,      // spooler_task.delay_spooler_process gesetzt
        s_running_wait_for_order,
        s_running_process,      // Läuft in einem externen Prozess, auf dessen Ende nur gewartet wird
        s_suspended,            // Angehalten
        s_ending,               // end(), also in spooler_close()
        s_ended,
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


    struct In_call
    {
                                In_call                     ( Task* task, const string& name, const string& extra = "" );
                                In_call                     ( Job* job  , const string& name );
                               ~In_call                     ();

        void                    set_result                  ( bool result )                     { _result = result; _result_set = true; }

        Job*                   _job;
        Log_indent             _log_indent;
        string                 _name;                       // Fürs Log
        bool                   _result_set;
        bool                   _result;
    };


    typedef list< Sos_ptr<Task> >               Task_queue;
    typedef list< Sos_ptr<Directory_watcher> >  Directory_watcher_list;
    typedef map< int, Time >                    Delay_after_error;

                                Job                         ( Thread* );
                               ~Job                         (); 

    void                        set_xml                     ( const xml::Element_ptr& );
    xml::Element_ptr            xml                         ( xml::Document_ptr, bool show_all );

    void                        init                        ();
    void                        init2                       ();

    const string&               name                        () const                    { return _name; }
    State_cmd                   state_cmd                   () const                    { return _state_cmd; }
    State                       state                       () const                    { return _state; }
    Object_set_descr*           object_set_descr            () const                    { return _object_set_descr; }
    int                         priority                    () const                    { return _priority; }
    Thread*                     thread                      () const                    { return _thread; }
    string                      job_state                   ();
    string                      include_path                () const;
    string                      title                       ()                          { string title; THREAD_LOCK( _lock )  title = _title;  return title; }
    string                      jobname_as_filename         ();
    string                      profile_section             ();
    bool                        temporary                   () const                    { return _temporary; }
    void                        set_in_call                 ( const string& name, const string& extra = "" );
    void                        set_delay_after_error       ( int error_steps, Time delay ) { _delay_after_error[error_steps] = delay; }

    xml::Element_ptr            read_history                ( xml::Document_ptr doc, int id, int n, bool with_log ) { return _history.read_tail( doc, id, n, with_log ); }

    void                        close                       ();
    void                        close_engine                ();
    void                        close_engine2               ();

    void                        start                       ( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name, Time = 0 );
    Sos_ptr<Task>               start_without_lock          ( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name, Time = 0, bool log = false );
    void                        start_when_directory_changed( const string& directory_name, const string& filename_pattern );
    void                        clear_when_directory_changed();
    void                        signal                      ( const string& signal_name = "" )  { _event.signal(signal_name); }
    void                        wake                        ()                                  { _event.signal( "wake" ); }
    void                        interrupt_script            ();
    void                        select_period               ( Time = Time::now() );
    bool                        is_in_period                ( Time = Time::now() );
    bool                        its_current_task            ( Task* task )              { return task == _task; }
    Task*                       current_task                ()                          { return _task; }
    bool                        queue_filled                ()                          { return !_task_queue.empty(); }

    Sos_ptr<Task>               create_task                 ( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name, Time = latter_day );
    bool                        dequeue_task                ( Time now = Time::now() );
    void                        remove_from_task_queue      ( Task* );
    void                        close_task                  ();
    bool                        read_script                 ();
    bool                        load                        ();
    void                        end                         ();
    void                        stop                        ();
    void                        finish                      ();
    void                        set_next_start_time         ( Time now = Time::now() );
    void                        set_next_time               ( Time now = Time::now() );

    bool                        execute_state_cmd           ();
    void                        reread                      ();
    void                        task_to_start               ();
    bool                        do_something                ();
    bool                        should_removed              ()                          { return _temporary && _state == s_stopped; }
    void                        set_mail_defaults           ();
    void                        clear_mail                  ();
    void                        send_collected_log          ();

    void                        set_repeat                  ( double seconds )          { _log.debug( "repeat=" + as_string(seconds) );  _repeat = seconds; }

    void                        set_error_xc                ( const Xc& );
    void                        set_error_xc_only           ( const Xc& );
    void                        set_error                   ( const Xc& x )             { set_error_xc( x ); }
    void                        set_error                   ( const exception& );
    void                        set_error                   ( const _com_error& );
    Xc_copy                     error                       ()                          { Xc_copy result; THREAD_LOCK( _lock )  result = _error;  return result; }
    bool                        has_error                   ()                          { return !!_error; }  //|| _log.highest_level() >= log_error; }
    void                        reset_error                 ()                          { _error = NULL;  _log.reset_highest_level(); }

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );
    void                        kill_task                   ( int id );

    string                      state_name                  ()                          { return state_name( _state ); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    string                      state_cmd_name              ()                          { return state_cmd_name( _state_cmd ); }
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );

    void                        set_state_text              ( const string& text )      { _state_text = text; _log.debug9( "state_text = " + text ); }

    CComPtr<Com_job>&           com_job                     ()                          { return _com_job; }
    void                        signal_object               ( const string& object_set_class_name, const Level& );

    Order_queue*                order_queue                 () const                    { return _order_queue; }

    virtual string             _obj_name                    () const                    { return "Job " + _name; }


    friend struct               Object_set;
    friend struct               Task;
    friend struct               Script_task;
    friend struct               Job_script_task;
    friend struct               Object_set_task;
    friend struct               Process_task;
    friend struct               Com_job;
    friend struct               Thread;


    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Spooler*                   _spooler;

  protected:
    friend struct Job_history;

    string                     _name;
    Thread*                    _thread;


    Prefix_log                 _log;
    bool                       _log_append;                 // Jobprotokoll fortschreiben <job log_append=(yes|no)>

    Sos_ptr<Object_set_descr>  _object_set_descr;           // Job nutzt eine Objektemengeklasse
    Level                      _output_level;
    Script                     _script;                     // Job hat ein eigenes Skript
    xml::Element_ptr           _script_element;             // <script> (mit <include>) für <modify_job cmd="reload"/>
    
    string                     _process_filename;           // Job ist ein externes Programm
    string                     _process_param;              // Parameter für das Programm
    string                     _process_log_filename;

    Run_time                   _run_time;
    int                        _priority;
    bool                       _temporary;                  // Job nach einem Lauf entfernen
    bool                       _start_once;                 // <run_time start_once="">, wird false nach Start
    Delay_after_error          _delay_after_error;
    int                        _error_steps;                // Zahl aufeinanderfolgender Fehler

    string                     _title;                      // <job title="">
    string                     _description;                // <description>
    string                     _state_text;                 // spooler_job.state_text = "..."

    CComPtr<Com_variable_set>  _default_params;

    xml::Element_ptr           _script_xml_element;         // <script> aus <config>
    Script*                    _script_ptr;
    Script_instance            _script_instance;            // Für use_engine="job"

    int                        _step_count;                 // Anzahl spooler_process() aller Tasks
    int                        _last_task_step_count;       // Anzahl spooler_process() der letzten Task
    bool                       _has_spooler_process;
    Directory_watcher_list     _directory_watcher_list;
    Event                      _event;                      // Zum Starten des Jobs

    State                      _state;
    State_cmd                  _state_cmd;
    bool                       _reread;                     // <script> neu einlesen, also <include> erneut ausführen
    string                     _in_call;                    // "spooler_process" etc.
    Time                       _delay_until;                // Nach Fehler verzögern
    Time                       _next_start_time;
    Time                       _next_time;                  // Für Thread::wait(): Um diese Zeit soll Job::do_something() gerufen werden.
    Period                     _period;                     // Derzeitige oder nächste Period
    Time                       _next_single_start;
    Time                       _repeat;                     // spooler_task.repeat

    CComPtr<Com_job>           _com_job;
    CComPtr<Com_log>           _com_log;
    CComPtr<Com_task>          _com_task;                   // Objekt bleibt, Inhalt wechselt über die Tasks hinweg (für use_engine="job")
    Xc_copy                    _error;
    bool                       _close_engine;               // Bei einem Fehler in spooler_init()
    Sos_ptr<Task>              _task;                       // Es kann nur eine Task geben. Zirkel: _task->_job == this
    Task_queue                 _task_queue;                 // Warteschlange der nächsten zu startenden Tasks

    ptr<Order_queue>           _order_queue;

    Job_history                _history;
};

//------------------------------------------------------------------------------------------Job_list

typedef list< Sos_ptr<Job> >    Job_list;

//----------------------------------------------------------------------------------------------Task

struct Task : Sos_self_deleting
{
                                Task                        ( Spooler*, const Sos_ptr<Job>& );
                               ~Task                        ();

    int                         id                          ()                              { return _id; }

    void                        cmd_end                     ();

    void                        close                       ();

    bool                        start                       ();
    void                        end                         ();
    bool                        step                        ();
    void                        on_error_on_success         ();

    void                        set_cause                   ( Start_cause );
    void                        set_history_field           ( const string& name, const CComVariant& value );
    void                        set_close_engine            ( bool b )                      { _close_engine = b; }
    bool                        has_parameters              ();
    xml::Document_ptr           parameters_as_dom           ();


    bool                        wait_until_terminated       ( double wait_time = latter_day );
  //void                        set_start_at                ( Time );
    void                        set_delay_spooler_process   ( Time t )                      { _job->_log.debug("delay_spooler_process=" + t.as_string() ); _next_spooler_process = Time::now() + t; }

    Job*                        job                         ()                              { return _job; }
    Order*                      order                       ()                              { return _order; }

    virtual string             _obj_name                    () const                        { return "Task \"" + _name + "\" (" + _job->obj_name() + ")"; }
  
    friend struct               Job;
    friend struct               Object_set;
    friend struct               Com_task;

  protected:
    friend struct Job_history;

    virtual bool                loaded                      ()                              { return true; }
    virtual void                do_close                    ()                              {}
    virtual bool                do_load                     ()                              { return true; }
    virtual bool                do_start                    () = 0;
    virtual void                do_stop                     ()                              {}
    virtual void                do_end                      () = 0;
    virtual bool                do_step                     () = 0;
    virtual void                do_on_success               () = 0;
    virtual void                do_on_error                 () = 0;
    virtual bool                has_step_count              ()                              { return true; }

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;                        // Zirkel!

    int                        _id;
    Start_cause                _cause;
    double                     _cpu_time;
    int                        _step_count;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _job._period.end() beenden
    bool                       _opened;
    bool                       _on_error_called;
    bool                       _closed;

    Time                       _enqueue_time;
    Time                       _start_at;                   // Zu diesem Zeitpunkt (oder danach) starten
    Time                       _running_since;

    CComPtr<spooler_com::Ivariable_set> _params;
    CComVariant                _result;
    string                     _name;
    Time                       _next_spooler_process;
    bool                       _close_engine;               // Nach Task-Ende Scripting Engine schließen (für use_engine="job")
  //Xc_copy                    _error;
    ptr<Order>                 _order;

    Thread_semaphore           _terminated_events_lock;
    vector<Event*>             _terminated_events;
};

typedef list< Sos_ptr<Task> >   Task_list;

//--------------------------------------------------------------------------------------Script_task

struct Script_task : Task
{
                                Script_task                 ( Spooler* sp, const Sos_ptr<Job>& j ) : Task(sp,j) {}

    virtual bool                loaded                      ()                              { return _job->_script_instance.loaded(); }
  //virtual bool                do_load                     ();
  //bool                        do_start                    ();
  //void                        do_end                      ();
  //bool                        do_step                     ();
    void                        do_on_success               ();
    void                        do_on_error                 ();
};

//----------------------------------------------------------------------------------Object_set_task

struct Object_set_task : Script_task
{
                                Object_set_task             ( Spooler* sp, const Sos_ptr<Job>& j ) : Script_task(sp,j) {}

    virtual bool                loaded                      ()                              { return _object_set && Script_task::loaded(); }
    virtual bool                do_load                     ();
    void                        do_close                    ();
    bool                        do_start                    ();
    void                        do_end                      ();
    bool                        do_step                     ();
  //void                        do_on_success               ();
  //void                        do_on_error                 ();

    Sos_ptr<Object_set>        _object_set;
    CComPtr<Com_object_set>    _com_object_set;
};

//----------------------------------------------------------------------------------Job_script_task

struct Job_script_task : Script_task
{
                                Job_script_task             ( Spooler* sp, const Sos_ptr<Job>& j ) : Script_task(sp,j) {}

  //virtual bool                loaded                      ();
    virtual bool                do_load                     ();
    bool                        do_start                    ();
    void                        do_end                      ();
    bool                        do_step                     ();
  //void                        do_on_success               ();
  //void                        do_on_error                 ();
};

//-------------------------------------------------------------------------------------Process_task

struct Process_task : Task
{
                                Process_task                ( Spooler* sp, const Sos_ptr<Job>& j ) : Task(sp,j), _process_handle("process_handle",(HANDLE)NULL) {}
        
  //virtual bool                loaded                      ();
  //virtual bool                do_load                     ();
    bool                        do_start                    ();
    void                        do_stop                     ();
    void                        do_end                      ();
    bool                        do_step                     ();
    void                        do_on_success               ()                                  {}
    void                        do_on_error                 ()                                  {}
    virtual bool                has_step_count              ()                                  { return false; }

    Process_id                 _process_id;
    Event                      _process_handle;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
