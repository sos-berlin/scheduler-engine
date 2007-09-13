// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SPOOLER_JOB_H
#define __SPOOLER_JOB_H

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Task;
struct Module_task;
struct Job_folder;

//------------------------------------------------------------------------------Combined_job_nodes

struct Combined_job_nodes : Object
{
                                Combined_job_nodes          ( Job* );
                               ~Combined_job_nodes          ();

    void                        close                       ();
    bool                        is_empty                    () const                                { return _job_node_set.empty(); }
    Order_queue*                any_order_queue             () const;
    bool                        request_order               ( const Time& now, const string& cause );
    void                        withdraw_order_requests     ();
    Time                        next_time                   ();
    Order*                      fetch_and_occupy_order      ( const Time& now, const string& cause, Task* occupying_task );
    void                        connect_with_order_queues   ();
    void                        connect_job_node            ( job_chain::Job_node* );
    void                        disconnect_job_node         ( job_chain::Job_node* );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what&, Job_chain* );
    Spooler*                    spooler                     () const                                { return _spooler; }

  private:
    Fill_zero                  _zero_;
    Job*                       _job;
    Spooler*                   _spooler;
    typedef stdext::hash_set< job_chain::Job_node* >  Job_node_set;
    Job_node_set               _job_node_set;
};

//----------------------------------------------------------------------------------------------Job

