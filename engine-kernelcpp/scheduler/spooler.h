// $Id: spooler.h 15016 2011-08-24 12:27:20Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_H 
#define __SPOOLER_H

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/string_list.h"
#include "../zschimmer/xml_java.h"
#include "../zschimmer/xslt_java.h"
#include "../kram/sos.h"
#include "../kram/sysxcept.h"
#include "../kram/sosopt.h"

#if defined Z_LINK_STATIC || defined Z_HPUX_IA64   // HP-UX/Itantium-Java schafft es nicht, libhostjava.so nachzuladen. Also binden wir es ein.
#   define SCHEDULER_WITH_HOSTJAVA
#endif

#if defined Z_WINDOWS && !defined Z_64
#   import <msxml.tlb> rename_namespace("msxml")    // Wir brauchen IXMLDOMDocument (wird von spooler.odl in Variable_set::get_dom() verlangt)
#else
    namespace msxml { struct IXMLDOMDocument; }     // Dummy
#endif

#ifndef Z_WINDOWS
    const int _dstbias = -3600;
#endif

#include <stdio.h>

#ifdef Z_WINDOWS
#   include "../zschimmer/z_windows.h"
#   define _CRTDBG_MAPALLOC
#   include <crtdbg.h>
#endif
        
#include <set>
#include <map>
#include <list>
#include <time.h>

#include "../kram/sossock1.h"
#include "../kram/sosdate.h"
#include "../kram/sosprof.h"
#include "../kram/com_simple_standards.h"
#include "../kram/log.h"

#include "../zschimmer/file.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/threads.h"
#include "../zschimmer/com_remote.h"
#include "../zschimmer/java.h"
#include "../zschimmer/Has_proxy.h"
#include "../zschimmer/z_sql.h"
#include "../zschimmer/message.h"
#include "../zschimmer/file_path.h"
#include "../zschimmer/xml.h"
#include "../zschimmer/z_io.h"

using namespace zschimmer;
using namespace zschimmer::com;

namespace sos {
namespace scheduler {

namespace time {
    struct Duration;
}

using time::Duration;

//--------------------------------------------------------------------------------------------const
    
extern const char               version_string[];
extern volatile int             ctrl_c_pressed;
extern const int                const_order_id_length_max;
extern const string             variable_set_name_for_substitution;
const int                       max_memory_file_size          = 10*1024*1024;   // Für Dateien, die komplett in den Speicher geladen werden
extern const int                max_open_log_files;
extern const Duration           delete_temporary_files_delay;
extern const Duration           delete_temporary_files_retry;

const int                       max_processes                 = 10000;    // kein Limit (HP-UX erlaubt 64 aktive fork())

extern bool                     static_ld_library_path_changed;
extern string                   static_original_ld_library_path;                   // Inhalt der Umgebungsvariablen LD_LIBRARY_PATH
#ifdef Z_HPUX
    extern string               static_ld_preload;                        // Inhalt der Umgebungsvariablen LD_PRELOAD
#endif

//-------------------------------------------------------------------------------------------------

using namespace ::std;
using namespace ::zschimmer::file;

struct Abstract_scheduler_object;
struct Api_process;
struct Command_processor;
struct Communication;
struct Event;
struct File_logger;
struct Get_events_command_response;
struct Job;
struct Job_folder;
struct Job_subsystem;
struct Module;
struct Module_instance;
struct Object_server;
struct Order_folder_interface;
struct Process;
struct Process_class;
struct Process_class_folder;
struct Scheduler_script_folder;
struct Scheduler_object;
struct Scheduler_event;
struct Show_calendar_options;
struct Show_what;
struct Spooler;
typedef Spooler Scheduler;
struct Task_subsystem;
struct Subprocess;
struct Subprocess_register;
struct Task;
struct Web_service;
struct Web_service_operation;
struct Web_service_request;
struct Web_service_response;
struct Xml_client_connection;
struct Xslt_stylesheet;

//struct Pause_scheduler_call;
//struct Continue_scheduler_call;
//struct Reload_scheduler_call;
//struct Terminate_scheduler_call;
//struct Let_run_terminate_and_restart_scheduler_call;

namespace database
{
    struct Database;
    struct Transaction;
}
using namespace database;


namespace directory_observer
{
  //struct Directory_observer;
    struct Directory_entry;
    struct Directory_tree;
    struct Directory;
}


namespace folder
{
    enum Configuration_origin
    {
        confdir_none,
        confdir_local,
        confdir_cache,
        confdir__max = confdir_cache
    };

