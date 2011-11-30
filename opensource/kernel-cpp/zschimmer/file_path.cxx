// $Id: file_path.cxx 13659 2008-09-11 06:35:58Z jz $

#include "zschimmer.h"
#include "file.h"
#include "file_path.h"
#include "directory_lister.h"
#include "log.h"
#include <sys/stat.h>

#ifdef Z_WINDOWS
#   include <direct.h>      // rmdir()
#endif

using namespace std;

namespace zschimmer {
namespace file {

//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "Z-FILEPATH-101", "Path does not denote a directory: '$1'" },
    { "Z-FILEPATH-102", "Path denotes a file beyound the root (too much \"..\")" },
    { "Z-FILEPATH-103", "Drive letter is not allowed in '$1'" },
    { NULL            , NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( file_path )
{
    add_message_code_texts( error_codes );
}

//---------------------------------------------------------------------------is_directory_separator

bool is_directory_separator( char c )
{
    bool result = false;

    if( c == '/' )  result = true;

    #ifdef Z_WINDOWS
        if( c == '\\' )  result = true;
    #endif

    return result;
}

//------------------------------------------------------------------------simplified_path_part_list

//list<string> simplified_path_part_list( const list<string>& part_list )
//{
//    list<string>           result = part_list;
//    list<string>::iterator it     = result.begin(); 
//
//    if( it != part_list.end() )
//    {
//        #ifdef Z_WINDOWS
//        {
//            string first_part = *it;
//
//            if( *first_part.rbegin() == ':' )  it++;                                           // "C:"
//            else
//            if( first_part.length() >= 2  &&  is_directory_separator( first_part[0] )  &&  is_directory_separator( first_part[1] ) )
//            {
//                it++;
//                if( it != part_list.end() )  it++;
//            }
//        }
//        #endif
//
//        if( it->length() == 1  &&  is_directory_separator( *it->begin() ) )  it++;      // "/"
//    }
//
//    while( it != result.end() )
//    {
//        if( *it == "." )
//        {
//            it = result.erase( it );
//        }
//        else
//        if( it != result.begin()  &&  *it == ".." )
//        {
//            it--;
//            
//            if( *it == ".." )    
//            {
//                it++, it++;                 // ../.. ist nicht kürzbar
//            }
//            else
//            {
//                it = result.erase( it );    // Vorgänger
//                it = result.erase( it );    // ".."
//            }
//        }
//        else
//            it++;
//    }
//
//    return result;
//}

//-----------------------------------------------------------------------------File_path::File_path
    
File_path::File_path( const string& directory, const string& tail_path ) 
{ 
    if( int len = directory.length() + tail_path.length() )
    {
        if( directory != ""  &&  *directory.rbegin() != Z_DIR_SEPARATOR[0]  &&  *directory.rbegin() != '/' )
        {
            reserve( len + 1 );
            *this = directory;
            *this += directory_separator( *this );
        }
        else
        {
            reserve( len );
            *this = directory;
        }

        if( tail_path != "" )
        {
            if( directory != ""  &&  ( *tail_path.begin() == Z_DIR_SEPARATOR[0]  ||  *tail_path.begin() == '/' ) )
            {
                *this += tail_path.c_str() + 1;
            }
            else
            {
                *this += tail_path;
            }
        }
    }
}

//-----------------------------------------------------------------------------File_path::File_path
    
//File_path::File_path( const list<string>& part_list ) 
//{ 
//    *this = join( part_list, "" );
//}

//-------------------------------------------------------------------File_path::directory_separator
    
char File_path::directory_separator( const string& example_path )
{
    if( example_path.find( '/' ) != string::npos )  return '/';
    return Z_DIR_SEPARATOR[0];
}

//-------------------------------------------------------------------File_path::directory_separator
    
char File_path::directory_separator( const string& example_path1, const string& example_path2 )
{
    if( example_path1.find( '/' ) != string::npos )  return '/';
    if( example_path2.find( '/' ) != string::npos )  return '/';
    return Z_DIR_SEPARATOR[0];
}

//----------------------------------------------------------------------File_path::is_absolute_path

bool File_path::is_absolute_path() const
{
    bool result = false;

#   if defined Z_WINDOWS

        if( length() >= 3  
         && isalpha( (unsigned char)(*this)[0] )
         && (*this)[ 1 ] == ':' 
         && ( (*this)[ 2 ] == '/' || (*this)[2] == '\\' )  )  result = true;          // "c:\"

        else

        if( length() >= 2  
         && ( (*this)[0] == '/' || (*this)[0] == '\\' )  
         && ( (*this)[1] == '/' || (*this)[1] == '\\' ) )  result = true;           // \\host\share

#   endif

    if( !result )  result = length() >= 1  &&  ( (*this)[0] == '/' || (*this)[0] == Z_DIR_SEPARATOR[0] );       // "/x" gilt auch unter Windows absolut

    return result;
}

//----------------------------------------------------------------------File_path::is_absolute_path

bool File_path::is_relative_path() const
{
    bool is_relative = true;
    
    if( length() >= 0 )
    {
        if( is_relative )  is_relative = (*this)[0] != '/'  &&                  // Fängt nicht mit '/' an
                                         (*this)[0] != Z_DIR_SEPARATOR[0];

        #if defined Z_WINDOWS

            if( is_relative )  is_relative = find( ':' ) == string::npos;       // Enthält kein ':'

        #endif
    }

    return is_relative;
}

//------------------------------------------------------------------------------File_path::set_name
    
void File_path::set_name( const string& name )
{
    string dir = directory();
    if( dir != "" )  dir += directory_separator( dir );
    set_path( dir + name );
}

//-------------------------------------------------------------------------File_path::set_directory

void File_path::set_directory( const string& directory_path )
{
    set_path( File_path( directory_path, name() ) );
}

//---------------------------------------------------------------------File_path::prepend_directory

void File_path::prepend_directory( const string& directory_path )
{
    set_path( File_path( directory_path, path() ) );
    //string path = directory_path;

    //if( path != ""  &&  ( *path.rbegin() == Z_DIR_SEPARATOR[0]  ||  *path.rbegin() == '/' ) )   // directory_path ohne '/' am Ende?
    //{
    //    path += directory_separator( directory_path, *this );                                   // '/' anfügen
    //}

    //if( *this != ""  &&  ( *rbegin() == Z_DIR_SEPARATOR[0]  ||  *rbegin() == '/' ) )            // Name mit '/' am Anfang?
    //{
    //    erase( 0, 1 );                                                                          // '/' entfernen
    //}

    //set_path( path + *this );
}

//-----------------------------------------------------------------------------File_path::directory

File_path File_path::directory() const
{
    return directory_of_path( path() );
}

//----------------------------------------------------------------------------------File_path::name

string File_path::name() const
{
    return filename_of_path( path() );
}

//-----------------------------------------------------------------------------File_path::base_name

string File_path::base_name() const
{
    return basename_of_path( path() );
}

//-----------------------------------------------------------------------------File_path::extension

string File_path::extension() const
{
    return extension_of_path( path() );
}

//----------------------------------------------------------------File_path::assert_not_beyond_root

//void File_path::assert_not_beyond_root( int current_depth )
//{
//    int          depth     = current_depth;
//    list<string> part_list = simplified_path_part_list( this->part_list() );
//    
//    if( !part_list.empty() )
//    {
//        #ifdef Z_WINDOWS
//        {
//            string first_part = *part_list.begin();
//
//            if( *first_part.rbegin() == ':'  ||
//                first_part.length() >= 2  &&  is_directory_separator( first_part[0] )  && is_directory_separator( first_part[1] ) )
//            {
//                z::throw_xc( "Z-FILEPATH-103", *this );       // Erster Teil endet auf ':'? (Laufwerksbuchstabe)
//            }
//        }
//        #endif
//
//        for( list<string>::iterator it = part_list.begin(); it != part_list.end()  &&  *it == ".."; it++ )  depth--;
//    }
//
//    if( depth < 0 )  z::throw_xc( "Z-FILEPATH-102", *this );
//}
//
////----------------------------------------------------------------------------File_path::simplified
//
//File_path File_path::simplified() const
//{
//    return join( "", simplified_path_part_list( part_list() ) );
//}
//
////-----------------------------------------------------------------------------File_path::part_list
//    
//list<string> File_path::part_list() const
//{
//    list<string> result;
//    string       part;
//    const char*  p     = c_str();
//    const char*  p_end = p + length();
//
//    part.reserve( 255 );
//
//    while( p < p_end )
//    {
//        if( is_directory_separator( p[0] ) )
//        {
//            while( p < p_end  &&  is_directory_separator( *++p ) )  p++;
//            if( result.empty() )  result.push_back( "/" );
//        }
//        else
//        {
//            part = "";
//
//            while( p < p_end  &&  !is_directory_separator( p[0] ) )
//            {
//                part += *p++;
//
//                #ifdef Z_WINDOWS
//                    if( *p == ':'  &&  result.empty() )  break;     // "x:" ist eigener Teil
//                #endif
//            }
//
//            assert( part != "" );
//
//            if( p < p_end  Z_WINDOWS_ONLY( && *part.rbegin() != ':' ) )  part += "/";
//            result.push_back( part );
//        }
//    }
//
//    return result;
//}

//-------------------------------------------------------------------------------File_path::compare

int File_path::compare( const File_path& path ) const
{
    string path1 = normalized();
    string path2 = path.normalized();
    return strcmp( path1.c_str(), path2.c_str() );
}

//----------------------------------------------------------------------------File_path::normalized

string File_path::normalized() const
{
#   ifdef Z_WINDOWS

        // Werden alle Großbuchstaben erkannt?
        // Beim Umstellung auf UTF-8 lcase() prüfen!

        string result = lcase( *this );

        for( size_t i = 0; i < result.length(); i++ )  if( result[i] == '/' )  result[i] = '\\';

        return result;

#    else

        return *this;

#   endif
}

//--------------------------------------------------------------------------------File_path::unlink

void File_path::unlink() const
{
    z_unlink( path() );
}

//----------------------------------------------------------------------------File_path::try_unlink

bool File_path::try_unlink( Has_log* log ) const
{
    int err = ::unlink( c_str() );

    if( err )   
    {
        // Doppelt in File::try_unlink(). Zusammenfassen!

        int    errn = errno;
        string msg = message_string( errno_code( errn ), "unlink", *this );

        if( errn != ENOENT )
        {
         if ( log ) {
            if( errn == EACCES )        // permission denied
               log->info( msg );
            else
               log->warn( msg );
         }
         err = 0;
        }

        if( Log_ptr log = "" )
        {
            log << "unlink(\"" << *this << "\")";
            if( err )  log << "  ERROR  " << msg;
            *log << endl;
        }
    }

    return err == 0;
}

//-------------------------------------------------------------------------------File_path::move_to

void File_path::move_to( const File_path& destination_path ) const
{
    File_path d = destination_path;
    if( d.directory() != ""  &&  d.name() == "" )  d.set_name( name() );

    int err = rename( c_str(), d.c_str() );
    int errn = errno;

#   ifdef Z_WINDOWS
        if( err && errno == EEXIST )
        {
            d.try_unlink();
            err = rename( c_str(), d.c_str() );
            errn = errno;
        }
#   endif

    if( Log_ptr log = "" )
    {
        log << "rename(\"" << *this << "\",\"" << d << ")";
        if( err )  log << "  ERRNO-" << errn << " " << z_strerror( errn );
        log << '\n';
    }

    if( err )  
    {
        throw_errno( errno, "rename", c_str(), d.c_str() );
    }
}

//-------------------------------------------------------------File_path::remove_complete_directory

void File_path::remove_complete_directory() const
{
    Z_LOGI( Z_FUNCTION << "\n" );

    list< ptr<File_info> > file_info_list;

    Directory_lister dir ( *this );
    while( ptr<File_info> file_info = dir.get() )  file_info_list.push_back( file_info );
    dir.close();

    for( list< ptr<File_info> >::iterator it = file_info_list.end(); !file_info_list.empty(); )
    {
        File_info* file_info = *--it;

        if( file_info->is_directory() )
        {
            file_info->path().remove_complete_directory();
        }
        else
        {
            file_info->path().unlink();
        }

        file_info_list.erase( it );
    }

    int err = rmdir( c_str() );
    if( err )  throw_errno( errno, "rmdir", Z_FUNCTION );
}

//--------------------------------------------------------------------------------File_path::exists

bool File_path::exists() const
{
    // Unter Windows nicht folgenden Aufruf verwenden, denn der sieht in einem Cache nach. 
    // Die Datei kann schon gelöscht sein (wenigstens bem Zugriff übers Netzwerk)
    // GetFileAttributes( path.c_str() ) != -1;

    struct stat s;
    string      path = *this;
    
    while( path != ""  &&  ( *path.rbegin() == '/'  ||  *path.rbegin() == Z_DIR_SEPARATOR[0] ) )  path.erase( path.length() - 1 );

    int err = ::stat( path.c_str(), &s );
    return !err;
}

//-----------------------------------------------------------------------------File_path::self_test

void File_path::self_test()
{
    {
        File_path p ( "directory", "name" );
        assert( p == "directory" Z_DIR_SEPARATOR "name" );
    }

    {
        File_path p ( "directory", "/name" );
        assert( p == "directory" Z_DIR_SEPARATOR "name" );
    }

    {
        File_path p ( "directory/", "name" );
        assert( p == "directory/name" );
    }

    {
        File_path p ( "directory/", "/name" );
        assert( p == "directory/name" );
    }

    {
        File_path p ( "directory", "name1/name2" );
        assert( p == "directory" Z_DIR_SEPARATOR "name1/name2" );
    }

    {
        File_path p ( "directory", "/name1/name2" );
        assert( p == "directory" Z_DIR_SEPARATOR "name1/name2" );
    }

    {
        File_path p ( "directory/", "name1/name2" );
        assert( p == "directory/name1/name2" );
    }

    {
        File_path p ( "directory/", "/name1/name2" );
        assert( p == "directory/name1/name2" );
    }

    {
        File_path p;
        p.set_directory( "directory" );
        assert( p == "directory" Z_DIR_SEPARATOR );
    }

    {
        File_path p;
        p.set_directory( "directory/" );
        assert( p == "directory/" );
    }

    {
        File_path p ( "directory", "" );
        assert( p == "directory" Z_DIR_SEPARATOR );
    }

    {
        File_path p ( "directory", "" );
        p.set_name( "name1/name2" );
        assert( p == "directory" Z_DIR_SEPARATOR "name1/name2" );
    }

    {
        File_path p ( "dir1/dir2/name" );
        p.set_directory( "directory" );
        assert( p == "directory" Z_DIR_SEPARATOR "name" );
    }

    {
        File_path p ( "/dir1/dir2/name" );
        p.set_directory( "directory" );
        assert( p == "directory" Z_DIR_SEPARATOR "name" );
    }

    {
        File_path p ( "name" );
        assert( !p.is_absolute_path() );
        assert( p.is_relative_path() );
    }

    #if defined Z_WINDOWS
        {
            File_path p ( "/name" );
            assert( p.is_absolute_path() );
            assert( !p.is_relative_path() );
        }

        {
            File_path p ( "c:name" );
            assert( !p.is_absolute_path() );
            assert( !p.is_relative_path() );
        }

        {
            File_path p ( "c:/name" );
            assert( p.is_absolute_path() );
            assert( !p.is_relative_path() );
        }
    #else
        {
            File_path p ( "/name" );
            assert( p.is_absolute_path() );
            assert( !p.is_relative_path() );
        }

        {
            File_path p ( "c:name" );
            assert( !p.is_absolute_path() );
            assert( p.is_relative_path() );
        }

        {
            File_path p ( "c:/name" );
            assert( !p.is_absolute_path() );
            assert( p.is_relative_path() );
        }
    #endif
}

//-----------------------------------------------------------------------------File_info::File_info

File_info::File_info( const File_path& file_path )
: 
    _zero_( this+1 ) 
{ 
    _file_path = file_path; 

    call_stat(); 
}

//-------------------------------------------------------------------------File_info::assert_uint32

void File_info::assert_uint32( time_t t, const string& function )
{
    assert( t <= UINT_MAX );
    if( t > UINT_MAX )  z::throw_xc( function, string_gmt_from_time_t( t ) );
}

//-------------------------------------------------------------------------File_info::set_directory

void File_info::set_directory( bool b )
{
    _is_directory_filled = true;

    if( b ) _st_mode |= S_IFDIR;
      else  _st_mode &= ~S_IFDIR;
}

//--------------------------------------------------------------------------File_info::is_directory

bool File_info::is_directory() 
{ 
    if( !_is_directory_filled )  call_stat();
    return ( _st_mode & S_IFDIR ) != 0; 
}

//-----------------------------------------------------------------------File_info::is_regular_file

//bool File_info::is_regular_file() 
//{ 
//    if( !_st_mode )  call_stat(),  assert( _st_mode );
//    return ( _st_mode & S_IFREG ) != 0; 
//}

//-----------------------------------------------------------------------File_info::set_create_time

void File_info::set_create_time( time_t t )
{ 
    assert_uint32( t, Z_FUNCTION );

    _create_time        = (uint32)t;  
    _create_time_filled = true; 
}

//------------------------------------------------------------------File_info::set_last_access_time

void File_info::set_last_access_time( time_t t )
{ 
    assert_uint32( t, Z_FUNCTION );

    _last_access_time        = (uint32)t;  
    _last_access_time_filled = true; 
}

//-------------------------------------------------------------------File_info::set_last_write_time

void File_info::set_last_write_time( time_t t )
{ 
    assert_uint32( t, Z_FUNCTION );

    _last_write_time        = (uint32)t;  
    _last_write_time_filled = true; 
}

//---------------------------------------------------------------------------File_info::create_time

time_t File_info::create_time()
{
    if( !_create_time_filled )  call_stat(),  assert( _create_time_filled );
    return _create_time;
}

//----------------------------------------------------------------------File_info::last_access_time

time_t File_info::last_access_time()
{
    if( !_last_access_time_filled )  call_stat(),  assert( _last_access_time_filled );
    return _last_access_time;
}

//-----------------------------------------------------------------------File_info::last_write_time

time_t File_info::last_write_time() 
{
    if( !_last_write_time_filled )  call_stat(),  assert( _last_write_time_filled );
    return _last_write_time;
}

//-----------------------------------------------------------------------------File_info::call_stat

void File_info::call_stat()
{
    bool ok = try_call_stat();
    if( !ok )  throw_errno( errno, "stat", _file_path.c_str() );
}

//-------------------------------------------------------------------------File_info::try_call_stat

bool File_info::try_call_stat()
{
    bool         result              = false;
    struct stat  stat_buf;
    string       path                = _file_path;
    bool         should_be_directory = false;

    memset( &stat_buf, 0, sizeof stat_buf );

    if( string_ends_with( path, Z_DIR_SEPARATOR )  ||
        string_ends_with( path, "/"             ) )
    {
        _file_path.erase( path.length() - 1 );
        should_be_directory = true;
    }

    int error = stat( _file_path.c_str(), &stat_buf );
    if( !error )
    {
        read_stat( stat_buf );
        if( should_be_directory  &&  !is_directory() )  z::throw_xc( "Z-FILEPATH-101", _file_path );
        result = true;
    }
    else
    if( errno != ENOENT )  throw_errno( errno, "stat", _file_path.c_str() );

    assert( result || errno );
    return result;
}

//----------------------------------------------------------------------------File_info::call_fstat

void File_info::call_fstat( int file_handle )
{
    struct stat  stat_buf;

    memset( &stat_buf, 0, sizeof stat_buf );

    int error = fstat( file_handle, &stat_buf );
    if( error )  throw_errno( errno, "stat", _file_path.c_str() );

    read_stat( stat_buf );
}

//-----------------------------------------------------------------------------File_info::read_stat

void File_info::read_stat( const struct stat& stat_buf )
{
    set_create_time     ( stat_buf.st_ctime );
    set_last_write_time ( stat_buf.st_mtime );
    set_last_access_time( stat_buf.st_atime );
    _st_mode = stat_buf.st_mode;
    _is_directory_filled = true;
}

//--------------------------------------------------------------File_info::quick_compare_last_write

bool File_info::quick_last_write_less( const File_info* a, const File_info* b )
{ 
    time_t at = a->_last_write_time;     // Muss gefüllt sein!
    time_t bt = b->_last_write_time;     // Muss gefüllt sein!

    if( at < bt )  return true;
    if( at > bt )  return false;

    if( a->_file_path.path() < b->_file_path.path() )  return true;

    return false;
}

//--------------------------------------------------------------File_info::quick_last_write_compare

int File_info::quick_last_write_compare( const File_info* a, const File_info* b )
{ 
    time_t at = a->_last_write_time;     // Muss gefüllt sein!
    time_t bt = b->_last_write_time;     // Muss gefüllt sein!

    if( at < bt )  return -1;
    if( at > bt )  return +1;

    string a_path = a->_file_path.path();   // name() ist vermutlich langsamer als path()
    string b_path = b->_file_path.path();

    if( a_path < b_path )  return -1;
    if( a_path > b_path )  return +1;

    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace file
} //namespace zschimmer
