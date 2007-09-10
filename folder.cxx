// $Id: cluster.h 5126 2007-07-13 08:59:30Z jz $

#include "spooler.h"
#include "../zschimmer/directory_lister.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace zschimmer::file;

//--------------------------------------------------------------------------------------------const

const int                   directory_watch_interval        = 10;
const char                  job_chain_order_separator       = '#';                                  // Dateiname "jobchainname#orderid.order.xml"

//---------------------------------------------------------------Folder_subsystem::Folder_subsystem

Folder_subsystem::Folder_subsystem( Scheduler* scheduler )
:
    _zero_(this+1),
    Subsystem( scheduler, this, type_folder_subsystem )
{
}

//--------------------------------------------------------------Folder_subsystem::~Folder_subsystem
    
Folder_subsystem::~Folder_subsystem()
{
    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( __FUNCTION__ << "  ERROR  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------Folder_subsystem::close

void Folder_subsystem::close()
{
    _subsystem_state = subsys_stopped;
    set_async_manager( NULL );

    // Sollten wir alle Ordner schließen?
}

//------------------------------------------------------------------Folder_subsystem::set_directory

void Folder_subsystem::set_directory( const File_path& directory )
{
    _directory = directory;
}

//-----------------------------------------------------------Folder_subsystem::subsystem_initialize

bool Folder_subsystem::subsystem_initialize()
{
    _root_folder = Z_NEW( Folder( this, NULL, "" ) );

    _subsystem_state = subsys_initialized;
    return true;
}

//-----------------------------------------------------------------Folder_subsystem::subsystem_load

bool Folder_subsystem::subsystem_load()
{
    _subsystem_state = subsys_loaded;
    return true;
}

//-------------------------------------------------------------Folder_subsystem::subsystem_activate

bool Folder_subsystem::subsystem_activate()
{
    int VERZEICHNISUEBERWACHUNG_STARTEN;

    set_async_manager( _spooler->_connection_manager );
    async_continue();  // IM FEHLERFALL trotzdem subsys_active setzen? Prüfen, ob Verzeichnis überhaupt vorhanden ist, sonst Abbruch. Oder warten, bis es da ist?

    _subsystem_state = subsys_active;
    return true;
}

//----------------------------------------------------------------Folder_subsystem::async_continue_

bool Folder_subsystem::async_continue_( Continue_flags )
{
    _root_folder->adjust_with_directory();
    set_async_delay( directory_watch_interval );

    return true;
}

//--------------------------------------------------------------Folder_subsystem::async_state_text_

string Folder_subsystem::async_state_text_() const
{
    S result;

    result << obj_name();

    return result;
}

//-----------------------------------------------------------------------------------Folder::Folder

Folder::Folder( Folder_subsystem* folder_subsystem, Folder* parent_folder, const string& name )
:
    Scheduler_object( folder_subsystem->_spooler, this, type_folder ),
  //_parent_folder(parent_folder),
    _name(name),
    _zero_(this+1)
{
    if( parent_folder )
    {
        _path      = parent_folder->_path + "/" + name;
        _directory = File_path( parent_folder->_directory, name );
    }
    else
    {
        _path      = name;
        _directory = folder_subsystem->directory();
    }

    _process_class_folder = spooler()->process_class_subsystem()->new_process_class_folder( this );
    _lock_folder          = spooler()->lock_subsystem         ()->new_lock_folder         ( this );
    _job_folder           = spooler()->job_subsystem          ()->new_job_folder          ( this );
    _job_chain_folder     = spooler()->order_subsystem        ()->new_job_chain_folder    ( this );

    add_to_typed_folder_map( _process_class_folder );
    add_to_typed_folder_map( _lock_folder          );
    add_to_typed_folder_map( _job_folder           );
    add_to_typed_folder_map( _job_chain_folder     );

    _log->set_prefix( obj_name() );
}

//--------------------------------------------------------------------------add_to_typed_folder_map
    
void Folder::add_to_typed_folder_map( Typed_folder* typed_folder )
{
    _typed_folder_map[ typed_folder->subsystem()->filename_extension() ] = typed_folder;
}

//----------------------------------------------------------------------------------Folder::~Folder
    
Folder::~Folder()
{
    _typed_folder_map.clear();
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

//--------------------------------------------------------------------Folder::adjust_with_directory

void Folder::adjust_with_directory()
{
    typedef stdext::hash_map< Typed_folder*, list<Base_file_info> >   File_list_map;
    
    File_list_map file_list_map;

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )  file_list_map[ it->second ] = list<Base_file_info>();

    try
    {
        // DATEINAMEN EINSAMMELN

        try
        {
            string last_normalized_name;
            string last_filename;

            for( file::Directory_lister dir ( _directory );; )
            {
                ptr<file::File_info> file_info = dir.get();
                if( !file_info )  break;

                string filename  = file_info->path().name();
                string name      = object_name_of_filename( filename );
                string extension = extension_of_filename( filename );

                Typed_folder_map::iterator it = _typed_folder_map.find( extension );
                if( it != _typed_folder_map.end()  &&  name != "" )
                {
                    Typed_folder* typed_folder    = it->second;
                    string        normalized_name = typed_folder->subsystem()->normalized_name( name );

                    if( normalized_name == last_normalized_name )
                    {
                        log()->warn( message_string( "SCHEDULER-889", last_filename, filename ) );
                    }
                    else
                        file_list_map[ typed_folder ].push_back( Base_file_info( filename, (double)file_info->last_write_time(), normalized_name ) );

                    last_normalized_name = normalized_name;
                    last_filename = filename;
                }
            }
        }
        catch( exception& x )
        {
            //_log->error( message_string( "SCHEDULER-427", x ) );
            if( _directory.file_exists() )  throw;                          // Problem beim Lesen des Verzeichnisses
            
            _log->error( message_string( "SCHEDULER-882", _directory, x ) );   // Jemand hat das Verzeichnis enfernt

            //Z_FOR_EACH( Folder_set, _child_folder_set, it )
            //{
            //    Folder* child_folder = *it;
            //    child_folder->remove_all_objects();
            //}
            
            // Die Objekte dieses Ordners werden werden unten gelöscht, weil file_list_map leere File_info_list enthält
        }


        Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
        {
            Typed_folder*           typed_folder       = it->second;
            list<Base_file_info>*   file_info_list     = &file_list_map[ typed_folder ];    // Liste der vorgefundenen Dateien

            typed_folder->adjust_with_directory( *file_info_list );

        }
    }
    catch( exception& x ) 
    {
        _log->error( message_string( "SCHEDULER-431", x ) );
        int VERZEICHNISFEHLER_MERKEN;  // Fehler merken für <show_state>, oder was machen wir mit dem Fehler? Später wiederholen
    }
}

//---------------------------------------------------------------------------------Folder::obj_name

string Folder::obj_name() const
{
    S result;
    result << Scheduler_object::obj_name();
    if( path() != "" )  result << " " << path();
    return result;
}

//-----------------------------------------------------------------------Typed_folder::Typed_folder

Typed_folder::Typed_folder( Folder* folder, Type_code type_code )
: 
    Scheduler_object( folder->_spooler, this, type_code ),
    _zero_(this+1),
    _folder(folder)
{
    _log->set_prefix( obj_name() );
}

//--------------------------------------------------------------Typed_folder::adjust_with_directory
    
void Typed_folder::adjust_with_directory( const list<Base_file_info>& file_info_list )
{
    vector<const Base_file_info*> ordered_file_infos;     // Geordnete Liste der vorgefundenen Dateien    
    vector<File_based*>           ordered_file_based;     // Geordnete Liste der bereits bekannten (geladenen) Dateien

    ordered_file_infos.reserve( file_info_list.size() );
    Z_FOR_EACH_CONST( list<Base_file_info>, file_info_list, it )  ordered_file_infos.push_back( &*it );
    sort( ordered_file_infos.begin(), ordered_file_infos.end(), Base_file_info::less_dereferenced );

    int NICHT_JEDESMAL_ORDNEN;  // Verbesserung: Nicht jedesmal ordnen. Nicht nötig, wenn Typed_folder nicht verändert worden ist.
    ordered_file_based.reserve( _file_based_map.size() );
    Z_FOR_EACH( File_based_map, _file_based_map, fb )  if( fb->second->has_base_file() )  ordered_file_based.push_back( &*fb->second );
    sort( ordered_file_based.begin(), ordered_file_based.end(), File_based::less_dereferenced );


    vector<File_based*>::iterator fb = ordered_file_based.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen
    Z_FOR_EACH_CONST( vector<const Base_file_info*>, ordered_file_infos, it )
    {
        const Base_file_info* file_info = *it;

        if( fb == ordered_file_based.end()  ||  file_info->_normalized_name < (*fb)->_base_file_info._normalized_name )  // Datei hinzugefügt?
        {
            call_on_base_file_changed( (File_based*)NULL, file_info );       // Lädt die Datei in das Objekt 
        }
        else
        for(; fb != ordered_file_based.end()  &&  (*fb)->_base_file_info._normalized_name < file_info->_normalized_name; fb++ )   // Datei entfernt?
        { 
            if( !(*fb)->_file_is_removed )
            {
                call_on_base_file_changed( *fb, NULL );
            }
        }

        if( fb != ordered_file_based.end()  &&  (*fb)->_base_file_info._normalized_name == file_info->_normalized_name )   // Gleicher Name?
        {
            File_based* current_file_based = (*fb)->replacement()? (*fb)->replacement() : (*fb);

            if( (*fb)->_file_is_removed  ||
                current_file_based->_base_file_info._timestamp_utc != file_info->_timestamp_utc )  
            {
                call_on_base_file_changed( *fb, file_info );
            }

            fb++;
        }
    }


    // Zu den übrigenden File_based gibt es keine Dateien, diese sind also gelöscht:

    for(; fb != ordered_file_based.end(); fb++ )  
    { 
        (*fb)->_file_is_removed = true;
        call_on_base_file_changed( *fb, NULL );
    }
}

//----------------------------------------------------------Typed_folder::call_on_base_file_changed

File_based* Typed_folder::call_on_base_file_changed( File_based* old_file_based, const Base_file_info* base_file_info )
{
    ptr<File_based> file_based = NULL;

    if( old_file_based )  old_file_based->_file_is_removed = base_file_info == NULL;

    try
    {
        if( !base_file_info )
        {
            folder()->log()->info( message_string( "SCHEDULER-890", old_file_based->_base_file_info._filename, old_file_based->name() ) );
            file_based = old_file_based;                // Für catch()
            old_file_based->on_base_file_removed();
            file_based = NULL;
        }
        else
        {
            string name = Folder::object_name_of_filename( base_file_info->_filename );

            if( old_file_based )  folder()->log()->info( message_string( "SCHEDULER-892", base_file_info->_filename, old_file_based->obj_name() ) );
                            else  folder()->log()->info( message_string( "SCHEDULER-891", base_file_info->_filename, subsystem()->object_type_name(), name ) );

            file_based = subsystem()->call_new_file_based();
            file_based->set_name( name );
            file_based->set_base_file_info( *base_file_info );

            if( !old_file_based )  add_file_based( file_based );
            else
            if( file_based != old_file_based )  old_file_based->set_replacement( file_based );

            xml::Document_ptr dom_document ( string_from_file( File_path( folder()->directory(), base_file_info->_filename ) ) );
            if( spooler()->_validate_xml )  spooler()->_schema.validate( dom_document );

            assert_empty_attribute( dom_document.documentElement(), "spooler_id" );
            assert_empty_attribute( dom_document.documentElement(), "replace"    );

            file_based->set_dom( dom_document.documentElement() );

            if( old_file_based )  
            {
                bool can_be_replaced_now = old_file_based->prepare_to_replace();

                if( can_be_replaced_now ) 
                {
                    file_based = old_file_based->replace_now();
                    assert( !file_based->replacement() );
                    if( file_based == old_file_based )  
                    {
                        file_based->set_base_file_info( *base_file_info );
                        file_based->_base_file_xc      = zschimmer::Xc();
                        file_based->_base_file_xc_time = 0;
                        file_based->activate();
                    }
                }
            }
            else  
            {
                file_based->activate();
            }

            assert( file_based->typed_folder() );
        }
    }
    catch( exception& x )
    {
        if( !file_based )  throw;   // Sollte nicht passieren

        file_based->set_file_based_state( File_based::s_error );
        file_based->_base_file_xc      = x;
        file_based->_base_file_xc_time = double_from_gmtime();
        file_based->log()->error( message_string( "SCHEDULER-428", File_path( folder()->directory(), base_file_info->_filename ), x ) );

        int EMAIL_SCHICKEN;
    }

    return file_based;
}

//-----------------------------------------------------------------Typed_folder::ordered_file_infos
    
//vector<Base_file_info*> Typed_folder::ordered_file_infos()
//{
//    vector<Base_file_info*> result;
//    result.reserve( _file_based_map.size() );
//
//    Z_FOR_EACH( File_based_map, _file_based_map, it )
//    {
//        File_based* file_based = it->second;
//        if( file_based->has_base_file() )  result.push_back( &file_based->_base_file_info );
//    }
//
//    sort( result.begin(), result.end(), Base_file_info::less_dereferenced );
//
//    return result;
//}

//---------------------------------------------------------------------Typed_folder::add_file_based

void Typed_folder::add_file_based( File_based* file_based )
{
    if( !file_based )  z::throw_xc( __FUNCTION__, "NULL" );
    assert( !file_based->typed_folder() );

    string normalized_name = file_based->normalized_name();
    if( normalized_name == ""  &&  !is_empty_name_allowed() )  z::throw_xc( "SCHEDULER-432", subsystem()->object_type_name() );

    if( file_based_or_null( normalized_name ) )  z::throw_xc( "SCHEDULER-160", subsystem()->object_type_name(), file_based->path() );

    subsystem()->add_file_based( file_based );

    file_based->set_typed_folder( this );
    _file_based_map[ normalized_name ] = file_based;
}

//------------------------------------------------------------------Typed_folder::remove_file_based

void Typed_folder::remove_file_based( File_based* file_based )
{
    if( !file_based )  z::throw_xc( __FUNCTION__, "NULL" );
    assert( file_based->typed_folder() == this );
    assert( file_based->can_be_removed_now() );
    
    string object_name = file_based->obj_name();

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
    }


    subsystem()->remove_file_based( file_based );


    log()->log( subsystem()->subsystem_state() < subsys_stopped? log_info : log_debug9, message_string( "SCHEDULER-861", object_name ) );
}

//-----------------------------------------------------------------Typed_folder::replace_file_based

File_based* Typed_folder::replace_file_based( File_based* old_file_based )
{
    assert( old_file_based->replacement() );

    File_based* new_file_based  = old_file_based->replacement();
    string      normalized_name = new_file_based->normalized_name();

    if( old_file_based->normalized_name() != normalized_name )  z::throw_xc( __FUNCTION__ );
    if( file_based( normalized_name ) != old_file_based )       z::throw_xc( __FUNCTION__ );
    if( new_file_based->typed_folder() )                        z::throw_xc( __FUNCTION__ );

    old_file_based->set_typed_folder( NULL );
    new_file_based->set_typed_folder( this );
    _file_based_map[ normalized_name ] = new_file_based;

    subsystem()->replace_file_based( old_file_based, new_file_based );

    return new_file_based;
}

//-------------------------------------------------------------------------Typed_folder::file_based

File_based* Typed_folder::file_based( const string& name ) const
{
    File_based* result = file_based_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-430", name );      // Sollte nicht passieren
    return result;
}

//-----------------------------------------------------------------Typed_folder::file_based_or_null

File_based* Typed_folder::file_based_or_null( const string& name ) const
{
    const File_based_map::const_iterator it = _file_based_map.find( subsystem()->normalized_name( name ) );
    return it == _file_based_map.end()? NULL 
                                      : it->second;
}

//---------------------------------------------------------------------------Typed_folder::obj_name

string Typed_folder::obj_name() const
{
    S result;
    result << Scheduler_object::obj_name();
    if( _folder->path() != "" )  result << " " << _folder->path();
                           else  result << " /";
    return result;
}

//---------------------------------------------------------------------------File_based::File_based

File_based::File_based( File_based_subsystem* subsystem, IUnknown* iunknown, Type_code type_code )
: 
    Scheduler_object( subsystem->spooler(), iunknown, type_code ), 
    _zero_(this+1),
    _file_based_subsystem(subsystem)
{
}

//--------------------------------------------------------------------------File_based::~File_based
    
File_based::~File_based()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << "  ERROR  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------------File_based::close
    
void File_based::close()
{
    set_file_based_state( s_closed );
}

//---------------------------------------------------------------------------File_based::initialize

bool File_based::initialize()
{
    bool ok = _state == s_initialized;

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
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );

    bool ok = _state == s_loaded;

    if( !ok )
    {
        initialize();

        if( _state == s_initialized  &&  subsystem()->subsystem_state() >= subsys_loaded )
        {
            ok = on_load();
            if( ok )  set_file_based_state( s_loaded );
        }
    }

    return ok;
}

