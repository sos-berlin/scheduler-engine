// $Id: spooler_config.cxx,v 1.53 2002/12/11 08:50:36 jz Exp $

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
    int              count = 0;
    xml::Element_ptr result;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( name ) )  result = e, count++;
    }

    if( count > 1 )  throw_xc( "SOS-1423", name );

    return result;
/*
    xml::NodeList_ptr list = element.getElementsByTagName( name );

    int len = list.length();
    if( len == 0 )  return xml::Element_ptr();

    if( len > 1 )  throw_xc( "SOS-1423", name );

    return list.item(0);
*/
}

//---------------------------------------------------------------------------default_single_element

xml::Element_ptr default_single_element( const xml::Element_ptr& element, const string& name )
{
    xml::Element_ptr result = optional_single_element( element, name );
    if( result == NULL ) {
        return element.ownerDocument().createElement( name );     // Künstliches Element mit Attributwerten aus der DTD
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
    
    xml::Element_ptr text_element = result.firstChild();
    if( text_element == NULL )  return empty_string;

    return text_element.nodeValue();
}

//-----------------------------------------------------------------------text_from_xml_with_include

Source_with_parts text_from_xml_with_include( const xml::Element_ptr& element, const Time& mod_time, const string& include_path )
{
    Source_with_parts result;
    string text;
    string inc = include_path;
    int    linenr_base = 0;

    if( !inc.empty() )  { char c = inc[inc.length()-1];  if( c != '/'  &&  c != '\\' )  inc += "/"; }


    for( xml::Node_ptr n = element.firstChild(); n; n = n.nextSibling() )
    {
        string text;

        switch( n.nodeType() )
        {
            case xml::CDATA_SECTION_NODE:
            {
                xml::CDATASection_ptr c = n;
                text = c.data();
                goto TEXT;
            }

            case xml::TEXT_NODE:
            {
                xml::Text_ptr t = n;
                text = t.data();
                goto TEXT;

            }

            TEXT:
                result.add( linenr_base, text, mod_time );
                linenr_base += count_if( text.begin(), text.end(), bind2nd( equal_to<char>(), '\n' ) );
                break;

            case xml::ELEMENT_NODE:     // <include file="..."/>
            {
                xml::Element_ptr e = n;
                string filename = e.getAttribute( "file" );

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

                result.add( 0, string_from_file( filename ), modification_time_of_file( filename ) );
                break;
            }

            default: ;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------Security::set_dom

void Security::set_dom( const xml::Element_ptr& security_element ) 
{ 
    bool ignore_unknown_hosts = as_bool( security_element.getAttribute( "ignore_unknown_hosts" ) );

    DOM_FOR_EACH_ELEMENT( security_element, e )
    {
        if( e.nodeName_is( "allowed_host" ) )
        {
            string    hostname = e.getAttribute( "host" );
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
                _host_map[ *h ] = as_level( e.getAttribute( "level" ) );
            }
        }
    }
}

//------------------------------------------------------------------------Object_set_class::set_dom

void Object_set_class::set_dom( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    _name = element.getAttribute( "name" );

    string iface = element.getAttribute( "script_interface", "oo" );
    _object_interface = iface == "oo";

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "script" ) )
        {
            _module.set_dom( e, xml_mod_time, _spooler->include_path() );
        }
        else
        if( e.nodeName_is( "level_decls" ) )
        {
            DOM_FOR_EACH_ELEMENT( e, e2 )
            {
                if( e2.nodeName_is( "level_decl" ) )
                {
                    int    level = as_int( e2.getAttribute( "level" ) );
                    string name  = e2.getAttribute( "name" );

                    _level_map[ level ] = name;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------Level_interval::set_dom

void Level_interval::set_dom( const xml::Element_ptr& element )
{
    _low_level  = as_int( element.getAttribute( "low" ) );
    _high_level = as_int( element.getAttribute( "high" ) );
}

//------------------------------------------------------------------------Object_set_descr::set_dom

void Object_set_descr::set_dom( const xml::Element_ptr& element )
{ 
    _class_name     = element.getAttribute( "class" );
    _level_interval.set_dom( single_element( element, "levels" ) );
}

//--------------------------------------------------------Spooler::load_object_set_classes_from_xml

void Spooler::load_object_set_classes_from_xml( Object_set_class_list* liste, const xml::Element_ptr& element, const Time& xml_mod_time )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "object_set_class" ) )  liste->push_back( SOS_NEW( Object_set_class( this, &_prefix_log, e, xml_mod_time ) ) );
    }
}

//--------------------------------------------------------------------------Spooler_thread::set_dom

void Spooler_thread::set_dom( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    string str;

    _name = element.getAttribute( "name" );

    str = element.getAttribute( "free_threading" );
    if( !str.empty() )  _free_threading = as_bool( str );

#   ifdef Z_WINDOWS
        str = element.getAttribute( "priority" );
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
#   endif

    if( element.getAttributeNode( "include_path" ) )  _include_path = element.getAttribute( "include_path" );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "script" ) )  _module.set_dom( e, xml_mod_time, include_path() );
        else
        if( e.nodeName_is( "jobs"   ) )  load_jobs_from_xml( e, xml_mod_time );
    }
}

//-------------------------------------------------------------------Spooler::load_threads_from_xml

void Spooler::load_threads_from_xml( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "thread" ) )
        {
            string spooler_id = e.getAttribute( "spooler_id" );
            if( _manual  ||  spooler_id.empty()  ||  spooler_id == _spooler_id )
            {
                string thread_name = e.getAttribute( "name" );
                ptr<Spooler_thread> thread = get_thread_or_null( thread_name );
                if( !thread )  
                {
                    thread = Z_NEW( Spooler_thread( this ) );
                    _thread_list.push_back( thread );

                    if( !thread->_free_threading )  _spooler_thread_list.push_back( thread );
                }

                thread->set_dom( e, xml_mod_time );
            }
        }
    }
}

//-----------------------------------------------------------------------------Spooler::load_config

void Spooler::load_config( const xml::Element_ptr& config_element, const Time& xml_mod_time, const string& source_filename )
{
    _config_element  = NULL;
    _config_document = NULL;

    try
    {
        {DOM_FOR_EACH_ELEMENT( config_element, e )
        {
            if( e.nodeName_is( "base" ) )
            {
                string config_filename = e.getAttribute( "file" );
                
                Command_processor cp ( this );
                cp._load_config_immediately = true;
                cp.execute_file( make_absolute_filename( directory_of_path( source_filename ), config_filename ) );
            }
        }}

        _config_document = config_element.ownerDocument();
        _config_element = config_element;

        _tcp_port      = as_int( config_element.getAttribute( "tcp_port"     , as_string( _tcp_port )     ) );
        _udp_port      = as_int( config_element.getAttribute( "udp_port"     , as_string( _udp_port )     ) );
        _priority_max  = as_int( config_element.getAttribute( "priority_max" , as_string( _priority_max ) ) );

        _java_vm._config_class_path = config_element.getAttribute( "java_class_path" );

        string log_dir =   config_element.getAttribute( "log_dir" );

        if( !_log_directory_as_option_set && log_dir != "" )  _log_directory = log_dir;
        if( !_spooler_param_as_option_set )  _spooler_param = config_element.getAttribute( "param"        , _spooler_param );
        if( !_include_path_as_option_set  )  _include_path  = config_element.getAttribute( "include_path" , _include_path  );

        _free_threading_default = config_element.bool_getAttribute( "free_threading", _free_threading_default );

        DOM_FOR_EACH_ELEMENT( config_element, e )
        {
            if( e.nodeName_is( "security" ) )
            {
                _security.clear();
                _security.set_dom( e );
            }
            else
            if( e.nodeName_is( "object_set_classes" ) )
            {
                _object_set_class_list.clear();
                load_object_set_classes_from_xml( &_object_set_class_list, e, xml_mod_time );
            }
            else
            if( e.nodeName_is( "holidays" ) )
            {
                _holiday_set.clear();

                DOM_FOR_EACH_ELEMENT( e, e2 )
                {
                    if( e2.nodeName_is( "holiday" ) )
                    {
                        Sos_optional_date_time dt;
                        dt.assign( e2.getAttribute( "date" ) );
                        _holiday_set.insert( dt.as_time_t() );
                    }
                }
            }
            else
            if( e.nodeName_is( "holiday" ) )
            {
                Sos_optional_date_time dt;
                dt.assign( e.getAttribute( "date" ) );
                _holiday_set.insert( dt.as_time_t() );
            }
            else
            if( e.nodeName_is( "script" ) )
            {
                _module.set_dom( e, xml_mod_time, include_path() );
            }
            else
            if( e.nodeName_is( "threads" ) )
            {
                load_threads_from_xml( e, xml_mod_time );
            }
        }
    }
    catch( const _com_error& com_error ) { throw_com_error(com_error);  }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
