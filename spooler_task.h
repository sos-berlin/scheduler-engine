// $Id: spooler_task.h,v 1.25 2002/02/28 16:46:06 jz Exp $

#ifndef __SPOOLER_TASK_H
#define __SPOOLER_TASK_H

//#include <queue>
//using std::queue;

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
        s_pending,              // Warten auf Start
        s_start_task,           // Task aus der Warteschlange genommen, muss noch gestartet werden. 
        s_starting,             //
        s_loaded,               // Skript geladen (mit spooler_init), aber nicht gestartet (spooler_open)
        s_running,              // Läuft
        s_running_process,      // Läuft in einem externen Prozess, auf dessen Ende nur gewartet wird
        s_suspended,            // Angehalten
        s_ending,               // end(), also in spooler_close()
        s_ended,
      //s_closed,               // 
        s__max
    };

    enum State_cmd
    {
        sc_none,
        sc_stop,                // s_running | s_suspended  -> s_stopped
        sc_unstop,              // s_stopped                -> s_pending
        sc_start,               // s_pending                -> s_running
        sc_wake,                // s_pending | s_running    -> s_running
        sc_end,                 // s_running                -> s_pending
        sc_suspend,             // s_running                -> s_suspended
        sc_continue,            // s_suspended              -> s_running
        sc__max
    };


    struct In_call
    {
                                In_call                     ( Task* task, const string& name );
                                In_call                     ( Job* job  , const string& name );
                               ~In_call                     ();

        void                    set_result                  ( bool result )                     { _result = result; _result_set = true; }

        Job*                   _job;
        Log_indent             _log_indent;
        string                 _name;                       // Fürs Log
        bool                   _result_set;
        bool                   _result;
    };


    typedef list< Sos_ptr<Task> >                   Task_queue;
    typedef vector< Sos_ptr<Directory_watcher> >    Directory_watcher_array;


                                Job                         ( Thread* );
                               ~Job                         (); 

    void                        set_xml                     ( const xml::Element_ptr& );
    xml::Element_ptr            xml                         ( xml::Document_ptr, bool show_all );

    void                        init                        ();

    const string&               name                        () const                    { return _name; }
    State_cmd                   state_cmd                   () const                    { return _state_cmd; }
    State                       state                       () const                    { return _state; }
    Object_set_descr*           object_set_descr            () const                    { return _object_set_descr; }
    int                         priority                    () const                    { return _priority; }
    Thread*                     thread                      () const                    { return _thread; }
    string                      job_state                   ();
    string                      include_path                () const;
    void                        set_in_call                 ( const string& name );

    void                        close                       ();
    void                        close_engine                ();

    void                        start                       ( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name );
    Sos_ptr<Task>               start_without_lock          ( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name );
    void                        start_when_directory_changed( const string& directory_name );
    void                        clear_when_directory_changed();
    void                        wake                        ()                          { _event.signal( "wake" ); }
    void                        interrupt_script            ();

    Sos_ptr<Task>               create_task                 ( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name );
    bool                        dequeue_task                ();
    void                        remove_from_task_queue      ( Task* );
    void                        close_task                  ();
    bool                        load                        ();
    void                        end                         ();
    void                        stop                        ();
    void                        set_next_start_time         ( Time now = Time::now() );
    bool                        do_something                ();
    bool                        should_removed              ()                          { return _temporary && _state == s_stopped; }

    void                        set_repeat                  ( double seconds )          { _repeat = seconds; }

    void                        set_error                   ( const Xc& );
    void                        set_error                   ( const exception& );
    Xc_copy                     error                       ()                          { THREAD_LOCK( _lock )  return _error; }
    bool                        has_error                   ()                          { return !!_error; }

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );

    string                      state_name                  ()                          { return state_name( _state ); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    string                      state_cmd_name              ()                          { return state_cmd_name( _state_cmd ); }
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );

    CComPtr<Com_job>&           com_job                     ()                          { return _com_job; }
    void                        signal_object               ( const string& object_set_class_name, const Level& );

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

  protected:
    string                     _name;
    Spooler*                   _spooler;
    Thread*                    _thread;


    Prefix_log                 _log;
    Sos_ptr<Object_set_descr>  _object_set_descr;           // Job nutzt eine Objektemengeklasse
    Level                      _output_level;
    Script                     _script;                     // Job hat ein eigenes Skript
    string                     _process_filename;           // Job ist ein externes Programm
    string                     _process_param;              // Parameter für das Programm
    Run_time                   _run_time;
    int                        _priority;
    bool                       _temporary;                  // Job nach einem Lauf entfernen
    string                     _title;
    string                     _description;
    xml::Element_ptr           _xml_element;                // <job> aus <config>

    Script*                    _script_ptr;
    Script_instance            _script_instance;            // Für use_engine="job"
    bool                       _has_spooler_process;
    Directory_watcher_array    _directory_watcher_array;
    Event                      _event;                      // Zum Starten des Jobs

    State                      _state;
    State_cmd                  _state_cmd;
    string                     _in_call;                    // "spooler_process" etc.
    Time                       _next_start_time;
    Time                       _next_start_at;
    Period                     _period;                     // Derzeitige oder nächste Period
    Time                       _repeat;                     // spooler_task.repeat

    CComPtr<Com_job>           _com_job;
    CComPtr<Com_log>           _com_log;
    CComPtr<Com_task>          _com_task;                   // Objekt bleibt, Inhalt wechselt über die Tasks hinweg
    Xc_copy                    _error;
    bool                       _load_error;                 // Fehler beim Laden oder spooler_init()
    Sos_ptr<Task>              _task;                       // Es kann nur eine Task geben. Zirkel: _task->_job == this
    Task_queue                 _task_queue;                 // Warteschlange der nächsten zu startenden Tasks
};

