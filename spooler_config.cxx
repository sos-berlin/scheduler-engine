// $Id: spooler_config.cxx,v 1.29 2002/03/05 20:49:37 jz Exp $

//#include <precomp.h>

/*
    Hier sind die Methoden verschiedenener Klassen implementiert, die die Konfiguration laden.
*/

#include "../kram/sos.h"
#include "spooler.h"
#include "../file/anyfile.h"


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------optional_single_element

xml::Element_ptr optional_single_element( const xml::Element_ptr& element, const string& name )
{
    xml::NodeList_ptr list = element->getElementsByTagName( as_dom_string( name ) );

    int len = list->length;
    if( len == 0 )  return xml::Element_ptr();

    if( len > 1 )  throw_xc( "SOS-1423", name );

    return list->Getitem(0);
}

//---------------------------------------------------------------------------default_single_element

xml::Element_ptr default_single_element( const xml::Element_ptr& element, const string& name )
{
    xml::Element_ptr result = optional_single_element( element, name );
    if( result == NULL ) {
        return element->ownerDocument->createElement( as_dom_string(name) );     // Künstliches Element mit Attributwerten aus der DTD
    }

    return result;
}

//-----------------------------------------------------------------------------------single_element

xml::Element_ptr single_element( const xml::Element_ptr& element, const string& name )
{
    xml::Element_ptr result = optional_single_element( element, name );
    if( result == NULL )  throw_xc( "SOS-1422", name );

    return result;
}

//-----------------------------------------------------------------------------------single_element

string optional_single_element_as_text( const xml::Element_ptr& element, const string& name )
{
    xml::Element_ptr result = optional_single_element( element, name );
    if( result == NULL )  return empty_string;
    
    xml::Element_ptr text_element = result->firstChild;
    if( text_element == NULL )  return empty_string;

    return as_string( text_element->nodeValue );
}

//-----------------------------------------------------------------------text_from_xml_with_include

string text_from_xml_with_include( const xml::Element_ptr& element, const string& include_path )
{
    string text;
    string inc = include_path;

    if( !inc.empty() )  { char c = inc[inc.length()-1];  if( c != '/'  &&  c != '\\' )  inc += "/"; }


    for( xml::Node_ptr n = element->firstChild; n; n = n->nextSibling )
    {
        switch( n->GetnodeType() )
        {
            case xml::NODE_CDATA_SECTION:
            {
                xml::Cdata_section_ptr c = n;
                text += as_string( c->data );
                break;
            }

            case xml::NODE_TEXT:
            {
                xml::Text_ptr t = n;
                text += as_string( t->data );
                break;
            }

            case xml::NODE_ELEMENT:     // <include file="..."/>
            {
                xml::Element_ptr e = n;
                string filename = as_string( e->getAttribute( L"file" ) );

                if( filename.length() >= 1 ) 
                {
                    if( filename[0] == '\\' 
                     || filename[0] == '/' 
                     || filename.length() >= 2 && filename[1] == ':' )  ; // ok, absoluter Dateiname
                    else  
                    {
                        filename = inc + filename;
                    }
                }
                     
                text += file_as_string( filename );
                break;
            }

            default: ;
        }
    }

    return text;
}

//--------------------------------------------------------------------------------Security::set_xml

void Security::set_xml( const xml::Element_ptr& security_element ) 
{ 
    bool ignore_unknown_hosts = as_bool( security_element->getAttribute( L"ignore_unknown_hosts" ) );

    for( xml::Element_ptr e = security_element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "allowed_host" )
        {
            string    hostname = as_string( e->getAttribute( L"host" ) );
            set<Host> host_set;

            try {
                host_set = Host::get_host_set_by_name( hostname );
            }
            catch( const Xc& x )
            {
                _spooler->log().error( "<allowed_host host=\"" + hostname + "\">  " + x.what() );
                if( !ignore_unknown_hosts )  throw;
            }
            
            FOR_EACH( set<Host>, host_set, h )
            {
                _host_map[ *h ] = as_level( as_string( e->getAttribute( L"level" ) ) );
            }
        }
    }
}

//----------------------------------------------------------------------------------Period::set_xml
namespace time {

void Period::set_xml( const xml::Element_ptr& element, const Period* deflt )
{
    Sos_optional_date_time dt;

    if( deflt )  *this = *deflt;

    string let_run = as_string( element->getAttribute( L"let_run" ) );
    if( !let_run.empty() )  _let_run = as_bool( let_run );

    string single_start = as_string( element->getAttribute( L"single_start" ) );
    if( !single_start.empty() ) 
    {
        dt.set_time( single_start );
        _begin = dt;
        _repeat = latter_day;
        _single_start = true;
        if( _let_run )  _end = _begin;
    }
    else
    {
        string begin = as_string( element->getAttribute( L"begin" ) );
        if( !begin.empty() )  dt.set_time( begin ), _begin = dt;

        string repeat = as_string( element->getAttribute( L"repeat" ) );
        if( !repeat.empty() )
        {
            if( repeat.find( ':' ) != string::npos )
            {
                Sos_optional_date_time dt;
                dt.set_time( repeat );
                _repeat = dt.time_as_double();
            }
            else
                _repeat = as_double( repeat );
        }
    }

    string end = as_string( element->getAttribute( L"end" ) );
    if( !end.empty() )  dt.set_time( end ), _end = dt;

    check();
}

//-------------------------------------------------------------------------------------Day::set_xml

void Day::set_xml( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    if( default_day )  _period_set = default_day->_period_set;

  //Period my_default_period ( element, default_period );
    bool   first = true;

    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( first )  first = false, _period_set.clear();
        _period_set.insert( Period( e, default_period ) );
    }

