// $Id: spooler_job.h 14676 2011-06-25 20:58:20Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SPOOLER_JOB_H
#define __SPOOLER_JOB_H

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Task;
struct Module_task;
struct Job_folder;
struct Job_schedule_use;
struct Combined_job_nodes;
struct Standard_job;

//----------------------------------------------------------------------------------------------Job

struct Job : file_based< Job, Job_folder, Job_subsystem >,
             javabridge::has_proxy<Job>,
             Process_class_requestor,
             Object
{
    enum State
    {
        // Mögliche Verbesserung: Alle Zustände, die auch File_based kennt, hier streichen. 
        // Stattdessen s_inactive, ein Zustand, der sich in die File_based-Zustände aufspaltet
        // Die übrigen Zustände gelten für s_active.
        // s_stopped durch _is_stopped und s_pending ersetzen. Auch mit anderen Zuständen kombinierbar.
        // s_stopping streichen? stop(false) lässt Tasks weiterlaufen. Ist das klug? Sollten ein gestoppte Job nicht immer die Tasks beenden?
        // s_running streichen? Besagt nur, dass eine Task läuft, während bei s_pending keine Task läuft. s_pending, s_running -> s_ready?

        s_not_initialized,
        s_initialized,
        s_loaded,   
        s_stopping,             // Wird gestoppt (Zustand, solange noch Tasks laufen, danach s_stopped). Sollte durch _is_stopped ersetzt werden!
        s_stopped,              // Gestoppt (z.B. wegen Fehler). Keine Task wird gestartet.
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
        sc_wake_when_in_period,
        sc_end,                 // s_running                 -> s_pending
        sc_suspend,             // s_running                 -> s_suspended
        sc_continue,            // s_suspended               -> s_running
        sc_reread,              // Null-Operation, Jira JS-208
        sc_remove,              // Spooler::remove_job
        sc_enable,              
        sc_disable,              
        sc__max
    };

    static const bool           force_start_default;


                                Job                         (Scheduler*);

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Object::Release(); }

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Job>::java_sister(); }
    const JobJ&                 typed_java_sister           () const                                { return _typed_java_sister; }

    // Abstract_scheduler_object:
    string                      obj_name                    () const                                { return "Job " + path().without_slash(); }
    void                        write_element_attributes    ( const xml::Element_ptr& element ) const { element.setAttribute( "job", path().with_slash() ); }


    Job_folder*                 job_folder                  () const                                { return typed_folder(); }
    virtual bool                waiting_for_process         () const                                = 0;

    bool                        is_visible_in_xml_folder    ( const Show_what& ) const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& show_what )  { return dom_element( document, show_what, (Job_chain*)NULL ); }
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what&, Job_chain*) = 0;
    virtual xml::Element_ptr    why_dom_element             ( const xml::Document_ptr& )            = 0;
    virtual vector<string>      unavailable_lock_path_strings() const                               = 0;
    virtual void                append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* ) = 0;
    virtual string              description                 () const                                = 0;
    virtual void                set_schedule_dom            (const xml::Element_ptr&)               = 0;

    virtual void                on_schedule_loaded          ()                                      = 0;
    virtual void                on_schedule_modified        ()                                      = 0;
    virtual bool                on_schedule_to_be_removed   ()                                      = 0;

    virtual State               state                       () const                                = 0;
    virtual bool                is_permanently_stopped      () const                                = 0;
    virtual string              title                       ()                                      = 0;
    string                      profile_section             ();

    virtual void                on_prepare_to_remove        ()                                      = 0;
    virtual bool                on_remove_now               ()                                      = 0;
    virtual zschimmer::Xc       remove_error                ()                                      = 0;

    virtual int                 max_order_setbacks          () const                                = 0;
    virtual xml::Element_ptr    read_history                ( const xml::Document_ptr& doc, int id, int n, const Show_what& show ) = 0;

    virtual void                close                       ()                                      = 0;

    virtual bool                queue_filled                ()                                      = 0;
    ptr<Task>                   start_task                  (Com_variable_set* params, const string& task_name = "", const Time& = Time(0) );
    void                        start_task                  (Com_variable_set* params, Com_variable_set* environment);
    ptr<Task>                   start_task                  (Com_variable_set* params, Com_variable_set* environment, const Time& at, bool force, const string& task_name, const string& web_service_name) { return start_task_(params, environment, at, force, task_name, web_service_name); }
    virtual ptr<Task>           start_task_                 (Com_variable_set* params, Com_variable_set* environment, const Time& at, bool force, const string& task_name, const string& web_service_name) = 0;
    virtual void                enqueue_taskPersistentState (const TaskPersistentStateJ&)           = 0;
    virtual bool                try_to_end_task             (Process_class_requestor*, Process_class*) = 0;
    
    virtual void                remove_running_task         ( Task* )                               = 0;
    virtual void                stop                        ( bool end_all_tasks )                  = 0;
    virtual void                stop_simply                 ( bool end_all_tasks )                  = 0;

    void                        signal_earlier_order        (Order*);
    virtual void                signal_earlier_order        ( const Time& next_time, const string& order_name, const string& function ) = 0;

    virtual int64               next_start_time_millis      () const                                = 0;
    virtual jlong               next_possible_start_millis  () const                                = 0;

    virtual void                set_state_cmd               ( State_cmd )                           = 0;
    virtual void                set_state_cmd               (const string&)                         = 0;
    virtual void                kill_task                   (int task_id, bool immediately = false, const Duration& timeout = Duration(0)) = 0;

    string                      state_name                  ()                                      { return state_name(state()); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );

    virtual void                set_state_text              ( const string& text )                  = 0;
    virtual string              state_text                  () const                                = 0;

    virtual void                notify_a_process_is_available()                                     = 0;

    virtual ptr<Com_job>&       com_job                     ()                                      = 0;

    virtual bool                connect_job_node            ( job_chain::Job_node* )                = 0;
    virtual void                disconnect_job_node         ( job_chain::Job_node* )                = 0;
    virtual bool                is_in_job_chain             () const                                = 0;
    virtual void            set_order_controlled            ()                                      = 0;

    virtual void            set_idle_timeout                ( const Duration& )                     = 0;
    void                        set_job_chain_priority      ( int pri )                             { if( _job_chain_priority < pri )  _job_chain_priority = pri; }
    int                         job_chain_priority          () const                                { return _job_chain_priority; }
    static bool                 higher_job_chain_priority   ( const Job* a, const Job* b )          { return a->job_chain_priority() > b->job_chain_priority(); }
    virtual void                on_order_possibly_available ()                                      = 0;

    virtual bool on_monitors_loaded() = 0;
    virtual bool on_monitor_to_be_removed(Monitor*) = 0;;

    virtual string script_text() const = 0;
    virtual const Absolute_path& default_process_class_path() const = 0;
    virtual bool is_in_period(const Time& = Time::now()) = 0;
    virtual bool max_tasks_reached() const = 0;
    virtual int max_tasks() const = 0;
    virtual int running_tasks_count() const = 0;
    virtual bool is_task_ready_for_order(Process_class*) = 0;
    virtual ArrayListJ java_tasks() const = 0;

  private:
    Fill_zero                  _zero_;
    const JobJ                 _typed_java_sister;
    int                        _job_chain_priority;         // Maximum der Prioritäten aller Jobkettenknoten mit diesem Job. 
};

