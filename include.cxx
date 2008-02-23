// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace include {

using namespace directory_observer;
using namespace zschimmer::file;

//-----------------------------------------------------------------Include_command::Include_command

Include_command::Include_command( const File_based* source_file_based, const xml::Element_ptr& element )
:
    _zero_(this+1)
{
    string file      = subst_env( element.getAttribute( "file"      ) );
    string live_file = subst_env( element.getAttribute( "live_file" ) );

    if( file == ""  &&  live_file == "" )  z::throw_xc( "SCHEDULER-231", "live_file" );
    if( file != ""  &&  live_file != "" )  z::throw_xc( "SCHEDULER-442", "file", "live_file" );

    #ifdef Z_WINDOWS
        if( live_file.find( ':' ) != string::npos )  z::throw_xc( "SCHEDULER-417", live_file );     // Laufwerksbuchstabe?
        for( int i = 0; i < live_file.length(); i++ )  if( live_file[i] == '\\' )  live_file[i] = '/';
    #endif

    
    if( file != "" )    // file="..."
    {
        _file_path = file;
    }
    else    // live_file="..."
    if( source_file_based )
    {
        Path path = source_file_based->path().folder_path() + "/" + live_file;
        _path.set_simplified_dot_dot_path( path );

        string configuration_root_directory = source_file_based->has_base_file()? source_file_based->configuration_root_directory() 
                                                                                : source_file_based->spooler()->_configuration_directories[ confdir_local ];
        _file_path = File_path( configuration_root_directory, path );
    }
    else
    {
        z::throw_xc( "SCHEDULER-462" );     // live_file="..." hier nicht möglich
        //_file_path = File_path( scheduler->_configuration_directories[ confdir_local ], live_file );
    }
}

//----------------------------------------------------Include_command::add_include_and_read_content
    
string Include_command::add_include_and_read_content( File_based* source_file_based )
{
    file::Mapped_file file;

    if( file.try_open( file_path(), "rb" ) )
    {
        ptr<file::File_info> file_info = Z_NEW( file::File_info() );
        file_info->set_path( file_path() );

        file_info->call_fstat( file.file_no() );
        file_info->last_write_time();      // Jetzt diesen Wert holen
        _file_info = file_info;
    }


    if( source_file_based  &&  denotes_configuration_file() )
    {
        source_file_based->add_include( path(), _file_info );       // Auch wenn Datei sich nicht öffnen lässt
    }

    file.check_error( "open" );

    return string( (const char*)file.map(), file.map_length() );
}

//--------------------------------------------------------------------Include_command::read_content
    
//string Include_command::read_content()
//{
//    file::Mapped_file file ( file_path(), "rb" );
//    return string( (const char*)file.map(), file.map_length() );
//}

//-----------------------------------------------------------------------Include_command::file_info

//file::File_info* Include_command::file_info() 
//{
//    if( !_file_info )
//    {
//        ptr<file::File_info> file_info = Z_NEW( file::File_info() );
//        file_info->set_path( file_path() );
//
//        try
//        {
//            if( file_info->try_call_stat() )    // Datei ist da?
//            {
//                file_info->last_write_time();      // Jetzt diesen Wert holen
//                _file_info = file_info;
//            }
//        }
//        catch( exception& x )  { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }      // Für andere Fehler als ENOENT
//    }
//
//    return _file_info;
//}

//-----------------------------------------------------------------------Has_includes::Has_includes
    
Has_includes::Has_includes()
:
    _zero_(this+1)
{
}

//----------------------------------------------------------------------Has_includes::~Has_includes
    
Has_includes::~Has_includes()
{
    try
    {
        remove_includes();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }

    //#ifndef NDEBUG
    //    if( _configuration )  _configuration->_include_register->assert_no_has_includes( this );
    //#endif
}

//------------------------------------------------------------------------Has_includes::add_include

void Has_includes::add_include( const Absolute_path& path, file::File_info* file_info )
{
    assert( !path.empty() );

    _configuration = spooler()->folder_subsystem()->configuration( which_configuration() );

    _include_map[ path ] = file_info;   // NULL, wenn Datei nicht da ist
    //_configuration->_include_register->add_include( this, path );
}

//---------------------------------------------------------------------Has_includes::remove_include

