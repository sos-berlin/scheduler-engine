#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace directory_observer;

//---------------------------------------------------------------Folder_subsystem::Folder_subsystem

Folder_subsystem::Folder_subsystem( Scheduler* scheduler )
:
    file_based_subsystem<Folder>( scheduler, this, type_folder_subsystem ),
    javabridge::has_proxy<Folder_subsystem>(),
    _zero_(this+1),
    _configurations(confdir__max+1)
{
}

//--------------------------------------------------------------Folder_subsystem::~Folder_subsystem
    
Folder_subsystem::~Folder_subsystem()
{
    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------Folder_subsystem::close

void Folder_subsystem::close()
{
    Z_FOR_EACH( vector<Configuration>, _configurations, c )
    {
        if( c->_directory_observer  )  c->_directory_observer->close();
    }

    _subsystem_state = subsys_stopped;

    if( _root_folder )  
    {
        //typed_folder<>::_file_based_map hat keine ptr<>!  Zeiger k�nnen also ung�ltig sein:  _root_folder->remove_all_file_baseds();
        //typed_folder<>::_file_based_map hat keine ptr<>!  Zeiger k�nnen also ung�ltig sein:  remove_file_based( _root_folder );
        _root_folder = NULL;
    }
}

//-----------------------------------------------------Folder_subsystem::initialize_cache_directory

void Folder_subsystem::initialize_cache_directory()
{
    if( !_configurations[ confdir_cache ]._directory_observer )
    {
        _configurations[ confdir_cache ]._directory_observer = Z_NEW( Directory_observer( spooler(), _spooler->_configuration_directories[ confdir_cache ], confdir_cache ) );
      //_configurations[ confdir_cache ]._include_register   = Z_NEW( Include_register );
    }
}

//-----------------------------------------------------------Folder_subsystem::subsystem_initialize

bool Folder_subsystem::subsystem_initialize()
{
    _root_folder = Z_NEW( Folder( this, (Folder*)NULL ) );
    add_file_based( _root_folder );

    _subsystem_state = subsys_initialized;
    return true;
}

//-----------------------------------------------------------------Folder_subsystem::subsystem_load

bool Folder_subsystem::subsystem_load()
{
    _subsystem_state = subsys_loaded;

    if( !_spooler->_configuration_directories[ confdir_local ].exists() )
    {
        log()->warn( message_string( "SCHEDULER-895", _spooler->_configuration_directories[ confdir_local ] ) );
    }
    else
    {
        _configurations[ confdir_local ]._directory_observer = Z_NEW( Directory_observer( spooler(), _spooler->_configuration_directories[ confdir_local ], confdir_local ) );
        _configurations[ confdir_local ]._directory_observer->register_directory_handler( this );
      //_configurations[ confdir_local ]._include_register   = Z_NEW( Include_register );
    }

    _root_folder->load();
    handle_folders();

    return true;
}

//-------------------------------------------------------------Folder_subsystem::subsystem_activate

bool Folder_subsystem::subsystem_activate()
{
    bool result = false;

    if( Directory_observer* d = _configurations[ confdir_cache ]._directory_observer )     // Die zentrale Konfiguration (im Cache) zuerst, sie hat Vorrang
    {
        log()->info( message_string("SCHEDULER-718", "cache", d->directory_path() ) );
        //_cache_directory_observer->activate();
        result = true;
    }

    if( Directory_observer* d = _configurations[ confdir_local ]._directory_observer )
    {
        log()->info( message_string("SCHEDULER-718", "local", d->directory_path() ) );
        _configurations[ confdir_local ]._directory_observer->activate();
        result = true;
    }

    if( result )
    {
        _subsystem_state = subsys_active;
        file_based_subsystem<Folder>::subsystem_activate();     // Tut bislang nichts, au�er f�r jeden Ordner eine Meldung "SCHEDULER-893 Folder is 'active' now" auszugeben
        handle_folders();
    }

    return result;
}

//---------------------------------------------Folder_subsystem::merged_cache_and_local_directories

ptr<Directory> Folder_subsystem::merged_cache_and_local_directories()
{
    ptr<Directory> result = _configurations[ confdir_cache ]._directory_observer->directory_tree()->root_directory()->clone();
    result->merge_new_entries( _configurations[ confdir_local ]._directory_observer->directory_tree()->root_directory() );

    return result;
}

//-----------------------------------------------------------------Folder_subsystem::new_file_based

ptr<Folder> Folder_subsystem::new_file_based()
{
    assert(0);
    zschimmer::throw_xc( Z_FUNCTION );    // Subfolder_folder::on_base_file_changed() legt selbst Folder an
}

//------------------------------------------------------------Folder_subsystem::on_handle_directory

bool Folder_subsystem::on_handle_directory( directory_observer::Directory_observer* )
{
    //Z_LOGI2( "zschimmer", Z_FUNCTION << " check configuration directory " << _local_directory_observer->directory_tree()->directory_path() << "\n" );

    handle_folders();
    
    return true;
}

//-------------------------------------------------------------Folder_subsystem::is_valid_extension

bool Folder_subsystem::is_valid_extension( const string& extension )
{
    assert( _root_folder );
    return _root_folder->is_valid_extension( extension );
}

//--------------------------------------------------------------------Folder_subsystem::execute_xml

xml::Element_ptr Folder_subsystem::execute_xml( const xml::Element_ptr& element )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "configuration.fetch_updated_files" ) )  
    {
        
    }
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName(), Z_FUNCTION );

    return result;
}

