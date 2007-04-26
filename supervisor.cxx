// $Id: spooler_remote.cxx 4705 2007-01-05 16:20:23Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
const int main_scheduler_retry_time = 60;

//-------------------------------------------------------------------------------------------------

struct Supervisor_client_connection;

//---------------------------------------------------------------------------------Remote_scheduler

struct Remote_scheduler : Remote_scheduler_interface
{
                                Remote_scheduler            ()                                      : _zero_(this+1){}

    void                        connection_lost_event       ( const exception* );
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& show );


    Fill_zero                  _zero_;
    Host_and_port              _host_and_port;
    string                     _scheduler_id;
    string                     _version;
    Time                       _connected_at;
    Time                       _disconnected_at;
    bool                       _logged_on;
    bool                       _is_connected;
    Xc_copy                    _error;

  //ptr<object_server::Proxy>  _scheduler_proxy;
};

//------------------------------------------------------------------------Remote_scheduler_register

struct Remote_scheduler_register
{
                                Remote_scheduler_register   ()                                      : _zero_(this+1){}


    void                        add                         ( Remote_scheduler* );
    Remote_scheduler*           get_or_null                 ( const Host_and_port& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    Fill_zero                  _zero_;
    typedef map< Host_and_port, ptr<Remote_scheduler> >   Map;
    Map                                                  _map;
};

//---------------------------------------------------------------------------------------Supervisor

struct Supervisor : Supervisor_interface
{
                                Supervisor                  ( Scheduler* scheduler )                : Supervisor_interface( scheduler, type_supervisor ), _zero_(this+1) {}

    // Subsystem
    void                        close                       ();
    //bool                        subsystem_initialize        ();
    //bool                        subsystem_load              ();
    //bool                        subsystem_activate          ();

    // Supervisor_interface
    void                        execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

  private:
    Fill_zero                  _zero_;
    Remote_scheduler_register  _remote_scheduler_register;
};

//--------------------------------------------------------------------------------Supervisor_client

struct Supervisor_client : Supervisor_client_interface
{
                                Supervisor_client           ( Scheduler*, const Host_and_port& );

    // Subsystem
    void                        close                       ();
    bool                        subsystem_initialize        ();
                                Subsystem::obj_name;

  private:
    Fill_zero                  _zero_;
    ptr<Supervisor_client_connection> _client_connection;
};

//---------------------------------------------------------------------Supervisor_client_connection

struct Supervisor_client_connection : Async_operation, Scheduler_object
{
    enum State
    {
        s_not_connected,
        s_connecting,
        s_registering,
        s_registered
    };

    static string               state_name                  ( State );


                                Supervisor_client_connection( Supervisor_client*, const Host_and_port& );
                               ~Supervisor_client_connection();

    virtual string              obj_name                    () const;

    State                       state                       () const                                { return _state; }

    void                        connect                     ();


  protected:
    string                      async_state_text_           () const                                { return obj_name(); }
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return _state == s_not_connected  
                                                                                                          || _state == s_registered; }
  //bool                        async_signaled_             ()                                      { return _socket_operation && _socket_operation->async_signaled(); }

  private:
    Fill_zero                  _zero_;
    State                      _state;
    Host_and_port              _host_and_port;
    Supervisor_client*         _supervisor_client;
    ptr<Xml_client_connection> _xml_client_connection;
};

//------------------------------------------------------------------------Main_scheduler_connection
// Verbindung zum Main Scheduler
/*
struct Main_scheduler_connection : Async_operation
{
    enum State
    {
        s_initial,
        s_connecting,
        s_stand_by,
                                Main_scheduler_connection( Spooler*, const Host_and_port& );


  protected:
    string                      async_state_text_           ();
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             ()                                      { return _state == s_initial  

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Supervisor_client_connection      _xml_client_connection;
};
*/
//-----------------------------------------------------------------------------Xml_client_operation
/*
struct Xml_client_operation : Operation
{
    Xml_client_operation( Supervisor_client_connection* conn ) : _connection( conn ) {}

    ptr<Supervisor_client_connection> _connection;
};
*/
//-------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------Object_server_processor
/*
struct Object_server_processor : Communication::Operation
{
                                Object_server_processor     ( Object_server_processor_channel* );


    void                        put_request_part            ( const char* data, int length );
    bool                        request_is_complete         ();

    void                        process                     ();

    bool                        response_is_complete        ();
    string                      get_response_part           ();
    bool                        should_close_connection     ();


    Fill_zero                          _zero_;
    Object_server_processor_channel*      _operation_channel;
    object_server::Input_message          _input_message;
    object_server::Input_message::Builder _input_message_builder;
    object_server::Output_message         _output_message;
};

//--------------------------------------------------------------------Object_server_processor_channel

struct Object_server_processor_channel : Communication::Operation_channel
{
                                Object_server_processor_channel( Communication::Channel* );

    ptr<Communication::Operation> processor                 ()                                      { ptr<Object_server_processor> result = Z_NEW( Object_server_processor( this ) ); 
                                                                                                      return +result; }

    ptr<object_server::Session> _session;
};
*/
//-----------------------------------------------------Supervisor_client_connection::Supervisor_client_connection
    
