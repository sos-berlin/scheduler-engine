// $Id: spooler_config.cxx,v 1.6 2001/01/25 17:45:45 jz Exp $

//#include <precomp.h>

/*
    Hier sind die Methoden verschiedenener Klassen implementiert, die die Konfiguration laden.
*/

#include "../kram/sos.h"
#include "spooler.h"


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

//--------------------------------------------------------------------------------Security::set_xml

void Security::set_xml( const xml::Element_ptr& security_element ) 
{ 
    bool ignore_unknown_hosts = as_bool( security_element->getAttribute( "ignore_unknown_hosts" ) );

    for( xml::Element_ptr e = security_element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "allowed_host" )
        {
            string    hostname = as_string( e->getAttribute( "host" ) );
            set<Host> host_set;

            try {
                host_set = Host::get_host_set_by_name( hostname );
            }
            catch( const Xc& x )
            {
                _spooler->_log.error( "<allowed_host host=\"" + hostname + "\">  " + x.what() );
                if( !ignore_unknown_hosts )  throw;
            }
            
            FOR_EACH( set<Host>, host_set, h )
            {
                _host_map[ *h ] = as_level( as_string( e->getAttribute( "level" ) ) );
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

    string single_start = as_string( element->getAttribute( "single_start" ) );
    if( !single_start.empty() ) 
    {
        dt.set_time( single_start );
        _begin = dt;
        _retry_interval = latter_day;
        _single_start = true;
    }
    else
    {
        string begin = as_string( element->getAttribute( "begin" ) );
        if( !begin.empty() )  dt.set_time( begin ), _begin = dt;

        string retry_interval = as_string( element->getAttribute( "retry_interval" ) );
        if( !retry_interval.empty() )  _retry_interval = as_double( retry_interval );
    }

    string end = as_string( element->getAttribute( "end" ) );
    if( !end.empty() )  dt.set_time( end ), _end = dt;

    string let_run = as_string( element->getAttribute( "let_run" ) );
    if( !let_run.empty() )  _let_run = as_bool( let_run );

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

            int day = as_int( e->getAttribute( "day" ) );
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
            dt.assign( as_string( e->getAttribute( "date" ) ) );
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
            dt.assign( as_string( e->getAttribute( "date" ) ) );
            _holiday_set.insert( dt.as_time_t() );
        }
    }

    if( !a_day_set )  for( int i = 0; i < 7; i++ )  _weekday_set._days[i] = default_day;
}

} //namespace time

//----------------------------------------------------------------------------------Script::set_xml

void Script::set_xml( const xml::Element_ptr& element )
{
    _language = as_string( element->getAttribute( "language" ) );
    _text = as_string( element->text );
    
    string use_engine = as_string( element->getAttribute( "use_engine" ) );
    
    if( use_engine == "task"   )  _reuse = reuse_task;
    if( use_engine == "job"    )  _reuse = reuse_job;
  //if( use_engine == "global" )  _reuse = reuse_global;


/*
    xml::Element_ptr text_element = element->firstChild;
    if( text_element == NULL )  throw_xc( "SPOOLER-107" );
    _text = as_string( text_element->nodeValue );
*/
}

//------------------------------------------------------------------------Object_set_class::set_xml

void Object_set_class::set_xml( const xml::Element_ptr& element )
{
    _name = as_string( element->getAttribute( "name" ) );

    string iface = as_string( element->getAttribute( "script_interface" ) );
    _object_interface = iface == "oo";

    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "script" )
        {
            _script.set_xml( e );
        }
        else
        if( e->tagName == "level_decls" )
        {
            xml::Element_ptr e2 = e->firstChild;

            while( e2 )
            {
                if( e2->tagName == "level_decl" ) 
                {
                    int    level = as_int( e2->getAttribute( "level" ) );
                    string name  = as_string( e2->getAttribute( "name" ) );

                    _level_map[ level ] = name;
                }

                e2 = e2->nextSibling;
            }
        }

        e = e->nextSibling;
    }
}

