// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../zschimmer/base64.h"

#include <sys/stat.h>           // mkdir()

#ifdef Z_WINDOWS
#   include <direct.h>          // mkdir()
#   include <sys/utime.h>       // utime()
#else
#   include <utime.h>           // utime()
#endif

namespace sos {
namespace scheduler {
namespace supervisor {

//-------------------------------------------------------------------------------------------------

using namespace directory_observer;

using xml::Xml_writer;

struct Supervisor;
struct Supervisor_client;

//--------------------------------------------------------------------------------------------const

//const int                       supervisor_configuration_poll_interval = 60;
const double                    udp_timeout                 = 60;

//------------------------------------------------------------------------------------Xml_file_info

struct Xml_file_info : Base_file_info
{
    Xml_file_info( const xml::Element_ptr& element )
    :
        Base_file_info( element.getAttribute( "name" ), Time().set_datetime( element.getAttribute( "last_write_time" ) ).as_utc_double(), element.getAttribute( "name" ), 0 ),
        _element( element )
    {
        _md5.set_hex( element.getAttribute( "md5" ) );
    }

    xml::Element_ptr           _element;
    Md5                        _md5;                        // Leer bei einem Verzeichnis
};

//---------------------------------------------------------------------------------Remote_scheduler

struct Remote_scheduler : Remote_scheduler_interface, 
                          Scheduler_object
{
                                Remote_scheduler            ( Supervisor* supervisor );

    void                        connection_lost_event       ( const exception* );
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& show );

    ptr<Command_response>       execute_xml                 ( const xml::Element_ptr&, Command_processor* );
    ptr<Command_response>       execute_configuration_fetch_updated_files( const xml::Element_ptr&, Command_processor* );
    void                        write_updated_files_to_xml  ( Xml_writer*, Directory*, const xml::Element_ptr& reference_element );
    void                        write_file_to_xml           ( Xml_writer*, Directory*, const Directory_entry&, const Xml_file_info* reference );
    bool                        check_remote_configuration  ();
    void                        signal_remote_scheduler     ();
    string                      obj_name                    () const;

    string                      async_state_text_           () const                                { return obj_name(); }
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return false; }

    Fill_zero                  _zero_;
    Supervisor*                _supervisor;
    Host_and_port              _host_and_port;
    int                        _udp_port;
    string                     _scheduler_id;
    string                     _scheduler_version;
    Time                       _connected_at;
    Time                       _disconnected_at;
    bool                       _logged_on;
    bool                       _is_connected;
    Xc_copy                    _error;
    int                        _configuration_version;
};

//------------------------------------------------------------------------Remote_scheduler_register

struct Remote_scheduler_register
{
                                Remote_scheduler_register   ()                                      : _zero_(this+1){}


    void                        add                         ( Remote_scheduler* );
    Remote_scheduler*           get                         ( const Host_and_port& );
    Remote_scheduler*           get_or_null                 ( const Host_and_port& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    Fill_zero                  _zero_;
    typedef map< Host_and_port, ptr<Remote_scheduler> >   Map;
    Map                                                  _map;
};

//----------------------------------------------------------------------------Remote_configurations

struct Remote_configurations : Scheduler_object,
                               Event_operation
{
                                Remote_configurations       ( Supervisor*, const File_path& directory );
                               ~Remote_configurations       ();

    void                        close                       ();



    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    Socket_event*               async_event                 ()                                      { return &_directory_event; }
  //bool                        async_signaled_             ()                                      { return is_signaled(); }
    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }


  //File_path                   directory_path              () const                                { return _directory_path; }
    Directory_tree*             directory_tree              () const                                { return _directory_tree; }

  //bool                     is_signaled                    ()                                      { return _directory_event.signaled(); }
  //void                    set_signaled                    ( const string& text )                  { _directory_event.set_signaled( text ); }

    void                        activate                    ();
    bool                     is_activated                   () const                                { return _is_activated; }

    bool                        check                       ( double minimum_age = 0 );


  private:
    Fill_zero                  _zero_;
    Supervisor*                _supervisor;
    Event                      _directory_event;
    ptr<Directory_tree>        _directory_tree;
    int                        _directory_watch_interval;
    bool                       _is_activated;
};

//---------------------------------------------------------------------------------------Supervisor

struct Supervisor : Supervisor_interface
{
                                Supervisor                  ( Scheduler* );

    // Subsystem
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    // Supervisor_interface
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                        execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* );
    ptr<Command_response>       execute_xml                 ( const xml::Element_ptr&, Command_processor* );


