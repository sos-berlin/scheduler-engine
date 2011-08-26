// $Id: dir.cxx 11394 2005-04-03 08:30:29Z jz $
// dir.cxx                                     ©1997-2002 SOS GmbH Berlin
//                                             Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef SYSTEM_WIN
#   include <direct.h>
#else
#   include <unistd.h>
#   include <dirent.h>
#   include <fnmatch.h>
#   include <sys/stat.h>
#   define MAX_PATH 1024
#endif

#include <stack>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"
#include "../kram/sosdate.h"
#include "../file/absfile.h"

namespace sos {   

using std::stack;


//-------------------------------------------------------------------------------------Dir_file

struct Dir_file : Abs_file
{
    struct File_entry
    {
                                    // Ein Destruktor wird nicht aufgerufen!

        Sos_limited_text<MAX_PATH>  _filename;
        Sos_limited_text<MAX_PATH>  _path;
        Sos_optional_date_time      _creation_time;
        Sos_optional_date_time      _write_time;
        Sos_optional_date_time      _access_time;
        Big_int                     _size;
        Bool                        _archived;
        Bool                        _compressed;
        Bool                        _directory;
        Bool                        _hidden;
        Bool                        _read_only;
        Bool                        _system;
        Bool                        _temporary;
    };

    struct Directory
    {
                                    Directory       ( Dir_file*, const string& dir_name, const string& pattern );

        void                        close           ();
        void                        get             ();
        void                        copy_to         ( Area* );

        bool                        is_subdir       () const;

        Fill_zero                  _zero_;                      
        Dir_file*                  _file;
        string                     _dir_name;       // Mit "\\" abgeschlossen

#       ifdef SYSTEM_WIN
            string                  full_path       () const                            { return _dir_name + _data.cFileName; }
            HANDLE                 _handle;
            WIN32_FIND_DATA        _data;
#        else
            string                  full_path       () const                            { return _dir_name + _dirent->d_name; }
            DIR*                   _dir;
            dirent*                _dirent;
            struct stat            _stat;
            string                 _pattern;
#       endif

        bool                       _first;
        bool                       _eof;

    //private:
      //                            Directory       ( const Directory& );             // Nicht implementiert
      //void                        operator =      ( const Directory& );             // Nicht implementiert
    };


                                Dir_file                ();
                               ~Dir_file                ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );
    void                        get_record              ( Area& );

    void                        pop                     ();


    Fill_zero                  _zero_;
    Sos_ptr<Record_type>       _file_entry_type;

    bool                       _recursive;              // Unterverzeichnisse liefern
    bool                       _depth_first;            // Erst Inhalt eines Unterverzeichnisses, dann dessen Name
    bool                       _dot;
    stack<Directory>           _dir_stack;
};


//------------------------------------------------------------------------------------Dir_file_type

struct Dir_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "dir"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Dir_file> f = SOS_NEW( Dir_file );
        return +f;
    }
};

const Dir_file_type            _dir_file_type;
const Abs_file_type&            dir_file_type = _dir_file_type;

//-----------------------------------------------------------------------------------make_full_path
#ifdef SYSTEM_WIN

static string make_full_path( const string& path )
{

    int len;
    char buffer [MAX_PATH], *p;
    //if( path == "" || *(path.end()-1) == ':' )  path += ".";
    len = GetFullPathName( path.c_str(), sizeof buffer, buffer, &p );
    if( len == 0 )  throw_mswin_error( "GetFullPathName", path );
    return string( buffer, len );
}

#endif
//-------------------------------------------------------------------Dir_file::Directory::Directory
#ifdef SYSTEM_WIN

Dir_file::Directory::Directory( Dir_file* file, const string& dir_name, const string& pattern )
: 
    _zero_(this+1), 
    _file(file),
    _handle(INVALID_HANDLE_VALUE) 
{
    _dir_name = dir_name;

    if( *(_dir_name.end()-1) != '\\' )  _dir_name += "\\";

    string filename = _dir_name + pattern;

    _handle = FindFirstFile( filename.c_str(), &_data );

    if( _handle == INVALID_HANDLE_VALUE ) 
    {
        if( GetLastError() == ERROR_NO_MORE_FILES 
         || GetLastError() == ERROR_FILE_NOT_FOUND ) 
        {
            _eof = true;
        }
        else
            throw_mswin_error( "FindFirstFile", filename.c_str() );
    }

    _first = true;
}
#endif
//-----------------------------------------------------------------------Dir_file::Directory::close
#ifdef SYSTEM_WIN

void Dir_file::Directory::close()
{
    if( _handle != INVALID_HANDLE_VALUE )  FindClose( _handle ),  _handle = INVALID_HANDLE_VALUE;
}

#endif
//-------------------------------------------------------------------------Dir_file::Directory::get
#ifdef SYSTEM_WIN

void Dir_file::Directory::get()
{
    while( !_eof )
    {
        if( !_first )
        {
            BOOL ok = FindNextFile( _handle, &_data );
            if( !ok ) {
                if( GetLastError() == ERROR_NO_MORE_FILES )  
                {
                    _eof = true;
                }
                else  
                    throw_mswin_error( "FindNextFile" );
            }
        }

        _first = false;

        if( _file->_dot )  break;
        
        if( strcmp( _data.cFileName, "."  ) != 0  
         && strcmp( _data.cFileName, ".." ) != 0 )  break;
    }
}

#endif
//-------------------------------------------------------------------Dir_file::Directory::is_subdir
#ifdef SYSTEM_WIN

bool Dir_file::Directory::is_subdir() const
{ 

    return ( _data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0
           && strcmp( _data.cFileName, "."  ) != 0
           && strcmp( _data.cFileName, ".." ) != 0;

}

#endif
//---------------------------------------------------------------------Dir_file::Directory::copy_to
#ifdef SYSTEM_WIN

void Dir_file::Directory::copy_to( Area* buffer )
{
    File_entry* e = (File_entry*)buffer->ptr();
    new (e) File_entry;

    e->_filename         = _data.cFileName;
    e->_path             = full_path();
    e->_creation_time    = _data.ftCreationTime;
    e->_write_time       = _data.ftLastWriteTime;
    e->_access_time      = _data.ftLastAccessTime;
    e->_size             = ( (__int64)_data.nFileSizeHigh << 32 ) + _data.nFileSizeLow; 
    e->_directory        = ( _data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
    e->_hidden           = ( _data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN    ) != 0;
    e->_read_only        = ( _data.dwFileAttributes & FILE_ATTRIBUTE_READONLY  ) != 0;
    e->_system           = ( _data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM    ) != 0;
    e->_temporary        = ( _data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY ) != 0;
}

#endif
//-----------------------------------------------------------------------------------make_full_path
#ifndef SYSTEM_WIN

static string make_full_path( const string& path )
{
    return path;
}

#endif
//-------------------------------------------------------------------Dir_file::Directory::Directory
#ifndef SYSTEM_WIN

Dir_file::Directory::Directory( Dir_file* file, const string& dir_name, const string& pattern )
: 
    _zero_(this+1), 
    _file(file)
{
    _dir_name = dir_name;
    if( *(_dir_name.end()-1) != '/' )  _dir_name += '/';

    //if( pattern != "" && pattern != "*" )  throw_xc( "SOS-1452", pattern );
    _pattern = pattern;

    _dir = opendir( dir_name.c_str() );
    if( !_dir )  throw_errno( errno, "opendir", dir_name.c_str() );

    _first = true;
}

#endif
//-----------------------------------------------------------------------Dir_file::Directory::close
#ifndef SYSTEM_WIN

void Dir_file::Directory::close()
{
    if( _dir )  closedir( _dir ),  _dir = NULL;
}

#endif
//-------------------------------------------------------------------------Dir_file::Directory::get
#ifndef SYSTEM_WIN

void Dir_file::Directory::get()
{
    _first = false;

    while( !_eof )
    {
        _dirent = readdir( _dir );
        if( !_dirent )  { _eof = true; return; } //throw_errno( errno, "readdir" );

        if( fnmatch( _pattern.c_str(), _dirent->d_name, 0 ) != 0 )  continue;

        if( _file->_dot )  break;
        
        if( strcmp( _dirent->d_name, "."  ) != 0  
         && strcmp( _dirent->d_name, ".." ) != 0 )  break;
    }


    int ret = lstat( full_path().c_str(), &_stat );
    if( ret == -1 )  throw_errno( errno, "lstat" );
}

#endif
//-------------------------------------------------------------------Dir_file::Directory::is_subdir
#ifndef SYSTEM_WIN

bool Dir_file::Directory::is_subdir() const
{ 
//#   if defined SYSTEM_HPUX || defined SYSTEM_SOLARIS
//        return false;
//#    else
        return    _dirent
               && _stat.st_mode & S_IFDIR
               && !( _stat.st_mode & S_IFLNK )
               && strcmp( _dirent->d_name, "."  ) != 0
               && strcmp( _dirent->d_name, ".." ) != 0;
//#   endif
}

#endif
//---------------------------------------------------------------------Dir_file::Directory::copy_to
#ifndef SYSTEM_WIN

void Dir_file::Directory::copy_to( Area* buffer )
{
    File_entry* e = (File_entry*)buffer->ptr();
    new (e) File_entry;

    e->_filename         = _dirent->d_name;
    e->_path             = full_path();

//#   if !defined SYSTEM_HPUX && !defined SYSTEM_SOLARIS
        e->_directory    = _stat.st_mode & S_IFDIR && !( _stat.st_mode & S_IFLNK );     //_dirent->d_type == DT_DIR;
//#   endif


    e->_creation_time    = _stat.st_ctime;
    e->_write_time       = _stat.st_mtime;
    e->_access_time      = _stat.st_atime;
    e->_size             = _stat.st_size;
  //e->_read_only        = _stat.st_mode ??
  //e->_system           = ( _data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM    ) != 0;
  //e->_temporary        = ( _data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY ) != 0;

    //LOG2( "joacim", "d_name=" << _dirent->d_name << ", d_type=" << (int)_dirent->d_type << "\n" );
}

#endif
//-------------------------------------------------------------------------------------------remark

static void remark( Record_type* t, const char* text ) 
{
    t->field_descr_ptr( t->field_count() - 1 )->_remark = text;
}

//-------------------------------------------------------------------------------Dir_file::Dir_file

Dir_file::Dir_file()
:
    _zero_(this+1)
{
    _file_entry_type = Record_type::create();
    Record_type* t = _file_entry_type;
    File_entry* o = 0;

    t->name( "file_entry" );
    t->allocate_fields( 10 );

    RECORD_TYPE_ADD_LIMTEXT     ( filename     , 0 );   remark( t, "Dateiname ohne Pfad" );
    RECORD_TYPE_ADD_LIMTEXT     ( path         , 0 );   remark( t, "Vollständiger Pfadname" );
    RECORD_TYPE_ADD_FIELD       ( creation_time, 0 );
    RECORD_TYPE_ADD_FIELD       ( write_time   , 0 );
    RECORD_TYPE_ADD_FIELD       ( access_time  , 0 );
    RECORD_TYPE_ADD_FIELD       ( size         , 0 );   remark( t, "Größe der Datei in Bytes" );
    RECORD_TYPE_ADD_FIELD       ( directory    , 0 );
    RECORD_TYPE_ADD_FIELD       ( hidden       , 0 );
    RECORD_TYPE_ADD_FIELD       ( read_only    , 0 );
    RECORD_TYPE_ADD_FIELD       ( system       , 0 );
    RECORD_TYPE_ADD_FIELD       ( temporary    , 0 );
}

//------------------------------------------------------------------------------Dir_file::~Dir_file

Dir_file::~Dir_file()
{
    close(close_normal);
}

//-----------------------------------------------------------------------------------Dir_file::open

void Dir_file::open( const char* parameter, Open_mode open_mode, const File_spec& spec )
{
    Sos_string filename;

    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
        if( opt.flag( 'r', "recursive" ) )      _recursive = opt.set();
        else
        if( opt.flag( "depth-first"    ) )      _depth_first = opt.set();
        else
        if( opt.flag( '.', "dot"       ) )      _dot = opt.set();
        else
        if( opt.param() || opt.pipe() )         filename = opt.rest();
        else 
            throw_sos_option_error( opt );
    }

    if( open_mode & out )  throw_xc( "D127" );

    _depth_first &= _recursive;

    _any_file_ptr->_spec._field_type_ptr = +_file_entry_type;


    if( !_recursive )
    {
        if( filename == ""  ||  *(filename.end()-1) == ':'  )  filename += "*";
        if( filename != ""  &&  ( *(filename.end()-1) == '/' || *(filename.end()-1) == '\\' ) )  filename += "*";
    }

    string dir_name = directory_of_path( filename );    // "dir_name/pattern"
    string pattern  = filename_of_path( filename );

    if( dir_name == "" )  dir_name = ".";               // Kein Verzeichnis? Dann Arbeitsverzeichnis!

    if( pattern == "."  ||  pattern == ".."  || pattern == "" )           // "." und ".." sind kein Pattern.
    {                                                                     // Also auflösen!
        dir_name = make_full_path( filename );
        if( dir_name != ""  &&  *(dir_name.end()-1) == '\\' )  dir_name.erase( dir_name.length()-1 );
        pattern  = filename_of_path( dir_name );
        dir_name = directory_of_path( dir_name );
    }
    else
    {
        dir_name = make_full_path( dir_name );
    }

    _dir_stack.push( Directory( this, dir_name, pattern ) );
}

//----------------------------------------------------------------------------------Dir_file::close

void Dir_file::close( Close_mode )
{
    while( !_dir_stack.empty() )  pop();
}

//-----------------------------------------------------------------------------Dir_file::get_record

void Dir_file::get_record( Area& buffer )
{
    if( _dir_stack.empty() )  throw_eof_error();
  
    buffer.allocate_length( _file_entry_type->field_size() );   //sizeof (File_entry) );
    memset( buffer.ptr(), 0, _file_entry_type->field_size() );  //sizeof (File_entry) );


    if( _depth_first )
    {
        _dir_stack.top().get();

        if( _dir_stack.top()._eof )
        {
            pop();
            if( _dir_stack.empty() )  throw_eof_error();

            _dir_stack.top().copy_to( &buffer );
        }
        else
        {
            while( _recursive & _dir_stack.top().is_subdir() )
            {
                _dir_stack.push( Directory( this, _dir_stack.top().full_path(), "*" ) );
                _dir_stack.top().get();
                if( _dir_stack.top()._eof )  { pop();  break; }
            }

            _dir_stack.top().copy_to( &buffer );
        }
    }
    else
    {
        if( _recursive & !_dir_stack.top()._first & _dir_stack.top().is_subdir() )
        {
            _dir_stack.push( Directory( this, _dir_stack.top().full_path(), "*" ) );
        }

        _dir_stack.top().get();

        while( _dir_stack.top()._eof )
        {
            pop();
            if( _dir_stack.empty() )  throw_eof_error();
            _dir_stack.top().get();
        }

        _dir_stack.top().copy_to( &buffer );
    }
}

//------------------------------------------------------------------------------------Dir_file::pop

void Dir_file::pop()
{
    _dir_stack.top().close();
    _dir_stack.pop();
}

//---------------------------------------------------------------------------------remove_directory

void remove_directory( const string& path, bool force )
{
    LOG( "remove_directory " << path << '\n' );

    int err = rmdir( path.c_str() );
    if( err )
    {
        if( !force  ||  errno != ENOTEMPTY )  throw_errno( errno, "rmdir", path.c_str() );


        Any_file f ( "-in dir -recursive -depth-first " + path );

        if( f.eof() )  throw_errno( errno, "rmdir", path.c_str() );

        while( !f.eof() )
        {
            Record r = f.get_record();
            string path = r.as_string("path");

#           ifdef SYSTEM_WIN
                if( r.as_int("read_only") )  
                {
                    LOG( "attrib " << path << " -r\n" );
                    DWORD attrs = GetFileAttributes( path.c_str() );
                    if( attrs == -1 )  throw_mswin_error( "GetFileAttributes", path.c_str() );

                    BOOL ok = SetFileAttributes( path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY );
                    if( !ok )  throw_mswin_error( "SetFileAttributes", path.c_str() );
                }
#           endif

            if( r.as_int("directory") )  
            {
                LOG( "rmdir  " << path << '\n' );
                err = rmdir( path.c_str() );
                if( err )  throw_errno( errno, "rmdir", path.c_str() );
            }
            else
            {
                LOG( "unlink " << path << '\n' );
                err = unlink( path.c_str() );
                if( err )  throw_errno( errno, "unlink", path.c_str() );
            }
        }

        f.close();
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