  //if( _period_set.empty() )  _period_set.insert( my_default_period );
}

//---------------------------------------------------------------------------------Day_set::set_xml

void Day_set::set_xml( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    //Period my_default_period ( element, default_period );

    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "day" )
        {
            Day my_default_day ( e, default_day, default_period );

            int day = as_int( e->getAttribute( L"day" ) );
            if( (uint)day >= NO_OF(_days) )  throw_xc( "SPOOLER-INVALID-DAY", day );
            _days[day].set_xml( e, &my_default_day, default_period );
        }
    }
}

//--------------------------------------------------------------------------------Run_time::set_xml

void Run_time::set_xml( const xml::Element_ptr& element )
{
    Sos_optional_date_time  dt;
    Period                  default_period;
    Day                     default_day;
    bool                    period_seen = false;
    

    _once = as_bool( element->getAttribute( L"once" ) );

    default_period.set_xml( element, NULL );
    default_day = default_period;

    bool a_day_set = false;
    
    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "period" )
        {
            if( !period_seen )  period_seen = true, default_day = Day();
            default_day.add( Period( e, &default_period ) );
        }
        else
        if( e->tagName == "date" )
        {
            a_day_set = true;
            dt.assign( as_string( e->getAttribute( L"date" ) ) );
            Date date;
            date._day_nr = dt.as_time_t() / (24*60*60);
            date._day.set_xml( e, &default_day, &default_period );
            _date_set._date_set.insert( date );
        }
        else
        if( e->tagName == "weekdays" )
        {
            a_day_set = true;
            _weekday_set.set_xml( e, &default_day, &default_period );
        }
        else
        if( e->tagName == "monthdays" )
        {
            a_day_set = true;
            _monthday_set.set_xml( e, &default_day, &default_period );
        }
        else
        if( e->tagName == "ultimos" )
        {
            a_day_set = true;
            _ultimo_set.set_xml( e, &default_day, &default_period );
        }
        else
        if( e->tagName == "holiday" )
        {
            dt.assign( as_string( e->getAttribute( L"date" ) ) );
            _holiday_set.insert( dt.as_time_t() );
        }
    }

    if( !a_day_set )  for( int i = 0; i < 7; i++ )  _weekday_set._days[i] = default_day;
}

} //namespace time

//------------------------------------------------------------------------Object_set_class::set_xml

void Object_set_class::set_xml( const xml::Element_ptr& element )
{
    _name = as_string( element->getAttribute( L"name" ) );

    string iface = as_string( element->getAttribute( L"script_interface" ) );
    _object_interface = iface == "oo";

    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "script" )
        {
            _script.set_xml( e, _spooler->include_path() );
        }
        else
        if( e->tagName == "level_decls" )
        {
            for( xml::Element_ptr e2 = e->firstChild; e2; e2 = e2->nextSibling )
            {
                if( e2->tagName == "level_decl" ) 
                {
                    int    level = as_int( e2->getAttribute( L"level" ) );
                    string name  = as_string( e2->getAttribute( L"name" ) );

                    _level_map[ level ] = name;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------Level_interval::set_xml

void Level_interval::set_xml( const xml::Element_ptr& element )
{
    _low_level  = as_int( element->getAttribute( L"low" ) );
    _high_level = as_int( element->getAttribute( L"high" ) );
}

//------------------------------------------------------------------------Object_set_descr::set_xml

void Object_set_descr::set_xml( const xml::Element_ptr& element )
{ 
    _class_name     = as_string( element->getAttribute( L"class" ) );
    _level_interval.set_xml( single_element( element, "levels" ) );
}

//-------------------------------------------------------------------------------------Job::set_xml

void Job::set_xml( const xml::Element_ptr& element )
{
    bool run_time_set = false;

    _name             = as_string( element->getAttribute( L"name" ) );
  //_rerun            = as_bool  ( element->getAttribute( L"rerun" ) ) ),
  //_stop_after_error = as_bool  ( element->getAttribute( L"stop_after_errorn ) );
    _temporary        = as_bool  ( element->getAttribute( L"temporary" ) );
    _priority         = as_int   ( element->getAttribute( L"priority" ) );
    _title            = as_string( element->getAttribute( L"title" ) );
    _log_append       = as_bool  ( element->getAttribute( L"log_append" ) );

    string text;

    text = as_string( element->getAttribute( L"output_level" ) );
    if( !text.empty() )  _output_level = as_int( text );

    //for( time::Holiday_set::iterator it = _spooler->_run_time._holidays.begin(); it != _spooler->_run_time._holidays.end(); it++ )
    //    _run_time._holidays.insert( *it );
    _run_time.set_holidays( _spooler->holidays() );

    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "description" )  
        {
            try { _description = text_from_xml_with_include( e, _spooler->include_path() ); }
            catch( const Xc& x         ) { _spooler->_log.error( x.what() );  _description = x.what(); }
            catch( const _com_error& x ) { string d = bstr_as_string(x.Description()); _spooler->_log.error(d);  _description = d; }
        }
        else
        if( e->tagName == "object_set"  )  _object_set_descr = SOS_NEW( Object_set_descr( e ) );
        else
        if( e->tagName == "script"      )  _script_xml_element = e;
        else
        if( e->tagName == "process"     )  _process_filename = as_string( e->getAttribute( L"file" ) ),
                                           _process_param    = as_string( e->getAttribute( L"param" ) );
        else
        if( e->tagName == "run_time"    )  _run_time.set_xml( e ),  run_time_set = true;
    }

    if( !run_time_set )  _run_time.set_xml( element->ownerDocument->createElement( L"run_time" ) );

    if( _object_set_descr )  _object_set_descr->_class = _spooler->get_object_set_class( _object_set_descr->_class_name );
}

//--------------------------------------------------------Spooler::load_object_set_classes_from_xml

void Spooler::load_object_set_classes_from_xml( Object_set_class_list* liste, const xml::Element_ptr& element )
{
    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "object_set_class" )  liste->push_back( SOS_NEW( Object_set_class( this, e ) ) );
    }
}

