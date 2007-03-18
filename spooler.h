// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
// §1172

#ifndef __SPOOLER_H
#define __SPOOLER_H

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/string_list.h"
#include "../kram/sos.h"
#include "../kram/sysxcept.h"
#include "../kram/sosopt.h"

#if defined Z_HPUX_PARISC //&& !defined __IA64__
#   define SCHEDULER_WITH_HOSTJAVA
#endif

#ifdef Z_WINDOWS
#   define SPOOLER_USE_LIBXML2              // Gnomes libxml2
//# define SPOOLER_USE_MSXML                // Microsofts msxml3
#else
#   define SPOOLER_USE_LIBXML2              // Gnomes libxml2
#endif


#ifdef SPOOLER_USE_MSXML
#   include "../zschimmer/xml_msxml.h"
    using namespace zschimmer::xml_msxml;
#else
#   ifdef Z_WINDOWS
#       include "../zschimmer/xml_msxml.h"              // Wir brauchen IXMLDOMDocument (wird von spooler.odl in Variable_set::get_dom() verlangt)
#   else
        namespace msxml { struct IXMLDOMDocument; }     // Dummy
#   endif
#endif

#ifdef SPOOLER_USE_LIBXML2
#   include "../zschimmer/xml_libxml2.h"
#   include "../zschimmer/xslt_libxslt.h"
    //using namespace zschimmer::xml_libxml2;
    namespace zschimmer
    {
        namespace xml
        {
            using namespace libxml2;
        }
    }
#endif

#ifndef Z_WINDOWS
    const int _dstbias = 3600;
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
#include "../kram/thread_semaphore.h"
#include "../kram/com_simple_standards.h"
#include "../kram/log.h"

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/file.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/threads.h"
#include "../zschimmer/com_remote.h"
#include "../zschimmer/java.h"
#include "../zschimmer/z_sql.h"
#include "../zschimmer/message.h"
#include "../zschimmer/file_path.h"
#include "../zschimmer/xml.h"
#include "../zschimmer/z_io.h"
#include "../zschimmer/pipe_collector.h"

using namespace zschimmer;
using namespace zschimmer::com;

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
extern const char*              temporary_process_class_name;
extern volatile int             ctrl_c_pressed;
extern const string             xml_schema_path;            // "scheduler.xsd"
extern const string             scheduler_character_encoding;
extern const int                const_order_id_length_max;
extern const string             variable_set_name_for_substitution;


#ifdef Z_WINDOWS
    const int                   max_processes                 =    30;    // Summe aller Handles darf MAXIMUM_WAIT_OBJECTS-1=63 nicht überschreiten
    const int                   max_communication_connections =    28;    // Summe aller Handles darf MAXIMUM_WAIT_OBJECTS-1=63 nicht überschreiten, inkl. udp und listen()
#else
    const int                   max_processes                 =   200;    // kein Limit (HP-UX erlaubt 64 aktive fork())
    const int                   max_communication_connections =   800;    // Limit ist FD_SETSIZE, inkl. udp und listen()
#endif

//-------------------------------------------------------------------------------------------------

using namespace ::std;


struct Communication;
struct Get_events_command_response;
struct Job;
struct Job_chain;
struct Job_subsystem_interface;
struct Module;
struct Module_instance;
struct Order_queue;
struct Order;
struct Process;
struct Process_class;
struct Remote_scheduler_interface;
struct Scheduler_object;
struct Scheduler_event;
struct Show_what;
struct Spooler;
typedef Spooler Scheduler;
struct Supervisor_interface;
struct Supervisor_client_interface;
struct Task_subsystem;
struct Subprocess;
struct Subprocess_register;
struct Task;
struct Transaction;
struct Web_service;
struct Web_service_operation;
struct Web_service_request;
struct Web_service_response;
struct Xml_client_connection;
struct Xslt_stylesheet;


typedef stdext::hash_set<string> String_set;

} //namespace scheduler

