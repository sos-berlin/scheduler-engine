// $Id: spooler_xml.cxx,v 1.7 2001/01/05 20:31:22 jz Exp $

//#include <precomp.h>


#include "../kram/sos.h"
#include "../kram/olestd.h"
#include "../file/anyfile.h"
#include "spooler.h"


namespace sos {
namespace spooler {


//-----------------------------------------------------------------------------------single_element

xml::Element_ptr optional_single_element( xml::Element_ptr element, const string& name )
{
    xml::NodeList_ptr list = element->getElementsByTagName( as_dom_string( name ) );

    int len = list->length;
    if( len == 0 )  return xml::Element_ptr();
    if( len > 1 )  throw_xc( "SOS-1423", name );

    return list->Getitem(0);
}

//-----------------------------------------------------------------------------------single_element

xml::Element_ptr single_element( xml::Element_ptr element, const string& name )
{
    xml::Element_ptr result = optional_single_element( element, name );
    if( result == NULL )  throw_xc( "SOS-1422", name );

    return result;
}

//-----------------------------------------------------------------------------------single_element

string optional_single_element_as_text( xml::Element_ptr element, const string& name )
{
    xml::Element_ptr result = optional_single_element( element, name );
    if( result == NULL )  return empty_string;

    return as_string( result->firstChild->nodeValue );
}

//---------------------------------------------------------------Object_set_class::Object_set_class

Object_set_class::Object_set_class( xml::Element_ptr element )
{
    _name = as_string( element->getAttribute( "name" ) );

    xml::Element_ptr e = element->firstChild;

    while( e )
    {
        if( e->tagName == "script" )
        {
            _script = as_string( e->firstChild->nodeValue );
            _script_language = as_string( e->getAttribute( "language" ) );
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

Level_interval::Level_interval( xml::Element_ptr element )
:
    _low_level ( as_int( element->getAttribute( "low" ) ) ),
    _high_level( as_int( element->getAttribute( "high" ) ) )
{
}

//---------------------------------------------------------------Object_set_descr::Object_set_descr

Object_set_descr::Object_set_descr( xml::Element_ptr element )
: 
    _class_name( as_string( element->getAttribute( "class" ) ) ),
    _level_interval( single_element( element, "levels" ) )
{
}

//---------------------------------------------------------------------------------Day_set::Day_set

Day_set::Day_set( xml::Element_ptr element )
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

Run_time::Run_time( xml::Element_ptr element )
: 
    _zero_(this+1)
{
    Sos_optional_date_time  dt;
    xml::Element_ptr        e = element->firstChild;

    _retry_period = as_double( element->getAttribute( "retry_period" ) );

    while( e )
    {
        if( e->tagName == "time_of_day" )
        {
            dt.set_time( as_string( e->getAttribute( "begin" ) ) );
            _begin_time_of_day = dt.hour() * 60*60 + dt.minute() * 60 + dt.second();

            dt.set_time( as_string( e->getAttribute( "end" ) ) );
            _end_time_of_day = dt.hour() * 60*60 + dt.minute() * 60 + dt.second();
        }
        else
        if( e->tagName == "date" )
        {
            dt.assign( as_string( e->getAttribute( "date" ) ) );
            _date_set.insert( dt.as_time_t() );
        }
        else
        if( e->tagName == "weekdays" )
        {
            _weekday_set = e;
        }
        else
        if( e->tagName == "monthdays" )
        {
            _monthday_set = e;
        }
        else
        if( e->tagName == "ultimos" )
        {
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

    check();
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( xml::Element_ptr element )
: 
    _zero_(this+1)
{
    _name               = as_string( element->getAttribute( "name" ) );
    _object_set_descr   = single_element( element, "object_set" );
    _output_level       = as_int( as_string( element->getAttribute( "output_level" ) ) );
    _run_time           = single_element( element, "run_time" );
    _rerun              = as_bool( as_string( element->getAttribute( "rerun" ) ) );
}

//--------------------------------------------------------Spooler::load_object_set_classes_from_xml

void Spooler::load_object_set_classes_from_xml( Object_set_class_list* list, xml::Element_ptr element )
{
    xml::NodeList_ptr node_list = element->childNodes;

    for( int i = 0; i < node_list->length; i++ )
    {
        xml::Node_ptr n = node_list->Getitem(i);

        if( n->nodeName == "object_set_class" )  list->push_back( SOS_NEW( Object_set_class( n ) ) );
    }
}

//----------------------------------------------------------------------Spooler::load_jobs_from_xml

void Spooler::load_jobs_from_xml( Job_list* list, xml::Element_ptr element )
{
    xml::NodeList_ptr node_list = element->childNodes;

    for( int i = 0; i < node_list->length; i++ )
    {
        xml::Node_ptr n = node_list->Getitem(i);

        if( n->nodeName == "job" ) 
        {
            Sos_ptr<Job> job = SOS_NEW( Job( n ) );

            for( Object_set_class_list::iterator it = _object_set_class_list.begin(); it != _object_set_class_list.end(); it++ )
            {
                if( (*it)->_name == job->_object_set_descr._class_name )  break;
            }
            if( it == _object_set_class_list.end() )  throw_xc( "SPOOLER-101", job->_object_set_descr._class_name );

            job->_object_set_descr._class = *it;

            list->push_back( job );
        }
    }
}

//--------------------------------------------------------------------------------Spooler::load_xml

void Spooler::load_xml()
{
    try 
    {
        xml::Document_ptr document ( __uuidof(xml::DOMDocument30), NULL );

        int ok = document->loadXML( as_dom_string( file_as_string( "spooler_config.xml" ) ) );
        if( !ok ) 
        {
            xml::IXMLDOMParseErrorPtr error = document->GetparseError();

            string text = w_as_string( error->reason );
            if( text[ text.length()-1 ] == '\n' )  text = as_string( text.c_str(), text.length() - 1 );
            if( text[ text.length()-1 ] == '\r' )  text = as_string( text.c_str(), text.length() - 1 );

            text += ", code="   + as_hex_string( error->errorCode );
            text += ", line="   + as_string( error->line );
            text += ", column=" + as_string( error->linepos );

            throw_xc( "XML-ERROR", text );
        }

        xml::Element_ptr  spooler_config = document->documentElement;
        xml::NodeList_ptr node_list      = spooler_config->childNodes;

        for( int i = 0; i < node_list->length; i++ )
        {
            xml::Node_ptr node = node_list->Getitem(i);

            if( node->nodeName == "object_set_classes" )
            {
                load_object_set_classes_from_xml( &_object_set_class_list, node );
            }
            else
            if( node->nodeName == "jobs" ) 
            {
                load_jobs_from_xml( &_job_list, node );
            }
        }
    }
    catch( const _com_error& com_error )  { throw_com_error( com_error ); }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