//--------------------------------------------------------------------------Level_interval::set_xml

void Level_interval::set_xml( const xml::Element_ptr& element )
{
    _low_level  = as_int( element->getAttribute( "low" ) );
    _high_level = as_int( element->getAttribute( "high" ) );
}

//------------------------------------------------------------------------Object_set_descr::set_xml

void Object_set_descr::set_xml( const xml::Element_ptr& element )
{ 
    _class_name     = as_string( element->getAttribute( "class" ) );
    _level_interval.set_xml( single_element( element, "levels" ) );
}

//-------------------------------------------------------------------------------------Job::set_xml

void Job::set_xml( const xml::Element_ptr& element )
{
    _name             = as_string( element->getAttribute( "name" ) );
  //_rerun            = as_bool( element->getAttribute( "rerun" ) ) ),
  //_stop_after_error = as_bool( element->getAttribute( "stop_after_errorn ) );
    _priority         = as_int( element->getAttribute( "priority" ) );

    string text;

    text = as_string( element->getAttribute( "output_level" ) );
    if( !text.empty() )  _output_level = as_int( text );

    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "object_set" )  _object_set_descr = SOS_NEW( Object_set_descr( e ) );
        else
        if( e->tagName == "script"     )  _script.set_xml( e );
        else
        if( e->tagName == "run_time"   )  _run_time.set_xml( e ); //, cerr << _run_time;
     
        e = e->nextSibling;
    }

}

//--------------------------------------------------------Spooler::load_object_set_classes_from_xml

void Spooler::load_object_set_classes_from_xml( Object_set_class_list* liste, const xml::Element_ptr& element )
{
    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "object_set_class" )  liste->push_back( SOS_NEW( Object_set_class( e ) ) );
        e = e->nextSibling;
    }
}

//----------------------------------------------------------------------Spooler::load_jobs_from_xml

void Spooler::load_jobs_from_xml( Job_list* liste, const xml::Element_ptr& element )
{
    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "job" ) 
        {
            string spooler_id = as_string( e->getAttribute( "spooler_id" ) );
            if( spooler_id.empty()  ||  spooler_id == _spooler_id )
            {
                Sos_ptr<Job> job = SOS_NEW( Job( this ) );
                job->set_xml( e );

                if( job->_object_set_descr )        // job->_object_set_descr->_class ermitteln
                {
                    for( Object_set_class_list::iterator it = _object_set_class_list.begin(); it != _object_set_class_list.end(); it++ )
                    {
                        if( (*it)->_name == job->_object_set_descr->_class_name )  break;
                    }
                    if( it == _object_set_class_list.end() )  throw_xc( "SPOOLER-101", job->_object_set_descr->_class_name );

                    job->_object_set_descr->_class = *it;
                }

                liste->push_back( job );
            }
        }

        e = e->nextSibling;
    }
}

//-----------------------------------------------------------------------------Spooler::load_config

void Spooler::load_config( const xml::Element_ptr& config_element )
{
                                   _tcp_port      = as_int   ( config_element->getAttribute( "tcp_port"      ) );
                                   _udp_port      = as_int   ( config_element->getAttribute( "udp_port"      ) );
                                   _priority_max  = as_int   ( config_element->getAttribute( "priority_max"  ) );
    if( empty( _log_directory ) )  _log_directory = as_string( config_element->getAttribute( "log_dir"       ) );
    if( empty( _spooler_param ) )  _spooler_param = as_string( config_element->getAttribute( "param" ) );


    xml::Element_ptr e = config_element->firstChild;
    while( e )
    {
        if( e->tagName == "security" )
        {
            _security.set_xml( e );
        }
        else
        if( e->tagName == "script" )
        {
            _script.set_xml( e );
        }
        else
        if( e->tagName == "object_set_classes" )
        {
            load_object_set_classes_from_xml( &_object_set_class_list, e );
        }
        else
        if( e->tagName == "jobs" ) 
        {
            load_jobs_from_xml( &_job_list, e );
        }

        e = e->nextSibling;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