//namespace http
//{
//} //namespace http
} //namespace sos

//-------------------------------------------------------------------------------------------------

#include "spooler_com.h"
#include "spooler_xslt_stylesheet.h"
#include "spooler_common.h"
#include "spooler_time.h"
#include "spooler_mail.h"
#include "spooler_log.h"
#include "scheduler_object.h"
#include "subsystem.h"
#include "scheduler_script.h"
#include "spooler_event.h"
#include "spooler_security.h"
#include "spooler_wait.h"
#include "spooler_communication.h"
#include "spooler_http.h"
#include "spooler_command.h"
#include "spooler_module.h"
#include "spooler_module_com.h"
#include "spooler_module_internal.h"
#include "spooler_module_java.h"
#include "spooler_module_process.h"
#include "spooler_history.h"
#include "spooler_order.h"
#include "spooler_order_file.h"
#include "spooler_job.h"
#include "spooler_subprocess.h"
#include "spooler_task.h"
#include "spooler_process.h"
#include "spooler_thread.h"
#include "spooler_service.h"
#include "spooler_web_service.h"
#include "spooler_module_remote.h"
#include "spooler_module_remote_server.h"
#include "cluster.h"
#include "java_subsystem.h"
#include "supervisor.h"
#include "xml_client_connection.h"

//-------------------------------------------------------------------------------------------------

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

extern volatile int             ctrl_c_pressed;
extern Spooler*                 spooler_ptr;

//-------------------------------------------------------------------------------------------------

//Source_with_parts               text_from_xml_with_include  ( const xml::Element_ptr&, const Time& xml_mod_time, const string& include_path );
int                             read_profile_mail_on_process( const string& profile, const string& section, const string& entry, int deflt );
int                             read_profile_history_on_process( const string& prof, const string& section, const string& entry, int deflt );
Archive_switch                  read_profile_archive        ( const string& profile, const string& section, const string& entry, Archive_switch deflt );
With_log_switch                 read_profile_with_log       ( const string& profile, const string& section, const string& entry, Archive_switch deflt );
First_and_last                  read_profile_yes_no_last_both( const string& profile, const string& section, const string& entry, First_and_last deflt );

//----------------------------------------------------------------------------State_changed_handler

typedef void (*State_changed_handler)( Spooler*, void* );

typedef map<Thread_id,Task_subsystem*>      Thread_id_map;

//------------------------------------------------------------------------------------------Spooler

