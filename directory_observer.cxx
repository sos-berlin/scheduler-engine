// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace directory_observer {

using namespace zschimmer::file;

//-------------------------------------------------------------------------------------------------

const int                       file_timestamp_delay        = 2+1;      // FAT-Zeitstempel sind 2 Sekunden genau
const int                       remove_delay                = 2;        // Nur Dateien, die solange weg sind, gelten als gelöscht. Sonst wird removed/added zu modified

#ifdef Z_WINDOWS
    const int                   directory_watch_interval_min = 60;
    const int                   directory_watch_interval_max = 60;
#else
    const int                   directory_watch_interval_min =  5;
    const int                   directory_watch_interval_max = 60;
#endif

//------------------------------------------------------------------------------file_info_is_lesser
    
static bool file_info_is_lesser( const file::File_info* a, const file::File_info* b )
{ 
    return (string)a->path() < (string)b->path(); 
}

//-------------------------------------------------------------------Directory_tree::Directory_tree

Directory_tree::Directory_tree( Scheduler* scheduler, const file::File_path& directory_path )
:
    Scheduler_object( scheduler, this, type_directory_tree ),
    _zero_(this+1),
    _directory_path(directory_path),
    _refresh_aged_entries_at(double_time_max)
{
    _root_directory = Z_NEW( Directory( this, (Directory*)NULL, root_path ) );
}

//------------------------------------------------------------------Directory_tree::~Directory_tree
    
Directory_tree::~Directory_tree()
{
}

//----------------------------------------------------------------Directory_tree::directory_or_null

Directory* Directory_tree::directory_or_null( const string& name )
{
    Directory* result = NULL;

    if( const Directory_entry* entry = _root_directory->entry_or_null( name ) )
    {
        result = entry->_subdirectory;
    }

    return result;
}

//-----------------------------------------------------------------------------Directory_tree::read

//bool Directory_tree::read()
//{
//    bool directory_has_changed = false;
//
//    directory_has_changed = _root_directory->read_deep( 0.0 );
//
//    return directory_has_changed;
//}

//-------------------------------------------------------------Directory_tree::refresh_aged_entries

//void Directory_tree::refresh_aged_entries()
//{
//    if( _refresh_aged_entries_at < double_time_max &&  _refresh_aged_entries_at >= double_from_gmtime() )
//    {
//        bool anything_is_refreshed = _root_directory->refresh_aged_entries();
//        if( anything_is_refreshed )  _refresh_aged_entries_at = double_time_max;
//    }
//}

//----------------------------------------------------Directory_entry::normalized_less_dereferenced

bool Directory_entry::normalized_less_dereferenced( const Directory_entry* a, const Directory_entry* b )
{ 
    int cmp = strcmp( a->_normalized_name.c_str(), b->_normalized_name.c_str() );
    if( cmp == 0 )
    {
        cmp = strcmp( a->_file_info->path().name().c_str(), b->_file_info->path().name().c_str() );     // Nur im Fall einer Namenskollision
    }

    return cmp < 0;
}

//-----------------------------------------------------------------Directory_entry::Directory_entry

Directory_entry::Directory_entry()
:
    _zero_(this+1),
    _version(1)
{
}

//---------------------------------------------------------------------------Directory::clone_entry

Directory_entry Directory_entry::clone( Directory* new_parent ) const
{
    Directory_entry result;

    result = Directory_entry( *this );
    if( result._subdirectory )  result._subdirectory = _subdirectory->clone2( new_parent );

    return result;
}

//------------------------------------------------------------------------------Directory::obj_name

//string Directory::obj_name() const
//{
//    S result;
//    result << Scheduler_object::obj_name();
//    result << " " << path();
//    return result;
//}

//-------------------------------------------------------------------------------------------------

//bool file_filter( File_info* file_info )
//{
//    if( file_info->is_directory() )  return true;
//    string extension = lcase( file_info->path()->extension() );
//    if( extension == ".job.xml" )  return true;
//    return false;
//}

//-----------------------------------------------------------------------------Directory::Directory

Directory::Directory( Directory_tree* tree, Directory* parent, const string& name ) 
: 
    _zero_(this+1),
    _directory_tree(tree),
    _parent(parent),
    _name(name) 
{
}

//----------------------------------------------------------------------------------Directory::path

Absolute_path Directory::path() const 
{ 
    return Absolute_path( _parent? _parent->path() : root_path, _name );  
}

//-----------------------------------------------------------------------------Directory::file_path

File_path Directory::file_path() const
{
    if( !_directory_tree )  z::throw_xc( Z_FUNCTION );

    return File_path( _directory_tree->directory_path(), path() );
}

//---------------------------------------------------------------------------------Directory::entry

const Directory_entry* Directory::entry_or_null( const string& name ) const
{
    Z_FOR_EACH_CONST( Entry_list, _ordered_list, it )  
    {
        if( it->_file_info->path().name() == name )  return &*it;
    }

    return NULL;
}

//----------------------------------------------------------------------------------Directory::read

bool Directory::read( Read_flags read_what, double minimum_age )
{
    if( !_directory_tree )  z::throw_xc( Z_FUNCTION );

    bool   directory_has_changed = false;
    double now                   = double_from_gmtime();

    if( _name != ""  &&                                 // Verzeichnis "" ist leer
        ( !( read_what & read_subdirectories )  ||
          _last_read_at + minimum_age <= now ) )
    {
        if( read_what & read_subdirectories )  _last_read_at = now;

        Folder_directory_lister       dir               ( _directory_tree->log() );
        list< ptr<file::File_info> >  file_info_list;
        vector< file::File_info*>     ordered_file_infos;       // Geordnete Liste der vorgefundenen Dateien    

        dir.open( _directory_tree->directory_path(), path() );
        if( dir.is_opened() )
        {
            while( ptr<file::File_info> file_info = dir.get() )  file_info_list.push_back( &*file_info );
            dir.close();
        }

        ordered_file_infos.reserve( file_info_list.size() );
        Z_FOR_EACH_CONST( list< ptr<file::File_info> >, file_info_list, it )  ordered_file_infos.push_back( &**it );
        sort( ordered_file_infos.begin(), ordered_file_infos.end(), file_info_is_lesser );

        vector<file::File_info*>::iterator fi  = ordered_file_infos.begin();
        list<Directory_entry>   ::iterator e   = _ordered_list.begin();
        double                             now = double_from_gmtime();

        while( fi != ordered_file_infos.end()  ||
               e  != _ordered_list.end() )
        {
            /// Dateinamen gleich?

            while( e  != _ordered_list.end()  &&
                   fi != ordered_file_infos.end()  &&
                   e->_file_info->path().name() == (*fi)->path().name() )
            {
                e->_is_removed = false;

                if( (*fi)->is_directory() )
                {
                    if( !e->_subdirectory )  
                    {
                        e->_subdirectory = Z_NEW( Directory( _directory_tree, this, e->_file_info->path().name() ) );
                        directory_has_changed = true;
                    }

                    if( read_what & read_subdirectories )  directory_has_changed |= e->_subdirectory->read( read_what );
                }
                else
                {
                    e->_subdirectory = NULL;

                    if( e->_file_info->last_write_time() != (*fi)->last_write_time() ) 
                    {
                        e->_file_info      = *fi;
                        e->_is_aging_until = now + file_timestamp_delay;
                        _directory_tree->set_aging_until( e->_is_aging_until );
                    }
                    else
                    if( now < e->_is_aging_until )      // Noch nicht genug gealtert?
                    {
                        _directory_tree->set_aging_until( e->_is_aging_until );
                    }
                    else
                    if( e->_is_aging_until )
                    {
                        e->_is_aging_until = 0;
                        e->_version++;
                        _directory_tree->set_last_change_at( now );
                        directory_has_changed = true;
                    }
                }

                fi++, e++;
            }



            /// Dateien hinzugefügt?

            while( fi != ordered_file_infos.end()  &&
                   ( e == _ordered_list.end()  ||  (*fi)->path().name() < e->_file_info->path().name() ) )
            {
                list<Directory_entry>::iterator new_entry = _ordered_list.insert( e, Directory_entry() );
                new_entry->_file_info     = *fi;
                new_entry->_is_from_cache = _directory_tree->is_cache();
                
                if( (*fi)->is_directory() )  
                {
                    new_entry->_subdirectory = Z_NEW( Directory( _directory_tree, this, (*fi)->path().name() ) );
                    if( read_what & read_subdirectories )  new_entry->_subdirectory->read( read_what );
                    _directory_tree->set_last_change_at( now );
                    directory_has_changed = true;
                }
                else
                if( !( read_what & read_suppress_aging )  ||  _repeated_read )
                {
                    new_entry->_is_aging_until = now + file_timestamp_delay;
                    _directory_tree->set_aging_until( new_entry->_is_aging_until );
                }

                fi++;
            }

            assert( fi == ordered_file_infos.end()  || 
                    e == _ordered_list.end() ||
                    (*fi)->path().name() >= e->_file_info->path().name() );
            


            /// Dateien gelöscht?

            while( e != _ordered_list.end()  &&
                   ( fi == ordered_file_infos.end()  ||  (*fi)->path().name() > e->_file_info->path().name() ) )  // Datei entfernt?
            {
                if( e->_subdirectory )
                {
                    e = _ordered_list.erase( e );       // Verzeichniseinträge nicht altern lassen, sofort löschen
                    _directory_tree->set_last_change_at( now );
                    directory_has_changed = true;
                }
                else
                if( !e->_is_removed )
                {
                    e->_is_removed     = true;
                    e->_is_aging_until = now + remove_delay;
                    _directory_tree->set_aging_until( e->_is_aging_until );
                    e++;
                }
                else
                if( now < e->_is_aging_until )      // Noch nicht genug gealtert?
                {
                    _directory_tree->set_aging_until( e->_is_aging_until );
                    e++;
                }
                else
                {
                    e = _ordered_list.erase( e );
                    _directory_tree->set_last_change_at( now );
                    directory_has_changed = true;
                }
            }

            assert( e == _ordered_list.end()  ||
                    fi == ordered_file_infos.end()  ||
                    (*fi)->path().name() <= e->_file_info->path().name() );
        }
    }

    assert_ordered_list();

    _repeated_read = true;
    if( directory_has_changed )  _version++;
    return directory_has_changed;
}