//-----------------------------------------------------------------------------File_based::activate

bool File_based::activate()
{
    bool ok = _state == s_active;

    if( !ok )
    {
        load();

        if( _state == s_loaded  &&  subsystem()->subsystem_state() >= subsys_active )
        {
            ok = on_activate();
            if( ok )  set_file_based_state( s_active );
        }
    }

    return ok;
}

//---------------------------------------------------------------------File_based::assert_is_active

void File_based::assert_is_active()
{
    if( _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-158", obj_name(), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-158", obj_name() );
    }
}

//-------------------------------------------------------------------------------File_based::remove

bool File_based::remove()
{
    bool is_removable = prepare_to_remove();

    if( is_removable )  typed_folder()->remove_file_based( this );
                  else  log()->info( message_string( "SCHEDULER-989", subsystem()->object_type_name() ) );

    return is_removable;
}

//-------------------------------------------------------------------------File_based::replace_with

bool File_based::replace_with( File_based* file_based_replacement )
{
    set_replacement( file_based_replacement );
    
    bool can_be_replaced_now = prepare_to_replace();

    if( can_be_replaced_now )  replace_now();
                         else  log()->info( message_string( "SCHEDULER-888", subsystem()->object_type_name() ) );

    return can_be_replaced_now;
}

//------------------------------------------------------File_based::check_for_replacing_or_removing

