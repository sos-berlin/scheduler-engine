// $Id: spooler.h,v 1.3 2001/01/02 13:51:36 jz Exp $

#ifndef __SPOOLER_H

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

//#include "../kram/xml_dom.h"

namespace sos {
namespace spooler {

using namespace std;

#define FOR_EACH( TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator ITERATOR = CONTAINER.begin(); ITERATOR != CONTAINER.end(); ITERATOR++ )
                                              
typedef int Level;


/*
struct Named_level
{
    Level                      _level;
    string                     _name;
};
*/


//typedef pair<Level,Level>       Level_intervall;    // [a,b) oder a <= x < b == x in Level_intervall

//---------------------------------------------------------------------------------Object_set_class

struct Object_set_class : Sos_self_deleting
{
                                Object_set_class            ()                      {}
                                Object_set_class            ( xml::Element_ptr );

    string                     _name;
    map<Level,string>          _level_map;
    
    string                     _script_language;
    string                     _script;

  //double                     _process_timeout;
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

//-------------------------------------------------------------------------------------------Object
/*
struct Object
{
};
*/
//----------------------------------------------------------------------------------------Job_descr

struct Job_descr : Sos_self_deleting
{
                                Job_descr                   ()                     : _zero_(this+1) {}
                                Job_descr                   ( xml::Element_ptr );


    Fill_zero                  _zero_;
    string                     _name;
    Object_set_descr           _object_set_descr;
    Level                      _output_level;
  //Start_time                 _start_time;
  //Duration                   _duration;
  //Repeat_time                _repeat_time;
    bool                       _stop_at_end_of_duration;
    bool                       _continual;
    bool                       _stop_after_error;
    bool                       _rerun;
    bool                       _start_after_spooler;
    int                        _priority;
};

typedef list< Sos_ptr<Job_descr> >    Job_descr_list;

//----------------------------------------------------------------------------------------------Job

struct Job : Sos_self_deleting
{
                                Job                         ( const Sos_ptr<Job_descr>& descr )    : _zero_(this+1), _job_descr(descr) {}

    void                        start                       ();
    void                        end                         ();
    bool                        step                        ();


    Fill_zero                  _zero_;
    Sos_ptr<Job_descr>         _job_descr;
    bool                       _running;
    time_t                     _running_since;
    int                        _running_priority;

    Sos_ptr<Object_set>        _object_set;
    time_t                     _next_try;                   // Zeitpunkt des nächsten Startversuchs, nachdem Objektemenge leer war
};

typedef list< Sos_ptr<Job> >    Job_list;

//---------------------------------------------------------------------------------------------Task
/*
struct Task : Sos_self_deleting
{
                                Task                        ( Job* job ) : _zero_(this+1), _job(job) {}

    void                        start                       ();
    void                        end                         ();
    void                        step                        ();


    Fill_zero                  _zero_;

    Sos_ptr<Job>               _job;
    bool                       _running;
    time_t                     _running_since;
    int                        _priority;
};
*/
//------------------------------------------------------------------------------------------Spooler

struct Spooler
{
                                Spooler                     () : _zero_(this+1) {}

    void                        load                        ();
    void                        load_xml                    ();

    void                        load_object_set_classes_from_xml( Object_set_class_list*, xml::Element_ptr );
    void                        load_jobs_from_xml          ( Job_descr_list*, xml::Element_ptr );

    void                        start                       ();       
    void                        run                         ();       

    bool                        step                        ();
    void                        start_jobs                  ();

    Fill_zero                  _zero_;
    Object_set_class_list      _object_set_class_list;
    Job_descr_list             _job_descr_list;
    Job_list                   _job_list;
    time_t                     _next_try_period;
};


} //namespace spooler
} //namespace sos

#endif