//--------------------------------------------------------------------------------Directory::clone2

ptr<Directory> Directory::clone2( Directory* new_parent ) const
{
    ptr<Directory> result = Z_NEW( Directory( (Directory_tree*)NULL, new_parent, _name ) );

    Z_FOR_EACH_CONST( Entry_list, _ordered_list, it )  
    {
        result->_ordered_list.push_back( it->clone( new_parent ) );
    }

    return result;
}

//---------------------------------------------------------------------Directory::merge_new_entries

void Directory::merge_new_entries( const Directory* other )
{
    Entry_list::iterator my = _ordered_list.begin();
    Z_FOR_EACH_CONST( Entry_list, other->_ordered_list, o )  
    {
        while( my != _ordered_list.end()  &&  my->_file_info->path().name() < o->_file_info->path().name() )  
            my++;

        if( my != _ordered_list.end()  &&  my->_file_info->path().name() == o->_file_info->path().name() )
        {
            if( o->_subdirectory )
            {
                if( my->_subdirectory )  my->_subdirectory->merge_new_entries( o->_subdirectory );
                                  else  *my = o->clone( this );
            }
            else
            if( !o->is_aging() )  my->_duplicate_version = o->_version;     // Meldung SCHEDULER-703 wird auf das Duplikat hinweisen
        }

        if( my == _ordered_list.end()  ||  my->_file_info->path().name() > o->_file_info->path().name() )  
            _ordered_list.insert( my, o->clone( this ) );
    }


    assert_ordered_list();
}