    Remote_configurations*      remote_configurations       () const                                { return _remote_configurations; }
    void                        read_configuration_directory_names();
    Directory*                  configuration_directory_for_remote_scheduler( Remote_scheduler* );
    bool                        check_remote_configurations ();
    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }

  private:
    Fill_zero                  _zero_;
    Remote_scheduler_register  _remote_scheduler_register;
    ptr<Remote_configurations> _remote_configurations;
    
    typedef stdext::hash_map< Host_and_port, string >  Configuration_directory_map;
    Configuration_directory_map _configuration_directory_map;
};

//---------------------------------------------------------------------Supervisor_client_connection

struct Supervisor_client_connection : Async_operation, Scheduler_object
{
    enum State
    {
        s_not_connected,
        s_connecting,
        s_registering,
        s_registered,
        s_fetching_configuration,
        s_configuration_fetched
    };

    static string               state_name                  ( State );


                                Supervisor_client_connection( Supervisor_client*, const Host_and_port& );
                               ~Supervisor_client_connection();

    virtual string              obj_name                    () const;

    State                       state                       () const                                { return _state; }
    void                        connect                     ();
    const Host_and_port&        host_and_port               () const                                { return _host_and_port; }
    bool                        is_ready                    () const                                { return _is_ready; }
    bool                        connection_failed           () const                                { return _connection_failed; }
    void                        start_update_configuration  ();

  protected:
    string                      async_state_text_           () const                                { return obj_name(); }
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return _state == s_not_connected  
                                                                                                          || _state == s_registered; }
  //bool                        async_signaled_             ()                                      { return _socket_operation && _socket_operation->async_signaled(); }

    void                        write_directory_structure   ( xml::Xml_writer*, const Absolute_path& );
    void                        update_directory_structure  ( const Absolute_path&, const xml::Element_ptr& );

  private:
    Fill_zero                  _zero_;
    State                      _state;
    Host_and_port              _host_and_port;
    Supervisor_client*         _supervisor_client;
    ptr<Xml_client_connection> _xml_client_connection;
    bool                       _is_ready;
    bool                       _connection_failed;
};

//--------------------------------------------------------------------------------Supervisor_client

struct Supervisor_client : Supervisor_client_interface
{
                                Supervisor_client           ( Scheduler*, const Host_and_port& );

    // Subsystem
    void                        close                       ();
    bool                        subsystem_initialize        ();
                                Subsystem::obj_name;

    // Supervisor_client_interface
    bool                        is_ready                    () const                                { return _client_connection && _client_connection->is_ready(); }
    bool                        connection_failed           () const                                { return _client_connection && _client_connection->connection_failed(); }
    void                        start_update_configuration  ()                                      { if( _client_connection )  _client_connection->start_update_configuration(); }

    // IDispatch_implementation
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Supervisor_client"; }
    STDMETHODIMP            get_Hostname                    ( BSTR* );
    STDMETHODIMP            get_Tcp_port                    ( int* );

  private:
    Fill_zero                  _zero_;
    ptr<Supervisor_client_connection> _client_connection;

    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
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

//--------------------------------------------------------------------------------------------const
    
const int                               main_scheduler_retry_time           = 60;
Supervisor_client::Class_descriptor     Supervisor_client::class_descriptor ( &typelib, "Spooler.Supervisor", Supervisor_client::_methods );

//----------------------------------------------------------------------Supervisor_client::_methods

const Com_method Supervisor_client::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Supervisor_client,  1, Java_class_name     , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Supervisor_client,  2, Hostname            , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Supervisor_client,  3, Tcp_port            , VT_INT     , 0 ),
#endif
    {}
};

//-----------------------------------------Supervisor_client_interface::Supervisor_client_interface

Supervisor_client_interface::Supervisor_client_interface( Scheduler* scheduler, Type_code tc, Class_descriptor* class_descriptor )  
: 
    Idispatch_implementation( class_descriptor ),
    Subsystem( scheduler, static_cast<IDispatch*>( this ), tc ) 
{
}

//---------------------------------------Supervisor_client_connection::Supervisor_client_connection
    
Supervisor_client_connection::Supervisor_client_connection( Supervisor_client* supervisor_client, const Host_and_port& host_and_port )
: 
    Scheduler_object( supervisor_client->_spooler, this, type_supervisor_client_connection ),
    _zero_(this+1), 
    _host_and_port(host_and_port)
{
    _supervisor_client = supervisor_client;
    _log = supervisor_client->log();
}

//--------------------------------------Supervisor_client_connection::~Supervisor_client_connection

Supervisor_client_connection::~Supervisor_client_connection()
{
    if( _xml_client_connection )  
    {
        _xml_client_connection->set_async_parent( NULL );
        _xml_client_connection->set_async_manager( NULL );
    }
}

//------------------------------------------------------------Supervisor_client_connection::connect

void Supervisor_client_connection::connect()
{
    _xml_client_connection = Z_NEW( Xml_client_connection( _spooler, _host_and_port ) );
    _xml_client_connection->set_async_manager( _spooler->_connection_manager );
    _xml_client_connection->set_async_parent( this );
    _xml_client_connection->connect();
    _state = s_connecting;
}

