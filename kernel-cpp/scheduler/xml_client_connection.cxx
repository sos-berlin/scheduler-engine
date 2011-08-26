// $Id: xml_client_connection.cxx 14221 2011-04-29 14:18:28Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"

namespace sos {
namespace scheduler {

//-----------------------------------------------------Xml_client_connection::Xml_client_connection
    
Xml_client_connection::Xml_client_connection( Spooler* sp, const Host_and_port& host_and_port )
: 
    Scheduler_object( sp, this, type_xml_client_connection ),
    _zero_(this+1), 
    _host_and_port(host_and_port)
{
    _log->set_prefix( S() << "Xml_client_connection " << _host_and_port.as_string() );

    //set_async_manager( _spooler->_connection_manager );
}

//----------------------------------------------------Xml_client_connection::~Xml_client_connection

Xml_client_connection::~Xml_client_connection()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }

    if( _socket_operation )
    {
        _socket_operation->set_async_parent( NULL );
        _socket_operation->set_async_manager( NULL );
    }
}

//---------------------------------------------------------------------Xml_client_connection::close

void Xml_client_connection::close()
{
    if( _socket_operation )
    {
        _socket_operation->set_async_parent( NULL );
        _socket_operation->set_async_manager( NULL );
        _socket_operation->close();
        _socket_operation = NULL;
    }

    _state = s_closed;
}

//-------------------------------------------------------------------Xml_client_connection::connect

void Xml_client_connection::connect()
{
    async_wake(); 
}

//----------------------------------------------------------Xml_client_connection::is_send_possible

bool Xml_client_connection::is_send_possible()
{
    return _state == s_connected;
}

//----------------------------------------------------------------------Xml_client_connection::send

void Xml_client_connection::send( const string& s )
{
    Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << " " << s << "\n");
    if( !is_send_possible() )  assert(0), z::throw_xc( Z_FUNCTION, "Connection is currently in use" );

    _received_data.clear();
    _send_data = s;

    if( _state == s_connected )
    {
        async_wake();
    }
}

//-----------------------------------------------Xml_client_connection::fetch_received_dom_document

xml::Document_ptr Xml_client_connection::fetch_received_dom_document()
{
    Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << "\n");
    xml::Document_ptr result;

    if( _state == s_connected  &&  !_received_data.is_empty() )
    {
        result.create();

        result.load_xml( _received_data );
        _received_data.clear();

        DOM_FOR_EACH_ELEMENT( result.documentElement(), e1 )
            if( e1.nodeName_is( "answer" ) )
                DOM_FOR_EACH_ELEMENT( e1, e2 )
                    if( e2.nodeName_is( "ERROR" ) )  z::throw_xc( "SCHEDULER-223", _host_and_port.as_string(), e2.getAttribute( "text" ) );
    }

    if (result)  Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " result=" << result.xml() << "\n");
    return result;
}

//-----------------------------------------------------------Xml_client_connection::async_continue_
    
