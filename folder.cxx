// $Id: cluster.h 5126 2007-07-13 08:59:30Z jz $

#include "spooler.h"
#include "../zschimmer/directory_lister.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace zschimmer::file;

//--------------------------------------------------------------------------------------------const

const char                      folder_separator            = '/';
const int                       directory_watch_interval    = 10;

//---------------------------------------------------------------------------------------Path::Path

Path::Path( const string& folder_path, const string& tail_path ) 
{ 
    if( int len = folder_path.length() + tail_path.length() )
    {
        if( folder_path != ""  &&  *folder_path.rbegin() != folder_separator )
        {
            reserve( len + 1 );
            *this = folder_path;
            *this += folder_separator;
        }
        else
        {
            reserve( len );
            *this = folder_path;
        }

        if( tail_path != "" )
        {
            if( folder_path != ""  &&  *tail_path.begin() == folder_separator )  *this += tail_path.c_str() + 1;
                                                                           else  *this += tail_path;
        }
    }
}

//---------------------------------------------------------------------------Path::is_absolute_path

bool Path::is_absolute_path() const
{
    return length() >= 0  &&  (*this)[0] == folder_separator;
}

//-----------------------------------------------------------------------------------Path::set_name

void Path::set_name( const string& name )
{
    string f = folder_path();
    if( f != "" )  f += folder_separator;
    set_path( f + name );
}

//----------------------------------------------------------------------------Path::set_folder_path

void Path::set_folder_path( const string& folder_path )
{
    set_path( Path( folder_path, name() ) );
}

//------------------------------------------------------------------------Path::prepend_folder_path

void Path::prepend_folder_path( const string& folder_path )
{
    set_path( Path( folder_path, to_string() ) );
}

//--------------------------------------------------------------------------------Path::folder_path

Path Path::folder_path() const
{
    const char* p0 = c_str();
    const char* p  = p0 + length();

    if( p > p0 )
    {
        while( p > p0  &&  p[-1] != folder_separator )  p--;
        if( p > p0+1  &&  p[-1] == folder_separator )  p--;
    }

    return substr( 0, p - c_str() );
}

//---------------------------------------------------------------------------------------Path::name

string Path::name() const
{
    const char* p0 = c_str();
    const char* p  = p0 + length();

    while( p > p0  &&  p[-1] != folder_separator )  p--;

    return p;
}

//------------------------------------------------------------------------------------Path::compare

//int Path::compare( const Path& path ) const
//{
//    string path1 = normalized();
//    string path2 = path.normalized();
//    return strcmp( path1.c_str(), path2.c_str() );
//}

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

#   ifdef Z_WINDOWS
        Z_LOG2( "scheduler", "FindCloseChangeNotification()\n" );
        FindCloseChangeNotification( _directory_event._handle );
        _directory_event._handle = NULL;
#   endif
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
#   ifdef Z_WINDOWS

        //Z_LOG2( "scheduler", "FindFirstChangeNotification( \"" << _directory << "\", TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
        //HANDLE h = FindFirstChangeNotification( _directory.c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );
        //if( !h  ||  h == INVALID_HANDLE_VALUE )  throw_mswin( "FindFirstChangeNotification", _directory );

        //_directory_event._handle = h;
        //_directory_event.add_to( &_spooler->_wait_handles );
        //int WIE_WIRD_ASYNC_CONTINUE_GERUFEN;

        /*
            Neu: Event mit Async_operation koppeln.
            Event_async_operation : Async_operation
        */

#   endif

    set_async_manager( _spooler->_connection_manager );
    async_continue();  // IM FEHLERFALL trotzdem subsys_active setzen? Prüfen, ob Verzeichnis überhaupt vorhanden ist, sonst Abbruch. Oder warten, bis es da ist?

    _subsystem_state = subsys_active;
    return true;
}

//----------------------------------------------------------------Folder_subsystem::async_continue_

