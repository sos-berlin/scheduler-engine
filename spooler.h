// $Id: spooler.h,v 1.15 2001/01/09 22:39:02 jz Exp $

#ifndef __SPOOLER_H

#include "../kram/olestd.h"
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

#   include <atlbase.h>
#   include "../kram/olestd.h"
#   include "../kram/sosscrpt.h"
#endif
        
#include <set>
#include <map>
#include <list>
#include <time.h>

#include "../kram/sosdate.h"
#include "../kram/sossock1.h"
#include "../kram/thread_semaphore.h"

#define FOR_EACH( TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )

namespace sos {



typedef _bstr_t Dom_string;

template<typename T>
inline Dom_string               as_dom_string               ( const T& t )                          { return as_bstr_t( t ); }


namespace spooler {

using namespace std;

                                              
typedef int                     Level;
struct                          Spooler;
typedef double                  Time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970

Time                            now();

const Time                      latter_day                  = INT_MAX;

//--------------------------------------------------------------------------------------------Mutex

template<typename T>
struct Mutex
{
    typedef Thread_semaphore::Guard Guard;


                                Mutex                   ( const T& t = T() )    : _value(t) {}

    Mutex&                      operator =              ( const T& t )          { Guard g = &_semaphore; _value = t; return *this; }
                                operator T              ()                      { Guard g = &_semaphore; T v = _value; return v; }
    T                           read_and_reset          ()                      { Guard g = &_semaphore; T v = _value; _value = T(); return v; }

    Thread_semaphore           _semaphore;
    T                          _value;
};

//-------------------------------------------------------------------------------------------Script

struct Script
{
                                Script                      ()                              {}
                                Script                      ( const xml::Element_ptr& e )   { *this = e; }

    void                        operator =                  ( const xml::Element_ptr& );

    bool                        empty                       () const                        { return _text.empty(); }

    string                     _language;
    string                     _text;
};

//----------------------------------------------------------------------------------Script_instance

struct Script_instance
{
                                Script_instance             ( Script* script )              : _script(script) {}

    void                        load                        ();
    void                        close                       ();
    CComVariant                 call                        ( const char* name );
    CComVariant                 call                        ( const char* name, int param );
    CComVariant                 property_get                ( const char* name );

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

    bool                        is_in_interval              ( Level level )         { return level >= _low_level && level < _high_level; }

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
                                Object_set                  ( Spooler*, const Sos_ptr<Object_set_descr>& );
                               ~Object_set                  ();

    void                        open                        ();
    void                        close                       ();
    bool                        eof                         ();
    Spooler_object              get                         ();
    bool                        step                        ( Level result_level );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
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

    bool                        start                       ();
    void                        end                         ();
    bool                        step                        ();

    void                        do_something                ();

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

    void                        set_new_start_time          ();


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;
    Script_instance            _script_instance;
    
    int                        _running_priority;
    int                        _step_count;

    bool                       _run_until_end;              // Nach Kommando sc_start: Task zuende laufen lassen, nicht bei _next_end_time beenden
    Mutex<State>               _state;
    Mutex<State_cmd>           _state_cmd;

    Time                       _running_since;

    Xc_copy                    _error;

    Sos_ptr<Object_set>        _object_set;
    Time                       _next_start_time;            // Zeitpunkt des nächsten Startversuchs, nachdem Objektemenge leer war
};

typedef list< Sos_ptr<Task> >   Task_list;

//----------------------------------------------------------------------------Communication_channel

struct Communication_channel
{
                                Communication_channel       ( Spooler* );
                               ~Communication_channel       ();

    void                        start_thread                ();

    int                         run                         ();
    void                        wait_for_connection         ();
    string                      recv_xml                    ();
    void                        send_text                   ( const string& );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    SOCKET                     _listen_socket;
    SOCKET                     _socket;
    HANDLE                     _thread;
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
                                Spooler                     () : _zero_(this+1), _comm_channel(this), _command_processor(this) {}

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
    void                        cmd_pause                   ()                                  { _pause = true; cmd_wake(); }
    void                        cmd_continue                ()                                  { _paused = false; cmd_wake(); }
    void                        cmd_stop                    ();
    void                        cmd_terminate               ();
    void                        cmd_terminate_and_restart   ();
    void                        cmd_wake                    ()                                  { _sleeping = false; }

    void                        step                        ();
    void                        start_jobs                  ();

    Fill_zero                  _zero_;
    Object_set_class_list      _object_set_class_list;
    Job_list                   _job_list;
    Task_list                  _task_list;
    int                        _running_jobs_count;
    Communication_channel      _comm_channel;
    Command_processor          _command_processor;
    Time                       _spooler_start_time;
    Thread_semaphore           _semaphore;
    volatile bool              _sleeping;                    // Besser: sleep mit Signal unterbrechen
    Time                       _next_start_time;
    bool                       _reload;
    bool                       _stop;
    bool                       _pause;
    bool                       _paused;
    bool                       _terminate;
    bool                       _terminate_and_restart;
    int                        _tcp_port;
    int                        _udp_port;
    string                     _config_filename;
    string                     _log_filename;
    string                     _spooler_id;
    string                     _object_set_param;
};


} //namespace spooler
} //namespace sos

#endif