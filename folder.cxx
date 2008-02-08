// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace zschimmer::file;
using namespace directory_observer;

//--------------------------------------------------------------------------------------------const

const char                      folder_separator            = '/';
//---------------------------------------------------------------Folder_subsystem::Folder_subsystem

Folder_subsystem::Folder_subsystem( Scheduler* scheduler )
:
    file_based_subsystem<Folder>( scheduler, this, type_folder_subsystem ),
    _zero_(this+1)
  //_directory_watch_interval( directory_watch_interval_max )
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
    if( _live_directory_observer  )  _live_directory_observer ->close();
    if( _cache_directory_observer )  _cache_directory_observer->close();
    _subsystem_state = subsys_stopped;

    if( _root_folder )  
    {
        //typed_folder<>::_file_based_map hat keine ptr<>!  Zeiger können also ungültig sein:  _root_folder->remove_all_file_baseds();
        //typed_folder<>::_file_based_map hat keine ptr<>!  Zeiger können also ungültig sein:  remove_file_based( _root_folder );
        _root_folder = NULL;
    }
}

//------------------------------------------------------------------Folder_subsystem::set_directory

void Folder_subsystem::set_directory( const File_path& directory )
{
    assert_subsystem_state( subsys_initialized, Z_FUNCTION );
    assert( !_live_directory_observer );

    _live_directory_observer = Z_NEW( Directory_observer( spooler(), directory ) );
    _live_directory_observer->register_directory_handler( this );

    //handle_folders();
}

//------------------------------------------------------------Folder_subsystem::set_cache_directory

void Folder_subsystem::set_cache_directory( const File_path& directory )
{
    assert_subsystem_state( subsys_initialized, Z_FUNCTION );
    assert( !_cache_directory_observer );

    _cache_directory_observer = Z_NEW( Directory_observer( spooler(), directory ) );
    _cache_directory_observer->directory_tree()->set_is_cache( true );
    _cache_directory_observer->register_directory_handler( this );

    //handle_folders();
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

    if( _live_directory_observer   &&  _live_directory_observer ->directory_tree()->directory_path().exists()  ||
        _cache_directory_observer  &&  _cache_directory_observer->directory_tree()->directory_path().exists() )
    {
        _root_folder->load();
        handle_folders();
    }
    else
        log()->warn( message_string( "SCHEDULER-895", _live_directory_observer->directory_tree()->directory_path() ) );

    return true;
}

//-------------------------------------------------------------Folder_subsystem::subsystem_activate

bool Folder_subsystem::subsystem_activate()
{
    bool result = false;

    if( _live_directory_observer->directory_tree()->directory_path().exists() )
    {
        _live_directory_observer->activate();
        _live_directory_observer->run_handler();

        _subsystem_state = subsys_active;

        file_based_subsystem<Folder>::subsystem_activate();     // Tut bislang nichts, außer für jeden Ordner eine Meldung "SCHEDULER-893 Folder is 'active' now" auszugeben

        result = true;
    }

    if( _cache_directory_observer )
    {
        _cache_directory_observer->activate();
        _cache_directory_observer->run_handler();
    }

    return result;
}

//----------------------------------------------Folder_subsystem::merged_cache_and_live_directories

