// $Id$

#ifndef __SPOOLER_JOB_H
#define __SPOOLER_JOB_H

namespace sos {
namespace scheduler {


//typedef int                     Level;
struct                          Task;
struct                          Module_task;

//-----------------------------------------------------------------------------------Level_interval

//struct Level_interval
//{
//                                Level_interval              ()                                      : _low_level(0), _high_level(0) {}
//    explicit                    Level_interval              ( const xml::Element_ptr& e )           { set_dom( e ); }
//
//    void                        set_dom                     ( const xml::Element_ptr& );
//
//    bool                        is_in_interval              ( Level level )                         { return level >= _low_level && level < _high_level; }
//
//    Level                      _low_level;
//    Level                      _high_level;
//};

//---------------------------------------------------------------------------------Object_set_class

//struct Object_set_class : Sos_self_deleting
//{
//                                Object_set_class            ( Spooler* sp, Prefix_log* log )        : _spooler(sp), _module(sp,log) {}
//    explicit                    Object_set_class            ( Spooler* sp, Prefix_log* log, const xml::Element_ptr& e, const Time& xml_mod_time )  : _spooler(sp), _module(sp,log) { set_dom( e, xml_mod_time ); }
//
//    void                        set_dom                     ( const xml::Element_ptr&, const Time& xml_mod_time );
//
//    Spooler*                   _spooler;
//    string                     _name;
//    map<Level,string>          _level_map;
//    
//    Module                     _module;
//    bool                       _object_interface;
//
//  //Time                       _process_timeout;
//};
//
//typedef list< Sos_ptr<Object_set_class> >  Object_set_class_list;

//-----------------------------------------------------------------------------------Spooler_object

//struct Spooler_object
//{
//                                Spooler_object              ( const ptr<IDispatch>& dispatch = NULL ) : _idispatch(dispatch) {}
//
//    Spooler_object&             operator =                  ( const ptr<IDispatch>& dispatch )      { _idispatch = dispatch; return *this; }
//    Level                       level                       ();
//    void                        process                     ( Level output_level );
//    bool                        is_null                     ()                                      { return _idispatch == NULL; }
//
//    ptr<IDispatch>             _idispatch;
//};

//---------------------------------------------------------------------------------Object_set_descr

//struct Object_set_descr : Sos_self_deleting
//{
//                                Object_set_descr            ()                                      {}
//    explicit                    Object_set_descr            ( const xml::Element_ptr& e )           { set_dom( e ); }
//
//    void                        set_dom                     ( const xml::Element_ptr& );
//
//    string                     _class_name;
//    Sos_ptr<Object_set_class>  _class;
//    Level_interval             _level_interval;
//};

//---------------------------------------------------------------------------------------Object_set

//struct Object_set : Sos_self_deleting
//{
//                                Object_set                  ( Spooler*, Module_task*, const Sos_ptr<Object_set_descr>& );
//                               ~Object_set                  ();
//
//    bool                        open                        ();
//    void                        close                       ();
//    Spooler_object              get                         ();
//    bool                        step                        ( Level result_level );
//    void                        set_in_call                 ( const string& name );
//
//  //Task_subsystem*             thread                      () const;
//
//    Fill_zero                  _zero_;
//    Spooler*                   _spooler;
//    Module_task*               _task;
//    Sos_ptr<Object_set_descr>  _object_set_descr;
//    Object_set_class*          _class;
//    ptr<IDispatch>             _idispatch;                  // Zeiger auf ein Object_set des Skripts
//};

//----------------------------------------------------------------------------------------------Job

struct Job : Object,
             Scheduler_object
{
    enum State
    {
        s_not_initialized,
        s_initialized,
        s_loaded,   
      //s_suspended,            // Alle Tasks sind suspended
        s_stopping,             // Wird gestoppt (Zustand, solange noch Tasks laufen, danach s_stopped)
        s_stopped,              // Gestoppt (z.B. wegen Fehler). Keine Task wird gestartet.
        s_read_error,           // Skript kann nicht aus Datei (include) gelesen werden
        s_error,                // Ein Fehler ist aufgetreten (nicht vom Skript), der Job ist nicht mehr aufrufbar.
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
        sc_remove,              // Spooler::remove_job
        sc__max
    };


/*
    struct Delay_after_error
    {
                              //Delay_after_error           ()                                      : _max_errors_then_stop(0) {}

        bool                    empty                       ()                                      { return _map.empty(); }
        void                    set                         ( int error_steps, const Time& delay );
        void                    set_stop                    ( int error_steps )                     { set( error_steps, Time::never ); }
        void                    clear                       ()                                      { _map.clear(); }

        typedef map< int, Time >  Map;
        Map                    _map;

      //int                    _max_errors_then_stop;       // Bei dieser Fehlerzahl soll der Job gestoppt werden (und nicht mehr verzögert)
    };
*/

    struct Task_queue
    {
        typedef list< ptr<Task> >       Queue;
        typedef Queue::iterator         iterator;

        enum Why_remove
        {
            w_task_killed,
            w_task_started
        };

        
                                Task_queue                  ( Job* job )                            : _job(job), _spooler(job->_spooler) {}

      //void                    load_from_db                ();
        void                    clear                       ()                                      { _queue.clear(); }
        int                     size                        () const                                { return _queue.size(); }
        bool                    empty                       () const                                { return _queue.empty(); }
        iterator                begin                       ()                                      { return _queue.begin(); }
        iterator                end                         ()                                      { return _queue.end(); }

        void                    enqueue_task                ( const ptr<Task>& );
        bool                    remove_task                 ( int task_id, Why_remove );
        void                    remove_task_from_db         ( int task_id );
        bool                    has_task_waiting_for_period ();
      //Time                    next_at_start_time          ();

      private:
        Job* const             _job;
        Spooler* const         _spooler;
        Queue                  _queue;
    };


    typedef list< ptr<Task> >                   Task_list;
    typedef list< ptr<Directory_watcher> >      Directory_watcher_list;
    typedef map< int, Time >                    Delay_after_error;
    typedef map< int, Time >                    Delay_order_after_setback;

                                Job                         ( Spooler*, const ptr<Module>& = NULL );
    virtual                    ~Job                         (); 


    // Scheduler_object:
    virtual string              obj_name                    () const                                { return "Job " + _name; }
    virtual IDispatch*          idispatch                   ()                                      { return _com_job; }


    void                    set_dom                         ( const xml::Element_ptr&, const Time& mod_time );
    void                        add_on_exit_commands_element( const xml::Element_ptr& commands_element, const Time& mod_time );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what&, Job_chain* = NULL );
    xml::Element_ptr            calendar_dom_element_or_null( const xml::Document_ptr&, const Time& from, const Time& until, int* const limit );