    struct Configuration;
    struct File_based;
}
using namespace folder;


namespace include
{
    struct Has_includes;
};
using namespace include;


namespace lock
{
    struct Holder;
    struct Lock;
    struct Lock_folder;
    struct Lock_subsystem;
    struct Requestor;
}

struct Monitor_folder;

namespace order
{
    struct Job_chain;
    struct Job_chain_folder_interface;
    struct Order_queue;
    struct Order;
    struct Order_state_transition;
    struct Order_subsystem;
    //struct Standing_order;
    struct Standing_order_folder;
    struct Standing_order_subsystem;

    namespace job_chain
    {
        struct Nested_job_chain_node;
        struct Node;
        struct Job_node;
        struct Order_queue_node;
        struct Sink_node;
    }
}
using namespace order;


namespace schedule
{
    struct Period;
    struct Schedule;
    struct Schedule_use;
    struct Schedule_folder;
    struct Schedule_subsystem_interface;
}
using schedule::Period;
using schedule::Schedule;  
using schedule::Schedule_use;  
using schedule::Schedule_folder;  
using schedule::Schedule_subsystem_interface;  



namespace supervisor
{
    struct Remote_scheduler_interface;
    struct Supervisor_interface;
    struct Supervisor_client_interface;
};



typedef stdext::hash_set<string> String_set;

} //namespace scheduler
} //namespace sos

//-------------------------------------------------------------------------------------------------

#include "javaproxy.h"
#include "settings.h"
#include "spooler_com.h"
#include "spooler_xslt_stylesheet.h"
#include "spooler_common.h"
#include "spooler_time.h"
#include "spooler_mail.h"
#include "spooler_log.h"
#include "scheduler_object.h"
#include "subsystem.h"
#include "spooler_event.h"
#include "Event_subsystem.h"
#include "spooler_security.h"
#include "spooler_wait.h"
#include "path.h"
#include "directory_observer.h"
#include "include.h"
#include "folder.h"
#include "register.h"
#include "lock.h"
#include "Module_monitor.h"
#include "Module_monitors.h"
#include "Module_monitor_instances.h"
#include "Monitor.h"
#include "Monitor_folder.h"
#include "Monitor_subsystem.h"
#include "schedule.h"
#include "Timed_call.h"
#include "scheduler_script.h"
#include "spooler_communication.h"
#include "spooler_http.h"
#include "spooler_command.h"
#include "spooler_process.h"
#include "spooler_module.h"
#include "spooler_module_com.h"
#include "spooler_module_internal.h"
#include "spooler_module_java.h"
#include "spooler_module_process.h"
#include "database.h"
#include "spooler_order.h"
#include "spooler_order_file.h"
#include "spooler_job.h"
#include "spooler_subprocess.h"
#include "spooler_task.h"
#include "spooler_thread.h"
#include "spooler_service.h"
#include "spooler_web_service.h"
#include "spooler_module_remote.h"
#include "spooler_module_remote_server.h"
#include "cluster.h"
#include "java_subsystem.h"
#include "supervisor.h"
#include "supervisor_client.h"
#include "standing_order.h"
#include "xml_client_connection.h"

//-------------------------------------------------------------------------------------------------

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

extern volatile int             ctrl_c_pressed;

//-------------------------------------------------------------------------------------------------

