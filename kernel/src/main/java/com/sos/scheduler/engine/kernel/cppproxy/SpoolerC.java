package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppThreadSafe;

@CppClass(clas="sos::scheduler::Spooler", directory="scheduler", include="spooler.h")
public interface SpoolerC extends CppProxyWithSister<Scheduler> {
    Prefix_logC log();
//    void                        print_xml_child_elements_for_event( String_stream*, Scheduler_event* );
//    Thread_id                   thread_id                   () const                            { return _thread_id; }
    void set_id(String id);
    String id();
    String id_for_db();
    String http_url();
    String name();
    String param();
    int udp_port();
    int tcp_port();
    String hostname();
    String hostname_complete();
    String include_path();
    String temp_dir();
//??  State                       state                       () const                            { return _state; }
    String state_name();
    String log_directory();
//??    Time                        start_time                  () const                            { return _spooler_start_time; }
//??    Security::Level             security_level              ( const Ip_address& );
//??    const schedule::Holidays&   holidays                    () const                            { return _holidays; }
    boolean is_service();
    String directory();
    String string_need_db();
//??    void                        log_show_state              ( Prefix_log* log = NULL );
    void log_show_state();
    void assign_stdout ();
//    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }
    String execute_xml(String xml);
    void cmd_pause();
    void cmd_continue();
    void cmd_terminate_after_error(String function_name, String message_text);
    void cmd_terminate(boolean restart, int timeout, String continue_exclusive_operation, boolean terminate_all_schedulers);
    void cmd_terminate(boolean restart, int timeout, String continue_exclusive_operation);
    void cmd_terminate(boolean restart, int timeout);
    void cmd_terminate(boolean restart);
    void cmd_terminate();
    void cmd_terminate_and_restart(int timeout);
    void cmd_let_run_terminate_and_restart();
//??    void                        cmd_add_jobs                ( const xml::Element_ptr& element );
//??    void                        cmd_job                     ( const xml::Element_ptr& );
//
    void abort_immediately_after_distribution_error(String debug_text);
    void abort_immediately(boolean restart, String message_text);
    void abort_now(boolean restart);
//    enum Kill_all_processs_option { kill_registered_pids_only, kill_task_subsystem };
//??    void                        kill_all_processes          ( Kill_all_processs_option );
//
//??    void                        cmd_load_config             ( const xml::Element_ptr&, const string& source_filename );
    void execute_state_cmd();
    boolean is_termination_state_cmd();
//    ptr<Task>                   get_task                    ( int task_id );
//    ptr<Task>                   get_task_or_null            ( int task_id );
    void load_arg();
    void load();
//    void                        load_config                 ( const xml::Element_ptr& config, const string& source_filename );
//??    xml::Element_ptr            state_dom_element           ( const xml::Document_ptr&, const Show_what& = show_standard );
//??    void                        set_state                   ( State );
    void self_check();
//??    void                        report_event                ( Scheduler_event* e )              { if( _scheduler_event_manager )  _scheduler_event_manager->report_event( e ); }
    void update_console_title(int level);
    void start();
    void activate();
    void execute_config_commands();
    void run_check_ctrl_c();
//    void                        stop                        ( const exception* = NULL );
    void stop();
    void end_waiting_tasks();
    void nichts_getan(int anzahl, String x);
    void run();
//??    bool                        run_continue                ( const Time& now );
//
    boolean name_is_valid(String name);
    void check_name(String name);

    // Cluster
    void check_cluster();
//    bool                        assert_is_still_active      ( const string& debug_function, const string& debug_text = "", Transaction* = NULL );
//??    bool                        check_is_active             ( Transaction* = NULL );
    void assert_is_activated(String function );

    boolean is_cluster();
    boolean cluster_is_active();
    boolean has_exclusiveness();
    boolean orders_are_distributed();
    void assert_are_orders_distributed(String message_text);
    String cluster_member_id();
    String distributed_member_id();
    String db_cluster_member_id();
    String db_distributed_member_id();
//    Process_id                  get_next_process_id         ()                                  { return _next_process_id++; }
    //void wait();
//--    void                        wait                        ( Wait_handles*, const Time& wait_until, Object* wait_until_object, const Time& resume_at, Object* resume_at_object );
    void signal(String signal_name);
    boolean signaled();
    void send_cmd();
//--    void                        register_process_handle     ( Process_handle );                 // Für abort_immediately()
//--    void                        unregister_process_handle   ( Process_handle );                 // Für abort_immediately()
    void register_pid(int pid, boolean is_process_group);
    void register_pid(int pid);
    void unregister_pid(int pid);
    boolean is_machine_suspendable();
    void begin_dont_suspend_machine();
    void end_dont_suspend_machine();
    void suspend_machine();
//    Folder*                     root_folder                 ()                                  { return _folder_subsystem->root_folder(); }
    DatabaseC db();
//    sql::Database_descriptor*   database_descriptor         ()                                  { return db()->database_descriptor(); }//
    String java_work_dir();
//    Scheduler_script_subsystem_interface* scheduler_script_subsystem() const                    { return _scheduler_script_subsystem; }
    Folder_subsystemC folder_subsystem();
//    Process_class_subsystem*    process_class_subsystem     () const;
//    Task_subsystem*             task_subsystem              () const;
//    Task_subsystem*             task_subsystem_or_null      () const                            { return _task_subsystem; }
    Job_subsystemC job_subsystem();
    Job_subsystemC job_subsystem_or_null();
    Order_subsystemC order_subsystem();
//    Standing_order_subsystem*   standing_order_subsystem    () const;
//    Schedule_subsystem_interface* schedule_subsystem        () const;
//    Java_subsystem_interface*   java_subsystem              ()                                  { return _java_subsystem; }
//    lock::Lock_subsystem*       lock_subsystem              ()                                  { return _lock_subsystem; }
//    Event_subsystem*            event_subsystem             () const                            { return _event_subsystem; }

//    Process_class_subsystem*    subsystem                   ( Process_class* ) const            { return _process_class_subsystem; }
//    lock::Lock_subsystem*       subsystem                   ( lock::Lock* ) const               { return _lock_subsystem; }
//    Job_subsystem_interface*    subsystem                   ( Job* ) const                      { return _job_subsystem; }
//    Order_subsystem_interface*  subsystem                   ( Job_chain* ) const                { return _order_subsystem; }
//
//    supervisor::Supervisor_client_interface*supervisor_client ();
    boolean has_any_task();
    void detect_warning_and_send_mail();
    @CppThreadSafe void write_to_scheduler_log(String category, String text);
}