Supervisor_client_connection::Supervisor_client_connection( Supervisor_client* supervisor_client, const Host_and_port& host_and_port )
: 
    Scheduler_object( supervisor_client->_spooler, this, type_supervisor_client_connection ),
    _zero_(this+1), 
    _host_and_port(host_and_port)
{
    _supervisor_client = supervisor_client;
    _log = supervisor_client->log();
}

//----------------------------------------------Supervisor_client_connection::~Supervisor_client_connection

Supervisor_client_connection::~Supervisor_client_connection()
{
    if( _xml_client_connection )  
    {
        _xml_client_connection->set_async_parent( NULL );
        _xml_client_connection->set_async_manager( NULL );
    }
}

//----------------------------------------------------------------Supervisor_client_connection::connect

void Supervisor_client_connection::connect()
{
    _xml_client_connection = Z_NEW( Xml_client_connection( _spooler, _host_and_port ) );
    _xml_client_connection->set_async_manager( _spooler->_connection_manager );
    _xml_client_connection->set_async_parent( this );
    _xml_client_connection->connect();
    _state = s_connecting;
}

//--------------------------------------------------------Supervisor_client_connection::async_continue_
    
bool Supervisor_client_connection::async_continue_( Continue_flags )
{
    Z_DEBUG_ONLY( Z_LOGI2( "joacim", __FUNCTION__ << "\n" ); )

    bool something_done = false;

    try
    {
        if( _xml_client_connection )  _xml_client_connection->async_check_exception();

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
                if( xml::Document_ptr response_document = _xml_client_connection->received_dom_document() )
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

//---------------------------------------------------------Supervisor_client_connection::async_state_text_

string Supervisor_client_connection::state_name( State state )
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

//------------------------------------------------------------------Supervisor_client_connection::obj_name

string Supervisor_client_connection::obj_name() const
{
    return S() << "Supervisor_client_connection(" << _host_and_port << " " << state_name( _state ) << ")";
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

//-----------------------------------------------------------------------------------new_supervisor

ptr<Supervisor_interface> new_supervisor( Scheduler* scheduler )
{
    ptr<Supervisor> supervisor = Z_NEW( Supervisor( scheduler ) );
    return +supervisor;
}

//--------------------------------------------------------------------------------Supervisor::close

void Supervisor::close()
{
}

//----------------------------------------------------Supervisor::execute_register_remote_scheduler

void Supervisor::execute_register_remote_scheduler( const xml::Element_ptr& register_remote_scheduler_element, Communication::Operation* communication_operation )
{
    Xml_operation* xml_processor = dynamic_cast<Xml_operation*>( communication_operation );
    if( !xml_processor )  z::throw_xc( "SCHEDULER-222", register_remote_scheduler_element.nodeName() );
 
    Host_and_port host_and_port ( communication_operation->_connection->peer_host(), register_remote_scheduler_element.int_getAttribute( "tcp_port" ) );

    ptr<Remote_scheduler> remote_scheduler = _remote_scheduler_register.get_or_null( host_and_port );

    if( !remote_scheduler )  remote_scheduler = Z_NEW( Remote_scheduler );

    remote_scheduler->_host_and_port = host_and_port;
    remote_scheduler->_host_and_port._host.resolve_name();
    remote_scheduler->set_dom( register_remote_scheduler_element );
    
    xml_processor->_operation_connection->_remote_scheduler = +remote_scheduler;        // Remote_scheduler mit TCP-Verbindung verknüpfen
    _remote_scheduler_register.add( remote_scheduler );
}

//--------------------------------------------------------------------------Supervisor::dom_element

xml::Element_ptr Supervisor::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    return _remote_scheduler_register.dom_element( dom_document, show_what );
}

//----------------------------------------------------------------------------new_supervisor_client

ptr<Supervisor_client_interface> new_supervisor_client( Scheduler* scheduler, const Host_and_port& host_and_port )
{
    ptr<Supervisor_client> supervisor_client = Z_NEW( Supervisor_client( scheduler, host_and_port ) );
    return +supervisor_client;
}

//--------------------------------------------------------------upervisor_client::Supervisor_client

Supervisor_client::Supervisor_client( Scheduler* scheduler, const Host_and_port& host_and_port )
: 
    Supervisor_client_interface( scheduler, type_supervisor_client ),
    _zero_(this+1)
{
    _log->set_prefix( S() << obj_name() << ' ' << host_and_port.as_string() );
    _client_connection = Z_NEW( Supervisor_client_connection( this, host_and_port ) );
}

//-------------------------------------------------------------------------Supervisor_client::close
    
void Supervisor_client::close()
{
    if( _client_connection )
    {
        _client_connection->set_async_manager( NULL );
        _client_connection = NULL;
    }
}

//----------------------------------------------------------Supervisor_client::subsystem_initialize
    
bool Supervisor_client::subsystem_initialize()
{
    _subsystem_state = subsys_initialized;
    _client_connection->set_async_manager( _spooler->_connection_manager );
    _client_connection->connect();

    return true;
}

//----------------------------------------------------------------Supervisor_client::subsystem_load

//bool Supervisor_client::subsystem_load()
//{
//    _subsystem_state = subsys_loaded;
//    return true;
//}

//------------------------------------------------------------Supervisor_client::subsystem_activate

//bool Supervisor_client::subsystem_activate()
//{
//    _subsystem_state = subsys_activated;
//    return true;
//}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
