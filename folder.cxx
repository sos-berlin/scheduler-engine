// $Id: cluster.h 5126 2007-07-13 08:59:30Z jz $

#include "spooler.h"
#include "../zschimmer/directory_lister.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace zschimmer::file;

//--------------------------------------------------------------------------------------------const

const char                      folder_separator            = '/';
const Absolute_path             root_path                   ( "/" );
const int                       file_timestamp_delay        = 2+1;      // FAT-Zeitstempel sind 2 Sekunden genau
#ifdef Z_WINDOWS
    const int                   directory_watch_interval_min = 60;
    const int                   directory_watch_interval_max = 60;
#else
    const int                   directory_watch_interval_min =  5;
    const int                   directory_watch_interval_max = 60;
#endif

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

//-------------------------------------------------------------------------------Path::set_absolute

void Path::set_absolute( const Absolute_path& absolute_base, const Path& relative )
{
    assert( !absolute_base.empty() );
    if( absolute_base.empty() )  assert(0), z::throw_xc( Z_FUNCTION, relative );

    if( relative.empty() )
    {
        set_path( "" );
    }
    else
    {
        if( relative.is_absolute_path() )
        {
            set_path( relative );
        }
        else
        {
            set_path( Path( absolute_base, relative ) );
        }
    }
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

//--------------------------------------------------------------------------------Path::to_filename

string Path::to_filename() const
{
    string result = to_string();

    if( string_begins_with( result, "/" ) )  result.erase( 0, 1 );

    for( int i = 0; i < result.length(); i++ ) 
    {
        if( result[i] == '/' )  result[i] = ',';

        if( strchr(     "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
                    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
                    "<>:\"/\\|",
                    result[i] ) )  result[i] = '_';
    }

    return result;
}

//---------------------------------------------------------------------Absolute_path::Absolute_path

Absolute_path::Absolute_path( const Path& path )
{
    assert( path.empty()  ||  path.is_absolute_path() );
    if( !path.empty()  &&  !path.is_absolute_path() )  assert(0), z::throw_xc( Z_FUNCTION, path );

    set_path( path );
}

//------------------------------------------------------------------------Absolute_path::with_slash

string Absolute_path::with_slash() const
{
    assert( empty() || is_absolute_path() );
    return to_string();
}

//---------------------------------------------------------------------Absolute_path::without_slash

string Absolute_path::without_slash() const
{
    if( empty() )
    {
        return "";
    }
    else
    {
        assert( string_begins_with( to_string(), "/" ) );
        return to_string().substr( 1 );
    }
}

//--------------------------------------------------------------------------Absolute_path::set_path

void Absolute_path::set_path( const string& path )
{ 
    Path::set_path( path ); 
    assert( empty() || is_absolute_path() );
}

//---------------------------------------------------------------Folder_subsystem::Folder_subsystem

Folder_subsystem::Folder_subsystem( Scheduler* scheduler )
:
    file_based_subsystem<Folder>( scheduler, this, type_folder_subsystem ),
    _zero_(this+1),
    _directory_watch_interval( directory_watch_interval_max )
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
    _subsystem_state = subsys_stopped;
    set_async_manager( NULL );


#   ifdef Z_WINDOWS
        Z_LOG2( "scheduler", "FindCloseChangeNotification()\n" );
        FindCloseChangeNotification( _directory_event._handle );
        _directory_event._handle = NULL;
#   endif

    if( _root_folder )  
    {
        //typed_folder<>::_file_based_map hat keine ptr<>!  Zeiger können also ungültig sein:  _root_folder->remove_all_file_baseds();
        //typed_folder<>::_file_based_map hat keine ptr<>!  Zeiger können also ungültig sein:  remove_file_based( _root_folder );
        _root_folder = false;
    }
}

//------------------------------------------------------------------Folder_subsystem::set_directory

void Folder_subsystem::set_directory( const File_path& directory )
{
    assert_subsystem_state( subsys_initialized, Z_FUNCTION );
    _directory = directory;
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
    return true;
}