bool Folder_subsystem::async_continue_( Continue_flags )
{
    _directory_event.reset();
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

    _process_class_folder  = spooler()->process_class_subsystem ()->new_process_class_folder ( this );
    _lock_folder           = spooler()->lock_subsystem          ()->new_lock_folder          ( this );
    _job_folder            = spooler()->job_subsystem           ()->new_job_folder           ( this );
    _job_chain_folder      = spooler()->order_subsystem         ()->new_job_chain_folder     ( this );
    _standing_order_folder = spooler()->standing_order_subsystem()->new_standing_order_folder( this );

    add_to_typed_folder_map( _process_class_folder  );
    add_to_typed_folder_map( _lock_folder           );
    add_to_typed_folder_map( _job_folder            );
    add_to_typed_folder_map( _job_chain_folder      );
    add_to_typed_folder_map( _standing_order_folder );

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

//--------------------------------------------------------------------------------Folder::make_path

Path Folder::make_path( const string& name )
{
    return Path( path(), name );
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
    vector<File_based*>           ordered_file_baseds;    // Geordnete Liste der bereits bekannten (geladenen) Dateien

    ordered_file_infos.reserve( file_info_list.size() );
    Z_FOR_EACH_CONST( list<Base_file_info>, file_info_list, it )  ordered_file_infos.push_back( &*it );
    sort( ordered_file_infos.begin(), ordered_file_infos.end(), Base_file_info::less_dereferenced );

    // Mögliche Verbesserung: Nicht jedesmal ordnen. Nicht nötig, wenn Typed_folder nicht verändert worden ist.
    ordered_file_baseds.reserve( _file_based_map.size() );
    Z_FOR_EACH( File_based_map, _file_based_map, fb )  ordered_file_baseds.push_back( &*fb->second );
    sort( ordered_file_baseds.begin(), ordered_file_baseds.end(), File_based::less_dereferenced );


    vector<const Base_file_info*>::iterator fi = ordered_file_infos.begin();
    vector<File_based*          >::iterator fb = ordered_file_baseds.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen

    while( fi != ordered_file_infos.end()  ||
           fb != ordered_file_baseds.end() )
    {
        /// Dateinamen gleich Objektnamen?

        while( fb != ordered_file_baseds.end()  &&
               fi != ordered_file_infos.end()  &&
               (*fb)->_base_file_info._normalized_name == (*fi)->_normalized_name )
        {
            File_based* current_file_based = (*fb)->replacement()? (*fb)->replacement() 
                                                                 : (*fb);

            if( (*fb)->_file_is_removed  ||
                current_file_based->_base_file_info._timestamp_utc != (*fi)->_timestamp_utc )  
            {
                call_on_base_file_changed( *fb, *fi );
            }

            fi++, fb++;
        }



        /// Dateien hinzugefügt?

        while( fi != ordered_file_infos.end()  &&
               ( fb == ordered_file_baseds.end()  ||  (*fi)->_normalized_name < (*fb)->_base_file_info._normalized_name ) )
        {
            call_on_base_file_changed( (File_based*)NULL, (*fi) );
            fi++;
        }

        assert( fi == ordered_file_infos.end()  || 
                fb == ordered_file_baseds.end() ||
                (*fi)->_normalized_name >= (*fb)->_base_file_info._normalized_name );
        


        /// Dateien gelöscht?

        while( fb != ordered_file_baseds.end()  &&
               ( fi == ordered_file_infos.end()  ||  (*fi)->_normalized_name > (*fb)->_base_file_info._normalized_name ) )  // Datei entfernt?
        {
            if( !(*fb)->_file_is_removed )
            {
                call_on_base_file_changed( *fb, NULL );
            }
            fb++;
        }

        assert( fb == ordered_file_baseds.end()  ||
                fi == ordered_file_infos.end()  ||
                (*fi)->_normalized_name <= (*fb)->_base_file_info._normalized_name );
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
            if( old_file_based->has_base_file() )   // Nicht dateibasiertes Objekt, also aus anderer Quelle, nicht löschen
            {
                folder()->log()->info( message_string( "SCHEDULER-890", old_file_based->_base_file_info._filename, old_file_based->name() ) );
                file_based = old_file_based;                // Für catch()
                old_file_based->on_base_file_removed();
                file_based = NULL;
            }
        }
        else
        {
            string name = Folder::object_name_of_filename( base_file_info->_filename );

            file_based = subsystem()->call_new_file_based();
            file_based->set_name( name );
            file_based->set_base_file_info( *base_file_info );

            if( !old_file_based )  add_file_based( file_based );
            else
            if( file_based != old_file_based )  old_file_based->set_replacement( file_based );

            string relative_file_path = folder()->make_path( base_file_info->_filename );
            if( old_file_based )  old_file_based->log()->info( message_string( "SCHEDULER-892", relative_file_path, subsystem()->object_type_name() ) );
                            else  file_based    ->log()->info( message_string( "SCHEDULER-891", relative_file_path, subsystem()->object_type_name() ) );

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
                        file_based->switch_file_based_state( File_based::s_active );
                    }
                }
            }
            else  
            {
                file_based->switch_file_based_state( File_based::s_active );
            }
        }
    }
    catch( exception& x )
    {
        string msg;

        if( !file_based )  throw;   // Sollte nicht passieren

        if( base_file_info )    // Fehler beim Löschen soll das Objekt nicht als fehlerhaft markien
        {
            file_based->set_file_based_state( File_based::s_error );
            file_based->_base_file_xc      = x;
            file_based->_base_file_xc_time = double_from_gmtime();
            msg = message_string( "SCHEDULER-428", File_path( folder()->directory(), base_file_info->_filename ), x );
        }
        else
            msg = message_string( "SCHEDULER-439", File_path( folder()->directory(), file_based->base_file_info()._filename ), 
                                                   file_based->subsystem()->object_type_name(), x );

        file_based->log()->error( msg );

        if( _spooler->_mail_on_error )
        {
            Scheduler_event scheduler_event ( scheduler::evt_base_file_error, log_error, spooler() );
            scheduler_event.set_error( x );

            Mail_defaults mail_defaults( spooler() );
            mail_defaults.set( "subject", msg );
            mail_defaults.set( "body"   , msg );

            scheduler_event.send_mail( mail_defaults );
        }
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

    if( file_based_or_null( normalized_name ) )  z::throw_xc( "SCHEDULER-160", subsystem()->object_type_name(), file_based->path().to_string() );

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
    _wished_state = s_closed;
    set_file_based_state( s_closed );
}

