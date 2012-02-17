#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace directory_observer;

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
    _schedule_folder         = spooler()->schedule_subsystem        ()->new_schedule_folder      ( this );
    _subfolder_folder        = spooler()->folder_subsystem          ()->new_subfolder_folder     ( this );

  //add_to_typed_folder_map( _scheduler_script_folder );
    add_to_typed_folder_map( _process_class_folder    );
    add_to_typed_folder_map( _lock_folder             );
    add_to_typed_folder_map( _job_folder              );
    add_to_typed_folder_map( _job_chain_folder        );
    add_to_typed_folder_map( _schedule_folder         );
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

//-----------------------------------------------------------------------------Folder::typed_folder

Typed_folder* Folder::typed_folder( const File_based_subsystem* subsystem ) const
{
    Typed_folder_map::const_iterator it = _typed_folder_map.find( subsystem->filename_extension() );
    if( it == _typed_folder_map.end() )  z::throw_xc( Z_FUNCTION, subsystem->obj_name() );
    return it->second;
}

//-----------------------------------------------------------------------------Folder::typed_folder

Typed_folder* Folder::typed_folder(const string& type_name) const
{
    string filename_extension = type_name == "folder"? _spooler->_folder_subsystem->filename_extension()
        : string(".")+ type_name +".xml";
    Typed_folder_map::const_iterator it = _typed_folder_map.find(filename_extension);
    if( it == _typed_folder_map.end() )  z::throw_xc( Z_FUNCTION, type_name );
    return it->second;
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
    typedef stdext::hash_map< Typed_folder*, list< const Directory_entry* > >   File_list_map;
    
    File_list_map file_list_map;
    bool          something_changed = false;
  //Absolute_path folder_path       = path();

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )  file_list_map[ it->second ] = list< const Directory_entry* >();

    try
    {
        // DATEINAMEN EINSAMMELN

        if( directory  &&  !base_file_is_removed() )
        {
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
                    // Hier abhängige Dateien (die keine Objekte sind) prüfen, nur für live/, nicht für cache!
                    // Pfad im zentralen Register nachsehen, 
                    // Objekt ermitteln
                    // Wenn Zeitstempel des Pfads im Objekt verschieden ist:
                    //      Objekt ersetzen:  object->reread();     load_from_xml(), set_replacement(), später ersetzen. Aber mehrfache Ersetzung vermeiden, also nur signalisieren.
                    //folder_subsystem()->Included_files()->check_file( Absolute_path( folder_path, name ), directory_entry->_file_info );

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

                    file_list_map[ typed_folder ].push_back( directory_entry );
                }
            }
        }

        Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
        {
            Typed_folder* typed_folder = it->second;

            typed_folder->remove_duplicates_from_list( &file_list_map[ typed_folder ] );

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

    // Auswerten des flags für die Auflösung der Ordnerstrukur (no_subfolders)
    if ( !show_what.is_set(show_no_subfolders_flag) ) {                                                           // JS-506

        // JS-506: Das flag show_no_subfolders_flag wird nur intern verwendet. Es ergänzt die Struktur show_what.
        Show_what myShowWhat = show_what;
        if (show_what.is_set( show_no_subfolders ) )
           myShowWhat |= show_no_subfolders_flag;

        Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
        {
            Typed_folder* typed_folder = it->second;
            if( !typed_folder->is_empty() )
            {
                if(show_what.is_subsystem_set_empty())
                    result.appendChild( typed_folder->dom_element( dom_document, myShowWhat ) );
                else
                if( show_what.is_subsystem_set( typed_folder->subsystem() ) )
                    result.appendChild( typed_folder->dom_element( dom_document, myShowWhat ) );
            }
        }
    }                                                                           // JS-2010-05-25

    return result;
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

//---------------------------------------------------------------------Folder::on_prepare_to_remove

void Folder::on_prepare_to_remove()
{
    //Das wird besser von adjust_with_directory() erledigt: remove_all_file_baseds();
    My_file_based::on_prepare_to_remove();
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

//---------------------------------------------------------------------------------Folder::obj_name

string Folder::obj_name() const
{
    S result;
    result << Scheduler_object::obj_name();
    result << " " << path();
    return result;
}

}}} //namespace sos::scheduler::folder