bool File_based::check_for_replacing_or_removing()
{
    bool result = false;

    if( can_be_replaced_now() )
    {
        log()->info( message_string( "SCHEDULER-936", subsystem()->object_type_name() ) );
        replace_now();
        result = true;
    }
    else
    if( can_be_removed_now() )
    {
        log()->info( message_string( "SCHEDULER-937", subsystem()->object_type_name() ) );
        typed_folder()->remove_file_based( this );
        result = true;
    }

    return result;
}

//----------------------------------------------------------------File_based::file_based_state_name

string File_based::file_based_state_name( State state )
{
    switch( state )
    {
        case s_not_initialized: return "not_initialized";
        case s_initialized:     return "initialized";
        case s_loaded:          return "loaded";
        case s_active:          return "active";
        case s_closed:          return "closed";
        case s_error:           return "error";
        default:                return S() << "File_based_state-" << state;
    }
}

//--------------------------------------------------------------------------File_based::dom_element

xml::Element_ptr File_based::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr element;   

    element = document.createElement( "file_based" );

    if( has_base_file() )
    {
        element.setAttribute_optional( "filename", _base_file_info._filename );
        if( _file_is_removed )  element.setAttribute( "removed", "yes" );

        Time t;
        t.set_utc( _base_file_info._timestamp_utc );
        element.setAttribute( "last_write_time", t.xml_value() );

        element.setAttribute( "state", file_based_state_name() );

        if( base_file_has_error() )  element.appendChild( create_error_element( document, _base_file_xc, (time_t)_base_file_xc_time ) );
    }

    return element;
}

