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

    if( count > 1 )  z::throw_xc( "SOS-1423", name );

    return result;
/*
    xml::NodeList_ptr list = element.getElementsByTagName( name );

    int len = list.length();
    if( len == 0 )  return xml::Element_ptr();

    if( len > 1 )  z::throw_xc( "SOS-1423", name );

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
    if( result == NULL )  z::throw_xc( "SOS-1422", name );

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

//-----------------------------------------------------------------------------Spooler::load_config

void Spooler::load_config( const xml::Element_ptr& config_element, const string& source_filename )
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
                cp.execute_config_file( config_filename );


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

        if( !_configuration_directories_as_option_set[ confdir_local ] )
            _configuration_directories[ confdir_local ] = config_element.getAttribute( "configuration_directory", _configuration_directories[ confdir_local ] );

        _configuration_start_job_after_added    = Absolute_path( root_path, config_element.getAttribute( "configuration_add_event"   , _configuration_start_job_after_added    ) );
        _configuration_start_job_after_modified = Absolute_path( root_path, config_element.getAttribute( "configuration_modify_event", _configuration_start_job_after_modified ) );
        _configuration_start_job_after_deleted  = Absolute_path( root_path, config_element.getAttribute( "configuration_delete_event", _configuration_start_job_after_deleted  ) );

        if( !_central_configuration_directory_as_option_set )
            _central_configuration_directory = config_element.getAttribute( "central_configuration_directory", _central_configuration_directory );

        if( !_tcp_port_as_option_set )
        {
            _tcp_port  = config_element.int_getAttribute( "port"         , _tcp_port );
            _tcp_port  = config_element.int_getAttribute( "tcp_port"     , _tcp_port );
        }

        if( !_udp_port_as_option_set )
        {
            _udp_port  = config_element.int_getAttribute( "port"         , _udp_port );
            _udp_port  = config_element.int_getAttribute( "udp_port"     , _udp_port );
        }

        if( !_ip_address_as_option_set )
            _ip_address = config_element.getAttribute( "ip_address" );

      //_priority_max  = config_element.int_getAttribute( "priority_max" , _priority_max );
         
        _config_java_class_path = subst_env( config_element.getAttribute( "java_class_path" ) );
        _config_java_options    = subst_env( config_element.getAttribute( "java_options"    ) );

        string log_dir =   config_element.getAttribute( "log_dir" );

        if( !_log_directory_as_option_set && log_dir != "" )  _log_directory = log_dir;
        if( !_spooler_param_as_option_set )  _spooler_param = config_element.getAttribute( "param"        , _spooler_param );
        if( !_include_path_as_option_set  )  _include_path  = subst_env( config_element.getAttribute( "include_path" , _include_path  ) );

      //_free_threading_default = config_element.bool_getAttribute( "free_threading", _free_threading_default );

        string host_and_port = config_element.getAttribute( "supervisor" );
        if( host_and_port == "" )  host_and_port = config_element.getAttribute( "main_scheduler" );   //TODO Veraltetes Attribut
        if( host_and_port != "" )  _supervisor_client = supervisor::new_supervisor_client( this, host_and_port );

        set_mail_xslt_stylesheet_path( subst_env( config_element.getAttribute( "mail_xslt_stylesheet" ) ) );

        DOM_FOR_EACH_ELEMENT( config_element, e )
        {
            if( e.nodeName_is( "params" ) )
            {
                _variables->set_dom( e, &_spooler->_variable_set_map );
            }
            else
            if( e.nodeName_is( "security" ) )
            {
                _security.set_dom( e );
            }
            else
            if( e.nodeName_is( "cluster" ) )
            {
                _cluster_configuration.set_dom( e );
            }
            else
            if( e.nodeName_is( "holidays" ) )
            {
                _holidays.set_dom( (File_based*)NULL, e );
            }
            else
            if( e.nodeName_is( "holiday" ) )
            {
                _holidays.set_dom( (File_based*)NULL, e );
            }
            else
            if( e.nodeName_is( "http_server" ) )
            {
                _web_services->set_dom( e );
            }
            else
            if( e.nodeName_is( "web_services" ) )       // Veraltet, heißt jetzt <http_server>
            {
                _log->info( message_string( "SCHEDULER-847", "<http_server>", e.nodeName() ) );
                _web_services->set_dom( e );
            }
            else
            if( e.nodeName_is( "process_classes" ) )
            {
                if( !_ignore_process_classes_set )  _ignore_process_classes = e.bool_getAttribute( "ignore", _ignore_process_classes );
                if( !_ignore_process_classes )  root_folder()->process_class_folder()->set_dom( e );
            }
            else
            if( e.nodeName_is( "schedules" ) )
            {
                root_folder()->schedule_folder()->set_dom( e );
            }
            else
            if( e.nodeName_is( "locks" ) )
            {
                root_folder()->lock_folder()->set_dom( e );
            }
            else
            if( e.nodeName_is( "scheduler_script" )  ||
                e.nodeName_is( "script" )               )
            {
                _scheduler_script_subsystem->set_dom( e );
            }
            else
            if( e.nodeName_is( "jobs" ) )
            {
                root_folder()->job_folder()->set_dom( e );
            }
            else
            if( e.nodeName_is( "job_chains" ) )
            {
                root_folder()->job_chain_folder()->set_dom( e );
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
