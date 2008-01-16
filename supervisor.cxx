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
struct Remote_configurations;

//--------------------------------------------------------------------------------------------const

//const int                       supervisor_configuration_poll_interval = 60;
const double                    udp_timeout                 = 60;
const double                    max_hostname_age            = 15*60;                                // Nach dieser Zeit gethostbyname() erneut rufen
const string                    directory_name_for_all_schedulers = "_all";
const double                    allowed_directory_age       = 0.0;                                  // Verzeichnis nur lesen, wenn letztes Lesen länger her ist

//------------------------------------------------------------------------------------Xml_file_info

struct Xml_file_info : Base_file_info
{
    Xml_file_info( const xml::Element_ptr& element )
    :
        Base_file_info( element.getAttribute( "name" ), Time().set_datetime( element.getAttribute( "last_write_time" ) ).as_utc_double(), element.getAttribute( "name" ), 0 ),
        _element( element )
    {
        string md5_hex = element.getAttribute( "md5" );
        if( md5_hex != "" )  _md5.set_hex( md5_hex );
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
    void                        write_file_to_xml           ( Xml_writer*, const Directory_entry&, const Xml_file_info* reference );
    bool                        check_remote_configuration  ();
    void                        signal_remote_scheduler     ();
  //Directory*                  configuration_directory     ();
    Directory*                  configuration_directory_or_null();
    string                      obj_name                    () const;

    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return false; }

    Fill_zero                  _zero_;
    Supervisor*                _supervisor;
    Remote_configurations*     _remote_configurations;
    Host_and_port              _host_and_port;
    int                        _udp_port;
    bool                       _is_cluster_member;
    string                     _scheduler_id;
    string                     _scheduler_version;
    Time                       _connected_at;
    Time                       _disconnected_at;
    bool                       _logged_on;
    bool                       _is_connected;
    Xc_copy                    _error;
    string                     _configuration_directory_name;
    int                        _configuration_version;
    int                        _configuration_version_all;
    bool                       _configuration_changed;
    Time                       _configuration_transfered_at;
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
    typedef stdext::hash_map< Host_and_port, ptr<Remote_scheduler> >   Map;
    Map                        _map;
};

//-----------------------------------------------------------------------------------Hostname_cache

struct Hostname_cache
{
    Ip_address                  try_resolve_name            ( const string& hostname );

    struct Hostname_map_entry
    {
                                Hostname_map_entry          ()                                      : _timestamp(0) {}
        Ip_address             _ip_address;
        double                 _timestamp;
    };

    typedef stdext::hash_map<string,Hostname_map_entry> Hostname_map;
    Hostname_map               _hostname_map;
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
    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }


    Directory_tree*             directory_tree              () const                                { return _directory_tree; }

    void                        activate                    ();
    bool                     is_activated                   () const                                { return _is_activated; }

    bool                        check                       ();
    void                        set_alarm                   ();
    void                        resolve_configuration_directory_names();
    Directory*                  configuration_directory_for_host_and_port( const Host_and_port& );
    Directory*                  configuration_directory_for_all_schedulers_or_null();


  private:
    Fill_zero                  _zero_;
    Supervisor*                _supervisor;
    Event                      _directory_event;
    ptr<Directory_tree>        _directory_tree;
    double                     _next_check_at;
    bool                       _is_activated;

    typedef stdext::hash_map< Host_and_port, string >  Hostport_directory_map;
    Hostport_directory_map     _hostport_directory_map;

    Hostname_cache             _hostname_cache;
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
    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }

  private:
    Fill_zero                  _zero_;
    ptr<Remote_configurations> _remote_configurations;

  public:
    Remote_scheduler_register  _remote_scheduler_register;
};

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
    set_subsystem_state( subsys_stopped );

    if( _remote_configurations )
    {
        _remote_configurations->close();
        _remote_configurations = NULL;
    }
}

//-----------------------------------------------------------------Supervisor::subsystem_initialize

