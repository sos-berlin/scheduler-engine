// $Id: z_windows_directory.cxx 12116 2006-06-06 18:03:24Z jz $

#include "zschimmer.h"
#include "z_directory.h"

namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------Simple_directory_reader::close
    
void Simple_directory_reader::close() 
{ 
    if( _handle != -1 )  _findclose( _handle ),  _handle = -1; 
}

//-------------------------------------------------------------------Simple_directory_reader::first

void Simple_directory_reader::open( const string& path, Flags flags, const string& pattern ) 
{ 
    //string path_pattern = path + "/" + pattern;
    // Windows vergleicht nur die ersten drei Zeichen der Erweiterung. *.jar liefert also auch x.jar~
    // Also machen wir das mit regulären Ausdrücken

    S regex_pattern;

    regex_pattern << '^';

    for( uint i = 0; i < pattern.length(); i++ )
    {
        char c = pattern[ i ];
        switch( c )
        {
            case '*': regex_pattern << ".*";    break;
            case '?': regex_pattern << ".";     break;
            case '.':
            case '+':
            case '{':
            case '}':
            case '(':
            case ')':
            case '|':
            case '[':
            case ']':
            case '^':
            case '$':
            case '\\': regex_pattern << '\\' << c;  break;
            default: regex_pattern << c;
        }
    }

    regex_pattern << '$';

    _regex.compile( regex_pattern, Regex::rx_ignore_case );


    string path_pattern = ( path == ""? "." : path ) + "/*";
    _flags = flags;

    _handle = _findfirsti64( path_pattern.c_str(), &_entry ); 
    if( _handle == -1 )  throw_errno( errno, "_findfirst", path.c_str() );  

    _is_first_entry = true;
}

//---------------------------------------------------------------------Simple_directory_reader::get

string Simple_directory_reader::get() 
{ 
    while(1)
    {
        if( _is_first_entry )  _is_first_entry = false;
        else
        {
            int ret = _findnexti64( _handle, &_entry ); 
            if( ret == -1 )  
            {
                if( errno == ENOENT )  return "";
                throw_errno( errno, "_findnext" ); 
            }
        }

        if( !_regex.match( _entry.name ) )  continue;

        if( _entry.attrib & _A_SUBDIR )
        {
            if( _flags & no_subdirectory )  continue;
            if( strcmp( _entry.name, "."  ) == 0 )  continue;
            if( strcmp( _entry.name, ".." ) == 0 )  continue;
        }

        break;
    }

    //if( _entry.attrib & _A_SUBDIR )  return string( _entry.name ) + Z_DIR_SEPARATOR;
    return _entry.name; 
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer
