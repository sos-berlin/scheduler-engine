// $Id$

#include "spooler.h"

// com_remote.cxx ändern: R4, R8, DATE rechner-unabhängig übertragen!

/*
    Scheduler setzt einen Server auf. 


    VERBINDUNGSAUFBAU
        Verbindung über <config tcp_port=>, also mit spooler_communication koppeln.

        com_remote.cxx: Bei Verbindungsaufbau einen Header übertragen, am besten ein XML-Dokument:

        Länge in Bytes <LF> XML-Dokument

        Oder HTTP-POST.



    ANMELDUNG
        Client überträgt Referenz auf Spooler, Server legt das in _spooler_proxy ab.
        Der Client übergibt zugleich ein paar Standardinfos: id, version (mit set_property?).

        Server antwortet mit Referenz auf seinen Spooler und des Remote_scheduler-Objekts.


    ABMELDUNG
        Client ruft logoff() des Remote_scheduler-Objekts auf 


    VERBINDUNGSABBRUCH
        Wenn kein logoff(): _connection_lost = true;

        Remote_scheduler wird nie gelöscht, es bleibt für immer im Remote_scheduler_register.





    ASYNCHRONER BETRIEB
        TCP-Verbindung wird von spooler_communication.cxx (Channel) gehalten.
        
        Channel wird mit Remote_scheduler verknüpft.
        Abstrakte Klasse für Remote_scheduler, Http_server, TCP-Kommando.

        Bei Verbindungsende wird Channel gelöscht, nicht aber Remote_scheduler.



    AUFRUF DES ANGEMELDETEN SCHEDULER ALS SERVER
        Eigentlich ist der angemeldete Scheduler der Client.

        Aufruf asynchron: Remote_schedler._remote_scheduler_proxy->call__begin();

        Gleichzeitige Aufrufe beider Seiten vermeiden.
        Oder: Ein Aufruf des anderen Schedulers darf nicht zu einem Rückruf führen.
        Das würde bei gleichzeitigen Aufrufen überkreuz com_remote.cxx nicht unterstützen. 
        Und wäre auch verwirrend. Wir brauchen das bestimmt nicht (versuch eine Sperre einzubauen)

*/

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const
    
const int main_scheduler_retry_time = 60;

//---------------------------------------------------------------------Xml_client_connection::start
/*    
void Xml_client_connection::start( ,const Host_and_port& host_and_port )
{
    call_connect( host_and_port );
};
*/

//------------------------------------------------------ml_client_connection::Xml_client_connection
    
Xml_client_connection::Xml_client_connection( Spooler* sp, const Host_and_port& host_and_port )
: 
    _zero_(this+1), 
    _spooler(sp),
    _log( sp->_log ),
    _host_and_port(host_and_port)
{
    _log.set_prefix( S() << "Haupt-Scheduler " + _host_and_port );
    set_async_next_gmtime( 0 );
    //set_async_child( &_socket_operation );
}

//----------------------------------------------------Xml_client_connection::~Xml_client_connection

Xml_client_connection::~Xml_client_connection()
{
    _socket_operation = NULL;
}

//-----------------------------------------------------Xml_client_connection::add_to_socket_manager

void Xml_client_connection::add_to_socket_manager( Socket_manager* manager )
{
    _socket_manager = manager;

    set_async_manager( manager );

    set_async_manager( manager );
    set_async_next_gmtime( 0 );     // Sofort starten!
}

//---------------------------------------------------------Xml_client_connection::async_state_text_

string Xml_client_connection::async_state_text_()
{
    switch( _state )
    {
        case s_initial:     return "intial";
        case s_connecting:  return "connecting";
        case s_stand_by:    return "stand_by";
        case s_sending:     return "sending";
        case s_waiting:     return "waiting_for_response";
        case s_receiving:   return "receiving";
        default:            return "state=" + as_int( _state );
    }
}

//-----------------------------------------------------------Xml_client_connection::async_continue_
    
