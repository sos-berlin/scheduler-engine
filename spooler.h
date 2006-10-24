// $Id$
// §1172

#ifndef __SPOOLER_H
#define __SPOOLER_H

#include "../zschimmer/zschimmer.h"
#include "../kram/sos.h"
#include "../kram/sysxcept.h"
#include "../kram/sosopt.h"

#ifdef Z_HPUX
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
    using namespace zschimmer::xml_libxml2;
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

using namespace zschimmer;
using namespace zschimmer::com;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const
    
extern const char*              temporary_process_class_name;
extern const char               dtd_string[];
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

using namespace std;
struct Communication;
struct Spooler;
struct Spooler_thread;
struct Job;
struct Task;
struct Job_chain;
struct Order_queue;
struct Order;
struct Process_class;
struct Remote_scheduler;
struct Show_what;
struct Subprocess;
struct Subprocess_register;
struct Web_service;
struct Web_service_operation;
struct Web_service_request;
struct Web_service_response;
struct Xslt_stylesheet;

} //namespace spooler

namespace http
{
} //namespace http
} //namespace sos

//-------------------------------------------------------------------------------------------------

#include "spooler_com.h"
#include "spooler_xslt_stylesheet.h"
#include "spooler_common.h"
#include "spooler_time.h"
#include "spooler_mail.h"
#include "spooler_event.h"
#include "spooler_log.h"
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
#include "spooler_remote.h"
#include "spooler_job.h"
#include "spooler_subprocess.h"
#include "spooler_task.h"
#include "spooler_process.h"
#include "spooler_thread.h"
#include "spooler_service.h"
#include "spooler_web_service.h"
#include "spooler_module_remote.h"
#include "spooler_module_remote_server.h"

//-------------------------------------------------------------------------------------------------

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

extern volatile int             ctrl_c_pressed;
extern Spooler*                 spooler_ptr;

//-------------------------------------------------------------------------------------------------

Source_with_parts               text_from_xml_with_include  ( const xml::Element_ptr&, const Time& xml_mod_time, const string& include_path );
int                             read_profile_mail_on_process( const string& profile, const string& section, const string& entry, int deflt );
int                             read_profile_history_on_process( const string& prof, const string& section, const string& entry, int deflt );
Archive_switch                  read_profile_archive        ( const string& profile, const string& section, const string& entry, Archive_switch deflt );
With_log_switch                 read_profile_with_log       ( const string& profile, const string& section, const string& entry, Archive_switch deflt );

//----------------------------------------------------------------------------State_changed_handler

typedef void (*State_changed_handler)( Spooler*, void* );