//----------------------------------------------------------------------File_based::normalized_name

string File_based::normalized_name() const
{ 
    return _file_based_subsystem->normalized_name( _name ); 
}

//----------------------------------------------------------------------File_based::normalized_path

string File_based::normalized_path() const
{ 
    return _file_based_subsystem->normalized_name( path() ); 
}

//-----------------------------------------------------------------------------File_based::obj_name

string File_based::obj_name() const
{ 
    S result;
    
    result << _file_based_subsystem->object_type_name();
    result << " ";
    result << path(); 

    return result;
}

//-----------------------------------------------------------------------------File_based::set_name
    
void File_based::set_name( const string& name )
{
    _spooler->check_name( name );

    if( normalized_name() != _file_based_subsystem->normalized_name( name ) )
    {
        if( is_in_folder() )  z::throw_xc( "SCHEDULER-429", obj_name(), name );       // Name darf nicht geändert werden, außer Großschreibung
    }

    _name = name;

    log()->set_prefix( obj_name() );
}

//-------------------------------------------------------------------------------File_based::folder
    
Folder* File_based::folder() const
{ 
    if( !_typed_folder )  z::throw_xc( __FUNCTION__, "no folder" );

    return _typed_folder->folder(); 
}

//-------------------------------------------------------Order_folder::job_chain_name_from_filename

//string Order_folder::job_chain_name_from_filename( const string& filename ) const
//{
//    size_t separator = filename.find( job_chain_order_separator );
//    if( separator == string::npos )  separator = 0;
//
//    return filename.substr( 0, separator );
//}
//
//-------------------------------------------------------------Order_folder::order_id_from_filename
//
//string Order_folder::order_id_from_filename( const string& filename ) const
//{
//    string object_name = Folder::object_name_of_filename( filename );
//
//    size_t separator = object_name.find( job_chain_order_separator );
//    if( separator == string::npos )  separator = object_name.length();
//
//    return object_name.substr( separator );
//}

//-------------------------------------------------------------------------------------------------

} //namespace configuration
} //namespace scheduler
} //namespace sos
