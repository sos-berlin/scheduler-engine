// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace directory_observer {

using namespace zschimmer::file;

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

//------------------------------------------------------------------irectory_entry::Directory_entry

Directory_entry::Directory_entry()
:
    _zero_(this+1)
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

//------------------------------------------------------------------------------irectory::Directory

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

bool Directory::read( Read_subdirectories read_what, double minimum_age )
{
    if( !_directory_tree )  z::throw_xc( Z_FUNCTION );

    bool   directory_has_changed = false;
    double now                   = double_from_gmtime();

    if( read_what != read_subdirectories  ||
        _last_read_at + minimum_age <= now )
    {
        if( read_what == read_subdirectories )  _last_read_at = now;

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
                if( e->_is_aging_until )
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

        if( my == _ordered_list.end()  ||  my->_file_info->path().name() > o->_file_info->path().name() )  
            _ordered_list.insert( my, o->clone( this ) );
    }


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

//-------------------------------------------------------------------------------------------------

} //namespace directory_observer
} //namespace scheduler
} //namespace sos