    void                        initialize                  ();                                     // Wird vor Spooler-Skript gerufen
    void                        load                        ( Transaction* );                       // Wird nach Spooler-Skript gerufen, ruft auch init2()
    void                        activate                    ();                                     // Wird nach reread() gerufen
    void                        init_start_when_directory_changed( Task* = NULL );
    void                        prepare_on_exit_commands    ();
    void                        set_log                     ();
    void                        init_run_time               ();
    void                        set_run_time                ( const xml::Element_ptr& );

    void                    set_name                        ( const string& );
    const string&               name                        () const                                { return _name; }
    State_cmd                   state_cmd                   () const                                { return _state_cmd; }
    State                       state                       () const                                { return _state; }
  //Object_set_descr*           object_set_descr            () const                                { return _object_set_descr; }
  //string                      priority                    () const                                { return _priority; }
  //Task_subsystem*             thread                      () const                                { return _thread; }
    string                      job_state                   ();
    string                      include_path                () const;
    string                      title                       ()                                      { return _title; }
    string                      jobname_as_filename         ();
    string                      profile_section             ();
    void                    set_visible                     ( bool b )                              { _visible = b; }
    bool                        visible                     () const                                { return _visible; }
    bool                        temporary                   () const                                { return _temporary; }
    void                    set_remove                      ( bool );
    void                    set_replacement_job             ( Job* );
    void                        set_delay_after_error       ( int error_steps, const string& delay );
    void                        set_delay_after_error       ( int error_steps, const Time& delay )  { _log->debug9( "delay_after_error["        +as_string(error_steps)+"]="+delay.as_string() ); _delay_after_error[ error_steps ] = delay; }
    void                        set_stop_after_error        ( int error_steps )                     { _log->debug9( "delay_after_error["        +as_string(error_steps)+"]=\"STOP\""           ); _delay_after_error[ error_steps ] = Time::never; }
    void                        clear_delay_after_error     ()                                      { _log->debug9( "clear_delay_after_error()" ); _delay_after_error.clear(); }
    void                        set_delay_order_after_setback( int setbacks, const string& delay );
    void                        set_delay_order_after_setback( int setbacks, const Time& delay )    { _log->debug9( "delay_order_after_setback["+as_string(setbacks   )+"]="+delay.as_string() ); _delay_order_after_setback[setbacks   ] = delay; }
    Time                        get_delay_order_after_setback( int setback_count );
    void                        set_max_order_setbacks      ( int n )                               { _log->debug9( "max_order_setbacks"+as_string(n) ); _max_order_setbacks = n; }
    int                         max_order_setbacks          () const                                { return _max_order_setbacks; }
    bool                        request_order               ( const Time& now, const string& cause );   // Fordert einen Auftrag für die _order_queue an
    void                        withdraw_order_request      ();
    void                        load_tasks_from_db          ( Transaction* );
    xml::Element_ptr            read_history                ( const xml::Document_ptr& doc, int id, int n, const Show_what& show ) { return _history.read_tail( doc, id, n, show ); }

