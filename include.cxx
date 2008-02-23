// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace include {

using namespace directory_observer;
using namespace zschimmer::file;

//-----------------------------------------------------------------Include_command::Include_command

Include_command::Include_command( const File_based& source_file_based, const xml::Element_ptr& element )
:
    _zero_(this+1)
{
    string file      = element.getAttribute( "file"      );
    string live_file = element.getAttribute( "live_file" );

    if( file == ""  &&  live_file == "" )  z::throw_xc( "SCHEDULER-231", "live_file" );
    if( file != ""  &&  live_file != "" )  z::throw_xc( "SCHEDULER-442", "file", "live_file" );

    #ifdef Z_WINDOWS
        if( live_file.find( ':' ) != string::npos )  z::throw_xc( "SCHEDULER-417", live_file );     // Laufwerksbuchstabe?
        for( int i = 0; i < live_file.length(); i++ )  if( live_file[i] == '\\' )  live_file[i] = '/';
    #endif

    
    if( file != "" )
    {
        _file_path = file;
    }
    else
    {
        assert( source_file_based.has_base_file() );
        _path.set_simplified_dot_dot_path( source_file_based.path().folder_path() + "/" + live_file );

        string configuration_root_directory = source_file_based.has_base_file()? source_file_based.configuration_root_directory() 
                                                                               : source_file_based.spooler()->_configuration_directories[ confdir_local ];
        _file_path = File_path( configuration_root_directory, _path );
    }
}

//--------------------------------------------------------------------Include_command::read_content
    
string Include_command::read_content()
{
    _file_info = Z_NEW( file::File_info( file_path() ) );
    _file_info->last_write_time();      // Jetzt diesen Wert holen

    file::Mapped_file file ( file_path(), "rb" );
    return string( (const char*)file.map(), file.map_length() );
}

//-----------------------------------------------------------------------Include_command::file_info

file::File_info* Include_command::file_info() const
{
    assert( _file_info );
    return _file_info;
}

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
    _configuration = spooler()->folder_subsystem()->configuration( which_configuration() );

    _include_map[ path ] = file_info;
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

bool Has_includes::include_has_changed()
{
    bool result = false;

    if( _configuration )
    {
        Z_FOR_EACH( Include_map, _include_map, inc ) 
        {
            Path             path      = inc->first;
            file::File_info* file_info = inc->second;

            if( const Directory_entry* directory_entry = _configuration->_directory_observer->directory_tree()->root_directory()->entry_of_path_or_null( path ) )
            {
                if( !directory_entry->is_aging()  &&
                    directory_entry->_file_info->last_write_time() != file_info->last_write_time() )
                {
                    result = true;
                    break;
                }
            }
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