struct Job : file_based< Job, Job_folder, Job_subsystem_interface >,
           //Pendant,
             Object
{
    enum State
    {
        s_not_initialized,
        s_initialized,
        s_loaded,   
        s_stopping,             // Wird gestoppt (Zustand, solange noch Tasks laufen, danach s_stopped)
        s_stopped,              // Gestoppt (z.B. wegen Fehler). Keine Task wird gestartet.
      //s_read_error,           // Skript kann nicht aus Datei (include) gelesen werden
        s_error,                // Ein Fehler ist aufgetreten (nicht vom Skript), der Job ist nicht mehr aufrufbar.
      //s_incomplete,           // Eine Resource fehlt, Lock oder Process_class
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
        sc_remove,              // Spooler::remove_job
        sc__max
    };


    struct Task_queue : Object
    {
        typedef list< ptr<Task> >       Queue;
        typedef Queue::iterator         iterator;

        enum Why_remove
        {
            w_task_killed,
            w_task_started
        };

        
                                Task_queue                  ( Job* job )                            : _job(job), _spooler(job->_spooler) {}
        
        void                    clear                       ()                                      { _queue.clear(); }
        int                     size                        () const                                { return _queue.size(); }
        bool                    empty                       () const                                { return _queue.empty(); }
        iterator                begin                       ()                                      { return _queue.begin(); }
        iterator                end                         ()                                      { return _queue.end(); }

        void                    enqueue_task                ( const ptr<Task>& );
        bool                    remove_task                 ( int task_id, Why_remove );
        void                    remove_task_from_db         ( int task_id );
        bool                    has_task_waiting_for_period ();
        void                    move_to_new_job             ( Job* );
        void                    append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

      private:
        Job*                   _job;
        Spooler* const         _spooler;
        Queue                  _queue;
    };


    typedef list< ptr<Task> >                   Task_list;
    typedef list< ptr<Directory_watcher> >      Directory_watcher_list;
    typedef map< int, Time >                    Delay_after_error;
    typedef map< int, Time >                    Delay_order_after_setback;

                                Job                         ( Scheduler*, const string& name = "", const ptr<Module>& = NULL );
    virtual                    ~Job                         (); 

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Object::Release(); }


    // Scheduler_object:
    virtual string              obj_name                    () const                                { return "Job " + name(); }
    virtual IDispatch*          idispatch                   ()                                      { return _com_job; }
    virtual void                write_element_attributes    ( const xml::Element_ptr& element ) const { element.setAttribute( "job", path() ); }


    // File_based:
    bool                        on_initialize               (); 
    bool                        on_load                     (); 
  //void                        on_load                     ( Transaction* );
    bool                        on_activate                 ();
    bool                        can_be_removed_now          ();


    // Pendant:
    bool                        on_dependant_incarnated     ( File_based* );
    bool                        on_dependant_to_be_removed  ( File_based* );
  //void                        on_dependant_removed        ( File_based* );

    Job_folder*                 job_folder                  () const                                { return typed_folder(); }


    void                    set_dom                         ( const xml::Element_ptr& );
    void                        add_on_exit_commands_element( const xml::Element_ptr& commands_element );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what&, Job_chain* = NULL );
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );


    void                        init_start_when_directory_changed( Task* = NULL );
    void                        prepare_on_exit_commands    ();
    void                        set_log                     ();
    void                        init_run_time               ();
    void                        set_run_time                ( const xml::Element_ptr& );

    State_cmd                   state_cmd                   () const                                { return _state_cmd; }
    State                       state                       () const                                { return _state; }
    string                      job_state                   ();
    string                      include_path                () const;
    string                      title                       ()                                      { return _title; }
    string                      jobname_as_filename         ();
    string                      profile_section             ();
    void                    set_visible                     ( bool b )                              { _visible = b; }
    bool                        visible                     () const                                { return _visible; }
    bool                        temporary                   () const                                { return _temporary; }

  //bool                        remove                      ();
    bool                        prepare_to_remove           ();

    bool                        prepare_to_replace          ();
    bool                        can_be_replaced_now         ();
    Job*                        replace_now                 ();
  //void                    set_replacement_job             ( Job* );

    void                        set_delay_after_error       ( int error_steps, const string& delay );
    void                        set_delay_after_error       ( int error_steps, const Time& delay )  { _log->debug9( "delay_after_error["        +as_string(error_steps)+"]="+delay.as_string() ); _delay_after_error[ error_steps ] = delay; }
    void                        set_stop_after_error        ( int error_steps )                     { _log->debug9( "delay_after_error["        +as_string(error_steps)+"]=\"STOP\""           ); _delay_after_error[ error_steps ] = Time::never; }
    void                        clear_delay_after_error     ()                                      { _log->debug9( "clear_delay_after_error()" ); _delay_after_error.clear(); }
    void                        set_delay_order_after_setback( int setbacks, const string& delay );
    void                        set_delay_order_after_setback( int setbacks, const Time& delay )    { _log->debug9( "delay_order_after_setback["+as_string(setbacks   )+"]="+delay.as_string() ); _delay_order_after_setback[setbacks   ] = delay; }
    Time                        get_delay_order_after_setback( int setback_count );
    void                        set_max_order_setbacks      ( int n )                               { _log->debug9( "max_order_setbacks="+as_string(n) ); _max_order_setbacks = n; }
    int                         max_order_setbacks          () const                                { return _max_order_setbacks; }
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
    bool                        queue_filled                ()                                      { return !_task_queue->empty(); }
    
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

    void                        remove_running_task         ( Task* );
    void                        stop                        ( bool end_all_tasks );
    void                        stop_after_task_error       ( const string& error_message );   // _ignore_error verhindert stop()
    bool                        stops_on_task_error         ()                                      { return _stop_on_error; }
    void                        set_next_start_time         ( const Time& now, bool repeat = false );
    void                        set_next_time               ( const Time& );
    void                        calculate_next_time         ( const Time& now );
    void                        signal_earlier_order        ( Order* );

    Time                        next_time                   ()                                      { return _next_time; }
    Time                        next_start_time             ();
    bool                        has_next_start_time         ()                                      { return next_start_time() < Time::never; }
    bool                     is_machine_resumable           () const                                { return _machine_resumable; }
    void                    set_machine_resumable           ( bool b )                              { _machine_resumable = b; }

    lock::Requestor*            lock_requestor_or_null      () const                                { return _lock_requestor; }
  //void                        on_removing_lock            ( lock::Lock* );

    bool                        execute_state_cmd           ();
    ptr<Task>                   task_to_start               ();
    bool                        do_something                ();

    void                    set_repeat                      ( double seconds )                      { _log->debug( "repeat=" + as_string(seconds) ),  _repeat = seconds; }
    Time                        repeat                      ()                                      { return _repeat; }

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );
    void                        end_tasks                   ( const string& task_warning );
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
    void                        on_process_class_active     ( Process_class* );

    ptr<Com_job>&               com_job                     ()                                      { return _com_job; }

  //Order_queue*                order_queue                 () const                                { return _order_queue; }
    Order_queue*                any_order_queue             () const;
    Combined_job_nodes*      combined_order_queues          () const                                { return _combined_job_nodes; }
    bool                        connect_job_node            ( job_chain::Job_node* );
    void                        disconnect_job_node         ( job_chain::Job_node* );
    bool                        is_in_job_chain             () const                                { return _combined_job_nodes && !_combined_job_nodes->is_empty(); }
    bool                     is_order_controlled            () const                                { return _is_order_controlled; }
    void                    set_order_controlled            ();
    void                    set_idle_timeout                ( const Time& );
    bool                        request_order               ( const Time& now, const string& cause );   // Fordert einen Auftrag für die _order_queue an
    void                        withdraw_order_request      ();
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
    friend struct               Com_job;
    friend struct               Task_subsystem;


    Fill_zero                  _zero_;
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

    ptr<Task_queue>            _task_queue;                 // Warteschlange der nächsten zu startenden Tasks
    Task_list                  _running_tasks;              // Alle laufenden Tasks (auch die gestarteten, aber wartenden, z.B. s_running_waiting_for_order)
    long32                     _running_tasks_count;        // Anzahl der Tasks, die tatsächlich laufen (und nicht gerade warten)
    int                        _min_tasks;                  // Min. Anzahl Tasks, die der Scheduler stets laufen lassen soll
    bool                       _start_min_tasks;            // Starte Tasks solange _running_tasks.count() < _min_tasks
    int                        _max_tasks;                  // Max. Anzahl gleichzeitig laufender Tasks. _running_tasks.size() <= _max_tasks!

    Job_history                _history;

  //ptr<Order_queue>           _order_queue;
    bool                       _is_order_controlled;

    ptr<Combined_job_nodes> _combined_job_nodes;

    int                        _job_chain_priority;         // Maximum der Prioritäten aller Jobkettenknoten mit diesem Job. 

    Delay_order_after_setback  _delay_order_after_setback;
    int                        _max_order_setbacks;

    typedef list< pair<string,string> > Start_when_directory_changed_list;  
    Start_when_directory_changed_list  _start_when_directory_changed_list;      // Für <start_when_directory_changed>

    ptr<lock::Requestor>       _lock_requestor;
};