//-----------------------------------------------------------------Folder_subsystem::handle_folders

bool Folder_subsystem::handle_folders( double minimum_age )
{
    bool something_changed = false;

    if( subsystem_state() == subsys_loaded  ||
        subsystem_state() == subsys_active )
    {
        double now = double_from_gmtime();

        if( _last_change_at + minimum_age <= now )
        {
            ptr<Directory> directory;
            
            if( _configurations[ confdir_local ]._directory_observer )
            {
                directory = _configurations[ confdir_local ]._directory_observer->directory_tree()->root_directory();

                Directory::Read_flags read_flags = subsystem_state() == subsys_active? Directory::read_subdirectories
                                                                                     : Directory::read_subdirectories_suppress_aging;   // Beim ersten Mal neue Dateien nicht altern lassen, sondern sofort lesen
                directory->read( read_flags, 0.0 );     
            }

            if( _configurations[ confdir_cache ]._directory_observer )
            {
                Directory* cache_dir = _configurations[ confdir_cache ]._directory_observer->directory_tree()->root_directory();

                // Die Dateien aus dem Cache lassen wir nicht altern.
                // a) �berfl�ssig, weil der Scheduler selbst die Dateien erzeugt hat, sie werden nicht gleichzeitig geschrieben und gelesen
                // b) damit beim Start des Scheduler die vorrangigen cache-Dateien sofort gelesen werden (sonst w�rde in den ersten zwei Sekunden nur das live-Verzeichnis gelten)

                cache_dir->read_deep( 0.0 );     // Ohne Alterung, weil Verzeichnis nicht �berwacht wird (!is_watched() weil kein activate())
                if( _configurations[ confdir_local ]._directory_observer )  directory = merged_cache_and_local_directories();
                                                                      else  directory = cache_dir;
            }

            if( directory )
            {
                something_changed = _root_folder->adjust_with_directory( directory );
            }
            
            if( something_changed )  _last_change_at = now;
        }
    }

    return something_changed;
}

//-------------------------------------------------------------------Folder_subsystem::set_signaled

void Folder_subsystem::set_signaled( const string& text )
{ 
    // Besser: nur den Verzeichnisbaum signalisieren. Die Verzeichnisse m�ssen nicht neu gelesen werden.
    if( _configurations[ confdir_local ]._directory_observer )  _configurations[ confdir_local ]._directory_observer->set_signaled( text );
    if( _configurations[ confdir_cache ]._directory_observer )  _configurations[ confdir_cache ]._directory_observer->set_signaled( text );
}

//------------------------------------------------------------------Folder_subsystem::configuration

Configuration* Folder_subsystem::configuration( Configuration_origin which )
{
    Configuration* result = &_configurations[ which ];
    if( !result )  z::throw_xc( Z_FUNCTION, (int)which );
    return result;
}

//---------------------------------------------------Folder_subsystem::write_configuration_file_xml

void Folder_subsystem::write_configuration_file_xml( const Absolute_path& folder_path, const xml::Element_ptr& element )
{
    if( !element )  z::throw_xc( Z_FUNCTION, "no element" );

    File_path configuration_directory = _spooler->_configuration_directories[ confdir_local ];
    File_path directory ( configuration_directory, folder_path.without_slash() );
    
    vector<string> directories = vector_split( "/", folder_path.without_slash() );
    File_path      d           = _spooler->_configuration_directories[ confdir_local ];

    for( int i = 0; i < directories.size(); i++ )  
    {
        if( directories[i] != "" )
        {
            d = File_path( d, directories[ i ] );

            if( !d.exists() )
            {
                log()->info( message_string( "SCHEDULER-707", d ) );

                int err = call_mkdir( d.c_str(), 0777 );
                if( err  &&  errno != EEXIST )  z::throw_errno( errno, "mkdir", d.c_str() );
            }
        }
    }


    string element_name = element.nodeName();

    Z_FOR_EACH( Spooler::File_based_subsystems, _spooler->_file_based_subsystems, s )
    {
        File_based_subsystem* subsystem = *s;

        if( subsystem != this  &&    // Folder_subsystem kennt xml_element_name() nicht
            subsystem->xml_element_name() == element_name )
        {
            // Attribute sollen gel�scht werden, also Element klonen
            // (Mit gel�schten Attributen kann die Datei umbenannt werden, ohne dass die Attribute angepasst werden m�ssten.)
            xml::Document_ptr clone;
            clone.create();
            clone.appendChild( clone.clone( element ) );

            string name = subsystem->name_from_xml_attributes( clone.documentElement(), remove_attributes );

            File file ( File_path( directory, name + subsystem->filename_extension() ), "w" );
            string indent_string = "    ";
            string xml_string = clone.xml( xml::default_character_encoding, indent_string );
            if( !string_ends_with( xml_string, "\n" ) )  xml_string += '\n';
            file.print( xml_string );
            file.close();
            break;
        }
    }

    handle_folders();
}

}}} //namespace sos::scheduler::folder