bool Xml_client_connection::async_continue_( bool wait )
{
    bool something_done = false;

    try
    {
        if( _socket_operation )
        {
            if( !_socket_operation->async_finished() )  return false;
            _socket_operation->async_check_error();

            if( _socket_operation->_eof )  throw_xc( "SCHEDULER-224" );
        }

        //if( !_socket_operation->async_signaled() )
        {
        }


        switch( _state )
        {
            case s_initial:
                //if( async_next_gmtime() > ::time(NULL) )  return false;
                if( !async_next_gmtime_reached() )  return false;
                //set_async_next_gmtime( double_time_max );

                _socket_operation = Z_NEW( Buffered_socket_operation( _socket_manager ) );

                _socket_operation->set_async_parent( this );

                _socket_manager->add_socket_operation( _socket_operation );

                _socket_operation->connect__start( _host_and_port );
                _socket_operation->set_keepalive( true );

                _state = s_connecting;
                something_done = true;
                break;

            case s_connecting:
            {
                if( !_socket_operation->async_finished() )  break;

                S xml;
                xml << "<register_remote_scheduler";
                xml << " scheduler_id='" << _spooler->_spooler_id  << "'";
                xml << " tcp_port='"     << _spooler->_tcp_port    << "'";
                xml << " version='"      << _spooler->_version     << "'";
                xml << "/>";

                _socket_operation->send__start( xml );
                _state = s_sending;
                //break;
            }

            case s_sending:
                if( !_socket_operation->async_finished() )  break;
                
                _state = s_waiting;
                //break;

            case s_waiting:
                _recv_data = "";

            case s_receiving:
            {
                _socket_operation->recv__continue();
                if( _socket_operation->_eof )  throw_xc( "SCHEDULER-224" );

                string data = _socket_operation->recv_data();
                if( data.length() == 0 )  break;

                _state = s_receiving;
                something_done = true;

                _socket_operation->recv_clear();
                _recv_data.append( data );
                

                if( !_xml_end_finder.is_complete( data.data(), data.length() ) )  break;

                _state = s_stand_by;

                //_log.info( "ANTWORT: " + _recv_data );

                xml::Document_ptr response_document;
                response_document.create();

                response_document.load_xml( _recv_data );
                _recv_data = "";

                DOM_FOR_EACH_ELEMENT( response_document.documentElement(), e1 )
                    if( e1.nodeName_is( "answer" ) )
                        DOM_FOR_EACH_ELEMENT( e1, e2 )
                            if( e2.nodeName_is( "ERROR" ) )  throw_xc( "SCHEDULER-223", e2.getAttribute( "text" ) );

                _log.info( S() << "Scheduler ist registriert" );
                break;
            }

            default: ;
        }
    }
    catch( exception& x )
    {
        _log.warn( x.what() );

        if( _socket_operation )
        {
            try
            {
                _socket_operation->close();
            }
            catch( exception& ) {}

            _socket_operation->async_reset_error();
        }

        _state = s_initial;
        set_async_next_gmtime( ::time(NULL) + main_scheduler_retry_time );
        something_done = true;
    }

    return something_done;
}

//-----------------------------------------------------------Remote_scheduler_register::get_or_null

Remote_scheduler* Remote_scheduler_register::get_or_null( const Host_and_port& hp )
{
    Map::iterator it = _map.find( hp );
    return it != _map.end()? it->second : NULL;
}

//-------------------------------------------------------------------Remote_scheduler_register::add

void Remote_scheduler_register::add( Remote_scheduler* remote_scheduler )
{
    _map[ remote_scheduler->_host_and_port ] = remote_scheduler;
}

//-----------------------------------------------------------Remote_scheduler_register::dom_element

