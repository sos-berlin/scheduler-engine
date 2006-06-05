// $Id$

#include "spooler.h"

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const
    
const int main_scheduler_retry_time = 60;

//------------------------------------------------------ml_client_connection::Xml_client_connection
    
Xml_client_connection::Xml_client_connection( Spooler* sp, const Host_and_port& host_and_port )
: 
    _zero_(this+1), 
    _spooler(sp),
    _log( sp->_log ),
    _host_and_port(host_and_port)
{
    _log.set_prefix( S() << "Haupt-Scheduler " + _host_and_port.to_string() );
    set_async_next_gmtime( (time_t)0 );
    //set_async_child( &_socket_operation );
}

//----------------------------------------------------Xml_client_connection::~Xml_client_connection

Xml_client_connection::~Xml_client_connection()
{
}

//--------------------------------------------------------Xml_client_connection::set_socket_manager

void Xml_client_connection::set_socket_manager( Socket_manager* manager )
{
    _socket_manager = manager;

    set_async_manager( manager );
    set_async_next_gmtime( (time_t)0 );     // Sofort starten!
}

//---------------------------------------------------------Xml_client_connection::async_state_text_

string Xml_client_connection::state_name( State state )
{
    switch( state )
    {
        case s_initial:     return "initial";
        case s_connecting:  return "connecting";
        case s_finished:    return "finished";
        case s_sending:     return "sending";
        case s_waiting:     return "waiting_for_response";
        case s_receiving:   return "receiving";
        default:            return "state=" + as_int( state );
    }
}

//------------------------------------------------------------------Xml_client_connection::obj_name

string Xml_client_connection::obj_name() const
{
    return S() << "Xml_client_connection(" << _host_and_port << " " << state_name( _state ) << ")";
}

//-----------------------------------------------------------Xml_client_connection::async_continue_
    
bool Xml_client_connection::async_continue_( Continue_flags flags )
{
    bool something_done = false;

    try
    {
        if( _socket_operation )
        {
            if( !_socket_operation->async_finished() )  return false;
            _socket_operation->async_check_error();

            if( _socket_operation->_eof )  z::throw_xc( "SCHEDULER-224" );
        }


        switch( _state )
        {
            case s_initial:
                if( !( flags & cont_next_gmtime_reached ) )  return false;

                _socket_operation = Z_NEW( Buffered_socket_operation( _socket_manager ) );

                _socket_operation->set_async_parent( this );
                //_socket_operation->add_to_socket_manager( _socket_manager );

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
                if( _socket_operation->_eof )  z::throw_xc( "SCHEDULER-224" );

                string data = _socket_operation->recv_data();
                if( data.length() == 0 )  break;

                _state = s_receiving;
                something_done = true;

                _socket_operation->recv_clear();
                _recv_data.append( data );
                

                if( !_xml_end_finder.is_complete( data.data(), data.length() ) )  break;

                _state = s_finished;

                //_log.info( "ANTWORT: " + _recv_data );

                xml::Document_ptr response_document;
                response_document.create();

                response_document.load_xml( _recv_data );
                _recv_data = "";

                DOM_FOR_EACH_ELEMENT( response_document.documentElement(), e1 )
                    if( e1.nodeName_is( "answer" ) )
                        DOM_FOR_EACH_ELEMENT( e1, e2 )
                            if( e2.nodeName_is( "ERROR" ) )  z::throw_xc( "SCHEDULER-223", e2.getAttribute( "text" ) );

                _log.info( message_string( "SCHEDULER-950" ) );   // "Scheduler ist registriert"
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
    if( !register_scheduler_element )  return;

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

xml::Element_ptr Remote_scheduler::dom_element( const xml::Document_ptr& document, const Show_what& )
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

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