//-------------------------------------------------------------------------------------Internal_job

struct Internal_job : Job
{
                                Internal_job                ( Scheduler*, const string& name, const ptr<Module>& );
};

//------------------------------------------------------------------------------------------Job_map

#define FOR_EACH_JOB( JOB )  FOR_EACH_FILE_BASED( Job, JOB )

//---------------------------------------------------------------------------------------Job_folder

struct Job_folder : typed_folder< Job >
{
                                Job_folder                  ( Folder* );


    void                        set_dom                     ( const xml::Element_ptr&, bool activate = false );
    void                        load_job_from_xml           ( const xml::Element_ptr&, bool activate = false );
    void                        add_or_replace_job_from_xml ( const string& name, const xml::Element_ptr&, bool activate = false );
    void                        add_job                     ( const ptr<Job>&, bool activate );
    Job*                        job                         ( const string& name )                  { return file_based( name ); }
    Job*                        job_or_null                 ( const string& name )                  { return file_based_or_null( name ); }
    virtual xml::Element_ptr    jobs_dom_element            ( const xml::Document_ptr&, const Show_what& );
};

//---------------------------------------------------------------------------Job_subsystem_interface

struct Job_subsystem_interface: Object, 
                                file_based_subsystem< Job >
{
                                Job_subsystem_interface     ( Scheduler*, Type_code );

    virtual ptr<Job_folder>     new_job_folder              ( Folder* )                             = 0;
    virtual int                 remove_temporary_jobs       ()                                      = 0;
    virtual bool                is_any_task_queued          ()                                      = 0;
    virtual void                append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* ) = 0;
    Job*                        job                         ( const string& job_path )              { return file_based( job_path ); } 
    Job*                        job_or_null                 ( const string& job_path )              { return file_based_or_null( job_path ); }
};

ptr<Job_subsystem_interface>    new_job_subsystem           ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
