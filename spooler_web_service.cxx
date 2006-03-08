// $Id$
// Joacim Zschimmer

#include "spooler.h"
#include "../zschimmer/charset.h"

namespace sos {
namespace spooler {

using namespace zschimmer::com;

//--------------------------------------------------------------------------------------------const

const string Web_service::forwarding_job_chain_name           = "scheduler_service_forwarding";
const string Web_service::forwarding_job_chain_forward_state  = "forward";
const string Web_service::forwarding_job_chain_finished_state = "finished";

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
                             "           name='" + Web_service::forwarding_job_chain_name + "'>\n" 
                             "    <job_chain_node state='" + Web_service::forwarding_job_chain_forward_state + "'"
                                                " job='"   + forwarder_job_name + "'/>\n"
                             "    <job_chain_node state='" + Web_service::forwarding_job_chain_finished_state + "'/>\n"
                             "</job_chain>";

//-------------------------------------------------------------------------------------------------

Web_service          ::Class_descriptor   Web_service          ::class_descriptor ( &typelib, "Spooler.Web_service"          , Web_service          ::_methods );
Web_service_operation::Class_descriptor   Web_service_operation::class_descriptor ( &typelib, "Spooler.Web_service_operation", Web_service_operation::_methods );
Web_service_request  ::Class_descriptor   Web_service_request  ::class_descriptor ( &typelib, "Spooler.Web_service_request"  , Web_service_request  ::_methods );
Web_service_response ::Class_descriptor   Web_service_response ::class_descriptor ( &typelib, "Spooler.Web_service_response" , Web_service_response ::_methods );

//-----------------------------------------------------------------------------Subprocess::_methods

const Com_method Web_service::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Web_service,  1, Java_class_name               , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service,  2, Name                          , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service,  3, Request_xslt_stylesheet_path  , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service,  4, Response_xslt_stylesheet_path , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service,  5, Forward_xslt_stylesheet_path  , VT_BSTR    , 0 ),
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
    /*
    ptr<Web_service> default_web_service = Z_NEW( Web_service( _spooler ) );

    default_web_service->set_name( "scheduler_default" );
    default_web_service->set_url_path( "/" );

    add_web_service( default_web_service );
    */

    Command_processor command_processor ( _spooler, Security::seclev_all );

    command_processor.execute_2( job_xml      , Time::now() );
    command_processor.execute_2( job_chain_xml, Time::now() );


    Z_FOR_EACH( Url_web_service_map, _url_web_service_map, ws )
    {
        ws->second->check();
    }
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

//------------------------------------------------------------------------Web_services::dom_element

xml::Element_ptr Web_services::dom_element( const xml::Document_ptr& document, const Show_what& what ) const
{
    xml::Element_ptr result = document.createElement( "web_services" );

    Z_FOR_EACH_CONST( Name_web_service_map, _name_web_service_map, ws )
    {
        result.appendChild( ws->second->dom_element( document, what ) );
    }

    return result;
}

//-------------------------------------------------------------------------Web_service::Web_service

Web_service::Web_service( Spooler* sp )
: 
    Idispatch_implementation( &class_descriptor ),
    Scheduler_object( sp, (Iweb_service*)this, Scheduler_object::type_web_service ),
    _zero_(this+1),
    _next_operation_id(1),
    _timeout( INT_MAX )
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

    if( _request_xslt_stylesheet_path != "" )
        _request_xslt_stylesheet .load_file( _request_xslt_stylesheet_path  );

    if( _response_xslt_stylesheet_path != "" )
        _response_xslt_stylesheet.load_file( _response_xslt_stylesheet_path );

    if( _forward_xslt_stylesheet_path != "" )
        _forward_xslt_stylesheet.load_file( _forward_xslt_stylesheet_path );
}

//-------------------------------------------------------------------------------Web_service::check

void Web_service::check()
{
    if( _job_chain_name != "" )  _spooler->job_chain( _job_chain_name );  // Jobkette ist bekannt?
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
    _job_chain_name                =            element.     getAttribute( "job_chain"               , _job_chain_name                );
    _timeout                       =            element. int_getAttribute( "timeout"                 , _timeout                       );
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
    web_service_element.setAttribute_optional( "job_chain"               , _job_chain_name                );

    if( _debug )
    web_service_element.setAttribute         ( "debug"                   , _debug );

    /*
    if( what & show_web_service_operations )
    {

    }
    */
    
    return web_service_element;
}

