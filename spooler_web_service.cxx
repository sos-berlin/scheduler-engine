// $Id$
// Joacim Zschimmer

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

//-------------------------------------------------------------------------------------------------

Web_service::Class_descriptor   Web_service::class_descriptor ( &typelib, "Spooler.Web_service", Web_service::_methods );

//-----------------------------------------------------------------------------Subprocess::_methods

const Com_method Web_service::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Subprocess,  1, Java_class_name               , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Subprocess,  2, Name                          , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Subprocess,  3, Forward_xslt_stylesheet_path  , VT_BSTR    , 0 ),
#endif
    {}
};

//-------------------------------------------------------------------Web_services::add_web_services

void Web_services::add_web_services( const xml::Element_ptr& web_services_element )
{
    DOM_FOR_EACH_ELEMENT( web_services_element, e )
    {
        if( e.nodeName_is( "web_service" ) )
        {
            ptr<Web_service> web_service = web_service_by_url_path_or_null( web_services_element.getAttribute( "name" ) );
            if( !web_service )   web_service = Z_NEW( Web_service( _spooler ) );

            web_service->set_dom( e );
            add_web_service( web_service );
        }
    }
}

//--------------------------------------------------------------------Web_services::add_web_service

void Web_services::add_web_service( Web_service* web_service )
{
    if( web_service_by_name_or_null    ( web_service->name()     ) )  throw_xc( "SCHEDULER-236", web_service->name()     );
    if( web_service_by_url_path_or_null( web_service->url_path() ) )  throw_xc( "SCHEDULER-238", web_service->url_path() );

    _name_web_service_map[ web_service->name() ] = web_service;
    _url_web_service_map[ web_service->url_path() ] = web_service;
}

//-------------------------------------------------------------------------------Web_services::init

void Web_services::init()
{
    Command_processor command_processor ( _spooler );

    command_processor.execute_2( job_xml      , Time::now() );
    command_processor.execute_2( job_chain_xml, Time::now() );
}

//----------------------------------------------------Web_services::web_service_by_url_path_or_null

Web_service* Web_services::web_service_by_url_path_or_null( const string& url_path )
{
    Url_web_service_map::iterator ws = _url_web_service_map.find( url_path );
    return ws != _url_web_service_map.end()? ws->second : NULL;
}

//----------------------------------------------------------------Web_services::web_service_by_name

Web_service* Web_services::web_service_by_name( const string& name )
{
    Web_service* result = web_service_by_name_or_null( name );
    if( !result )  throw_xc( "SCHEDULER-235", name );

    return result;
}

//--------------------------------------------------------Web_services::web_service_by_name_or_null

Web_service* Web_services::web_service_by_name_or_null( const string& name )
{
    Name_web_service_map::iterator ws = _name_web_service_map.find( name );
    return ws != _name_web_service_map.end()? ws->second : NULL;
}

//-------------------------------------------------------------------------Web_service::Web_service

Web_service::Web_service( Spooler* sp )
: 
    Idispatch_implementation( &class_descriptor ),
    Scheduler_object( sp, (Iweb_service*)this, Scheduler_object::type_web_service ),
    _zero_(this+1),
    _next_transaction_number(1)
{
    _log = Z_NEW( Prefix_log( this, "Web_service" ) );
}

//------------------------------------------------------------------------Web_service::~Web_service

Web_service::~Web_service()
{
}

//--------------------------------------------------------------------------------Web_service::load

void Web_service::load()
{
    _log->set_prefix( obj_name() );
    _log->set_title( obj_name() );

    if( _debug  &&  _log->log_level() > log_debug_spooler )  _log->set_log_level( log_debug_spooler );
    
    if( !string_begins_with( _spooler->log_directory(), "*" ) ) 
    {
        _log_filename_prefix = S() << _spooler->log_directory() << "/web_service." << _name;
        _log->set_filename( _log_filename_prefix + ".log" );
        _log->set_remove_after_close( false );
        _log->open();

        if( _debug )  _log_xml = true;
    }

    _request_xslt_stylesheet .load_file( _request_xslt_stylesheet_path  );
    _response_xslt_stylesheet.load_file( _response_xslt_stylesheet_path );

    if( _forward_xslt_stylesheet_path != "" )
        _forward_xslt_stylesheet.load_file( _forward_xslt_stylesheet_path );
}

//-----------------------------------------------------------------------------Web_service::set_dom
    
