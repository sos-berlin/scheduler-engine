// $Id: spooler_task.h,v 1.10 2001/02/10 11:38:07 jz Exp $

#ifndef __SPOOLER_TASK_H
#define __SPOOLER_TASK_H

namespace sos {
namespace spooler {

typedef int                     Level;

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
                                Object_set_class            ()                              {}
    explicit                    Object_set_class            ( const xml::Element_ptr& e )   { set_xml( e ); }

    void                        set_xml                     ( const xml::Element_ptr& );

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

    void                        open                        ();
    void                        close                       ();
    Spooler_object              get                         ();
    bool                        step                        ( Level result_level );

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
        s_loaded,               // Skript geladen (mit spooler_init), aber nicht gestartet (spooler_open)
        s_pending,              // Warten auf Start
        s_task_created,
        s_running,              // Läuft
        s_running_process,      // Läuft in einem externen Prozess, auf dessen Ende nur gewartet wird
        s_suspended,            // Angehalten
        s_ending,               // end()
        s_ended,
        s__max
    };

    enum State_cmd
    {
        sc_none,
        sc_stop,                // s_running | s_suspended -> s_stopped
        sc_unstop,              // s_stopped -> s_pending
        sc_start,               // s_pending -> s_running
        sc_end,                 // s_running -> s_pending
        sc_suspend,             // s_running -> s_suspended
        sc_continue,            // s_suspended -> s_running
        sc__max
    };


                                Job                         ( Thread* );
                               ~Job                         (); 

    void                        set_xml                     ( const xml::Element_ptr& );
    xml::Element_ptr            xml                         ( xml::Document_ptr );

    void                        init                        ();

    const string&               name                        () const                    { return _name; }
    State_cmd                   state_cmd                   () const                    { return _state_cmd; }
    State                       state                       () const                    { return _state; }
    Object_set_descr*           object_set_descr            () const                    { return _object_set_descr; }
    int                         priority                    () const                    { return _priority; }
    Thread*                     thread                      () const                    { return _thread; }

    void                        close                       ();
    void                        close_engine                ();

    void                        create_task                 ();
    void                        close_task                  ();
    void                        start                       ( const CComPtr<spooler_com::Ivariable_set>& params = NULL );
    void                        start_without_lock          ( const CComPtr<spooler_com::Ivariable_set>& params = NULL );
    void                        start_when_directory_changed( const string& directory_name );
    void                        load                        ();
    void                        end                         ();
    void                        stop                        ();
    void                        set_next_start_time         ( Time now = Time::now() );
    bool                        do_something                ();

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

    friend struct               Object_set;
    friend struct               Task;
    friend struct               Script_task;
    friend struct               Job_script_task;
    friend struct               Object_set_task;
    friend struct               Process_task;
    friend struct               Com_job;
    friend struct               Thread;


  protected:
    Fill_zero                  _zero_;
    string                     _name;
    Spooler*                   _spooler;
    Thread*                    _thread;

    Thread_semaphore           _lock;

    Sos_ptr<Object_set_descr>  _object_set_descr;           // Job nutzt eine Objektemengeklasse
    Level                      _output_level;
    Script                     _script;                     // Job hat ein eigenes Skript
    string                     _process_filename;           // Job ist ein externes Programm
    string                     _process_param;              // Parameter für das Programm
    Run_time                   _run_time;
    int                        _priority;

    Script*                    _script_ptr;
    Script_instance            _script_instance;            // Für use_engine="job"
    bool                       _has_spooler_process;
    Directory_watcher          _directory_watcher;

    State                      _state;
    State_cmd                  _state_cmd;
    CComPtr<spooler_com::Ivariable_set> _params;
    Time                       _next_start_time;
    Period                     _period;                     // Derzeitige oder nächste Period
    Time                       _repeat;                     // spooler_task.repeat

    Prefix_log                 _log;
    CComPtr<Com_job>           _com_job;
    CComPtr<Com_log>           _com_log;
    CComPtr<Com_task>          _com_task;                   // Objekt bleibt, Inhalt wechselt über die Tasks hinweg
    Xc_copy                    _error;
    Sos_ptr<Task>              _task;                       // Es kann nur eine Task geben. Zirkel: _task->_job == this
};

typedef list< Sos_ptr<Job> >    Job_list;

//----------------------------------------------------------------------------------------------Task

struct Task : Sos_self_deleting
{
                                Task                        ( Spooler*, const Sos_ptr<Job>& );
                               ~Task                        ();

    void                        close                       ();

    void                        start                       ();
    void                        end                         ();
    bool                        step                        ();
    void                        on_error_on_success         ();


    bool                        wait_until_terminated       ( double wait_time = latter_day );

    Job*                        job                         ()                              { return _job; }
  
    friend struct               Job;
    friend struct               Object_set;
    friend struct               Com_task;

  protected:
    virtual void                do_close                    ()                              {}
    virtual void                do_start                    () = 0;
    virtual void                do_end                      () = 0;
    virtual bool                do_step                     () = 0;
    virtual void                do_on_success               () = 0;
    virtual void                do_on_error                 () = 0;

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;                        // Zirkel!
    Thread_semaphore           _lock;
    
    double                     _cpu_time;
    int                        _step_count;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _job._period.end() beenden
    bool                       _opened;

    Time                       _running_since;

    CComPtr<spooler_com::Ivariable_set> _params;
    CComVariant                _result;
    Xc_copy                    _error;

    Thread_semaphore           _terminated_events_lock;
    vector<Event*>             _terminated_events;
};

typedef list< Sos_ptr<Task> >   Task_list;

//--------------------------------------------------------------------------------------Script_task

struct Script_task : Task
{
                                Script_task                 ( Spooler* sp, const Sos_ptr<Job>& j ) : Task(sp,j) {}

  //void                        do_start                    ();
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
    void                        do_start                    ();
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

    void                        do_start                    ();
    void                        do_end                      ();
    bool                        do_step                     ();
  //void                        do_on_success               ();
  //void                        do_on_error                 ();
};

//-------------------------------------------------------------------------------------Process_task

struct Process_task : Task
{
                                Process_task                ( Spooler* sp, const Sos_ptr<Job>& j ) : Task(sp,j) {}
        
    void                        do_start                    ();
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