typedef map<Thread_id,Spooler_thread*>      Thread_id_map;

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


    // Aufrufe für andere Threads:
    Thread_id                   thread_id                   () const                            { return _thread_id; }
    const string&               id                          () const                            { return _spooler_id; }
    string                      id_for_db                   () const                            { return _spooler_id.empty()? "-" : _spooler_id; }
    string                      name                        () const;                           // "Scheduler -id=... host:port"
    const string&               param                       () const                            { return _spooler_param; }
    int                         udp_port                    () const                            { return _udp_port; }
    int                         tcp_port                    () const                            { return _tcp_port; }
    string                      include_path                () const                            { return _include_path; }
    string                      temp_dir                    () const                            { return _temp_dir; }
    int                         priority_max                () const                            { return _priority_max; }
    State                       state                       () const                            { return _state; }
    string                      state_name                  () const                            { return state_name( _state ); }
    static string               state_name                  ( State );
  //bool                        free_threading_default      () const                            { return _free_threading_default; }
    Prefix_log*                 log                         ()                                  { return &_log; }
    const string&               log_directory               () const                            { return _log_directory; }                      
    Time                        start_time                  () const                            { return _spooler_start_time; }
    Security::Level             security_level              ( const Ip_address& );
    const time::Holiday_set&    holidays                    () const                            { return _holiday_set; }
    bool                        is_service                  () const                            { return _is_service; }
    string                      directory                   () const                            { return _directory; }

  //xml::Element_ptr            threads_as_xml              ( const xml::Document_ptr&, const Show_what& );

    int                         launch                      ( int argc, char** argv, const string& params );                                
    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    Thread_id                   run_single_thread                   () const                            { return _thread_id; }

    // Für andere Threads:
    Spooler_thread*             get_thread                  ( const string& thread_name );
    Spooler_thread*             get_thread_or_null          ( const string& thread_name );
    Spooler_thread*             select_thread_for_task      ( Task* );

  //Object_set_class*           get_object_set_class        ( const string& name );
    Object_set_class*           get_object_set_class_or_null( const string& name );

    bool                        has_any_order               ();

    void                        signal_object               ( const string& object_set_class_name, const Level& );
    void                        cmd_reload                  ();
    void                        cmd_pause                   ()                                  { _state_cmd = sc_pause; signal( "pause" ); }
    void                        cmd_continue                ();
  //void                        cmd_stop                    ();
    void                        cmd_terminate               ( int timeout = INT_MAX );
    void                        cmd_terminate_and_restart   ( int timeout = INT_MAX );
    void                        cmd_let_run_terminate_and_restart();

    void                        abort_immediately           ( bool restart = false );
    void                        abort_now                   ( bool restart = false );
    void                        kill_all_processes          ();

    void                        cmd_load_config             ( const xml::Element_ptr&, const Time& xml_mod_time, const string& source_filename );
    void                        execute_state_cmd           ();

    Job*                        get_job                     ( const string& job_name, bool can_be_not_initialized = false );
    Job*                        get_job_or_null             ( const string& job_name );
  //Job*                        get_next_job_to_start       ();
    ptr<Task>                   get_task                    ( int task_id );
    ptr<Task>                   get_task_or_null            ( int task_id );

    friend struct               Com_spooler;

    void                        load_arg                    ();
    void                        load                        ();
    void                        load_config                 ( const xml::Element_ptr& config, const Time& xml_mod_time, const string& source_filename );
    void                        set_next_daylight_saving_transition();

    void                        load_object_set_classes_from_xml( Object_set_class_list*, const xml::Element_ptr&, const Time& xml_mod_time );

    xml::Element_ptr            state_dom_element           ( const xml::Document_ptr&, const Show_what& = show_standard );
    void                        set_state                   ( State );
  //void                        connect_to_main_scheduler   ();

    void                        create_window               ();
    void                        start                       ();
    void                        run_scheduler_script        ();
    void                        run_check_ctrl_c            ();
    void                        stop                        ( const exception* = NULL );
  //void                        signal_threads              ( const string& signal_name );
  //void                        wait_until_threads_stopped  ( Time until );
    void                        reload                      ();
    void                        end_waiting_tasks           ();
    void                        nichts_getan                ( int anzahl, const string& );
    void                        run                         ();
    bool                        run_continue                ();
  //void                        start_threads               ();
    Spooler_thread*             new_thread                  ( bool free_threading = true );
  //void                        close_threads               ();
  //bool                        run_single_thread           ();

  //void                        single_thread_step          ();
    void                        wait                        ();

    void                        signal                      ( const string& signal_name )       { if( _log.log_level() <= log_debug9 )  _log.debug9( "Signal \"" + signal_name + "\"" ); _event.signal( signal_name ); }
    void                        async_signal                ( const char* signal_name = "" )    { _event.async_signal( signal_name ); }
    bool                        signaled                    ()                                  { return _event.signaled(); }

    Spooler_thread*             thread_by_thread_id         ( Thread_id );
  //void                        send_error_email            ( const string& subject, const string& body );

    void                        send_cmd                    ();

    // Jobs
    void                        add_job                     ( const ptr<Job>&, bool init );
    void                        cmd_add_jobs                ( const xml::Element_ptr& );
    void                        cmd_job                     ( const xml::Element_ptr& );
  //void                        do_add_jobs                 ();
    int                         remove_temporary_jobs       ( Job* which_job = NULL );
    void                        remove_job                  ( Job* );
    void                        init_jobs                   ();
    void                        close_jobs                  ();

    // Order
    void                        load_jobs_from_xml          ( const xml::Element_ptr&, const Time& xml_mod_time, bool init = false );
    void                        load_job_from_xml           ( const xml::Element_ptr&, const Time& xml_mod_time, bool init = false );
    void                        init_job                    ( Job*, bool call_init_too = true );
    xml::Element_ptr            jobs_dom_element            ( const xml::Document_ptr&, const Show_what& );

    void                        init_job_chains             ();                                 // In spooler_order.cxx
    void                        init_file_order_sink        ();                                 // In spooler_order_file.cxx
    void                        load_job_chains_from_xml    ( const xml::Element_ptr& );
    void                        add_job_chain               ( Job_chain* );
    Job_chain*                  job_chain                   ( const string& name );
    Job_chain*                  job_chain_or_null           ( const string& name );
    xml::Element_ptr            job_chains_dom_element      ( const xml::Document_ptr&, const Show_what& );

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

  private:
    Fill_zero                  _zero_;
    int                        _argc;
    char**                     _argv;
    string                     _parameter_line;
    bool                       _jobs_initialized;

  public:
    Thread_semaphore           _lock;                       // Command_processor::execute_show_state() sperrt auch, für Zugriff auf _db.
    string                     _spooler_id;                 // -id=
    string                     _log_directory;              // -log-dir=
    bool                       _log_directory_as_option_set;// -log-dir= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _log_filename;
    string                     _include_path;
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
    string                     _version;
    Log                        _base_log;
    Prefix_log                 _log;
    int                        _pid;
    string                     _my_program_filename;
    bool                       _is_service;                 // NT-Dienst
    bool                       _debug;
    Log_level                  _log_level;
    bool                       _mail_on_warning;            // Für Job-Protokolle
    bool                       _mail_on_error;              // Für Job-Protokolle
    int                        _mail_on_process;            // Für Job-Protokolle
    bool                       _mail_on_success;            // Für Job-Protokolle
    string                     _mail_encoding;

    Mail_defaults              _mail_defaults;

    int                        _log_collect_within;
    int                        _log_collect_max;

    Time                       _last_mail_timestamp;

    string                     _variables_tablename;
    string                     _orders_tablename;

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


    string                     _factory_ini;                // -ini=factory.ini
    string                     _sos_ini;                    // -sos.ini=sos.ini
    string                     _hostname;

    ptr<Com_spooler>           _com_spooler;                // COM-Objekt spooler
    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log

    Thread_id_map              _thread_id_map;              // Thread_id -> Spooler_thread
    Thread_semaphore           _thread_id_map_lock;

    Thread_semaphore           _job_name_lock;              // Sperre von get_job(name) bis add_job() für eindeutige Jobnamen
    Thread_semaphore           _serialize_lock;             // Wenn die Threads nicht nebenläufig sein sollen

    string                     _db_name;
    ptr<Spooler_db>            _db;
    bool                       _need_db;                    // need_db=yes|strict  Wenn DB sich nicht öffnen lässt, ohne DB arbeiten und Historie ggfs. in Dateien schreiben
    bool                       _wait_endless_for_db_open;   // need_db=yes
    int                        _max_db_errors;              // Nach so vielen Fehlern im Scheduler-Leben DB abschalten (wie need_db)

    int                        _waiting_errno;              // Scheduler unterbrochen wegen errno (spooler_log.cxx)
    string                     _waiting_errno_filename;
  //bool                       _waiting_errno_continue;     // Nach Fehler fortsetzen

    bool                       _has_java;                   // Es gibt ein Java-Modul. Java muss also gestartet werden
    bool                       _has_java_source;            // Es gibt Java-Quell-Code. Wir brauchen ein Arbeitsverzeichnis.
    ptr<java::Vm>              _java_vm;
  //string                     _java_work_dir;              // Zum Compilieren, für .class
    string                     _config_java_class_path;     // <config java_class_path="">
    string                     _config_java_options;        // <config java_config="">

    bool                       _interactive;                // Kommandos über stdin zulassen
    bool                       _manual;
    string                     _job_name;                   // Bei manuellem Betrieb

    string                     _send_cmd;                   // Der Spooler soll nur dem eigentlichen Spooler dieses Kommando schicken und sich dann beenden.
    string                     _xml_cmd;                    // Parameter -cmd, ein zuerst auszuführendes Kommando.
    string                     _pid_filename;

    Web_services               _web_services;
    Job_list                   _job_list;
    Wait_handles               _wait_handles;

    Event                      _event;                      // Für Signale aus anderen Threads, mit Betriebssystem implementiert (nicht Unix)
  //Simple_event               _simple_event;               // Für Signale aus demselben Thread, ohne Betriebssystem implementiert

    int                        _loop_counter;               // Zähler der Schleifendurchläufe in spooler.cxx
    int                        _wait_counter;               // Zähler der Aufrufe von wait_until()
    time_t                     _last_time_enter_pressed;    // int wegen Threads (Spooler_communication und Spooler_wait)

    ptr<object_server::Connection_manager>  _connection_manager;
  //ptr<Async_manager>                      _async_manager;

  //xml::Dtd_ptr               _dtd;
    bool                       _validate_xml;
    xml::Schema_ptr            _schema;
    string                     _config_filename;            // -config=
    bool                       _configuration_is_job_script;        // Als Konfigurationsdatei ist eine Skript-Datei angegeben worden
    string                     _configuration_job_script_language; 
    string                     _html_directory;
    bool                       _executing_command;          // true: spooler_history wartet nicht auf Datenbank (damit Scheduler nicht blockiert)
    int                        _process_count;

    bool                       _subprocess_own_process_group_default;
    Process_handle             _process_handles[ max_processes ];   // Für abort_immediately(), mutex-frei alle abhängigen Prozesse
    struct Killpid { int _pid; bool _is_process_group; };
    Killpid                    _pids[ max_processes ];              // Für abort_immediately(), mutex-frei alle Task.add_pid(), Subprozesse der Tasks
  //Process_group_handle       _process_groups[ max_processes ];    // Für abort_immediately(), mutex-frei alle Task.add_pid(), Subprozesse der Tasks