//-----------------------------------------------------------------------Web_service::new_operation

ptr<Web_service_operation> Web_service::new_operation( http::Operation* http_operation )
{
    return Z_NEW( Web_service_operation( this, http_operation, _next_operation_id++ ) );
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
        xml::Document_ptr command_document = transform_forward( payload_dom );

        if( _debug )
        {
            _log->debug( "forward_xslt_stylesheet " + _forward_xslt_stylesheet_path + " liefert:\n" );
            _log->debug( command_document.xml( true ) );
            if( _log_xml )  File( _log_filename_prefix + ".forward.xml", "w" ).print( command_document.xml() );
        }

        
        Command_processor command_processor ( _spooler, Security::seclev_all );

        _spooler->_executing_command = false;   // Command_processor() hat es true gesetzt. Trotzdem bei Datenbank-Fehler auf DB warten

        command_processor.set_validate( false );            // <content> enthält unbekannte XML-Elemente <task> und <order>
        command_processor.execute_2( command_document );
        
        /*
        ptr<Order> order = new Order( _spooler );

        order->set_state( forwarding_job_chain_forward_state );
        order->set_payload( Variant( transformed_payload_dom.xml() ) );
        order->add_to_job_chain( _spooler->job_chain( forwarding_job_chain_name ) );
        */
    }
    catch( exception& x )
    {
        _log->error( "Forward: " + string(x.what()) );
        _log->info( payload_dom.xml( true ) );
    }
}

//------------------------------------------------------------------Web_service_operation::_methods

const Com_method Web_service_operation::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Web_service_operation,  1, Java_class_name               , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service_operation,  2, Web_service                   , VT_DISPATCH, 0 ),
    COM_PROPERTY_GET( Web_service_operation,  3, Request                       , VT_DISPATCH, 0 ),
    COM_PROPERTY_GET( Web_service_operation,  4, Response                      , VT_DISPATCH, 0 ),
  //COM_PROPERTY_GET( Web_service_operation,  5, Execute_stylesheets           , VT_DISPATCH, 0 ),
#endif
    {}
};

//-----------------------------------------------------Web_service_operation::Web_service_operation

Web_service_operation::Web_service_operation( Web_service* ws, http::Operation* ht, int operation_id ) 
: 
    _zero_(this+1), 
    Idispatch_implementation( &class_descriptor ),
    Scheduler_object( ws->_spooler, (Iweb_service_operation*)this, Scheduler_object::type_web_service_operation ),
    _web_service(ws), 
    _http_operation(ht),
    _id(operation_id)
{
    _log = Z_NEW( Prefix_log( this, "Web_service " + ws->name() ) );

    _log->set_prefix( obj_name() );
    _log->set_title( obj_name() );

    if( _web_service->_debug  &&  _log->log_level() > log_debug_spooler )  _log->set_log_level( log_debug_spooler );

    if( _web_service->_debug  &&  !string_begins_with( _spooler->log_directory(), "*" ) )   
    {
        _log_filename_prefix = S() << _spooler->log_directory() << "/web_service." << _web_service->_name << "." << _id;

        _log->set_filename( _log_filename_prefix +  ".log" );
        _log->set_remove_after_close( !_web_service->_debug );
        _log->open();
    }

    _request  = Z_NEW( Web_service_request( this ) );
    _response = Z_NEW( Web_service_response( this ) );
}

//----------------------------------------------------Web_service_operation::~Web_service_operation

Web_service_operation::~Web_service_operation()
{
    Z_LOG2( "joacim", __FUNCTION__ << "\n" );
    close();
}

//------------------------------------------------------------------Web_service_operation::obj_name

string Web_service_operation::obj_name() const
{ 
    return S() << _web_service->obj_name() << ":" << _id; 
}

//---------------------------------------------------------------------Web_service_operation::begin