    void                        close                       ();

    ptr<Task>                   start                       ( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, const Time& = 0 );
    void                        enqueue_task                ( Task* );
    void                        start_when_directory_changed( const string& directory_name, const string& filename_pattern );
    void                        clear_when_directory_changed();
    bool                        check_for_changed_directory ( const Time& now );
    void                        update_changed_directories  ( Directory_watcher* );
    string                      trigger_files               ( Task* task = NULL );
    void                        interrupt_script            ();
    void                        select_period               ( const Time& );
    bool                        is_in_period                ( const Time& );
    bool                        queue_filled                ()                                      { return !_task_queue.empty(); }
    
    void                        on_task_finished            ( Task* );                              // Task::finished() ruft das
    void                        check_min_tasks             ( const string& cause );                // Setzt _start_min_tasks
    bool                        above_min_tasks             () const;
    bool                        below_min_tasks             () const;
    bool                        should_start_task_because_of_min_tasks();
    int                         not_ending_tasks_count      () const;

    ptr<Task>                   create_task                 ( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, const Time& = Time::never );
    ptr<Task>                   create_task                 ( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, const Time&, int id );
    ptr<Task>                   get_task_from_queue         ( const Time& now );
    void                        run_task                    ( const ptr<Task>&  );

  //void                        remove_from_task_queue      ( Task*, Log_level );
    void                        remove_running_task         ( Task* );
  //void                        close_task                  ();
    bool                        read_script                 ( Module* );
  //void                        end                         ();
    void                        stop                        ( bool end_all_tasks );
    void                        stop_after_task_error       ( bool end_all_tasks, const string& error_message );   // _ignore_error verhindert stop()
    void                        set_next_start_time         ( const Time& now, bool repeat = false );
    void                        set_next_time               ( const Time& );
    void                        calculate_next_time         ( const Time& now );
    void                        signal_processable_order    ( Order* );

    Time                        next_time                   ()                                      { return _next_time; }
    Time                        next_start_time             ();
    bool                        has_next_start_time         ()                                      { return next_start_time() < Time::never; }
    bool                     is_machine_resumable           () const                                { return _machine_resumable; }
    void                    set_machine_resumable           ( bool b )                              { _machine_resumable = b; }

    bool                        execute_state_cmd           ();
    void                        reread                      ();
    ptr<Task>                   task_to_start               ();
    bool                        do_something                ();
    bool                        should_removed              ();

    void                    set_repeat                      ( double seconds )                      { _log->debug( "repeat=" + as_string(seconds) ),  _repeat = seconds; }
    Time                        repeat                      ()                                      { return _repeat; }

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );
    void                        kill_task                   ( int task_id, bool immediately = false );
    void                        kill_queued_task            ( int task_id );

    string                      state_name                  ()                                      { return state_name( _state ); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    string                      state_cmd_name              ()                                      { return state_cmd_name( _state_cmd ); }
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );

    void                        set_state_text              ( const string& text )                  { _state_text = text, _log->debug9( "state_text = " + text ); }

    void                        set_error_xc                ( const Xc& );
    void                        set_error_xc_only           ( const Xc& );
    void                        set_error                   ( const exception& );
    void                        reset_error                 ()                                      { _error = NULL,  _log->reset_highest_level(); }
    void                        set_job_error               ( const exception& );