ptr<Directory> Folder_subsystem::merged_cache_and_live_directories()
{
    ptr<Directory> result = _cache_directory_observer->directory_tree()->root_directory()->clone();
    result->merge_new_entries( _live_directory_observer->directory_tree()->root_directory() );

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
    //Z_LOGI2( "joacim", Z_FUNCTION << " Prüfe Konfigurationsverzeichnis " << _live_directory_observer->directory_tree()->directory_path() << "\n" );

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
            
            if( _live_directory_observer )
            {
                directory = _live_directory_observer->directory_tree()->root_directory();
                directory->read_deep( 0.0 );
            }

            if( _cache_directory_observer )
            {
                Directory* cache_dir = _cache_directory_observer->directory_tree()->root_directory();

                // Die Dateien aus dem Cache lassen wir nicht altern.
                // a) überflüssig, weil der Scheduler selbst die Dateien erzeugt hat, sie werden nicht gleichzeitig geschrieben und gelesen
                // b) damit beim Start des Scheduler die vorrangigen cache-Dateien sofort gelesen werden (sonst würde in den ersten zwei Sekunden nur das live-Verzeichnis gelten)

                cache_dir->read( Directory::read_subdirectories_suppress_aging, 0.0 );     
                if( _live_directory_observer ) directory = merged_cache_and_live_directories();
                                          else directory = cache_dir;
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
    // Besser: nur den Verzeichnisbaum signalisieren, der auch betroffen ist
    if( _live_directory_observer  )  _live_directory_observer ->set_signaled( text );
    if( _cache_directory_observer )  _cache_directory_observer->set_signaled( text );
}

//-----------------------------------------------------------------------------------Folder::Folder

Folder::Folder( Folder_subsystem* folder_subsystem, Folder* parent )
:
    file_based< Folder, Subfolder_folder, Folder_subsystem >( folder_subsystem, this, type_folder ),
    _parent(parent),
    _zero_(this+1)
{
    if( !_parent )
    _scheduler_script_folder = spooler()->scheduler_script_subsystem()->new_scheduler_script_folder( this );

    _process_class_folder    = spooler()->process_class_subsystem   ()->new_process_class_folder ( this );
    _lock_folder             = spooler()->lock_subsystem            ()->new_lock_folder          ( this );
    _job_folder              = spooler()->job_subsystem             ()->new_job_folder           ( this );
    _job_chain_folder        = spooler()->order_subsystem           ()->new_job_chain_folder     ( this );
    _standing_order_folder   = spooler()->standing_order_subsystem  ()->new_standing_order_folder( this );
    _subfolder_folder        = spooler()->folder_subsystem          ()->new_subfolder_folder     ( this );

  //add_to_typed_folder_map( _scheduler_script_folder );
    add_to_typed_folder_map( _process_class_folder    );
    add_to_typed_folder_map( _lock_folder             );
    add_to_typed_folder_map( _job_folder              );
    add_to_typed_folder_map( _job_chain_folder        );
    add_to_typed_folder_map( _standing_order_folder   );
    add_to_typed_folder_map( _subfolder_folder        );

    _log->set_prefix( obj_name() );     // Noch ohne Pfad
}

//--------------------------------------------------------------------------add_to_typed_folder_map
    
void Folder::add_to_typed_folder_map( Typed_folder* typed_folder )
{
    if( typed_folder )
    {
        _typed_folder_map[ typed_folder->subsystem()->filename_extension() ] = typed_folder;
    }
}

//----------------------------------------------------------------------------------Folder::~Folder
    
Folder::~Folder()
{
    _typed_folder_map.clear();
}

//------------------------------------------------------------------------------------Folder::close

void Folder::close()
{
}

//-----------------------------------------------------------------------Folder::is_valid_extension

bool Folder::is_valid_extension( const string& extension )
{
    Typed_folder_map::iterator it = _typed_folder_map.find( extension );
    return it != _typed_folder_map.end();
}

//------------------------------------------------------------------Folder::scheduler_script_folder

Scheduler_script_folder* Folder::scheduler_script_folder()
{
    if( !_scheduler_script_folder )  z::throw_xc( Z_FUNCTION );
    return _scheduler_script_folder;
}

//---------------------------------------------------------------------------------Folder::set_name

void Folder::set_name( const string& name )
{
    My_file_based::set_name( name );

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
    {
        Typed_folder* typed_folder = it->second;
        typed_folder->log()->set_prefix( typed_folder->obj_name() );        // Jetzt mit Pfad
    }
}

//----------------------------------------------------------------------------------Folder::is_root

bool Folder::is_root() const
{
    return this == _spooler->folder_subsystem()->root_folder();
}

//----------------------------------------------------------------------------Folder::on_initialize

bool Folder::on_initialize()
{
    return true;
}

//----------------------------------------------------------------------------------Folder::on_load

bool Folder::on_load()
{
    return true;
}

//------------------------------------------------------------------------------Folder::on_activate

bool Folder::on_activate()
{
    return true;
}

//--------------------------------------------------------------Folder::position_of_extension_point

int Folder::position_of_extension_point( const string& filename )
{
    int e = filename.length() - 1;
    while( e >= 0  &&  filename[ e ]  != '.' )  e--;        assert( e < 0  ||  filename[ e ] == '.' );
    if( e >= 0 )  e--;                                       
    while( e >= 0  &&  filename[ e ]  != '.' )  e--;        assert( e < 0  ||  filename[ e ] == '.' );

    if( e < 0 )  e = filename.length();     // Keine Dateinamenserweiterung

    return e;
}

//------------------------------------------------------------------Folder::object_name_of_filename

string Folder::object_name_of_filename( const string& filename )
{
    return filename.substr( 0, position_of_extension_point( filename ) );
}

//--------------------------------------------------------------------Folder::extension_of_filename

string Folder::extension_of_filename( const string& filename )
{
    string result = filename.substr( position_of_extension_point( filename ) );      // Zum Bespiel ".job.xml"
    Z_WINDOWS_ONLY( result = lcase( result ) );
    return result;
}

//--------------------------------------------------------------------------------Folder::directory

file::File_path Folder::directory() const
{ 
    //return File_path( spooler()->folder_subsystem()->directory(), path().without_slash() ); 
    return base_file_info()._path;
}

//--------------------------------------------------------------------------------Folder::make_path

Absolute_path Folder::make_path( const string& name )
{
    return Absolute_path( path(), name );
}

//--------------------------------------------------------------------Folder::adjust_with_directory

bool Folder::adjust_with_directory( Directory* directory )
{
    assert( directory );  if( !directory )  z::throw_xc( Z_FUNCTION, "directory==NULL" );

    typedef stdext::hash_map< Typed_folder*, list< const Directory_entry* > >   File_list_map;
    
    File_list_map file_list_map;
    bool          something_changed = false;

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )  file_list_map[ it->second ] = list< const Directory_entry* >();

    try
    {
        // DATEINAMEN EINSAMMELN

        if( !base_file_is_removed() )
        {
            string last_normalized_name;
            string last_extension;
            string last_filename;

            Z_FOR_EACH( Directory::Entry_list, directory->_ordered_list, directory_iterator )
            {
                Directory_entry* directory_entry      = &*directory_iterator;
                string           filename             = directory_entry->_file_info->path().name();
                string           name;
                string           extension            = extension_of_filename( filename );
                string           normalized_extension = lcase( extension );
                Typed_folder*    typed_folder         = NULL;

                if( directory_entry->_file_info->is_directory() )
                {
                    name         = filename;
                    typed_folder = _subfolder_folder;
                }
                else
                {
                    name = object_name_of_filename( filename );
                    
                    if( name != "" )
                    {
                        Typed_folder_map::iterator it = _typed_folder_map.find( extension );
                        if( it != _typed_folder_map.end() )  typed_folder = it->second;
                    }
                }

                if( typed_folder )
                {
                    directory_entry->_normalized_name = typed_folder->subsystem()->normalized_name( name );

                    if( directory_entry->_normalized_name == last_normalized_name  &&
                        extension                         == last_extension           )
                    {
                        zschimmer::Xc x ( "SCHEDULER-889", last_filename, filename );

                        log()->warn( x.what() );

                        // Liefert minütlich eine eMail:
                        //if( _spooler->_mail_on_error )
                        //{
                        //    Scheduler_event scheduler_event ( scheduler::evt_base_file_error, log_error, spooler() );
                        //    scheduler_event.set_error( x );

                        //    Mail_defaults mail_defaults( spooler() );
                        //    mail_defaults.set( "subject", x.what() );
                        //    mail_defaults.set( "body"   , x.what() );

                        //    scheduler_event.send_mail( mail_defaults );
                        //}
                    }
                    else
                    {
                        file_list_map[ typed_folder ].push_back( directory_entry );
                    }

                    last_normalized_name = directory_entry->_normalized_name;
                    last_extension       = normalized_extension;
                    last_filename        = filename;
                }
            }
        }

        Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
        {
            Typed_folder* typed_folder = it->second;

            something_changed |= typed_folder->adjust_with_directory( file_list_map[ typed_folder ] );
        }
    }
    catch( exception& x ) 
    {
        log()->error( message_string( "SCHEDULER-431", x ) );
        //? Fehler merken für <show_state>, oder was machen wir mit dem Fehler? Später wiederholen
    }


    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )  it->second->handle_replace_or_remove_candidates();

    return something_changed;
}

//------------------------------------------------------------------------------Folder::dom_element