bool Xml_client_connection::async_continue_( Continue_flags flags )
{
    Z_DEBUG_ONLY( Z_LOGI2( "zschimmer", Z_FUNCTION << "\n" ); )

    bool something_done = false;

    try
    {
        if( _socket_operation )
        {
            _socket_operation->async_check_exception();

            //if( !_socket_operation->async_finished() ) 
            //{
            //    Z_DEBUG_ONLY( Z_LOG2( "scheduler", Z_FUNCTION << " !_socket_operation->async_finished()\n" ); )
            //    return false;
            //}

            if( _socket_operation->_eof )  z::throw_xc( "SCHEDULER-224", _host_and_port.as_string() );
        }


        for( int again = 0; again >= 0; --again )  
        switch( _state )
        {
            case s_not_connected:
            {
                if( !( flags & cont_next_gmtime_reached ) )  
                { 
                    Z_DEBUG_ONLY( Z_LOG( "*** " << Z_FUNCTION << " !cont_next_gmtime_reached\n" ); 
                    Z_WINDOWS_ONLY( DebugBreak(); ) ) 
                    break;
                }

                _socket_operation = Z_NEW( Buffered_socket_operation );

                _socket_operation->add_to_socket_manager( _spooler->_connection_manager );
                _socket_operation->set_async_parent( this );

                _socket_operation->connect__start( _host_and_port );

                _state = _socket_operation->state() == Buffered_socket_operation::s_connecting? s_connecting : s_connected;
                something_done = true;
                break;
            }


            case s_connecting:
            {
                if( !_socket_operation->async_finished() )  break;

                _socket_operation->try_set_keepalive( true );

                _state = s_connected;
                something_done = true;
                again = 1;
                break;
            }


            case s_connected:
            {
                if( !_send_data.empty() )
                {
                    _socket_operation->send__start( _send_data );
                    _state = s_sending;
                    again = 1;
                    something_done = true;
                }

                break;
            }


            case s_sending:
                if( !_socket_operation->async_finished() )  break;

                _received_data.clear();
                _xml_end_finder = Xml_end_finder();

                something_done = true;
                _state = s_waiting;

            case s_waiting:
            case s_receiving:
            {
                while(1)
                {
                    _socket_operation->recv__continue();
                    if( _socket_operation->_eof )  z::throw_xc( "SCHEDULER-224", _host_and_port.as_string() );

                    string data = _socket_operation->recv_data();
                    if( data.length() == 0 )  break;

                    _state = s_receiving;
                    something_done = true;

                    _socket_operation->recv_clear();
                    Z_LOG2( "scheduler", Z_FUNCTION << ": " << data << "\n" );
                    _received_data.append( data );

                    if( _xml_end_finder.is_complete( data.data(), data.length() ) )  break;
                }

                if( _xml_end_finder.is_complete() ) 
                {
                    _send_data = "";
                    _state = s_connected;

                    //log()->info( "ANTWORT: " + _recv_data );
                    //log()->info( message_string( "SCHEDULER-950" ) );   // "Scheduler ist registriert"
                }
                else
                {
                    Z_LOG2( "scheduler", Z_FUNCTION << ", waiting for XML-response\n" );
                }

                break;
            }


            default: ;
        }
    }
    catch( exception& x )
    {
        if( _state == s_connecting  &&  _wait_for_connection )
        {
            log()->warn( x.what() );

            if( _socket_operation )
            {
                _socket_operation->close();
                _socket_operation = NULL;
            }

            _state = s_not_connected;
            set_async_delay( _wait_for_connection );
            something_done = true;
        }
        else
            throw;
    }

    return something_done;
}

//---------------------------------------------------------Xml_client_connection::async_state_text_

string Xml_client_connection::state_name( State state )
{
    switch( state )
    {
        case s_not_connected:   return "not_connected";
        case s_connecting:      return "connecting";
        case s_connected:       return "connected";
        case s_sending:         return "sending";
        case s_waiting:         return "waiting";
        case s_receiving:       return "receiving";
        case s_closed:          return "closed";
        default:                return S() << "state=" << state;
    }
}

//------------------------------------------------------------------Xml_client_connection::obj_name

string Xml_client_connection::obj_name() const
{
    return S() << "Xml_client_connection(" << _host_and_port << ")";
}

//---------------------------------------------------------Xml_client_connection::async_state_text_

string Xml_client_connection::async_state_text_() const
{
    S result;

    result << "Xml_client_connection(";
    result << _host_and_port;
    result << " ";
    result << state_name( _state );

    if( _send_data != ""           )  result << ", sending "   << quoted_string( truncate_to_one_line_with_ellipsis( _send_data                , 100 ) );
    if( !_received_data.is_empty() )  result << ", receiving " << quoted_string( truncate_to_one_line_with_ellipsis( _received_data.to_string(), 100 ) );

    result << ")";

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