void Web_service::set_dom( const xml::Element_ptr& element, const Time& )
{
    if( !element )  return;

    _name                          =            element.     getAttribute( "name" );
    _url_path                      = subst_env( element.     getAttribute( "url_path"                , _url_path                      ) );
    _request_xslt_stylesheet_path  = subst_env( element.     getAttribute( "request_xslt_stylesheet" , _request_xslt_stylesheet_path  ) );
    _response_xslt_stylesheet_path = subst_env( element.     getAttribute( "response_xslt_stylesheet", _response_xslt_stylesheet_path ) );
    _forward_xslt_stylesheet_path  = subst_env( element.     getAttribute( "forward_xslt_stylesheet" , _forward_xslt_stylesheet_path  ) );
    _debug                         =            element.bool_getAttribute( "debug"                   , _debug                         );


    if( _forward_xslt_stylesheet_path != "" )
    {
        // Interne Jobkette und Job jetzt sichtbar machen

        _spooler->job_chain( forwarding_job_chain_name )->set_visible( true );
        _spooler->get_job( forwarder_job_name, true )->set_visible( true );
    }

    load();
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

    if( _debug )
    web_service_element.setAttribute         ( "debug"                   , _debug );
    
    return web_service_element;
}

//---------------------------------------------------------------------Web_service::new_transaction

ptr<Web_service_transaction> Web_service::new_transaction( Http_processor* http_processor )
{
    return Z_NEW( Web_service_transaction( this, http_processor, _next_transaction_number++ ) );
}

//-------------------------------------------------------------------Web_service::transform_request

xml::Document_ptr Web_service::transform_request( const xml::Document_ptr& request_document )
{
    xml::Document_ptr result = _request_xslt_stylesheet.apply( request_document );
    if( !result.documentElement() )  throw_xc( "SCHEDULER-237", _request_xslt_stylesheet_path );
    return result;
}

//------------------------------------------------------------------Web_service::transform_response

xml::Document_ptr Web_service::transform_response( const xml::Document_ptr& command_answer_document )
{
    xml::Document_ptr result = _response_xslt_stylesheet.apply( command_answer_document );
    if( !result.documentElement() )  throw_xc( "SCHEDULER-237", _response_xslt_stylesheet_path );
    return result;
}

//------------------------------------------------------------------Web_service::transform_response

xml::Document_ptr Web_service::transform_forward( const xml::Document_ptr& order_or_task_document )
{
    xml::Document_ptr result = _forward_xslt_stylesheet.apply( order_or_task_document );
    if( !result.documentElement() )  throw_xc( "SCHEDULER-237", _forward_xslt_stylesheet_path );

    if( result.documentElement().nodeName() != "service_request" )  throw_xc( "SCHEDULER-242", _forward_xslt_stylesheet_path );

    return result;
}

//-----------------------------------------------------------------------Web_service::forward_order

void Web_service::forward_order( const Order& order, Job* last_job )
{
    if( _forward_xslt_stylesheet_path != "" )
    {
        xml::Document_ptr order_document = order.dom( show_all );
        if( last_job )  order_document.documentElement().setAttribute( "last_job", last_job->name() );   // Stylesheet will den letzten Job haben
        forward( order_document );
    }
}

//------------------------------------------------------------------------Web_service::forward_task

void Web_service::forward_task( const Task& task )
{
    if( _forward_xslt_stylesheet_path != "" )
    {
        forward( task.dom( show_all ) );
    }
}

//-----------------------------------------------------------------------------Web_service::forward

void Web_service::forward( const xml::Document_ptr& payload_dom )
{
    try
    {
        xml::Document_ptr transformed_payload_dom = transform_forward( payload_dom );

        if( _debug )
        {
            _log->debug( "forward_xslt_stylesheet " + _forward_xslt_stylesheet_path + " liefert:\n" );
            _log->debug( transformed_payload_dom.xml( true ) );
            if( _log_xml )  File( _log_filename_prefix + ".command.xml", "w" ).print( transformed_payload_dom.xml() );
        }


        ptr<Order> order = new Order( _spooler );

        order->set_state( forwarding_job_chain_forward_state );
        order->set_payload( Variant( transformed_payload_dom.xml() ) );
        order->add_to_job_chain( _spooler->job_chain( forwarding_job_chain_name ) );
    }
    catch( exception& x )
    {
        _log->error( "Forward: " + string(x.what()) );
        _log->info( payload_dom.xml( true ) );
    }
}

//--------------------------------------------------eb_service_transaction::Web_service_transaction

Web_service_transaction::Web_service_transaction( Web_service* ws, Http_processor* ht, int transaction_number ) 
: 
    _zero_(this+1), 
    Scheduler_object( ws->_spooler, this, Scheduler_object::type_web_service_transaction ),
    _web_service(ws), 
    _http_processor(ht),
    _transaction_number(transaction_number)
{
    _log = Z_NEW( Prefix_log( this, "Web_service " + ws->name() ) );

    _log->set_prefix( obj_name() );
    _log->set_title( obj_name() );

    if( _web_service->_debug  &&  _log->log_level() > log_debug_spooler )  _log->set_log_level( log_debug_spooler );

    if( !string_begins_with( _spooler->log_directory(), "*" ) )   
    {
        _log_filename_prefix = S() << _spooler->log_directory() << "/web_service." << _web_service->_name << "." << _transaction_number;

        _log->set_filename( _log_filename_prefix +  ".log" );
        _log->set_remove_after_close( !_web_service->_debug );
        _log->open();
    }
}

