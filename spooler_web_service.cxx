// $Id$

#include "spooler.h"

namespace sos {
namespace spooler {

const string forwarding_job_chain_name           = "scheduler_service_forwarding";
const string forwarding_job_chain_forward_state  = "forward";
const string forwarding_job_chain_finished_state = "finished";
const string forwarder_job_name                  = "scheduler_service_forwarder";
const string forwarder_job_title                 = "Web-service forwarder";
const string forwarder_job_java_class_name       = "sos.spooler.jobs.Web_service_forwarder";

const string job_xml = "<job visible='no'\n"
                       "     name='" + forwarder_job_name + "'\n"
                       "     order='yes'\n"
                       "     title='" + forwarder_job_title + "'>\n"
                       "    <script language='java' java_class='" + forwarder_job_java_class_name + "'/>\n"
                       "</job>";

const string job_chain_xml = "<job_chain visible='no'\n"
                             "           name='" + forwarding_job_chain_name + "'>\n" 
                             "    <job_chain_node state='" + forwarding_job_chain_forward_state + "'"
                                                " job='"   + forwarder_job_name + "'/>\n"
                             "    <job_chain_node state='" + forwarding_job_chain_finished_state + "'/>\n"
                             "</job_chain>";

//-----------------------------------------------------------------------Spooler::init_web_services
    
void Spooler::init_web_services()
{
    Command_processor command_processor ( _spooler );

    command_processor.execute_2( job_xml      , Time::now() );
    command_processor.execute_2( job_chain_xml, Time::now() );
}

//---------------------------------------------------------Spooler::web_service_by_url_path_or_null
    
Web_service* Spooler::web_service_by_url_path_or_null( const string& url_path )
{
    Z_FOR_EACH( Web_service_map, _web_service_map, ws )
    {
        if( ws->second->url_path() == url_path )  return ws->second;
    }

    return NULL;
}

//---------------------------------------------------------------------Spooler::web_service_by_name

Web_service* Spooler::web_service_by_name( const string& name )
{
    Web_service_map::iterator ws = _web_service_map.find( name );
    if( ws == _web_service_map.end() )  throw_xc( "SCHEDULER-235", name );

    return ws->second;
}

//-----------------------------------------------------------------------------Web_service::set_dom
    
void Web_service::set_dom( const xml::Element_ptr& element, const Time& )
{
    _name                          = element.getAttribute( "name" );
    _url_path                      = element.getAttribute( "url_path" );
    _request_xslt_stylesheet_path  = element.getAttribute( "request_xslt_stylesheet" );
    _response_xslt_stylesheet_path = element.getAttribute( "response_xslt_stylesheet" );
    _forward_xslt_stylesheet_path  = element.getAttribute( "forward_xslt_stylesheet" );
    _debug                         = element.bool_getAttribute( "debug", _debug );


    if( _forward_xslt_stylesheet_path != "" )
    {
        // Interne Jobkette und Job jetzt sichtbar machen

        _spooler->job_chain( forwarding_job_chain_name )->set_visible( true );
        _spooler->get_job( forwarder_job_name )->set_visible( true );
    }
}

//-------------------------------------------------------------------------Web_service::dom_element

xml::Element_ptr Web_service::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr web_service_element = document.createElement( "web_service" );

    web_service_element.setAttribute         ( "name"                    , _name                          );
    web_service_element.setAttribute         ( "url_path"                , _url_path                      );
    web_service_element.setAttribute_optional( "request_xslt_stylesheet" , _request_xslt_stylesheet_path  );
    web_service_element.setAttribute_optional( "response_xslt_stylesheet", _response_xslt_stylesheet_path );
    web_service_element.setAttribute_optional( "forward_xslt_stylesheet" , _forward_xslt_stylesheet_path  );

    return web_service_element;
}

//---------------------------------------------------------------------Web_service::new_transaction

ptr<Web_service_transaction> Web_service::new_transaction( Http_processor* http_processor )
{
    return Z_NEW( Web_service_transaction( this, http_processor ) );
}

//-------------------------------------------------------------------Web_service::transform_request

xml::Document_ptr Web_service::transform_request( const xml::Document_ptr& request_document )
{
    if( !_request_xslt_stylesheet.loaded() )  
    {
        _request_xslt_stylesheet.load_file( _request_xslt_stylesheet_path );
    }

    return _request_xslt_stylesheet.apply( request_document );
}

//------------------------------------------------------------------Web_service::transform_response

xml::Document_ptr Web_service::transform_response( const xml::Document_ptr& command_answer_document )
{
    if( !_response_xslt_stylesheet.loaded() )  
    {
        _response_xslt_stylesheet.load_file( _response_xslt_stylesheet_path );
    }

    return _response_xslt_stylesheet.apply( command_answer_document );
}

//-------------------------------------------------------------------Web_service::transform_forward
/*
string Web_service::transform_forward( const string& request_xml )
{
}
*/
//-----------------------------------------------------------------------Web_service::forward_order

void Web_service::forward_order( Order* order )
{
    order->set_state( forwarding_job_chain_forward_state );
    order->add_to_job_chain( _spooler->job_chain( forwarding_job_chain_name ) );
}

//------------------------------------------------------------------------Web_service::forward_task

void Web_service::forward_task( Task* task )
{
    ptr<Order> order = new Order( _spooler );

    xml::Document_ptr task_document;
    task_document.create();
    task_document.appendChild( task->dom_element( task_document, Show_what( show_log ) ) );

    order->set_payload( Variant( task_document.xml() ) );

    forward_order( order );
}

//---------------------------------------------------------Web_service_transaction::process_request

string Web_service_transaction::process_request( const string& request_data )
{
    xml::Document_ptr request_document;
    request_document.create();


    bool is_xml = string_begins_with( request_data, "<" );

    if( is_xml )
    {
        int ok = request_document.try_load_xml( request_data );
        if( !ok )
        {
            string text = request_document.error_text();
            _spooler->_log.error( text );       // Log ist möglicherweise noch nicht geöffnet
            throw_xc( "XML-ERROR", text );
        }
    }


    // In <service_request> einwickleln

    xml::Element_ptr service_request_element = request_document.createElement( "service_request" );
    service_request_element.appendChild( _web_service->dom_element( service_request_element.ownerDocument() ) );  // <web_service> anhängen

    xml::Element_ptr body_element = request_document.createElement( "body" );

    if( is_xml )
    {
        service_request_element.appendChild( request_document.documentElement() );      // request_data anhängen
    }
    else
    {
        service_request_element.appendChild( request_document.createTextNode( request_data ) );     // POST-Daten als Text anhängen (nicht spezifiziert)
    }


    request_document.appendChild( service_request_element );


    // KOMMANDO AUSFÜHREN

    xml::Document_ptr command_document = _web_service->transform_request( request_document );   

    xml::Document_ptr response_document;


    if( command_document.documentElement().nodeName() != "service_response" )
    {
        Command_processor command_processor ( _spooler );
        command_processor.set_host( _http_processor->_host );

        command_processor.execute_command( command_document.documentElement(), Time::now() );

        response_document = _web_service->transform_response( command_processor._answer );
    }
    else
    {
        response_document = command_document;
    }


    return response_document.xml();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
