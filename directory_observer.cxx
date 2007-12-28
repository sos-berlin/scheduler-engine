// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace directory_observer {

using namespace zschimmer::file;

//---------------------------------------------------------------------------new_directory_observer

//ptr<Directory_observer> new_directory_observer( Scheduler* scheduler, const File_path& directory_path )                
//{ 
//    return Z_NEW( Directory_observer( scheduler, directory_path ) ); 
//}
//
////-----------------------------------------------------------Directory_observer::Directory_observer
//    
//Directory_observer::Directory_observer( Scheduler* scheduler, const File_path& directory_path )
//:
//    Scheduler_object( scheduler, this, type_directory_observer ),
//    _zero_(this+1),
//    _directory_watch_interval( folder::directory_watch_interval_max )
//{
//    _directory_tree = Z_NEW( Directory_tree( scheduler, directory_path ) );
//}
//
////----------------------------------------------------------Directory_observer::~Directory_observer
//    
//Directory_observer::~Directory_observer()
//{
//    try
//    {
//        close();
//    }
//    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
//}
//
////--------------------------------------------------------------------------Directory_observer::close
//
//void Directory_observer::close()
//{
//    set_async_manager( NULL );
//
//#   ifdef Z_WINDOWS
//        Z_LOG2( "scheduler", "FindCloseChangeNotification()\n" );
//        FindCloseChangeNotification( _directory_event._handle );
//        _directory_event._handle = NULL;
//#   endif
//
//    _directory_event.close();
//
//    _directory_tree = NULL;
//}
//
////------------------------------------------------------------------Directory_observer::set_directory
//
////void Directory_observer::set_directory( const File_path& directory )
////{
////    assert_subsystem_state( subsys_initialized, Z_FUNCTION );
////    _directory = directory;
////}
//
////-----------------------------------------------------------Directory_observer::subsystem_initialize
//
////bool Directory_observer::subsystem_initialize()
////{
////    _directory_tree = Z_NEW( Directory( this, (Directory*)NULL ) );
////    add_file_based( _directory_tree );
////
////    _subsystem_state = subsys_initialized;
////    return true;
////}
//
////-----------------------------------------------------------------Directory_observer::subsystem_load
//
////bool Directory_observer::subsystem_load()
////{
////    _subsystem_state = subsys_loaded;
////
////    if( _directory.exists() )
////    {
////        _directory_tree->load();
////        check();
////    }
////    else
////        log()->warn( message_string( "SCHEDULER-895", _directory ) );
////
////    return true;
////}
//
////---------------------------------------------------------------------Directory_observer::activate
//
//void Directory_observer::activate()
//{
//    if( !_is_activated )
//    {
//        _is_activated = true;
//
//#       ifdef Z_WINDOWS
//
//            assert( !_directory_event );
//
//            _directory_event.set_name( "Directory_observer " + _directory_tree->directory_path() );
//
//            Z_LOG2( "scheduler", "FindFirstChangeNotification( \"" << _directory_tree->directory_path() << "\", TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
//            
//            HANDLE h = FindFirstChangeNotification( _directory_tree->directory_path().c_str(), 
//                                                    TRUE,                               // Mit Unterverzeichnissen
//                                                    FILE_NOTIFY_CHANGE_FILE_NAME  |  
//                                                    FILE_NOTIFY_CHANGE_DIR_NAME   |
//                                                    FILE_NOTIFY_CHANGE_LAST_WRITE );
//
//            if( !h  ||  h == INVALID_HANDLE_VALUE )  throw_mswin( "FindFirstChangeNotification", _directory_tree->directory_path() );
//
//            _directory_event._handle = h;
//            _directory_event.add_to( &spooler()->_wait_handles );
//
//#       endif
//
//        set_async_manager( _spooler->_connection_manager );
//    }
//}
//
////----------------------------------------------------------------Directory_observer::async_continue_
//
//bool Directory_observer::async_continue_( Continue_flags )
//{
//    Z_LOGI2( "scheduler", Z_FUNCTION << " Prüfe Konfigurationsverzeichnis " << _directory_tree->directory_path() << "\n" );
//
//    _directory_event.reset();
//
//#   ifdef Z_WINDOWS
//        //Z_LOG2( "scheduler", "FindNextChangeNotification(\"" << _directory_path << "\")\n" );
//        BOOL ok = FindNextChangeNotification( _directory_event );
//        if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
//#   endif
//
//    check();
//    
//    return true;
//}
//
////--------------------------------------------------------------Directory_observer::async_state_text_
//
//string Directory_observer::async_state_text_() const
//{
//    S result;
//
//    result << obj_name();
//
//    return result;
//}
//
////-----------------------------------------------------------------Directory_observer::check
//
//bool Directory_observer::check( double minimum_age )
//{
//    bool something_changed = false;
//    double now = double_from_gmtime();
//
//    if( _last_change_at + minimum_age <= now )
//    {
//        if( _read_again_at < now )  _read_again_at = 0;     // Verstrichen?
//
//        something_changed = _directory_tree->read();
//        
//        if( something_changed )  _last_change_at = now;
//
//        _directory_watch_interval = now - _last_change_at < folder::directory_watch_interval_max? folder::directory_watch_interval_min
//                                                                                                : folder::directory_watch_interval_max;
//
//        if( _read_again_at )  set_async_next_gmtime( _read_again_at );
//                        else  set_async_delay( _directory_watch_interval );
//    }
//
//    return something_changed;
//}
//
//------------------------------------------------------------------irectory_entry::Directory_entry