void Web_service_operation::begin()
{
    //if( _web_service->_debug  &&  http_operation->_parser )  _log->debug( "\n" "HTTP request:\n " ), _log->debug( http_operation->_parser->_text ), _log->debug( "" );;

    
    if( _web_service->_job_chain_name == "" )
    {
        execute_stylesheets();
    }
    else
    {
        if( _http_operation->_connection->_security_level < Security::seclev_all )  throw http::Http_exception( http::status_403_forbidden );


        ptr<Order> order = new Order( _spooler );

        //order->_store_in_database = _web_service->_store_order_in_database;
        order->set_http_operation( _http_operation );      // Order wird Eigentümer von Web_service_operation
        order->add_to_job_chain( _spooler->job_chain( _web_service->_job_chain_name ) );
        _http_operation->set_order( order );     // ~Order ruft Http_operation::unlink_order(), der setzt _order = NULL
        _log->info( "Created " + order->obj_name() );

        if( _web_service->_timeout != INT_MAX )
            _http_operation->set_gmtimeout( (double)( ::time(NULL) + _web_service->_timeout ) );
    }
}

//------------------------------------------------------------Web_service_operation::async_continue
/*
bool Web_service_operation::async_continue( Async_operation::Continue_flags )
{
    return true;
}
*/
//------------------------------------------------------------Web_service_operation::async_finished
/*
bool Web_service_operation::async_finished()
{
    return http_response()->is_ready();
    //    response->finish();
}
*/
//---------------------------------------------------------Web_service_operation::process_http__end
/*
ptr<Http_response> Web_service_operation::process_http__end()
{
    ptr<Http_response>  response;
    string              response;
    response = process_request__end( request->body(), request->charset_name() );

    if( _web_service->_debug )  _log->debug( "\n" "HTTP RESPONSE:" ), _log->debug( http_response->header_text() ), _log->debug( response );


    return _http_response;
}
*/
//---------------------------------------------------------------Web_service_operation::dom_element

xml::Element_ptr Web_service_operation::dom_element( const xml::Document_ptr& document, const Show_what& ) const
{
    xml::Element_ptr result = document.createElement( "web_service_operation" );

    result.setAttribute( "web_service", _web_service->name() );

    return result;
}

//---------------------------------------------------------------Web_service_operation::get_Request

STDMETHODIMP Web_service_operation::get_Request( spooler_com::Iweb_service_request** result )
{ 
    *result = _request.copy();
    if( !*result )  return E_POINTER;

    return S_OK; 
}

//--------------------------------------------------------------Web_service_operation::get_Response

STDMETHODIMP Web_service_operation::get_Response( spooler_com::Iweb_service_response** result )  
{ 
    *result = _response.copy();
    if( !*result )  return E_POINTER;

    return S_OK; 
}

//-------------------------------------------------------Web_service_operation::execute_stylesheets
/*
STDMETHODIMP Web_service_operation::Execute_stylesheets()
{
    HRESULT hr = S_OK;
    
    try
    {
        execute_stylesheets();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}
*/
//-------------------------------------------------------Web_service_operation::execute_stylesheets