xml::Element_ptr Folder::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "folder" );
    fill_file_based_dom_element( result, show_what );

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
    {
        Typed_folder* typed_folder = it->second;

        if( !typed_folder->is_empty() )
        {
            if( !show_what.is_set( show_jobs )  &&  typed_folder->subsystem() == spooler()->job_subsystem() )
                result.append_new_comment( "<jobs> suppressed. Use what=\"jobs\"." );
            else
            if( show_what.is_set( show_no_subfolders )  &&  typed_folder->subsystem() == spooler()->folder_subsystem() )
            {
                // nix
            }
            else
                result.appendChild( typed_folder->dom_element( dom_document, show_what ) );
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Folder::obj_name

string Folder::obj_name() const
{
    S result;
    result << Scheduler_object::obj_name();
    result << " " << path();
    return result;
}

//---------------------------------------------------------------Subfolder_folder::Subfolder_folder

Subfolder_folder::Subfolder_folder( Folder* folder )
:
    typed_folder<Folder>( folder->spooler()->folder_subsystem(), folder, type_folder_folder )
{
}

//--------------------------------------------------------------Subfolder_folder::~Subfolder_folder
    
Subfolder_folder::~Subfolder_folder()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//-----------------------------------------------------------Subfolder_folder::on_base_file_changed

bool Subfolder_folder::on_base_file_changed( File_based* file_based, const Directory_entry* directory_entry )
{
    bool    something_changed = false;
    Folder* subfolder         = static_cast<Folder*>( file_based );

    if( !subfolder )
    {
        ptr<Folder> new_subfolder = Z_NEW( Folder( subsystem(), folder() ) );
        new_subfolder->set_folder_path( folder()->path() );
        new_subfolder->set_name( directory_entry->_normalized_name );
        new_subfolder->fix_name();
        new_subfolder->set_base_file_info( Base_file_info( *directory_entry ) ); 
        add_file_based( new_subfolder );                    
        something_changed = true;

        new_subfolder->activate();
        if( new_subfolder->file_based_state() >= File_based::s_loaded )  new_subfolder->adjust_with_directory( directory_entry->_subdirectory );
    }
    else
    {
        subfolder->_file_is_removed = directory_entry == NULL;
        subfolder->_remove_xc       = zschimmer::Xc();

        if( directory_entry )
        {
            subfolder->set_base_file_info( Base_file_info( *directory_entry ) );
            subfolder->set_to_be_removed( false ); 
            something_changed = subfolder->adjust_with_directory( directory_entry->_subdirectory );    
        }
        else
        if( !subfolder->is_to_be_removed() ) 
        {
            string p = folder()->make_path( subfolder->base_file_info()._filename );
            subfolder->log()->info( message_string( "SCHEDULER-898" ) );
            subfolder->adjust_with_directory( directory_entry->_subdirectory );
            subfolder->remove();

            something_changed = true;
        }
        else
        {
            // Verzeichnis ist gelöscht, aber es leben vielleicht noch Objekte, die gelöscht werden müssen.
            // adjust_with_directory() wird diese mit handle_replace_or_remove_candidates() löschen
            something_changed = subfolder->adjust_with_directory( directory_entry->_subdirectory );    
        }
    }

    return something_changed;
}

//-------------------------------------------------------------------Folder::remove_all_file_baseds

//void Folder::remove_all_file_baseds()
//{
//    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
//    {
//        Typed_folder* typed_folder = it->second;
//        typed_folder->remove_all_file_baseds();
//    }
//}

//------------------------------------------------------------------------Folder::prepare_to_remove

void Folder::prepare_to_remove()
{
    //Das wird besser von adjust_with_directory() erledigt: remove_all_file_baseds();
    My_file_based::prepare_to_remove();
}

//-----------------------------------------------------------------------Folder::can_be_removed_now

bool Folder::can_be_removed_now()
{
    bool result = true;

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
    {
        Typed_folder* typed_folder = it->second;

        result = typed_folder->is_empty();
        if( !result )  break;
    }

    return result;
}

//-----------------------------------------------------------------------Typed_folder::Typed_folder

Typed_folder::Typed_folder( Folder* folder, Type_code type_code )
: 
    Scheduler_object( folder->_spooler, this, type_code ),
    _zero_(this+1),
    _folder(folder)
{
    log()->set_prefix( obj_name() );     // Noch ohne Pfad
}

//--------------------------------------------------------------Typed_folder::adjust_with_directory
    
bool Typed_folder::adjust_with_directory( const list<const Directory_entry*>& directory_entries )
{
    bool                           something_changed  = false;
    vector<const Directory_entry*> ordered_directory_entries;     // Geordnete Liste der Dateinamen
    vector<File_based*>            ordered_file_baseds;           // Geordnete Liste der bereits bekannten (geladenen) Dateien

    ordered_directory_entries.reserve( _file_based_map.size() );
    Z_FOR_EACH_CONST( list<const Directory_entry*>, directory_entries, de )  ordered_directory_entries.push_back( *de );
    sort( ordered_directory_entries.begin(), ordered_directory_entries.end(), Directory_entry::normalized_less_dereferenced );

    ordered_file_baseds.reserve( _file_based_map.size() );
    Z_FOR_EACH( File_based_map, _file_based_map, fb )  ordered_file_baseds.push_back( &*fb->second );
    sort( ordered_file_baseds.begin(), ordered_file_baseds.end(), File_based::less_dereferenced );


    vector<const Directory_entry*>::const_iterator de = ordered_directory_entries.begin();
    vector<File_based*           >::const_iterator fb = ordered_file_baseds.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen

    while( de != ordered_directory_entries.end()  ||
           fb != ordered_file_baseds.end() )
    {
        /// Dateinamen gleich Objektnamen?

        while( fb != ordered_file_baseds.end()  &&
               de != ordered_directory_entries.end()  &&
               (*fb)->_base_file_info._normalized_name == (*de)->_normalized_name )
        {
            something_changed |= on_base_file_changed( *fb, *de );
            de++, fb++;
        }



        /// Dateien hinzugefügt?

        while( de != ordered_directory_entries.end()  &&
               ( fb == ordered_file_baseds.end()  ||  (*de)->_normalized_name < (*fb)->_base_file_info._normalized_name ) )
        {
            something_changed |= on_base_file_changed( (File_based*)NULL, *de );
            de++;
        }

        assert( de == ordered_directory_entries.end()  || 
                fb == ordered_file_baseds.end() ||
                (*de)->_normalized_name >= (*fb)->_base_file_info._normalized_name );
        


        /// Dateien gelöscht?

        while( fb != ordered_file_baseds.end()  &&
               ( de == ordered_directory_entries.end()  ||  (*de)->_normalized_name > (*fb)->_base_file_info._normalized_name ) )  // Datei entfernt?
        {
            something_changed |= on_base_file_changed( *fb, NULL );
            fb++;
        }

        assert( fb == ordered_file_baseds.end()  ||
                de == ordered_directory_entries.end()  ||
                (*de)->_normalized_name <= (*fb)->_base_file_info._normalized_name );
    }

    return something_changed;
}

//---------------------------------------------------------------Typed_folder::on_base_file_changed

bool Typed_folder::on_base_file_changed( File_based* old_file_based, const Directory_entry* directory_entry )
{
#   ifdef Z_DEBUG
        if( zschimmer::Log_ptr log = "joacim" )
        {
            log << Z_FUNCTION << "( ";
            if( old_file_based )  log << old_file_based->obj_name() << " " << Time().set_utc( old_file_based->_base_file_info._last_write_time ).as_string()
                                      << ( old_file_based->_file_is_removed? " file_is_removed" : "" );
                            else  log << "new";
            log << ", ";
            if( directory_entry )  log << Time().set_utc( directory_entry->_file_info->last_write_time() ).as_string() << " " << directory_entry->_file_info->path();
                             else  log << "removed file";
            log << " )\n";
        }
#   endif

    //const Base_file_info* base_file_info = directory_entry->_file_info;
    bool            something_changed  = false;
    ptr<File_based> file_based         = NULL;
    File_based*     current_file_based = old_file_based;        // File_based der zuletzt gelesenen Datei
    bool            is_new             = !old_file_based  ||    
                                         old_file_based->_file_is_removed;     // Datei ist wieder aufgetaucht?
    File_path       file_path;

    if( old_file_based )  
    {
        if( !old_file_based->_file_is_removed )  old_file_based->_remove_xc = zschimmer::Xc();      // Datei ist wieder da
        old_file_based->_file_is_removed = directory_entry == NULL;
        if( old_file_based->replacement() )  current_file_based = old_file_based->replacement();    // File_based der zuletzt geladenen Datei
    }


    try
    {
        if( directory_entry )
        {
            if( !directory_entry->is_aging() )
            {
                string name              = Folder::object_name_of_filename( directory_entry->_file_info->path().name() );
                bool   timestamp_changed = is_new  ||                 // Dieselbe Datei ist wieder aufgetaucht
                                           current_file_based  &&
                                           current_file_based->_base_file_info._last_write_time != directory_entry->_file_info->last_write_time(); //  ||
                                             //current_file_based->name() != name );      // Objekt ist unter anderer Großschreibung bekannt?

                if( !is_new  &&  !timestamp_changed )
                {
                    ignore_duplicate_configuration_file( current_file_based, (File_based*)NULL, *directory_entry );
                }
                else
                {
                    string content;
                  //Md5    content_md5;
                    z::Xc  content_xc;
                    
                    try
                    {
                        content = string_from_file( directory_entry->_file_info->path() );
                        //content_md5 = md5( content );
                    }
                    catch( exception& x ) { content_xc = x; }

                    if( content_xc.code() == ( S() << "ERRNO-" << ENOENT ).to_string() )    // ERRNO-2 (Datei gelöscht)?
                    {
                        if( old_file_based )  
                        {
                            old_file_based->_file_is_removed = true;
                            old_file_based->remove();
                            old_file_based = NULL;
                        }
                    }
                    else
                    {   
                        something_changed = true;

                        file_based = subsystem()->call_new_file_based();
                        file_based->set_file_based_state( File_based::s_undefined );    // Erst set_dom() definiert das Objekt
                      //file_based->_md5        = content_md5;
                        file_based->set_base_file_info( Base_file_info( *directory_entry ) );
                        file_based->set_folder_path( folder()->path() );
                        file_based->set_name( name );
                        file_based->fix_name();
                        file_based->_is_from_cache = directory_entry->_is_from_cache;

                        ignore_duplicate_configuration_file( current_file_based, file_based, *directory_entry );
                        
                        string rel_path = folder()->make_path( name );
                        Time   t; 
                        t.set_utc( directory_entry->_file_info->last_write_time() );

                        if( old_file_based ) 
                        {
                            old_file_based->log()->info( message_string( "SCHEDULER-892", rel_path, t.as_string(), subsystem()->object_type_name() ) );
                            old_file_based->handle_event( File_based::bfevt_modified ); 
                            old_file_based->set_replacement( file_based );
                            current_file_based = NULL;
                        }
                        else
                        {
                            file_based->log()->info( message_string( "SCHEDULER-891", rel_path, t.as_string(), subsystem()->object_type_name() ) );
                            file_based->handle_event( File_based::bfevt_added );

                            add_file_based( file_based );
                        }


                        if( !content_xc.is_empty() )  throw content_xc;


                        xml::Document_ptr dom_document ( content );
                        xml::Element_ptr  element      = dom_document.documentElement();
                        subsystem()->assert_xml_element_name( element );
                        if( spooler()->_validate_xml )  spooler()->_schema.validate( dom_document );

                        assert_empty_attribute( element, "spooler_id" );
                        if( !element.bool_getAttribute( "replace", true ) )  z::throw_xc( "SCHEDULER-232", element.nodeName(), "replace", element.getAttribute( "replace" ) );

                        Z_LOG2( "scheduler", file_path << ":\n" << content << "\n" );

                        file_based->set_dom( element );
                        file_based->set_file_based_state( File_based::s_not_initialized );

                        file_based->initialize();


                        if( file_based->file_based_state() == File_based::s_initialized )
                        {
                            if( !old_file_based )           // Neues Objekt?
                            {
                                file_based->activate();     
                            }
                            else
                            {
                                old_file_based->prepare_to_replace();

                                if( old_file_based->can_be_replaced_now() ) 
                                {
                                    file_based = old_file_based->replace_now();     assert( !file_based->replacement() );
                                }
                            }
                        }
                    }
                }
            }
        }
        else                                        // Datei ist gelöscht
        if( old_file_based->has_base_file()  &&     // Nicht dateibasiertes Objekt, also aus anderer Quelle, nicht löschen
            !old_file_based->is_to_be_removed() )
        {
            something_changed = true;

            string p = folder()->make_path( old_file_based->base_file_info()._filename );
            old_file_based->log()->info( message_string( "SCHEDULER-890", p, subsystem()->object_type_name() ) );

            file_based = old_file_based;                // Für catch()
            assert( file_based->_file_is_removed );
            
            file_based->handle_event( File_based::bfevt_removed );
            file_based->remove();
            file_based = NULL;
        }
    }
    catch( exception& x )
    {
        if( !file_based )  throw;   // Sollte nicht passieren

        string msg;


        if( directory_entry )        // Fehler beim Löschen soll das Objekt nicht als fehlerhaft markieren
        {
            file_based->_base_file_xc      = x;
            file_based->_base_file_xc_time = double_from_gmtime();

            Time t;
            t.set_utc( directory_entry->_file_info->last_write_time() );

            msg = message_string( "SCHEDULER-428", directory_entry->_file_info->path(), t.as_string(), x );
        }
        else
        {
            msg = message_string( "SCHEDULER-439", file_based->base_file_info()._path, 
                                                   file_based->subsystem()->object_type_name(), x );
        }

        file_based->log()->error( msg );

        if( msg != ""  &&  _spooler->_mail_on_error )
        {
            Scheduler_event scheduler_event ( scheduler::evt_base_file_error, log_error, spooler() );
            scheduler_event.set_error( x );

            Mail_defaults mail_defaults( spooler() );
            mail_defaults.set( "subject", msg );
            mail_defaults.set( "body"   , msg );

            scheduler_event.send_mail( mail_defaults );
        }
    }
    return something_changed;
}

//------------------------------------------------Typed_folder::ignore_duplicate_configuration_file

void Typed_folder::ignore_duplicate_configuration_file( File_based* current_file_based, File_based* new_file_based, const Directory_entry& directory_entry )
{
    //if( new_file_based  &&  current_file_based )  new_file_based->_duplicate_version = current_file_based->_duplicate_version;

    File_based* file_based = new_file_based? new_file_based : current_file_based;

    if( directory_entry._duplicate_version  &&  
        directory_entry._duplicate_version != file_based->_duplicate_version  &&
        !directory_entry.is_aging() )
    {
        if( !new_file_based )
        {
            // Lokale Datei geändert, Objekt ist aber zentral definiert: 
            file_based->log()->warn( message_string( "SCHEDULER-460", subsystem()->object_type_name() ) );  // Geänderte lokale Datei wird ignoriert
        }
        else
        //if( current_file_based  &&  !current_file_based->_is_from_cache )
        //{
        //    file_based->log()->warn( message_string( "SCHEDULER-703" ) );   // Bereits gelesene lokale Datei wird durch zentrale ersetzt
        //}
        //else
        {
            file_based->log()->warn( message_string( "SCHEDULER-703" ) );   // Lokale Datei wird ignoriert und zentrale Version wird genommen
        }
    }

    if( current_file_based )  current_file_based->_duplicate_version = directory_entry._duplicate_version;
    if( new_file_based     )  new_file_based    ->_duplicate_version = directory_entry._duplicate_version;
}

//-----------------------------------------------------Typed_folder::new_initialized_file_based_xml

ptr<File_based> Typed_folder::new_initialized_file_based_xml( const xml::Element_ptr& element, const string& default_name )
{
    subsystem()->check_file_based_element( element );
    //assert_empty_attribute( element, "replace"    );

    ptr<File_based> file_based = subsystem()->call_new_file_based();
    file_based->set_file_based_state( File_based::s_undefined );    // Erst set_dom() definiert das Objekt
    file_based->set_folder_path( folder()->path() );
    file_based->set_name( element.getAttribute( "name", default_name ) );
    file_based->set_dom( element );
    file_based->set_file_based_state( File_based::s_not_initialized );
    file_based->initialize();

    return file_based;
}

//-----------------------------------------------------------------Typed_folder::add_file_based_xml

void Typed_folder::add_file_based_xml( const xml::Element_ptr& element, const string& default_name )
{
    ptr<File_based> file_based = new_initialized_file_based_xml( element, default_name );
    add_file_based( file_based );
    file_based->activate();
}

//------------------------------------------------------Typed_folder::add_or_replace_file_based_xml

void Typed_folder::add_or_replace_file_based_xml( const xml::Element_ptr& element, const string& default_name )
{
    subsystem()->check_file_based_element( element );

    if( ptr<File_based> file_based = file_based_or_null( element.getAttribute( "name", default_name ) ) )
    {
        bool replace_yes        =  element.bool_getAttribute( "replace", false );                   // replace="yes"
        bool replace_no         = !element.bool_getAttribute( "replace", true  );                   // replace="no"
        bool use_base_mechanism = file_based_subsystem()->subsystem_state() <= subsys_initialized;  // Wird noch die Scheduler-Konfigurationsdatei geladen?

        //if( replace_no )  z::throw_xc( "SCHEDULER-441", obj_name() );   // replace="no" und Objekt ist bekannt

        if( replace_no  ||  
            use_base_mechanism  &&  !replace_yes )
        {
            file_based->set_dom( element );         // Objekt ergänzen (<base>) oder ändern. Evtl. Exception, wenn Objekt das nicht kann, z.B. <job>
        }
        else
        {
            ptr<File_based> replacement = new_initialized_file_based_xml( element );
            file_based->replace_with( replacement );
        }
    }
    else
    {
        //if( replace_no  && 
        //    file_based_subsystem()->subsystem_state() > subsys_initialized )   // Wird nicht die Scheduler-Konfigurationsdatei geladen? (Sonst <base> erlauben)
        //{
        //    z::throw_xc( SCHEDULER-441, obj_name() );   // replace="no" und Objekt ist nicht bekannt
        //}

        add_file_based_xml( element, default_name );
    }
}

//------------------------------------------------Typed_folder::add_to_replace_or_remove_candidates

void Typed_folder::add_to_replace_or_remove_candidates( const File_based& file_based )             
{ 
    _replace_or_remove_candidates_set.insert( file_based.name() ); 
    spooler()->folder_subsystem()->set_signaled( Z_FUNCTION );      // Könnte ein getrenntes Ereignis sein, denn das Verzeichnis muss nicht erneut gelesen werden.
}

//------------------------------------------------Typed_folder::handle_replace_or_remove_candidates

void Typed_folder::handle_replace_or_remove_candidates()
{
    if( !_replace_or_remove_candidates_set.empty() )
    {
        String_set my_path_set = _replace_or_remove_candidates_set;
        _replace_or_remove_candidates_set.clear();
        
        Z_FOR_EACH( String_set, my_path_set, it )
        {
            if( File_based* file_based = file_based_or_null( *it ) )
            {
                if( file_based->replacement()  &&  file_based->can_be_replaced_now() )
                {
                    file_based->log()->info( message_string( "SCHEDULER-936", subsystem()->object_type_name() ) );
                    file_based->replace_now();
                }
                else
                if( file_based->is_to_be_removed()  &&  file_based->can_be_removed_now() ) 
                {
                    file_based->log()->info( message_string( "SCHEDULER-937", subsystem()->object_type_name() ) );
                    file_based->remove_now();
                }
            }
        }
    } 
}

//---------------------------------------------------------------------Typed_folder::add_file_based

void Typed_folder::add_file_based( File_based* file_based )
{
    if( !file_based )  assert(0), z::throw_xc( Z_FUNCTION, "NULL" );
    assert( !file_based->typed_folder() );

    string normalized_name = file_based->normalized_name();
    if( normalized_name == ""  &&  !is_empty_name_allowed() )  z::throw_xc( "SCHEDULER-432", subsystem()->object_type_name() );

    if( file_based_or_null( normalized_name ) )  
        z::throw_xc( "SCHEDULER-160", subsystem()->object_type_name(), file_based->path().to_string() );

    file_based->fix_name();
    file_based->set_typed_folder( this );
    file_based->log()->set_prefix( file_based->obj_name() );        // Jetzt mit Pfad

    _file_based_map[ normalized_name ] = file_based;

    subsystem()->add_file_based( file_based );
}

//------------------------------------------------------------------Typed_folder::remove_file_based

void Typed_folder::remove_file_based( File_based* file_based )
{
    if( !file_based )  assert(0), z::throw_xc( Z_FUNCTION, "NULL" );
    assert( file_based->typed_folder() == this );
    assert( file_based->can_be_removed_now() );
    
    ptr<File_based> holder      = file_based;
    string          object_name = file_based->obj_name();

    _replace_or_remove_candidates_set.erase( file_based->path() );


    subsystem()->remove_file_based( file_based );


    File_based_map::iterator it = _file_based_map.find( file_based->normalized_name() );
    assert( it != _file_based_map.end() );
    if( it != _file_based_map.end() )
    {
        try
        {
            file_based->close();
        }
        catch( exception& x )
        {
            log()->warn( S() << x.what() << ", when removing " << file_based->obj_name() );
        }

        file_based->set_typed_folder( NULL );
        _file_based_map.erase( it );

        if( _file_based_map.empty() )  folder()->check_for_replacing_or_removing();
    }


    log()->log( subsystem()->subsystem_state() < subsys_stopped? log_info : log_debug9, message_string( "SCHEDULER-861", object_name ) );
}

//-----------------------------------------------------------------Typed_folder::replace_file_based

File_based* Typed_folder::replace_file_based( File_based* old_file_based )
{
    assert( old_file_based->replacement() );

    File_based* new_file_based  = old_file_based->replacement();
    string      normalized_name = new_file_based->normalized_name();

    if( old_file_based->normalized_name() != normalized_name )  assert(0), z::throw_xc( Z_FUNCTION );
    if( file_based( normalized_name ) != old_file_based )       assert(0), z::throw_xc( Z_FUNCTION );
    if( new_file_based->typed_folder() )                        assert(0), z::throw_xc( Z_FUNCTION );

    _replace_or_remove_candidates_set.erase( old_file_based->path() );

    old_file_based->set_typed_folder( NULL );
    new_file_based->set_typed_folder( this );
    _file_based_map[ normalized_name ] = new_file_based;

    subsystem()->replace_file_based( old_file_based, new_file_based );

    return new_file_based;
}

//-------------------------------------------------------------Typed_folder::remove_all_file_baseds

//void Typed_folder::remove_all_file_baseds()
//{
//    for( File_based_map::iterator it = _file_based_map.begin(); it != _file_based_map.end(); )
//    {
//        File_based_map::iterator next_it = it;  next_it++;
//
//        File_based* file_based = it->second;
//        if( !file_based->is_to_be_removed() )  file_based->remove();
//
//        it = next_it;
//    }
//}

//-------------------------------------------------------------------------Typed_folder::file_based

File_based* Typed_folder::file_based( const string& name ) const
{
    File_based* result = file_based_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-430", file_based_subsystem()->object_type_name(), name );      // Sollte nicht passieren
    return result;
}

//-----------------------------------------------------------------Typed_folder::file_based_or_null

File_based* Typed_folder::file_based_or_null( const string& name ) const
{
    const File_based_map::const_iterator it = _file_based_map.find( subsystem()->normalized_name( name ) );
    return it == _file_based_map.end()? NULL 
                                      : it->second;
}

//----------------------------------------------------------------------------Typed_folder::set_dom

void Typed_folder::set_dom( const xml::Element_ptr& element )
{
    if( !element.nodeName_is( subsystem()->xml_elements_name() ) )  z::throw_xc( "SCHEDULER-409", subsystem()->xml_elements_name(), element.nodeName() );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( !e.nodeName_is( subsystem()->xml_element_name() ) )  z::throw_xc( "SCHEDULER-409", subsystem()->xml_elements_name(), element.nodeName() );
        
        string spooler_id = element.getAttribute( "spooler_id" );

        if( spooler_id.empty()  ||  spooler_id == _spooler->id() )
        {
            add_or_replace_file_based_xml( e );
        }
    }
}

//------------------------------------------------------------------------Typed_folder::dom_element

xml::Element_ptr Typed_folder::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = new_dom_element( document, show_what );

    Z_FOR_EACH( File_based_map, _file_based_map, it )
    {
        File_based* file_based = it->second;
        
        if( file_based->is_visible_in_xml_folder( show_what ) )
        {
            result.appendChild( file_based->dom_element( document, show_what ) );
        }
    }

    return result;
}

