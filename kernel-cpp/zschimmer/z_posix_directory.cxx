// $Id: z_posix_directory.cxx 12090 2006-05-30 07:00:24Z jz $

#include "zschimmer.h"
#include <dirent.h>
#include "z_posix_directory.h"

namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------Simple_directory_reader::close
/* Nicht geprüft
    
void Simple_directory_reader::close() 
{ 
    if( _handle ) 
    {
        closedir( _handle );
        _handle = NULL;
    }
}

//-------------------------------------------------------------------Simple_directory_reader::first

void Simple_directory_reader::open( const string& path, Flags flags ) 
{ 
    _flags = flags;

    _handle = opendir( path.c_str() );
    if( !_handle )  throw_errno( errno, "opendir", path.c_str() );
}

//---------------------------------------------------------------------Simple_directory_reader::get

string Simple_directory_reader::get() 
{ 
    while(1)
    {
        struct dirent* entry = readdir( _handle );
        if( !entry )  return "";

        if( entry->d_type == DT_DIR )
        {
            if( _flags & no_subdirectory )  continue;
            return entry->d_name + Z_DIR_SEPARATOR;
        }

        return entry->d_name;
    }
}
*/
//---------------------------------------------------------------------------------------Glob::Glob
    
Glob::Glob( const string& pattern, int glob_flags )
{
    memset( &_glob, 0, sizeof _glob );

    int err = glob( pattern.c_str(), glob_flags, NULL, &_glob );

    if( err )
    {
        if( err == GLOB_NOMATCH )  return;

        string msg = "Error in glob(\"" + pattern + "\")";
        
        switch( err )
        {
            case GLOB_NOSPACE: throw_xc( "GLOB_NOSPACE", msg );
            case GLOB_ABORTED: throw_xc( "GLOB_ABORTED", msg );
            default: throw_xc( "GLOB", msg );
        }
    }
}

//--------------------------------------------------------------------------------------Glob::~Glob

Glob::~Glob() 
{ 
    globfree( &_glob ); 
}


//--------------------------------------------------------------------list_directory_with_wildcards
/*
std::list<string> list_directory_with_wildcards( const string& pattern, Simple_directory_reader::Flags flags )
{
    // '*', '?', '[', `{', oder '~' am Anfang

    std::list<string> result;

    int flags = 0;

  //flags |= GLOB_ERR       // which means to return upon read error (because a directory does not have read permission, for example),
  //flags |= GLOB_MARK      // which means to append a slash to each path which corresponds to a directory,
  //flags |= GLOB_NOSORT    // which means don't sort the returned pathnames (they are by default),
  //flags |= GLOB_DOOFFS    // which means that pglob->gl_offs slots will be reserved at the beginning of the list of strings in pglob->pathv,
  //flags |= GLOB_NOCHECK   // which means that, if no pattern matches, to return the original pattern,
  //flags |= GLOB_APPEND    // which means to append to the results of a previous call.  Do not set this flag on the first invocation of glob().
  //flags |= GLOB_NOESCAPE  // which means that meta characters cannot be quoted by backslashes.

    // The flags may also include some of the following, which are GNU extensions and not defined by POSIX.2:
  //flags |= GLOB_PERIOD        // which means that a leading period can be matched by meta characters,
  //flags |= GLOB_ALTDIRFUNC    // which means that alternative functions pglob->gl_closedir, pglob->gl_readdir, pglob->gl_opendir, pglob->gl_lstat, and pglob->gl_stat are used for file system access instead of the normal library functions,
    flags |= GLOB_BRACE         // which means that csh(1) style brace expressions {a,b} are expanded,
  //flags |= GLOB_NOMAGIC       // which means that the pattern is returned if it contains no metacharacters,
    flags |= GLOB_TILDE         // which means that tilde expansion is carried out, and
  //flags |= GLOB_ONLYDIR       // which means that only directories are matched.



    Unix_glob unix_glob ( pattern );

    for( const char** p = unix_glob._glob.gl_pathv; *p; p++ )  
    {
        string path = *p;
        if( *path.rbegin() != '/' )  result.push_back( *p );
    }

    return result;
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer
