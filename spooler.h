// $Id: spooler.h,v 1.6 2001/01/03 22:15:31 jz Exp $

#ifndef __SPOOLER_H

#include "../kram/olestd.h"

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

#define FOR_EACH( TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )

namespace sos {



typedef _bstr_t Dom_string;

inline Dom_string               as_dom_string           ( const string& str )                       { return as_bstr_t( str ); }
inline Dom_string               as_dom_string           ( const char* str )                         { return as_bstr_t( str ); }


namespace spooler {

using namespace std;

                                              
typedef int                     Level;
struct                          Spooler;
typedef double                  Time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970

Time                            now();

const Time                      latter_day                  = INT_MAX;

//---------------------------------------------------------------------------------Object_set_class

struct Object_set_class : Sos_self_deleting
{
                                Object_set_class            ()                      {}
                                Object_set_class            ( xml::Element_ptr );

    string                     _name;
    map<Level,string>          _level_map;
    
    string                     _script_language;
    string                     _script;

  //Time                       _process_timeout;
};

typedef list< Sos_ptr<Object_set_class> >  Object_set_class_list;

//-----------------------------------------------------------------------------------Level_interval

struct Level_interval
{
                                Level_interval              ( xml::Element_ptr );
                                Level_interval              ()                      : _low_level(0), _high_level(0) {}

    bool                        is_in_interval              ( Level level )         { return level >= _low_level && level < _high_level; }

    Level                      _low_level;
    Level                      _high_level;
};

//-----------------------------------------------------------------------------------Spooler_object

struct Spooler_object
{
                              //Spooler_object              ( IDispatch* dispatch = NULL         ) : _dispatch(dispatch) {}
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
                                Object_set_descr            ()                      {}
                                Object_set_descr            ( xml::Element_ptr );

    string                     _class_name;
    Sos_ptr<Object_set_class>  _class;
    Level_interval             _level_interval;
};

//---------------------------------------------------------------------------------------Object_set

struct Object_set : Sos_self_deleting
{
                                Object_set                  ( const Sos_ptr<Object_set_descr>& descr ) : _object_set_descr(descr) {}

    void                        open                        ();
    void                        close                       ();
    Spooler_object              get                         ();

    CComPtr<Script_site>       _script_site;
    CComPtr<IDispatch>         _dispatch;
    Sos_ptr<Object_set_descr>  _object_set_descr;
};

//------------------------------------------------------------------------------------------Day_set

struct Day_set
{
                                Day_set                     ()                      { memset( _days, 0, sizeof _days ); }
                                Day_set                     ( xml::Element_ptr );

    bool                        is_empty                    ()                      { return memchr( _days, (char)true, sizeof _days ) == NULL; }
    char                        operator []                 ( int i )               { return _days[i]; }

    char                       _days                        [31];
};

//--------------------------------------------------------------------------------------Weekday_set

struct Weekday_set : Day_set
{
                                Weekday_set                 ()                      {}
                                Weekday_set                 ( xml::Element_ptr e )  : Day_set( e ) {}

    Time                        next_date                   ( Time );             // Mitternacht des nächsten gesetzten Tages
};

//--------------------------------------------------------------------------------------Monthday_set

struct Monthday_set : Day_set
{
                                Monthday_set                ()                      {}
                                Monthday_set                ( xml::Element_ptr e )  : Day_set( e ) {}

    Time                        next_date                   ( Time );               // Mitternacht des nächsten gesetzten Tages
};

//--------------------------------------------------------------------------------------Ultimo_set

struct Ultimo_set : Day_set
{
                                Ultimo_set                  ()                      {}
                                Ultimo_set                  ( xml::Element_ptr e )  : Day_set( e ) {}

    Time                        next_date                   ( Time );               // Mitternacht des nächsten gesetzten Tages
};

//---------------------------------------------------------------------------------------Start_time

struct Start_time
{
                                Start_time                  ()                      : _zero_(this+1) {}
                                Start_time                  ( xml::Element_ptr );

    Time                        next                        ()                      { return next( now() ); }
    Time                        next                        ( Time );


    Fill_zero                  _zero_;
    int                        _time_of_day;                // Sekunden seit Mitternacht

    set<time_t>                _date_set;
    Weekday_set                _weekday_set;
    Monthday_set               _monthday_set;
    Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
    set<time_t>                _holiday_set;

    Time                       _duration;
    Time                       _period;

    Time                       _next_start_time;
};

//---------------------------------------------------------------------------------------------Job

struct Job : Sos_self_deleting
{
                                Job                         ()                     : _zero_(this+1) {}
                                Job                         ( xml::Element_ptr );


    Fill_zero                  _zero_;
    string                     _name;
    Object_set_descr           _object_set_descr;
    Level                      _output_level;
    Start_time                 _start_time;
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
                                Task                        ( Spooler*, const Sos_ptr<Job>& );

    void                        start                       ();
    void                        end                         ();
    bool                        step                        ();

    void                        set_new_start_time          ();


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Job>               _job;
    bool                       _running;
    Time                       _running_since;
    int                        _running_priority;
    int                        _step_count;

    Sos_ptr<Object_set>        _object_set;
    Time                       _next_start_time;            // Zeitpunkt des nächsten Startversuchs, nachdem Objektemenge leer war
    Time                       _next_end_time;              // + _start_time._duration
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
    xml::Element_ptr            execute_command             ( xml::Element_ptr );
    xml::Element_ptr            execute_show_state          ();
    xml::Element_ptr            execute_show_tasks          ();

    Spooler*                   _spooler;
    xml::Document_ptr          _answer;
};

//------------------------------------------------------------------------------------------Spooler

struct Spooler
{
                                Spooler                     () : _zero_(this+1), _comm_channel(this), _command_processor(this) {}

    void                        load                        ();
    void                        load_xml                    ();

    void                        load_object_set_classes_from_xml( Object_set_class_list*, xml::Element_ptr );
    void                        load_jobs_from_xml          ( Job_list*, xml::Element_ptr );

    void                        start                       ();       
    void                        run                         ();
    void                        wait                        ();

    bool                        step                        ();
    void                        start_jobs                  ();

    Fill_zero                  _zero_;
    Object_set_class_list      _object_set_class_list;
    Job_list                   _job_list;
    Task_list                  _task_list;
    Time                       _try_start_job_period;
    int                        _running_jobs_count;
    Communication_channel      _comm_channel;
    Command_processor          _command_processor;
    Time                       _spooler_start_time;
};


} //namespace spooler
} //namespace sos

#endif