//---------------------------------------------------------------------------File_based::initialize

bool File_based::initialize()
{
    _wished_state = s_initialized;
    return initialize2();
}

//--------------------------------------------------------------------------File_based::initialize2

bool File_based::initialize2()
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
    _wished_state = s_loaded;
    return load2();
}

//--------------------------------------------------------------------------------File_based::load2

bool File_based::load2()
{
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );

    bool ok = _state == s_loaded;
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
    _wished_state = s_active;
    return activate2();
}

//----------------------------------------------------------------------------File_based::activate2

bool File_based::activate2()
{
    bool ok = _state == s_active;
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
        _state = state; 

        Z_DEBUG_ONLY( log()->debug( message_string( "SCHEDULER-893", file_based_state_name() ) ); )
    }
}

//--------------------------------------------------------------File_based::try_set_to_wished_state

bool File_based::try_switch_wished_file_based_state()
{
    return switch_file_based_state( _wished_state );
}

//--------------------------------------------------------------File_based::switch_file_based_state

bool File_based::switch_file_based_state( State state )
{
    bool result = false;

    switch( state )
    {
        case s_initialized: result = initialize();  break;
        case s_loaded:      result = load();        break;
        case s_active:      result = activate();    break;
        case s_error:       result = true;  set_file_based_state( state );  break;
      //case s_incomplete:  result = true;  set_file_based_state( state );  break;
        case s_closed:      result = true;  close(); break;
        default:            assert(0);
    }

    return result;
}