    void                        signal                      ( const string& signal_name );
    void                        notify_a_process_is_idle    ();                                     // Vielleicht wird bald ein Prozess frei?
    void                        remove_waiting_job_from_process_list();

    ptr<Com_job>&               com_job                     ()                                      { return _com_job; }
  //void                        signal_object               ( const string& object_set_class_name, const Level& );

    Order_queue*                order_queue                 () const                                { return _order_queue; }
    bool                        order_controlled            () const                                { return _order_queue != NULL; }
    void                    set_order_controlled            ();
  //bool                        is_requesting_order         () const                                { return _is_requesting_order; }
    void                    set_idle_timeout                ( const Time& );

    void                        set_job_chain_priority      ( int pri )                             { if( _job_chain_priority < pri )  _job_chain_priority = pri; }
    static bool                 higher_job_chain_priority   ( const Job* a, const Job* b )          { return a->_job_chain_priority > b->_job_chain_priority; }

    Module*                     module                      ()                                      { return _module; }
    ptr<Module_instance>        create_module_instance      ();
    Module_instance*            get_free_module_instance    ( Task* );
    void                        release_module_instance     ( Module_instance* );

    void                        increment_running_tasks     ()                                      { InterlockedIncrement( &_running_tasks_count ); }
    void                        decrement_running_tasks     ()                                      { InterlockedDecrement( &_running_tasks_count ); }

    void                        count_task                  ()                                      { InterlockedIncrement( &_tasks_count ); }
    void                        count_step                  ()                                      { InterlockedIncrement( &_step_count ); }


    friend struct               Object_set;
    friend struct               Task;
    friend struct               Module_task;
    friend struct               Job_module_task;
    friend struct               Object_set_task;
  //friend struct               Process_task;
    friend struct               Com_job;
    friend struct               Task_subsystem;


    Fill_zero                  _zero_;
    string                     _name;
    bool                       _waiting_for_process;        // Task kann nicht gestartet werden, weil kein Prozess in der Prozessklasse verfügbar ist
    bool                       _waiting_for_process_try_again;  
    string                     _description;                // <description>
    ptr<Job>                   _replacement_job;

  protected:
    friend struct               Job_history;

    string                     _title;                      // <job title="">
    string                     _state_text;                 // spooler_job.state_text = "..."

    long32                     _tasks_count;                // Anzahl gestarteter Tasks seit Spooler-Start
    long32                     _step_count;                 // Anzahl spooler_process() aller Tasks

    State                      _state;
    State_cmd                  _state_cmd;
    bool                       _reread;                     // <script> neu einlesen, also <include> erneut ausführen
    Time                       _delay_until;                // Nach Fehler verzögern
    Time                       _next_start_time;
    Time                       _next_time;                  // Für Task_subsystem::wait(): Um diese Zeit soll Job::do_something() gerufen werden.
    Time                       _next_single_start;
    Time                       _repeat;                     // spooler_task.repeat
    Time                       _task_timeout;               // Frist für einen Schritt einer Task
    Time                       _idle_timeout;               // Frist für den Zustand Task::s_running_waiting_for_order
    bool                       _force_idle_timeout;         // _idle_timeout wirkt beendet auch Tasks, wenn _min_tasks unterschritten wird
  //string                     _priority;                   // "", "-20" bis "+20" oder "idle", "below_normal" etc.
    bool                       _visible;
    bool                       _temporary;                  // Job nach einem Lauf entfernen
    bool                       _remove;                     // Job enfernen sobald möglich. _state == s_stopping, dann s_stopped
    bool                       _start_once;                 // <run_time start_once="">, wird false nach Start
    bool                       _start_once_for_directory;
    bool                       _machine_resumable;          // Test
    bool                       _stop_on_error;              // Nach Task-Fehler Job stoppen (default)
    bool                       _ignore_every_signal;        // Nur Unix: Nach Beendigung mit Signal (kill, Absturz) den Job nicht stoppen 
    stdext::hash_set<int>      _ignore_signals_set;

    bool                       _log_append;                 // Jobprotokoll fortschreiben <job log_append=(yes|no)>

