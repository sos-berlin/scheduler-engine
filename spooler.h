// $Id: spooler.h,v 1.42 2001/01/30 13:32:36 jz Exp $

#ifndef __SPOOLER_H
#define __SPOOLER_H

#include "../kram/sysxcept.h"


#ifdef SYSTEM_WIN

#   import <msxml3.dll> rename_namespace("xml")

    namespace xml {
        typedef IXMLDOMElement     Element;
        typedef IXMLDOMElementPtr  Element_ptr;
        typedef IXMLDOMNode        Node;
        typedef IXMLDOMNodePtr     Node_ptr;
        typedef IXMLDOMNodeList    NodeList;
        typedef IXMLDOMNodeListPtr NodeList_ptr;
        typedef IXMLDOMDocument    Document;
        typedef IXMLDOMDocumentPtr Document_ptr;
    }

#endif

#include <stdio.h>
#include <process.h>    // _beginthreadex()
        
#include <set>
#include <map>
#include <list>
#include <time.h>

#include "../kram/sosdate.h"
#include "../kram/sossock1.h"
#include "../kram/thread_semaphore.h"
#include "../kram/log.h"

namespace sos {
    namespace spooler {
        using namespace std;
    }
}

#include "spooler_com.h"
#include "spooler_common.h"
#include "spooler_log.h"
#include "spooler_time.h"
#include "spooler_wait.h"
#include "spooler_communication.h"
#include "spooler_security.h"
#include "spooler_command.h"
#include "spooler_script.h"
#include "spooler_task.h"
#include "spooler_service.h"

#define FOR_EACH( TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )
#define FOR_EACH_CONST( TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )

namespace sos {


typedef _bstr_t                 Dom_string;

template<typename T>
inline Dom_string               as_dom_string               ( const T& t )                          { return as_bstr_t( t ); }


namespace spooler {


struct                          Spooler;
struct                          Task;


//----------------------------------------------------------------------------State_changed_handler

typedef void (*State_changed_handler)( Spooler*, void* );

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
        s__max
    };

    enum State_cmd
    {
        sc_none,
        sc_stop,                // s_running | s_paused -> s_stopped
        sc_terminate,           // s_running | s_paused -> s_stopped, exit()
        sc_terminate_and_restart,
        sc_load_config,         
        sc_reload,
        sc_pause,               // s_running -> s_paused
        sc_continue,            // s_paused -> s_running
        sc__max
    };


                                Spooler                     ();
                               ~Spooler                     ();

    int                         launch                      ( int argc, char** argv );                                
    void                        load_arg                    ();
    void                        load                        ();
    void                        load_config                 ( const xml::Element_ptr& config );

    void                        load_object_set_classes_from_xml( Object_set_class_list*, const xml::Element_ptr& );
    void                        load_jobs_from_xml          ( Job_list*, const xml::Element_ptr& );

    void                        start                       ();
    void                        stop                        ();
    void                        reload                      ();
    void                        run                         ();
    void                        restart                     ();

    void                        single_thread_step          ();
    void                        wait                        ();
    void                        remove_ended_tasks          ();


    void                        cmd_reload                  ();
    void                        cmd_pause                   ()                                  { _state_cmd = sc_pause; cmd_wake(); }
    void                        cmd_continue                ();
    void                        cmd_stop                    ();
    void                        cmd_terminate               ();
    void                        cmd_terminate_and_restart   ();
    void                        cmd_load_config             ( const xml::Element_ptr& config )  { _config_document=config->ownerDocument; _config_element=config; _state_cmd=sc_load_config; cmd_wake(); }
    void                        cmd_wake                    ();

    void                        set_state                   ( State );
    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    Job*                        get_job                     ( const string& job_name );

    Fill_zero                  _zero_;
    
    int                        _argc;
    char**                     _argv;
    string                     _spooler_id;                 // -id=
    string                     _spooler_param;              // -param= Parameter für Skripten
    string                     _config_filename;            // -config=
    int                        _tcp_port;                   // <config tcp=...>
    int                        _udp_port;                   // <config udp=...>
    int                        _priority_max;               // <config priority_max=...>
    string                     _log_directory;              // -log-dir=
    string                     _log_filename;
    bool                       _use_threads;

    State_changed_handler      _state_changed_handler;      // Callback für NT-Dienst SetServiceStatus()

    Thread_semaphore           _semaphore;

    Thread_semaphore           _sleep_semaphore;
    bool                       _sleeping;
    bool                       _wake;
    Handle                     _command_arrived_event;      // Kommando über TCP oder UDP eingetroffen

                                                            // <config> wird vom Haupt-Thread ausgeführt
    xml::Document_ptr          _config_document;            // Das Dokument zu _config_element
    xml::Element_ptr           _config_element;             // Für cmd_load_config()

    Log                        _log;
    bool                       _is_service;                 // NT-Dienst

    Script                     _script;                     // <script>
    Script_instance            _script_instance;

    Security                   _security;                   // <security>
    Object_set_class_list      _object_set_class_list;      // <object_set_classes>
    Job_list                   _job_list;                   // <jobs>
    Task_list                  _task_list;                  // Nur Spooler-Thread änder
    Thread_semaphore           _task_list_lock;             // Spooler-Thread ändert, anderer Thread liest
    Wait_handles               _wait_handles;
    Communication              _communication;              // TCP und UDP (ein Thread)

    CComPtr<Com_spooler>       _com_spooler;                // COM-Objekt spooler
    CComPtr<Com_log>           _com_log;                    // COM-Objekt spooler.log

  //list<Thread>               _thread_list;

    Time                       _spooler_start_time;
    Time                       _next_start_time;
    State                      _state;
    State_cmd                  _state_cmd;

    int                        _running_tasks_count;        // Wenn 0, dann warten

                                                            // Statistik
    int                        _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    int                        _task_count;                 // Seit Spooler-Start gestartetet Tasks
};



} //namespace spooler
} //namespace sos

#endif