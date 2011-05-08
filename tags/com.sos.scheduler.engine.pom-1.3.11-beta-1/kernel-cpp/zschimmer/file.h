// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_FILE_H
#define __ZSCHIMMER_FILE_H

#include <stdio.h>
#include <fcntl.h>

#include "base.h"
#include "file_path.h"
#include "z_io.h"

#ifdef Z_WINDOWS
#   include "z_windows.h"
#endif



namespace zschimmer {

struct Has_log;

namespace io
{
    struct Char_sequence;
}

//-------------------------------------------------------------------------------------------------

namespace file {

struct Mapped_file;

//-------------------------------------------------------------------------------------------------

string                          string_from_file            ( const string& path );
inline string                   string_from_file            ( const File_path& path )               { return string_from_file( path.path() ); }
string                          string_from_fileno          ( int );
//string                          string_from_mapped_file     ( Mapped_file* );
time_t                          modification_time_of_file   ( const string& path );

int64                           z_filelength                ( const char* path );
inline int64                    z_filelength                ( const string& path )                  { return z_filelength( path.c_str() ); }
int64                           z_filelength                ( int file_handle, const string& debug_path = "" );

void                            z_unlink                    ( const char* path );
inline void                     z_unlink                    ( const string& path )                  { return z_unlink( path.c_str() ); }
void                            z_mkdir                     ( const string& );
int                             call_mkdir                  ( const string&, int mode = 0777 );

string                          get_temp_path               ();
//string                        get_temp_filename           ();

string                          directory_of_path           ( const string& path );
string                          filename_of_path            ( const string& path );
string                          basename_of_path            ( const string& path );
string                          extension_of_path           ( const string& path );

//----------------------------------------------------------------------------------------File_base

struct File_base : Object
{ 
                                File_base               ()                                  : _zero_(this+1) {}
                                File_base               ( const File_base& f )              : _zero_(this+1), _path(f._path) {}
    virtual                    ~File_base               ();

    File_base&                  operator =              ( const File_base& f )              { assign( f );  return *this; }
    void                        assign                  ( const File_base& f )              { _last_errno = 0; _path = f._path; _do_unlink = false; }

  //virtual void                open                    ( const string& filename, const string& mode, int rights = 0600 ) = 0;
    virtual void                close                   ()                                  = 0;
    virtual void                write                   ( const void*, size_t )             = 0;
    void                        print                   ( const char* s )                   { write( s, strlen(s) ); }
    void                        print                   ( const string& s )                 { write( s.data(), s.length() ); }
    void                        print                   ( int );
    void                        print                   ( char c )                          { put_char( c ); }
    void                        print                   ( const io::Char_sequence& );
    virtual void                put_char                ( char c )                          { write( &c, 1 ); }
  //void                        printf                  ( const char* format, ... );
    string                      read_string             ( size_t );
    virtual size_t              read                    ( void*, size_t )                   = 0;
    virtual void                seek                    ( int64, int origin = SEEK_SET )    = 0;
    virtual int64               tell                    ()                                  = 0;
    virtual int64               length                  ();
    virtual void                flush                   ()                                  = 0;
    virtual void                sync                    ()                                  = 0;
    virtual void                syncdata                ()                                  { sync(); }
    virtual string              read_all                ();
    int                         last_errno              () const                            { return _last_errno; }
    File_path                   path                    () const                            { return _path; }
    virtual bool                opened                  () const                            = 0;
    virtual bool                eof                     () const                            = 0;
    virtual int                 file_no                 () const                            = 0;
    void                        unlink_later            ( bool b = true )                   { _do_unlink = b; }             // Bei ~File_base
    bool                        try_unlink              ( Has_log* = NULL );
    bool                     is_to_be_unlinked          () const                            { return _do_unlink; }

  protected:
    Fill_zero                  _zero_;
    int                        _last_errno;
    File_path                  _path;
    bool                       _do_unlink;
};

//---------------------------------------------------------------------------------------------File

struct File : File_base, Non_cloneable
{ 
    enum {
        open_private      = 0x01,
        open_unlink       = 0x02,
        open_unlink_later = 0x04,       // ~File entfernt den Dateinamen
        open_inheritable  = 0x08
    } Open_flags;

                                File                    ()                                          : _file(-1), _is_my_file(true), _eof(false) {}
                                File                    ( int file )                                : _file(file), _is_my_file(false), _eof(false) {}
                                File                    ( const string& path, const string& mode );
                                //File                    ( const File& );
                               ~File                    ();

