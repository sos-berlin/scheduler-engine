// $Id$

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
namespace scheduler {

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
    string inc              = include_path;
    int    linenr_base      = element.line_number(); //? element.line_number() - 1 : 0;

    if( !inc.empty() )  { char c = inc[inc.length()-1];  if( c != '/'  &&  c != '\\' )  inc += "/"; }


    for( xml::Node_ptr n = element.firstChild(); n; n = n.nextSibling() )
    {
        if( n.line_number() )  linenr_base = n.line_number();

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
              //result.add( n.line_number(), text, mod_time );
                result.add( linenr_base, text, mod_time );
                linenr_base += count_if( text.begin(), text.end(), bind2nd( equal_to<char>(), '\n' ) );     // Für MSXML
                break;

            case xml::ELEMENT_NODE:     // <include file="..."/>
            {
                xml::Element_ptr e = n;
                string filename = subst_env( e.getAttribute( "file" ) );

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
    if( !security_element )  return;

    bool ignore_unknown_hosts = as_bool( security_element.getAttribute( "ignore_unknown_hosts" ), true );

    DOM_FOR_EACH_ELEMENT( security_element, e )
    {
        if( e.nodeName_is( "allowed_host" ) )
        {
            string    hostname = e.getAttribute( "host" );
            set<Ip_address> host_set;

            try {
                host_set = Ip_address::get_host_set_by_name( hostname );
            }
            catch( exception& x )
            {
                _spooler->log()->warn( "<allowed_host host=\"" + hostname + "\">  " + x.what() );
                if( !ignore_unknown_hosts )  throw;
            }
            
            FOR_EACH( set<Ip_address>, host_set, h )
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
                    int    level = e2.int_getAttribute( "level", 0 );
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
        if( e.nodeName_is( "object_set_class" ) )  liste->push_back( SOS_NEW( Object_set_class( this, _log, e, xml_mod_time ) ) );
    }
}

//--------------------------------------------------------------------------Spooler_thread::set_dom
/*
void Spooler_thread::set_dom( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    string str;

    _name = element.getAttribute( "name" );

    str = element.getAttribute( "free_threading" );
    if( !str.empty() )  z::throw_xc( "SCHEDULER-189", "free_threading" );    //_free_threading = as_bool( str );

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

    if( element.getAttributeNode( "include_path" ) )  z::throw_xc( "SCHEDULER-189", "<thread include_path=>" );  //_include_path = element.getAttribute( "include_path" );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "script" ) )  z::throw_xc( "SCHEDULER-189", "<script in thread>" );  //_module.set_dom( e, xml_mod_time, include_path() );
        else
        if( e.nodeName_is( "jobs"   ) )  _spooler->load_jobs_from_xml( e, xml_mod_time );   // Zur Kompatibilität
    }
}
*/
//-----------------------------------------------------------------------------Spooler::load_config

void Spooler::load_config( const xml::Element_ptr& config_element, const Time& xml_mod_time, const string& source_filename )
{
    //config_element.ownerDocument().validate_dtd_string( dtd_string );  // Nur <spooler> <config> validieren, nicht die Kommandos. Deshalb hier (s. spooler_command.cxx)


    _config_element  = NULL;
    _config_document = NULL;

    try
    {
        DOM_FOR_EACH_ELEMENT( config_element, e )
        {
            if( e.nodeName_is( "base" ) )
            {
                string config_filename = make_absolute_filename( directory_of_path( source_filename ), subst_env( e.getAttribute( "file" )  ) );
                
                Command_processor cp ( this, Security::seclev_all );
                cp._load_config_immediately = true;
                cp.execute_file( config_filename );


                // Für http://.../show_config?:   Das Basisdokument in <base> ablegen, damit wir eine große Konfiguration bekommen. Nur für HTTP.
                /*
                try
                {
                    xml::Document_ptr d;
                    d.create();
                    int ok = d.try_load_xml( string_from_file( config_filename ) );
                    if( ok )  e.appendChild( d.documentElement().cloneNode( true ) );
                }
                catch( exception& ) {}
                */
            }
        }

        _config_document = config_element.ownerDocument();
        _config_element = config_element;

        if( !_tcp_port_as_option_set )
        {
            _tcp_port  = config_element.int_getAttribute( "port"         , _tcp_port     );
            _tcp_port  = config_element.int_getAttribute( "tcp_port"     , _tcp_port     );
        }

        if( !_udp_port_as_option_set )
        {
            _udp_port  = config_element.int_getAttribute( "port"         , _udp_port     );
            _udp_port  = config_element.int_getAttribute( "udp_port"     , _udp_port     );
        }

        if( !_ip_address_as_option_set )
            _ip_address = config_element.getAttribute( "ip_address" );

        _priority_max  = config_element.int_getAttribute( "priority_max" , _priority_max );

#     ifdef _DEBUG
      //_max_threads   = config_element.int_getAttribute( "threads"      , _max_threads  );
#     endif
          
        if( _max_threads < 1 )  _max_threads = 1;

        _config_java_class_path = subst_env( config_element.getAttribute( "java_class_path" ) );
        _config_java_options    = subst_env( config_element.getAttribute( "java_options"    ) );

        string log_dir =   config_element.getAttribute( "log_dir" );

        if( !_log_directory_as_option_set && log_dir != "" )  _log_directory = log_dir;
        if( !_spooler_param_as_option_set )  _spooler_param = config_element.getAttribute( "param"        , _spooler_param );
        if( !_include_path_as_option_set  )  _include_path  = config_element.getAttribute( "include_path" , _include_path  );

      //_free_threading_default = config_element.bool_getAttribute( "free_threading", _free_threading_default );

        string host_and_port = config_element.getAttribute( "main_scheduler" );
        if( host_and_port != "" )  _main_scheduler_connection = Z_NEW( Xml_client_connection( this, host_and_port ) );

        set_mail_xslt_stylesheet_path( subst_env( config_element.getAttribute( "mail_xslt_stylesheet" ) ) );

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
            if( e.nodeName_is( "web_services" ) )
            {
                _web_services.add_web_services( e );
            }
            else
            if( e.nodeName_is( "holiday" ) )
            {
                Sos_optional_date_time dt;
                dt.assign( e.getAttribute( "date" ) );
                _holiday_set.insert( dt.as_time_t() );
            }
            else
            if( e.nodeName_is( "process_classes" ) )
            {
                if( !e.bool_getAttribute( "ignore", false )  &&  !_ignore_process_classes )
                    load_process_classes_from_dom( e, xml_mod_time );
            }
            else
            if( e.nodeName_is( "script" ) )
            {
                _module.set_dom( e, xml_mod_time, include_path() );
            }
            else
            if( e.nodeName_is( "jobs" ) )
            {
                load_jobs_from_xml( e, xml_mod_time );
            }
            else
            if( e.nodeName_is( "job_chains" ) )
            {
                order_subsystem()->load_job_chains_from_xml( e );
            }
            else
            if( e.nodeName_is( "commands" ) )
            {
                if( !_commands_document ) 
                {
                    _commands_document.create();
                    _commands_document.appendChild( _commands_document.createElement( "commands" ) );       // Wurzel
                }

                DOM_FOR_EACH_ELEMENT( e, command_element )
                {
                    _commands_document.documentElement().appendChild( command_element.cloneNode(true) );
                }
            }
        }
    }
    catch( const _com_error& com_error ) { throw_com_error(com_error);  }
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