//-------------------------------------------------------------------Directory::assert_ordered_list

void Directory::assert_ordered_list()
{
#   ifndef NDEBUG   // Ordnung prüfen
    {
        Entry_list::const_iterator a = _ordered_list.begin();
        if( a != _ordered_list.end() )  
        {
            Entry_list::const_iterator b = a;
            b++;

            while( b != _ordered_list.end() )
            {
                assert( a->_file_info->path().name() < b->_file_info->path().name() );
                a++;
                b++;
            }   
        }
    }
#   endif
}

//------------------------------------------------------------------Directory::refresh_aged_entries

//bool Directory::refresh_aged_entries()
//{
//    bool   anything_is_refreshed = true;
//    double now                   = double_from_gmtime();
//
//    for( list<Directory_entry>::iterator it = _ordered_list.begin(); it != _ordered_list.end(); )
//    {
//        Directory_entry* entry = &*it++;
//
//        if( entry->_subdirectory )  anything_is_refreshed &= entry->_subdirectory->refresh_aged_entries();
//
//        if( entry->_is_aging )
//        {
//            if( entry->_is_removed )
//            {
//                if( now < entry->_timestamp + remove_delay )
//                {
//                    anything_is_refreshed = false;
//                }
//                else
//                {
//                    ptr<file::File_info> new_file_info = Z_NEW( file::File_info );
//                    
//                    if( new_file_info->try_call_stat() )    // Gelöschte Datei ist wieder da?
//                    {
//                        entry->_file_info  = new_file_info;
//                        entry->_is_removed = false;
//                        entry->_timestamp  = now;
//                    }
//                    else   
//                    if( entry->_is_removed )
//                    {
//                        --it;                               // it war schon weitergeschaltet
//                        it = _ordered_list.erase( it );
//                    }
//                }
//            }
//            else
//            if( now < entry->_timestamp + file_timestamp_delay )
//            {
//                anything_is_refreshed = false;
//            }
//            else
//            {
//                ptr<file::File_info> new_file_info = Z_NEW( file::File_info );
//                
//                if( new_file_info->try_call_stat() )  
//                {
//                    if( new_file_info->last_write_time() != entry->_file_info->last_write_time() )
//                    {
//                        entry->_file_info = new_file_info;
//                        entry->_timestamp = now;
//                    }
//                    else
//                    {
//                        entry->_is_aging = false;
//                    }
//                }
//                else   
//                {
//                    entry->_is_removed = true;
//                    entry->_timestamp  = now;
//                }
//            }
//        }
//    }
//
//    return anything_is_refreshed;
//}

//-----------------------------------------------------------Directory_observer::Directory_observer

Directory_observer::Directory_observer( Scheduler* scheduler, const File_path& directory_path )
:
    Scheduler_object( scheduler, this, type_directory_observer ),
    _zero_(this+1)
{
    if( directory_path == "" )  z::throw_xc( Z_FUNCTION );

    _directory_tree = Z_NEW( Directory_tree( spooler(), directory_path ) );
}

//----------------------------------------------------------Directory_observer::~Directory_observer
    
Directory_observer::~Directory_observer()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
}

//------------------------------------------------------------------------Directory_observer::close

void Directory_observer::close()
{
    remove_from_event_manager();

    #ifdef Z_WINDOWS
        if( _directory_event._handle )
        {
            Z_LOG2( "scheduler", "FindCloseChangeNotification(" << _directory_event << ")\n" );
            FindCloseChangeNotification( _directory_event._handle );
            _directory_event._handle = NULL;
        }
    #endif

    _directory_event.close();
    _directory_tree = NULL;
}