//-----------------------------------------Supervisor_client_connection::start_update_configuration

void Supervisor_client_connection::start_update_configuration()
{
    if( _state == s_configuration_fetched )
    {
        _state = s_registered;
        async_continue();
    }
}

//----------------------------------------------------Supervisor_client_connection::async_continue_

bool Supervisor_client_connection::async_continue_( Continue_flags )
{
    Z_DEBUG_ONLY( Z_LOGI2( "joacim", Z_FUNCTION << "\n" ); )

    bool something_done = false;

    try
    {
        if( _xml_client_connection )  _xml_client_connection->async_check_exception();

        switch( _state )
        {
            case s_not_connected:
                _connection_failed = false;
                connect();
                break;


            case s_connecting:
            {
                if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

                ptr<io::String_writer> string_writer = Z_NEW( io::String_writer() );
                ptr<xml::Xml_writer>   xml_writer    = Z_NEW( xml::Xml_writer( string_writer ) );

                xml_writer->set_encoding( scheduler_character_encoding );
                xml_writer->write_prolog();

                xml_writer->begin_element( "register_remote_scheduler" );
                xml_writer->set_attribute( "scheduler_id", _spooler->_spooler_id );

                if( _spooler->_tcp_port )
                xml_writer->set_attribute( "tcp_port"    , _spooler->_tcp_port   );

                if( _spooler->_udp_port )
                xml_writer->set_attribute( "udp_port"    , _spooler->_udp_port );
                
                //if( _spooler->_udp_port )
                //xml_writer->set_attribute( "udp_port"    , _spooler->_udp_port   );

                xml_writer->set_attribute( "version"     , _spooler->_version );
                xml_writer->end_element( "register_remote_scheduler" );
                
                xml_writer->close();

                _xml_client_connection->send( string_writer->to_string() );
                _state = s_registering;
            }


            case s_registering:
            {
                if( xml::Document_ptr response_document = _xml_client_connection->fetch_received_dom_document() )
                {
                    log()->info( message_string( "SCHEDULER-950" ) );
                    _state = s_registered;
                }

                if( _state != s_registered )  break;
            }


            case s_registered:
#ifdef Z_DEBUG
            {
                // Wird nach Verbindungsverlust nochmal durchlaufen

                if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

                ptr<io::String_writer> string_writer = Z_NEW( io::String_writer() );
                ptr<xml::Xml_writer>   xml_writer    = Z_NEW( xml::Xml_writer( string_writer ) );

                xml_writer->set_encoding( scheduler_character_encoding );
                xml_writer->write_prolog();

                xml_writer->begin_element( "supervisor.configuration.fetch_updated_files" );
                xml_writer->set_attribute( "scheduler_id", _spooler->id() );

                if( _spooler->_tcp_port )
                xml_writer->set_attribute( "tcp_port"    , _spooler->_tcp_port );

                if( _spooler->_udp_port )  xml_writer->set_attribute( "signal_next_change_at_udp_port", _spooler->_udp_port );
                                     else  log()->warn( message_string( "SCHEDULER-899" ) );//, supervisor_configuration_poll_interval ) );

                write_directory_structure( xml_writer, root_path );

                xml_writer->end_element( "supervisor.configuration.fetch_updated_files" );
                xml_writer->close();

                _xml_client_connection->send( string_writer->to_string() );
                _state = s_fetching_configuration;
            }


            case s_fetching_configuration:
            {
                if( xml::Document_ptr response_document = _xml_client_connection->fetch_received_dom_document() )
                {
                    //log()->info( message_string( "SCHEDULER-950" ) );

                    update_directory_structure( root_path, response_document.select_node_strict( "/spooler/answer/configuration.directory" ) );
                    
                    _state = s_configuration_fetched;
                }

                if( _state != s_configuration_fetched )  break;
            }


            case s_configuration_fetched:
#endif
                _is_ready = true;   // Nach Verbindungsverlust bereits true
                break;

            default: 
                assert(0);
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
        _connection_failed = true;

        set_async_delay( main_scheduler_retry_time );
        something_done = true;
    }

    return something_done;
}

//------------------------------------------Supervisor_client_connection::write_directory_structure

void Supervisor_client_connection::write_directory_structure( xml::Xml_writer* xml_writer, const Absolute_path& path )
{
    Folder_directory_lister dir ( _log );
    
    bool ok = dir.open( spooler()->folder_subsystem()->directory(), path );

    if( ok )
    {
        while( ptr<file::File_info> file_info = dir.get() )
        {
            string filename = file_info->path().name();

            if( file_info->is_directory() )
            {
                xml_writer->begin_element( "configuration.directory" );
                xml_writer->set_attribute( "name", path.name() );

                write_directory_structure( xml_writer, Absolute_path( path, filename ) );

                xml_writer->end_element( "configuration.directory" );
            }
            else
            {
                string name      = Folder::object_name_of_filename( filename );
                string extension = Folder::extension_of_filename( filename );
                
                if( name != "" )
                {
                    if( spooler()->folder_subsystem()->is_valid_extension( extension ) ) 
                    {
                        File_path file_path       ( spooler()->folder_subsystem()->directory(), Absolute_path( path, filename ) );
                        Time      last_write_time ( file_info->last_write_time(), Time::is_utc );

                        xml_writer->begin_element( "configuration.file" );
                        xml_writer->set_attribute( "name"           , filename );
                        xml_writer->set_attribute( "last_write_time", last_write_time.xml_value() );
                        xml_writer->set_attribute( "md5"            , md5( string_from_file( file_path ) ) );
                        xml_writer->end_element( "configuration.file" );
                    }
                }
            }
        }
    }
}

//-----------------------------------------Supervisor_client_connection::update_directory_structure

void Supervisor_client_connection::update_directory_structure( const Absolute_path& directory_path, const xml::Element_ptr& element )
{
    assert( element );
    assert( element.nodeName_is( "configuration.directory" ) );
    assert( element.getAttribute( "name" ) == directory_path.name() );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        Absolute_path path      ( directory_path, e.getAttribute( "name" ) );
        File_path     file_path ( _spooler->folder_subsystem()->directory(), path );

        if( e.nodeName_is( "configuration.directory" ) )
        {
            if( e.bool_getAttribute( "removed", false ) )
            {
                log()->info( message_string( "SCHEDULER-702", path + "/" ) );
                if( file_path.name() == "" )  z::throw_xc( Z_FUNCTION, file_path );     // Vorsichtshalber
                file_path.remove_complete_directory();
            }
            else
            {
                log()->info( message_string( "SCHEDULER-702", path ) );

#               ifdef Z_WINDOWS
                    int err = mkdir( file_path.c_str() );
#                else
                    int err = mkdir( file_path.c_str(), 0700 );
#               endif

                if( err && errno != EEXIST )  zschimmer::throw_errno( errno, "mkdir" );

                update_directory_structure( path, e );
            }
        }
        else
        if( e.nodeName_is( "configuration.file" ) )
        {
            if( e.bool_getAttribute( "removed", false ) )
            {
                log()->info( message_string( "SCHEDULER-702", path ) );
                file_path.unlink();
            }
            else
            {
                const xml::Element_ptr& content_element = e.select_node_strict( "content" );
                string content;

                if( content_element.getAttribute( "encoding" ) == "base64" )  content = base64_decoded( content_element.text() );
                else
                    z::throw_xc( Z_FUNCTION, "invalid <content>-encoding" );

                log()->info( message_string( "SCHEDULER-701", path ) );

                File_path temporary_path = file_path + "~";
                //if( temporary_path.exists() )  temporary_path.unlink();     // Löschen, damit Dateirechte gesetzt werden können (Datei sollte nicht vorhanden sein)

                File file ( temporary_path, "wb" ); //, 0400 );                   // Nur lesbar, damit keiner versehentlich die Datei ändert
                file.print( content );
                file.close();

                time_t last_write_time = Time().set_datetime( e.getAttribute( "last_write_time" ) ).as_utc_time_t();

                struct utimbuf utimbuf;
                utimbuf.actime  = ::time(NULL);
                utimbuf.modtime = last_write_time;
                int err = utime( file.path().c_str(), &utimbuf );
                if( err )  zschimmer::throw_errno( errno, "utime", Z_FUNCTION );

                file.path().move_to( file_path );
            }
        }
        else
            assert(0);
    }
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

Remote_scheduler* Remote_scheduler_register::get( const Host_and_port& hp )
{
    Remote_scheduler* result = get_or_null( hp );
    if( !result ) z::throw_xc( "SCHEDULER-457", hp.as_string() );
    return result;
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

//---------------------------------------------------------------Remote_scheduler::Remote_scheduler

Remote_scheduler::Remote_scheduler( Supervisor* supervisor )              
: 
    Scheduler_object( supervisor->spooler(), this, type_remote_scheduler ),
    _zero_(this+1), 
    _supervisor(supervisor) 
{
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
        _logged_on         = true;
        _is_connected      = true;
        _scheduler_id      = register_scheduler_element.getAttribute( "scheduler_id" );
        _scheduler_version = register_scheduler_element.getAttribute( "version" );
        _connected_at      = Time::now();
    }


    _error = NULL;

    DOM_FOR_EACH_ELEMENT( register_scheduler_element, e )
    {
        if( e.nodeName_is( "ERROR" ) )  _error = xc_from_dom_error( e );
    }
}

//--------------------------------------------------------------------Remote_scheduler::execute_xml

ptr<Command_response> Remote_scheduler::execute_xml( const xml::Element_ptr& element, Command_processor* command_processor )
{
    if( element.nodeName_is( "supervisor.configuration.fetch_updated_files" ) )  
        return execute_configuration_fetch_updated_files( element, command_processor );
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
}

//--------------------------------------Remote_scheduler::execute_configuration_fetch_updated_files

ptr<Command_response> Remote_scheduler::execute_configuration_fetch_updated_files( const xml::Element_ptr& element, Command_processor* command_processor )
{
    assert( element.nodeName_is( "supervisor.configuration.fetch_updated_files" ) );
    if( command_processor->security_level() < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    if( !command_processor->communication_operation() )  z::throw_xc( "SCHEDULER-222", element.nodeName() );

    set_async_delay( double_time_max );   // UDP-Nachricht nicht mehr wiederholen


    _supervisor->switch_subsystem_state( subsys_active );
    _supervisor->remote_configurations()->activate();


    ptr<File_buffered_command_response> response = Z_NEW( File_buffered_command_response() );
    response->begin_standard_response();

    Xml_writer xml_writer ( response );

    xml_writer.begin_element( "configuration.directory" );
    write_updated_files_to_xml( &xml_writer, _supervisor->configuration_directory_for_remote_scheduler( this ), element );
    xml_writer.end_element( "configuration.directory" );

    xml_writer.close();
    response->end_standard_response();
    response->close();
    return +response;
}

//-----------------------------------------------------Remote_scheduler::write_updated_files_to_xml

void Remote_scheduler::write_updated_files_to_xml( Xml_writer* xml_writer, Directory* directory, const xml::Element_ptr& reference_element )
{
    list<Xml_file_info>           xml_file_info_list;
    vector<const Xml_file_info*>  xml_ordered_file_infos;   // Geordnete Liste der bereits bekannten (replizierten) Dateien
    

    if( reference_element )
    {
        DOM_FOR_EACH_ELEMENT( reference_element, e )
        {
            if( e.nodeName_is( "configuration.directory" )  ||
                e.nodeName_is( "configuration.file"      )     )
            {
                string filename = e.getAttribute( "name" );
                xml_file_info_list.push_back( Xml_file_info( e ) );
            }
        }
    }

    xml_ordered_file_infos.reserve( xml_file_info_list.size() );
    Z_FOR_EACH( list<Xml_file_info>, xml_file_info_list, it )  xml_ordered_file_infos.push_back( &*it );
    sort( xml_ordered_file_infos.begin(), xml_ordered_file_infos.end(), Base_file_info::less_dereferenced );


    directory->read( Directory::read_no_subdirectories );

    Directory::Entry_list       ::iterator e   = directory->_ordered_list.begin();
    vector<const Xml_file_info*>::iterator xfi = xml_ordered_file_infos.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen

    while( e   != directory->_ordered_list.end()  ||
           xfi != xml_ordered_file_infos.end() )
    {
        /// Dateinamen gleich?

        while( xfi != xml_ordered_file_infos.end()  &&
               e   != directory->_ordered_list.end()  &&
               (*xfi)->_filename == e->_file_info->path().name() )
        {
            if( e->_file_info->last_write_time() != (*xfi)->_timestamp_utc )
            {
                if( !e->is_aging() )  write_file_to_xml( xml_writer, directory, *e, *xfi );
            }

            e++, xfi++;
        }



        /// Dateien hinzugefügt?

        while( e != directory->_ordered_list.end()  &&
               ( xfi == xml_ordered_file_infos.end()  ||  e->_file_info->path().name() < (*xfi)->_filename ) )
        {
            if( !e->is_aging() )  write_file_to_xml( xml_writer, directory, *e, (Xml_file_info*)NULL );
            e++;
        }

        assert( e == directory->_ordered_list.end()  || 
                xfi == xml_ordered_file_infos.end()  ||
                e->_file_info->path().name() >= (*xfi)->_normalized_name );
        


        /// Dateien gelöscht?

        while( xfi != xml_ordered_file_infos.end()  &&
               ( e == directory->_ordered_list.end()  ||  e->_file_info->path().name() > (*xfi)->_filename ) )  // Datei entfernt?
        {
            xml_writer->begin_element( (*xfi)->_element.nodeName() );    // "configuration.directory" oder "configuration.file"
            xml_writer->set_attribute( "name"   , (*xfi)->_filename );
            xml_writer->set_attribute( "removed", "yes" );
            xml_writer->end_element( (*xfi)->_element.nodeName() );
            xfi++;
        }

        assert( xfi == xml_ordered_file_infos.end()  ||
                e == directory->_ordered_list.end()  ||
                e->_file_info->path().name() <= (*xfi)->_filename );
    }

    _configuration_version = directory->version();
}

//--------------------------------------------------------------Remote_scheduler::write_file_to_xml

void Remote_scheduler::write_file_to_xml( Xml_writer* xml_writer, Directory* directory, const Directory_entry& directory_entry,
                                          const Xml_file_info* reference )
{
    if( directory_entry._file_info->is_directory() )
    {
        xml_writer->begin_element( "configuration.directory" );
        xml_writer->set_attribute( "name", directory_entry._file_info->path().name() );

        write_updated_files_to_xml( xml_writer, directory_entry._subdirectory, reference? reference->_element : NULL );

        xml_writer->end_element( "configuration.directory" );
    }
    else
    {
        try
        {
            string content = string_from_file( File_path( directory->file_path(), directory_entry._file_info->path().name() ) );

            if( !reference  ||  reference->_md5 != md5( content ) )
            {
                xml_writer->begin_element( "configuration.file" );
                xml_writer->set_attribute( "name"           , directory_entry._file_info->path().name() );
                xml_writer->set_attribute( "last_write_time", Time( directory_entry._file_info->last_write_time(), Time::is_utc ).xml_value() );

                    xml_writer->begin_element( "content" );
                    xml_writer->set_attribute( "encoding", "base64" );

                        xml_writer->write( base64_encoded( content ) );

                    xml_writer->end_element( "content" );
                xml_writer->end_element( "configuration.file" );
            }
        }
        catch( exception& x )
        {
            if( !string_begins_with( x.what(), S() << "ERRNO-" << ENOENT ) )  log()->warn( x.what() );
        }
    }
}

//-----------------------------------------------------Remote_scheduler::check_remote_configuration

bool Remote_scheduler::check_remote_configuration()
{
    bool result = false;

    // Konfigurationsverzeichnis ist möglicherweise geändert worden.

    if( _udp_port )
    {
        if( Directory* directory = _supervisor->configuration_directory_for_remote_scheduler( this ) )
        {
            if( _configuration_version == directory->version() )  
                directory->read( Directory::read_subdirectories );

            if( _configuration_version != directory->version() )  
            {
                signal_remote_scheduler();
                result = true;
            }
        }
    }

    return result;
}

//--------------------------------------------------------Remote_scheduler::signal_remote_scheduler

void Remote_scheduler::signal_remote_scheduler()
{
    send_udp_message( Host_and_port( _host_and_port.host(), _udp_port ), "<check_folders/>" );  int EXCEPTION;
    set_async_delay( udp_timeout );   // Danach UDP-Nachricht wiederholen
}

//----------------------------------------------------------------Remote_scheduler::async_continue_

bool Remote_scheduler::async_continue_( Continue_flags )
{
    if( _is_connected )
    {
        signal_remote_scheduler();
    }

    return true;
}

//--------------------------------------------------------------------Remote_scheduler::dom_element

xml::Element_ptr Remote_scheduler::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr result = document.createElement( "remote_scheduler" );

    result.setAttribute         ( "ip"              , _host_and_port._host.ip_string() );
    result.setAttribute_optional( "hostname"        , _host_and_port._host.name() );
    result.setAttribute         ( "tcp_port"        , _host_and_port._port );
    result.setAttribute_optional( "scheduler_id"    , _scheduler_id );
    result.setAttribute         ( "version"         , _scheduler_version );

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

//-----------------------------------------------------------------------Remote_scheduler::obj_name

string Remote_scheduler::obj_name() const
{ 
    S result;
    result << Scheduler_object::obj_name();
    result << " ";
    result << _host_and_port.as_string();
    return result;
}

//----------------------------------------------------------Remote_scheduler::connection_lost_event
  
void Remote_scheduler::connection_lost_event( const exception* x )
{
    _disconnected_at = Time::now();
    _is_connected = false;

    if( _logged_on )  _error = x;
}

//-----------------------------------------------------Remote_configurations::Remote_configurations

Remote_configurations::Remote_configurations( Supervisor* supervisor, const File_path& directory_path )
:
    Scheduler_object( supervisor->spooler(), this, type_remote_configuration_observer ),
    _zero_(this+1),
    _supervisor(supervisor),
    _directory_watch_interval( folder::directory_watch_interval_max )
{
    _directory_tree = Z_NEW( Directory_tree( spooler(), directory_path ) );
}

//----------------------------------------------------Remote_configurations::~Remote_configurations
    
Remote_configurations::~Remote_configurations()
{
    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------Remote_configurations::close

void Remote_configurations::close()
{
    remove_from_event_manager();

#   ifdef Z_WINDOWS
        Z_LOG2( "scheduler", "FindCloseChangeNotification()\n" );
        FindCloseChangeNotification( _directory_event._handle );
        _directory_event._handle = NULL;
#   endif

    _directory_event.close();

    _directory_tree = NULL;
}

//------------------------------------------------------------------Remote_configurations::activate

void Remote_configurations::activate()
{
    if( !_is_activated )
    {
        _is_activated = true;

#       ifdef Z_WINDOWS

            assert( !_directory_event );

            _directory_event.set_name( "Remote_configurations " + _directory_tree->directory_path() );

            Z_LOG2( "scheduler", "FindFirstChangeNotification( \"" << _directory_tree->directory_path() << "\", TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
            
            HANDLE h = FindFirstChangeNotification( _directory_tree->directory_path().c_str(), 
                                                    TRUE,                               // Mit Unterverzeichnissen
                                                    FILE_NOTIFY_CHANGE_FILE_NAME  |  
                                                    FILE_NOTIFY_CHANGE_DIR_NAME   |
                                                    FILE_NOTIFY_CHANGE_LAST_WRITE );

            if( !h  ||  h == INVALID_HANDLE_VALUE )  throw_mswin( "FindFirstChangeNotification", _directory_tree->directory_path() );

            _directory_event._handle = h;


#       endif

        add_to_event_manager( _spooler->_connection_manager );
        set_async_delay( folder::directory_watch_interval_min );
    }
}

//-----------------------------------------------------------Remote_configurations::async_continue_

bool Remote_configurations::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler", Z_FUNCTION << " Prüfe Konfigurationsverzeichnis " << _directory_tree->directory_path() << "\n" );

    if( _directory_event.signaled() )
    {
        _directory_event.reset();
        

#       ifdef Z_WINDOWS
            Z_LOG2( "joacim", "FindNextChangeNotification(\"" << _directory_tree->directory_path() << "\")\n" );
            ok = FindNextChangeNotification( _directory_event );
            if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
#       endif
    }

    check();
    
    return true;
}

//---------------------------------------------------------Remote_configurations::async_state_text_

string Remote_configurations::async_state_text_() const
{
    S result;

    result << obj_name();

    return result;
}

//---------------------------------------------------------------------Remote_configurations::check

bool Remote_configurations::check( double minimum_age )
{
    log()->warn( Z_FUNCTION );

    bool   something_changed = false;
    double now               = double_from_gmtime();

    //if( _directory_tree->last_change_at() + minimum_age <= now )
    {
        //if( _directory_tree->refresh_aged_entries_at() < now )  _read_again_at = 0;     // Verstrichen?
        _directory_tree->reset_aging();

        something_changed = _supervisor->check_remote_configurations();
        
        _directory_watch_interval = now - _directory_tree->last_change_at() < folder::directory_watch_interval_max? folder::directory_watch_interval_min
                                                                                                                  : folder::directory_watch_interval_max;

        now = double_from_gmtime();
        set_async_next_gmtime( min( _directory_tree->refresh_aged_entries_at(), now + _directory_watch_interval ) );
    }

    return something_changed;
}

//-----------------------------------------------------------------------------------new_supervisor

ptr<Supervisor_interface> new_supervisor( Scheduler* scheduler )
{
    ptr<Supervisor> supervisor = Z_NEW( Supervisor( scheduler ) );
    return +supervisor;
}

//---------------------------------------------------------------------------Supervisor::Supervisor

Supervisor::Supervisor( Scheduler* scheduler )            
: 
    Supervisor_interface( scheduler, type_supervisor ), 
    _zero_(this+1) 
{
}

//--------------------------------------------------------------------------------Supervisor::close

void Supervisor::close()
{
    if( _remote_configurations )
    {
        _remote_configurations->close();
        _remote_configurations = NULL;
    }
}

//-----------------------------------------------------------------Supervisor::subsystem_initialize

bool Supervisor::subsystem_initialize()
{
    set_subsystem_state( subsys_initialized );
    return true;
}

//-----------------------------------------------------------------------Supervisor::subsystem_load

bool Supervisor::subsystem_load()
{
    _remote_configurations = Z_NEW( Remote_configurations( this, spooler()->_remote_configuration_directory ) );

    set_subsystem_state( subsys_loaded );
    return true;
}

//-------------------------------------------------------------------Supervisor::subsystem_activate

bool Supervisor::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    return true;
}

//----------------------------------------------------Supervisor::execute_register_remote_scheduler

void Supervisor::execute_register_remote_scheduler( const xml::Element_ptr& register_remote_scheduler_element, Communication::Operation* communication_operation )
{
    Xml_operation* xml_processor = dynamic_cast<Xml_operation*>( communication_operation );
    if( !xml_processor )  z::throw_xc( "SCHEDULER-222", register_remote_scheduler_element.nodeName() );
 
    Host_and_port host_and_port ( communication_operation->_connection->peer_host(), register_remote_scheduler_element.int_getAttribute( "tcp_port" ) );

    ptr<Remote_scheduler> remote_scheduler = _remote_scheduler_register.get_or_null( host_and_port );

    if( !remote_scheduler )  remote_scheduler = Z_NEW( Remote_scheduler( this ) );

    remote_scheduler->_host_and_port = host_and_port;
    remote_scheduler->_host_and_port._host.resolve_name();
    remote_scheduler->set_dom( register_remote_scheduler_element );
    remote_scheduler->_udp_port = register_remote_scheduler_element.int_getAttribute( "udp_port", 0 );
    
    xml_processor->_operation_connection->_remote_scheduler = +remote_scheduler;        // Remote_scheduler mit TCP-Verbindung verknüpfen
    _remote_scheduler_register.add( remote_scheduler );
}

//--------------------------------------------------------------------------Supervisor::execute_xml

ptr<Command_response> Supervisor::execute_xml( const xml::Element_ptr& element, Command_processor* command_processor )
{
    int tcp_port = element.int_getAttribute( "tcp_port", 0 );
    if( tcp_port == 0 )  z::throw_xc( Z_FUNCTION, "TCP port is missing" );

    Remote_scheduler* remote_scheduler = _remote_scheduler_register.get( Host_and_port( command_processor->communication_operation()->_connection->peer_host(), tcp_port ) );

    return remote_scheduler->execute_xml( element, command_processor );
}

//---------------------------------------------------Supervisor::read_configuration_directory_names

void Supervisor::read_configuration_directory_names()
{
    _configuration_directory_map.clear();

    Directory* root_directory = remote_configurations()->directory_tree()->root_directory();

    root_directory->read( Directory::read_no_subdirectories );

    Z_FOR_EACH_CONST( Directory::Entry_list, root_directory->_ordered_list, e )
    {
        if( e->_file_info->is_directory() )
        {
            string s   = e->_file_info->path().name();
            size_t pos = s.find( '#' );                     // Verzeichnisname "host#port"

            if( pos != string::npos )
            {
                s[ pos ] = ':';

                try
                {
                    Host_and_port host_and_port ( s );      // Exception bei unbekannten Hostnamen

                    Configuration_directory_map::iterator it = _configuration_directory_map.find( host_and_port );
                    if( it != _configuration_directory_map.end() )  z::throw_xc( "SCHEDULER-454", it->second, e->_file_info->path().name() );   // Nicht eindeutig

                    _configuration_directory_map[ host_and_port ] = e->_file_info->path().name();
                }
                catch( exception& x ) { Z_LOG( Z_FUNCTION << "  " << x.what() << "\n" ); }    // Ungültiger Verzeichnisname
            }
        }
    }


    // Überwachung starten (auch wenn beim Lesen der Dateien ein Fehler auftritt)
    // Async_operation - Verzeichnisbaum speichern und periodisch vergleichen
    // Windows: Änderungssignal ist nur ein Hinweis, der Baum für den Scheduler wird trotzdem verglichen
    // Baum des Schedulers im Register speichern? Verbindung zum Registereintrag herstellen
    // Der Scheduler braucht seine Dateien nicht zu übergeben, wenn der Supervisor doch schon bescheid weiß? Auch im Fehlerfall?
    //async_continue();
}

//-----------------------------------------Supervisor::configuration_directory_for_remote_scheduler

Directory* Supervisor::configuration_directory_for_remote_scheduler( Remote_scheduler* remote_scheduler )
{
    Directory* result = NULL;

    read_configuration_directory_names();

    Configuration_directory_map::iterator it = _configuration_directory_map.find( remote_scheduler->_host_and_port );
    if( it != _configuration_directory_map.end() )  
    {
        Directory* root_directory = remote_configurations()->directory_tree()->root_directory();
        result = root_directory->entry( it->second )._subdirectory;
    }

    return result;
}

//----------------------------------------------------------Supervisor::check_remote_configurations

bool Supervisor::check_remote_configurations()
{
    bool result = false;

    Z_FOR_EACH( Remote_scheduler_register::Map, _remote_scheduler_register._map, it )
    {
        Remote_scheduler* remote_scheduler = it->second;
        result |= remote_scheduler->check_remote_configuration();
    }

    return result;
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
    Supervisor_client_interface( scheduler, type_supervisor_client, &class_descriptor ),
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

//------------------------------------------------------------------Supervisor_client::get_Hostname

STDMETHODIMP Supervisor_client::get_Hostname( BSTR* result )
{
    HRESULT hr = S_OK;

    try
    {
        string hostname = _client_connection->host_and_port().host().name();
        if( hostname == "" )  hostname = _client_connection->host_and_port().host().ip_string();
        hr = String_to_bstr( hostname, result );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//------------------------------------------------------------------Supervisor_client::get_Tcp_port

STDMETHODIMP Supervisor_client::get_Tcp_port( int* result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = _client_connection->host_and_port().port();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace supervisor
} //namespace scheduler
} //namespace sos