bool Supervisor::subsystem_initialize()
{
    _remote_configurations = Z_NEW( Remote_configurations( this, spooler()->_central_configuration_directory ) );
    set_subsystem_state( subsys_initialized );
    return true;
}

//-----------------------------------------------------------------------Supervisor::subsystem_load

bool Supervisor::subsystem_load()
{
    set_subsystem_state( subsys_loaded );
    return true;
}

//-------------------------------------------------------------------Supervisor::subsystem_activate

bool Supervisor::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    return true;
}

//--------------------------------------------------------------------------Supervisor::execute_xml

ptr<Command_response> Supervisor::execute_xml( const xml::Element_ptr& element, Command_processor* command_processor )
{
    ptr<Command_response> result;

    if( string_begins_with( element.nodeName(), "supervisor.remote_scheduler." ) )
    {
        Xml_operation* xml_processor = dynamic_cast<Xml_operation*>( command_processor->communication_operation() );
        if( !xml_processor )  z::throw_xc( "SCHEDULER-222", element.nodeName() );

        Remote_scheduler_interface* remote_scheduler = xml_processor->_operation_connection->_remote_scheduler; 
        if( !remote_scheduler )  z::throw_xc( "SCHEDULER-457", command_processor->communication_operation()->_connection->peer_host() );

        result = remote_scheduler->execute_xml( element, command_processor );
    }
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );

    return result;
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

//--------------------------------------------------------------------------Supervisor::dom_element

xml::Element_ptr Supervisor::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    return _remote_scheduler_register.dom_element( dom_document, show_what );
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
    _supervisor(supervisor),
    _remote_configurations(supervisor->remote_configurations())
{
    assert( _remote_configurations );
}

//--------------------------------------------------------Remote_scheduler::configuration_directory
    
//Directory* Remote_scheduler::configuration_directory()
//{
//    Directory* result = configuration_directory_or_null();
//    if( !result)  throw_xc( "SCHEDULER-455", obj_name() );
//    return result;
//}

//------------------------------------------------Remote_scheduler::configuration_directory_or_null

Directory* Remote_scheduler::configuration_directory_or_null()
{
    Directory* result = NULL;

    //if( _configuration_directory_name != ""  &&
    //    File_path( _remote_configurations->directory_tree()->directory_path(), _configuration_directory_name ).exists() )
    {
        result = _remote_configurations->directory_tree()->directory_or_null( _configuration_directory_name );
    }
    
    if( !result )
    {
        result = _is_cluster_member? _remote_configurations->directory_tree()->directory_or_null( _scheduler_id )
                                   : _remote_configurations->configuration_directory_for_host_and_port( _host_and_port );
    }

    _configuration_directory_name = result? result->name() : "";

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
        _logged_on         = true;
        _is_connected      = true;
        _scheduler_id      = register_scheduler_element.     getAttribute( "scheduler_id" );
        _scheduler_version = register_scheduler_element.     getAttribute( "version" );
        _is_cluster_member = register_scheduler_element.bool_getAttribute( "is_cluster_member", false );
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
    if( element.nodeName_is( "supervisor.remote_scheduler.configuration.fetch_updated_files" ) )  
        return execute_configuration_fetch_updated_files( element, command_processor );
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
}

//--------------------------------------Remote_scheduler::execute_configuration_fetch_updated_files

ptr<Command_response> Remote_scheduler::execute_configuration_fetch_updated_files( const xml::Element_ptr& element, Command_processor* command_processor )
{
    assert( element.nodeName_is( "supervisor.remote_scheduler.configuration.fetch_updated_files" ) );
    if( command_processor->security_level() < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    if( !command_processor->communication_operation() )  z::throw_xc( "SCHEDULER-222", element.nodeName() );

    set_async_delay( double_time_max );   // UDP-Nachricht nicht mehr wiederholen


    _supervisor->switch_subsystem_state( subsys_active );
    _remote_configurations->activate();

    _remote_configurations->resolve_configuration_directory_names();


    Directory*     all_directory    = _remote_configurations->configuration_directory_for_all_schedulers_or_null();
    Directory*     my_directory     = configuration_directory_or_null();
    ptr<Directory> merged_directory = all_directory;
    
    if( all_directory )  all_directory->read_deep( allowed_directory_age );
    if( my_directory  )  my_directory ->read_deep( allowed_directory_age ),  merged_directory = my_directory;
    if( !merged_directory )  log()->info( message_string( "SCHEDULER-455", obj_name() ) );

    if( all_directory && my_directory )
    {
        merged_directory = my_directory->clone();
        merged_directory->merge_new_entries( all_directory );
    }


    ptr<File_buffered_command_response> response = Z_NEW( File_buffered_command_response() );
    response->begin_standard_response();

    Xml_writer xml_writer ( response );

    if( merged_directory )
    {
        xml_writer.begin_element( "configuration.directory" );
        write_updated_files_to_xml( &xml_writer, merged_directory, element );
        xml_writer.end_element( "configuration.directory" );
    }

    xml_writer.close();

    response->end_standard_response();
    response->close();


    _configuration_version       = my_directory ? my_directory ->version() : 0;
    _configuration_version_all   = all_directory? all_directory->version() : 0;
    _configuration_changed       = false;
    _configuration_transfered_at = Time::now();

    _remote_configurations->set_alarm();    // Für alternde Dateieinträge: Nach kurzer Zeit Verzeichnis nochmal prüfen 

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
                xml_file_info_list.push_back( Xml_file_info( e ) );
            }
        }
    }

    xml_ordered_file_infos.reserve( xml_file_info_list.size() );
    Z_FOR_EACH( list<Xml_file_info>, xml_file_info_list, it )  xml_ordered_file_infos.push_back( &*it );
    sort( xml_ordered_file_infos.begin(), xml_ordered_file_infos.end(), Base_file_info::less_dereferenced );


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
                if( !e->is_aging() )  write_file_to_xml( xml_writer, *e, *xfi );
            }

            e++, xfi++;
        }



        /// Dateien hinzugefügt?

        while( e != directory->_ordered_list.end()  &&
               ( xfi == xml_ordered_file_infos.end()  ||  e->_file_info->path().name() < (*xfi)->_filename ) )
        {
            if( !e->is_aging() )  write_file_to_xml( xml_writer, *e, (Xml_file_info*)NULL );
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
}

//--------------------------------------------------------------Remote_scheduler::write_file_to_xml

void Remote_scheduler::write_file_to_xml( Xml_writer* xml_writer, const Directory_entry& directory_entry, const Xml_file_info* reference )
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
            string content = string_from_file( directory_entry._file_info->path() );

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
    // Konfigurationsverzeichnis ist möglicherweise geändert worden.
    
    bool changed = false;

    if( _udp_port )
    {
        if( Directory* configuration_directory = this->configuration_directory_or_null() )
        {
            if( _configuration_version == configuration_directory->version() )  configuration_directory->read_deep( 0.0 );
            changed = _configuration_version != configuration_directory->version();
        }

        if( !changed )
        {
            if( Directory* all_directory = _remote_configurations->configuration_directory_for_all_schedulers_or_null() )
                changed = _configuration_version_all != all_directory->version();
        }

        if( changed )  
        {
            signal_remote_scheduler();
        }
    }

    _configuration_changed |= changed;

    return changed;
}

//--------------------------------------------------------Remote_scheduler::signal_remote_scheduler

void Remote_scheduler::signal_remote_scheduler()
{
    send_udp_message( Host_and_port( _host_and_port.host(), _udp_port ), "<check_folders/>" );
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

//--------------------------------------------------------------Remote_scheduler::async_state_text_

string Remote_scheduler::async_state_text_() const
{ 
    S result;
    result << obj_name(); 
    return result;
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

    result.setAttribute_optional( "configuration_directory", _configuration_directory_name );

    if( _configuration_changed )
    result.setAttribute         ( "configuration_changed", "yes" );

    if( _configuration_transfered_at )
    result.setAttribute         ( "configuration_transfered_at", _configuration_transfered_at.xml_value() );
    
    if( _error )
    append_error_element( result, _error );

    return result;
}

//-----------------------------------------------------------------------Remote_scheduler::obj_name

string Remote_scheduler::obj_name() const
{ 
    S result;

    result << Scheduler_object::obj_name();
    if( _scheduler_id != "" )  result << " " << _scheduler_id;
    result << " (" << _host_and_port.as_string() << ")";

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
    _supervisor(supervisor)
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
        Z_LOG2( "scheduler", "FindCloseChangeNotification(" << _directory_event << ")\n" );
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
        if( !_directory_tree->directory_path().exists()  ||
            !file::File_info( _directory_tree->directory_path() ).is_directory() )  z::throw_xc( "SCHEDULER-458", _directory_tree->directory_path() );

#       ifdef Z_WINDOWS
        {
            assert( !_directory_event );

            _directory_event.set_name( "Remote_configurations " + _directory_tree->directory_path() );

            Z_LOG2( "scheduler", "FindFirstChangeNotification( \"" << _directory_tree->directory_path() << "\", TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
            
            HANDLE h = FindFirstChangeNotification( _directory_tree->directory_path().c_str(), 
                                                    TRUE,                               // Mit Unterverzeichnissen
                                                    FILE_NOTIFY_CHANGE_FILE_NAME  |  
                                                    FILE_NOTIFY_CHANGE_DIR_NAME   |
                                                    FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                    FILE_NOTIFY_CHANGE_SIZE       );

            if( !h  ||  h == INVALID_HANDLE_VALUE )  throw_mswin( "FindFirstChangeNotification", _directory_tree->directory_path() );

            _directory_event._handle = h;
        }
#       endif

        add_to_event_manager( _spooler->_connection_manager );
        set_async_delay( folder::directory_watch_interval_min );

        _is_activated = true;
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
            for( int i = 0; i < 2; i++ )
            {
                Z_LOG2( "joacim", "FindNextChangeNotification(\"" << _directory_tree->directory_path() << "\")\n" );
                BOOL ok = FindNextChangeNotification( _directory_event );
                if( !ok )  throw_mswin_error( "FindNextChangeNotification" );

                DWORD ret = WaitForSingleObject( _directory_event, 0 );     // Warum wird es doppelt signalisiert?
                if( ret != WAIT_OBJECT_0 )  break;                          // Mit dieser Schleife wird async_continue_ bei einem Ereignis nicht doppelt gerufen
            }
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

bool Remote_configurations::check()
{
    bool   something_changed = false;
    double now               = double_from_gmtime();

    //if( _directory_tree->last_change_at() + minimum_age <= now )
    {
        //if( _directory_tree->refresh_aged_entries_at() < now )  _read_again_at = 0;     // Verstrichen?
        _directory_tree->reset_aging();
        resolve_configuration_directory_names();
        
        if( Directory* all_directory = configuration_directory_for_all_schedulers_or_null() )
            all_directory->read_deep( 0.0 );

        Z_FOR_EACH( Remote_scheduler_register::Map, _supervisor->_remote_scheduler_register._map, it )
        {
            Remote_scheduler* remote_scheduler = it->second;
            
            if( remote_scheduler->_is_connected )
            {
                something_changed |= remote_scheduler->check_remote_configuration();
            }
        }

       
        double interval = now - _directory_tree->last_change_at() < folder::directory_watch_interval_max? folder::directory_watch_interval_min
                                                                                                        : folder::directory_watch_interval_max;
        _next_check_at = now + interval;

        set_alarm();
    }

    return something_changed;
}

//------------------------------------------------------------------Remote_configuration::set_alarm

void Remote_configurations::set_alarm()
{
    set_async_next_gmtime( min( _directory_tree->refresh_aged_entries_at(), _next_check_at ) );
}

//-------------------------------------Remote_configurations::resolve_configuration_directory_names

void Remote_configurations::resolve_configuration_directory_names()
{
    bool has_changed = directory_tree()->root_directory()->read_without_subdirectories();

    if( has_changed )
    {
        Hostport_directory_map                 new_hostport_directory_map;
        stdext::hash_map<string,Host_and_port> directory_map;

        Z_FOR_EACH( Hostport_directory_map, _hostport_directory_map, it )  directory_map[ it->second ] = it->first;     // Um gethostbyname() bekannter Verzeichnisse zu vermeiden
   
        Z_FOR_EACH_CONST( Directory::Entry_list, directory_tree()->root_directory()->_ordered_list, e )
        {
            if( e->_file_info->is_directory() )
            {
                string d   = e->_file_info->path().name();
                size_t pos = d.find( '#' );                     // Verzeichnisname "host#port"

                if( pos != string::npos )
                {
                    stdext::hash_map<string,Host_and_port>::iterator it = directory_map.find( d );
                    if( it != directory_map.end() )     // Verzeichnis ist schon bekannt?
                    {
                        new_hostport_directory_map[ it->second ] = it->first;
                    }
                    else
                    {
                        try
                        {
                            Host_and_port host_and_port;
                            host_and_port._host = _hostname_cache.try_resolve_name( d.substr( 0, pos ) );
                            
                            if( host_and_port._host != Ip_address( 0, 0, 0, 0 ) )
                            {
                                host_and_port._port = as_int( d.substr( pos + 1 ) );                            // Exception

                                Hostport_directory_map::iterator it = new_hostport_directory_map.find( host_and_port );
                                if( it != new_hostport_directory_map.end() )  z::throw_xc( "SCHEDULER-454", it->second, e->_file_info->path().name() );   // Nicht eindeutig

                                new_hostport_directory_map[ host_and_port ] = e->_file_info->path().name();
                            }
                        }
                        catch( exception& x ) { log()->warn( x.what() ); }    // Ungültiger Verzeichnisname
                    }
                }
            }
        }

        _hostport_directory_map = new_hostport_directory_map;
    }
}

//---------------------------------Remote_configurations::configuration_directory_for_host_and_port

Directory* Remote_configurations::configuration_directory_for_host_and_port( const Host_and_port& host_and_port )
{
    Directory* result = NULL;

    Hostport_directory_map::iterator it = _hostport_directory_map.find( host_and_port );
    if( it != _hostport_directory_map.end() )  
    {
        Directory* root_directory = directory_tree()->root_directory();
        if( const Directory_entry* entry = root_directory->entry_or_null( it->second ) )
            result = entry->_subdirectory;
    }

    return result;
}

//------------------------Remote_configurations::configuration_directory_for_all_schedulers_or_null

Directory* Remote_configurations::configuration_directory_for_all_schedulers_or_null()
{
    return directory_tree()->directory_or_null( directory_name_for_all_schedulers );
}

//---------------------------------------------------------------------Hostname_cache::resolve_name

Ip_address Hostname_cache::try_resolve_name( const string& hostname )
{
    Hostname_map_entry& e   = _hostname_map[ lcase( hostname ) ];

    if( !e._timestamp  ||  e._timestamp + max_hostname_age < double_from_gmtime() )     // Neuer oder veralteter Eintrag?
    {
        e._timestamp  = double_from_gmtime();

        try
        {
            e._ip_address = Host( hostname );           // gethostbyname(), kann Exception auslösen
        }
        catch( exception& x )
        {
            Z_LOG( "ERROR " << Z_FUNCTION << " gethostbyname() ==> " << x.what() << "\n" );
            e._ip_address = Ip_address( 0, 0, 0, 0 );
        }
    }

    return e._ip_address;
}

//-------------------------------------------------------------------------------------------------

} //namespace supervisor
} //namespace scheduler
} //namespace sos