//---------------------------------------------------------------------------Typed_folder::obj_name

string Typed_folder::obj_name() const
{
    S result;
    result << Scheduler_object::obj_name();

    if( _folder )
    {
        result << " " << _folder->path();
    }

    return result;
}

//---------------------------------------------------------------------------File_based::File_based

File_based::File_based( File_based_subsystem* subsystem, IUnknown* iunknown, Type_code type_code )
: 
    Scheduler_object( subsystem->spooler(), iunknown, type_code ), 
    _zero_(this+1),
    _file_based_subsystem(subsystem),
    _state(s_not_initialized)
{
}

//--------------------------------------------------------------------------File_based::~File_based
    
File_based::~File_based()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------------File_based::close
    
void File_based::close()
{
    //_wished_state = s_closed;
    _state = s_closed;   //pure virtual function called: set_file_based_state( s_closed );
}

//---------------------------------------------------------------------------File_based::initialize

bool File_based::initialize()
{
    //if( _wished_state < s_initialized )  _wished_state = s_initialized;
    return initialize2();
}

//--------------------------------------------------------------------------File_based::initialize2

bool File_based::initialize2()
{
    bool ok = _state >= s_initialized;

    if( _state == s_not_initialized  &&  subsystem()->subsystem_state() >= subsys_initialized )
    {
        ok = on_initialize();
        if( ok )  set_file_based_state( s_initialized );
    }

    return ok;
}

//---------------------------------------------------------------------------------File_based::load

bool File_based::load()
{
    //if( _wished_state < s_loaded )  _wished_state = s_loaded;
    return load2();
}

//--------------------------------------------------------------------------------File_based::load2

bool File_based::load2()
{
    if( !is_in_folder()  &&  this != spooler()->root_folder() )  assert(0), z::throw_xc( "NOT-IN-FOLDER", Z_FUNCTION, obj_name() );

    bool ok = _state >= s_loaded;
    if( !ok )
    {
        initialize2();

        if( _state == s_initialized  &&  subsystem()->subsystem_state() >= subsys_loaded )
        {
            ok = on_load();

            if( ok )
            {
                set_file_based_state( s_loaded );
                subsystem()->dependencies()->announce_dependant_loaded( this ); 
            }
        }
    }

    return ok;
}

//-----------------------------------------------------------------------------File_based::activate

bool File_based::activate()
{
    //if( _wished_state < s_active )  _wished_state = s_active;
    return activate2();
}

//----------------------------------------------------------------------------File_based::activate2

bool File_based::activate2()
{
    bool ok = _state >= s_active;
    if( !ok )
    {
        load2();

        if( _state == s_loaded  &&  subsystem()->subsystem_state() >= subsys_active )
        {
            ok = on_activate();
            
            if( ok )
            {
                set_file_based_state( s_active );
            }
        }
    }

    return ok;
}

