// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//--------------------------------------------------------------------------------------------const

const Absolute_path             root_path                   ( "/" );

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

//------------------------------------------------------------------------------------Path::is_root

bool Path::is_root() const
{ 
    return to_string() == root_path.to_string(); 
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

//---------------------------------------------------------------------------Path::root_folder_name

string Path::root_folder_name() const
{
    size_t s = find( folder_separator );

    return s == string::npos? "" 
                            : substr( 0, s );
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

//-------------------------------------------------------------------------------------------------

} //namespace folder
} //namespace scheduler
} //namespace sos
