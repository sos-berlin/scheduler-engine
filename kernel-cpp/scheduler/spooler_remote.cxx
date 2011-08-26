// $Id: spooler_remote.cxx 13691 2008-09-30 20:42:20Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
const int main_scheduler_retry_time = 60;

//-----------------------------------------------------Remote_client_connection::Remote_client_connection
    
Remote_client_connection::Remote_client_connection( Spooler* sp, const Host_and_port& host_and_port )
: 
    Scheduler_object( sp, this, type_remote_client ),
    _zero_(this+1), 
    _host_and_port(host_and_port)
{
    _log->set_prefix( S() << obj_name() << ' ' << _host_and_port.as_string() );
}

//----------------------------------------------Remote_client_connection::~Remote_client_connection

Remote_client_connection::~Remote_client_connection()
{
    if( _xml_client_connection )  
    {
        _xml_client_connection->set_async_parent( NULL );
        _xml_client_connection->set_async_manager( NULL );
    }
}

//----------------------------------------------------------------Remote_client_connection::connect

void Remote_client_connection::connect()
{
    _xml_client_connection = Z_NEW( Xml_client_connection( _spooler, _host_and_port ) );
    _xml_client_connection->set_async_manager( _spooler->_connection_manager );
    _xml_client_connection->set_async_parent( this );
    _xml_client_connection->connect();
    _state = s_connecting;
}

//--------------------------------------------------------Remote_client_connection::async_continue_
    
bool Remote_client_connection::async_continue_( Continue_flags flags )
{
    Z_DEBUG_ONLY( Z_LOGI2( "zschimmer", Z_FUNCTION << "\n" ); )

    bool something_done = false;

    try
    {
        switch( _state )
        {
            case s_not_connected:
                connect();
                break;

            case s_connecting:
            {
                if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

                S xml;
                xml << "<register_remote_scheduler";
                xml << " scheduler_id='" << _spooler->_spooler_id  << "'";
                xml << " tcp_port='"     << _spooler->_tcp_port    << "'";
                xml << " version='"      << _spooler->_version     << "'";
                xml << "/>";

                _xml_client_connection->send( xml );
                _state = s_registering;
            }

            case s_registering:
            {
                if( xml::Document_ptr response_document = _xml_client_connection->fetch_received_dom_document() )
                {
                    log()->info( message_string( "SCHEDULER-950" ) );
                    _state = s_registered;
                }

                break;
            }

            default: ;
        }
    }
    catch( exception& x )
    {
        log()->warn( x.what() );

        if( _xml_client_connection ) 
        {
            _xml_client_connection->set_async_manager( NULL );
            _xml_client_connection = NULL;
        }

        _state = s_not_connected;
        set_async_delay( main_scheduler_retry_time );
        something_done = true;
    }

    return something_done;
}

//---------------------------------------------------------Remote_client_connection::async_state_text_

string Remote_client_connection::state_name( State state )
{
    switch( state )
    {
        case s_not_connected:   return "not_connected";
        case s_connecting:      return "connecting";
        case s_registering:     return "registering";
        case s_registered:      return "registered";
        default:                return "state=" + as_int( state );
    }
}

//------------------------------------------------------------------Remote_client_connection::obj_name

string Remote_client_connection::obj_name() const
{
    return S() << "Remote_client_connection(" << _host_and_port << " " << state_name( _state ) << ")";
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

        if( show.is_set( show_remote_schedulers ) )  result.appendChild( remote_scheduler->dom_element( document, show ) );
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

} //namespace scheduler
} //namespace sos