int                             read_profile_mail_on_process( const string& profile, const string& section, const string& entry, int deflt );
int                             read_profile_history_on_process( const string& prof, const string& section, const string& entry, int deflt );
int                             make_history_on_process  ( const string& v, int deflt );
Archive_switch                  read_profile_archive        ( const string& profile, const string& section, const string& entry, Archive_switch deflt );
Archive_switch                  make_archive          ( const string& value, Archive_switch deflt );
With_log_switch                 read_profile_with_log       ( const string& profile, const string& section, const string& entry, Archive_switch deflt );
First_and_last                  read_profile_yes_no_last_both( const string& profile, const string& section, const string& entry, First_and_last deflt );
First_and_last                  make_yes_no_last_both  ( const string& setting_name, const string& value, First_and_last deflt );


//----------------------------------------------------------------------------State_changed_handler

typedef void (*State_changed_handler)( Spooler*, void* );

typedef map<Thread_id,Task_subsystem*>      Thread_id_map;

//------------------------------------------------------------------------------------------Spooler

struct Spooler : Object,
                 Abstract_scheduler_object,
                 javabridge::has_proxy<Spooler>
{
    enum State  // Align with Java SchedulerState !
    {
        s_none,
        s_stopped,
        s_loading,
        s_starting,
        s_waiting_for_activation,
        s_waiting_for_activation_paused,
        s_running,
        s_paused,
        s_stopping,
        s_stopping_let_run,
        s__max
    };

    enum State_cmd
    {
        sc_none,
        sc_terminate,           // s_running | s_paused -> s_stopped, exit()
        sc_terminate_and_restart,
        sc_let_run_terminate_and_restart,
        sc_load_config,         
        sc_reload,
        sc_pause,               // s_running -> s_paused
        sc_continue,            // s_paused -> s_running
        sc__max
    };

                                Spooler                     (jobject java_main_context = NULL);
                               ~Spooler                     ();

    void                        close                       ();

    // Abstract_scheduler_object:
    void                        print_xml_child_elements_for_event( String_stream*, Scheduler_event* );


    jobject                     java_main_context           () const                            { return _java_main_context; }
    Thread_id                   thread_id                   () const                            { return _thread_id; }
    const Settings*             settings                    () const;
    Settings*                   modifiable_settings         () const;
    void                        modify_settings             (const Com_variable_set& v)         { _settings->set_from_variables(v); }
    void                    set_id                          ( const string& );
    const string&               id                          () const                            { return _spooler_id; }
    string                      id_for_db                   () const                            { return _spooler_id.empty()? "-" : _spooler_id; }
    string                      http_url                    () const;
    string                      name                        () const;                           // "Scheduler -id=... host:port"
    const string&               param                       () const                            { return _spooler_param; }
    ptr<Com_variable_set>       variables                   () const                            { return _variables; }
    int                         udp_port                    () const                            { return _udp_port; }
    int                         tcp_port                    () const                            { return _tcp_port; }
    string                      hostname                    () const                            { return _short_hostname; } 
    string                      hostname_complete           () const                            { return _complete_hostname; } 
    File_path                   include_path                () const                            { return _include_path; }
    string                      temp_dir                    () const                            { return _temp_dir; }
  //int                         priority_max                () const                            { return _priority_max; }
    State                       state                       () const                            { return _state; }      // Thread-sicher!
    string                      state_name                  () const                            { return state_name( _state ); }
    static string               state_name                  ( State );
    const string&               log_directory               () const                            { return _log_directory; }                      
    Time                        start_time                  () const                            { return _spooler_start_time; }
    Security::Level             security_level              ( const Ip_address& );
    const schedule::Holidays&   holidays                    () const                            { return _holidays; }
    bool                        is_service                  () const                            { return _is_service; }
    const file::File_path&      configuration_file_path     () const                            { return _configuration_file_path; }
    string                      directory                   () const                            { return _directory; }
    string                      home_directory              () const                            { return _home_directory; }
    string                      local_configuration_directory() const                           { return _configuration_directories[confdir_local]; }
    string                      time_zone_name              () const                            { return _time_zone_name; }

    Timed_call*                 enqueue_call                (Timed_call*);
    void                        cancel_call                 (Timed_call*);

    void                        log_show_state              ( Prefix_log* log = NULL );
    int                         launch                      ( int argc, char** argv, const string& params );
    void                        assign_stdout               ();

    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    string                      execute_xml_string          (const string& xml);
    string                      execute_xml_string_with_security_level (const string& xml, const string& security_level, const string& client_host);
    string                      execute_xml_string_with_security_level (const string& xml, Security::Level security_level, const Host& client_host);
    http::Java_response*        java_execute_http           (const SchedulerHttpRequestJ&, const SchedulerHttpResponseJ&);
    http::Java_response*        java_execute_http_with_security_level (const SchedulerHttpRequestJ&, const SchedulerHttpResponseJ&, const string& security_level);
    void                        cmd_reload                  ();
    void                        cmd_pause                   ();
    void                        cmd_continue                ();
    void                        cmd_terminate_after_error   ( const string& function_name, const string& message_text );
    void                        cmd_terminate               ( bool restart = false, int timeout = INT_MAX, 
                                                              const string& continue_exclusive_operation = cluster::continue_exclusive_non_backup, 
                                                              bool terminate_all_schedulers = false );
    void                        cmd_terminate_and_restart   ( int timeout = INT_MAX )           { return cmd_terminate( true, timeout ); }
    void                        cmd_let_run_terminate_and_restart();
    void                        cmd_add_jobs                ( const xml::Element_ptr& element );
    void                        cmd_job                     ( const xml::Element_ptr& );

    void                        abort_immediately_after_distribution_error( const string& debug_text );
    void                        abort_immediately           ( bool restart = false, const string& message_text = "" );
    void                        abort_now                   ( bool restart = false );
    enum Kill_all_processs_option { kill_registered_pids_only, kill_task_subsystem };
    void                        kill_all_processes          ( Kill_all_processs_option );

    void                        cmd_load_config             ( const xml::Element_ptr&, const string& source_filename );
    void                        execute_state_cmd           ();
    bool                        is_termination_state_cmd    ();

    ptr<Task>                   get_task                    ( int task_id );
    ptr<Task>                   get_task_or_null            ( int task_id );

    friend struct               Com_spooler;

    string                      truncate_head(const string& str);
    void                        check_licence();
    void                        release_com_objects         ();
    void                        load_arg                    ();
    void                        read_ini_filename           ();
    void                        read_ini_file               ();
    void                        read_command_line_arguments ();
    void                        handle_configuration_directories();
    void                        set_home_directory          ();
    void                        set_check_memory_leak       (bool);
    void                        load                        ();
    void                        open_pid_file               ();
    int                         pid                         () const { return _pid; }
    void                        fetch_hostname              ();
    void                        read_xml_configuration      ();
    void                        initialize_java_subsystem     ();
    void                        load_config                 ( const xml::Element_ptr& config, const string& source_filename, bool is_base = false );
    string                      configuration_for_single_job_script();
    xml::Element_ptr            state_dom_element           ( const xml::Document_ptr&, const Show_what& = show_standard );
#ifdef Z_WINDOWS
    MEMORYSTATUS                memory_status_init();
    SIZE_T                      memory_status_calculate_reserved_virtual(MEMORYSTATUS m);
    string                      mb_formatted(SIZE_T value);
#endif

    void                        set_state                   ( State );
    void                        self_check                  ();
    void                        report_event                ( Scheduler_event* e )              { if( _scheduler_event_manager )  _scheduler_event_manager->report_event( e ); }

  //void                        create_window               ();
    void                        update_console_title        ( int level = 1 );
    void                        start                       ();
    void                        activate                    (State);
    void                        execute_config_commands     ();
    void                        run_check_ctrl_c            ();
    void                        stop                        ( const exception* = NULL );
    void                        end_waiting_tasks           ();
    void                        nichts_getan                ( int anzahl, const string& );
    void                        try_run                     ();
    void                        run                         ();
    bool                        run_continue                ( const Time& now );

    bool                        name_is_valid               ( const string& name );
    void                        check_name                  ( const string& name );

    void                        initialize_sleep_handler    ();

    // Cluster
    void                        initialize_cluster          ();
    void                        stop_cluster                ();
    void                        check_cluster               ();
    bool                        assert_is_still_active      ( const string& debug_function, const string& debug_text = "", Transaction* = NULL );
    bool                        check_is_active             ( Transaction* = NULL );
    void                        assert_is_activated         ( const string& function );
    void                        do_a_heart_beat_when_needed (const string& debug_text);

    bool                        is_cluster                  () const                            { return _cluster != NULL; }
    bool                        cluster_is_active           ();
    bool                        has_exclusiveness           ();
    bool                        orders_are_distributed      ();
    void                        assert_are_orders_distributed( const string& message_text );
    string                      cluster_member_id           ();
    string                      distributed_member_id       ();
    string                      db_distributed_member_id    ();


    Process_id                  get_next_process_id         ()                                  { return _next_process_id++; }

    void                        wait                        ();
    void                        wait                        ( Wait_handles*, const Time& wait_until, Object* wait_until_object, const Time& resume_at, Object* resume_at_object );

    void                        signal                      ();
    void                        async_signal                ( const char* signal_name = "" )    { _scheduler_event.async_signal( signal_name ); }

    void                        send_cmd                    ();
    void                        send_cmd_via_tcp            ();
    void                        send_cmd_via_http           ();

    void                        register_api_process       (Api_process*);
    void                        unregister_api_process     (Process_id);
    Api_process*                task_process                (Process_id);

    // Prozesse
    void                        register_process_handle     ( Process_handle );                 // Für abort_immediately()
    void                        unregister_process_handle   ( Process_handle );                 // Für abort_immediately()
    void                        register_pid                ( int, bool is_process_group = false ); // Für abort_immediately()
    void                        unregister_pid              ( int );                            // Für abort_immediately()

    bool                        is_machine_suspendable      () const                            { return _dont_suspend_machine_counter == 0; }
    void                        begin_dont_suspend_machine  ();
    void                        end_dont_suspend_machine    ();
    void                        suspend_machine             ();
    static string               backup_logfile              ( const File_path );

  //Folder*                     folder                      ( const string& path )              { return _folder_subsystem->folder( path ); }
    Folder*                     root_folder                 ()                                  { return _folder_subsystem->root_folder(); }

    Database*                   db                          ()                                  { return _db; }

    InjectorJ                   injectorJ                   () const                            { return _java_subsystem->injectorJ(); }
    SchedulerJ&                 schedulerJ                  () const                            { return _java_subsystem->schedulerJ(); }
    string                      java_work_dir               ()                                  { return temp_dir() + Z_DIR_SEPARATOR "java"; }

    void                        new_subsystems              ();
    void                        destroy_subsystems          ();
    void                        initialize_subsystems       ();
    void                        initialize_subsystems_after_base_processing();
    void                        load_subsystems             ();
    void                        activate_subsystems         ();
    void                        stop_subsystems             ();

    Scheduler_script_subsystem_interface* scheduler_script_subsystem() const                    { return _scheduler_script_subsystem; }
    Folder_subsystem*           folder_subsystem            () const                            { return _folder_subsystem; }
    Process_class_subsystem*    process_class_subsystem     () const;
    Task_subsystem*             task_subsystem              () const;
    Task_subsystem*             task_subsystem_or_null      () const                            { return _task_subsystem; }
    Job_subsystem*              job_subsystem               () const;
    Job_subsystem*              job_subsystem_or_null       () const                            { return _job_subsystem; }
    Monitor_subsystem*          monitor_subsystem           () const;
    Order_subsystem*            order_subsystem             () const;
    Standing_order_subsystem*   standing_order_subsystem    () const;
    Schedule_subsystem_interface* schedule_subsystem        () const;
    Java_subsystem_interface*   java_subsystem              ()                                  { return _java_subsystem; }
    lock::Lock_subsystem*       lock_subsystem              ()                                  { return _lock_subsystem; }
    Event_subsystem*            event_subsystem             () const                            { return _event_subsystem; }

    Process_class_subsystem*    subsystem                   ( Process_class* ) const            { return _process_class_subsystem; }
    lock::Lock_subsystem*       subsystem                   ( lock::Lock* ) const               { return _lock_subsystem; }
    Job_subsystem*              subsystem                   ( Job* ) const                      { return _job_subsystem; }
    Order_subsystem*            subsystem                   ( Job_chain* ) const                { return _order_subsystem; }

    supervisor::Supervisor_client_interface* supervisor_client();
    string                      supervisor_uri              ();
    bool                        has_any_task                ();

    void                        detect_warning_and_send_mail();
    void                        write_to_scheduler_log      (const string& category, const string& text) { Z_LOG2(category, text); }  // Für Java nicht mit Mutex abgesichert

  private:
    //void                        on_call                     (const Pause_scheduler_call&);
    //void                        on_call                     (const Continue_scheduler_call&);
    //void                        on_call                     (const Reload_scheduler_call&);
    //void                        on_call                     (const Terminate_scheduler_call&);
    //void                        on_call                     (const Let_run_terminate_and_restart_scheduler_call&);
    string                      interface_and_port_to_uri_authority(const string& interface_and_port) const;

  private:
    Fill_zero                  _zero_;
    jobject const              _java_main_context;
    int                        _argc;
    char**                     _argv;
    string                     _parameter_line;
    string                     _xml_schema_url;
    ptr<Settings>              _settings;

  public:
    Thread_semaphore           _lock;                       // Command_processor::execute_show_state() sperrt auch, für Zugriff auf _db.
    string                     _spooler_id;                 // -id=
    string                     _log_directory;              // -log-dir=
    bool                       _log_directory_as_option_set;// -log-dir= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _log_filename;
    bool                       _log_to_stderr;              // Zusätzlich nach stdout schreiben
    Log_level                  _log_to_stderr_level;
    ptr<log::cache::Request_cache> _log_file_cache;
    Cached_log_category        _scheduler_wait_log_category;
    File_path                  _include_path;
    bool                       _include_path_as_option_set; // -include-path= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _temp_dir;
    string                     _spooler_param;              // -param= Parameter für Skripten
    bool                       _spooler_param_as_option_set;// -param= als Option gesetzt, überschreibt Angabe in spooler.xml
  //int                        _priority_max;               // <config priority_max=...>
    bool                       _http_port_as_option_set;
    bool                       _https_port_as_option_set;
    int                        _tcp_port;                   // <config tcp=...>
    bool                       _tcp_port_as_option_set;
    int                        _udp_port;                   // <config udp=...>
    bool                       _udp_port_as_option_set;
    bool                       _reuse_addr;
    Host                       _ip_address;
    bool                       _ip_address_as_option_set;
    string                     _version;
    Log                        _base_log;
    int                        _pid;
    string                     _my_program_filename;
    bool                       _is_service;                 // NT-Dienst
    bool                       _debug;
    Log_level                  _log_level;
    bool                       _mail_on_warning;            // Für Job-Protokolle
    bool                       _mail_on_error;              // Für Job-Protokolle
    int                        _mail_on_process;            // Für Job-Protokolle
    bool                       _mail_on_success;            // Für Job-Protokolle
    First_and_last             _mail_on_delay_after_error;  // Für Job-Protokolle
    string                     _mail_encoding;

    Mail_defaults              _mail_defaults;

    Duration                   _log_collect_within;
    Duration                   _log_collect_max;

    Time                       _last_mail_timestamp;

    string                     _job_history_columns;
    bool                       _job_history_yes;
    int                        _job_history_on_process;
    Archive_switch             _job_history_archive;
    With_log_switch            _job_history_with_log;

    bool                       _order_history_yes;
    With_log_switch            _order_history_with_log;
    Log_level                  _db_log_level;

    string                     _factory_ini;                // -ini=factory.ini
    string                     _sos_ini;                    // -sos.ini=sos.ini
    string                     _short_hostname;             // Ohne Netzwerk
    string                     _complete_hostname;          // Mit Netzwerk

    ptr<Com_spooler>           _com_spooler;                // COM-Objekt spooler
    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log

    ptr<Database>              _db;
    bool                       _db_check_integrity;
    bool                       _remote_commands_allowed_for_licence;   // executing of remote commands are not allowed for "normal" scheduler

    int                        _waiting_errno;              // Scheduler unterbrochen wegen errno (spooler_log.cxx)
    string                     _waiting_errno_filename;

    bool                       _interactive;                // Kommandos über stdin zulassen
    bool                       _manual;
  //string                     _job_name;                   // Bei manuellem Betrieb

    string                     _send_cmd_xml_bytes;         // Der Spooler soll nur dem eigentlichen Spooler dieses Kommando schicken und sich dann beenden.
    string                     _cmd_xml_bytes;              // Parameter -cmd, ein zuerst auszuführendes Kommando.
    string                     _pid_filename;


    typedef set<File_based_subsystem*>  File_based_subsystems;
    File_based_subsystems              _file_based_subsystems;
    Subsystem_register                 _subsystem_register;         // alle Subsysteme

    ptr<Scheduler_script_subsystem_interface> _scheduler_script_subsystem;
    ptr<Folder_subsystem>            _folder_subsystem;
    ptr<Process_class_subsystem>     _process_class_subsystem;
    ptr<Job_subsystem>               _job_subsystem;
    ptr<Task_subsystem>              _task_subsystem;
    ptr<Monitor_subsystem>           _monitor_subsystem;
    ptr<Order_subsystem>             _order_subsystem;
    ptr<Standing_order_subsystem>    _standing_order_subsystem;
    ptr<Schedule_subsystem_interface>_schedule_subsystem;
    ptr<http::Http_server_interface> _http_server;
    ptr<Web_services_interface>      _web_services;
    ptr<Java_subsystem_interface>    _java_subsystem;
    ptr<lock::Lock_subsystem>        _lock_subsystem;
    ptr<Event_subsystem>             _event_subsystem;

    Wait_handles               _wait_handles;
    Event                      _scheduler_event;
    typed_call_register<Spooler> _call_register;

    int                        _loop_counter;               // Zähler der Schleifendurchläufe in spooler.cxx
    int                        _wait_counter;               // Zähler der Aufrufe von wait_until()
    time_t                     _last_time_enter_pressed;    // int wegen Threads (Spooler_communication und Spooler_wait)
    Rotating_bar               _wait_rotating_bar;

    ptr<object_server::Connection_manager>  _connection_manager;
    bool                       _validate_xml;
    xml::Schema_ptr            _schema;
    file::File_path            _configuration_file_path;            // -config=
    file::File_path            _opt_configuration_directory;        // JS-462
    bool                       _configuration_is_job_script;        // Als Konfigurationsdatei ist eine Skript-Datei angegeben worden
    string                     _configuration_job_script_language; 

    vector<file::File_path>    _configuration_directories;
    vector<bool>               _configuration_directories_as_option_set;
    Absolute_path              _configuration_start_job_after_added;
    Absolute_path              _configuration_start_job_after_modified;
    Absolute_path              _configuration_start_job_after_deleted;
    file::File_path            _central_configuration_directory;         // Für den Supervisor zur Replikation
    bool                       _central_configuration_directory_as_option_set;

    bool                       _executing_command;          // true: database wartet nicht auf Datenbank (damit Scheduler nicht blockiert)
    bool                       _is_reopening_database_after_error;       // true: database.cxx wartet nicht auf Datenbank (damit Scheduler nicht blockiert)
    int                        _process_count;

    bool                       _subprocess_own_process_group_default;
    Process_handle             _process_handles[ max_processes ];   // Für abort_immediately(), mutex-frei alle abhängigen Prozesse
    struct Killpid { int _pid; bool _is_process_group; };
    Killpid                    _pids[ max_processes ];              // Für abort_immediately(), mutex-frei alle Task.add_pid(), Subprozesse der Tasks
    bool                       _are_all_tasks_killed;

    schedule::Holidays         _holidays;                   // Feiertage für alle Jobs

    State_changed_handler      _state_changed_handler;      // Callback für NT-Dienst SetServiceStatus()

    xml::Document_ptr          _config_document_to_load;    // Für cmd_load_config(), das Dokument zu _config_element_to_load
    xml::Element_ptr           _config_element_to_load;     // Für cmd_load_config()
    string                     _config_source_filename;     // Für cmd_load_config(), der Dateiname der Quelle

    xml::Document_ptr          _config_document;            // Das Dokument zu _config_element
    xml::Element_ptr           _config_element;             // Die gerade geladene Konfiguration (und Job hat einen Verweis auf <job>)

    xml::Document_ptr          _commands_document;          // Zusammengebautes <commands>

    ptr<Com_variable_set>      _variables;
    Security                   _security;                   // <security>
    Communication              _communication;              // TCP und UDP

    ptr<supervisor::Supervisor_interface>        _supervisor;
    ptr<supervisor::Supervisor_client_interface> _supervisor_client;

    ptr<Scheduler_event_manager> _scheduler_event_manager;


    bool                       _ignore_process_classes;
    bool                       _ignore_process_classes_set;


    Time                       _last_wait_until;            // Für <show_state>
    Time                       _last_resume_at;             // Für <show_state>
    bool                       _print_time_every_second;

    Thread_id                  _thread_id;                  // Haupt-Thread
    Time                       _spooler_start_time;
    bool                       _pause_at_start;
    State                      _state;
    State_cmd                  _state_cmd;
    State_cmd                  _shutdown_cmd;               // run() beenden, also alle Tasks beenden!
    bool                       _shutdown_ignore_running_tasks;
    time_t                     _termination_gmtimeout_at;   // Für sc_terminate und sc_terminate_with_restart
    string                     _terminate_continue_exclusive_operation;
    bool                       _terminate_all_schedulers;
    bool                       _terminate_all_schedulers_with_restart;
    ptr<Async_operation>       _termination_async_operation;

    ptr<Async_operation>       _log_categories_reset_operation;
    Log_categories_content     _original_log_categories;

    string                     _home_directory;
    string                     _directory;
    File                       _pid_file;

    bool                       _zschimmer_mode;
    int                        _dont_suspend_machine_counter;   // >0: Kein suspend
    bool                       _suspend_after_resume;
    bool                       _should_suspend_machine;

    Event                      _waitable_timer;
    bool                       _is_waitable_timer_set;

    ptr<Com_variable_set>      _environment;
    Variable_set_map           _variable_set_map;           // _variable_set_map[""] = _environment; für <params>, Com_variable_set::set_dom()
    
    ptr<cluster::Cluster_subsystem_interface>      _cluster;
    cluster::Configuration     _cluster_configuration;
    bool                       _is_activated;
    bool                       _assert_is_active;
    int                        _is_in_check_is_active;
    bool                       _has_windows_console;
    bool                       _check_memory_leak;
    Process_id                 _next_process_id;
    typedef map<Process_id, ptr<Api_process> >  Api_process_register;
    Api_process_register      _api_process_register;
    Duration                   _max_micro_step_time;

    string                     _java_options;
    string                     _time_zone_name;
};

//------------------------------------------------------------------------------------Object_server 

struct Object_server : com::object_server::Server
{
                                Object_server               ();
};

//-------------------------------------------------------------------------------------------------

void                            spooler_restart             ( Log* log, bool is_service );
void                            send_error_email            ( const exception&, int argc, char** argv, const string& parameter_line, Spooler* spooler = NULL );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler

int                             spooler_main                ( int argc, char** argv, const string& parameters, jobject java_main_context = NULL );

} //namespace sos

#endif