Directory_entry::Directory_entry()
:
    _zero_(this+1)
{
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

//------------------------------------------------------------------------------------------------f
/*
void f()
{
    File_path directory_path ( "..." )

    Directory directory ( Directory_path, file_filter );
    old_directory.read();

    Directory new_directory_tree ( File_path( "..." ) );
    new_directory_tree.read_depth();

    Directory_tree_comparator comparator ( old_directory_tree, new_directory_tree );
    comparator.compare();
    if( com

    Directory_difference difference ( directory, directory_path, file_filter );
    if( difference )
    {
    }

    if( difference.check_again_at() )
    {
    }

    difference = old_directory.compare_with( new_directory_tree );
}

//------------------------------------------------------------------------------------------------g

void g()
{
    Directory_observer directory_observer;

    directory_observer.async_continue();  // Auf Signal hin und periodisch gerufen
    
    if( directory_observer.has_difference() )  send_udp_message();
}
*/
//-------------------------------------------------------------------------------------------------

//struct Filename_difference
//{
//    enum Kind { filename_is_same, filename_is_added, filename_is_removed };
//
//    Kind           _kind;
//    ptr<File_info> _file_info;
//    list<Filename_difference> _children;    // !_children.empty() ==> _file_info->is_directory()
//};

//------------------------------------------------------------------------------fetch_changed_files
/*
void fetch_changed_files()
{
    list<Filename_difference> directory_difference = directory_observer.get_difference();    // Bei neuesten Änderungen nach 2 Sekunden erneut prüfen!
}
*/

//------------------------------------------------------------------------------irectory::Directory

Directory::Directory( Directory_tree* tree, Directory* parent, const string& name ) 
: 
    _zero_(this+1),
    _directory_tree(tree),
    _parent(parent),
    _name(name) 
{
    assert( _directory_tree );
}

//----------------------------------------------------------------------------------Directory::path

Absolute_path Directory::path() const 
{ 
    return Absolute_path( _parent? _parent->path() : root_path, _name );  
}

//-----------------------------------------------------------------------------Directory::file_path

File_path Directory::file_path() const
{
    return File_path( _directory_tree->directory_path(), path() );
}

//--------------------------------------------------------------------------Directory::subdirectory

//Directory* Directory::subdirectory( const string& name )
//{
//    const Directory_entry& entry = entry( name );
//}

//---------------------------------------------------------------------------------Directory::entry

const Directory_entry& Directory::entry( const string& name ) const
{
    Z_FOR_EACH_CONST( Entry_list, _ordered_list, it )  
    {
        if( it->_file_info->path().name() == name )  return *it;
    }

    z::throw_xc( Z_FUNCTION, "unknown entry", name );
}

//----------------------------------------------------------------------------------Directory::read

bool Directory::read( Read_subdirectories read_what )
{
    bool directory_has_changed = false;

    Folder_directory_lister dir   ( _directory_tree->log() );
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
            {
                e->_is_aging_until = 0;
                _directory_tree->set_last_change_at( now );
                directory_has_changed = true;
            }

            if( (*fi)->is_directory() )
            {
                if( !e->_subdirectory )  e->_subdirectory = Z_NEW( Directory( _directory_tree, this, e->_file_info->path().name() ) );
                if( read_what == read_subdirectories )  directory_has_changed = e->_subdirectory->read( read_what );
            }
            else
                e->_subdirectory = NULL;

            fi++, e++;
        }



        /// Dateien hinzugefügt?

        while( fi != ordered_file_infos.end()  &&
               ( e == _ordered_list.end()  ||  (*fi)->path().name() < e->_file_info->path().name() ) )
        {
            list<Directory_entry>::iterator new_entry = _ordered_list.insert( e, Directory_entry() );
            new_entry->_file_info = *fi;
            
            if( (*fi)->is_directory() )  
            {
                new_entry->_subdirectory = Z_NEW( Directory( _directory_tree, this, (*fi)->path().name() ) );
                if( read_what == read_subdirectories )  new_entry->_subdirectory->read( read_what );
                _directory_tree->set_last_change_at( now );
                directory_has_changed = true;
            }
            else
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
                e++;
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


    if( directory_has_changed )  _version++;
    return directory_has_changed;
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

//------------------------------------------------------------------------Directory_tree::directory

Directory* Directory_tree::directory( const Absolute_path& path )
{
    assert( path.folder_path().is_root() );

    const Directory_entry& entry = _root_directory->entry( path.name() );
    
    if( !entry._subdirectory )  z::throw_xc( Z_FUNCTION, "no directory", path );

    return entry._subdirectory;
}

//-----------------------------------------------------------------------------Directory_tree::read

//bool Directory_tree::read()
//{
//    bool directory_has_changed = false;
//
//    directory_has_changed = _root_directory->read( Directory::read_subdirectories );
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

//-------------------------------------------------------------------------------------------------

} //namespace directory_observer
} //namespace scheduler
} //namespace sos