struct Spooler : Object,
                 Scheduler_object
{
    enum State
    {
        s_none,
        s_stopped,
        s_loading,
        s_starting,
        s_waiting_for_activation,
      //s_starting_waiting,
        s_running,
        s_paused,
        s_stopping,
        s_stopping_let_run,
        s__max
    };

    enum State_cmd
    {
        sc_none,
      //sc_stop,                // s_running | s_paused -> s_stopped
        sc_terminate,           // s_running | s_paused -> s_stopped, exit()
        sc_terminate_and_restart,
        sc_let_run_terminate_and_restart,
        sc_load_config,         
        sc_reload,
        sc_pause,               // s_running -> s_paused
        sc_continue,            // s_paused -> s_running
        sc__max
    };

    /*
    enum Need_db
    {
        need_db_no = false,
        need_db_yes,
        need_db_strict
    };
    */
                                Spooler                     ();
                               ~Spooler                     ();

    // Scheduler_object:
    void                        print_xml_child_elements_for_event( String_stream*, Scheduler_event* );


    // Aufrufe für andere Threads:
    Thread_id                   thread_id                   () const                            { return _thread_id; }
    void                    set_id                          ( const string& );
    const string&               id                          () const                            { return _spooler_id; }
    string                      id_for_db                   () const                            { return _spooler_id.empty()? "-" : _spooler_id; }
    string                      http_url                    () const;
    string                      name                        () const;                           // "Scheduler -id=... host:port"
    const string&               param                       () const                            { return _spooler_param; }
    int                         udp_port                    () const                            { return _udp_port; }
    int                         tcp_port                    () const                            { return _tcp_port; }
    File_path                   include_path                () const                            { return _include_path; }
    string                      temp_dir                    () const                            { return _temp_dir; }
    int                         priority_max                () const                            { return _priority_max; }
    State                       state                       () const                            { return _state; }
    string                      state_name                  () const                            { return state_name( _state ); }
    static string               state_name                  ( State );
    const string&               log_directory               () const                            { return _log_directory; }                      
    Time                        start_time                  () const                            { return _spooler_start_time; }
    Security::Level             security_level              ( const Ip_address& );
    const time::Holidays&       holidays                    () const                            { return _holidays; }
    bool                        is_service                  () const                            { return _is_service; }
    string                      directory                   () const                            { return _directory; }

    int                         launch                      ( int argc, char** argv, const string& params );                                
    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    // Für andere Threads:
    //Task_subsystem*             get_thread                  ( const string& thread_name );
    //Task_subsystem*             get_thread_or_null          ( const string& thread_name );
    //Task_subsystem*             select_thread_for_task      ( Task* );

  //Object_set_class*           get_object_set_class        ( const string& name );
  //Object_set_class*           get_object_set_class_or_null( const string& name );

    void                        cmd_reload                  ();
    void                        cmd_pause                   ()                                  { _state_cmd = sc_pause; signal( "pause" ); }
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
    void                        kill_all_processes          ();

    void                        cmd_load_config             ( const xml::Element_ptr&, const Time& xml_mod_time, const string& source_filename );
    void                        execute_state_cmd           ();
    bool                        is_termination_state_cmd    ();

    ptr<Task>                   get_task                    ( int task_id );
    ptr<Task>                   get_task_or_null            ( int task_id );

    friend struct               Com_spooler;

    void                        load_arg                    ();
    void                        load                        ();
    void                        load_config                 ( const xml::Element_ptr& config, const Time& xml_mod_time, const string& source_filename );
    void                        set_next_daylight_saving_transition();

  //void                        load_object_set_classes_from_xml( Object_set_class_list*, const xml::Element_ptr&, const Time& xml_mod_time );

    xml::Element_ptr            state_dom_element           ( const xml::Document_ptr&, const Show_what& = show_standard );
    void                        set_state                   ( State );
    void                        report_event                ( Scheduler_event* e )              { if( _scheduler_event_manager )  _scheduler_event_manager->report_event( e ); }

  //void                        create_window               ();
    void                        update_console_title        ( int level = 1 );
    void                        start                       ();
    void                        activate                    ();
    void                        execute_config_commands     ();
    void                        run_check_ctrl_c            ();
    void                        stop                        ( const exception* = NULL );
    void                        reload                      ();
    void                        end_waiting_tasks           ();
    void                        nichts_getan                ( int anzahl, const string& );
    void                        run                         ();
    bool                        run_continue                ( const Time& now );

    // Cluster
    void                        start_cluster               ();
    void                        check_cluster               ();
    bool                        assert_is_still_active      ( const string& debug_function, const string& debug_text = "", Transaction* = NULL );
    bool                        check_is_active             ( Transaction* = NULL );
    void                        assert_is_activated         ( const string& function );

    bool                        is_active                   ();
    bool                        has_exclusiveness           ();
    bool                        are_orders_distributed      ();
    void                        assert_are_orders_distributed( const string& message_text );
    void                        assert_has_exclusiveness    ( const string& message_text );
    string                      cluster_member_id         ();


    void                        wait                        ();
    void                        simple_wait_step            ();
    void                        wait                        ( Wait_handles*, const Time& wait_until, Object* wait_until_object, const Time& resume_at, Object* resume_at_object );

    void                        signal                      ( const string& signal_name );
    void                        async_signal                ( const char* signal_name = "" )    { _event.async_signal( signal_name ); }
    bool                        signaled                    ()                                  { return _event.signaled(); }

    void                        send_cmd                    ();

    // Prozesse
    void                        load_process_classes_from_dom( const xml::Element_ptr&, const Time& xml_mod_time );
    void                        add_process_class           ( Process_class* );
    Process_class*              process_class_or_null       ( const string& name );
    Process_class*              process_class               ( const string& name );
    xml::Element_ptr            process_classes_dom_element ( const xml::Document_ptr&, const Show_what& );
    Process*                    new_temporary_process       ();
    void                        init_process_classes        ();
    Process_class*              temporary_process_class     ()                                  { return *_process_class_list.begin(); }
    bool                        has_process_classes         ()                                  { return _process_class_list.size() > 1; }   // Die erste ist nur für temporäre Prozesse
    bool                        try_to_free_process         ( Job* for_job, Process_class*, const Time& now );

    void                        register_process_handle     ( Process_handle );                 // Für abort_immediately()
    void                        unregister_process_handle   ( Process_handle );                 // Für abort_immediately()
    void                        register_pid                ( int, bool is_process_group = false ); // Für abort_immediately()
    void                        unregister_pid              ( int );                            // Für abort_immediately()

    bool                        is_machine_suspendable      () const                            { return _dont_suspend_machine_counter == 0; }
    void                        begin_dont_suspend_machine  ();
    void                        end_dont_suspend_machine    ();
    void                        suspend_machine             ();

    Database*                   db                          ()                                  { return _db; }
    sql::Database_descriptor*   database_descriptor         ()                                  { return db()->database_descriptor(); }

    Scheduler_script_interface* scheduler_script            () const                            { return _scheduler_script; }

    Task_subsystem*             task_subsystem              ();
    Task_subsystem*             task_subsystem_or_null      ()                                  { return _task_subsystem; }
    Job_subsystem_interface*    job_subsystem               ();
    Job_subsystem_interface*    job_subsystem_or_null       ()                                  { return _job_subsystem; }
    Order_subsystem_interface*  order_subsystem             ();
    Java_subsystem_interface*   java_subsystem              ()                                  { return _java_subsystem; }
    bool                        has_any_order               ();
    bool                        has_any_task                ();

    void                        detect_warning_and_send_mail();

  private:
    Fill_zero                  _zero_;
    int                        _argc;
    char**                     _argv;
    string                     _parameter_line;

  public:
    Thread_semaphore           _lock;                       // Command_processor::execute_show_state() sperrt auch, für Zugriff auf _db.
    string                     _spooler_id;                 // -id=
    string                     _log_directory;              // -log-dir=
    bool                       _log_directory_as_option_set;// -log-dir= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _log_filename;
    bool                       _log_to_stderr;              // Zusätzlich nach stdout schreiben
    Log_level                  _log_to_stderr_level;
    File_path                  _include_path;
    bool                       _include_path_as_option_set; // -include-path= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _temp_dir;
    string                     _spooler_param;              // -param= Parameter für Skripten
    bool                       _spooler_param_as_option_set;// -param= als Option gesetzt, überschreibt Angabe in spooler.xml
    int                        _priority_max;               // <config priority_max=...>
    int                        _tcp_port;                   // <config tcp=...>
    bool                       _tcp_port_as_option_set;
    int                        _udp_port;                   // <config udp=...>
    bool                       _udp_port_as_option_set;
    bool                       _reuse_port;
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

    int                        _log_collect_within;
    int                        _log_collect_max;

    Time                       _last_mail_timestamp;

    string                     _variables_tablename;
    string                     _orders_tablename;
    string                     _clusters_tablename;
    string                     _tasks_tablename;
    string                     _job_history_tablename;
    string                     _job_history_columns;
    bool                       _job_history_yes;
    int                        _job_history_on_process;
    Archive_switch             _job_history_archive;
    With_log_switch            _job_history_with_log;

    string                     _order_history_tablename;
    bool                       _order_history_yes;
    With_log_switch            _order_history_with_log;
    Log_level                  _db_log_level;

    string                     _factory_ini;                // -ini=factory.ini
    string                     _sos_ini;                    // -sos.ini=sos.ini
    string                     _short_hostname;             // Ohne Netzwerk
    string                     _complete_hostname;          // Mit Netzwerk

    ptr<Com_spooler>           _com_spooler;                // COM-Objekt spooler
    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log

    string                     _db_name;
    ptr<Database>              _db;
    bool                       _need_db;                    // need_db=yes|strict  Wenn DB sich nicht öffnen lässt, ohne DB arbeiten und Historie ggfs. in Dateien schreiben
    bool                       _wait_endless_for_db_open;   // need_db=yes
    int                        _max_db_errors;              // Nach so vielen Fehlern im Scheduler-Leben DB abschalten (wie need_db)
    bool                       _db_check_integrity;

    int                        _waiting_errno;              // Scheduler unterbrochen wegen errno (spooler_log.cxx)
    string                     _waiting_errno_filename;

    bool                       _has_java;                   // Es gibt ein Java-Modul. Java muss also gestartet werden
    bool                       _has_java_source;            // Es gibt Java-Quell-Code. Wir brauchen ein Arbeitsverzeichnis.
    string                     _config_java_class_path;     // <config java_class_path="">
    string                     _config_java_options;        // <config java_config="">

    bool                       _interactive;                // Kommandos über stdin zulassen
    bool                       _manual;
    string                     _job_name;                   // Bei manuellem Betrieb

    string                     _send_cmd;                   // Der Spooler soll nur dem eigentlichen Spooler dieses Kommando schicken und sich dann beenden.
    string                     _xml_cmd;                    // Parameter -cmd, ein zuerst auszuführendes Kommando.
    string                     _pid_filename;

    ptr<Job_subsystem_interface>     _job_subsystem;
    ptr<Task_subsystem>              _task_subsystem;
    ptr<Order_subsystem_interface>   _order_subsystem;
    ptr<http::Http_server_interface> _http_server;
    ptr<Web_services_interface>      _web_services;
    ptr<Java_subsystem_interface>    _java_subsystem;

    Wait_handles               _wait_handles;

    Event                      _event;                      // Für Signale aus anderen Threads, mit Betriebssystem implementiert (nicht Unix)

    int                        _loop_counter;               // Zähler der Schleifendurchläufe in spooler.cxx
    int                        _wait_counter;               // Zähler der Aufrufe von wait_until()
    time_t                     _last_time_enter_pressed;    // int wegen Threads (Spooler_communication und Spooler_wait)
    Rotating_bar               _wait_rotating_bar;

    ptr<object_server::Connection_manager>  _connection_manager;
    bool                       _validate_xml;
    xml::Schema_ptr            _schema;
    string                     _config_filename;            // -config=
    bool                       _configuration_is_job_script;        // Als Konfigurationsdatei ist eine Skript-Datei angegeben worden
    string                     _configuration_job_script_language; 
    bool                       _executing_command;          // true: spooler_history wartet nicht auf Datenbank (damit Scheduler nicht blockiert)
    int                        _process_count;

    bool                       _subprocess_own_process_group_default;
    Process_handle             _process_handles[ max_processes ];   // Für abort_immediately(), mutex-frei alle abhängigen Prozesse
    struct Killpid { int _pid; bool _is_process_group; };
    Killpid                    _pids[ max_processes ];              // Für abort_immediately(), mutex-frei alle Task.add_pid(), Subprozesse der Tasks
    bool                       _are_all_tasks_killed;
  //Process_group_handle       _process_groups[ max_processes ];    // Für abort_immediately(), mutex-frei alle Task.add_pid(), Subprozesse der Tasks
//private:
    time::Holidays             _holidays;                   // Feiertage für alle Jobs

    State_changed_handler      _state_changed_handler;      // Callback für NT-Dienst SetServiceStatus()

    xml::Document_ptr          _config_document_to_load;    // Für cmd_load_config(), das Dokument zu _config_element_to_load
    xml::Element_ptr           _config_element_to_load;     // Für cmd_load_config()
    Time                       _config_element_mod_time;    // Modification time
    string                     _config_source_filename;     // Für cmd_load_config(), der Dateiname der Quelle

    xml::Document_ptr          _config_document;            // Das Dokument zu _config_element
    xml::Element_ptr           _config_element;             // Die gerade geladene Konfiguration (und Job hat einen Verweis auf <job>)

    xml::Document_ptr          _commands_document;          // Zusammengebautes <commands>

    ptr<Com_variable_set>      _variables;
    Security                   _security;                   // <security>
  //Object_set_class_list      _object_set_class_list;      // <object_set_classes>
    Communication              _communication;              // TCP und UDP (ein Thread)

    ptr<Supervisor_interface>  _supervisor;
    ptr<Supervisor_client_interface> _supervisor_client;
    ptr<Scheduler_event_manager> _scheduler_event_manager;

    ptr<Scheduler_script_interface> _scheduler_script;

    Process_class_list         _process_class_list;
    Process_list               _process_list;
    bool                       _ignore_process_classes;


    Time                       _last_wait_until;            // Für <show_state>
    Time                       _last_resume_at;             // Für <show_state>
    bool                       _print_time_every_second;

    Thread_id                  _thread_id;                  // Haupt-Thread
    Time                       _spooler_start_time;
    State                      _state;
    State_cmd                  _state_cmd;
    State_cmd                  _shutdown_cmd;               // run() beenden, also alle Tasks beenden!
    bool                       _shutdown_ignore_running_tasks;
    time_t                     _termination_gmtimeout_at;   // Für sc_terminate und sc_terminate_with_restart
    string                     _terminate_continue_exclusive_operation;
    bool                       _terminate_all_schedulers;
    bool                       _terminate_all_schedulers_with_restart;
    ptr<Async_operation>       _termination_async_operation;

    string                     _directory;
    File                       _pid_file;

    bool                       _zschimmer_mode;
    int                        _dont_suspend_machine_counter;   // >0: Kein suspend
    bool                       _suspend_after_resume;
    bool                       _should_suspend_machine;

    ptr<time::Daylight_saving_time_transition_detector_interface> _daylight_saving_time_transition_detector;
    Event                      _waitable_timer;
    bool                       _is_waitable_timer_set;

  //double                     _clock_difference;           // -now="..."   Zum Debuggen: Mit dieser Differenz zur tatsächlichen Uhrzeit arbeiten

    ptr<Com_variable_set>      _environment;
    Variable_set_map           _variable_set_map;           // _variable_set_map[""] = _environment; für <params>, Com_variable_set::set_dom()
    
    ptr<cluster::Cluster_subsystem_interface>      _cluster;
    cluster::Configuration     _cluster_configuration;
    bool                       _is_activated;
    bool                       _assert_is_active;
    int                        _is_in_check_is_active;
    bool                       _has_windows_console;
  //string                     _session_id;
    bool                       _check_memory_leak;
};

//-------------------------------------------------------------------------------------------------

void                            spooler_restart             ( Log* log, bool is_service );
void                            send_error_email            ( const exception&, int argc, char** argv, const string& parameter_line, Spooler* spooler = NULL );
//void                          send_error_email            ( const string& subject, const string& body );

//extern bool                     spooler_is_running;

//-------------------------------------------------------------------------------------------------

} //namespace scheduler

int                             spooler_main                ( int argc, char** argv, const string& parameters );

} //namespace sos

#ifdef Z_WINDOWS
extern "C" int  __declspec(dllexport) spooler_program       ( int argc, char** argv );
#endif

#endif
