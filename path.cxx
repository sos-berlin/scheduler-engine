// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//--------------------------------------------------------------------------------------------const

const Absolute_path             root_path                   ( "/" );

//-------------------------------------------------------------------------------------------------

static string simplified_path( const string& path )                  
{ 
    string result;
    
    result.reserve( path.length() );

    for( int i = 0; i < path.length(); i++ )
    {
        if( i > 0  &&  path[ i-1 ] == '/'  &&  path[ i ] == '/' )  
        {
            // Doppelten Schrägstrich unterdrücken
        }
        else
            result += path[i];
    }


    // Hier können "." und "xx/.." gekürzt werden

    return result;
}

//---------------------------------------------------------------------------------------Path::Path

Path::Path( const string& folder_path, const string& tail_path ) 
{ 
    if( int len = folder_path.length() + tail_path.length() )
    {
        if( folder_path != ""  &&  *folder_path.rbegin() != folder_separator )
        {
            reserve( len + 1 );
            *this = simplified_path( folder_path );
            *this += folder_separator;
        }
        else
        {
            reserve( len );
            *this = simplified_path( folder_path );
        }

        if( tail_path != "" )
        {
            if( folder_path != ""  &&  *tail_path.begin() == folder_separator )  *this += simplified_path( tail_path.c_str() + 1 );
                                                                           else  *this += simplified_path( tail_path );
        }
    }

    assert( to_string() == simplified_path( *this ) );
}

//-----------------------------------------------------------------------------------Path::set_path

void Path::set_path( const string& path )                  
{ 
    *static_cast<string*>( this ) = simplified_path( path );
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

//--------------------------------------------------------------------------------------Path::depth

//int Path::depth() const
//{
//    const char* p = c_str();
//    
//    if( *p == '/' )  p++;           // Erstes '/' zählt nicht
//
//    while( *p )
//    {
//        if( p[0] == '/'  &&  p[1] )  // Letztes '/' zählt nicht
//        {
//            depth++;
//            assert( p[1] != '/' );
//        }
//    }
//}

//-----------------------------------------------------------------------------------Path::set_name

void Path::set_name( const string& name )
{
    if( name.find( '/' ) != string::npos )  z::throw_xc( "SCHEDULER-417", name );

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
    if( absolute_base == ""  &&  relative == "" )
    {
        set_path( "" );
    }
    else
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

//-------------------------------------------------------Absolute_path::set_simplified_dot_dot_path

void Absolute_path::set_simplified_dot_dot_path( const string& path )                  
{ 
    // Wenn ".." überall erlaubt ist, können wir das in den Konstruktor tun

    if( !string_begins_with( path, "/" ) )  z::throw_xc( Z_FUNCTION, path );

    vector<string> parts     = vector_split( "/", simplified_path( path ) );
    list<string>   part_list;

    for( int i = 1; i < parts.size(); i++ )  
    {
        assert( parts[i] != ""  ||  i == parts.size() - 1 );
        part_list.push_back( parts[i] );
    }

    for( list<string>::iterator p = part_list.begin(); p != part_list.end(); )
    {
        if( *p == "." )  
        {
            p = part_list.erase( p );
        }
        else
        if( *p == ".." )  
        {
            if( p == part_list.begin() )  z::throw_xc( "SCHEDULER-461", path );
            p--;
            assert( *p != ".." );
            p = part_list.erase( p );
            p = part_list.erase( p );
        }
        else 
            p++;
    }

    set_path( "/" + join( "/", part_list ) );
}

//-------------------------------------------------------------------------------------------------

} //namespace folder
} //namespace scheduler
} //namespace sos