//private:
  //bool                       _free_threading_default;
    time::Holiday_set          _holiday_set;                // Feiertage für alle Jobs

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
    Object_set_class_list      _object_set_class_list;      // <object_set_classes>
    Communication              _communication;              // TCP und UDP (ein Thread)

    Remote_scheduler_register  _remote_scheduler_register;
    ptr<Xml_client_connection> _main_scheduler_connection;

    Module                     _module;                     // <script>
    ptr<Module_instance>       _module_instance;

    Process_class_list         _process_class_list;
    Process_list               _process_list;
  //int                        _process_count_max;
    bool                       _ignore_process_classes;


    //Time                       _next_start_time;
    Time                       _last_wait_until;            // Für <show_state>
    Time                       _last_resume_at;             // Für <show_state>
    bool                       _print_time_every_second;

    Thread_list                _thread_list;                // Alle Threads
    int                        _max_threads;
    Spooler_thread*            _single_thread;              // Es gibt nur einen Thread! Threads sind eigentlich ein Überbleibsel aus alter Zeit


  //typedef list< Spooler_thread* >  Spooler_thread_list;
  //Spooler_thread_list         _spooler_thread_list;        // Nur Threads mit _free_threading=no, die also keine richtigen Threads sind und im Spooler-Thread laufen


    Thread_semaphore           _job_chain_lock;
    typedef map< string, ptr<Job_chain> >  Job_chain_map;
    Job_chain_map              _job_chain_map;
    int                        _job_chain_map_version;             // Zeitstempel der letzten Änderung (letzter Aufruf von Spooler::add_job_chain()), 
    long32                     _next_free_order_id;

    Thread_id                  _thread_id;                  // Haupt-Thread
    Time                       _spooler_start_time;
    State                      _state;
    State_cmd                  _state_cmd;
    State_cmd                  _shutdown_cmd;               // run() beenden, also alle Tasks beenden!
    bool                       _shutdown_ignore_running_tasks;
    time_t                     _termination_gmtimeout_at;   // Für sc_terminate und sc_terminate_with_restart
    ptr<Async_operation>       _termination_async_operation;

    string                     _directory;
    File                       _pid_file;

    bool                       _zschimmer_mode;
    int                        _dont_suspend_machine_counter;   // >0: Kein suspend
    bool                       _suspend_after_resume;
    bool                       _should_suspend_machine;

    Time                       _next_daylight_saving_transition_time;
    string                     _next_daylight_saving_transition_name;
    Event                      _waitable_timer;
    bool                       _is_waitable_timer_set;

  //double                     _clock_difference;           // -now="..."   Zum Debuggen: Mit dieser Differenz zur tatsächlichen Uhrzeit arbeiten

    ptr<Com_variable_set>      _environment;
    Variable_set_map           _variable_set_map;           // _variable_set_map[""] = _environment; für <params>, Com_variable_set::set_dom()
};

//-------------------------------------------------------------------------------------------------

void                            spooler_restart             ( Log* log, bool is_service );
void                            send_error_email            ( const exception&, int argc, char** argv, const string& parameter_line, Spooler* spooler = NULL );
//void                          send_error_email            ( const string& subject, const string& body );

//extern bool                     spooler_is_running;

//-------------------------------------------------------------------------------------------------

} //namespace spooler

int                             spooler_main                ( int argc, char** argv, const string& parameters );

} //namespace sos

#ifdef Z_WINDOWS
extern "C" int  __declspec(dllexport) spooler_program       ( int argc, char** argv );
#endif

#endif