xml::Element_ptr Remote_scheduler_register::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr result = document.createElement( "remote_schedulers" );

    int n = 0;
    int connected_count = 0;

    Z_FOR_EACH( Map, _map, s )
    {
        Remote_scheduler* remote_scheduler = s->second;

        n++;
        if( remote_scheduler->_is_connected )  connected_count++;

        if( show & show_remote_schedulers )  result.appendChild( remote_scheduler->dom_element( document, show ) );
    }

    result.setAttribute( "count"    , n );
    result.setAttribute( "connected", connected_count );

    return result;
}

//------------------------------------------------------------------------Remote_scheduler::set_dom

void Remote_scheduler::set_dom( const xml::Element_ptr& register_scheduler_element )
{
    if( register_scheduler_element.bool_getAttribute( "logoff", false ) )
    {
        _logged_on = false;
    }
    else
    {
        _logged_on    = true;
        _is_connected = true;
        _scheduler_id = register_scheduler_element.getAttribute( "scheduler_id" );
        _version      = register_scheduler_element.getAttribute( "version" );
        _connected_at = Time::now();
    }


    _error = NULL;

    DOM_FOR_EACH_ELEMENT( register_scheduler_element, e )
    {
        if( e.nodeName_is( "ERROR" ) )  _error = xc_from_dom_error( e );
    }
}

//--------------------------------------------------------------------Remote_scheduler::dom_element

xml::Element_ptr Remote_scheduler::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr result = document.createElement( "remote_scheduler" );

    result.setAttribute         ( "ip"              , _host_and_port._host.ip_string() );
    result.setAttribute_optional( "hostname"        , _host_and_port._host.name() );
    result.setAttribute         ( "tcp_port"        , _host_and_port._port );
    result.setAttribute_optional( "scheduler_id"    , _scheduler_id );
    result.setAttribute         ( "version"         , _version );

  //result.setAttribute         ( "logged_on"       , _logged_on? "yes" : "no" );

    result.setAttribute         ( "connected"       , _is_connected? "yes" : "no" );

    if( _connected_at )
    result.setAttribute         ( "connected_at"    , _connected_at.as_string() );

    if( _disconnected_at )
    result.setAttribute         ( "disconnected_at" , _disconnected_at.as_string() );

    if( _error )
    append_error_element( result, _error );

    return result;
}

//----------------------------------------------------------Remote_scheduler::connection_lost_event
  
void Remote_scheduler::connection_lost_event( const exception* x )
{
    _disconnected_at = Time::now();
    _is_connected = false;

    if( _logged_on )  _error = x;
}

//----------------------------------bject_server_processor_channel::Object_server_processor_channel
/*
Object_server_processor_channel::Object_server_processor_channel( Communication::Channel* ch )
: 
    Communication::Processor_channel( ch )
{
    //_session = Z_NEW( object_server::Session );
}

//-------------------------------------------------Object_server_processor::Object_server_processor

Object_server_processor::Object_server_processor( Object_server_processor_channel* ch )
:
    Communication::Processor(ch),
    _zero_(this+1),
    _processor_channel(ch),
    _input_message( ch->_session ),
    _input_message_builder( &_input_message ),
    _output_message( ch->_session )
{
}

//--------------------------------------------------------Object_server_processor::put_request_part

void Object_server_processor::put_request_part( const char* data, int length )
{ 
    _input_message_builder.add_data( (const Byte*)data, length );
}

//-----------------------------------------------------Object_server_processor::request_is_complete

bool Object_server_processor::request_is_complete()
{ 
    return _input_message.is_complete();
}

//-----------------------------------------------------------------Object_server_processor::process

void Object_server_processor::process()
{
    _processor_channel->_session->execute( &_input_message, &_output_message );
}

//----------------------------------------------------Object_server_processor::response_is_complete

bool Object_server_processor::response_is_complete()
{
    return true;
}

//-------------------------------------------------------Object_server_processor::get_response_part

string Object_server_processor::get_response_part()
{
    string result = _output_message._data;
    _output_message._data = "";
    return result;
}

//-------------------------------------------------Object_server_processor::should_close_connection

bool Object_server_processor::should_close_connection()
{
    return false;
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