//---------------------------------------------------Directory_observer::register_directory_handler

void Directory_observer::register_directory_handler( Directory_handler* directory_handler )
{
    if( _directory_handler )  z::throw_xc( Z_FUNCTION );
    _directory_handler = directory_handler;
}

//---------------------------------------------------------------------Directory_observer::activate

void Directory_observer::activate()
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
        
        //set_async_delay( folder::directory_watch_interval_min );
        //async_wake();
        //async_continue();

        _is_activated = true;
    }
}

//--------------------------------------------------------------Directory_observer::async_continue_

bool Directory_observer::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler", Z_FUNCTION << " Prüfe Konfigurationsverzeichnis " << _directory_tree->directory_path() << "\n" );

    bool something_done;

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
    
    something_done = run_handler();

    return something_done;
}

//------------------------------------------------------------------Directory_observer::run_handler

bool Directory_observer::run_handler()
{
    bool something_done;

    directory_tree()->reset_aging();

    if( !_directory_handler )  z::throw_xc( Z_FUNCTION, "_directory_handler==NULL" );
    something_done = _directory_handler->on_handle_directory( this );

    double now      = double_from_gmtime();
    double interval = now - directory_tree()->last_change_at() < directory_watch_interval_max? directory_watch_interval_min
                                                                                             : directory_watch_interval_max;
    _next_check_at = now + interval;
    set_alarm();

    return something_done;
}

//-----------------------------------------------------------------Directory_observer::set_signaled

void Directory_observer::set_signaled( const string& text )
{
    _directory_event.set_signaled( text );
}

//------------------------------------------------------------Directory_observer::async_state_text_

string Directory_observer::async_state_text_() const
{
    S result;

    result << obj_name();

    return result;
}

//---------------------------------------------------------------------Directory_observer::obj_name

string Directory_observer::obj_name() const
{
    S result;

    result << Scheduler_object::obj_name();
    if( _directory_tree )  result << "(" << _directory_tree->directory_path() << ")";

    return result;
}

//--------------------------------------------------------------------Directory_observer::set_alarm

void Directory_observer::set_alarm()
{
    set_async_next_gmtime( min( _directory_tree->refresh_aged_entries_at(), _next_check_at ) );
}

//--------------------------------------------------------------------Folder_directory_lister::open

bool Folder_directory_lister::open( const File_path& root, const Absolute_path& path )
{
    assert( _log );

    bool      result        = false;
    File_path complete_path ( root, path.without_slash() );

    try
    {
        Directory_lister::open( complete_path );
        result = true;
    }
    catch( zschimmer::Xc& x )
    {
        if( path.is_root()  &&  !complete_path.exists() )  z::throw_xc( "SCHEDULER-882", complete_path, x );    // Jemand hat das Konfigurationsverzeichnis entfernt

        if( x.code() == ( S() << "ERRNO-" << ENOENT ).to_string() )  // ERRNO-2  Verzeichnis gelöscht
        {
            _is_removed = true; 
            _log->debug3( x.what() );
        }
        else
        if( x.code() == ( S() << "ERRNO-" << EINVAL ).to_string() )  // ERRNO-22 "invalid argument"? Die Bedeutung ist nicht bekannt.
        {
            _log->info( x.what() ); 
        }
        else  
            throw;
    }

    return result;
}

//---------------------------------------------------------------------Folder_directory_lister::get

ptr<file::File_info> Folder_directory_lister::get()
{
    ptr<file::File_info> result;

    while(1)
    {
        result = Directory_lister::get();
        if( !result )  break;

#       ifdef Z_UNIX
            bool file_exists = result->try_call_stat();
#        else
            bool file_exists = true;        // Unter Windows hat File_info schon alle Informationen, kein stat() erforderlich
#       endif

        if( file_exists )  break;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace directory_observer
} //namespace scheduler
} //namespace sos
