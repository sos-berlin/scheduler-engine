// $Id: spooler.h,v 1.78 2002/04/09 08:55:43 jz Exp $

#ifndef __SPOOLER_H
#define __SPOOLER_H

#include "../kram/sysxcept.h"


#ifdef SYSTEM_WIN

#   import <msxml3.dll> rename_namespace("xml")

    namespace xml 
    {
        typedef IXMLDOMElement          Element;
        typedef IXMLDOMElementPtr       Element_ptr;
        typedef IXMLDOMTextPtr          Text_ptr;
        typedef IXMLDOMCDATASectionPtr  Cdata_section_ptr;
        typedef IXMLDOMNode             Node;
        typedef IXMLDOMNodePtr          Node_ptr;
        typedef IXMLDOMNodeList         NodeList;
        typedef IXMLDOMNodeListPtr      NodeList_ptr;
        typedef IXMLDOMDocument         Document;
        typedef IXMLDOMDocumentPtr      Document_ptr;
        typedef IXMLDOMDocumentTypePtr  DocumentType_ptr;
    }

#endif

#include <stdio.h>
#include <process.h>    // _beginthreadex()
        
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


namespace sos {
    namespace spooler {
        using namespace std;
        struct Spooler;
        struct Thread;
        struct Job;
        struct Task;
    }
}

#include "spooler_com.h"
#include "spooler_common.h"
#include "spooler_time.h"
#include "spooler_log.h"
#include "spooler_wait.h"
#include "spooler_communication.h"
#include "spooler_security.h"
#include "spooler_command.h"
#include "spooler_script.h"
#include "spooler_history.h"
#include "spooler_task.h"
#include "spooler_thread.h"
#include "spooler_service.h"

namespace sos {


typedef _bstr_t                 Dom_string;

template<typename T>
inline Dom_string               as_dom_string               ( const T& t )                          { return as_bstr_t( t ); }

string                          program_filename            ();
string                          directory_of_path           ( const string& );
string                          basename_of_path            ( const string& );
string                          extension_of_path           ( const string& );

namespace spooler {


string                          text_from_xml_with_include  ( const xml::Element_ptr&, const string& include_path );
int                             read_profile_on_process     ( const string& profile, const string& section, const string& entry, int deflt );
Archive_switch                  read_profile_archive        ( const string& profile, const string& section, const string& entry, Archive_switch deflt );

//----------------------------------------------------------------------------State_changed_handler

typedef void (*State_changed_handler)( Spooler*, void* );

typedef map<Thread_id,Thread*>      Thread_id_map;

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
    xml::Element_ptr            threads_as_xml              ( xml::Document_ptr, bool show_all );

    int                         launch                      ( int argc, char** argv );                                
    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    Thread_id                   thread_id                   () const                            { return _thread_id; }

    // Für andere Threads:
    Thread*                     get_thread                  ( const string& thread_name );
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
    void                        cmd_load_config             ( const xml::Element_ptr& );


    friend struct               Com_spooler;

    void                        load_arg                    ();
    void                        load                        ();
    void                        load_config                 ( const xml::Element_ptr& config );

    void                        load_object_set_classes_from_xml( Object_set_class_list*, const xml::Element_ptr& );
    void                        load_threads_from_xml       ( Thread_list*, const xml::Element_ptr& );

    void                        set_state                   ( State );

    void                        start                       ();
    void                        stop                        ();
    void                        signal_threads              ( const string& signal_name );
    void                        wait_until_threads_stopped  ( Time until );
    void                        reload                      ();
    void                        run                         ();

  //void                        single_thread_step          ();
    void                        wait                        ();

    void                        signal                      ( const string& signal_name = "" )  { _log.info( "Signal " + signal_name ); _event.signal( signal_name ); }

    Thread*                     thread_by_thread_id         ( Thread_id );

    Fill_zero                  _zero_;

    bool                       _is_service;                 // NT-Dienst
    int                        _argc;
    char**                     _argv;

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
    int                        _history_on_process;
    Archive_switch             _history_archive;
    bool                       _history_with_log;
    string                     _history_tablename;
    string                     _variables_tablename;

    string                     _factory_ini;                // -ini=factory.ini
    string                     _hostname;

    Log                        _log;
    Prefix_log                 _prefix_log;

    CComPtr<Com_spooler>       _com_spooler;                // COM-Objekt spooler
    CComPtr<Com_log>           _com_log;                    // COM-Objekt spooler.log

    Thread_id_map              _thread_id_map;              // Thread_id -> Thread
    Thread_semaphore           _thread_id_map_lock;

    Thread_semaphore           _job_name_lock;              // Sperre von get_job(name) bis add_job() für eindeutige Jobnamen
    Thread_semaphore           _serialize_lock;             // Wenn die Threads nicht nebenläufig sein sollen

    string                     _db_name;
    Spooler_db                 _db;

  private:
    string                     _config_filename;            // -config=
    string                     _spooler_id;                 // -id=
    string                     _log_directory;              // -log-dir=
    bool                       _log_directory_as_option_set;// -log-dir= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _log_filename;
    string                     _include_path;
    bool                       _include_path_as_option_set; // -include-path= als Option gesetzt, überschreibt Angabe in spooler.xml
    string                     _spooler_param;              // -param= Parameter für Skripten
    bool                       _spooler_param_as_option_set;// -param= als Option gesetzt, überschreibt Angabe in spooler.xml
    int                        _priority_max;               // <config priority_max=...>
    int                        _tcp_port;                   // <config tcp=...>
    int                        _udp_port;                   // <config udp=...>
    time::Holiday_set          _holiday_set;                // Feiertage für alle Jobs
    State_changed_handler      _state_changed_handler;      // Callback für NT-Dienst SetServiceStatus()

    xml::Element_ptr           _config_element_to_load;     // Für cmd_load_config()
    xml::Document_ptr          _config_document_to_load;    // Für cmd_load_config(), das Dokument zu _config_element_to_load

    xml::Element_ptr           _config_element;             // Die gerade geladene Konfiguration (und Job hat einen Verweis auf <job>)
    xml::Document_ptr          _config_document;            // Das Dokument zu _config_element

    Security                   _security;                   // <security>
    Object_set_class_list      _object_set_class_list;      // <object_set_classes>
    Communication              _communication;              // TCP und UDP (ein Thread)

    CComPtr<Com_variable_set>  _variables;
    Script                     _script;                     // <script>
    Script_instance            _script_instance;

    Thread_list                _thread_list;

    Thread_id                  _thread_id;                  // Haupt-Thread
    Time                       _spooler_start_time;
    State                      _state;
    State_cmd                  _state_cmd;

    Wait_handles               _wait_handles;
    Event                      _event;                      

    Thread_semaphore           _lock;
    bool                       _free_threading_default;
};

//-------------------------------------------------------------------------------------------------

void                            spooler_restart             ( bool is_service );

extern bool                     spooler_is_running;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif