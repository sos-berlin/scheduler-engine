// $Id: supervisor.cxx 14013 2010-09-14 07:21:33Z ss $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

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

struct Supervisor;struct Supervisor_client;
struct Remote_configurations;

//--------------------------------------------------------------------------------------------const

const double                    udp_timeout                 = 30;
const double                    udp_timeout_warn_period     = 5*60;
const double                    udp_warn_timeout            = 60;
const double                    max_hostname_age            = 15*60;                                // Nach dieser Zeit gethostbyname() erneut rufen
const string                    directory_name_for_all_schedulers = "_all";
const double                    allowed_directory_age       = 0.0;                                  // Verzeichnis nur lesen, wenn letztes Lesen länger her ist
const int                       default_deactivation_timeout = 3600;    // Wird bisher nicht verwendet, weil Client stets die Angabe liefert

//------------------------------------------------------------------------------------Xml_file_info

struct Xml_file_info : Base_file_info
{
    Xml_file_info( const xml::Element_ptr& element )
    :
        Base_file_info( element.getAttribute( "name" ), 
                        (time_t)Time::of_utc_date_time( element.getAttribute( "last_write_time" ) ).as_utc_double(), 
                        element.getAttribute( "name" ) ),
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
                          Abstract_scheduler_object
{
                                Remote_scheduler            (Supervisor* supervisor, const string& id);

    void                        update                      (const xml::Element_ptr&);

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& show );

    ptr<Command_response>       execute_xml                 ( const xml::Element_ptr&, Command_processor* );

    // Für Clients vor v1.6
    ptr<Command_response>       execute_configuration_fetch_updated_files(const xml::Element_ptr&, Command_processor*);
    void                        register_me                 (const xml::Element_ptr&, Communication::Operation*);
    void                        connection_lost_event       (const exception*);
    void                    set_dom                         (const xml::Element_ptr&);

    ptr<Command_response>       fetch_updated_files         (const xml::Element_ptr&);
    void                        write_updated_files_to_xml  ( Xml_writer*, Directory*, const xml::Element_ptr& reference_element );
    void                        write_file_to_xml           ( Xml_writer*, const Directory_entry&, const Xml_file_info* reference );
    bool                        check_remote_configuration  ();
    void                        signal_remote_scheduler     ();
    void                        check_timeout               ();
    Directory*                  configuration_directory_or_null();
    void                        set_alarm_clock             ();
    bool                        is_yet_active               () const  { return _is_active && (_is_connected || ::time(NULL) < _deactivate_at); }
    string                      obj_name                    () const;

    void set_host_and_udp(const Host& host, int udp_port) {
        _host = host;
        _host.resolve_name();
        _udp_port = udp_port;
    }

    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return false; }

    Fill_zero                  _zero_;
    Supervisor*                _supervisor;
    Remote_configurations*     _remote_configurations;
    string const               _id;                         // Neuer Client: (Scheduler-ID, host, port), alter Client: host:tcp_port
    Host                       _host;
    int                        _udp_port;
    bool                       _use_scheduler_id;
    string                     _scheduler_id;
    string                     _scheduler_version;
    time_t                     _connected_at;
    time_t                     _active_since;
    time_t                     _deactivate_at;
    time_t                     _disconnected_at;
    bool                       _is_active;
    bool                       _is_connected;
    Xml_operation_connection*  _connection_operation;                 /*! \change JS-481 Merken der Connection, um neue Connection damit zu vergleichen */
    bool                       _repeated_reading;
    Xc_copy                    _error;
    string                     _configuration_directory_name;
    int                        _configuration_version;
    int                        _configuration_version_all;
    bool                       _configuration_changed;
    time_t                     _configuration_changed_at;
    time_t                     _configuration_transfered_at;
    time_t                     _udp_timeout_warned_at;
};

//------------------------------------------------------------------------Remote_scheduler_register

struct Remote_scheduler_register
{
                                Remote_scheduler_register   ()                                      : _zero_(this+1){}

    void                        add                         ( Remote_scheduler* );
    Remote_scheduler*           get                         ( const string& );
    Remote_scheduler*           get_or_null                 ( const string& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    Fill_zero                  _zero_;
    typedef stdext::hash_map< string, ptr<Remote_scheduler> >   Map;
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

struct Remote_configurations : Object,
                               Abstract_scheduler_object,
                               Directory_observer::Directory_handler
{
                                Remote_configurations       ( Supervisor*, const File_path& directory );
                               ~Remote_configurations       ();


    // Directory_observer::Directory_handler:
    bool                        on_handle_directory          ( Directory_observer* );


    void                        close                       ();
    Directory_tree*             directory_tree              () const                                { return _directory_observer? _directory_observer->directory_tree() : NULL; }
    bool                        activate                    ();
    void                        set_alarm                   ();
    void                        resolve_configuration_directory_names();
    Directory*                  configuration_directory_for_host_and_port( const Host_and_port& );
    Directory*                  configuration_directory_for_all_schedulers_or_null();


  private:
    Fill_zero                  _zero_;
    Supervisor*                _supervisor;
    ptr<Directory_observer>    _directory_observer;
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
    string                      name                        () const                                { return "supervisor"; }
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    // Supervisor_interface
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                        execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* );
    ptr<Command_response>       execute_xml                 ( const xml::Element_ptr&, Command_processor* );
    ptr<Command_response>       execute_configuration_fetch (const xml::Element_ptr&, Security::Level, const Host&);


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
    if (element.nodeName_is("supervisor.configuration.fetch")) {
        return execute_configuration_fetch(element, command_processor->security_level(), command_processor->client_host());
    }
    else
    if (string_begins_with(element.nodeName(), "supervisor.remote_scheduler.")) {
        // Für Clients vor v1.6
        Xml_operation* xml_processor = dynamic_cast<Xml_operation*>( command_processor->communication_operation() );
        if( !xml_processor )  z::throw_xc( "SCHEDULER-222", element.nodeName() );

        Remote_scheduler_interface* remote_scheduler = xml_processor->_operation_connection->_remote_scheduler; 
        if( !remote_scheduler )  z::throw_xc( "SCHEDULER-457", command_processor->client_host());

        return remote_scheduler->execute_xml( element, command_processor );
    }
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
}

//--------------------------------------------------------------------------Supervisor::execute_xml

ptr<Command_response> Supervisor::execute_configuration_fetch(const xml::Element_ptr& element, Security::Level security_level, const Host& client_host)
{
    if (security_level < Security::seclev_no_add)  z::throw_xc("SCHEDULER-121");
    assert(element.nodeName_is("supervisor.configuration.fetch"));

    string scheduler_id = element.getAttribute("scheduler_id");
    string cluster_member_id = element.getAttribute("cluster_member_id");
    int udp_port = element.int_getAttribute("udp_port", 0);
    Host host = client_host;
    if (element.hasAttribute("ip")) 
        host = element.getAttribute("ip");
    string id = S() << (!cluster_member_id.empty() ? cluster_member_id : scheduler_id) << "," << host.ip_string() << ":" << udp_port;
    ptr<Remote_scheduler> remote_scheduler = _remote_scheduler_register.get_or_null(id);
    if (!remote_scheduler) {
        remote_scheduler = Z_NEW(Remote_scheduler(this, id));
        _remote_scheduler_register.add(remote_scheduler);
    }
    remote_scheduler->set_host_and_udp(host, udp_port);
    remote_scheduler->set_async_manager(_spooler->_connection_manager);
    remote_scheduler->update(element);
    remote_scheduler->set_alarm_clock();

    return remote_scheduler->fetch_updated_files(element);
}

//-------------------------------------------------------------------------Remote_scheduler::update

void Remote_scheduler::update(const xml::Element_ptr& element) {
    _is_active = true;
    _is_connected = false;
    _scheduler_id = element.getAttribute("scheduler_id");
    _scheduler_version = element.getAttribute("version");
    _use_scheduler_id = true;
    _active_since = ::time(NULL);
    _connected_at = _connected_at;
    _error = NULL;
    _deactivate_at = _active_since + element.int_getAttribute("interval", default_deactivation_timeout) * 2;   // Wenn nach doppelter Polling-Zeit der Client sich nicht wieder gemeldet hat, deaktivieren wir ihn.
}

//----------------------------------------------------Supervisor::execute_register_remote_scheduler
// Für Clients vor v1.6

void Supervisor::execute_register_remote_scheduler( const xml::Element_ptr& register_remote_scheduler_element, Communication::Operation* communication_operation )
{
    Host host = communication_operation->_connection->peer_host();
    string id = S() << host.name_or_ip() << ":" << register_remote_scheduler_element.int_getAttribute("tcp_port");
    int udp_port = register_remote_scheduler_element.int_getAttribute("udp_port", 0);

    ptr<Remote_scheduler> remote_scheduler = _remote_scheduler_register.get_or_null(id);
    if( !remote_scheduler )  remote_scheduler = Z_NEW(Remote_scheduler(this, id));
    remote_scheduler->set_host_and_udp(host, udp_port);
    remote_scheduler->register_me( register_remote_scheduler_element, communication_operation);
    _remote_scheduler_register.add( remote_scheduler );
}

//--------------------------------------------------------------------Remote_scheduler::register_me

// Für Clients vor v1.6
void Remote_scheduler::register_me( const xml::Element_ptr& register_remote_scheduler_element, Communication::Operation* communication_operation) 
{
    Xml_operation* xml_processor = dynamic_cast<Xml_operation*>( communication_operation );
    if( !xml_processor )  z::throw_xc( "SCHEDULER-222", register_remote_scheduler_element.nodeName() );

    Xml_operation_connection* connection_operation = xml_processor->_operation_connection;
 

    if ( Remote_scheduler_interface* r = connection_operation->_remote_scheduler ) {
        if ( r != this )
            Z_LOG2("scheduler",*this << ": The TCP connection is already linked with ANOTHER remote JobScheduler: " << *r << " <-- " << *connection_operation << "\n" );
        else
            Z_LOG2("scheduler", "The TCP connection is linked with the SAME remote JobScheduler: " << *r << " <-- " << *connection_operation << "\n" );
    }

    /*! \change JS-481 Es gibt bereits eine Verbindung und die neue Connection ist nicht dieselbe. Dass darf nicht sein. */
    if( _connection_operation  &&  _connection_operation != connection_operation ) {
        _log->warn( message_string( "SCHEDULER-714", _id, _connection_operation->_connection->obj_name(), connection_operation->_connection->obj_name() ) );
        _connection_operation->_connection->remove_me();
        _connection_operation = NULL;
        _is_connected = false;
    }


    _connection_operation = connection_operation; /*! \change JS-481 Connection merken */
    set_dom( register_remote_scheduler_element );

    connection_operation->_remote_scheduler = this;        // Remote_scheduler mit TCP-Verbindung verknüpfen
}

//--------------------------------------------------------------------------Supervisor::dom_element

xml::Element_ptr Supervisor::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    return _remote_scheduler_register.dom_element( dom_document, show_what );
}

//-----------------------------------------------------------Remote_scheduler_register::get_or_null

Remote_scheduler* Remote_scheduler_register::get(const string& id)
{
    Remote_scheduler* result = get_or_null(id);
    if( !result ) z::throw_xc( "SCHEDULER-457", id);
    return result;
}

//-----------------------------------------------------------Remote_scheduler_register::get_or_null

Remote_scheduler* Remote_scheduler_register::get_or_null(const string& id)
{
    Map::iterator it = _map.find(id);
    return it != _map.end()? it->second : NULL;
}

//-------------------------------------------------------------------Remote_scheduler_register::add

void Remote_scheduler_register::add( Remote_scheduler* remote_scheduler )
{
    _map[ remote_scheduler->_id ] = remote_scheduler;
}

//-----------------------------------------------------------Remote_scheduler_register::dom_element

xml::Element_ptr Remote_scheduler_register::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr result = document.createElement( "remote_schedulers" );

    int n = 0;
    int active_count = 0;
    Z_FOR_EACH( Map, _map, s ) {
        Remote_scheduler* remote_scheduler = s->second;
        n++;
        if (remote_scheduler->_is_active) 
            active_count++;
        if (show.is_set( show_remote_schedulers))  
            result.appendChild( remote_scheduler->dom_element( document, show ) );
    }

    result.setAttribute("count", n);
    result.setAttribute("active", active_count);

    return result;
}

//---------------------------------------------------------------Remote_scheduler::Remote_scheduler

Remote_scheduler::Remote_scheduler(Supervisor* supervisor, const string& id)
: 
    Abstract_scheduler_object( supervisor->spooler(), this, type_remote_scheduler ),
    _zero_(this+1), 
    _supervisor(supervisor),
    _remote_configurations(supervisor->remote_configurations()),
    _id(id)
{
    _log->set_prefix(obj_name());
    assert( _remote_configurations );
}

//------------------------------------------------Remote_scheduler::configuration_directory_or_null

Directory* Remote_scheduler::configuration_directory_or_null()
{
    Directory* result = NULL;

    if( _configuration_directory_name != "" ) // &&
    //    File_path( _remote_configurations->directory_tree()->directory_path(), _configuration_directory_name ).exists() )
    {
        result = _remote_configurations->directory_tree()->directory_or_null( _configuration_directory_name );
    }
    
    if( !result )
    {
        result = _use_scheduler_id? _remote_configurations->directory_tree()->directory_or_null(_scheduler_id)
                                  : _remote_configurations->configuration_directory_for_host_and_port(_id);
    }

    _configuration_directory_name = result? result->name() : "";

    return result;
}

//------------------------------------------------------------------------Remote_scheduler::set_dom

// Für Clients vor v1.6
void Remote_scheduler::set_dom( const xml::Element_ptr& register_scheduler_element )
{
    if( !register_scheduler_element )  return;

    if( register_scheduler_element.bool_getAttribute( "logoff", false ) )
    {
        _is_active = false;
    }
    else
    {
        _is_active         = true;
        _is_connected      = true;
        _scheduler_id      = register_scheduler_element.     getAttribute( "scheduler_id" );
        _scheduler_version = register_scheduler_element.     getAttribute( "version" );
        _use_scheduler_id  = register_scheduler_element.bool_getAttribute( "is_cluster_member", false );
        _connected_at      = ::time(NULL);
        _disconnected_at   = 0;
        _active_since      = _connected_at;
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
    if (element.nodeName_is("supervisor.remote_scheduler.configuration.fetch_updated_files"))
        return execute_configuration_fetch_updated_files(element, command_processor);
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
}

//--------------------------------------Remote_scheduler::execute_configuration_fetch_updated_files

// Für Clients vor v1.6
ptr<Command_response> Remote_scheduler::execute_configuration_fetch_updated_files(const xml::Element_ptr& element, Command_processor* command_processor)
{
    assert(element.nodeName_is("supervisor.remote_scheduler.configuration.fetch_updated_files"));
    if (command_processor->security_level() < Security::seclev_no_add)  z::throw_xc("SCHEDULER-121");
    if (!command_processor->communication_operation())  z::throw_xc("SCHEDULER-222", element.nodeName());
    set_alarm_clock();
    return fetch_updated_files(element);
}

//----------------------------------------------------------------Remote_scheduler::set_alarm_clock

void Remote_scheduler::set_alarm_clock() 
{
    if (_is_active) {
        if (_configuration_changed)
            set_async_delay(udp_timeout);
        else
        if (_deactivate_at)  // 0 bei alten Verfahren mit ständiger Verbindung
            set_async_next_gmtime(_deactivate_at);
    }
}

//------------------------------------------------------------Remote_scheduler::fetch_updated_files

ptr<Command_response> Remote_scheduler::fetch_updated_files(const xml::Element_ptr& element)
{
    ptr<Directory_tree> empty_directory_tree = Z_NEW( Directory_tree( _spooler, File_path(), confdir_none ) );
    ptr<Directory>      directory;
    Directory*          all_directory        = NULL;
    Directory*          my_directory         = NULL;


    _supervisor->switch_subsystem_state( subsys_active );
    bool is_active = _remote_configurations->activate(); 

    if( is_active )
    {
        _remote_configurations->resolve_configuration_directory_names();

        all_directory = _remote_configurations->configuration_directory_for_all_schedulers_or_null();
        my_directory  = configuration_directory_or_null();
        

        if( all_directory )  all_directory->read_deep( allowed_directory_age ),  directory = all_directory;
        if( my_directory  )  my_directory ->read_deep( allowed_directory_age ),  directory = my_directory;

        if( all_directory && my_directory )
        {
            directory = my_directory->clone();
            directory->merge_new_entries( all_directory->clone() );
        }
    }

    if( !directory )  
    {
        log()->info( message_string( "SCHEDULER-455", obj_name() ) );
        directory = empty_directory_tree->root_directory();
        assert( directory );
    }

    if( !_repeated_reading )    // Beim ersten Mal müssen die Dateien sofort geliefert werden. Also nicht altern lassen! 
    {
        directory = directory->clone();     // Nur in der Kopie die Alterung zurücknehmen
        directory->withdraw_aging_deep();     
        _repeated_reading = true;
    }


    ptr<File_buffered_command_response> response = Z_NEW( File_buffered_command_response() );
    response->begin_standard_response();

    Xml_writer xml_writer ( response );

    xml_writer.begin_element( "configuration.directory" );
    write_updated_files_to_xml( &xml_writer, directory, element );
    xml_writer.end_element( "configuration.directory" );

    xml_writer.close();

    response->end_standard_response();
    response->close();


    _configuration_version       = my_directory ? my_directory ->version() : 0;
    _configuration_version_all   = all_directory? all_directory->version() : 0;
    _configuration_changed       = false;
    _configuration_transfered_at = ::time(NULL);

    if( is_active )  _remote_configurations->set_alarm();    // Für alternde Dateieinträge: Nach kurzer Zeit Verzeichnis nochmal prüfen 
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
            if( e->_file_info->last_write_time() != (*xfi)->_last_write_time )
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

        write_updated_files_to_xml( xml_writer, directory_entry._subdirectory, reference? reference->_element : xml::Element_ptr() );

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
                xml_writer->set_attribute( "last_write_time", xml_of_time_t(directory_entry._file_info->last_write_time()) );

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

    if (_udp_port) {
        bool had_configuration_directory = _configuration_directory_name != "";

        if( Directory* configuration_directory = this->configuration_directory_or_null() )
        {
            if( _configuration_version == configuration_directory->version() )  configuration_directory->read_deep( 0.0 );
            changed = _configuration_version != configuration_directory->version();
        }
        else
            changed = had_configuration_directory;

        if( !changed )
        {
            int all_directory_version = 0;

            if( Directory* all_directory = _remote_configurations->configuration_directory_for_all_schedulers_or_null() )
                all_directory_version = all_directory->version();

            changed = _configuration_version_all != all_directory_version;
        }

        if( changed )  
        {
            signal_remote_scheduler();
            set_alarm_clock();
        }
    }

    if (changed && !_configuration_changed) {
        _configuration_changed = true;
        _configuration_changed_at = ::time(NULL);
        _udp_timeout_warned_at = 0;
    }

    return changed;
}

//--------------------------------------------------------Remote_scheduler::signal_remote_scheduler

void Remote_scheduler::signal_remote_scheduler()
{
    string command = "<check_folders/>";
    Host_and_port h(_host, _udp_port);
    Z_LOG2("scheduler", "Sending UDP command " << command << " to " << h << '\n' );
    check_timeout();
    send_udp_message(h, command);
}

//--------------------------------------------------------Remote_scheduler::check_timeout

void Remote_scheduler::check_timeout()
{
    if (_configuration_changed) {
        time_t now = ::time(NULL);
        if (now >= _configuration_changed_at + udp_warn_timeout && now >= _udp_timeout_warned_at + udp_timeout_warn_period) {
            time_t t = now - _configuration_changed_at;
            Msg_insertions mi;
            mi.append( string_of_time_t(_configuration_changed_at) );
            mi.append( (int)t );
            Xc x ("SCHEDULER-474",  mi );
            log()->warn(x.what());
            _error = x;
            _udp_timeout_warned_at = now;
        }
    }
}

//----------------------------------------------------------------Remote_scheduler::async_continue_

bool Remote_scheduler::async_continue_( Continue_flags )
{
    if (!is_yet_active()) {
        log()->info(message_string("SCHEDULER-458", ::time(NULL) - _active_since));
        _is_active = false;
    } else 
    if (_configuration_changed)
        signal_remote_scheduler();

    set_alarm_clock();
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

    result.setAttribute         ( "ip"              , _host.ip_string() );
    result.setAttribute_optional( "hostname"        , _host.name() );
    if (_udp_port) result.setAttribute("udp_port", _udp_port);
    result.setAttribute_optional( "scheduler_id"    , _scheduler_id );
    result.setAttribute         ( "version"         , _scheduler_version );
    result.setAttribute         ( "active"          , _is_active? "yes" : "no" );
    result.setAttribute         ( "connected"       , _is_connected? "yes" : "no" );
    if( _connected_at )
        result.setAttribute( "connected_at"    , xml_of_time_t(_connected_at) );
    if( _disconnected_at )
        result.setAttribute( "disconnected_at" , xml_of_time_t(_disconnected_at) );
    result.setAttribute_optional( "configuration_directory", _configuration_directory_name );

    if( _configuration_changed ) {
        result.setAttribute( "configuration_changed", "yes" );
        result.setAttribute( "configuration_changed_at", xml_of_time_t(_configuration_changed_at) );
    }
    if (_configuration_transfered_at)
        result.setAttribute( "configuration_transfered_at", xml_of_time_t(_configuration_transfered_at) );
    if (_deactivate_at)
        result.setAttribute("deactivate_at", xml_of_time_t(_deactivate_at));

    if (_error) append_error_element( result, _error );

    return result;
}

//-----------------------------------------------------------------------Remote_scheduler::obj_name

string Remote_scheduler::obj_name() const
{ 
    return S() << Scheduler_object::obj_name() << " " << _id;
}

//----------------------------------------------------------Remote_scheduler::connection_lost_event

// Für Clients vor v1.6
void Remote_scheduler::connection_lost_event( const exception* x )
{
    // x kann NULL sein
    
    Z_LOG2( "scheduler", Z_FUNCTION << " " << *this << "\n" ); /*! \change JS-481 */

    _disconnected_at = ::time(NULL);
    _active_since = 0;
    _is_connected = false;
    _connection_operation = NULL;  /*! \change JS-481 Zugefügtes Objektreferenz wieder auflösen */

    Z_LOG2("scheduler",Z_FUNCTION << ": " << *this << "\n" );
    if( _is_active )  _error = x;
    _is_active = false;
}

//-----------------------------------------------------Remote_configurations::Remote_configurations

Remote_configurations::Remote_configurations( Supervisor* supervisor, const File_path& directory_path )
:
    Abstract_scheduler_object( supervisor->spooler(), this, type_remote_configuration_observer ),
    _zero_(this+1),
    _supervisor(supervisor)
{
    _directory_observer = Z_NEW( Directory_observer( spooler(), directory_path, confdir_none ) );
    _directory_observer->register_directory_handler( this );
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
    if( _directory_observer )  _directory_observer->close();
}

//------------------------------------------------------------------Remote_configurations::activate

bool Remote_configurations::activate()
{
    if (_is_activated) 
        return true;
    else {
        _is_activated = true;
        log()->info(message_string("SCHEDULER-718", "remote", _directory_observer->directory_path()));
        return _directory_observer->activate();
    }
}

//-------------------------------------------------------Remote_configurations::on_handle_directory

bool Remote_configurations::on_handle_directory( Directory_observer* )
{
    bool something_changed = false;

    resolve_configuration_directory_names();
    
    if( Directory* all_directory = configuration_directory_for_all_schedulers_or_null() )
    {
        all_directory->read_deep( 0.0 );
    }

    Z_FOR_EACH( Remote_scheduler_register::Map, _supervisor->_remote_scheduler_register._map, it )
    {
        Remote_scheduler* remote_scheduler = it->second;
        if(remote_scheduler->is_yet_active())
            something_changed |= remote_scheduler->check_remote_configuration();
    }
   
    return something_changed;
}

//------------------------------------------------------------------Remote_configuration::set_alarm

void Remote_configurations::set_alarm()
{
    if( _directory_observer )  _directory_observer->set_alarm();
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
                        catch( exception& x ) { log()->warn( S() << x.what() << ", " << e->_file_info->path() ); }    // Ungültiger Verzeichnisname
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
