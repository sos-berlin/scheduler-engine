// $Id: spooler_config.cxx,v 1.44 2002/11/01 09:27:10 jz Exp $

//#include <precomp.h>

/*
    Hier sind die Methoden verschiedenener Klassen implementiert, die die Konfiguration laden.
*/

#include "spooler.h"
#include "../file/anyfile.h"

#include "../zschimmer/z_com.h"

using namespace zschimmer::com;


#include <algorithm>


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

Source_with_parts text_from_xml_with_include( const xml::Element_ptr& element, const string& include_path )
{
    Source_with_parts result;
    string text;
    string inc = include_path;
    int    linenr_base = 0;

    if( !inc.empty() )  { char c = inc[inc.length()-1];  if( c != '/'  &&  c != '\\' )  inc += "/"; }


    for( xml::Node_ptr n = element->firstChild; n; n = n->nextSibling )
    {
        string text;

        switch( n->GetnodeType() )
        {
            case xml::NODE_CDATA_SECTION:
            {
                xml::Cdata_section_ptr c = n;
                text = as_string( c->data );
                goto TEXT;
            }

            case xml::NODE_TEXT:
            {
                xml::Text_ptr t = n;
                text = as_string( t->data );
                goto TEXT;

            }

            TEXT:
                result.add( linenr_base, text );
                linenr_base += count_if( text.begin(), text.end(), bind2nd( equal_to<char>(), '\n' ) );
                break;

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
                     
                result.add( 0, file_as_string( filename ) );
                break;
            }

            default: ;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------Security::set_xml

void Security::set_xml( const xml::Element_ptr& security_element ) 
{ 
    bool ignore_unknown_hosts = as_bool( security_element->getAttribute( L"ignore_unknown_hosts" ) );

    DOM_FOR_ALL_ELEMENTS( security_element, e )
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

//------------------------------------------------------------------------Object_set_class::set_xml

void Object_set_class::set_xml( const xml::Element_ptr& element )
{
    _name = as_string( element->getAttribute( L"name" ) );

    string iface = as_string( variant_default( element->getAttribute( L"script_interface" ), "oo" ) );
    _object_interface = iface == "oo";

    DOM_FOR_ALL_ELEMENTS( element, e )
    {
        if( e->tagName == "script" )
        {
            _module.set_xml( e, _spooler->include_path() );
        }
        else
        if( e->tagName == "level_decls" )
        {
            DOM_FOR_ALL_ELEMENTS( e, e2 )
            {
                if( e2->tagName == "level_decl" ) 
                {
                    int    level = int_from_variant( e2->getAttribute( L"level" ) );
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
    _low_level  = int_from_variant( element->getAttribute( L"low" ) );
    _high_level = int_from_variant( element->getAttribute( L"high" ) );
}

//------------------------------------------------------------------------Object_set_descr::set_xml

void Object_set_descr::set_xml( const xml::Element_ptr& element )
{ 
    _class_name     = as_string( element->getAttribute( L"class" ) );
    _level_interval.set_xml( single_element( element, "levels" ) );
}

//--------------------------------------------------------Spooler::load_object_set_classes_from_xml

void Spooler::load_object_set_classes_from_xml( Object_set_class_list* liste, const xml::Element_ptr& element )
{
    DOM_FOR_ALL_ELEMENTS( element, e )
    {
        if( e->tagName == "object_set_class" )  liste->push_back( SOS_NEW( Object_set_class( this, &_prefix_log, e ) ) );
    }
}

//----------------------------------------------------------------------------------Thread::set_xml

void Thread::set_xml( const xml::Element_ptr& element )
{
    string str;

    _name = as_string( element->getAttribute( L"name" ) );

    str = as_string( element->getAttribute( L"free_threading" ) );
    if( !str.empty() )  _free_threading = as_bool( str );

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

    DOM_FOR_ALL_ELEMENTS( element, e )
    {
        if( e->tagName == "script" )  _module.set_xml( e, include_path() );
        else
        if( e->tagName == "jobs"   )  load_jobs_from_xml( e );
    }
}

//-------------------------------------------------------------------Spooler::load_threads_from_xml

void Spooler::load_threads_from_xml( const xml::Element_ptr& element )
{
    DOM_FOR_ALL_ELEMENTS( element, e )
    {
        if( e->tagName == "thread" ) 
        {
            string spooler_id = as_string( e->getAttribute( "spooler_id" ) );
            if( _manual  ||  spooler_id.empty()  ||  spooler_id == _spooler_id )
            {
                string thread_name = as_string( e->getAttribute( "name" ) );
                Sos_ptr<Thread> thread = get_thread_or_null( thread_name );
                if( !thread )  
                {
                    thread = SOS_NEW( Thread( this ) );
                    _thread_list.push_back( thread );
                }

                thread->set_xml( e );
            }
        }
    }
}

//-----------------------------------------------------------------------------Spooler::load_config

void Spooler::load_config( const xml::Element_ptr& config_element, const string& source_filename )
{
    _config_element  = NULL;
    _config_document = NULL;

    try
    {
        {DOM_FOR_ALL_ELEMENTS( config_element, e )
        {
            if( e->tagName == "base" )
            {
                string config_filename = as_string( e->getAttribute( "file" ) );
                
                Command_processor cp ( this );
                cp._load_config_immediately = true;
                cp._source_filename = make_absolute_filename( directory_of_path( source_filename ), config_filename );
                cp.execute_2( file_as_string( cp._source_filename ) );
            }
        }}

        _config_document = config_element->ownerDocument; 
        _config_element  = config_element;

        _tcp_port      = int_from_variant( variant_default( config_element->getAttribute( L"tcp_port"     ), _tcp_port     ) );
        _udp_port      = int_from_variant( variant_default( config_element->getAttribute( L"udp_port"     ), _udp_port     ) );
        _priority_max  = int_from_variant( variant_default( config_element->getAttribute( L"priority_max" ), _priority_max ) );

        _java_vm._class_path = as_string( config_element->getAttribute( L"java_class_path" ) );

        string log_dir =   as_string( config_element->getAttribute( L"log_dir"         ) );

        if( !_log_directory_as_option_set && log_dir != "" )  _log_directory = log_dir;
        if( !_spooler_param_as_option_set )  _spooler_param = as_string( variant_default( config_element->getAttribute( L"param"        ), _spooler_param ) );
        if( !_include_path_as_option_set  )  _include_path  = as_string( variant_default( config_element->getAttribute( L"include_path" ), _include_path  ) );

        _free_threading_default = as_bool( as_string( variant_default( config_element->getAttribute( L"free_threading" ), _free_threading_default ) ) );

        DOM_FOR_ALL_ELEMENTS( config_element, e )
        {
            if( e->tagName == "security" )
            {
                _security.clear();
                _security.set_xml( e );
            }
            else
            if( e->tagName == "object_set_classes" )
            {
                _object_set_class_list.clear();
                load_object_set_classes_from_xml( &_object_set_class_list, e );
            }
            else
            if( e->tagName == "holidays" )
            {
                _holiday_set.clear();

                DOM_FOR_ALL_ELEMENTS( e, e2 )
                {
                    if( e2->tagName == "holiday" )
                    {
                        Sos_optional_date_time dt;
                        dt.assign( as_string( e2->getAttribute( L"date" ) ) );
                        _holiday_set.insert( dt.as_time_t() );
                    }
                }
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
                _module.set_xml( e, include_path() );
            }
            else
            if( e->tagName == "threads" ) 
            {
                load_threads_from_xml( e );
            }
        }
    }
    catch( const _com_error& com_error ) { throw_com_error(com_error);  }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