void Web_service_operation::execute_stylesheets()
{
    // Erstmal nicht: Die Operation wird im Task-Prozess ausgeführt.
    // Erstmal nicht: Scheduler-Methoden, die im Hauptprozess ausgeführt werden sollen, nur über Invoke() aufrufen!


    xml::Document_ptr request_document;
    request_document.create();

    //bool is_xml = true;  //string_begins_with( request_data, "<" );

    //if( is_xml )
    {
        Variant request_data_variant;

        //hr = com_invoke( DISPATCH_PROPERTYGET, this, "String_content", &request_data_variant );
        //if( FAILED(hr) )  return hr;

        if( _web_service->_log_xml )  File( _log_filename_prefix + ".raw_request.txt", "w" ).print( http_request()->body() );
        bool ok = request_document.try_load_xml( http_request()->body(), http_request()->charset_name() );
        if( !ok )
        {
            _log->error( request_document.error_text() );
            throw http::Http_exception( http::status_404_bad_request );
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
    service_request_element.setAttribute( "url", http_request()->url() );

    xml::Element_ptr content_element = service_request_element.append_new_element( "content" );

    //if( is_xml )
    {
        xml::Element_ptr data_element = request_document.replaceChild( service_request_element, request_document.documentElement() );
        content_element.appendChild( data_element );      // request_data anhängen
    }
    //else
    //{
    //    content_element.appendChild( request_document.createTextNode( request_data ) );     // POST-Daten als Text anhängen (nicht spezifiziert)
    //}


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


    if( command_document.documentElement().nodeName() == "service_response" )
    {
        response_document = command_document;
    }
    else
    {
        Command_processor command_processor ( _spooler, Security::seclev_all );
        //command_processor.set_host( _http_operation->_connection->peer_host() );

        command_processor.execute( command_document );

        response_document = _web_service->transform_response( command_processor._answer );

        if( _web_service->_debug )
        {
            _log->debug( "Command response:\n" );
            _log->debug( command_processor._answer.xml( true ) );
        }
    }


    if( _web_service->_log_xml )  File( _log_filename_prefix + ".service_response.xml", "w" ).print( response_document.xml() );

    //if( _web_service->_debug )
    //{
    //    _log->debug( response_document.xml( true ) );
    //    if( _web_service->_log_xml )  File( _log_filename_prefix + ".response.xml", "w" ).print( response_document.xml() );
    //}


    xml::Node_ptr data_node = response_document.select_node( "/service_response/content/*" );
    if( !data_node )  throw_xc( "SCHEDULER-244" );

    http_response()->set_chunk_reader( Z_NEW( http::String_chunk_reader( data_node.xml(), "text/xml" ) ) );

    // Es soll nur ein Element geben!
    data_node = data_node.nextSibling();
    while( data_node  &&  data_node.nodeType() == xml::COMMENT_NODE )  data_node = data_node.nextSibling();
    if( data_node )  throw_xc( "SCHEDULER-245" );

    http_response()->set_ready();
}

//-------------------------------------------------------------Web_service_operation::assert_usable

void Web_service_operation::assert_usable()
{
    if( !_http_operation )  throw_xc( "SCHEDULER-248" );
}

//-------------------------------------------------------------Web_service_operation::Assert_usable

STDMETHODIMP Web_service_operation::Assert_usable()
{
    HRESULT hr = S_OK;
    
    try
    {
        assert_usable();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------Web_service_request::_methods

const Com_method Web_service_request::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Web_service_request,  1, Java_class_name               , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service_request,  3, Header                        , VT_BSTR    , 0, VT_BSTR ),
    COM_PROPERTY_GET( Web_service_request,  4, String_content                , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Web_service_request,  5, Binary_content                , (VARENUM)(VT_ARRAY|VT_UI1), 0 ),
    COM_PROPERTY_GET( Web_service_request,  6, Url                           , VT_BSTR    , 0 ),
#endif
    {}
};

//-----------------------------------------------------Web_service_operation::Web_service_operation

Web_service_request::Web_service_request( Web_service_operation* web_service_operation ) 
: 
    _zero_(this+1), 
    Idispatch_implementation( &class_descriptor ),
    Scheduler_object( web_service_operation->_spooler, (Iweb_service_request*)this, Scheduler_object::type_web_service_request ),
    _web_service_operation(web_service_operation)
{
}

//---------------------------------------------------------------------Web_service_operation::close
    
void Web_service_operation::close()
{ 
    /*
    _order = NULL;

    _http_operation = NULL;
    _response       = NULL;
    _request        = NULL;
    _web_service    = NULL;
    */
}

//---------------------------------------------------------------Web_service_request::assert_usable

void Web_service_request::assert_usable()
{
    if( !_web_service_operation )  throw_xc( "SCHEDULER-248" );
    _web_service_operation->assert_usable();
}

//---------------------------------------------------------------Web_service_request::Assert_usable

STDMETHODIMP Web_service_request::Assert_usable()
{
    HRESULT hr = S_OK;
    
    try
    {
        assert_usable();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------Web_service_request::get_Url

STDMETHODIMP Web_service_request::get_Url( BSTR* result )
{ 
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return String_to_bstr( http_request()->url(), result ); 
}

//------------------------------------------------------------------Web_service_request::get_Header

STDMETHODIMP Web_service_request::get_Header( BSTR name, BSTR* result ) 
{ 
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return String_to_bstr( http_request()->header( string_from_bstr( name ) ), result ); 
}

//----------------------------------------------------------Web_service_request::get_String_content

STDMETHODIMP Web_service_request::get_String_content( BSTR* result )
{
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_request()->get_String_content( result ); 
}

//----------------------------------------------------------Web_service_request::get_Binary_content

STDMETHODIMP Web_service_request::get_Binary_content( SAFEARRAY** result )
{
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_request()->get_Binary_content( result ); 
}

//-------------------------------------------------------------------Web_service_response::_methods

const Com_method Web_service_response::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Web_service_response,  1, Java_class_name               , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Web_service_response,  2, Status_code                                , 0, VT_INT ),
    COM_PROPERTY_PUT( Web_service_response,  3, Header                                     , 0, VT_BSTR, VT_BSTR ),
    COM_PROPERTY_PUT( Web_service_response,  4, String_content                             , 0, VT_BSTR  ),
    COM_PROPERTY_PUT( Web_service_response,  5, Binary_content                             , 0, (VARENUM)(VT_ARRAY|VT_UI1), 0 ),
    COM_METHOD      ( Web_service_response,  6, Send                          , VT_EMPTY   , 0 ),
#endif
    {}
};

//-------------------------------------------------------Web_service_response::Web_service_response

Web_service_response::Web_service_response( Web_service_operation* web_service_operation ) 
: 
    _zero_(this+1), 
    Idispatch_implementation( &class_descriptor ),
    Scheduler_object( web_service_operation->_spooler, (Iweb_service_response*)this, Scheduler_object::type_web_service_response ),
    _web_service_operation(web_service_operation)
{
}

//--------------------------------------------------------------Web_response_request::assert_usable
    
void Web_service_response::assert_usable()
{
    if( !_web_service_operation )  throw_xc( "SCHEDULER-248" );
    _web_service_operation->assert_usable();
}

//--------------------------------------------------------------Web_response_request::Assert_usable

STDMETHODIMP Web_service_response::Assert_usable()
{
    HRESULT hr = S_OK;
    
    try
    {
        assert_usable();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//------------------------------------------------------------Web_service_response::put_Status_code
    
STDMETHODIMP Web_service_response::put_Status_code( int code )
{ 
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_response()->put_Status_code( code );
}

//-----------------------------------------------------------------Web_service_response::put_Header
    
STDMETHODIMP Web_service_response::put_Header( BSTR name, BSTR value )
{ 
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_response()->put_Header( name, value );
}

//-----------------------------------------------------------------Web_service_response::get_Header

STDMETHODIMP Web_service_response::get_Header( BSTR name, BSTR* result )
{ 
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_response()->get_Header( name, result );
}

//-----------------------------------------------------Web_service_response::put_Character_encoding
/*
STDMETHODIMP Web_service_response::put_Character_encoding( BSTR encoding )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        return E_NOTIMPL;
        //_http_response->set_character_encoding( string_from_bstr( encoding ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------Web_service_response::get_Character_encoding

STDMETHODIMP Web_service_response::get_Character_encoding( BSTR* result )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        return E_NOTIMPL;
        //_http_response->set_character_encoding( string_from_bstr( encoding ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------Web_service_response::put_Content_type

STDMETHODIMP Web_service_response::put_Content_type( BSTR content_type )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        //http_response()->set_content_type( string_from_bstr( content_type ) ); 
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------Web_service_response::get_Content_type

STDMETHODIMP Web_service_response::get_Content_type( BSTR* result )
{
    HRESULT hr = S_OK;
    
    try
    {
        return E_NOTIMPL;
        //_http_response->set_content_type( string_from_bstr( content_type ) );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}
*/
//---------------------------------------------------------Web_service_response::put_String_content

STDMETHODIMP Web_service_response::put_String_content( BSTR content_bstr )
{
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_response()->put_String_content( content_bstr );
}

//---------------------------------------------------------Web_service_response::put_Binary_content

STDMETHODIMP Web_service_response::put_Binary_content( SAFEARRAY* safearray )
{
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_response()->put_Binary_content( safearray );
}

//-----------------------------------------------------------------------Web_service_response::Send

STDMETHODIMP Web_service_response::Send() // VARIANT* content, BSTR content_type_bstr )
{
    HRESULT hr = Assert_usable();
    if( FAILED(hr) )  return hr;

    return http_response()->Send();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