//-----------------------------------------------------------------File_based::set_file_based_state

void File_based::set_file_based_state( State state )                         
{ 
    if( _state != state )
    {
        assert( _state == s_not_initialized && state == s_undefined  ||  _state < state );
        _state = state; 

        log()->log( _state == s_active? log_info : log_debug9,
                    message_string( "SCHEDULER-893", subsystem()->object_type_name(), file_based_state_name() ) );
    }
}

//--------------------------------------------------------------------------File_based::set_defined

//void File_based::set_defined()
//{
//    if( _state != s_undefined )  assert(0), z::throw_xc( __FUNCTION__, obj_name() );
//
//    set_file_based_state( s_not_initialized );
//}

//--------------------------------------------------------------File_based::try_set_to_wished_state

//bool File_based::try_switch_wished_file_based_state()
//{
//    return switch_file_based_state( _wished_state );
//}

//--------------------------------------------------------------File_based::switch_file_based_state

bool File_based::switch_file_based_state( State state )
{
    bool result = false;

    switch( state )
    {
        case s_undefined:        break;
        case s_not_initialized:  break;
        case s_initialized: result = initialize();  break;
        case s_loaded:      result = load();        break;
        case s_active:      result = activate();    break;
      //case s_error:       result = true;  set_file_based_state( state );  break;
        case s_closed:      result = true;  close(); break;
        default:            assert(0);
    }

    return result;
}

//------------------------------------------------------------------File_based::on_dependant_loaded

bool File_based::on_dependant_loaded( File_based* )
{
    bool result = false;

    if( is_in_folder() )  check_for_replacing_or_removing();    // Für Standing_order: Wenn Jobkette gelöscht, Auftragsdatei geändert und Jobkette wieder geladen wird, 2008-02-01
    if( is_in_folder() )  result = activate();
    
    return result;
}

//-----------------------------------------------------------File_based::on_dependant_to_be_removed

bool File_based::on_dependant_to_be_removed( File_based* file_based )
{
    assert( file_based->is_in_folder() );

    return true;
}

//-----------------------------------------------------------------File_based::on_dependant_removed

void File_based::on_dependant_removed( File_based* file_based )
{
    assert( file_based->is_in_folder() );

    check_for_replacing_or_removing();
    Pendant::on_dependant_removed( file_based );
}

//---------------------------------------------------------------------File_based::assert_is_loaded

void File_based::assert_is_initialized()
{
    if( _state != s_initialized  &&
        _state != s_loaded  &&
        _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_initialized ), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_initialized ) );
    }
}

//---------------------------------------------------------------------File_based::assert_is_loaded

void File_based::assert_is_loaded()
{
    if( _state != s_loaded  &&
        _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_loaded ), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_loaded ) );
    }
}

//---------------------------------------------------------------------File_based::assert_is_active

void File_based::assert_is_active()
{
    if( _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_active ), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_active ) );
    }
}

//------------------------------------------------------------File_based::assert_is_not_initialized

void File_based::assert_is_not_initialized()
{
    if( _state >= s_initialized )
    {
        z::throw_xc( "SCHEDULER-148", obj_name() );
    }
}

//--------------------------------------------------------------------File_based::set_to_be_removed

void File_based::set_to_be_removed( bool b )
{ 
    _is_to_be_removed = b; 
    if( _is_to_be_removed )  set_replacement( NULL );
}

//----------------------------------------------------------------------File_based::set_replacement

void File_based::set_replacement( File_based* replacement )             
{ 
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );
    
    if( replacement  &&  
        _is_from_cache  &&  
        !replacement->_is_from_cache &&
        _base_file_info._path.exists() )    // Außer, die zentrale Datei ist gelöscht
    {
        z::throw_xc( "SCHEDULER-460", subsystem()->object_type_name() );  // Original ist zentral konfiguriert, aber Ersatz nicht
    }

    _replacement = replacement; 
    if( _replacement )  set_to_be_removed( false ); 
}

//--------------------------------------------------------------------File_based::prepare_to_remove

void File_based::prepare_to_remove()
{ 
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );

    set_to_be_removed( true );
    subsystem()->dependencies()->announce_dependant_to_be_removed( this ); 
}

//------------------------------------------------------------------------File_based::on_remove_now

void File_based::on_remove_now()
{
}

//---------------------------------------------------------------------------File_based::remove_now

void File_based::remove_now()
{
    on_remove_now();
    typed_folder()->remove_file_based( this );
}

//-------------------------------------------------------------------------------File_based::remove

bool File_based::remove( Remove_flags remove_flag )
{
    bool result = false;

    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );


    if( remove_flag == rm_base_file_too  &&  has_base_file() )
    {
        remove_base_file();
    }


    prepare_to_remove();

    if( can_be_removed_now() )  
    {
        _remove_xc = zschimmer::Xc();

        remove_now();
        result = true;
    }
    else  
    {
        _remove_xc = remove_error();
        log()->info( _remove_xc.what() );   // Kein Fehler, Löschen ist nur verzögert
    }


    return result;
}

//-------------------------------------------------------------------------File_based::remove_error

zschimmer::Xc File_based::remove_error()
{
    return zschimmer::Xc( "SCHEDULER-989", subsystem()->object_type_name() );
}

//----------------------------------------------------------------------File_base::remove_base_file

void File_based::remove_base_file()
{
    if( !has_base_file() )  assert(0), z::throw_xc( Z_FUNCTION );

    try
    {
#       ifdef Z_DEBUG
            _base_file_info._path.move_to( base_file_info()._path + "-REMOVED" );
#        else
            _base_file_info._path.unlink();
#       endif    

        handle_event( File_based::bfevt_removed );
    }
    catch( exception& )
    {
        if( _base_file_info._path.exists() )  throw;
    }

    _file_is_removed = true;
}

//-------------------------------------------------------------------------File_based::replace_with

bool File_based::replace_with( File_based* file_based_replacement )
{
    bool result = false;

    set_replacement( file_based_replacement );
    
    prepare_to_replace();

    if( can_be_replaced_now() )
    {
        replace_now();
        result = true;
    }
    else  
        log()->info( message_string( "SCHEDULER-888", subsystem()->object_type_name() ) );

    return result;
}

