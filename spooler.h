// $Id: spooler.h,v 1.49 2001/02/16 18:23:12 jz Exp $

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
        struct Spooler;
        struct Thread;
        struct Task;
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
#include "spooler_thread.h"
#include "spooler_service.h"

#define FOR_EACH( TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )
#define FOR_EACH_CONST( TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )

namespace sos {


typedef _bstr_t                 Dom_string;

template<typename T>
inline Dom_string               as_dom_string               ( const T& t )                          { return as_bstr_t( t ); }

string                          program_filename            ();
string                          directory_of_path           ( const string& );
string                          basename_of_path            ( const string& );
string                          extension_of_path           ( const string& );

namespace spooler {

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

    const string&               id                          () const                            { return _spooler_id; }
    int                         udp_port                    () const                            { return _udp_port; }
    int                         tcp_port                    () const                            { return _tcp_port; }
    State                       state                       () const                            { return _state; }
    Log&                        log                         ()                                  { return _log; }
    const Security&             security                    () const                            { return _security; }

    int                         launch                      ( int argc, char** argv );                                
    void                        set_state_changed_handler   ( State_changed_handler h )         { _state_changed_handler = h; }

    // Für andere Threads:
    Job*                        get_job                     ( const string& job_name );
    void                        signal_object               ( const string& object_set_class_name, const Level& );
    void                        cmd_reload                  ();
    void                        cmd_pause                   ()                                  { _state_cmd = sc_pause; signal( "pause" ); }
    void                        cmd_continue                ();
    void                        cmd_stop                    ();
    void                        cmd_terminate               ();
    void                        cmd_terminate_and_restart   ();
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
    void                        signal_threads              ( const string& signal_name = "" );
    void                        wait_until_threads_stopped  ();
    void                        reload                      ();
    void                        run                         ();

  //void                        single_thread_step          ();
    void                        wait                        ();

    void                        signal                      ( const string& signal_name = "" )  { _event.signal( signal_name ); }


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

    Log                        _log;
    Prefix_log                 _prefix_log;
    State_changed_handler      _state_changed_handler;      // Callback für NT-Dienst SetServiceStatus()

    xml::Document_ptr          _config_document;            // Das Dokument zu _config_element
    xml::Element_ptr           _config_element;             // Für cmd_load_config()

    bool                       _is_service;                 // NT-Dienst

    Security                   _security;                   // <security>
    Object_set_class_list      _object_set_class_list;      // <object_set_classes>
    Communication              _communication;              // TCP und UDP (ein Thread)

    CComPtr<Com_spooler>       _com_spooler;                // COM-Objekt spooler
    CComPtr<Com_log>           _com_log;                    // COM-Objekt spooler.log

    Thread_list                _thread_list;

    Time                       _spooler_start_time;
    State                      _state;
    State_cmd                  _state_cmd;

    Wait_handles               _wait_handles;
    Event                      _event;                      
                                                            // <config> wird vom Haupt-Thread ausgeführt
    Thread_semaphore           _lock;
};

//-------------------------------------------------------------------------------------------------

void                            spooler_restart             ( bool is_service );

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif