// $Id: spooler.h,v 1.25 2001/01/13 18:41:18 jz Exp $

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

#   include "spooler_com.h"
#endif

#include <stdio.h>
        
#include <set>
#include <map>
#include <list>
#include <time.h>

#include "../kram/sosdate.h"
#include "../kram/sossock1.h"
#include "../kram/thread_semaphore.h"

#include "spooler_wait.h"

#define FOR_EACH( TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )

namespace sos {



typedef _bstr_t Dom_string;

template<typename T>
inline Dom_string               as_dom_string               ( const T& t )                          { return as_bstr_t( t ); }


namespace spooler {

using namespace std;

struct Task;

                                              
typedef int                     Level;
struct                          Spooler;
typedef double                  Time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970

Time                            now();

const Time                      latter_day                  = INT_MAX;


enum Log_kind { log_msg, log_warn, log_error };




//-------------------------------------------------------------------------------------------Handle

struct Handle
{
#   ifdef SYSTEM_WIN
                                Handle                      ( HANDLE h = NULL )             : _handle(h) {}
                               ~Handle                      ()                              { close(); }

        void                    operator =                  ( HANDLE h )                    { close(); _handle = h; }
                                operator HANDLE             () const                        { return _handle; }
                                operator !                  () const                        { return _handle == 0; }
        HANDLE*                 operator &                  ()                              { return &_handle; }

        void                    close                       ()                              { if(_handle) { CloseHandle(_handle); _handle=0; } }

        HANDLE                 _handle;
#   endif
};

//--------------------------------------------------------------------------------------------Mutex

template<typename T>
struct Mutex
{
    typedef sos::Thread_semaphore::Guard Guard;


                                Mutex                       ( const T& t = T() )    : _value(t) {}

    Mutex&                      operator =                  ( const T& t )          { Guard g = &_semaphore; _value = t; return *this; }
                                operator T                  ()                      { Guard g = &_semaphore; T v = _value; return v; }
    T                           read_and_reset              ()                      { Guard g = &_semaphore; T v = _value; _value = T(); return v; }

    sos::Thread_semaphore      _semaphore;
    T                          _value;
};

//----------------------------------------------------------------------------------------------Log

struct Log
{
                                Log                         ( Spooler* );
                               ~Log                         ();

    void                        set_directory               ( const string& );
    void                        open_new                    ();

    void                        msg                         ( const string& line )              { log( log_msg, "", line ); }
    void                        warn                        ( const string& line )              { log( log_warn, "", line ); }
    void                        error                       ( const string& line )              { log( log_error, "", line ); }

    void                        log                         ( Log_kind log, const string& prefix, const string& );
    
    string                      filename                    () const                            { return _filename; }

  protected:
    void                        write                       ( const string& );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _directory;
    string                     _filename;
    FILE*                      _file;
    Thread_semaphore           _semaphore;
};

//-----------------------------------------------------------------------------------------Task_log

struct Task_log
{
                                Task_log                    ( Log*, Task* = NULL );

    void                        msg                         ( const string& line )              { log( log_msg, line ); }
    void                        warn                        ( const string& line )              { log( log_warn, line ); }
    void                        error                       ( const string& line )              { log( log_error, line ); }
    void                        log                         ( Log_kind log, const string& );

    Log*                       _log;
    Task*                      _task;
    string                     _prefix;
};

//-------------------------------------------------------------------------------------------Script

struct Script
{
    enum Reuse
    {
        reuse_task,
        reuse_job,
        reuse_global
    };
                                Script                      ()                              {}
                                Script                      ( const xml::Element_ptr& e )   { *this = e; }

    void                        operator =                  ( const xml::Element_ptr& );

    bool                        empty                       () const                        { return _text.empty(); }
    void                        clear                       ()                              { _language="", _text=""; }

    string                     _language;
    string                     _text;
    Reuse                      _reuse;
};

//----------------------------------------------------------------------------------Script_instance

struct Script_instance
{
                                Script_instance             ( Script* script )              : _script(script) {}

    void                        init                        ();
    void                        load                        ();
    IDispatch*                  dispatch                    () const                        { return _script_site? _script_site->dispatch() : NULL; }
    void                        add_obj                     ( const CComPtr<IDispatch>&, const string& name );
    void                        close                       ();
    CComVariant                 call                        ( const char* name );
    CComVariant                 call                        ( const char* name, int param );
    CComVariant                 property_get                ( const char* name );
    void                        property_put                ( const char* name, const CComVariant& v ) { _script_site->property_put( name, v ); } 
    void                        optional_property_put       ( const char* name, const CComVariant& v );
    bool                        name_exists                 ( const string& name )          { return _script_site->name_exists(name); }

    Script*                    _script;
    CComPtr<Script_site>       _script_site;
};

//---------------------------------------------------------------------------------Object_set_class

struct Object_set_class : Sos_self_deleting
{
                                Object_set_class            ()                              {}
                                Object_set_class            ( const xml::Element_ptr& e )   { *this = e; }

    void                        operator =                  ( const xml::Element_ptr& );

    string                     _name;
    map<Level,string>          _level_map;
    
    Script                     _script;

  //Time                       _process_timeout;
};

typedef list< Sos_ptr<Object_set_class> >  Object_set_class_list;

//-----------------------------------------------------------------------------------Level_interval

struct Level_interval
{
                                Level_interval              ()                              : _low_level(0), _high_level(0) {}
                                Level_interval              ( const xml::Element_ptr& e )   { *this = e; }

    void                        operator =                  ( const xml::Element_ptr& );

    bool                        is_in_interval              ( Level level )                 { return level >= _low_level && level < _high_level; }

    Level                      _low_level;
    Level                      _high_level;
};

//-----------------------------------------------------------------------------------Spooler_object

struct Spooler_object
{
                                Spooler_object              ( const CComPtr<IDispatch>& dispatch = NULL ) : _dispatch(dispatch) {}

    Spooler_object&             operator =                  ( const CComPtr<IDispatch>& dispatch ) { _dispatch = dispatch; return *this; }
    Level                       level                       ();
    void                        process                     ( Level output_level );
    bool                        is_null                     ()                              { return _dispatch == NULL; }

    CComPtr<IDispatch>         _dispatch;
};

//---------------------------------------------------------------------------------Object_set_descr

struct Object_set_descr : Sos_self_deleting
{
                                Object_set_descr            ()                              {}
                                Object_set_descr            ( const xml::Element_ptr& e )   { *this = e; }

    void                        operator =                  ( const xml::Element_ptr& );

    string                     _class_name;
    Sos_ptr<Object_set_class>  _class;
    Level_interval             _level_interval;
};

//---------------------------------------------------------------------------------------Object_set

struct Object_set : Sos_self_deleting
{
                                Object_set                  ( Spooler*, Task*, const Sos_ptr<Object_set_descr>& );
                               ~Object_set                  ();

    void                        open                        ();
    void                        close                       ();
    bool                        eof                         ();
    Spooler_object              get                         ();
    bool                        step                        ( Level result_level );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Task*                      _task;
    Sos_ptr<Object_set_descr>  _object_set_descr;
    Script_instance            _script_instance;
    bool                       _use_objects;                // Objektschnittstelle nutzen. Sonst prozedural
    CComPtr<IDispatch>         _dispatch;
};

//------------------------------------------------------------------------------------------Day_set

struct Day_set
{
                                Day_set                     ()                              { memset( _days, 0, sizeof _days ); }
                                Day_set                     ( const xml::Element_ptr& e )   { *this = e; }
    void                        operator =                  ( const xml::Element_ptr& );

    bool                        is_empty                    ()                      { return memchr( _days, (char)true, sizeof _days ) == NULL; }
    char                        operator []                 ( int i )               { return _days[i]; }

    char                       _days                        [31];
};

//--------------------------------------------------------------------------------------Weekday_set

struct Weekday_set : Day_set
{
                                Weekday_set                 ()                      {}
                                Weekday_set                 ( const xml::Element_ptr& e )  : Day_set( e ) {}

    Time                        next_date                   ( Time );               // 00:00:00 des nächsten gesetzten Tages
};

//--------------------------------------------------------------------------------------Monthday_set

struct Monthday_set : Day_set
{
                                Monthday_set                ()                      {}
                                Monthday_set                ( const xml::Element_ptr& e )  : Day_set( e ) {}

    Time                        next_date                   ( Time );               // 00:00:00 des nächsten gesetzten Tages
};

//--------------------------------------------------------------------------------------Ultimo_set

struct Ultimo_set : Day_set
{
                                Ultimo_set                  ()                      {}
                                Ultimo_set                  ( const xml::Element_ptr& e )  : Day_set( e ) {}

    Time                        next_date                   ( Time );               // 00:00:00 des nächsten gesetzten Tages
};

//----------------------------------------------------------------------------------------Run_time

struct Run_time
{
                                Run_time                    ()                      : _zero_(this+1) {}
                                Run_time                    ( const xml::Element_ptr& );

    void                        check                       ();                              
    Time                        next                        ()                      { return next( now() ); }
    Time                        next                        ( Time );
    bool                        should_run_now              ()                      { Time nw = now(); return nw >= _next_start_time && nw < _next_end_time; }


    Fill_zero                  _zero_;
    
    Time                       _retry_period;

    int                        _begin_time_of_day;          // Sekunden seit Mitternacht
    int                        _end_time_of_day;            // Sekunden seit Mitternacht

    set<time_t>                _date_set;
    Weekday_set                _weekday_set;
    Monthday_set               _monthday_set;
    Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
    set<time_t>                _holiday_set;

    Time                       _next_start_time;
    Time                       _next_end_time;
};

//---------------------------------------------------------------------------------------------Job

struct Job : Sos_self_deleting
{
                                Job                         ()                     : _zero_(this+1) {}
                                Job                         ( const xml::Element_ptr& );


    Fill_zero                  _zero_;
    string                     _name;
    Sos_ptr<Object_set_descr>  _object_set_descr;
    Level                      _output_level;
    Script                     _script;
    Run_time                   _run_time;
    bool                       _stop_at_end_of_duration;
    bool                       _continual;
    bool                       _stop_after_error;
    bool                       _rerun;
    bool                       _start_after_spooler;
    int                        _priority;
};

typedef list< Sos_ptr<Job> >    Job_list;

//----------------------------------------------------------------------------------------------Task

struct Task : Sos_self_deleting
{
    enum State
    {
        s_none,
        s_stopped,              // Gestoppt (z.B. wegen Fehler)
        s_pending,              // Warten auf Start
        s_running,              // Läuft
        s_suspended,            // Angehalten
        s_ending,               // end()
        s__max
    };

    enum State_cmd
    {
        sc_none,
        sc_stop,                // s_running | s_suspended -> s_stopped
        sc_unstop,              // s_stopped -> s_pending
        sc_start,               // s_pending -> s_running
        sc_end,                 // s_running -> s_pending
        sc_suspend,             // s_running -> s_suspended
        sc_continue,            // s_suspended -> s_running
        sc__max
    };


                                Task                        ( Spooler*, const Sos_ptr<Job>& );
                               ~Task                        ();

    bool                        start                       ();
    void                        prepare_script              ();
    void                        end                         ();
    void                        stop                        ();
    bool                        step                        ();

    bool                        do_something                ();

  //void                        wake                        ()                          { _wake = true; }   // Ein neues Objekt ist da, vielleicht.

    void                        set_state                   ( State );
    void                        set_state_cmd               ( State_cmd );

    string                      state_name                  ()                          { return state_name( _state ); }
    static string               state_name                  ( State );
    static State                as_state                    ( const string& );
    
    string                      state_cmd_name              ()                          { return state_cmd_name( _state_cmd ); }
    static string               state_cmd_name              ( State_cmd );
    static State_cmd            as_state_cmd                ( const string& );

    void                        start_error                 ( const Xc& );
    void                        end_error                   ( const Xc& );
    void                        step_error                  ( const Xc& );
    void                        error                       ( const Xc& );
    void                        error                       ( const exception& );

    void                        set_new_start_time          ();

    void                        wake_when_directory_changed ( const string& directory_name );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;
    Script_instance            _job_script_instance;
    Script_instance*           _script_instance;
    
    int                        _priority;
    double                     _cpu_time;
    int                        _step_count;

    bool                       _run_until_end;              // Nach Kommando sc_start: Task zuende laufen lassen, nicht bei _next_end_time beenden
    Mutex<State>               _state;
    Mutex<State_cmd>           _state_cmd;

    Time                       _running_since;

    Xc_copy                    _error;

    Sos_ptr<Object_set>        _object_set;
    Time                       _next_start_time;            // Zeitpunkt des nächsten Startversuchs, nachdem Objektemenge leer war
    Directory_watcher          _directory_watcher;
    Task_log                   _log;
    CComPtr<Com_log>           _com_log;
    CComPtr<Com_object_set>    _com_object_set;
    CComPtr<Com_task>          _com_task;
};

typedef list< Sos_ptr<Task> >   Task_list;

//-----------------------------------------------------------------------------------Xml_end_finder

const int xml_end_finder_token_count = 2;

struct Xml_end_finder
{
    enum Tok { cdata_tok, comment_tok };


    struct Tok_entry
    {
                                Tok_entry                   ()                          : _index(0),_active(false) {}

        void                    reset                       ()                          { _index = 0; _active = false; }
        bool                    step_begin                  ( char );
        void                    step_end                    ( char );

        int                    _index;
        bool                   _active;
        const char*            _begin;
        const char*            _end;
    };


                                Xml_end_finder              ();

    bool                        is_complete                 ( const char* p, int length );

    Fill_zero                  _zero_;

    int                        _open_elements;              // Anzahl der offenen Elemente (ohne <?..?> und <!..>)
    bool                       _at_start_tag;               // Letztes Zeichen war '<'
    bool                       _in_special_tag;             // <?, <!
    bool                       _in_tag;                 
    bool                       _in_end_tag;             
    bool                       _xml_is_complete;
    char                       _last_char;
    Tok_entry                  _tok [xml_end_finder_token_count];
};

//------------------------------------------------------------------------------------Communication

struct Communication
{                                                 
    struct Channel : Sos_self_deleting
    {

                                Channel                     ();
                               ~Channel                     ();


        void                    do_accept                   ( SOCKET listen_socket );
        void                    do_close                    ();
        void                    do_recv                     ();
        void                    do_send                     ();


        Fill_zero              _zero_;
        
        SOCKET                 _socket;
        struct sockaddr_in     _peer_addr;

        string                 _text;

        bool                   _receive_at_start;
        bool                   _receive_is_complete;
        bool                   _eof;

        Xml_end_finder         _xml_end_finder;

        bool                   _send_is_complete;
        int                    _send_progress;

    };

    typedef list< Sos_ptr<Channel> >  Channel_list;


                                Communication               ( Spooler* );
                               ~Communication               ();

    void                        start_thread                ();
    void                        close                       ();
    void                        rebind                      ()                                      { bind(); }
    int                         go                          ();

  private:
    int                         run                         ();
    void                        bind                        ();
    void                        start                       ();
    bool                        handle_socket               ( Channel* );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    SOCKET                     _listen_socket;
    Channel_list               _channel_list;
    SOCKET                     _udp_socket;
    int                        _nfds;
    FD_SET                     _read_fds;
    FD_SET                     _write_fds;
    Thread_semaphore           _semaphore;
    bool                       _terminate;
    int                        _tcp_port;
    int                        _udp_port;
    bool                       _rebound;

    Handle                     _thread;
};

//--------------------------------------------------------------------------------Command_processor

struct Command_processor
{
                                Command_processor           ( Spooler* spooler )                    : _spooler(spooler) {}

    string                      execute                     ( const string& xml_text );
    xml::Element_ptr            execute_command             ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_state          ();
    xml::Element_ptr            execute_show_tasks          ();
    xml::Element_ptr            execute_show_task           ( Task* );
    xml::Element_ptr            execute_modify_job          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_spooler      ( const xml::Element_ptr& );
    xml::Element_ptr            execute_signal_object       ( const xml::Element_ptr& );

    Spooler*                   _spooler;
    xml::Document_ptr          _answer;
};

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
        sc_start,               // s_paused -> s_running
        sc_terminate,           // s_running | s_paused -> s_stopped, exit()
        sc_terminate_and_restart,
        sc_reload,
        sc_pause,               // s_running -> s_paused
        sc_continue,            // s_suspended -> s_running
        sc__max
    };


                                Spooler                     ();
                               ~Spooler                     ();

    int                         launch                      ( int argc, char** argv );                                
    void                        load_arg                    ();
    void                        load                        ();
    void                        load_xml                    ();

    void                        load_object_set_classes_from_xml( Object_set_class_list*, const xml::Element_ptr& );
    void                        load_jobs_from_xml          ( Job_list*, const xml::Element_ptr& );

    void                        start                       ();
    void                        stop                        ();
    void                        reload                      ();
    void                        run                         ();
    void                        restart                     ();

    void                        wait                        ();

    void                        cmd_reload                  ();
    void                        cmd_pause                   ()                                  { _state_cmd = sc_pause; cmd_wake(); }
    void                        cmd_continue                ()                                  { if( _state == s_paused )  _state = s_running; cmd_wake(); }
    void                        cmd_stop                    ();
    void                        cmd_terminate               ();
    void                        cmd_terminate_and_restart   ();
    void                        cmd_wake                    ();

    void                        step                        ();
    void                        start_jobs                  ();

    Fill_zero                  _zero_;
    
    int                        _argc;
    char**                     _argv;
    string                     _spooler_id;
    string                     _spooler_param;              // Parameter für Skripten
    string                     _config_filename;
    int                        _tcp_port;
    int                        _udp_port;
    int                        _priority_max;
    string                     _log_directory;
    string                     _log_filename;


    Thread_semaphore           _semaphore;
    volatile bool              _sleeping;                   // Besser: sleep mit Signal unterbrechen
    Wait_handles               _wait_handles;               // Vor _task_list!
    Handle                     _command_arrived_event;

    Log                        _log;
    Script                     _script;
    Script_instance            _script_instance;
    Object_set_class_list      _object_set_class_list;
    Job_list                   _job_list;

    Task_list                  _task_list;
    Communication              _communication;
    Command_processor          _command_processor;

    CComPtr<Com_spooler>       _com_spooler;
    CComPtr<Com_log>           _com_log;

    Time                       _spooler_start_time;
    Time                       _next_start_time;
    State                      _state;
    State_cmd                  _state_cmd;

    int                        _running_jobs_count;
    int                        _step_count;
    int                        _task_count;
};


int spooler_service( int argc, char** argv );

} //namespace spooler
} //namespace sos

#endif