//------------------------------------------------------------------File_based::on_dependant_loaded

bool File_based::on_dependant_loaded( File_based* )
{
    return try_switch_wished_file_based_state();
}

//-----------------------------------------------------------File_based::on_dependant_to_be_removed

bool File_based::on_dependant_to_be_removed( File_based* )
{
    int FEHLT_WAS; //??
    return true;
}

//-----------------------------------------------------------------File_based::on_dependant_removed

void File_based::on_dependant_removed( File_based* )
{
    check_for_replacing_or_removing();
}

//---------------------------------------------------------------------File_based::assert_is_loaded

void File_based::assert_is_loaded()
{
    if( _state != s_loaded  &&
        _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name() );
    }
}

//---------------------------------------------------------------------File_based::assert_is_active

void File_based::assert_is_active()
{
    if( _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-154", obj_name(), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-154", obj_name() );
    }
}

//--------------------------------------------------------------------File_based::prepare_to_remove

bool File_based::prepare_to_remove()
{ 
    subsystem()->dependencies()->prepare_to_remove( this ); 
    //Dependant::prepare_to_remove();

    return can_be_removed_now(); 
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
      //case s_incomplete:      return "incomplete";
        case s_error:           return "error";
        default:                return S() << "File_based_state-" << state;
    }
}

//--------------------------------------------------------------------------File_based::dom_element

xml::Element_ptr File_based::dom_element( const xml::Document_ptr& document, const Show_what& )
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

//---------------------------------------------------------------------------------File_based::path

Path File_based::path() const
{ 
    return _typed_folder? _typed_folder->folder()->make_path( _name ) 
                        : _name;    // Keine Exception auslösen
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

//-------------------------------------------------------File_based_subsystem::File_based_subsystem

File_based_subsystem::File_based_subsystem( Spooler* spooler, IUnknown* iunknown, Type_code type_code ) 
: 
    Subsystem( spooler, iunknown, type_code ),
    _dependencies( this )
{
}

//------------------------------------------------------------File_based_subsystem::normalized_path
    
Path File_based_subsystem::normalized_path( const Path& path ) const
{
    return Path( path.folder_path(), normalized_name( path.name() ) );
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
    catch( exception& x )  { Z_LOG( __FUNCTION__ << "  ERROR  " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------------Pendant::remove_dependants

void Pendant::remove_dependants()
{
    Z_FOR_EACH( Dependant_sets, _missing_sets, it )
    {
        File_based_subsystem* subsystem   = it->first;
        Dependant_set&          missing_set = it->second;

        Z_FOR_EACH( Dependant_set, missing_set, it2 )
        {
            subsystem->dependencies()->remove_dependant( this, *it2 );
        }
    }
}

//---------------------------------------------------------------------------Pendant::add_dependant

void Pendant::add_dependant( File_based_subsystem* subsystem, const Path& path )
{
    _missing_sets[ subsystem ].insert( subsystem->normalized_path( path ) );
    subsystem->dependencies()->add_dependant( this, path );
}

//------------------------------------------------------------------------Pendant::remove_dependant

void Pendant::remove_dependant( File_based_subsystem* subsystem, const Path& path )
{
    _missing_sets[ subsystem ].erase( subsystem->normalized_path( path ) );
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

void Dependencies::add_dependant( Pendant* requestor, const string& missings_path )
{
    //Z_DEBUG_ONLY( _subsystem->log()->info( S() << __FUNCTION__ << " " << requestor->obj_name() << " " << quoted_string( missings_path ) ); )
    _path_requestors_map[ _subsystem->normalized_path( missings_path ) ].insert( requestor );
}

//-------------------------------------------------------------------Dependencies::remove_dependant

void Dependencies::remove_dependant( Pendant* requestor, const string& missings_path )
{
    //Z_DEBUG_ONLY( _subsystem->log()->info( S() << __FUNCTION__ << " " << requestor->obj_name() << " " << quoted_string( missings_path ) ); )

    Requestor_set&  requestors_set = _path_requestors_map[ missings_path ];
    
    requestors_set.erase( requestor );
    if( requestors_set.empty() )  _path_requestors_map.erase( _subsystem->normalized_path( missings_path ) );
}

//-------------------------------------------------------------------Dependencies::remove_requestor

void Dependencies::remove_requestor( Pendant* requestor )
{
    Z_FOR_EACH( Path_requestors_map, _path_requestors_map, path_requestors_it )
    {
        Requestor_set& requestor_set = path_requestors_it->second;

        Requestor_set::iterator requestor_it = requestor_set.find( requestor );
        assert( requestor_it == requestor_set.end() );
        //{
        //    Z_DEBUG_ONLY( log()->warn( S() << __FUNCTION__ << "  " << requestor->obj_name() << 
        //                                                      " " << subsystem_it->first->obj_name() << 
        //                                                      " " << path_requestors_it->first ) );
        //    assert(0);
            requestor_set.erase( requestor_it );
        //}
    }
}

//------------------------------------------------------Dependencies::announce_dependant_loaded

void Dependencies::announce_dependant_loaded( File_based* found_missing )
{
    //Z_DEBUG_ONLY( _subsystem->log()->info( S() << __FUNCTION__ << " " << found_missing->obj_name() ); )

    assert( found_missing->subsystem() == _subsystem );
    //assert( found_missing->file_based_state() == File_based::s_active );

    Path_requestors_map::iterator it = _path_requestors_map.find( found_missing->normalized_path() );

    if( it != _path_requestors_map.end() )
    {
        Requestor_set& requestor_set = it->second;

        //for( Requestor_set::iterator it2 = requestor_set.begin(); it2 != requestor_set.end(); )
        Z_FOR_EACH( Requestor_set, requestor_set, it2 )
        {
            Requestor_set::iterator next_it2  = it2;  next_it2++;
            Pendant*     requestor = *it2;
        
            Z_DEBUG_ONLY( _subsystem->log()->info( S() << "*** " << requestor->obj_name() << " on_dependant_loaded( " << found_missing->obj_name() << " ) " ); )
            bool ok = requestor->on_dependant_loaded( found_missing );
            //if( ok )  
            //{
            //    it = _path_requestors_map.find( found_missing->normalized_path() );
            //    if( it != _path_requestors_map.end()    // requestor_set ist noch da?
            //    {
            //        assert( it == &requestor_set );
            //        requestor_set.erase( it2 );
            //    }
            //}

            //it2 = next_it2;
        }
    }
}

//------------------------------------------------------------------Dependencies::prepare_to_remove

bool Dependencies::prepare_to_remove( File_based* to_be_removed )
{
    assert( to_be_removed->subsystem() == _subsystem );
  //assert( to_be_removed->file_based_state() == File_based::s_active );

    bool result = true;

    Path_requestors_map::iterator it = _path_requestors_map.find( to_be_removed->normalized_path() );

    if( it != _path_requestors_map.end() )
    {
        Requestor_set& requestor_set = it->second;

        //for( Requestor_set::iterator it2 = requestor_set.begin(); it2 != requestor_set.end(); )
        Z_FOR_EACH( Requestor_set, requestor_set, it2 )
        {
            Requestor_set::iterator next_it2  = it2;  next_it2++;
            Pendant*     requestor = *it2;
        
            Z_DEBUG_ONLY( _subsystem->log()->info( S() << "    " << requestor->obj_name() << " on_dependant_to_be_removed( " << to_be_removed->obj_name() << " ) " ); )
            result = requestor->on_dependant_to_be_removed( to_be_removed );
            if( !result )  break;
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace configuration
} //namespace scheduler
} //namespace sos
