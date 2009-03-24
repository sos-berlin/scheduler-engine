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
struct Supervisor_client;

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
    void                        try_connect                 ();

  protected:
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return _state == s_not_connected  
                                                                                                          || _state == s_registered; }

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
    bool                       _start_update_configuration_delayed;
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
    void                        try_connect                 ()                                      { if( _client_connection )  _client_connection->try_connect(); }
    bool                        is_using_central_configuration() const                              { return _is_using_central_configuration; }
    Host_and_port               host_and_port               () const                                { return _client_connection? _client_connection->host_and_port() : Host_and_port(); }

    // IDispatch_implementation
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Supervisor_client"; }
    STDMETHODIMP            get_Hostname                    ( BSTR* );
    STDMETHODIMP            get_Tcp_port                    ( int* );

  private:
    friend struct               Supervisor_client_connection;

    Fill_zero                  _zero_;
    ptr<Supervisor_client_connection> _client_connection;
    bool                       _is_using_central_configuration;

    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//--------------------------------------------------------------------------------------------const
    
const int                               supervisor_connect_retry_time       = 60;
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

//----------------------------------------------------------------------------new_supervisor_client

ptr<Supervisor_client_interface> new_supervisor_client( Scheduler* scheduler, const Host_and_port& host_and_port )
{
    ptr<Supervisor_client> supervisor_client = Z_NEW( Supervisor_client( scheduler, host_and_port ) );
    return +supervisor_client;
}

//-------------------------------------------------------------Supervisor_client::Supervisor_client

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


    if( !_spooler->_configuration_directories[ confdir_cache ].exists() )
    {
        #ifdef Z_WINDOWS
            int err = mkdir( _spooler->_configuration_directories[ confdir_cache ].c_str() );
        #else
            int err = mkdir( _spooler->_configuration_directories[ confdir_cache ].c_str(), 0777 );
        #endif
        if( err )  z::throw_errno( errno, "mkdir", _spooler->_configuration_directories[ confdir_cache ].c_str() );
    }

    _spooler->folder_subsystem()->initialize_cache_directory();

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
    else
        _start_update_configuration_delayed = true;
}

//--------------------------------------------------------Supervisor_client_connection::try_connect

void Supervisor_client_connection::try_connect()
{
    if( _state == s_not_connected )
    {
        async_wake();
    }
}

//----------------------------------------------------Supervisor_client_connection::async_continue_

bool Supervisor_client_connection::async_continue_( Continue_flags )
{
    Z_DEBUG_ONLY( Z_LOGI2( "zschimmer", Z_FUNCTION << "\n" ); )

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
            
                if( _spooler->is_cluster() )
                xml_writer->set_attribute( "is_cluster_member", "yes" );

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
            {
                // Wird nach Verbindungsverlust nochmal durchlaufen

                if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;


                ptr<io::String_writer> string_writer = Z_NEW( io::String_writer() );
                ptr<xml::Xml_writer>   xml_writer    = Z_NEW( xml::Xml_writer( string_writer ) );

                xml_writer->set_encoding( scheduler_character_encoding );
                xml_writer->write_prolog();

                xml_writer->begin_element( "supervisor.remote_scheduler.configuration.fetch_updated_files" );

                if( !_spooler->_udp_port )  log()->warn( message_string( "SCHEDULER-899" ) );//, supervisor_configuration_poll_interval ) );

                write_directory_structure( xml_writer, root_path );

                xml_writer->end_element( "supervisor.remote_scheduler.configuration.fetch_updated_files" );
                xml_writer->close();

                _xml_client_connection->send( string_writer->to_string() );
                _state = s_fetching_configuration;
            }


            case s_fetching_configuration:
            {
                if( xml::Document_ptr response_document = _xml_client_connection->fetch_received_dom_document() )
                {
                    //log()->info( message_string( "SCHEDULER-950" ) );

                    if( xml::Element_ptr directory_element = response_document.select_node( "/spooler/answer/configuration.directory" ) )
                    {
                        _supervisor_client->_is_using_central_configuration = true;
                        update_directory_structure( root_path, directory_element );
                        _spooler->folder_subsystem()->handle_folders();     // cache-Verzeichnis würde reichen
                    }
                    
                    _state = s_configuration_fetched;
                }

                if( _state != s_configuration_fetched )  break;
            }


            case s_configuration_fetched:
                if( _start_update_configuration_delayed )  
                {
                    start_update_configuration();
                    _start_update_configuration_delayed = false;
                }

                _is_ready = true;   // Nach Wiederholung wegen Verbindungsverlusts bereits true

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

        set_async_delay( supervisor_connect_retry_time );
        something_done = true;
    }

    return something_done;
}

//--------------------------------------------------Supervisor_client_connection::async_state_text_

string Supervisor_client_connection::async_state_text_() const
{ 
    S result;

    result << obj_name();
    result << "(";
    if( _is_ready )  result << ", ready";
    if( _connection_failed )  result << ", connection failed";
    if( _start_update_configuration_delayed )  result << ", start_update_configuration_delayed ";
    result << ")";

    return result;
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
        File_path     file_path ( _spooler->_configuration_directories[ confdir_cache ], path );

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
#               ifdef Z_WINDOWS
                    int err = mkdir( file_path.c_str() );
#                else
                    int err = mkdir( file_path.c_str(), 0700 );
#               endif

                if( err )
                {
                    if( errno != EEXIST )  zschimmer::throw_errno( errno, "mkdir" );
                }
                else
                    log()->info( message_string( "SCHEDULER-701", path + "/" ) );

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

                Time last_write_time = Time().set_datetime( e.getAttribute( "last_write_time" ) );

                if( content_element.getAttribute( "encoding" ) == "base64" )  content = base64_decoded( content_element.text() );
                else
                    z::throw_xc( Z_FUNCTION, "invalid <content>-encoding" );

                log()->info( message_string( "SCHEDULER-701", path, last_write_time.as_string() ) );

                File_path temporary_path = file_path + "~";
                //if( temporary_path.exists() )  temporary_path.unlink();     // Löschen, damit Dateirechte gesetzt werden können (Datei sollte nicht vorhanden sein)

                File file ( temporary_path, "wb" ); //, 0400 );                   // Nur lesbar, damit keiner versehentlich die Datei ändert
                file.print( content );
                file.close();

                struct utimbuf utimbuf;
                utimbuf.actime  = ::time(NULL);
                utimbuf.modtime = last_write_time.as_utc_time_t();
                int err = utime( file.path().c_str(), &utimbuf );
                if( err )  zschimmer::throw_errno( errno, "utime", Z_FUNCTION );

                file.path().move_to( file_path );
            }
        }
        else
            assert(0);
    }
}

//------------------------------------------Supervisor_client_connection::write_directory_structure

void Supervisor_client_connection::write_directory_structure( xml::Xml_writer* xml_writer, const Absolute_path& path )
{
    Folder_directory_lister dir ( _log );
    
    bool ok = dir.open( _spooler->_configuration_directories[ confdir_cache ], path );

    if( ok )
    {
        while( ptr<file::File_info> file_info = dir.get() )
        {
            string filename = file_info->path().name();

            if( file_info->is_directory() )
            {
                xml_writer->begin_element( "configuration.directory" );
                xml_writer->set_attribute( "name", filename );

                write_directory_structure( xml_writer, Absolute_path( path, filename ) );

                xml_writer->end_element( "configuration.directory" );
            }
            else
            {
                string name      = Folder::object_name_of_filename( filename );
                string extension = Folder::extension_of_filename( filename );
                
                if( name != "" )
                {
                    //if( spooler()->folder_subsystem()->is_valid_extension( extension ) ) 
                    {
                        File_path file_path       ( spooler()->_configuration_directories[ confdir_cache ], Absolute_path( path, filename ) );
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

//---------------------------------------------------------Supervisor_client_connection::state_name

string Supervisor_client_connection::state_name( State state )
{
    switch( state )
    {
        case s_not_connected:           return "not_connected";
        case s_connecting:              return "connecting";
        case s_registering:             return "registering";
        case s_registered:              return "registered";
        case s_fetching_configuration:  return "fetching_configuration";
        case s_configuration_fetched:   return "configuration_fetched";
        default:                        return "state=" + as_string( state );
    }
}

//------------------------------------------------------------------Supervisor_client_connection::obj_name

string Supervisor_client_connection::obj_name() const
{
    return S() << "Supervisor_client_connection(" << _host_and_port << " " << state_name( _state ) << ")";
}

//-------------------------------------------------------------------------------------------------

} //namespace supervisor
} //namespace scheduler
} //namespace sos
