// $Id: spooler_config.cxx,v 1.2 2001/01/20 23:39:16 jz Exp $

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

//-------------------------------------------------------------------------------Script::operator =

void Script::operator = ( const xml::Element_ptr& element )
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

//---------------------------------------------------------------------Object_set_class::operator =

void Object_set_class::operator = ( const xml::Element_ptr& element )
{
    _name = as_string( element->getAttribute( "name" ) );

    string iface = as_string( element->getAttribute( "script_interface" ) );
    _object_interface = iface == "oo";

    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "script" )
        {
            _script = e;
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

//-------------------------------------------------------------------Level_interval::Level_interval

void Level_interval::operator = ( const xml::Element_ptr& element )
{
    _low_level  = as_int( element->getAttribute( "low" ) );
    _high_level = as_int( element->getAttribute( "high" ) );
}

//---------------------------------------------------------------Object_set_descr::Object_set_descr

void Object_set_descr::operator = ( const xml::Element_ptr& element )
{ 
    _class_name     = as_string( element->getAttribute( "class" ) );
    _level_interval = single_element( element, "levels" );
}

//------------------------------------------------------------------------------Day_set::operator =

void Day_set::operator = ( const xml::Element_ptr& element )
{
    memset( _days, (char)false, sizeof _days );

    if( element == NULL )  return;

    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "day" )
        {
            int day = as_int( e->getAttribute( "day" ) );
            if( (uint)day >= NO_OF(_days) )  throw_xc( "SPOOLER-INVALID-DAY", day );
            _days[day] = true;
        }

        e = e->nextSibling;
    }
}

//-------------------------------------------------------------------------------Run_time::Run_time

Run_time::Run_time( const xml::Element_ptr& element )
: 
    _zero_(this+1)
{
    Sos_optional_date_time  dt;
    
    _retry_period = as_double( element->getAttribute( "retry_period" ) );
    
    string begin = as_string( element->getAttribute( "begin" ) );
    if( !begin.empty() )
    {
        dt.set_time( begin );
        _begin_time_of_day = dt;

        dt.set_time( as_string( element->getAttribute( "end" ) ) );
        _end_time_of_day = dt;
    }
    else
    {
        string single_start = as_string( element->getAttribute( "single_start" ) );
        if( !single_start.empty() ) 
        {
            dt.set_time( single_start );
            _begin_time_of_day = dt;
            _retry_period = latter_day;
        }
    }

    _let_run = as_bool( element->getAttribute( "let_run" ) );



    bool             a_day_set = false;
    xml::Element_ptr e = element->firstChild;
    while( e )
    {
        if( e->tagName == "date" )
        {
            a_day_set = true;
            dt.assign( as_string( e->getAttribute( "date" ) ) );
            _date_set.insert( dt.as_time_t() );
        }
        else
        if( e->tagName == "weekdays" )
        {
            a_day_set = true;
            _weekday_set = e;
        }
        else
        if( e->tagName == "monthdays" )
        {
            a_day_set = true;
            _monthday_set = e;
        }
        else
        if( e->tagName == "ultimos" )
        {
            a_day_set = true;
            _ultimo_set = e;
        }
        else
        if( e->tagName == "holiday" )
        {
            dt.assign( as_string( e->getAttribute( "date" ) ) );
            _holiday_set.insert( dt.as_time_t() );
        }

        e = e->nextSibling;
    }

    if( !a_day_set )  for( int i = 0; i < 7; i++ )  _weekday_set._days[i] = true;

    check();
}

//----------------------------------------------------------------------------------Job::operator =

void Job::operator = ( const xml::Element_ptr& element )
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
        if( e->tagName == "script"     )  _script = e;
        else
        if( e->tagName == "run_time"   )  _run_time = e;
     
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
                *job = e;

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
        if( e->tagName == "script" )
        {
            _script = e;
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