//-------------------------------------------------------------------------------------Standard_job

namespace job {
    struct State_cmd_call;
    struct Period_begin_call;
    struct Period_end_call;
    struct Start_queued_task_call;
    struct Calculated_next_time_do_something_call;
    struct Start_when_directory_changed_call;
    struct Order_timed_call;
    struct Order_possibly_available_call;
    struct Process_available_call;
    struct Below_min_tasks_call;
    struct Below_max_tasks_call;
    struct Locks_available_call;
    struct Remove_temporary_job_call;


    struct Task_closed_call : object_call<Standard_job, Task_closed_call> {
        Task* const _task;
        Task_closed_call(Task*);
    };
}

struct Standard_job : Job 
{
    struct Task_queue : Object
    {
        typedef list< ptr<Task> >       Queue;
        typedef Queue::iterator         iterator;

        enum Why_remove
        {
            w_task_killed,
            w_task_started
        };

        
                                Task_queue                  ( Standard_job* job )                   : _job(job), _spooler(job->_spooler) {}
        
        void                    clear                       ()                                      { _queue.clear(); }
        int                     size                        () const                                { return int_cast(_queue.size()); }
        bool                    empty                       () const                                { return _queue.empty(); }
        iterator                begin                       ()                                      { return _queue.begin(); }
        iterator                end                         ()                                      { return _queue.end(); }

