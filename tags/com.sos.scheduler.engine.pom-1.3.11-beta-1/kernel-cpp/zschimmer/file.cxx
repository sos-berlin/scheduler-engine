// $Id$

#include "zschimmer.h"
#include "file.h"
#include "log.h"
#include "z_io.h"

#include <sys/stat.h>

#ifdef Z_WINDOWS
#   include <io.h>
#   include <share.h>
#   include <direct.h>              // mkdir
#   include "z_windows.h"
#else
#   include <stdio.h>
#   include <unistd.h>
#   include <sys/mman.h>
#   include <pwd.h>
//#   include "z_unix.h"
#endif

#include <errno.h>


#ifndef _MSC_VER
    const int O_BINARY = 0;
#endif

using namespace std;

namespace zschimmer {
namespace file {

//-------------------------------------------------------------------------------------------------

const int                       read_all_block_size         = 100*1024;
const int                       read_line_block_size        = 10*1024;

//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "Z-FILE-101", "open mode ist nicht korrekt angegeben: $1" },
    { "Z-FILE-102", "Datei ist schon geöffnet (doppeltes open)" },
    { "Z-FILE-103", "Dateiposition >= 2GB wird auf diesem Betriebsystem nicht unterstützt" },
    { "Z-FILE-104", "Dateigröße >= 2GB wird nicht unterstützt" },
    { "Z-FILE-105", "Datei ist geschlossen" },
    { NULL         , NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( file )
{
    add_message_code_texts( error_codes );
}

//-------------------------------------------------------------------------------------------------

namespace file
{
    struct Buffer : Non_cloneable
    {
        Buffer( size_t size = 0 ) 
        :
            _buffer ( NULL )
        {
            allocate( size );
        }

        ~Buffer()
        { 
            if( _buffer )  delete [] _buffer; 
        }

        void allocate( size_t size )
        {
            if( _buffer )  delete [] _buffer; 
            _buffer = size == 0? NULL : new char [ size ];
        }

        char* _buffer;
    };
};

//----------------------------------------------------------------------------File_base::~File_base

File_base::~File_base()
{
    if( _do_unlink  &&  _path != "" )  
    {
        try_unlink();
    }
}

//----------------------------------------------------------------------------File_base::try_unlink

bool File_base::try_unlink( Has_log* log )
{
    bool   result = true;
    string msg;

    int err = unlink( _path.c_str() );

    if( err == 0 )
    {
        _last_errno =  0;
        _do_unlink = false;
    }
    else
    {
        _last_errno = errno;

        if( _last_errno != ENOENT )
        {
            msg = message_string( errno_code( _last_errno ), "unlink", _path );
            if( log )  log->warn( msg );
            result = false;
        }
    }


    if( Log_ptr log = "" )
    {
        log << "unlink(\"" << _path << "\")";
        if( err )  log << "  ERROR  " << msg;
        *log << endl;
    }

    return result;
}

//---------------------------------------------------------------------------------File_base::print

void File_base::print( const io::Char_sequence& seq )
{
    write( seq.ptr(), seq.length() );
}

//---------------------------------------------------------------------------------File_base::print

void File_base::print( int i )
{
    char buffer [ 50 ];
    _last_errno = 0;

    int len = z_snprintf( buffer, sizeof buffer, "%d", i );
    write( buffer, len );
}

//---------------------------------------------------------------------------File_base::read_string

string File_base::read_string( size_t size )
{
    file::Buffer buffer ( size );

    size_t length = read( buffer._buffer, size );
    return string( buffer._buffer, length );
}

//------------------------------------------------------------------------------File_base::read_all

string File_base::read_all()
{
    string       result;
    list<string> blocks;
    int          total_length = 0;

    while( !eof() )
    {
        blocks.push_back( read_string( read_all_block_size ) );
        total_length += blocks.rbegin()->length();
    }
    
    result.reserve( total_length );
    Z_FOR_EACH( list<string>, blocks, b )  result.append( *b );

    return result;
}

//--------------------------------------------------------------------------------File_base::length

int64 File_base::length()
{ 
    assert( opened() );  
    return z_filelength( file_no(), _path );    // _path nur für Fehlermeldung
}

//---------------------------------------------------------------------------------------File::File

File::File( const string& path, const string& mode )
:
    _file(-1),
    _is_my_file(false),
    _eof(false)
{
    open( path, mode );
}

//----------------------------------------------------------------------------------------ile::File
    
//File::File( const File& file )
//:
//    _file(-1),
//    _is_my_file(false),
//    _eof(false)
//{
//    *this = file;
//}

//--------------------------------------------------------------------------------------File::~File

File::~File()
{
    try
    {
        if( _is_my_file )  close();
    }
    catch( const exception& x ) 
    {
        Z_LOG( "~File(\"" << _path << "\"): " << x << "\n" );
    }
}

//-------------------------------------------------------------------------------------File::assign

//void File::assign( const File& file )
//{
//    close();
//
//    File_base::assign( file );
//
//    _is_my_file = false;
//    _eof        = false;
//
//    if( file.opened() )  
//    {
//        _file = dup( file.file_no() );
//        if( _file == -1 )  _last_errno = errno,  check_error( "dup" );
//
//        _is_my_file = true;
//    }
//}

//-------------------------------------------------------------------------------------File::attach

//void File::attach( const File& file )
//{
//    close();
//
//    File_base::assign( file );
//
//    _is_my_file = false;
//    _eof        = false;
//    _file       = file._file;
//}

//---------------------------------------------------------------------------------------File::open

void File::open( const string& path, const string& mode, int rights )
{
    bool ok = try_open( path, mode, rights );
    if( !ok )  check_error( "open" );
}

//---------------------------------------------------------------------------------------File::open

bool File::try_open( const string& path, const string& mode, int rights )
{
    int         imode  = O_NOINHERIT;
    const char* p      = mode.c_str();

    while( *p )
    {
        switch( *p )
        {
            case 'r': 
            {
                if( *++p == '+' )  imode |= O_RDWR,  p++;
                             else  imode |= O_RDONLY;
                break;
            }

            case 'w': 
            {
                if( *++p == '+' )  imode |= O_TRUNC | O_CREAT | O_RDWR,  p++;
                             else  imode |= O_TRUNC | O_CREAT | O_WRONLY;
                break;
            }

            case 'a': 
            {
                if( *++p == '+' )  imode |= O_APPEND | O_CREAT | O_RDWR,  p++;
                             else  imode |= O_APPEND | O_CREAT | O_WRONLY;
                break;
            }

            case 'b':
            case 'B':
            {
#               ifdef Z_WINDOWS
                    imode |= _O_BINARY;
#               endif

                p++;
                break;
            }

            case 'I':   // Inherit
            {
                imode &= ~O_NOINHERIT;
                p++;
                break;
            }

            case 'S':   // Sequential
            {
                Z_WINDOWS_ONLY( imode &= ~_O_SEQUENTIAL );
                p++;
                break;
            }
            
            case 'R':   // Random access
            {
                Z_WINDOWS_ONLY( imode &= ~_O_RANDOM );
                p++;
                break;
            }
            


            default: throw_xc( "Z-FILE-101", mode );
        }
    }

    return try_open( path, imode, rights );
}

//---------------------------------------------------------------------------------------File::open

void File::open( const string& path, int mode, int rights )
{
    bool ok = try_open( path, mode, rights );
    if( !ok )  check_error( "open" );
}

//---------------------------------------------------------------------------------------File::open

bool File::try_open( const string& path, int mode, int rights )
{
    bool result = true;

    _last_errno = 0;

    if( opened() )  throw_xc( "Z-FILE-102" );

    _path = path;
    _is_my_file = true;

    Z_LOG( "open(\"" << path << "\")  " );
    _file = ::open( path.c_str(), mode, rights );
    if( _file == -1 )  _last_errno = errno, result = false;
    Z_LOG( "=> " << _file << "\n" );

    _eof = false;

    return result;
}

//-----------------------------------------------------------------------------File::open_temporary

void File::open_temporary( int flags, const string& a_name )
{
    _last_errno = 0;

    if( opened() )  throw_xc( "Z-FILE-102" );

    int pmode = S_IREAD | S_IWRITE;


#   ifdef Z_WINDOWS

        create_temporary( 0, a_name );
        
        int oflag = _O_SHORT_LIVED;
        if(    flags & open_unlink        )  oflag |= O_TEMPORARY;      // close() löscht die Datei
        if( !( flags & open_inheritable ) )  oflag |= O_NOINHERIT;      // Nicht an Prozesse vererben (machen wir zum Default, nur Windows)

        Z_LOG( "sopen(\"" << _path << "\")\n" );
        int errn = _sopen_s( &_file, _path.c_str(), oflag | O_CREAT | O_TRUNC | O_RDWR | O_BINARY, flags & open_private? _SH_DENYRW : _SH_DENYWR, pmode );
        if( errn )  _last_errno = errn, throw_errno( _last_errno, "open", _path.c_str() );

#    else

#       ifdef Z_LINUX

            if( strchr( a_name.c_str(), '/' ) )  _path = a_name;
                                           else  _path = get_temp_path() + "/" + a_name;

            _path += ".XXXXXX";

            Z_LOG( "mkstemp(\"" << _path << "\")  " );
            _file = mkstemp( (char*)_path.c_str() );        // Öffnet die Datei
            if( _file == -1 )  throw_errno( errno, "mkstemp", _path.c_str() );
            Z_LOG( "=> " << _file << ' ' << _path << "\n" );

#        else

            string directory = directory_of_path( a_name );
            string prefix    = filename_of_path( a_name );

            if( directory.empty() )  directory = get_temp_path();

            for( int i = 0; i < 100; i++ )
            {
                Z_LOG( "tempnam(\"" << directory << "\",\"" << prefix << "\")  " );

                char* path_ptr = tempnam( directory.empty()? NULL : directory.c_str(), prefix.c_str() );
                if( !path_ptr )  _last_errno = errno, throw_errno( _last_errno, "tempnam" );

                _path = path_ptr;
                free( path_ptr );

                Z_LOG( "=> " << _path << "\n" );

                Z_LOG( "open(\"" << _path << "\")\n" );
                _file = ::open( _path.c_str(), O_EXCL | O_CREAT | O_RDWR, pmode );
                if( _file == -1 )
                {
                    if( errno == EEXIST )  continue;
                    _last_errno = errno, throw_errno( _last_errno, "open", _path.c_str() );
                }

                break;
            }

#       endif

        if( flags & open_unlink )
        {
            Z_LOG( "unlink(\"" << _path << "\")\n" );
            int ret = unlink( _path.c_str() );                    // Namen sofort wieder löschen
            if( ret == -1 )  _last_errno = errno, throw_errno( _last_errno, "unlink", _path.c_str() );
        }

#   endif

    _do_unlink = ( flags & open_unlink_later ) != 0;
    _eof = false;
}

//-----------------------------------------------------------------------------File::open_temporary

void File::create_temporary( int flags, const string& a_name )
{
    _last_errno = 0;

    if( opened() )  throw_xc( "Z-FILE-102" );


#   ifdef Z_WINDOWS

        char tmp_path [ MAX_PATH ];

        string path = get_temp_path();
        int ok = GetTempFileName( path.c_str(), a_name.c_str(), 0, tmp_path );
        if( !ok )  
        {
            File_path p ( path, a_name + "????.tmp" );
            throw_mswin( "GetTempFileName", p );
        }

        _path = tmp_path;

#    else

        open_temporary( 0, a_name );
        close();

#   endif

    _do_unlink = ( flags & open_unlink_later ) != 0;
}

//--------------------------------------------------------------------------------------File::close

void File::close()
{
    _last_errno = 0;

    if( _file == -1 )  return;

    Z_LOG( "close(" << _file << ") " << _path << "\n" );

    int ret = ::close( _file );
    _file = -1;

    if( ret == -1 )  _last_errno = errno, check_error( "close" );
}

//------------------------------------------------------------------------------File::assign_fileno

void File::assign_fileno( int f )
{
    if( opened() )  throw_xc( "Z-FILE-102" );

    _file = f;
    _is_my_file = false;
    _eof = false;
}

//--------------------------------------------------------------------------------File::take_fileno

void File::take_fileno( int f )
{
    if( opened() )  throw_xc( "Z-FILE-102" );

    _file = f;
    _is_my_file = true;
    _eof = false;
}

//--------------------------------------------------------------------------------------File::write

void File::write( const void* p, size_t len )
{
    _last_errno = 0;

    size_t ret = ::write( _file,  p, len );
    if( ret != len )  _last_errno = errno, check_error( "write" );
}

//---------------------------------------------------------------------------------------File::read

size_t File::read( void* p, size_t len )
{
    _last_errno = 0;

    size_t ret = ::read( _file, p, len );

    if( ret == (size_t)-1 )  _last_errno = errno, check_error( "read" );
    _eof = ret == 0;
    return ret;
}

//---------------------------------------------------------------------------------------File::seek

void File::seek( int64 pos, int origin )
{
    _last_errno = 0;

#   ifdef Z_WINDOWS
        int64 ret = _lseeki64( _file, pos, origin );
#    else
        if( pos >> 31 )  throw_xc( "Z-FILE-103" );
        int ret = lseek( _file, pos, origin );
#   endif

    if( ret == -1 )  _last_errno = errno, check_error( "seek" );
    _eof = false;
}

//---------------------------------------------------------------------------------------File::tell

int64 File::tell()
{
    _last_errno = 0;

#   ifdef Z_WINDOWS
        int64 ret = _telli64( _file );
        if( ret == -1 )  _last_errno = errno, check_error( "_telli64" );
        return ret;
#    else
        int ret = lseek( _file, 0, SEEK_CUR );
        if( ret == -1 )  _last_errno = errno, check_error( "lseek" );
        return ret;
#   endif

}

//-----------------------------------------------------------------------------------File::truncate

void File::truncate( int64 new_size )
{
#   ifdef Z_WINDOWS
        _last_errno = _chsize_s( _file, new_size );
#    else
        int err = ftruncate( _file, new_size );
        if( err )  _last_errno = errno;
#   endif

    check_error( "_chsize_s" );
}

//--------------------------------------------------------------------------------------File::flush

void File::flush()
{
    // Nicht zu tun
}

//---------------------------------------------------------------------------------------File::sync

void File::sync()
{
    _last_errno = 0;

    flush();

#   ifdef Z_WINDOWS
        int ret = _commit( _file );
        if( ret == -1 )  _last_errno = errno, check_error( "_commit" );
#    else
        int ret = fsync( _file );
        if( ret == -1 )  _last_errno = errno, check_error( "fsync" );
#   endif
}

//-----------------------------------------------------------------------------------File::syncdata       

void File::syncdata()
{
#   if defined Z_WINDOWS || defined Z_SOLARIS || defined Z_HPUX

        sync();

#    else

        _last_errno = 0;

        int ret = fdatasync( _file );
        if( ret == -1 )  _last_errno = errno, check_error( "fdatasync" );

#   endif
}

//--------------------------------------------------------------------------------File::check_error

void File::check_error( const string& operation )
{
    if( _last_errno )  throw_errno( _last_errno, operation.c_str(), _path.c_str() );
}

//-------------------------------------------------------------------------------------File::handle
#ifdef Z_WINDOWS

HANDLE File::handle() const
{ 
    return (HANDLE)_get_osfhandle( _file ); 
}

#endif
//-------------------------------------------------------------------------Mapped_file::Mapped_file

Mapped_file::Mapped_file( const string& path, const string& mode ) 
: 
    _zero_(this+1)
{
     open( path, mode );
}

//------------------------------------------------------------------------Mapped_file::~Mapped_file

Mapped_file::~Mapped_file()
{
    try
    {
        unmap();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << ": " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------------Mapped_file::open

void Mapped_file::open( const string& path, const string& mode, int rights ) 
{ 
    File::open( path, mode, rights ); 

    //map(); 
}

//--------------------------------------------------------------------------------Mapped_file::open

void Mapped_file::open( const string& path, int mode, int rights ) 
{ 
    File::open( path, mode, rights ); 

    //map(); 
}

//---------------------------------------------------------------------------Mapped_file::as_string

string Mapped_file::as_string()
{ 
    return string( (const char*)map(), map_length() ); 
}

//---------------------------------------------------------------------------------Mapped_file::map

void* Mapped_file::map()
{
#   ifdef Z_WINDOWS

        if( !_ptr && !_map )
        {
            int64 length64 = this->length();
            if( length64 > SIZE_MAX )  throw_xc( "Z-FILE-104", length64 );
            
            size_t length = (size_t)length64;

            if( length == -1 )  _last_errno = errno, throw_errno( _last_errno, "filelength", _path.c_str() );

            if( length > 0 )
            {
                Z_LOG2( "file.mmap", _path << ": CreateFileMapping(,,PAGE_READONLY,," << length << ")\n" );
                _map = windows::convert_to_noninheritable_handle( CreateFileMapping( (void*)_get_osfhandle( file_no() ), NULL, PAGE_READONLY, 0, length, NULL ) );           
                if( !_map )  throw_mswin( "CreateFileMapping", S() << _path << " (" << length << " bytes" << ")" );

                _ptr = MapViewOfFile( _map, FILE_MAP_READ, 0, 0, length );
                if( !_ptr )  throw_mswin( "MapViewOfFile", _path, S() << length );
            }

            _map_length = length;
        }

#    else
    
        if( _ptr == NULL || _ptr == MAP_FAILED )
        {
            int64 length64 = this->length();
            if( length64 > SIZE_MAX )  throw_xc( "Z-FILE-104", length64 );
            
            size_t length = (size_t)length64;

            if( length > 0 )
            {
                Z_LOG2( "file.mmap", _path << ": mmap(," << length << ",PROT_READ,MAP_SHARED," << file_no() << ")\n" );
            
                _ptr = mmap( NULL, length, PROT_READ, MAP_SHARED, file_no(), 0 );
                if( _ptr == MAP_FAILED )  _last_errno = errno, throw_errno( _last_errno, "mmap", S() << _path << " (" << length << " bytes" << ")" );
            }

            _map_length = length;
        }

#   endif 

    return _ptr;
}

//-------------------------------------------------------------------------------Mapped_file::unmap

void Mapped_file::unmap()
{
#   ifdef Z_WINDOWS

        if( _ptr )  
        {
            Z_LOG2( "file.mmap", _path << ": UnmapViewOfFile()\n" );

            BOOL ok = UnmapViewOfFile( _ptr );
            _ptr = NULL;
            if( !ok )  throw_mswin( "UnmapViewOfFile", _path.c_str() );

            _map.close();
        }

#    else

        if( _ptr && _ptr != MAP_FAILED )
        {
            Z_LOG2( "file.mmap", _path << ": munmap(," << _map_length << ")\n" );

            int err = ::munmap( _ptr, _map_length );
            _ptr = NULL;
            if( err )  _last_errno = errno, throw_errno( _last_errno, "munmap", _path.c_str() );
        }

#   endif 

    _map_length = 0;
}

//-------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------Stream_file::~Stream_file

Stream_file::~Stream_file()
{
    try
    {
        if( _is_my_file )
        {
            close();
        }
    }
    catch( const exception& ) {}
}

//--------------------------------------------------------------------------------Stream_file::open

void Stream_file::open( const string& path, const string& mode )
{
    bool ok = try_open( path, mode );
    if( !ok )  throw_error( "open" );
}

//----------------------------------------------------------------------------Stream_file::try_open

bool Stream_file::try_open( const string& path, const string& mode )
{
    _last_errno = 0;

    if( opened() )  throw_xc( "Z-FILE-102" );

    _path = path;
    
    _is_my_file = true;
    _file = fopen( path.c_str(), mode.c_str() );


    if( _file == NULL )
    {
        _last_errno = errno;
        return false;
    }
    else
        return true;
}

//-------------------------------------------------------------------------------Stream_file::close

void Stream_file::close()
{
    _last_errno = 0;

    if( !_file )  return;

    int ret = fclose( _file );
    _file = NULL;

    if( ret == -1 )  throw_error( "fclose" );
}

//-------------------------------------------------------------------------------Stream_file::write

void Stream_file::write( const void* p, size_t len )
{
    _last_errno = 0;

    size_t ret = fwrite( p, 1, len, _file );
    if( ret != len )  throw_error( "fwrite" );
}

//-----------------------------------------------------------------------------Stream_file::put_char

void Stream_file::put_char( char c )
{
    _last_errno = 0;

    int ret = fputc( c, _file );
    if( ret == -1 )  throw_error( "fputc" );
}

//-------------------------------------------------------------------------------Stream_file::write
/*
void Stream_file::printf( const char* format, ... )
{
    _last_errno = 0;

    int ret = fprintf( _file, format, ... );
    if( ret != -1 )  throw_error();
}
*/
//--------------------------------------------------------------------------------Stream_file::read

size_t Stream_file::read( void* p, size_t len )
{
    _last_errno = 0;

    size_t ret = fread( p, 1, len, _file );
    if( ret == (size_t)-1 )  throw_error( "fread" );
    return ret;
}

//---------------------------------------------------------------------------Stream_file::read_line

string Stream_file::read_line()
{
    string       result;
    file::Buffer buffer ( read_line_block_size );

    while( !eof() )
    {
        _last_errno = 0;
        char* ok = fgets( buffer._buffer, read_line_block_size - 1, _file );
        if( !ok )  
        {
            if( eof() )  return "";
            throw_error( "fgets" );
        }

        int length = strlen( buffer._buffer );
        if( buffer._buffer[ length - 1 ] == '\n' )
        {
            if( length >= 2  &&  buffer._buffer[ length - 2 ] == '\r' )  length--;
            result.append( buffer._buffer, length - 1 );
            break;
        }

        result.append( buffer._buffer, length );
    }
        
    return result;
}

//--------------------------------------------------------------------------------Stream_file::seek

void Stream_file::seek( int64 pos, int origin )
{
    _last_errno = 0;

    if( pos > LONG_MAX )  throw_xc( "Z-FILE-103" );

    int ret = fseek( _file, (long)pos, origin );
    if( ret == -1 )  throw_error( "fseek" );
}

//--------------------------------------------------------------------------------Stream_file::tell

int64 Stream_file::tell()
{
    _last_errno = 0;

    int ret = ftell( _file );
    if( ret == -1 )  throw_error( "ftell" );
    return ret;
}

//-------------------------------------------------------------------------------Stream_file::flush

void Stream_file::flush()
{
    _last_errno = 0;

    int ret = fflush( _file );
    if( ret == -1 )  throw_error( "fflush" );
}

//--------------------------------------------------------------------------------Stream_file::sync

void Stream_file::sync()
{
    _last_errno = 0;

    flush();

#   ifdef Z_WINDOWS
        int ret = _commit( file_no() );
        if( ret == -1 )  throw_error( "_commit" );
#    else
        int ret = fsync( file_no() );
        if( ret == -1 )  throw_error( "fsync" );
#   endif

}

//----------------------------------------------------------------------------Stream_file::syncdata       

void Stream_file::syncdata()
{
#   if defined Z_WINDOWS || defined Z_SOLARIS || defined Z_HPUX

        sync();

#    else

        _last_errno = 0;

        int ret = fflush( _file );
        if( ret == -1 )  throw_error( "fflush" );

        ret = fdatasync( file_no() );
        if( ret == -1 )  throw_error( "fdatasync" );

#   endif
}

////------------------------------------------------------------------------------Stream_file::length
//
//int64 Stream_file::length()
//{ 
//    assert( opened() );  
//    return z_filelength( file_no(), _path );    // _path nur für Fehlermeldung
//}
//
//-----------------------------------------------------------------------------Stream_file::file_no

int Stream_file::file_no() const
{ 
    return fileno( _file ); 
}

//-------------------------------------------------------------------------Stream_file::throw_error

void Stream_file::throw_error( const string& operation )
{
    // _last_errno ist vermutlich 0
    throw_errno( _last_errno, operation.c_str(), _path.c_str() );
}

//-------------------------------------------------------------------------------------------------
/*
    Für eine Klasse, die zeilenweise liest:
    (Line_reader, oder wie heißt die entsprechende Klasse in Java?)

    string line;
    line.reserve( 1000 );

    for( double until = double_from_gmtime() + timeout; until > double_from_gmtime(); )
    {
        string part = vmware_log.read_string( 200 );

        if( part == "" )
        {
            my_log << '.';
            _controller->my_sleep( 0.1 );
        }
        else
        {
            while( part != "" )
            {
                size_t n = part.find( '\n' );

                if( n == string::npos )
                {
                    line += part;
                    part = "";
                }
                else
                {
                    line.append( part.data(), n );
                    part.erase( 0, n + 1 );

                    line.erase( 0, 17 );            // "May 11 12:44:58: "

                    if( regex.match( line ) ) 
                    {
                        my_log << " ok\n";
                        return true;
                    }

                    line = "";
                }
            }
        }
    }
*/
//---------------------------------------------------------------------------------string_from_file

string string_from_file( const string& path )
{ 
    Mapped_file file ( path, "rS" );

    return file.as_string();
    //return string_from_mapped_file( &file );
}

//---------------------------------------------------------------------------------string_from_file

string string_from_fileno( int file_no )
{ 
    Mapped_file file;
    
    file.assign_fileno( file_no );

    return file.as_string();
    //return string_from_mapped_file( &file );
}

//--------------------------------------------------------------------------string_from_mapped_file

//string string_from_mapped_file( Mapped_file* file )
//{
//    if( !file->opened() )  throw_xc( "Z-FILE-105", file->path() );
//
//    off_t ret = lseek( file->_file, 0, SEEK_SET );
//    if( ret == (off_t)-1 )
//    {
//        string text;
//        int    allocate = 10*1024;
//
//        while(1)
//        {
//            char   buffer [ 16*1024 ];
//            size_t length = file->read( &buffer, sizeof buffer );
//            if( length == 0 )  break;
//
//            if( text.capacity() < text.length() + length )
//            {
//                if( allocate < 100*1024*1024 )  allocate = 2*allocate;
//                text.reserve( text.length() + allocate );
//            }
//
//            text.append( buffer, length );
//        }
//
//        return text;
//    }
//    else
//    {
//        return file->as_string();
//    }
//}

//------------------------------------------------------------------------modification_time_of_file

time_t modification_time_of_file( const string& path )
{
    struct stat s;
    int err = ::stat( path.c_str(), &s );                  
    if( err ) throw_errno( errno, "stat", path.c_str() );

    return s.st_mtime;                      // Ist lokale Zeit bei lokalen Dateien, GMT bei Samba (Fehler in Samba?)
}

//-------------------------------------------------------------------------------------z_filelength

int64 z_filelength( const char* path )
{
    struct stat s;

    int err = ::stat( path, &s );
    if( err ) throw_errno( errno, "stat", path );

    return s.st_size;
}

//-------------------------------------------------------------------------------------z_filelength

int64 z_filelength( int file_handle, const string& debug_path )
{
#   ifdef Z_WINDOWS

        int64 result = ::filelength( file_handle );
        if( result == -1 )  throw_errno( errno, "filelength", as_string( file_handle ), debug_path );
        return result;

#    else

        struct stat s;

        int err = ::fstat( file_handle, &s );
        if( err ) 
        {
            int errn = errno;
            throw_errno( errn, "fstat", as_string( file_handle ), debug_path );
        }

        return s.st_size;

#   endif
}

//-----------------------------------------------------------------------------------------z_unlink

void z_unlink( const char* path )
{
    Z_LOG( "unlink " << path << "\n" );
    int err = unlink( path );
    if( err )  throw_errno( errno, "unlink", path );
}

//---------------------------------------------------------------------------------------call_mkdir

int call_mkdir( const string& path, int mode )
{
    #ifdef Z_WINDOWS
        int err = mkdir( path.c_str() );
    #else
        int err = mkdir( path.c_str(), mode );
    #endif

    return err;
}

//------------------------------------------------------------------------------------------z_mkdir

void z_mkdir( const string& path )
{
    int err = call_mkdir( path, 0777 );
    if( err )  z::throw_errno( errno, "mkdir", path.c_str() );
}

//------------------------------------------------------------------------------------get_temp_path

string get_temp_path()
{
#   ifdef Z_WINDOWS

        char buffer [_MAX_PATH+1];

        int len = GetTempPath( sizeof buffer, buffer );
        if( len == 0 )  throw_mswin( "GetTempPath" );

        return string( buffer, len );

#    else

        char* t = getenv( "TMP" );
        string tmp = t? t : "";

        if( tmp == "" ||  tmp == "/tmp" )  
        {
            tmp = "/tmp/";
            int uid = getuid();
            struct passwd* p = getpwuid( uid );
            if( p )  tmp += p->pw_name;
               else  tmp += as_string(uid);

            mkdir( tmp.c_str(), 0777 );       // Fehler ignorieren wir
        }

        return tmp;

#   endif
}

//----------------------------------------------------------------------------directory_of_path
// Liefert alles außer dem Datennamen und Schrägstrich

string directory_of_path( const string& path )
{
    const char* p0 = c_str( path );
    const char* p  = p0 + length( path );

    if( p > p0 )
    {
#       if defined Z_WINDOWS
            if( length(path) >= 2  &&  p0[1] == ':' )  p0 += 2;
            while( p > p0  &&  p[-1] != '/'  &&  p[-1] != '\\'  &&  p[-1] != ':' )  p--;
            if( p > p0+1  &&  ( p[-1] == '/' || p[-1] == '\\' ) )  p--;
#        else
            while( p > p0  &&  p[-1] != '/' )  p--;
            if( p > p0+1  &&  p[-1] == '/' )  p--;
#       endif
    }

    return path.substr( 0, p - c_str(path) );
}

//-----------------------------------------------------------------------------filename_of_path
// Liefert den Dateinamen von "pfad/dateiname"

string filename_of_path( const string& path )
{
    const char* p0 = path.c_str();
    const char* p  = p0 + path.length();

#   if defined Z_WINDOWS
        while( p > p0  &&  p[-1] != ':'  &&  p[-1] != '/'  &&  p[-1] != '\\' )  p--;
#    else
        while( p > p0  &&  p[-1] != '/' )  p--;
#   endif

    return p;
}

//-----------------------------------------------------------------------------basename_of_path
// Liefert den Basisnamen  "pfad/basisname.ext"

string basename_of_path( const string& path )
{
    const char* p0 = c_str( path );
    const char* p2 = p0 + length( path );
    const char* p1;

#   if defined Z_WINDOWS
        while( p2 > p0  &&  p2[-1] != '.'  &&  p2[-1] != '/'  &&  p2[-1] != '\\' )  p2--;
        while( p2 > p0  &&  p2[-1] == '.' )  p2--;
        p1 = p2;
        while( p1 > p0  &&  p1[-1] != '/'  &&  p1[-1] != '\\' )  p1--;
#   else
        while( p2 > p0  &&  p2[-1] != '.'  &&  p2[-1] != '/' )  p2--;
        while( p2 > p0  &&  p2[-1] == '.' )  p2--;
        p1 = p2;
        while( p1 > p0  &&  p1[-1] != '/' )  p1--;
#   endif

    return string( p1, p2 - p1 );
}

//----------------------------------------------------------------------------extension_of_path
// Liefert die Extension "pfad/basisname.ext"

string extension_of_path( const string& path )
{
    const char* p0 = c_str( path );
    const char* p2 = p0 + length( path );

    while( p2 > p0  &&  p2[-1] == ' '  )  p2--;
    const char* p1 = p2;

#   if defined Z_WINDOWS
        while( p1 > p0  &&  p1[-1] != '.'  &&  p1[-1] != '/'  &&  p1[-1] != '\\' )  p1--;
#    else
        while( p1 > p0  &&  p1[-1] != '.'  &&  p1[-1] != '/' )  p1--;
#   endif

    if( p1[-1] != '.' )  p1 = p2;
    return string( p1, p2 - p1 );
}

//-------------------------------------------------------------------------------------------------

} //file
} //zschimmer
