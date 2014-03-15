package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppThreadSafe;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse;

@CppClass(clas="sos::scheduler::Spooler", directory="scheduler", include="spooler.h")
public interface SpoolerC extends CppProxyWithSister<Scheduler> {
    Prefix_logC log();
    String id();
    String id_for_db();
    String http_url();
    String name();
    String param();
    Variable_setC variables();
    int udp_port();
    int tcp_port();
    String hostname();
    String hostname_complete();
    String include_path();
    String temp_dir();
    String state_name();
    String log_directory();
    boolean is_service();
    String configuration_file_path();
    String directory();
    String local_configuration_directory();
    String string_need_db();
    void log_show_state();
    void assign_stdout ();
    String execute_xml_string(String xml);
    String execute_xml_string_with_security_level(String xml, String security_level, String clientHostName);
    HttpResponseC java_execute_http(SchedulerHttpRequest request, SchedulerHttpResponse response);
    HttpResponseC java_execute_http_with_security_level(SchedulerHttpRequest request, SchedulerHttpResponse response, String security_level);
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
    void abort_immediately_after_distribution_error(String debug_text);
    void abort_immediately(boolean restart, String message_text);
    void abort_now(boolean restart);
    void execute_state_cmd();
    boolean is_termination_state_cmd();
//    ptr<Task>                   get_task                    ( int task_id );
//    ptr<Task>                   get_task_or_null            ( int task_id );
    void load_arg();
    void load();
    void self_check();
    void update_console_title(int level);
    void start();
    void activate();
    void execute_config_commands();
    void run_check_ctrl_c();
    void stop();
    void end_waiting_tasks();
    void nichts_getan(int anzahl, String x);
    void run();
    boolean name_is_valid(String name);
    void check_name(String name);

    // Cluster
    void check_cluster();
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
    SettingsC modifiable_settings();
    @CppThreadSafe void signal();
    DatabaseC db();
    Folder_subsystemC folder_subsystem();
//    Process_class_subsystem*    process_class_subsystem     () const;
//    Task_subsystem*             task_subsystem              () const;
//    Task_subsystem*             task_subsystem_or_null      () const                            { return _task_subsystem; }
    Job_subsystemC job_subsystem();
    Job_subsystemC job_subsystem_or_null();
    Order_subsystemC order_subsystem();
    Task_subsystemC task_subsystem();
//    Standing_order_subsystem*   standing_order_subsystem    () const;
//    Schedule_subsystem_interface* schedule_subsystem        () const;
//    Java_subsystem_interface*   java_subsystem              ()                                  { return _java_subsystem; }
//    lock::Lock_subsystem*       lock_subsystem              ()                                  { return _lock_subsystem; }
//    Event_subsystem*            event_subsystem             () const                            { return _event_subsystem; }

//    supervisor::Supervisor_client_interface*supervisor_client ();
    boolean has_any_task();
    @CppThreadSafe void write_to_scheduler_log(String category, String text);
    String setting(int index);
    String time_zone_name();
}
