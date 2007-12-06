// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../zschimmer/base64.h"

#include <sys/utime.h>          // utime()
#include <sys/stat.h>           // mkdir()

#ifdef Z_WINDOWS
#   include <direct.h>          // mkdir()
#endif

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Supervisor_client_connection;

using xml::Xml_writer;

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

    xml::Element_ptr                   _element;
    Md5                                _md5;            // Leer bei einem Verzeichnis
};

//---------------------------------------------------------------------------------------Supervisor

struct Supervisor : Supervisor_interface
{
                                Supervisor                  ( Scheduler* );

    // Subsystem
    void                        close                       ();
    //bool                        subsystem_initialize        ();
    //bool                        subsystem_load              ();
    //bool                        subsystem_activate          ();

    // Supervisor_interface
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                        execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* );
    void                        execute_configuration_fetch_updated_files( Xml_writer*, const xml::Element_ptr&, Communication::Operation* );

  private:
    void                        write_updated_files_to_xml  ( Xml_writer*, const File_path&, const Absolute_path&, const xml::Element_ptr& );
    void                        write_file_to_xml           ( Xml_writer*, const File_path&, const Absolute_path&, file::File_info*, const xml::Element_ptr& );


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

//------------------------------------------------------------------------------file_info_is_lesser

inline bool file_info_is_lesser( const file::File_info* a, const file::File_info* b )
{
    return a->path() < b->path();
}

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
                xml_writer->set_attribute( "tcp_port"    , _spooler->_tcp_port   );
                xml_writer->set_attribute( "udp_port"    , _spooler->_udp_port   );
                xml_writer->set_attribute( "version"     , _spooler->_version    );
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
                if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

                ptr<io::String_writer> string_writer = Z_NEW( io::String_writer() );
                ptr<xml::Xml_writer>   xml_writer    = Z_NEW( xml::Xml_writer( string_writer ) );

                xml_writer->set_encoding( scheduler_character_encoding );
                xml_writer->write_prolog();

                xml_writer->begin_element( "supervisor.configuration.fetch_updated_files" );
                xml_writer->set_attribute( "scheduler_id"                  , _spooler->id() );
                xml_writer->set_attribute( "tcp_port"                      , _spooler->_tcp_port );
                xml_writer->set_attribute( "signal_next_change_at_udp_port", _spooler->_udp_port );

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

                break;
            }


            case s_configuration_fetched:
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
                if( file_path.name() == "" )  z::throw_xc( Z_FUNCTION, file_path );     // Vorsichtshalber
                file_path.remove_complete_directory();
            }
            else
            {
                int err = mkdir( file_path.c_str() );
                if( err && errno != EEXIST )  zschimmer::throw_errno( errno, "mkdir" );

                update_directory_structure( path, e );
            }
        }
        else
        if( e.nodeName_is( "configuration.file" ) )
        {
            if( e.bool_getAttribute( "removed", false ) )
            {
                file_path.unlink();
            }
            else
            {
                const xml::Element_ptr& content_element = e.select_node_strict( "content" );
                if( content_element.getAttribute( "encoding" ) == "base64" )
                {
                    File file ( file_path + "~", "w" );
                    file.print( base64_decoded( content_element.text() ) );
                    file.close();

                    time_t last_write_time = Time().set_datetime( e.getAttribute( "last_write_time" ) ).as_utc_time_t();

                    struct utimbuf utimbuf;
                    utimbuf.actime  = ::time(NULL);
                    utimbuf.modtime = last_write_time;
                    int err = utime( file.path().c_str(), &utimbuf );
                    if( err )  zschimmer::throw_errno( errno, "utime", Z_FUNCTION );

                    file.path().move_to( file_path );
                }
                else
                    z::throw_xc( Z_FUNCTION, "invalid <content>-encoding" );
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

//--------------------------------------------Supervisor::execute_configuration_fetch_updated_files

void Supervisor::execute_configuration_fetch_updated_files( Xml_writer* xml_writer, const xml::Element_ptr& element, Communication::Operation* communication_operation )
{
    assert( element.nodeName_is( "supervisor.configuration.fetch_updated_files" ) );

    int    tcp_port       = element.int_getAttribute( "tcp_port" );
    string directory_name;

    Directory_lister dir ( _spooler->_remote_configuration_directory );
    while( ptr<file::File_info> file_info = dir.get() )
    {
        if( file_info->is_directory() )
        {
            string s   = file_info->path().name();
            size_t pos = s.find( '#' );

            if( pos != string::npos )
            {
                s[ pos ] = ':';

                try
                {
                    Host_and_port host_and_port ( s );

                    if( host_and_port.ip()   == communication_operation->connection()->peer().ip()  &&  
                        host_and_port.port() == tcp_port )
                    {
                        if( directory_name != "" )  z::throw_xc( "SCHEDULER-454", directory_name, file_info->path().name() );
                        directory_name = file_info->path().name();         
                    }
                }
                catch( exception& x ) { Z_LOG( Z_FUNCTION << "  " << x.what() << "\n" ); }    // Ungültiger Verzeichnisname
            }
        }
    }
    dir.close();

    if( directory_name == "" )  z::throw_xc( "SCHEDULER-455", Host_and_port( communication_operation->connection()->peer().host(), tcp_port ).as_string() );


    xml_writer->begin_element( "configuration.directory" );

        write_updated_files_to_xml( xml_writer, File_path( _spooler->_remote_configuration_directory, directory_name ), root_path, element );

    xml_writer->end_element( "configuration.directory" );
}

//-----------------------------------------------------------Supervisor::write_updated_files_to_xml

void Supervisor::write_updated_files_to_xml( Xml_writer* xml_writer, const File_path& directory, const Absolute_path& path, const xml::Element_ptr& reference_element )
{
    Folder_directory_lister dir ( _log );
    dir.open( directory, path );

    if( dir.is_opened() )
    {
        list< ptr<file::File_info> >  file_info_list;
        list< Xml_file_info >       xml_file_info_list;
        
        while( ptr<file::File_info> file_info = dir.get() )  file_info_list.push_back( file_info );
        dir.close();

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

        vector<file::File_info*>      ordered_file_infos;       // Geordnete Liste der vorgefundenen Dateien    
        vector<const Xml_file_info*>  xml_ordered_file_infos;   // Geordnete Liste der bereits bekannten (replizierten) Dateien

        ordered_file_infos.reserve( file_info_list.size() );
        Z_FOR_EACH_CONST( list< ptr<file::File_info> >, file_info_list, it )  ordered_file_infos.push_back( *it );
        sort( ordered_file_infos.begin(), ordered_file_infos.end(), file_info_is_lesser );

        xml_ordered_file_infos.reserve( xml_file_info_list.size() );
        Z_FOR_EACH( list<Xml_file_info>, xml_file_info_list, it )  xml_ordered_file_infos.push_back( &*it );
        sort( xml_ordered_file_infos.begin(), xml_ordered_file_infos.end(), Base_file_info::less_dereferenced );


        vector<file::File_info*>    ::iterator fi  = ordered_file_infos.begin();
        vector<const Xml_file_info*>::iterator xfi = xml_ordered_file_infos.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen

        while( fi != ordered_file_infos.end()  ||
               xfi != xml_ordered_file_infos.end() )
        {
            /// Dateinamen gleich?

            while( xfi != xml_ordered_file_infos.end()  &&
                   fi  != ordered_file_infos.end()  &&
                   (*xfi)->_filename == (*fi)->path().name() )
            {
                if( (*fi)->last_write_time() != (*xfi)->_timestamp_utc  ||
                    md5( string_from_file( File_path( directory, Absolute_path( path, (*fi)->path().name() ) ) ) ) != (*xfi)->_md5 )  // MD5 vom Verzeichnis ist ""
                {
                    write_file_to_xml( xml_writer, directory, path, *fi, (*xfi)->_element );
                }

                fi++, xfi++;
            }



            /// Dateien hinzugefügt?

            while( fi != ordered_file_infos.end()  &&
                   ( xfi == xml_ordered_file_infos.end()  ||  (*fi)->path().name() < (*xfi)->_filename ) )
            {
                write_file_to_xml( xml_writer, directory, path, *fi, NULL );
                fi++;
            }

            assert( fi == ordered_file_infos.end()  || 
                    xfi == xml_ordered_file_infos.end() ||
                    (*fi)->path().name() >= (*xfi)->_normalized_name );
            


            /// Dateien gelöscht?

            while( xfi != xml_ordered_file_infos.end()  &&
                   ( fi == ordered_file_infos.end()  ||  (*fi)->path().name() > (*xfi)->_filename ) )  // Datei entfernt?
            {
                xml_writer->begin_element( (*fi)->is_directory()? "configuration.directory" : "configuration.file" );
                xml_writer->set_attribute( "removed", "yes" );
                xml_writer->end_element( (*fi)->is_directory()? "configuration.directory" : "configuration.file" );
                xfi++;
            }

            assert( xfi == xml_ordered_file_infos.end()  ||
                    fi == ordered_file_infos.end()  ||
                    (*fi)->path().name() <= (*xfi)->_filename );
        }
    }
}

//--------------------------------------------------------------------Supervisor::write_file_to_xml

void Supervisor::write_file_to_xml( Xml_writer* xml_writer, const File_path& directory, const Absolute_path& path, file::File_info* file_info, 
                                    const xml::Element_ptr& reference_element )
{
    xml_writer->begin_element( file_info->is_directory()? "configuration.directory" : "configuration.file" );
    xml_writer->set_attribute( "name"           , file_info->path().name() );
    xml_writer->set_attribute( "last_write_time", Time( file_info->last_write_time(), Time::is_utc ).xml_value() );

    if( file_info->is_directory() )
    {
        write_updated_files_to_xml( xml_writer, directory, path, reference_element );
    }
    else
    {

        try
        {
            string content = string_from_file( File_path( directory, Absolute_path( path, file_info->path().name() ) ) );

            xml_writer->begin_element( "content" );
            xml_writer->set_attribute( "encoding", "base64" );

                xml_writer->write( base64_encoded( content ) );

            xml_writer->end_element( "content" );
        }
        catch( exception& x )
        {
            _log->error( x.what() );
        }
    }

    xml_writer->end_element( file_info->is_directory()? "configuration.directory" : "configuration.file" );
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

} //namespace scheduler
} //namespace sos