//------------------------------------------------------File_based::check_for_replacing_or_removing

void File_based::check_for_replacing_or_removing( When_to_act when_to_act )
{
    try
    {
        if( is_in_folder() )
        {
            if( replacement() )
            {
                bool ok = replacement()->file_based_state() == File_based::s_initialized;

                if( !ok )
                {
                    ok = replacement()->initialize();
                    if( ok )  prepare_to_replace();
                }
                
                if( ok  &&  can_be_replaced_now() )
                {
                    if( when_to_act == act_now )  replace_now();
                                            else  typed_folder()->add_to_replace_or_remove_candidates( *this );
                }
            }
            else
            if( is_to_be_removed()  &&  can_be_removed_now() )
            {
                if( when_to_act == act_now )  remove_now();
                                        else  typed_folder()->add_to_replace_or_remove_candidates( *this );
            }
        }
    }
    catch( exception& x ) { log()->debug( message_string( "SCHEDULER-897", x ) ); }
}

//-------------------------------------------------------------------File_based::prepare_to_replace

void File_based::prepare_to_replace()
{
    assert( _replacement );
    ptr<File_based> replacement = _replacement;     // prepare_to_remove() entfernt _replacement

    prepare_to_remove();        

    _replacement = replacement;
}

//------------------------------------------------------------------File_based::can_be_replaced_now

bool File_based::can_be_replaced_now()
{
    return replacement()  &&  
           replacement()->file_based_state() == File_based::s_initialized  &&
           can_be_removed_now();
}

//-----------------------------------------------------------------------File_based::on_replace_now

File_based* File_based::on_replace_now()
{
    Typed_folder*   typed_folder = this->typed_folder();
    ptr<File_based> replacement  = this->replacement();

    assert( can_be_replaced_now() );

    typed_folder->remove_file_based( this );
    // this ist ungültig

    typed_folder->add_file_based( replacement );

    return replacement;
}

//--------------------------------------------------------------------------File_based::replace_now

File_based* File_based::replace_now()
{
    assert( replacement() );

    //State wished_state = _wished_state;
    Base_file_info file_info = replacement()->base_file_info();

    File_based* new_file_based = on_replace_now();
    // this ist ungültig

    if( new_file_based == this )              // Process_class und Lock werden nicht ersetzt. Stattdessen werden die Werte übernommen
    {                                       
        new_file_based->set_base_file_info( file_info );        // Alte Werte geänderten Objekts überschreiben
        new_file_based->_base_file_xc      = zschimmer::Xc();
        new_file_based->_base_file_xc_time = 0;
    }

    //new_file_based->switch_file_based_state( wished_state );
    new_file_based->activate();
    return new_file_based;
}

//----------------------------------------------------------------File_based::file_based_state_name

string File_based::file_based_state_name( State state )
{
    switch( state )
    {
        case s_undefined:       return "undefined";
        case s_not_initialized: return "not_initialized";
        case s_initialized:     return "initialized";
        case s_loaded:          return "loaded";
        case s_active:          return "active";
        case s_closed:          return "closed";
        default:                return S() << "File_based_state-" << state;
    }
}

//----------------------------------------------------------------------File_based::set_folder_path

void File_based::set_folder_path( const Absolute_path& folder_path )
{
    assert( !folder_path.empty() );
    assert( !_typed_folder );
    assert( _folder_path.empty()  ||  spooler()->folder_subsystem()->normalized_path( _folder_path ) == spooler()->folder_subsystem()->normalized_path( folder_path ) );

    _folder_path = folder_path;
}

//--------------------------------------------------------------------------File_based::folder_path

Absolute_path File_based::folder_path() const
{
    assert( !is_in_folder()  ||  spooler()->folder_subsystem()->normalized_path( _folder_path ) == folder()->normalized_path() );
    return _folder_path;
}

//---------------------------------------------------------------------File_based::set_typed_folder

void File_based::set_typed_folder( Typed_folder* typed_folder )
{ 
    if( typed_folder )
    {
        assert( _folder_path.empty()  ||  spooler()->folder_subsystem()->normalized_path( _folder_path ) == typed_folder->folder()->normalized_path() );
        _typed_folder = typed_folder; 
        _folder_path = _typed_folder->folder()->path();
    }
    else
        _typed_folder = NULL;
}

//----------------------------------------------------------File_based::fill_file_based_dom_element

void File_based::fill_file_based_dom_element( const xml::Element_ptr& element, const Show_what& show_what )
{
    element.setAttribute         ( "path", path().with_slash() );
    element.setAttribute_optional( "name", name() );

    if( has_base_file() )  element.appendChild( File_based::dom_element( element.ownerDocument(), show_what ) );
    if( replacement()   )  element.append_new_element( "replacement" ).appendChild( replacement()->dom_element( element.ownerDocument(), show_what ) );
}

//--------------------------------------------------------------------------File_based::dom_element

xml::Element_ptr File_based::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr element;   

    element = document.createElement( "file_based" );

    if( has_base_file() )
    {
        element.setAttribute_optional( "file", _base_file_info._path );
        if( _file_is_removed )  element.setAttribute( "removed", "yes" );

        Time t;
        t.set_utc( _base_file_info._last_write_time );
        element.setAttribute( "last_write_time", t.xml_value() );

        element.setAttribute( "state", file_based_state_name() );

        if( base_file_has_error() )  element.appendChild( create_error_element( document, _base_file_xc, (time_t)_base_file_xc_time ) );

        if( !_remove_xc.is_empty() )  element.append_new_element( "removed" ).appendChild( create_error_element( document, _remove_xc ) );
    }

    return element;
}

//---------------------------------------------------------------------------------File_based::path

Absolute_path File_based::path() const
{ 
    return _typed_folder? _typed_folder->folder()->make_path( _name ) : 
           _name == ""  ? root_path
                        : Absolute_path( _folder_path.empty()? Absolute_path( "/(not in a folder)" ) 
                                                             : _folder_path                        , _name ); 
}

//----------------------------------------------------------------------File_based::normalized_name

string File_based::normalized_name() const
{ 
    return _file_based_subsystem->normalized_name( _name ); 
}

//----------------------------------------------------------------------File_based::normalized_path

string File_based::normalized_path() const
{ 
    return _file_based_subsystem->normalized_path( path() ); 
}

//-----------------------------------------------------------------------------File_based::obj_name

string File_based::obj_name() const
{ 
    S result;
    
    result << _file_based_subsystem->object_type_name();
    result << " ";
    result << path().without_slash(); 

    return result;
}

//-----------------------------------------------------------------------------File_based::set_name
    
void File_based::set_name( const string& name )
{
    _spooler->check_name( name );

    string normalized_name = this->normalized_name();

    if( normalized_name != _file_based_subsystem->normalized_name( name ) )
    {
        if( _name_is_fixed )  z::throw_xc( "SCHEDULER-429", obj_name(), name );       // Name darf nicht geändert werden, außer Großschreibung
        _name = name;
        log()->set_prefix( obj_name() );    // Noch ohne Pfad

        if( !has_base_file() )
        {
            assert( _base_file_info._normalized_name == ""  ||  _base_file_info._normalized_name == normalized_name );
            _base_file_info._normalized_name = subsystem()->normalized_name( name );
        }
    }
}

//-------------------------------------------------------------------------------File_based::folder
    
Folder* File_based::folder() const
{ 
    return _typed_folder? _typed_folder->folder()
                        : spooler()->folder_subsystem()->folder( folder_path() );   // _state < s_initialized, noch nicht im Typed_folder eingehängt, replacement()
}

//-------------------------------------------------------------------------File_based::handle_event

