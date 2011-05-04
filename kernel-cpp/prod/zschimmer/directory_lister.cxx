// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include <stdio.h>
#include <fcntl.h>

#include "zschimmer.h"
#include "log.h"
#include "directory_lister.h"

#ifdef Z_WINDOWS
#   include "z_windows.h"
#endif


namespace zschimmer {
namespace file {

//---------------------------------------------------------------Directory_lister::Directory_lister
    
Directory_lister::Directory_lister()
:
    _zero_(this+1)
{
    init();
}

//---------------------------------------------------------------Directory_lister::Directory_lister
    
Directory_lister::Directory_lister( const File_path& path )
:
    _zero_(this+1)
{
    init();
    open( path );
}

//--------------------------------------------------------------Directory_lister::~Directory_lister
    
Directory_lister::~Directory_lister()
{
    try
    {
        close();
    }
    catch( exception& x ) 
    { 
        Z_LOG( Z_FUNCTION << "  ERROR " << x.what() << "\n" ); 
    }
}

//----------------------------------------------------------------------------Directory_lister:init
    
void Directory_lister::init()
{
#   ifdef Z_WINDOWS

        _handle = -1;

#   endif
}

//----------------------------------------------------------------------Directory_lister::is_opened

bool Directory_lister::is_opened() const
{
#   ifdef Z_WINDOWS
        return _handle != -1;
#    else
        return _handle != NULL;
#   endif
}

//---------------------------------------------------------------------------Directory_lister::open
    
void Directory_lister::open( const File_path& path )
{
    if( path == "" )  throw_xc( Z_FUNCTION, "Missing directory path" );

    _directory_path = path;

#   ifdef Z_WINDOWS

        File_path pattern ( _directory_path, "*" );

        memset( &_finddata, 0, sizeof _finddata );

        Z_LOG2( "file.directory", "_findfirst(" << quoted_string(pattern) << ")\n" );

        _handle = _findfirst( pattern.c_str(), &_finddata ); 
        if( _handle == -1 )  throw_errno( errno, "_findfirst", pattern.c_str() );  

        //Z_LOG2( "scheduler", "_findfirst(" << quoted_string(pattern) << ") OK\n" );

#    else

        Z_LOG2( "file.directory", "opendir(" << quoted_string(_directory_path) << ")\n" );

        _handle = opendir( _directory_path.c_str() );
        if( !_handle )  throw_errno( errno, "opendir", _directory_path.c_str() );

        //Z_LOG2( "scheduler.directory", "opendir(" << quoted_string(_directory_path) << ") OK\n" );

#   endif
}

//--------------------------------------------------------------------------Directory_lister::close

void Directory_lister::close()
{
#   ifdef Z_WINDOWS

        if( _handle != -1 )  _findclose( _handle ),  _handle = -1; 

#    else

        if( _handle )  closedir( _handle ), _handle = NULL;

#   endif
}

//----------------------------------------------------------------------------Directory_lister::get

ptr<File_info> Directory_lister::get()
{
    ptr<File_info> result;

    while(1)
    {
        result = read();
        if( !result )  break;

        string name = result->path().name();
        if( name == "."  )  continue;
        if( name == ".." )  continue;
        break;
    }

    if( result )  result->path().set_directory( _directory_path );
    return result;
}

//---------------------------------------------------------------------------Directory_lister::read
#ifdef Z_WINDOWS

ptr<File_info> Directory_lister::read()
{
    if( _finddata.name[ 0 ] == '\0' )
    {
        int ret = _findnext( _handle, &_finddata ); 
        if( ret == -1 )  
        {
            if( errno == ENOENT )  return NULL;
            throw_errno( errno, "_findnext" ); 
        }
    }


    ptr<File_info> result = Z_NEW( File_info );

    result->path().set_name( _finddata.name );
    result->set_create_time     ( _finddata.time_create == -1? 0 : _finddata.time_create );
    result->set_last_access_time( _finddata.time_access == -1? 0 : _finddata.time_access );
    result->set_last_write_time ( _finddata.time_write  == -1? 0 : _finddata.time_write  );
    result->set_directory( ( _finddata.attrib & _A_SUBDIR ) != 0 );

    memset( &_finddata, 0, sizeof _finddata );

    return result;
}

#endif

//---------------------------------------------------------------------------Directory_lister::read
#ifndef Z_WINDOWS

ptr<File_info> Directory_lister::read()
{
    struct dirent* entry = readdir( _handle );
    if( !entry )  return NULL;

    ptr<File_info> result = Z_NEW( File_info );
    result->path().set_name( entry->d_name );
    return result;
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace file
} //namespace zschimmer