        void                    enqueue_task                ( const ptr<Task>& );
        bool                    remove_task                 ( int task_id, Why_remove );
        void                    remove_task_from_db         ( int task_id );
        void                    move_to_new_job             ( Standard_job* );
        Time                    next_start_time             ();
        void                    append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );
        xml::Element_ptr        why_dom_element             (const xml::Document_ptr&, const Time& now, bool in_period);

      private:
        Job*                   _job;
        Spooler* const         _spooler;
      public:
        Queue                  _queue;
    };


    typedef stdext::hash_set< ptr<Task> >       Task_set;
    typedef list< ptr<Directory_watcher> >      Directory_watcher_list;
    typedef map< int, Duration >                Delay_after_error;
    typedef map< int, Duration >                Delay_order_after_setback;


                                Standard_job                ( Scheduler*, const string& name = "", const ptr<Module>& = NULL );
    virtual                    ~Standard_job                (); 


    // File_based:
    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();
    list<Requisite_path>        missing_requisites          ();
    bool                        can_be_removed_now          ();


    // Dependant:
    bool                        on_requisite_loaded         ( File_based* );
    bool                        on_requisite_to_be_removed  ( File_based* );
  //void                        on_requisite_removed        ( File_based* );

    bool                        waiting_for_process         () const                                { return _waiting_for_process; }

 //   Job*                        on_replace_now              ();
    void                        set_dom                     ( const xml::Element_ptr& );

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what&, Job_chain*  );
    xml::Element_ptr            why_dom_element             ( const xml::Document_ptr& );
    vector<string>              unavailable_lock_path_strings() const;
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );
    string                      description                 () const                                { return _description; }

    void                    set_schedule_dom                (const xml::Element_ptr&);
    Schedule_use*               schedule_use                () const;
    void                        on_schedule_loaded          ();
    void                        on_schedule_modified        ();
    bool                        on_schedule_to_be_removed   ();
  //void                        on_schedule_removed         ();

    bool on_monitors_loaded();
    bool on_monitor_to_be_removed(Monitor*);

    State                       state                       () const                                { return _state; }
    bool                        is_permanently_stopped      () const                                { return _is_permanently_stopped; }
    string                      job_state                   ();
    string                      title                       ()                                      { return _title; }

    void                        on_prepare_to_remove        ();
    bool                        on_remove_now               ();
    zschimmer::Xc               remove_error                ();

  //void                        prepare_to_replace          ();
  //bool                        can_be_replaced_now         ();
  //Job*                        on_replace_now              ();

    void                        set_delay_after_error       ( int error_steps, const string& delay );
    void                        set_delay_after_error       ( int error_steps, const Duration& delay )  { _log->debug9( "delay_after_error["        +as_string(error_steps)+"]="+delay.as_string() ); _delay_after_error[ error_steps ] = delay; }
    void                        set_stop_after_error        ( int error_steps )                     { _log->debug9( "delay_after_error["        +as_string(error_steps)+"]=\"STOP\""           ); _delay_after_error[ error_steps ] = Duration::eternal; }
    void                        clear_delay_after_error     ()                                      { _log->debug9( "clear_delay_after_error()" ); _delay_after_error.clear(); }
    void                        set_delay_order_after_setback( int setbacks, const string& delay );
    void                        set_delay_order_after_setback( int setbacks, const Duration& delay ){ _log->debug9( "delay_order_after_setback["+as_string(setbacks   )+"]="+delay.as_string() ); _delay_order_after_setback[setbacks   ] = delay; }
    Duration                    get_delay_order_after_setback( int setback_count );
    void                        set_max_order_setbacks      ( int n )                               { _log->debug9( "max_order_setbacks="+as_string(n) ); _max_order_setbacks = n; }
    int                         max_order_setbacks          () const                                { return _max_order_setbacks; }
    xml::Element_ptr            read_history                ( const xml::Document_ptr& doc, int id, int n, const Show_what& show ) { return _history.read_tail( doc, id, n, show ); }

    void                        close                       ();

    ptr<Task>                   start_task_                 (Com_variable_set* params, Com_variable_set* environment, const Time& at, bool force, const string& task_name, const string& web_service_name);
    void                        enqueue_task                ( Task* );
    void                        enqueue_taskPersistentState (const TaskPersistentStateJ&);
    void                        start_when_directory_changed( const string& directory_name, const string& filename_pattern );
    void                        clear_when_directory_changed();
    bool                        queue_filled                ()                                      { return !_task_queue->empty(); }
    
    ptr<Task>                   create_task                 (Com_variable_set* params, const string& task_name = "", bool force = force_start_default, const Time& = Time::never );
    ptr<Task>                   create_task                 (Com_variable_set* params, const string& task_name, bool force, const Time&, int id );

    void                        remove_running_task         ( Task* );
    void                        stop                        ( bool end_all_tasks );
    void                        stop_simply                 ( bool end_all_tasks );

    void                        on_call                     (const job::State_cmd_call&);
    void                        on_call                     (const job::Period_begin_call&);
    void                        on_call                     (const job::Period_end_call&);
    void                        on_call                     (const job::Start_queued_task_call&);
    void                        on_call                     (const job::Calculated_next_time_do_something_call&);
    void                        on_call                     (const job::Task_closed_call&);
    void                        on_call                     (const job::Start_when_directory_changed_call&);
    void                        on_call                     (const job::Order_timed_call&);
    void                        on_call                     (const job::Order_possibly_available_call&);
    void                        on_call                     (const job::Process_available_call&);
    void                        on_call                     (const job::Below_min_tasks_call&);
    void                        on_call                     (const job::Below_max_tasks_call&);
    void                        on_call                     (const job::Locks_available_call&);
    void                        on_call                     (const job::Remove_temporary_job_call&);

    bool                        is_in_period                (const Time&);
    void                        signal_earlier_order        ( const Time& next_time, const string& order_name, const string& function );

    int64                       next_start_time_millis      () const                                { return next_start_time().millis(); }
    jlong                       next_possible_start_millis  () const;

    bool                     is_machine_resumable           () const                                { return _machine_resumable; }
    void                    set_machine_resumable           ( bool b )                              { _machine_resumable = b; }

    void                    set_repeat                      (const Duration& d)                     { _log->debug( "repeat=" + d.as_string() ),  _repeat = d; }
    Duration                    repeat                      ()                                      { return _repeat; }

    void                        set_state_cmd               ( State_cmd );
    void                        set_state_cmd               (const string&);
    void                        kill_task                   (int task_id, bool immediately = false, const Duration& timeout = Duration(0));

    void                        set_state_text              ( const string& text )                  { _state_text = text, _log->debug9( "state_text = " + text ); }
    string                      state_text                  () const                                { return _state_text; }

    void                        notify_a_process_is_available();
    void                        remove_waiting_job_from_process_list();
    void                        on_locks_available          ();

    ptr<Com_job>&               com_job                     ()                                      { return _com_job; }

    Order_queue*                any_order_queue             () const;
    bool                        connect_job_node            ( job_chain::Job_node* );
    void                        disconnect_job_node         ( job_chain::Job_node* );
    bool                        is_in_job_chain             () const;
    bool                     is_order_controlled            () const                                { return _is_order_controlled; }    // Für shell-Jobs nicht mehr relevant. Nur für spooler_process()
    void                    set_order_controlled            ();

    void                    set_idle_timeout                ( const Duration& );
    void                        on_order_possibly_available ();

    Module*                     module                      ()                                      { return _module; }

    // public nur für Task:
    Duration                    get_step_duration_or_percentage( const string& value, const Duration& deflt );
    void                        init_start_when_directory_changed( Task* = NULL );
    Time                        next_order_time             () const;
    Order*                      fetch_and_occupy_order      (Task* occupying_task, const Time& now, const string& cause, const Process_class*);
    bool                        request_order               ( const Time& now, const string& cause );   // Fordert einen Auftrag für die _order_queue an
    bool                        try_to_end_task             (Process_class_requestor*, Process_class*);
    void                        kill_queued_task            ( int task_id );
    void                        end_tasks                   ( const string& task_warning );
    ptr<Module_instance>        create_module_instance      (Process_class*, const string& remote_scheduler, Task*);
    void                        count_task                  ()                                      { InterlockedIncrement( &_tasks_count ); }
    void                        count_step                  ()                                      { InterlockedIncrement( &_step_count ); }
    void                        increment_running_tasks     ()                                      { InterlockedIncrement( &_running_tasks_count ); }
    void                        decrement_running_tasks     ()                                      { InterlockedDecrement( &_running_tasks_count ); }
    string                      time_zone_name              () const;
    void                        stop_after_task_error       ( const string& error_message );   // _ignore_error verhindert stop()
    bool                        stops_on_task_error         ()                                      { return _stop_on_error; }
    bool                        above_min_tasks             () const;
    void                        on_task_finished            (Task*, Task_end_mode);                              // Task::finished() ruft das
    void                        try_start_tasks             ();
    bool                        try_start_one_task          ();
    public: Process_class* default_process_class() const;
    public: Process_class* default_process_class_or_null() const;

    public: bool has_own_process_class() const {
        return !_default_process_class_path.empty();
    }

    public: const Absolute_path& default_process_class_path() const {
        return _default_process_class_path;
    }

    public: bool max_tasks_reached() const {
        return running_tasks_count() >= _max_tasks;
    }

    public: int max_tasks() const {
        return _max_tasks;
    }
    
  protected:
    bool                       _stop_on_error;              // Nach Task-Fehler Job stoppen (default)

    public: int running_tasks_count() const {
        return _running_tasks.size();
    }

    public: bool is_task_ready_for_order(Process_class*);

    public: ArrayListJ java_tasks() const;

  private:
    void                        set_log                     ();
    Duration                    average_step_duration       ( const Duration& deflt );
    Duration                    db_average_step_duration    ( const Duration& deflt );
    void                        add_on_exit_commands_element( const xml::Element_ptr& commands_element );
    void                        prepare_on_exit_commands    ();
    bool                        temporary                   () const                                { return _temporary; }
    void                        database_record_store       ();
    void                        database_record_remove      ();
    void                        database_record_load        ( Read_transaction* );
    void                        load_tasks                  ( Read_transaction* );
    void                        load_tasks_with_java        ();
    void                        load_tasks_from_db          ( Read_transaction* );
    bool                        check_for_changed_directory ( const Time& now );
    void                        update_changed_directories  ( Directory_watcher* );
    string                      trigger_files               ( Task* task = NULL );
    void                        reset_scheduling            ();
    void                        select_period               ( const Time& );
    void                        set_period                  (const Period& p);
    void                        set_next_start_time         ( const Time& now, bool repeat = false );
    void                        set_next_start_time2        (const Time& now, bool repeat);
    void                        set_next_time               ( const Time& );
    Time                        next_start_time             () const;
    void                        calculate_next_time         ( const Time& now );
    void                        process_orders              ();
    void                        continue_tasks_waiting_for_order();
    ptr<Task>                   task_to_start               ();
    void                        set_state                   ( State );
    bool                        execute_state_cmd           (State_cmd);
    void                        set_error_xc                ( const Xc& );
    void                        set_error_xc_only           ( const Xc& );
    void                        set_error                   ( const exception& );
    void                        reset_error                 ()                                      { _error = NULL,  _log->reset_highest_level(); }
    void                        set_job_error               ( const exception& );
    void                        withdraw_order_request      ();
    ptr<Task>                   get_task_from_queue         ( const Time& now );
    bool                        below_min_tasks             () const;
    bool                        should_start_task_because_of_min_tasks();
    int                         not_ending_tasks_count      () const;
    void                        check_min_tasks             ( const string& cause );                // Setzt _start_min_tasks
    
    string script_text() const { 
        return _module? _module->read_source_script() : ""; 
    }

    friend struct               Task;
    friend struct               Job_history;

    Fill_zero                  _zero_;
    bool                       _waiting_for_process;        // Task kann nicht gestartet werden, weil kein Prozess in der Prozessklasse verfügbar ist
    bool                       _waiting_for_process_try_again;  
    string                     _description;                // <description>

    typed_call_register<Standard_job> _call_register;

    string                     _title;                      // <job title="">
    string                     _state_text;                 // spooler_job.state_text = "..."

    long32                     _tasks_count;                // Anzahl gestarteter Tasks seit Spooler-Start
    long32                     _step_count;                 // Anzahl spooler_process() aller Tasks

    State                      _state;
    bool                       _wake_when_in_period;
    bool                       _is_permanently_stopped;     // s_stopped wird zum Beenden verwendet und gilt nicht dauerhaft. Das sollte vereinfacht werden!
    bool                       _reread;                     // <script> neu einlesen, also <include> erneut ausführen
    Duration                   _task_timeout;               // Frist für einen Schritt einer Task
    Duration                   _idle_timeout;               // Frist für den Zustand Task::s_running_waiting_for_order
    bool                       _force_idle_timeout;         // _idle_timeout wirkt beendet auch Tasks, wenn _min_tasks unterschritten wird
    bool                       _temporary;                  // Job nach einem Lauf entfernen
    bool                       _start_once_for_directory;
    bool                       _machine_resumable;          // Test
    bool                       _ignore_every_signal;        // Nur Unix: Nach Beendigung mit Signal (kill, Absturz) den Job nicht stoppen 
    stdext::hash_set<int>      _ignore_signals_set;

    bool                       _log_append;                 // Jobprotokoll fortschreiben <job log_append=(yes|no)>

    ptr<Job_schedule_use>      _schedule_use;
    bool                       _start_once;                 // <run_time start_once="">, wird false nach Start
    Period                     _period;                     // Derzeitige oder nächste Period
    Time                       _next_single_start;
    Time                       _next_start_time;

    Duration                   _repeat;                     // spooler_task.repeat
    Time                       _delay_until;                // Nach Fehler verzögern

    xml::Document_ptr          _commands_document;          // <commands>...
    typedef map<int,xml::Element_ptr>  Exit_code_commands_map;
    Exit_code_commands_map     _exit_code_commands_map;
    xml::Element_ptr           _exit_code_commands_on_signal;
    Delay_after_error          _delay_after_error;
    long32                     _error_steps;                // Zahl aufeinanderfolgender Fehler

    Directory_watcher_list     _directory_watcher_list;
    bool                       _directory_changed;
    string                     _changed_directories;
    Xc_copy                    _error;

    ptr<Com_variable_set>      _default_params;

    Absolute_path              _default_process_class_path;
    ptr<Module>                _module;                     // Job hat ein eigenes Skript
    xml::Element_ptr           _script_element;             // <script> (mit <include>) für <modify_job cmd="reload"/>

    typedef vector< ptr<Module_instance> >  Module_instance_vector;
    Module_instance_vector     _module_instances;
    ptr<Com_job>               _com_job;

    ptr<Task_queue>            _task_queue;                 // Warteschlange der nächsten zu startenden Tasks
    Task_set                   _running_tasks;              // Alle laufenden Tasks (auch die gestarteten, aber wartenden, z.B. s_running_waiting_for_order)
    long32                     _running_tasks_count;        // Anzahl der Tasks, die tatsächlich laufen (und nicht gerade warten)
    int                        _min_tasks;                  // Min. Anzahl Tasks, die der Scheduler stets laufen lassen soll
    bool                       _start_min_tasks;            // Starte Tasks solange _running_tasks.count() < _min_tasks
    int                        _max_tasks;                  // Max. Anzahl gleichzeitig laufender Tasks. _running_tasks.size() <= _max_tasks!

    Job_history                _history;

    bool                       _is_order_controlled;

    ptr<Combined_job_nodes>    _combined_job_nodes;

    Delay_order_after_setback  _delay_order_after_setback;
    int                        _max_order_setbacks;

    typedef list< pair<string,string> > Start_when_directory_changed_list;  
    Start_when_directory_changed_list  _start_when_directory_changed_list;      // Für <start_when_directory_changed>

    ptr<lock::Requestor>       _lock_requestor;

    Time                       _db_next_start_time;
    bool                       _db_stopped;

    string                     _warn_if_shorter_than_string;
    string                     _warn_if_longer_than_string;
    bool                       _enabled;
    Log_level _stderr_log_level;
};

//-------------------------------------------------------------------------------------Internal_job

struct Internal_job : Standard_job
{
                                Internal_job                ( Scheduler*, const string& name, const ptr<Module>& );
};

//------------------------------------------------------------------------------------------Job_map

#define FOR_EACH_JOB( JOB )  FOR_EACH_FILE_BASED( Job, JOB )

//---------------------------------------------------------------------------------------Job_folder

struct Job_folder : typed_folder< Job >
{
                                Job_folder                  ( Folder* );


    void                        add_job                     ( const ptr<Job>& job )                 { add_file_based( +job ); }
    Job*                        job                         ( const string& name )                  { return file_based( name ); }
    Job*                        job_or_null                 ( const string& name )                  { return file_based_or_null( name ); }
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& )  { return doc.createElement( "jobs" ); }
};

//------------------------------------------------------------------------------------Job_subsystem

struct Job_subsystem: Object, 
                      file_based_subsystem< Job >,
                      javabridge::has_proxy<Job_subsystem>
{
                                Job_subsystem               ( Scheduler*, Type_code );

    virtual ptr<Job_folder>     new_job_folder              ( Folder* )                             = 0;
    virtual bool                is_any_task_queued          ()                                      = 0;
    virtual void                append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* ) = 0;
    virtual Schedule*           default_schedule            ()                                      = 0;
    virtual void                do_something                ()                                      = 0;

    Job*                        job                         ( const Absolute_path& job_path )       { return file_based( job_path ); } 
    Job*                        job_or_null                 ( const Absolute_path& job_path )       { return file_based_or_null( job_path ); }
    Job*                        job_by_string               (const string& absolute_path)           { return job(Absolute_path(absolute_path)); }
    Job*                        job_by_string_or_null       (const string& absolute_path)           { return job_or_null(Absolute_path(absolute_path)); }
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "jobs" ); }
};

ptr<Job_subsystem>              new_job_subsystem           ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
