// $Id: spooler.h,v 1.126 2003/03/17 18:40:18 jz Exp $

#ifndef __SPOOLER_H
#define __SPOOLER_H

#include "../kram/sos.h"
#include "../kram/sysxcept.h"


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
    using namespace zschimmer::xml_libxml2;
#endif

#ifndef Z_WINDOWS
    const int _dstbias = 3600;
#endif

#include <stdio.h>

#ifdef Z_WINDOWS
#   include "../zschimmer/z_windows.h"
#endif
        
#include <set>
#include <map>
#include <list>
#include <time.h>

#include "../kram/sosdate.h"
#include "../kram/sosprof.h"
#include "../kram/sossock1.h"
#include "../kram/thread_semaphore.h"
#include "../kram/com_simple_standards.h"
#include "../kram/log.h"

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/file.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/threads.h"

using namespace zschimmer;
using namespace zschimmer::com;

namespace sos {
    namespace spooler {
        using namespace std;
        struct Spooler;
        struct Spooler_thread;
        struct Job;
        struct Task;
        struct Job_chain;
        struct Order_queue;
        struct Order;
    }
}

#include "spooler_com.h"
#include "spooler_common.h"
#include "spooler_time.h"
#include "spooler_mail.h"
#include "spooler_log.h"
#include "spooler_wait.h"
#include "spooler_communication.h"
#include "spooler_security.h"
#include "spooler_command.h"
#include "spooler_module.h"
#include "spooler_module_com.h"
#include "spooler_module_java.h"
#include "spooler_history.h"
#include "spooler_order.h"
#include "spooler_task.h"
#include "spooler_thread.h"
#include "spooler_service.h"


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

struct Spooler
{
    enum State
    {
        s_none,
        s_stopped,
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
        sc_stop,                // s_running | s_paused -> s_stopped
        sc_terminate,           // s_running | s_paused -> s_stopped, exit()
        sc_terminate_and_restart,
        sc_let_run_terminate_and_restart,
        sc_load_config,         
        sc_reload,
        sc_pause,               // s_running -> s_paused
        sc_continue,            // s_paused -> s_running
        sc__max
    };

                                Spooler                     ();
                               ~Spooler                     ();

    // Aufrufe für andere Threads:
    const string&               id                          () const                            { return _spooler_id; }
    const string&               param                       () const                            { return _spooler_param; }
    int                         udp_port                    () const                            { return _udp_port; }
    int                         tcp_port                    () const                            { return _tcp_port; }
    string                      include_path                () const                            { return _include_path; }
    string                      temp_dir                    () const                            { return _temp_dir; }
    int                         priority_max                () const                            { return _priority_max; }
    State                       state                       () const                            { return _state; }
    string                      state_name                  () const                            { return state_name( _state ); }
    static string               state_name                  ( State );
    bool                        free_threading_default      () const                            { return _free_threading_default; }
    Log&                        log                         ()                                  { return _log; }
    const string&               log_directory               () const                            { return _log_directory; }                      
    Time                        start_time                  () const                            { return _spooler_start_time; }
    Security::Level             security_level              ( const Host& );
    const time::Holiday_set&    holidays                    () const                            { return _holiday_set; }
    bool                        is_service                  () const                            { return _is_service; }
    xml::Element_ptr            threads_as_xml              ( const xml::Document_ptr&, Show_what );

    int                         launch                      ( int argc, char** argv );                                
    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    Thread_id                   thread_id                   () const                            { return _thread_id; }

    // Für andere Threads:
    Spooler_thread*             get_thread                  ( const string& thread_name );
    Spooler_thread*             get_thread_or_null          ( const string& thread_name );
    Object_set_class*           get_object_set_class        ( const string& name );
    Object_set_class*           get_object_set_class_or_null( const string& name );
    Job*                        get_job                     ( const string& job_name );
    Job*                        get_job_or_null             ( const string& job_name );

    void                        signal_object               ( const string& object_set_class_name, const Level& );
    void                        cmd_reload                  ();
    void                        cmd_pause                   ()                                  { _state_cmd = sc_pause; signal( "pause" ); }
    void                        cmd_continue                ();
    void                        cmd_stop                    ();
    void                        cmd_terminate               ();
    void                        cmd_terminate_and_restart   ();
    void                        cmd_let_run_terminate_and_restart();
    void                        cmd_load_config             ( const xml::Element_ptr&, const Time& xml_mod_time, const string& source_filename );

    // Order
    long                        get_free_order_id           ()                                  { return InterlockedIncrement( &_next_free_order_id ); }
    void                        add_job_chain               ( Job_chain* );
    Job_chain*                  job_chain                   ( const string& name );
    xml::Element_ptr            xml_from_job_chains         ( const xml::Document_ptr&, Show_what );
    void                        set_job_chain_time          ( const Time& t )                   { THREAD_LOCK( _job_chain_lock )  _job_chain_time = t; }
    Time                        job_chain_time              ()                                  { THREAD_LOCK_RETURN( _job_chain_lock, Time, _job_chain_time ); }

    friend struct               Com_spooler;

    void                        load_arg                    ();
    void                        load                        ();
    void                        load_config                 ( const xml::Element_ptr& config, const Time& xml_mod_time, const string& source_filename );