    ptr<Run_time>              _run_time;
    Period                     _period;                     // Derzeitige oder nächste Period
    xml::Document_ptr          _commands_document;          // <commands>...
    Time                       _commands_document_time;
    typedef map<int,xml::Element_ptr>  Exit_code_commands_map;
    Exit_code_commands_map     _exit_code_commands_map;
    xml::Element_ptr           _exit_code_commands_on_signal;
    Delay_after_error          _delay_after_error;
    long32                     _error_steps;                // Zahl aufeinanderfolgender Fehler

    Directory_watcher_list     _directory_watcher_list;
    Time                       _directory_watcher_next_time;
    bool                       _directory_changed;
    string                     _changed_directories;
    Xc_copy                    _error;

    ptr<Com_variable_set>      _default_params;


    ptr<Module>                _module;                     // Job hat ein eigenes Skript
    xml::Element_ptr           _script_element;             // <script> (mit <include>) für <modify_job cmd="reload"/>

    typedef vector< ptr<Module_instance> >  Module_instance_vector;
    Module_instance_vector     _module_instances;
    ptr<Com_job>               _com_job;

    //Sos_ptr<Object_set_descr>  _object_set_descr;           // Job nutzt eine Objektemengeklasse
    //Level                      _output_level;
    
    Task_queue                 _task_queue;                 // Warteschlange der nächsten zu startenden Tasks
    Task_list                  _running_tasks;              // Alle laufenden Tasks (auch die gestarteten, aber wartenden, z.B. s_running_waiting_for_order)
    long32                     _running_tasks_count;        // Anzahl der Tasks, die tatsächlich laufen (und nicht gerade warten)
    int                        _min_tasks;                  // Min. Anzahl Tasks, die der Scheduler stets laufen lassen soll
    bool                       _start_min_tasks;            // Starte Tasks solange _running_tasks.count() < _min_tasks
    int                        _max_tasks;                  // Max. Anzahl gleichzeitig laufender Tasks. _running_tasks.size() <= _max_tasks!

    Job_history                _history;

    ptr<Order_queue>           _order_queue;
    int                        _job_chain_priority;         // Maximum der Prioritäten aller Jobkettenknoten mit diesem Job. 

    Delay_order_after_setback  _delay_order_after_setback;
    int                        _max_order_setbacks;

  //bool                       _is_requesting_order;

    typedef list< pair<string,string> > Start_when_directory_changed_list;  
    Start_when_directory_changed_list  _start_when_directory_changed_list;      // Für <start_when_directory_changed>
};

//--------------------------------------------------------------------------------------Internal_job

struct Internal_job : Job
{
                                Internal_job                ( const string& name, const ptr<Module>& );
};

//------------------------------------------------------------------------------------------Job_list

typedef list< ptr<Job> >    Job_list;

#define FOR_EACH_JOB( ITERATOR )  FOR_EACH( Job_list, _spooler->job_subsystem()->_job_list, ITERATOR )

//---------------------------------------------------------------------------Job_subsystem_interface

struct Job_subsystem_interface : Subsystem
{
                                Job_subsystem_interface     ( Scheduler* scheduler, Type_code t )   : Subsystem( scheduler, t ) {}

    virtual void                close_jobs                  ()                                      = 0;
    virtual void                initialize_job              ( Job* )                                = 0;
    virtual void                load_job                    ( Transaction*, Job* )                  = 0;
    virtual void                activate_job                ( Job* )                                = 0;
    virtual void                load_jobs_from_xml          ( const xml::Element_ptr&, const Time& xml_mod_time, bool activate = false ) = 0;
    virtual void                load_job_from_xml           ( const xml::Element_ptr&, const Time& xml_mod_time, bool activate = false ) = 0;
    virtual void                add_job                     ( const ptr<Job>&, bool activate )      = 0;
    virtual void                remove_job                  ( Job* )                                = 0;
    virtual int                 remove_temporary_jobs       ( Job* which_job = NULL )               = 0;
    virtual Job*                get_job                     ( const string& job_name, bool can_be_not_initialized = false ) = 0;
    virtual Job*                get_job_or_null             ( const string& job_name )              = 0;
    virtual bool                has_any_order               ()                                      = 0;
    virtual bool                is_any_task_queued          ()                                      = 0;
    virtual xml::Element_ptr    jobs_dom_element            ( const xml::Document_ptr&, const Show_what& ) = 0;


    Job_list                   _job_list;                   // Das ist offen zugänglich für FOR_EACH_JOB
};

ptr<Job_subsystem_interface>    new_job_subsystem           ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
