// $Id: spooler_task.h,v 1.3 2001/01/29 10:45:01 jz Exp $

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
                                Spooler_object              ( const CComPtr<IDispatch>& dispatch = NULL ) : _dispatch(dispatch) {}

    Spooler_object&             operator =                  ( const CComPtr<IDispatch>& dispatch ) { _dispatch = dispatch; return *this; }
    Level                       level                       ();
    void                        process                     ( Level output_level );
    bool                        is_null                     ()                              { return _dispatch == NULL; }

    CComPtr<IDispatch>         _dispatch;
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

//------------------------------------------------------------------------------------Job_interface
/*
struct Job_interface
{
    void                        spooler_init                () = 0;
    void                        spooler_open                () = 0;
    void                        spooler_close               () = 0;
    void                        spooler_process             () = 0;
};
*/
//---------------------------------------------------------------------------------------Object_set

struct Object_set : Sos_self_deleting
{
                                Object_set                  ( Spooler*, Task*, const Sos_ptr<Object_set_descr>& );
                               ~Object_set                  ();

    void                        open                        ();
    void                        close                       ();
    Spooler_object              get                         ();
    bool                        step                        ( Level result_level );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Task*                      _task;
    Sos_ptr<Object_set_descr>  _object_set_descr;
    Object_set_class*          _class;
  //Script_instance            _script_instance;
    CComPtr<IDispatch>         _dispatch;                   // Zeiger auf ein Object_set des Skripts
};

//----------------------------------------------------------------------------------------------Job

struct Job : Sos_self_deleting
{
                                Job                         ( Spooler* );
                               ~Job                         (); 

    void                        set_xml                     ( const xml::Element_ptr& );

    void                        set_current_task            ( Task* task )                      { _com_current_task->_task = task; }

    void                        start                       ( const CComPtr<spooler_com::Ivariable_set>& params );
    void                        start_when_directory_changed( const string& directory_name );
  //void                        stop_all_tasks              ()                                  { if( _task )  _task->stop(); }

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _name;
    Sos_ptr<Object_set_descr>  _object_set_descr;
    Level                      _output_level;
    Script                     _script;
    Script_instance            _script_instance;            // Für use_engine="job"
    Run_time                   _run_time;
    bool                       _stop_at_end_of_duration;
    bool                       _continual;
  //bool                       _stop_after_error;
  //bool                       _rerun;
  //bool                       _start_after_spooler;
    int                        _priority;
    CComPtr<Com_job>           _com_job;
    Task*                      _task;                       // Es kann nur eine Task geben
    CComPtr<Com_task>          _com_current_task;           // gerade laufende Task 
};

typedef list< Sos_ptr<Job> >    Job_list;

//----------------------------------------------------------------------------------------------Task

struct Task : Sos_self_deleting
{
    enum State
    {
        s_none,
        s_stopped,              // Gestoppt (z.B. wegen Fehler)
        s_loaded,               // Skript geladen (mit spooler_init), aber nicht gestartet (spooler_open)
        s_pending,              // Warten auf Start
        s_running,              // Läuft
        s_suspended,            // Angehalten
        s_ending,               // end()
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


                                Task                        ( Spooler*, const Sos_ptr<Job>& );
                               ~Task                        ();

    void                        close                       ();

    void                        start_thread                ();
    int                         run_thread                  ();
    void                        wait_until_stopped          ();
    void                        wake                        ();

    bool                        start                       ();
    void                        end                         ();
    void                        stop                        ();
    bool                        step                        ();

    void                        on_error_on_success         ();

    bool                        do_something                ();

  //void                        wake                        ()                          { _wake = true; }   // Ein neues Objekt ist da, vielleicht.

    void                        set_next_start_time         ();

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );

    string                      state_name                  ()                          { return state_name( _state ); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    string                      state_cmd_name              ()                          { return state_cmd_name( _state_cmd ); }
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );


    void                        start_error                 ( const Xc& );
    void                        end_error                   ( const Xc& );
    void                        step_error                  ( const Xc& );
    void                        error                       ( const Xc& );
    void                        error                       ( const exception& );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;
    CComPtr<spooler_com::Ivariable_set> _params;
    Script_instance            _script_instance;            // Für use_engine="task"
    Script_instance*           _script_instance_ptr;        // &_script_instance oder &_job->_script_instance
    bool                       _use_task_engine;            // use_engine="task"
    CComPtr<IDispatch>         _dispatch;
    
    Directory_watcher          _directory_watcher;
    int                        _priority;
    double                     _cpu_time;
    int                        _step_count;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _next_end_time beenden
    Mutex<State>               _state;
    Mutex<State_cmd>           _state_cmd;
    bool                       _opened;
    bool                       _has_spooler_process;        // Skript hat spooler_process()
    Xc_copy                    _error;

    Time                       _running_since;

    Sos_ptr<Object_set>        _object_set;
    Period                     _period;                     // Derzeitige oder nächste Period
    Time                       _next_start_time;
    Prefix_log                 _log;
    CComPtr<Com_log>           _com_log;
    CComPtr<Com_object_set>    _com_object_set;
    CComPtr<Com_task>          _com_task;

    // Threading
    Handle                     _thread;
    ulong                      _thread_id;
    Wait_handles               _wait_handles; 
    Handle                     _wake_event;                 // Task wecken
    Handle                     _task_event;                 // Änderung in der Task, Spooler wecken
};

typedef list< Sos_ptr<Task> >   Task_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