    void                        load_object_set_classes_from_xml( Object_set_class_list*, const xml::Element_ptr&, const Time& xml_mod_time );
    void                        load_threads_from_xml       ( const xml::Element_ptr&, const Time& xml_mod_time );

    void                        set_state                   ( State );

    void                        init_java_vm                ();

    void                        start                       ();
    void                        stop                        ();
    void                        signal_threads              ( const string& signal_name );
    void                        wait_until_threads_stopped  ( Time until );
    void                        reload                      ();
    void                        run                         ();
    void                        start_threads               ();
    void                        close_threads               ();
    bool                        run_threads                 ();

  //void                        single_thread_step          ();
    void                        wait                        ();

    void                        signal                      ( const string& signal_name = "" )  { _log.info( "Signal \"" + signal_name + "\"" ); _event.signal( signal_name ); }
    void                        async_signal                ( const char* signal_name = "" )    { _event.async_signal( signal_name ); }
    bool                        signaled                    ()                                  { return _event.signaled(); }

    Spooler_thread*             thread_by_thread_id         ( Thread_id );


  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    int                        _argc;
    char**                     _argv;

  public:
    Log                        _log;
    Prefix_log                 _prefix_log;
    int                        _pid;
    bool                       _is_service;                 // NT-Dienst
    bool                       _debug;
    int                        _log_level;
    bool                       _mail_on_error;              // Für Job-Protokolle
    int                        _mail_on_process;            // Für Job-Protokolle
    bool                       _mail_on_success;            // Für Job-Protokolle
    string                     _mail_queue_dir;
    string                     _mail_encoding;
    string                     _smtp_server;                // Für Job-Protokolle
    string                     _log_mail_from;
    string                     _log_mail_to;
    string                     _log_mail_cc;
    string                     _log_mail_bcc;
    string                     _log_mail_subject;
    int                        _log_collect_within;
    int                        _log_collect_max;

    string                     _history_columns;
    bool                       _history_yes;
    int                        _history_on_process;
    Archive_switch             _history_archive;
    With_log_switch            _history_with_log;
    string                     _history_tablename;
    string                     _variables_tablename;

    string                     _factory_ini;                // -ini=factory.ini
    string                     _hostname;

    ptr<Com_spooler>           _com_spooler;                // COM-Objekt spooler
    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log

    Thread_id_map              _thread_id_map;              // Thread_id -> Spooler_thread
    Thread_semaphore           _thread_id_map_lock;

    Thread_semaphore           _job_name_lock;              // Sperre von get_job(name) bis add_job() für eindeutige Jobnamen
    Thread_semaphore           _serialize_lock;             // Wenn die Threads nicht nebenläufig sein sollen

    string                     _db_name;
    Spooler_db                 _db;
    bool                       _need_db;

    bool                       _has_java;                   // Es gibt ein Java-Skript
    ptr<java::Vm>              _java_vm;
    string                     _java_work_dir;              // Zum Compilieren, für .class

    bool                       _manual;
    string                     _job_name;                   // Bei manuellem Betrieb

  private:
    string                     _config_filename;            // -config=
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
    int                        _udp_port;                   // <config udp=...>
    bool                       _free_threading_default;
    time::Holiday_set          _holiday_set;                // Feiertage für alle Jobs

    State_changed_handler      _state_changed_handler;      // Callback für NT-Dienst SetServiceStatus()

    Event                      _event;
    Wait_handles               _wait_handles;

    xml::Document_ptr          _config_document_to_load;    // Für cmd_load_config(), das Dokument zu _config_element_to_load
    xml::Element_ptr           _config_element_to_load;     // Für cmd_load_config()
    Time                       _config_element_mod_time;    // Modification time
    string                     _config_source_filename;     // Für cmd_load_config(), der Dateiname der Quelle

    xml::Document_ptr          _config_document;            // Das Dokument zu _config_element
    xml::Element_ptr           _config_element;             // Die gerade geladene Konfiguration (und Job hat einen Verweis auf <job>)

    ptr<Com_variable_set>      _variables;
    Security                   _security;                   // <security>
    Object_set_class_list      _object_set_class_list;      // <object_set_classes>
    Communication              _communication;              // TCP und UDP (ein Thread)

    Module                     _module;                     // <script>
    ptr<Module_instance>       _module_instance;

    Thread_list                _thread_list;                // Alle Threads

    typedef list< Spooler_thread* >  Spooler_thread_list;
    Spooler_thread_list         _spooler_thread_list;        // Nur Threads mit _free_threading=no, die also keine richtigen Threads sind und im Spooler-Thread laufen

    Time                       _next_time;
    Job*                       _next_job;

    Thread_semaphore           _job_chain_lock;
    typedef map< string, ptr<Job_chain> >  Job_chain_map;
    Job_chain_map              _job_chain_map;
    Time                       _job_chain_time;             // Zeitstempel der letzten Änderung (letzer Aufruf von Spooler::add_job_chain()), 
    long                       _next_free_order_id;

    Thread_id                  _thread_id;                  // Haupt-Thread
    Time                       _spooler_start_time;
    State                      _state;
    State_cmd                  _state_cmd;
};

//-------------------------------------------------------------------------------------------------

void                            spooler_restart             ( Log* log, bool is_service );
void                            send_error_email            ( const string& error_text, int argc, char** argv );

//extern bool                     spooler_is_running;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