//------------------------------------------------------------------------------------------Job_list

typedef list< Sos_ptr<Job> >    Job_list;

//----------------------------------------------------------------------------------------------Task

struct Task : Sos_self_deleting
{
                                Task                        ( Spooler*, const Sos_ptr<Job>& );
                               ~Task                        ();

    void                        cmd_end                     ();

    void                        close                       ();

    bool                        start                       ();
    void                        end                         ();
    bool                        step                        ();
    void                        on_error_on_success         ();


    bool                        wait_until_terminated       ( double wait_time = latter_day );
    void                        set_start_at                ( Time );

    Job*                        job                         ()                              { return _job; }

    virtual string             _obj_name                    () const                        { return "Task \"" + _name + "\" (" + _job->obj_name() + ")"; }
  
    friend struct               Job;
    friend struct               Object_set;
    friend struct               Com_task;

  protected:
    virtual void                do_close                    ()                              {}
    virtual bool                do_start                    () = 0;
    virtual void                do_stop                     ()                              {}
    virtual void                do_end                      () = 0;
    virtual bool                do_step                     () = 0;
    virtual void                do_on_success               () = 0;
    virtual void                do_on_error                 () = 0;

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;                        // Zirkel!
  //Thread_semaphore           _lock;
    
    double                     _cpu_time;
    int                        _step_count;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _job._period.end() beenden
    bool                       _opened;

    Time                       _enqueue_time;
    Time                       _start_at;                   // Zu diesem Zeitpunkt (oder danach) starten
    Time                       _running_since;

    CComPtr<spooler_com::Ivariable_set> _params;
    CComVariant                _result;
    string                     _name;
    Xc_copy                    _error;

    Thread_semaphore           _terminated_events_lock;
    vector<Event*>             _terminated_events;
};

typedef list< Sos_ptr<Task> >   Task_list;

//--------------------------------------------------------------------------------------Script_task

struct Script_task : Task
{
                                Script_task                 ( Spooler* sp, const Sos_ptr<Job>& j ) : Task(sp,j) {}

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
        
    bool                        do_start                    ();
    void                        do_stop                     ();
    void                        do_end                      ();
    bool                        do_step                     ();
    void                        do_on_success               ()                                  {}
    void                        do_on_error                 ()                                  {}

    Process_id                 _process_id;
    Event                      _process_handle;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
