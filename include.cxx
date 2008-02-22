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
        _path.set_simplified_dot_dot_path( source_file_based.path() + "/" + live_file );

        string configuration_root_directory = source_file_based.has_base_file()? source_file_based.configuration_root_directory() 
                                                                               : source_file_based.spooler()->_local_configuration_directory;
        _file_path = File_path( configuration_root_directory, _path );
    }
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
}

//------------------------------------------------------------------------Has_includes::add_include

void Has_includes::add_include( const Absolute_path& path )
{
    _include_set.insert( path );
    spooler()->folder_subsystem()->configuration( _which_configuration_directory )->_include_register->add_include( this, path );
}

//---------------------------------------------------------------------Has_includes::remove_include

void Has_includes::remove_include( const Absolute_path& path )
{
    _include_set.erase( path );
    spooler()->folder_subsystem()->configuration( _which_configuration_directory )->_include_register->remove_include( this, path );
}

//--------------------------------------------------------------------Has_includes::remove_includes

void Has_includes::remove_includes()
{
    int TODO;
    //Z_FOR_EACH( Path_set, _path_set, it )
    //{
    //    File_based_subsystem* subsystem     = it->first;
    //    Dependant_set&        dependant_set = it->second;

    //    Z_FOR_EACH( Dependant_set, dependant_set, it2 )
    //    {
    //        subsystem->dependencies()->remove_dependant( this, *it2 );
    //    }
    //}
}

//---------------------------------------------------------------Include_register::Include_register

Include_register::Include_register()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------Include_register::~Include_register

Include_register::~Include_register()
{
}

//--------------------------------------------------------------------Include_register::add_include

void Include_register::add_include( Has_includes*, const Absolute_path& )
{
    int TODO;
}

//-----------------------------------------------------------------Include_register::remove_include

void Include_register::remove_include( Has_includes*, const Absolute_path& )
{
    int TODO;
}

//--------------------------------------------------------------------Include_register::check_files

void Include_register::check_files( const Directory* root_directory )
{
    int TODO;
    //Z_FOR_EACH( Path_set, _path_set, e )
    //{
    //    Entry& entry = *e;

    //    if( const Directory_entry* directory_entry = root_directory->entry_of_path_or_null( entry->_path ) )
    //    {
    //        if( directory_entry->_file_info->last_written_time() != entry._file_info->last_written_time() )
    //        {
    //            Z_FOR_EACH( Include_set, entry._include_set, inc )  (*inc)->on_include_changed();
    //        }
    //    }
    //    else
    //    {
    //        Z_LOG2( "joacim", Z_FUNCTION << " <include live_file='" << entry._path << "'>: Datei fehlt\n" );
    //        // Wenn die inkludierte Datei gelöscht ist, lassen wir den Job in Ruhe
    //    }
    //}
}

//-------------------------------------------------------------------------------------------------

} //namespace include
} //namespace scheduler
} //namespace sos