//----------------------------------------------------------------------------------Thread::set_xml

void Thread::set_xml( const xml::Element_ptr& element )
{
    string str;

    _name = as_string( element->getAttribute( L"name" ) );

    str = as_string( element->getAttribute( L"free_threading" ) );
    _free_threading = str.empty()? _spooler->free_threading_default() : as_bool( str );

    str = as_string( element->getAttribute( L"priority" ) );
    if( !str.empty() )
    {
        if( str == "idle" )  _thread_priority = THREAD_PRIORITY_IDLE;
        else
        {
            _thread_priority = as_int( str );

            if( _thread_priority < -15 )  _thread_priority = -15; 
            if( _thread_priority >  +2 )  _thread_priority =  +2;   // In Windows sollte die Priorität nicht zu hoch werden
        }
    }

    if( element->getAttributeNode( L"include_path" ) )  _include_path = as_string( element->getAttribute( L"include_path" ) );
                                                  else  _include_path = _spooler->include_path();

    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "script" )  _script.set_xml( e, include_path() );
        else
        if( e->tagName == "jobs"   )  load_jobs_from_xml( e );
    }
}

//-------------------------------------------------------------------Spooler::load_threads_from_xml

void Spooler::load_threads_from_xml( Thread_list* liste, const xml::Element_ptr& element )
{
    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "thread" ) 
        {
            string spooler_id = as_string( e->getAttribute( L"spooler_id" ) );
            if( spooler_id.empty()  ||  spooler_id == _spooler_id )
            {
                Sos_ptr<Thread> thread = SOS_NEW( Thread( this ) );
                thread->set_xml( e );
                liste->push_back( thread );
            }
        }
    }
}

//-----------------------------------------------------------------------------Spooler::load_config

void Spooler::load_config( const xml::Element_ptr& config_element )
{
    _config_element  = NULL;
    _config_document = NULL;

    _config_document = config_element->ownerDocument; 
    _config_element  = config_element;


    _tcp_port      = as_int( config_element->getAttribute( L"tcp_port"     ) );
    _udp_port      = as_int( config_element->getAttribute( L"udp_port"     ) );
    _priority_max  = as_int( config_element->getAttribute( L"priority_max" ) );

    if( !_log_directory_as_option_set )  _log_directory = as_string( config_element->getAttribute( L"log_dir"      ) );
    if( !_spooler_param_as_option_set )  _spooler_param = as_string( config_element->getAttribute( L"param"        ) );
    if( !_include_path_as_option_set  )  _include_path  = as_string( config_element->getAttribute( L"include_path" ) );

    _free_threading_default = as_bool( as_string( config_element->getAttribute( L"free_threading" ) ) );

    try
    {
        for( xml::Element_ptr e = config_element->firstChild; e; e = e->nextSibling )
        {
            if( e->tagName == "security" )
            {
                _security.set_xml( e );
            }
            else
            if( e->tagName == "object_set_classes" )
            {
                load_object_set_classes_from_xml( &_object_set_class_list, e );
            }
            else
            if( e->tagName == "holiday" )
            {
                Sos_optional_date_time dt;
                dt.assign( as_string( e->getAttribute( L"date" ) ) );
                _holiday_set.insert( dt.as_time_t() );
            }
            else
            if( e->tagName == "script" )
            {
                _script.set_xml( e, include_path() );
            }
            else
            if( e->tagName == "threads" ) 
            {
                load_threads_from_xml( &_thread_list, e );
            }
        }
    }
    catch( const _com_error& com_error ) { throw_com_error(com_error);  }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