    //File&                       operator =              ( const File& file )                        { assign( file ); }
    //void                        assign                  ( const File& );
    //void                        attach                  ( const File& );

    void                        open                    ( const string& path, const string& mode, int rights = 0600 );
    bool                        try_open                ( const string& path, const string& mode, int rights = 0600 );
    void                        open                    ( const string& path, int           mode, int rights = 0600 );
    bool                        try_open                ( const string& path, int           mode, int rights = 0600 );
    void                        open_temporary          ( int open_flags = 0, const string& a_name = Z_TEMP_FILE_ID );
    void                        close                   ();
    void                        create_temporary        ( int open_flags = 0, const string& a_name = Z_TEMP_FILE_ID );  // Nur anlegen, kein open()

    void                        assign_fileno           ( int f );
    void                        take_fileno             ( int );                                    // close() am Ende

                                operator int            () const                                    { return file_no(); }
    int                         file_no                 () const                                    { return _file; }
    virtual bool                opened                  () const                                    { return _file != -1; }

    void                        write                   ( const void*, size_t );
    size_t                      read                    ( void*, size_t );
    void                        seek                    ( int64, int origin = SEEK_SET );
    int64                       tell                    ();
    void                        truncate                ( int64 new_size );
    void                        flush                   ();
    void                        sync                    ();
    void                        syncdata                ();
    bool                        eof                     () const                                    { return _eof; }
    void                        check_error             ( const string& operation );

#   ifdef Z_WINDOWS
        HANDLE                  handle                  () const;
#   endif


  private:
                                operator bool           () const;

    int                        _file;
    bool                       _is_my_file;
    bool                       _eof;
};

//--------------------------------------------------------------------------------------Mapped_file

struct Mapped_file : File
{
                                Mapped_file             ()                                             : _zero_(this+1) {}
                                Mapped_file             ( const string& path, const string& mode );
                               ~Mapped_file             ();

    void                        open                    ( const string&, const string& mode, int rights = 0600 );
    void                        open                    ( const string&, int           mode, int rights = 0600 );
    void                        close                   ()                                          { unmap(); File::close(); }
    void*                       map                     ();
    void                        unmap                   ();
    size_t                      map_length              ()                                          { map();  return _map_length; }
  //io::Byte_sequence           byte_sequence           ()                                          { return Byte_sequence( map(), map_length() ); }

    string                      as_string               ();

                                operator void*          ()                                          { return ptr(); }
    void*                       ptr                     ()                                          { map();  return _ptr; }


  private:
    Fill_zero                  _zero_;
    void*                      _ptr;
    size_t                     _map_length;

#   ifdef Z_WINDOWS
        windows::Handle        _map;
#   endif
};

//--------------------------------------------------------------------------------------Stream_file

struct Stream_file : File_base, Non_cloneable
{ 
                                Stream_file             ()                                          : _file(NULL), _is_my_file(false) {}
                                Stream_file             ( FILE* file )                              : _file(file), _is_my_file(false) {}
                                Stream_file             ( const string& path, const string& mode )  : _file(NULL), _is_my_file(false) { open( path, mode ); }
                               ~Stream_file             ();

    void                        open                    ( const string& path, const string& mode );
    bool                        try_open                ( const string& path, const string& mode );
    void                        close                   ();
    void                        write                   ( const void*, size_t );
    void                        put_char                ( char );
    size_t                      read                    ( void*, size_t );
    string                      read_line               ();
    void                        seek                    ( int64, int origin = SEEK_SET );
    int64                       tell                    ();
    void                        flush                   ();
    void                        sync                    ();
    void                        syncdata                ();
    void                        throw_error             ( const string& operation );
    virtual bool                opened                  () const                                    { return _file != NULL; }
    bool                        eof                     () const                                    { return feof( _file ) != 0; }
    int                         file_no                 () const;

                                operator FILE*          () const                                    { return _file; }

    FILE*                      _file;
    bool                       _is_my_file;
};

//---------------------------------------------------------------------------File_base_input_stream

struct File_base_input_stream : simple_iunknown_implementation< io::Input_stream >
{
                                File_base_input_stream  ( ptr<File_base> file ) : _file(file) {}

    void                        close                   ()                                          { _file->close(); }
    string                      read_bytes              ( size_t size )                             { return _file->read_string( size ); }

  private:
    ptr<File_base>             _file;
};

//-------------------------------------------------------------------------------------------------

} //namespace file
} //namespace zschimmer

#endif