void Has_includes::remove_include( const Absolute_path& path )
{
    //if( _configuration )  _configuration->_include_register->remove_include( this, path );
    _include_map.erase( path );
}

//--------------------------------------------------------------------Has_includes::remove_includes

void Has_includes::remove_includes()
{
    _include_map.clear();
    //Include_map my_include_map = _include_map;

    //Z_FOR_EACH( Path_set, my_path_set, p ) 
    //{
    //    try
    //    {
    //        remove_include( Absolute_path( *p ) );
    //    }
    //    catch( exception& x )  { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
    //}
}

//----------------------------------------------------------------Has_includes::include_has_changed

file::File_info* Has_includes::changed_included_file_info()
{
    file::File_info* result = NULL;

    if( _configuration )
    {
        Z_FOR_EACH( Include_map, _include_map, inc ) 
        {
            Path             path      = inc->first;
            file::File_info* file_info = inc->second;

            if( const Directory_entry* directory_entry = _configuration->_directory_observer->directory_tree()->root_directory()->entry_of_path_or_null( path ) )
            {
                if( !directory_entry->is_aging() )
                {
                    if( !file_info )  result = directory_entry->_file_info;  // Datei ist neu hinzugekommen
                    else
                    if( directory_entry->_file_info->last_write_time() != file_info->last_write_time() )  result = directory_entry->_file_info;  // Datei geändert
                }
            }
            else
            if( file_info )  result = file_info;      // Datei ist gelöscht (oder sollte das ignoriert werden, sodass der Job weiterläuft?)

            if( result )  break;
        }
    }

    return result;
}

//---------------------------------------------------------------Include_register::Include_register

//Include_register::Include_register()
//:
//    _zero_(this+1)
//{
//}
//
////--------------------------------------------------------------Include_register::~Include_register
//
//Include_register::~Include_register()
//{
//}
//
////--------------------------------------------------------------------Include_register::add_include
//
//void Include_register::add_include( Has_includes* has_includes, const Absolute_path& path )
//{
//    _include_map[ path ]._has_includes_set.insert( has_includes );
//}
//
////-----------------------------------------------------------------Include_register::remove_include
//
//void Include_register::remove_include( Has_includes* has_includes, const Absolute_path& path )
//{
//    Include_map::iterator inc = _include_map.find( path );
//    if( inc != _include_map.end() )
//    {
//        inc->second._has_includes_set.erase( has_includes );
//        if( inc->second._has_includes_set.empty() )  _include_map.erase( inc );
//    }
//}

//--------------------------------------------------------------------Include_register::check_files

//void Include_register::check_files( const Directory* root_directory )
//{
//    Z_FOR_EACH( Include_map, _include_map, inc )
//    {
//        Path   path  = inc->first;
//        Entry* entry = &inc->second;
//
//        if( const Directory_entry* directory_entry = root_directory->entry_of_path_or_null( path ) )
//        {
//            if( directory_entry->_file_info->last_write_time() != entry->_file_info->last_write_time() )
//            {
//                Z_FOR_EACH( Has_includes_set, entry->_has_includes_set, inc ) 
//                {
//                    try
//                    {
//                        (*inc)->on_include_changed();
//                    }
//                    catch( exception& x )  { (*inc)->log()->warn( S() << x.what() << ", on_include_changed()" ); }
//                }
//            }
//        }
//        else
//        {
//            Z_LOG2( "joacim", Z_FUNCTION << " <include live_file='" << path << "'>: Datei fehlt\n" );
//            // Wenn die inkludierte Datei gelöscht ist, lassen wir den Job in Ruhe
//        }
//    }
//}

//---------------------------------------------------------Include_register::assert_no_has_includes

//void Include_register::assert_no_has_includes( const Has_includes* has_includes )
//{
//    #ifndef NDEBUG
//        Z_FOR_EACH( Include_map, _include_map, inc )
//        {
//            Entry* entry = &inc->second;
//
//            Z_FOR_EACH( Has_includes_set, entry->_has_includes_set, inc )   assert( *inc != has_includes );
//        }
//    #endif
//}

//-------------------------------------------------------------------------------------------------

} //namespace include
} //namespace scheduler
} //namespace sos