//-------------------------------------------------------------Folder_subsystem::subsystem_activate

bool Folder_subsystem::subsystem_activate()
{
    bool result = false;

    if( _directory.exists() )
    {
#       ifdef Z_WINDOWS

            Z_LOG2( "scheduler", "FindFirstChangeNotification( \"" << _directory << "\", TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
            
            HANDLE h = FindFirstChangeNotification( _directory.c_str(), 
                                                    TRUE,                               // Mit Unterverzeichnissen
                                                    FILE_NOTIFY_CHANGE_FILE_NAME  |  
                                                    FILE_NOTIFY_CHANGE_DIR_NAME   |
                                                    FILE_NOTIFY_CHANGE_LAST_WRITE );

            if( !h  ||  h == INVALID_HANDLE_VALUE )  throw_mswin( "FindFirstChangeNotification", _directory );

            _directory_event._handle = h;
            _directory_event.add_to( &_spooler->_wait_handles );

#       endif

        set_async_manager( _spooler->_connection_manager );
        _subsystem_state = subsys_active;

        _root_folder->activate();
        async_continue();  // IM FEHLERFALL trotzdem subsys_active setzen? Prüfen, ob Verzeichnis überhaupt vorhanden ist, sonst Abbruch. Oder warten, bis es da ist?

        result = true;
    }
    else
        log()->warn( message_string( "SCHEDULER-895", _directory ) );


    return result;
}

//-----------------------------------------------------------------Folder_subsystem::new_file_based

ptr<Folder> Folder_subsystem::new_file_based()
{
    assert(0);
    zschimmer::throw_xc( Z_FUNCTION );    // Subfolder_folder::on_base_file_changed() legt selbst Folder an
}

//----------------------------------------------------------------Folder_subsystem::async_continue_

bool Folder_subsystem::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler", "Prüfe Konfigurationsverzeichnis " << _directory << "\n" );

    _directory_event.reset();

#   ifdef Z_WINDOWS
        //Z_LOG2( "scheduler", "FindNextChangeNotification(\"" << _directory << "\")\n" );
        BOOL ok = FindNextChangeNotification( _directory_event );
        if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
#   endif

    
    double now = double_from_gmtime();
    if( _read_again_at < now )  _read_again_at = 0;     // Verstrichen?

    bool something_changed = _root_folder->adjust_with_directory( now );
    
    if( something_changed )  _last_change_at = now;

    _directory_watch_interval = now - _last_change_at < directory_watch_interval_max? directory_watch_interval_min
                                                                                    : directory_watch_interval_max;
    //_directory_watch_interval = something_changed? directory_watch_interval_min
    //                                             : min( directory_watch_interval_max, ( _directory_watch_interval + 1 ) + ( _directory_watch_interval / 10 ) );

    if( _read_again_at )  set_async_next_gmtime( _read_again_at );
                    else  set_async_delay( _directory_watch_interval );

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

Folder::Folder( Folder_subsystem* folder_subsystem, Folder* parent )
:
    file_based< Folder, Subfolder_folder, Folder_subsystem >( folder_subsystem, this, type_folder ),
    _parent(parent),
    _zero_(this+1)
{
    _process_class_folder  = spooler()->process_class_subsystem ()->new_process_class_folder ( this );
    _lock_folder           = spooler()->lock_subsystem          ()->new_lock_folder          ( this );
    _job_folder            = spooler()->job_subsystem           ()->new_job_folder           ( this );
    _job_chain_folder      = spooler()->order_subsystem         ()->new_job_chain_folder     ( this );
    _standing_order_folder = spooler()->standing_order_subsystem()->new_standing_order_folder( this );
    _subfolder_folder      = spooler()->folder_subsystem        ()->new_subfolder_folder     ( this );

    add_to_typed_folder_map( _process_class_folder  );
    add_to_typed_folder_map( _lock_folder           );
    add_to_typed_folder_map( _job_folder            );
    add_to_typed_folder_map( _job_chain_folder      );
    add_to_typed_folder_map( _standing_order_folder );
    add_to_typed_folder_map( _subfolder_folder      );

    _log->set_prefix( obj_name() );     // Noch ohne Pfad
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

//------------------------------------------------------------------------------------Folder::close

void Folder::close()
{
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

//----------------------------------------------------------------------------Folder::on_initialize

bool Folder::on_initialize()
{
    return true;
}

//----------------------------------------------------------------------------------Folder::on_load

bool Folder::on_load()
{
    _directory = _parent? File_path( _parent->_directory, name() )
                        : subsystem()->directory();

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

//--------------------------------------------------------------------------------Folder::make_path

Absolute_path Folder::make_path( const string& name )
{
    return Absolute_path( path(), name );
}

//--------------------------------------------------------------------Folder::adjust_with_directory

bool Folder::adjust_with_directory( double now )
{
    bool something_changed = false;

    typedef stdext::hash_map< Typed_folder*, list<Base_file_info> >   File_list_map;
    
    File_list_map file_list_map;

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )  file_list_map[ it->second ] = list<Base_file_info>();

    try
    {
        // DATEINAMEN EINSAMMELN

        try
        {
            string last_normalized_name;
            string last_extension;
            string last_filename;

            for( file::Directory_lister dir ( _directory );; )
            {
                ptr<file::File_info> file_info   = dir.get();
                if( !file_info )  break;

#               ifdef Z_UNIX
                    bool file_exists = file_info->try_call_stat();
#                else
                    bool file_exists = true;        // Unter Windows hat File_info schon alle Informationen, kein stat() erforderlich
#               endif

                if( file_exists )
                {
                    string        filename     = file_info->path().name();
                    string        name;
                    string        extension    = extension_of_filename( filename );
                    string        normalized_extension = lcase( extension );
                    Typed_folder* typed_folder = NULL;

                    if( file_info->is_directory() )
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
                        string normalized_name = typed_folder->subsystem()->normalized_name( name );

                        if( normalized_name == last_normalized_name  &&
                            extension       == last_extension           )
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
                            file_list_map[ typed_folder ].push_back( Base_file_info( filename, (double)file_info->last_write_time(), normalized_name, now ) );
                        }

                        last_normalized_name = normalized_name;
                        last_extension       = normalized_extension;
                        last_filename        = filename;
                    }
                }
            }
        }
        catch( exception& x )
        {
            if( _directory.exists() )  throw;                                   // Problem beim Lesen des Verzeichnisses
            
            if( !_parent )  _log->error( message_string( "SCHEDULER-882", _directory, x ) );    // Jemand hat das Verzeichnis entfernt

            // Die Objekte dieses Ordners werden von adjust_with_directory() gelöscht, denn file_list_map enthält leere File_info_list.
        }


        Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
        {
            Typed_folder* typed_folder = it->second;

            something_changed |= typed_folder->adjust_with_directory( file_list_map[ typed_folder ], now );
        }
    }
    catch( exception& x ) 
    {
        _log->error( message_string( "SCHEDULER-431", x ) );
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
    //if( path_without_slash() != "" )  
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

bool Subfolder_folder::on_base_file_changed( File_based* file_based, const Base_file_info* base_file_info, double now )
{
    bool    something_changed = false;
    Folder* subfolder         = static_cast<Folder*>( file_based );

    if( !subfolder )
    {
        ptr<Folder> new_subfolder = Z_NEW( Folder( subsystem(), folder() ) );
        new_subfolder->set_folder_path( folder()->path() );
        new_subfolder->set_name( base_file_info->_normalized_name );
        new_subfolder->set_base_file_info( *base_file_info );
        add_file_based( new_subfolder );
        something_changed = true;

        bool ok = new_subfolder->activate();
        if( ok )  new_subfolder->adjust_with_directory( now );
    }
    else
    if( base_file_info )
    {
        subfolder->set_base_file_info( *base_file_info );
        something_changed = subfolder->adjust_with_directory( now );
    }
    else
    if( !subfolder->is_to_be_removed() ) 
    {
        subfolder->remove();
        something_changed = true;
    }

    return something_changed;
}

//-------------------------------------------------------------------Folder::remove_all_file_baseds

void Folder::remove_all_file_baseds()
{
    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
    {
        Typed_folder* typed_folder = it->second;
        typed_folder->remove_all_file_baseds();
    }
}

//------------------------------------------------------------------------Folder::prepare_to_remove

bool Folder::prepare_to_remove()
{
    remove_all_file_baseds();
    return can_be_removed_now();
}

//-----------------------------------------------------------------------Folder::can_be_removed_now

bool Folder::can_be_removed_now()
{
    bool result = true;

    Z_FOR_EACH( Typed_folder_map, _typed_folder_map, it )
    {
        Typed_folder* typed_folder = it->second;

        result |= typed_folder->is_empty();
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
    _log->set_prefix( obj_name() );     // Noch ohne Pfad
}

//--------------------------------------------------------------Typed_folder::adjust_with_directory
    
bool Typed_folder::adjust_with_directory( const list<Base_file_info>& file_info_list, double now )
{
    bool                          something_changed  = false;
    vector<const Base_file_info*> ordered_file_infos;     // Geordnete Liste der vorgefundenen Dateien    
    vector<File_based*>           ordered_file_baseds;    // Geordnete Liste der bereits bekannten (geladenen) Dateien

    ordered_file_infos.reserve( file_info_list.size() );
    Z_FOR_EACH_CONST( list<Base_file_info>, file_info_list, it )  ordered_file_infos.push_back( &*it );
    sort( ordered_file_infos.begin(), ordered_file_infos.end(), Base_file_info::less_dereferenced );

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
            something_changed |= on_base_file_changed( *fb, *fi, now );
            fi++, fb++;
        }



        /// Dateien hinzugefügt?

        while( fi != ordered_file_infos.end()  &&
               ( fb == ordered_file_baseds.end()  ||  (*fi)->_normalized_name < (*fb)->_base_file_info._normalized_name ) )
        {
            something_changed |= on_base_file_changed( (File_based*)NULL, (*fi), now );
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
                something_changed |= on_base_file_changed( *fb, NULL, now );
            }
            fb++;
        }

        assert( fb == ordered_file_baseds.end()  ||
                fi == ordered_file_infos.end()  ||
                (*fi)->_normalized_name <= (*fb)->_base_file_info._normalized_name );
    }

    return something_changed;
}

//---------------------------------------------------------------Typed_folder::on_base_file_changed

bool Typed_folder::on_base_file_changed( File_based* old_file_based, const Base_file_info* base_file_info, double now )
{
    bool            something_changed  = false;
    ptr<File_based> file_based         = NULL;
    File_based*     current_file_based = old_file_based;        // File_based der zuletzt gelesenen Datei
    bool            is_new             = !old_file_based  ||    
                                         old_file_based->_file_is_removed;     // Datei ist wieder aufgetaucht?
    File_path       file_path;

    if( old_file_based )  
    {
        old_file_based->_file_is_removed = base_file_info == NULL;
        old_file_based->_remove_xc       = zschimmer::Xc();
        if( old_file_based->replacement() )  current_file_based = old_file_based->replacement();   // File_based der zuletzt geladenen Datei
    }


    try
    {
        if( base_file_info )
        {
            file_path = File_path( folder()->directory(), base_file_info->_filename );

            bool timestamp_changed = current_file_based  &&
                                     current_file_based->_base_file_info._timestamp_utc != base_file_info->_timestamp_utc;

            //if( current_file_based )
            //{
            //    Z_LOG2( "scheduler", Time().set_utc(current_file_based->_base_file_info._info_timestamp+file_timestamp_delay ).as_string() << " " <<
            //                         Time().set_utc(now).as_string() << " " << file_path << "\n" );
            //}

            bool read_again = !timestamp_changed  &&  
                              current_file_based  &&
                              current_file_based->_read_again  &&
                              current_file_based->_base_file_info._info_timestamp + file_timestamp_delay <= now;    // Falls Datei geändert, aber Zeitstempel nicht

            if( is_new  ||  timestamp_changed  ||  read_again )
            {
                string content;
                Md5    content_md5;
                z::Xc  content_xc;
                
                try
                {
                    content = string_from_file( file_path );
                    content_md5 = md5( content );
                }
                catch( exception& x ) { content_xc = x; }

                if( content_xc.code() == ( S() << "ERRNO-" << ENOENT ).to_string() )    // ERRNO-2 (Datei gelöscht)?
                {
                    if( old_file_based )  old_file_based->remove(),  old_file_based = NULL;
                }
                else
                if( read_again  &&                                              // Inhalt nach file_timestamp_delay nochmal prüfen?
                    content_md5 == current_file_based->_md5  &&                 // Inhalt hat sich nicht geändert?
                    !current_file_based->_error_ignored )   
                {
                    current_file_based->_read_again = false;
                }
                else
                {   
                    string rel_path = folder()->make_path( base_file_info->_filename );

                    something_changed = true;


                    file_based = subsystem()->call_new_file_based();
                    file_based->_read_again = !current_file_based  ||  current_file_based->base_file_info()._timestamp_utc != base_file_info->_timestamp_utc;
                    file_based->_md5        = content_md5;
                    file_based->set_base_file_info( *base_file_info );
                    file_based->set_folder_path( folder()->path() );
                    file_based->set_name( Folder::object_name_of_filename( base_file_info->_filename ) );
                    
                    if( old_file_based ) 
                    {
                        old_file_based->log()->info( message_string( "SCHEDULER-892", rel_path, 
                                                                                      Time().set_utc( base_file_info->_timestamp_utc ).as_string(), 
                                                                                      subsystem()->object_type_name() ) );
                        old_file_based->set_replacement( file_based );
                        current_file_based = NULL;
                    }
                    else
                    {
                        file_based->log()->info( message_string( "SCHEDULER-891", rel_path, 
                                                                                  Time().set_utc( base_file_info->_timestamp_utc ).as_string(), 
                                                                                  subsystem()->object_type_name() ) );
                        add_file_based( file_based );
                    }


                    if( !content_xc.is_empty() )  throw content_xc;


                    xml::Document_ptr dom_document ( content );
                    xml::Element_ptr  element      = dom_document.documentElement();
                    subsystem()->assert_xml_element_name( element );      //Weil Püschel auch <add_order> haben will.  if( !dom_document.documentElement().nodeName_is( subsystem()->xml_element_name() ) )  z::throw_xc( "SCHEDULER-409", subsystem()->xml_element_name(), dom_document.documentElement().nodeName() );
                    if( spooler()->_validate_xml )  spooler()->_schema.validate( dom_document );

                    assert_empty_attribute( element, "spooler_id" );
                    if( !element.bool_getAttribute( "replace", true ) )  z::throw_xc( "SCHEDULER-232", element.nodeName(), "replace", element.getAttribute( "replace" ) );

                    Z_LOG2( "scheduler", file_path << ":\n" << content << "\n" );

                    file_based->set_dom( dom_document.documentElement() );
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
        else                                    // Datei ist gelöscht
        if( old_file_based->has_base_file() )   // Nicht dateibasiertes Objekt, also aus anderer Quelle, nicht löschen
        {
            something_changed = true;

            string p = folder()->make_path( old_file_based->base_file_info()._filename );
            old_file_based->log()->info( message_string( "SCHEDULER-890", p, subsystem()->object_type_name() ) );

            file_based = old_file_based;                // Für catch()
            file_based->remove();
            file_based = NULL;
        }
    }
    catch( exception& x )
    {
        if( !file_based )  throw;   // Sollte nicht passieren

        file_based->_error_ignored = file_based->_read_again  &&  file_based->_state < File_based::s_initialized;   // Wegen langsam schreibender Editoren

        Log_level log_level = file_based->_error_ignored? log_info : log_error;
        string    msg;


        if( base_file_info )        // Fehler beim Löschen soll das Objekt nicht als fehlerhaft markieren
        {
            file_based->_base_file_xc      = x;
            file_based->_base_file_xc_time = double_from_gmtime();

            msg = message_string( "SCHEDULER-428", File_path( folder()->directory(), base_file_info->_filename ), 
                                                   Time().set_utc( base_file_info->_timestamp_utc ).as_string(), 
                                                   x );
        }
        else
        {
            msg = message_string( "SCHEDULER-439", File_path( folder()->directory(), file_based->base_file_info()._filename ), 
                                                   file_based->subsystem()->object_type_name(), x );
        }

        file_based->log()->log( log_level, msg );

        if( msg != ""  &&  _spooler->_mail_on_error  &&  log_level > log_info )
        {
            Scheduler_event scheduler_event ( scheduler::evt_base_file_error, log_error, spooler() );
            scheduler_event.set_error( x );

            Mail_defaults mail_defaults( spooler() );
            mail_defaults.set( "subject", msg );
            mail_defaults.set( "body"   , msg );

            scheduler_event.send_mail( mail_defaults );
        }
    }


    if( file_based  &&  file_based->_read_again )  
    {
        double at = now + file_timestamp_delay;
        folder()->subsystem()->set_read_again_at( at );  
        file_based->log()->debug( message_string( "SCHEDULER-896", file_timestamp_delay, Time().set_utc( at ).as_string() ) );
    }


    return something_changed;
}

//-----------------------------------------------------Typed_folder::new_initialized_file_based_xml

ptr<File_based> Typed_folder::new_initialized_file_based_xml( const xml::Element_ptr& element )
{
    subsystem()->check_file_based_element( element );
    //assert_empty_attribute( element, "replace"    );

    ptr<File_based> file_based = subsystem()->call_new_file_based();
    file_based->set_folder_path( folder()->path() );
    file_based->set_name( element.getAttribute( "name" ) );
    file_based->set_dom( element );
    file_based->initialize();

    return file_based;
}

//-----------------------------------------------------------------Typed_folder::add_file_based_xml

void Typed_folder::add_file_based_xml( const xml::Element_ptr& element )
{
    ptr<File_based> file_based = new_initialized_file_based_xml( element );
    add_file_based( file_based );
    file_based->activate();
}

//------------------------------------------------------Typed_folder::add_or_replace_file_based_xml

void Typed_folder::add_or_replace_file_based_xml( const xml::Element_ptr& element )
{
    subsystem()->check_file_based_element( element );

    if( ptr<File_based> file_based = file_based_or_null( element.getAttribute( "name" ) ) )
    {
        bool replace_yes        =  element.bool_getAttribute( "replace", false );                   // replace="yes"
        bool replace_no         = !element.bool_getAttribute( "replace", true  );                   // replace="no"
        bool use_base_mechanism = file_based_subsystem()->subsystem_state() <= subsys_initialized;  // Wird noch die Scheduler-Konfigurationsdatei geladen?

        //if( replace_no )  z::throw_xc( "SCHEDULER-441", obj_name() );   // replace="no" und Objekt ist bekannt

        if( replace_no  ||  
            use_base_mechanism  &&  !replace_yes )
        {
            file_based->set_dom( element );     // Objekt ergänzen (<base>) oder ändern. Evtl. Exception, wenn Objekt das nicht kann, z.B. <job>
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

        add_file_based_xml( element );
    }
}

//------------------------------------------------Typed_folder::add_to_replace_or_remove_candidates

void Typed_folder::add_to_replace_or_remove_candidates( const string& name )             
{ 
    _replace_or_remove_candidates_set.insert( name ); 
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

    if( file_based_or_null( normalized_name ) )  z::throw_xc( "SCHEDULER-160", subsystem()->object_type_name(), file_based->path().to_string() );

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

void Typed_folder::remove_all_file_baseds()
{
    for( File_based_map::iterator it = _file_based_map.begin(); it != _file_based_map.end(); )
    {
        File_based_map::iterator next_it = it;  next_it++;

        File_based* file_based = it->second;
        file_based->remove();

        it = next_it;
    }
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
        //if( _folder->path_without_slash() != "" )  
            result << " " << _folder->path();
        //                                     else  result << " /";
    }

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
        _state = state; 

        log()->log( _state == s_active? log_info : log_debug9,
                    message_string( "SCHEDULER-893", subsystem()->object_type_name(), file_based_state_name() ) );
    }
}

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
        case s_not_initialized: break;
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
    return activate();
    //return try_switch_wished_file_based_state();
}

//-----------------------------------------------------------File_based::on_dependant_to_be_removed

bool File_based::on_dependant_to_be_removed( File_based* )
{
    return true;
}

//-----------------------------------------------------------------File_based::on_dependant_removed

void File_based::on_dependant_removed( File_based* file_based )
{
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

    _replacement = replacement; 
    if( _replacement )  set_to_be_removed( false ); 
}

//--------------------------------------------------------------------File_based::prepare_to_remove

bool File_based::prepare_to_remove()
{ 
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );

    set_to_be_removed( true );
    subsystem()->dependencies()->announce_dependant_to_be_removed( this ); 
    return can_be_removed_now(); 
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
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );


    if( remove_flag == rm_base_file_too  &&  has_base_file() )
    {
        remove_base_file();
    }

    bool is_removable = prepare_to_remove();

    if( is_removable )  
    {
        _remove_xc = zschimmer::Xc();

        remove_now();
    }
    else  
    {
        _remove_xc = remove_error();
        log()->info( _remove_xc.what() );   // Kein Fehler, Löschen ist nur verzögert
    }

    return is_removable;
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

    File_path file_path ( folder()->directory(), base_file_info()._filename );

    try
    {
#       ifdef Z_DEBUG
            file_path.move_to( file_path + "-REMOVED" );
#        else
            file_path.unlink();
#       endif    
    }
    catch( exception& )
    {
        if( file_path.exists() )  throw;
        _file_is_removed = true;
    }
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
                                        else  typed_folder()->add_to_replace_or_remove_candidates( name() );
            }
        }
        else
        if( is_to_be_removed()  &&  can_be_removed_now()   )
        {
            if( when_to_act == act_now )  remove_now();
                                    else  typed_folder()->add_to_replace_or_remove_candidates( name() );
        }
    }
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
        case s_not_initialized: return "not_initialized";
        case s_initialized:     return "initialized";
        case s_loaded:          return "loaded";
        case s_active:          return "active";
        case s_closed:          return "closed";
      //case s_error:           return "error";
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
        element.setAttribute_optional( "filename", _base_file_info._filename );
        if( _file_is_removed )  element.setAttribute( "removed", "yes" );

        Time t;
        t.set_utc( _base_file_info._timestamp_utc );
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
        if( is_in_folder() )  z::throw_xc( "SCHEDULER-429", obj_name(), name );       // Name darf nicht geändert werden, außer Großschreibung
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
    if( !_typed_folder )  assert(0), z::throw_xc( Z_FUNCTION, "no folder" );

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
    
string File_based_subsystem::normalized_path( const Path& path ) const
{
    return Path( spooler()->folder_subsystem()->normalized_name( path.folder_path() ), 
                 normalized_name( path.name() ) );
}

//---------------------------------------------------File_based_subsystem::check_file_based_element

void File_based_subsystem::check_file_based_element( const xml::Element_ptr& element )
{
    if( !element.nodeName_is( xml_element_name() ) )  z::throw_xc( "SCHEDULER-409", xml_element_name(), element.nodeName() );

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
        
            pendant->on_dependant_loaded( found_missing );
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

//-------------------------------------------------------------------------------------------------

} //namespace configuration
} //namespace scheduler
} //namespace sos