//----------------------------------------------------------------Web_service_transaction::obj_name

string Web_service_transaction::obj_name() const
{ 
    return S() << _web_service->obj_name() << ":" << _transaction_number; 
}

//---------------------------------------------------------Web_service_transaction::process_request

ptr<Http_response> Web_service_transaction::process_http( Http_processor* http_processor )
{
    Http_request*       http_request     = http_processor->_http_request;
    ptr<Http_response>  http_response;
    string              response;

    if( _web_service->_debug  &&  http_processor->_http_parser )  _log->debug( "\n" "HTTP request:\n " ), _log->debug( http_processor->_http_parser->_text ), _log->debug( "" );;


    try
    {
        response = process_request( http_request->_body );
        http_response = Z_NEW( Http_response( http_request, Z_NEW( String_chunk_reader( response ) ), "text/xml" ) );
    }
    catch( exception& x )
    {
        _log->error( x.what() );

        http_response = Z_NEW( Http_response( http_request, NULL, "" ) );
        http_response->set_status( 500, "Internal Server Error" );
    }


    http_response->finish();
    if( _web_service->_debug )  _log->debug( "\n" "HTTP RESPONSE:" ), _log->debug( http_response->header_text() ), _log->debug( response );


    return http_response;
}

//---------------------------------------------------------Web_service_transaction::process_request

string Web_service_transaction::process_request( const string& request_data )
{
    xml::Document_ptr request_document;
    request_document.create();


    bool is_xml = string_begins_with( request_data, "<" );

    if( is_xml )
    {
        if( _web_service->_log_xml )  File( _log_filename_prefix + ".raw_request.xml", "w" ).print( request_data );

        int ok = request_document.try_load_xml( request_data );
        if( !ok )
        {
            string text = request_document.error_text();
            _spooler->_log.error( text );       // Log ist möglicherweise noch nicht geöffnet
            throw_xc( "XML-ERROR", text );
        }
    }


    // In <service_request> einwickeln:
    //
    // <service_request>
    //     <web_service>...</web_service>
    //     <content> ...request_data... </content>
    // </service_request>


    xml::Element_ptr service_request_element = request_document.createElement( "service_request" );
    service_request_element.appendChild( _web_service->dom_element( request_document ) );           // <web_service> anhängen

    xml::Element_ptr content_element = service_request_element.append_new_element( "content" );

    if( is_xml )
    {
        xml::Element_ptr data_element = request_document.replaceChild( service_request_element, request_document.documentElement() );
        service_request_element.appendChild( data_element );      // request_data anhängen
    }
    else
    {
        service_request_element.appendChild( request_document.createTextNode( request_data ) );     // POST-Daten als Text anhängen (nicht spezifiziert)
    }


    if( _web_service->_debug )
    {
        _log->debug( "service_request.xml:\n" );
        _log->debug( request_document.xml( true ) );
        _log->debug( "\n" );
        if( _web_service->_log_xml )  File( _log_filename_prefix + ".service_request.xml", "w" ).print( request_document.xml() );
    }


    
    // <service_request> ---xslt---> XML-Kommando

    xml::Document_ptr command_document = _web_service->transform_request( request_document );   
    
    if( _web_service->_debug )
    {
        _log->debug( "request_xslt_stylesheet " + _web_service->_request_xslt_stylesheet_path + " liefert:\n" );
        _log->debug( command_document.xml( true ) );
        if( _web_service->_log_xml )  File( _log_filename_prefix + ".command.xml", "w" ).print( command_document.xml() );
    }



    // KOMMANDO AUSFÜHREN

    xml::Document_ptr response_document;


    if( command_document.documentElement().nodeName() != "service_response" )
    {
        Command_processor command_processor ( _spooler );
        command_processor.set_host( _http_processor->_host );

        command_processor.execute_2( command_document, Time::now() );

        if( _web_service->_debug )
        {
            _log->debug( "Command response:\n" );
            _log->debug( command_processor._answer.xml( true ) );
            if( _web_service->_log_xml )  File( _log_filename_prefix + ".response.xml", "w" ).print( response_document.xml() );
        }

        response_document = _web_service->transform_response( command_processor._answer );
    }
    else
    {
        response_document = command_document;
    }


    //if( _web_service->_debug )
    //{
    //    _log->debug( response_document.xml( true ) );
    //    if( _web_service->_log_xml )  File( _log_filename_prefix + ".response.xml", "w" ).print( response_document.xml() );
    //}


    return response_document.xml();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