void File_based::handle_event( Base_file_event base_file_event )
{
    if( spooler()->folder_subsystem()->subsystem_state() == subsys_active )
    {
        Absolute_path job_path;
        
        switch( base_file_event )
        {
            case bfevt_added:       job_path = spooler()->_configuration_start_job_after_added;     break;
            case bfevt_modified:    job_path = spooler()->_configuration_start_job_after_modified;  break;
            case bfevt_removed :    job_path = spooler()->_configuration_start_job_after_deleted;   break;
            default:                assert(0), throw_xc(__FUNCTION__ );
        }

        if( !job_path.empty() )
        {
            try
            {
                ptr<Com_variable_set> parameters  = new Com_variable_set;
                ptr<Com_variable_set> environment = new Com_variable_set;

                environment->set_var( "SCHEDULER_LIVE_FILEBASE", _base_file_info._filename );
                environment->set_var( "SCHEDULER_LIVE_FILEPATH", _base_file_info._path );
                environment->set_var( "SCHEDULER_LIVE_EVENT"   , base_file_event == bfevt_added   ? "add"      :
                                                                 base_file_event == bfevt_modified? "modified" :
                                                                 base_file_event == bfevt_removed ? "deleted"  : "" );

                Z_FOR_EACH( Com_variable_set::Map, environment->_map, v )  parameters->set_var( lcase( v->second->name() ), v->second->string_value() );
                
                Job*  job  = spooler()->job_subsystem()->job( job_path );
                ptr<Task> task = job->create_task( +parameters , "", 0 );
                task->merge_environment( environment );
                job->enqueue_task( task );
            }
            catch( exception& x )
            {
                spooler()->folder_subsystem()->log()->error( x.what() );
            }
        }
    }
}

//-------------------------------------------------------File_based_subsystem::File_based_subsystem

File_based_subsystem::File_based_subsystem( Spooler* spooler, IUnknown* iunknown, Type_code type_code ) 
: 
    Subsystem( spooler, iunknown, type_code ),
    _dependencies( this )
{
}

//------------------------------------------------------------File_based_subsystem::normalized_path
    
string File_based_subsystem::normalized_path( const Path& path ) const
{
    Path folder_path = path.folder_path();

    return Path( folder_path.is_root()  ||  folder_path.empty()? folder_path
                                                               : spooler()->folder_subsystem()->normalized_path( folder_path ), 
                 normalized_name( path.name() ) );
}

//---------------------------------------------------File_based_subsystem::check_file_based_element

void File_based_subsystem::check_file_based_element( const xml::Element_ptr& element )
{
    assert_xml_element_name( element );

    if( element.getAttribute( "spooler_id" ) != ""  &&
        element.getAttribute( "spooler_id" ) != _spooler->id() )
    {
        log()->warn( message_string( "SCHEDULER-232", element.nodeName(), "spooler_id", element.getAttribute( "spooler_id" ) ) );
    }
}

//----------------------------------------------------File_based_subsystem::assert_xml_element_name

void File_based_subsystem::assert_xml_element_name( const xml::Element_ptr& e ) const
{ 
    if( !e.nodeName_is( xml_element_name() ) )  z::throw_xc( "SCHEDULER-409", xml_element_name(), e.nodeName() );
}

//---------------------------------------------------------------------------------Pendant::Pendant

Pendant::Pendant()
{
}

//--------------------------------------------------------------------------------Pendant::~Pendant

Pendant::~Pendant()
{
    try
    {
        remove_dependants();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------------Pendant::remove_dependants

void Pendant::remove_dependants()
{
    Z_FOR_EACH( Dependant_sets, _dependants_sets, it )
    {
        File_based_subsystem* subsystem     = it->first;
        Dependant_set&        dependant_set = it->second;

        Z_FOR_EACH( Dependant_set, dependant_set, it2 )
        {
            subsystem->dependencies()->remove_dependant( this, *it2 );
        }
    }
}

//---------------------------------------------------------------------------Pendant::add_dependant

void Pendant::add_dependant( File_based_subsystem* subsystem, const Absolute_path& path )
{
    _dependants_sets[ subsystem ].insert( subsystem->normalized_path( path ) );
    subsystem->dependencies()->add_dependant( this, path );
}

//------------------------------------------------------------------------Pendant::remove_dependant

void Pendant::remove_dependant( File_based_subsystem* subsystem, const Absolute_path& path )
{
    _dependants_sets[ subsystem ].erase( subsystem->normalized_path( path ) );
    subsystem->dependencies()->remove_dependant( this, path );
}

//--------------------------------------------------------------Pendant::on_dependant_to_be_removed

bool Pendant::on_dependant_to_be_removed( File_based* )
{
    return false;
}

//--------------------------------------------------------------------Pendant::on_dependant_removed

void Pendant::on_dependant_removed( File_based* )
{
}

//-----------------------------------------------------------------------Dependencies::Dependencies

Dependencies::Dependencies( File_based_subsystem* subsystem )
:
    _zero_(this+1),
    _subsystem( subsystem )
{
}

//----------------------------------------------------------------------Dependencies::~Dependencies
    
Dependencies::~Dependencies()
{
}

//----------------------------------------------------------------------Dependencies::add_dependant

void Dependencies::add_dependant( Pendant* pendant, const string& missings_path )
{
    _path_requestors_map[ _subsystem->normalized_path( missings_path ) ].insert( pendant );
}

//-------------------------------------------------------------------Dependencies::remove_dependant

void Dependencies::remove_dependant( Pendant* pendant, const string& missings_path )
{
    Requestor_set&  requestors_set = _path_requestors_map[ missings_path ];
    
    requestors_set.erase( pendant );
    if( requestors_set.empty() )  _path_requestors_map.erase( _subsystem->normalized_path( missings_path ) );
}

//----------------------------------------------------------Dependencies::announce_dependant_loaded

void Dependencies::announce_dependant_loaded( File_based* found_missing )
{
    assert( found_missing->subsystem() == _subsystem );

    Path_requestors_map::iterator it = _path_requestors_map.find( found_missing->normalized_path() );

    if( it != _path_requestors_map.end() )
    {
        Requestor_set& requestor_set = it->second;

        Z_FOR_EACH( Requestor_set, requestor_set, it2 )
        {
            Requestor_set::iterator next_it2 = it2;  next_it2++;
            Pendant*                pendant  = *it2;
        
            try
            {
                pendant->on_dependant_loaded( found_missing );
            }
            catch( exception& x )
            {
                pendant->log()->error( message_string( "SCHEDULER-459", found_missing->obj_name(), x.what() ) );
            }
        }
    }
}

//---------------------------------------------------Dependencies::announce_dependant_to_be_removed

bool Dependencies::announce_dependant_to_be_removed( File_based* to_be_removed )
{
    assert( to_be_removed->subsystem() == _subsystem );

    bool result = true;

    Path_requestors_map::iterator it = _path_requestors_map.find( to_be_removed->normalized_path() );

    if( it != _path_requestors_map.end() )
    {
        Requestor_set& requestor_set = it->second;

        Z_FOR_EACH( Requestor_set, requestor_set, it2 )
        {
            Requestor_set::iterator next_it2 = it2;  next_it2++;
            Pendant*                pendant  = *it2;
        
            //Z_DEBUG_ONLY( _subsystem->log()->info( S() << "    " << pendant->obj_name() << " on_dependant_to_be_removed( " << to_be_removed->obj_name() << " ) " ); )
            result = pendant->on_dependant_to_be_removed( to_be_removed );
            if( !result )  break;
        }
    }

    return result;
}

//-------------------------------------------------------------------Base_file_info::Base_file_info

Base_file_info::Base_file_info( const directory_observer::Directory_entry& de ) 
: 
    _path           ( de._file_info->path()            ), 
    _filename       ( de._file_info->path().name()     ), 
    _last_write_time( de._file_info->last_write_time() ),
    _normalized_name( de._normalized_name              ) 
{
}

//-------------------------------------------------------------------------------------------------

} //namespace folder
} //namespace scheduler
} //namespace sos
