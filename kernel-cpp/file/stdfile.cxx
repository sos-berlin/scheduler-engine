// $Id$

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

// stdfile.cpp
// 15. 3.92                                                    Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"

//#ifndef NDEBUG
//#   define NDEBUG
//#endif

#if !defined _MSC_VER
#   include <values.h>                 // MAXINT
#endif

#include <limits.h>                 // MAX_PATH
#include <stdlib.h>
#include <string.h>

#if defined SYSTEM_WIN && defined JZ_TEST
//#   define USE_WINDOWS
#endif

#if !defined USE_WINDOWS
#   include <sys/stat.h>               // S_IREAD, stat()
#   include <fcntl.h>                  // O_RDONLY

#   if defined __BORLANDC__  ||  defined _MSC_VER
#       include <io.h>       // open(), read() etc.
#       include <direct.h>   // mkdir
#   else
#       include <stdio.h>    // fileno
#       include <unistd.h>   // read(), write(), close()
#   endif
#   include <errno.h>
#endif

#include <ctype.h>

#include "../kram/sosstrg0.h"
#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/soslimtx.h"
#include "../kram/log.h"
#include "../file/absfile.h"
#include "../file/stdfile.h"

namespace sos {


#if defined SYSTEM_WIN || defined SYSTEM_DOS
//    const int O_BINARY = 0;
#endif

//----------------------------------------------------------------------------------------const

#if defined SYSTEM_WIN16
    const uint default_buffer_size = 16*1024;        // Puffergrˆﬂe
#elif defined SYSTEM_WIN32
    // Windows 95 Systemsteuerung, Leistungsmerkmale, Dateisysteme:
    // Windows liest voraus, wenn das Programm in Schritten zu max. 64KB sequentiell liest.
    // Optimal sind offenbar 16KB zum Lesen und zum Schreiben (jedenfalls bei Joacims PC)
    // 64KB bringt wenig mehr, dar¸ber wird's wieder langsamer (wg. malloc)
    const uint default_buffer_size = 16*1024;       // Puffergrˆﬂe
#else
    const uint default_buffer_size = 16*1024;       // Puffergrˆﬂe
#endif

const int std_write_buffer_size = 1024;      // Gilt, wenn !_write_buffered

//---------------------------------------------------------------------------------------------

#if defined USE_WINDOWS
    // Keine Fehlercodes! Nur Fehler oder kein Fehler.
    typedef HFILE File_handle;
    inline HFILE open   ( const char* fname, int mode )     { _lopen( fname, mode ); }
    inline int   close  ( HFILE file )                      { _lclose( file ); }
    inline int   read   ( HFILE file, void* p, int size )   { _lread( file, p, size ); }
#else
    typedef int File_handle;
#endif

struct Std_file : Abs_file
{
                                Std_file                ();
                               ~Std_file                ();

    void                        open                    ( const char*, Open_mode );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );
    void                        rewind                  ( Key::Number = 0 );

    virtual void                set                     ( const Key& );
    void                        flush                   ();
  //File_info                   info                    ();

  protected:
    virtual void                get_record              ( Area& );
  //virtual void                get_position            ( Area* );
    virtual void                get_record_key          ( Area&, const Key& );
    virtual void                put_record              ( const Const_area& );
    void                        insert                  ( const Const_area& );
    void                        update                  ( const Const_area& );

  private:
    void                       _init                    ();
    void                        write_record            ( const Const_area& );
    void                        buffer_seek             ( size_t );
    void                        flush_read_buffer       ();
    void                        flush_write_buffer      ();
    void                        buffered_write          ( const void*, int );

    Fill_zero                  _zero_;
    File_handle                _file;
    bool                       _dont_close;
    int                        _column;                 // F¸r Tabulator-Expansion
    size_t                     _pos;                    // == tell( _file )
    size_t                     _record_pos;             // Position des zuletzt gelesenen Satzes
    Bool                       _nl;                     // Datei mit Newline (LF oder CR/LF)
    Bool                       _nonl;                   // '\n' im Satz nicht erlaubt!
    Bool                       _at_end_of_record;
    Bool                       _no_tabs;
    Bool                       _epipe;                  // errno == EPIPE aufgetreten
    Bool                       _eof;
    char                       _record_end_char;        // default: '\n'
    char                       _key_end_char;
    Dynamic_area               _current_key;
    Bool                       _emulate_key;
    uint                       _fixed_length;
    Dynamic_area               _buffer;
    size_t                     _buffer_pos;             // Position (tell) von _buffer in der Datei
    uint                       _min_buffer_size;
    uint                       _max_buffer_size;
    uint                       _write_buffer_size;
    uint                       _buffer_size;
    char*                      _read_ptr;
    char*                      _read_end;
    Bool                       _update_allowed;
    Bool                       _at_end;
    Bool                       _sam3;
    Bool                       _write_buffered;
    Bool                       _write_buffer_filled;
    Bool                       _log;
    int                        _get_length;
    Sos_limited_text<2>        _nl_string;         // "\n" oder "\r\n"
};


//----------------------------------------------------------------------statics

struct Std_file_type : Abs_file_type
{
                                Std_file_type           () : Abs_file_type() {};

    virtual const char*         name                    () const { return "file"; }

    virtual Sos_ptr<Abs_file>   create_base_file() const
    {
        Sos_ptr<Std_file> f = SOS_NEW_PTR( Std_file );
        return +f;
    }

    virtual void                erase                   ( const char* path );
    virtual void                rename                  ( const char* old_path, const char* new_path );
};

const Std_file_type   _std_file_type;
const Abs_file_type&   std_file_type = _std_file_type;

//---------------------------------------------------------------check_filename

void check_filename( const char* filename )
{
#   ifdef SYSTEM_WIN16
        const char* p = filename;
        int         n = 8;

        while( *p ) {
            if( *p == ':'  ||  *p == '\\'  ||  *p == '/' )  n = 8;
            else 
            if( *p == '.' ) n = 3;
            else 
            {
                if( *p == ' ' )  throw_xc( "SOS-1366", filename );
                if( n == 0 )  throw_xc( "SOS-1365", filename );
                n--;
            }
            p++;
        }
#   endif
}

//---------------------------------------------------------Std_file_type::erase

void Std_file_type::erase( const char* path )
{
    int ret = unlink( path );
    if( ret == -1 )  throw_errno( errno, path );
}

//--------------------------------------------------------Std_file_type::rename

void Std_file_type::rename( const char* old_path, const char* new_path_par )
{
    string new_path;
    bool has_dir = false;

    if( strchr( new_path_par, '/' ) != NULL )  has_dir = true;

#   ifdef SYSTEM_WIN
        if( strchr( new_path_par, '\\' ) != NULL )  has_dir = true;
#   endif

    if( has_dir )  new_path = new_path_par;
             else  new_path = directory_of_path( old_path ) + "/" + new_path_par;       // Kein Verzeichnis? Dann Verzeichnis des alten Namens nehmen

    int ret = ::rename( old_path, new_path.c_str() );
    if( ret == -1 )  throw_errno( errno, old_path );
}

//-----------------------------------------------------------Std_file::Std_file

Std_file::Std_file()
:
    _zero_ (this+1),
    _min_buffer_size ( 256 )
{
    _init();
}

//----------------------------------------------------------Std_file::~Std_file

Std_file::~Std_file()
{
    close();        // Falls flush_write_buffer() beim vorangehenden close() schiefgegangen ist.
}

//---------------------------------------------------------------Std_file::_init

void Std_file::_init()
{
    _file               = -1;
    _nl                 = true;
    _record_end_char    = '\n';
    _no_tabs            = true;
    _key_len            = sizeof (long);      // lseek
    _max_buffer_size    = default_buffer_size;

#   if NL_IS_CRLF
        _nl_string = "\r\n";
#    else
        _nl_string = "\n";
#   endif
}

//---------------------------------------------------------------Std_file::open

void Std_file::open( const char* filename, Open_mode open_mode )
{
    char                fn [ _MAX_PATH + 1 ];
    Sos_option_iterator opt ( filename );

#   if defined SYSTEM_WIN  // Unter Unix benutzt read_profile_bool Std_File, was zur Rekursion f¸hrt
        static Bool log_read = false;
        if( !log_read ) {
            log_read = true;
            _log = read_profile_bool( "", "stdfile", "log", false );
        }
#   endif

    if( open_mode & binary )  _nl = false; 

    for( ; !opt.end(); opt.next() )
    {
        if( opt.with_value( "fixed-length"   ) )  { _fixed_length = opt.as_uintK(); _nl = false; }
        else
        if( opt.flag      ( "sam3"           ) )  { _sam3 = opt.set(); _nl = !_sam3; }
        else
        if( opt.flag      ( "nl"             ) )  { _nl = opt.set(); }
        else
        if( opt.flag      ( "nonl"           ) )  { _nonl = opt.set(); }
        else
        if( opt.flag      ( "win"            ) )  { _nl_string = "\r\n"; }
        else
        if( opt.flag      ( "unix"           ) )  { _nl_string = "\n"; }
        else
        if( opt.flag      ( "b"              ) )  { _nl = false; _write_buffered = true; if( open_mode & in )  _fixed_length = 4096; }  // bin‰r gepuffert
        else
        if( opt.with_value( "b"              ) )  { _nl = false; _write_buffered = true; _write_buffer_size = opt.as_uintK(); if( open_mode & in )  _fixed_length = opt.as_uintK(); }  // bin‰r gepuffert
        else
        if( opt.with_value( "buffer-size"    ) )  _max_buffer_size = opt.as_uintK();
        else
        if( opt.flag      ( "write-buffered" ) )  _write_buffered = opt.set();
        else
        if( opt.flag      ( "log"            ) )  _log = opt.set();
        else
        if( opt.with_value( "record-end"     ) ) { _record_end_char = (char) as_short(c_str(opt.value())); _nl = true; }
        else
        if( opt.with_value( "key-end"      ) )  {
            if( length( opt.value() ) == 1 )  _key_end_char = opt.value()[ 0 ];
            else
            if( opt.value() == "tab"
             || opt.value() == "ht"
             || opt.value() == "\\t" )  _key_end_char = '\t';
            else throw_xc( "SOS-1271", c_str( opt.value() ) );
        }
        else
            break;
    }


    if( _sam3 )  _nl = false,  _fixed_length = false;

    if( open_mode & binary  &&  !_sam3  &&  !_nl )  {   // wie -b, jz 17.9.97, 12.11.97
        _write_buffered = true; 
        if( open_mode & in  &&  !_fixed_length )  _fixed_length = 4096; 
    }

    _write_buffered = false;   // funktioniert nicht (MS VC++ 4.2) 17.4.97

#   if defined SYSTEM_WIN16
        if( _max_buffer_size > 16*1024 )  _max_buffer_size = 16*1024;
#   endif
    if( _write_buffered  &&  !_write_buffer_size )  _write_buffer_size = _max_buffer_size;

    if( !_nl )  _no_tabs = true;

    strncpy( fn, c_str( opt.rest() ), sizeof fn - 1);
    fn[ sizeof fn - 1 ] = 0;

    if( empty( fn ) )  throw_xc( "D105" );
/* jz 3.11.98
    {
        char* p = strchr( fn, '(' );

        if( p ) {           // "library(member)"  ==> "library/member"
            int n = strlen( p ) - 1;
            if( position( p, ')' ) == n ) {
                *p = '/';
                p[ n ] = '\0';
            }
        }
    }
*/
    _dont_close = false;

#if !defined SYSTEM_WIN16
    if( strcmpi( fn, "*stdin"      ) == 0
     || strcmp ( fn, "/dev/stdin"  ) == 0 )  _file = 0, _dont_close = true; //stdin;
    else
    if( strcmpi( fn, "*stdout"     ) == 0
     || strcmp ( fn, "/dev/stdout" ) == 0 )  { _file = 1; _dont_close = true;  _at_end = true; }  //stdout;
    else
    if( strcmpi( fn, "*stderr"     ) == 0
     || strcmp ( fn, "/dev/stderr" ) == 0 )  { _file = 2; _dont_close = true;  _at_end = true; }  //stderr;
    else
    if( strcmp( fn, "-" ) == 0 ) {
        if( (open_mode & in) && !(open_mode & out) )  { _file = 0;  _dont_close = true;  _at_end = true; }  
        else
            if( (open_mode & out) && !(open_mode & in) )  { _file = 1;  _dont_close = true;  _at_end = true; }  
        else throw_xc( "SOS-1236" );
    }
    else
#endif
    {
#if 0
        // js: am 4.8.98 ausgebaut
        char* ptr = strchr( fn, ' ');
        if ( ptr ) { // Test auf nocrlf (muss genau Åbereinstimmen)
            char* ptr2 = strchr( ptr, 'n' );
            if (!ptr2) ptr2 = strchr( ptr, 'N' );
            if (ptr2) {
                if ( strncmpi( ptr2, "nocrlf nocrlf", 13 ) == 0 ) {
                    _nl = false;
                    memset( ptr2, ' ', 13 );
                }
                else if ( strncmpi( ptr2, "nocrlf", 6 ) == 0 ) {
                    _nl = false;
                    memset( ptr2, ' ', 6 );
                } else { // Optimierung s.u. for-Schleife
                    throw_syntax_error( "D104", filename );
                };
            };
            for ( ; (char) (*ptr) == ' '; ptr++ );
            if ( *ptr != '\0')  throw_syntax_error( "D104", filename );
        }
#endif

        int access = 0;
        if( !( open_mode & in     ) )  access |= O_WRONLY;
        if( !( open_mode & out    ) )  access |= O_RDONLY;
        if( ( ( open_mode & (in|out) ) == (in|out) ) )  access |= O_RDWR;
        if(    open_mode & app      )  access |= O_APPEND;
        if( !( open_mode & nocreate)
         &&  ( open_mode & out     ))  access |= O_CREAT;
        if(    open_mode & noreplace)  access |= O_EXCL;
        if(    open_mode & trunc    )  access |= O_TRUNC;
        if(    open_mode & binary   )  access |= O_BINARY;

        if( (open_mode & out) && !(open_mode & (in|app|noreplace)) )  access |= O_TRUNC;

        check_filename( fn );

        if( _log ) LOG( "open(\""<< fn << "\"," << (access | O_BINARY) << ',' << (S_IREAD_all|S_IWRITE_all) << ')' );
        _file = ::open( fn, access | O_BINARY | O_NOINHERIT, S_IREAD_all | S_IWRITE_all );
        if( _log ) LOG( " liefert " << _file << '\n' );

        if( _file == -1 )  throw_errno( errno, "open", this );

      //? if (open_mode & in )  { lseek( _file, 0, SEEK_BEG ); }

		_at_end  = ( access & ( O_APPEND | O_TRUNC ) ) != 0;
        _no_tabs = (open_mode & File_base::tabs) ? false : true;
    }
}

//---------------------------------------------------------------Std_file::open

void Std_file::open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    open( filename, open_mode );

    if( file_spec.key_specs().key_length() ) {
        _emulate_key  = true;
        _key_len      = file_spec.key_specs().key_length();
        _key_pos      = _key_len? file_spec.key_specs().key_position() : (uint)-1;
    }
    else {
        //Field_descr* f = file_spec.key_specs()._key_spec._field_descr_ptr;
        //if( f ) {
        //    _emulate_key  = true;
        //    _key_len   = f->type().field_size();
        //    _key_pos = f->offset();
        //}
    }

    _current_key.allocate_min( _key_len );

    //_current_key = Const_area( _current_key_value_ptr, _key_len );
    current_key_ptr( &_current_key );
}

//------------------------------------------------------------------------------Std_file::close

void Std_file::close( Close_mode )
{
    if( _file != -1 )
    {
        if( _write_buffer_filled )   flush_write_buffer();  // Bei Exception schlieﬂt ~Std_file() die Datei

        if( !_dont_close ) {   // nicht stdin, stdout oder stderr
            if( _log )  LOG( "close(" << _file << ")\n" );
            int rc = ::close( _file );
            _file = -1;
            if( rc == -1 )  throw_errno( errno, "close", this );
        }
    }
}

//------------------------------------------------------------------Std_file::flush_read_buffer

inline void Std_file::flush_read_buffer()
{
    _buffer_size = 0;
    _read_ptr    = NULL;
    _read_end    = NULL;
    _buffer_pos  = 0;
}

//-----------------------------------------------------------------------------Std_file::rewind

void Std_file::rewind( Key::Number )
{
    if( _write_buffer_filled )  flush_write_buffer();
    buffer_seek( 0 );
}

//-------------------------------------------------------------------------Std_file::get_record

void Std_file::get_record( Area& area )
{
    if( _write_buffer_filled )  flush_write_buffer();

    //jz 8.4.97 if( !_emulate_key )  _current_key.assign( &_pos, sizeof _pos );

    if( _fixed_length )
    {
        // if( !_read_pos_valid )  lseek( _pos )
        _record_pos = _pos;

        area.allocate_min( _fixed_length );
        if( _log )  LOG( "read(" << _file << ',' << area.ptr() << "," << _fixed_length << ")  _pos=" << _pos << "\n" );
        int len = read( _file, area.ptr(), _fixed_length );

        if( len < _fixed_length ) {
            if( len == -1 )  throw_errno( errno, "read", this );
            if( len == 0 )  throw_eof_error( "D310", this );
        }

        _pos += len;
        area.length( len );       // Der letzte Satz kann k¸rzer sein!
    }
    else
    {
        int  len_count     = 3;   // _sam3: Soviele L‰ngenbytes noch zu lesen
        int  rest_length   = 0;   // _sam3

        _record_pos = _read_ptr? _buffer_pos + ( _read_ptr - _buffer.char_ptr() )
                               : _pos;
        area.length( 0 );

        while(1) {   // bis '\n' bzw. bei -sam3 bis Satzende
            if( _read_ptr == _read_end )
            {
                int len;

                if( _eof ) {
                    len = 0;
                } else {
                    if( _buffer_size == 0 )  _buffer_size = _min_buffer_size;
                    else
                    if( _buffer_size <= _max_buffer_size / 2 && _buffer_size != _pos )  _buffer_size = 2 * _buffer_size; // Damit immer auf 2er-Potenz

                    _read_ptr   = NULL;
                    _read_end   = NULL;
                    _buffer_pos = _pos;
                    _buffer.allocate_min( _buffer_size );

                    if( _log )  LOG( "read(" << _file << ',' << _buffer.ptr() << "," << _buffer_size << ")  _buffer_pos=" << _buffer_pos << "\n" );
                    len = read( _file, _buffer.ptr(), _buffer_size );

                    if( len == -1 ) {
                        if( errno == EACCES /*permission denied, Bodo*/ )  LOG( "read(" << _file << ',' << _buffer.ptr() << "," << _buffer.size() << ")  _pos=" << _pos << "\n" );
                        throw_errno( errno, "read", this );
                    }

                    _pos += len;

                    //jz 25.1.97 if( len < _buffer_size )  _eof = true;
                    if( len == 0 )  _eof = true;
                }

                if( len == 0 ) {  // EOF?
                    if( area.length() > 0 )  goto OK;   // Satz ohne '\n'
                    throw_eof_error( "D310", this );
                }

                _buffer.length( len );
                _read_ptr = _buffer.char_ptr();
                _read_end = _read_ptr + len;
            }

            if( _sam3 ) {   // 3 Bytes Satzl‰ngenfeld (little endian)
                while( len_count  &&  _read_ptr < _read_end ) {
                    rest_length += (int4)*(Byte*)_read_ptr++ << ( --len_count * 8 );
                }
                int4 l = _read_end - _read_ptr;
                if( l > rest_length )  l = rest_length;
                area.append( _read_ptr, l );
                _read_ptr += l;
                rest_length -= l;
                if( rest_length == 0 ) {
                    if( len_count == 0 )  break;  // Fertig
                    if( len_count == 3 )  throw_eof_error();
                }
            }
            else
            {   // '\n' ist Satzende
                char* p = _read_ptr;
                char* z = (char*)memchr( p, _record_end_char, _read_end - p );
                if( !z )  z = _read_end;
                _read_ptr = z;

                while(1) {   // bis alle '\t' abgearbeitet
                    char* p2 = _no_tabs? 0 : (char*)memchr( p, '\t', z - p );
                    if( !p2 )  p2 = z;

                    area.append( p, p2 - p );

                    if( p2 == z )  break;

                    uint n = ( area.length() + 8 ) & ~7;
                    area.resize_min( n );
                    memset( area.char_ptr() + area.length(), ' ', n - area.length() );
                    area.length( n );

                    p = p2 + 1;  // \t ¸berspringen
                    if( p == z )  break;
                }

                _read_ptr = z;
                if( _read_ptr < _read_end )  break;
            }
        }

        if( !_sam3 && _record_end_char == '\n' ) {
            if( _read_ptr && *_read_ptr == _record_end_char )  _read_ptr++;  // '\n' ¸berspringen

            if( area.length() > 0  &&  area.char_ptr()[ area.length() - 1 ] == '\r' ) {
                area.length( area.length() - 1 );
                area.char_ptr()[ area.length() ] = '\0';
            }
        }
    }

OK:
    if( _emulate_key ) {     // Auch bei Truncate!!!
        int klen = min( (uint)_key_len, area.length() - _key_pos );
        if( klen > 0 ) {
            const Byte* p = area.byte_ptr() + _key_pos;

            if( _key_end_char ) {
                const Byte* e = (Byte*)memchr( p, _key_end_char, klen );
                if( e )  klen = e - p;
            }
            if( area.length() >= _key_pos + klen ) {
                memcpy( _current_key.ptr(), p, klen );
                _current_key.length( klen );
            }
        }
    } else {  //jz 8.4.97
        _current_key.assign( &_record_pos, sizeof _record_pos );
    }

    if( _buffer_pos > 0 ) {
        while( _min_buffer_size < area.length()  &&  _min_buffer_size < INT_MAX / 2 )  _min_buffer_size *= 2;
    }
    else ; // 2er-Potenzen beachten

    _update_allowed = true;
    _get_length = area.length();

#   if defined SYSTEM_WIN
#       ifndef __BORLANDC__     // jz 12.1.99 (WBRZ) 16bit-Stack vielleicht zu klein?
// jz 24.6.99 (Fehler bei stdin)    assert( _pos == tell( _file ) );
#       endif
#   endif
}

//-----------------------------------------------------------------Std_file::set

void Std_file::set( const Key& key )
{
    if( _write_buffer_filled )  flush_write_buffer();

    _update_allowed = false;

    if( _emulate_key )
    {
        Dynamic_area buffer ( 1024+1 );   // Um Too_long_error zu vermeiden

        try {
            get_record_key( buffer, key );
        }
        catch( const Not_found_error& ) {}

        buffer_seek( _record_pos );
    }
    else
    {
        size_t pos;
        memcpy( &pos, key.ptr(), sizeof _pos );
        buffer_seek( pos );
    }
}

//---------------------------------------------------------------------Std_file::get_record_key

void Std_file::get_record_key( Area& area, const Key& key )
{
    if( _write_buffer_filled )  flush_write_buffer();

    int  cmp;
    Bool truncated;

    _update_allowed = false;
    if( key.length() == 0 )  throw_xc( "KEYLEN=0", this );
    if( key.length() > _key_len )  throw_xc( "stdfile-key", "key.length() > _key_len" );

    //_buffer_size = 0;  _read_ptr = _read_end = 0;

    if( _emulate_key )
    {
        int first_cmp = _current_key.length()? memcmp( key.ptr(), _current_key.ptr(), _key_len ) : -1;

        if( first_cmp == 0 ) {
            buffer_seek( _record_pos );
            get_record( area );
        }
        else
        {
            if( first_cmp < 0 )  buffer_seek( 0 );

            while(1) {
                try {
                    get_record( area );
                    truncated = false;
                }
                catch( const Too_long_error& ) { truncated = true; }
                catch( const Eof_error& )      { throw_not_found_error( "D311", this ); }

                if( _key_end_char ) {
                    if( area.length() > _key_pos ) {
                        int klen = min( key.length(), area.length() - _key_pos );
                        const Byte* p = area.byte_ptr() + _key_pos;
                        const Byte* e = (Byte*)memchr( p, _key_end_char, klen );
                        if( e )  klen = e - p;
                        cmp = memcmp( p, key.ptr(), klen );
                        if( cmp == 0 ) {
                            for( int i = klen; i < key.length(); i++ ) {
                                if( key.char_ptr()[ i ] > ' ' )  { cmp = -1; break; }
                            }
                        }
                    }
                }
                else
                if( area.length() >= _key_pos + key.length() ) {
                    cmp = memcmp( area.char_ptr() + _key_pos, key.ptr(), key.length() );
                    if( cmp >= 0 )  break;
                }
            }

            if( cmp >  0  )  { throw_not_found_error( "D311", this ); }
            if( truncated )  { throw_too_long_error(); }
        }
    }
    else
    {
        size_t pos;
        memcpy( &pos, _current_key.ptr(), sizeof (long) );
        buffer_seek( pos );
        get_record( area );
    }

#   if defined SYSTEM_WIN
        assert( _pos == tell( _file ) );
#   endif
}

//------------------------------------------------------------------------Std_file::buffer_seek

void Std_file::buffer_seek( size_t pos )
{
    if( _write_buffer_filled )  flush_write_buffer();

//?? jz 7.4.97 (loga04.sql: buffer_seek(0) positioniert falsch auf ca. 6553X)
    if( pos >= _buffer_pos  &&  pos < _buffer_pos + _buffer.length() )
    {
        if( _log )  LOG( "Std_file::buffer_seek pos=" << pos << ", _buffer_pos=" << _buffer_pos << ", _buffer.length()=" << _buffer.length() << '\n' );
        _read_ptr = _buffer.char_ptr() + pos - _buffer_pos;
    }
    else
    {
        flush_read_buffer();
        _at_end = false;

        if( _log )  LOG( "lseek(" << _file << ',' << pos << ")\n" );
        int rc = lseek( _file, pos, SEEK_SET );
        if( rc == -1 )  throw_errno( errno, "lseek", this );

        _pos = pos;
        _eof = false;
    }
}

//-------------------------------------------------------------------------Std_file::put_record

void Std_file::put_record( const Const_area& area )
{
    int rc;

    _update_allowed = false;
    _current_key.length( 0 );
    flush_read_buffer();

    if( _nonl && memchr( area.ptr(), '\n', area.length() ) )  throw_xc( "SOS-1395", this );

    if( area.length() > (uint)MAXINT)  throw_too_long_error( "D420", this );

    if( _fixed_length  &&  area.length() != _fixed_length ) {
        Msg_insertions ins;
        ins.append( area.length() );
        ins.append( _fixed_length );
        throw_xc( "D422", ins );
    }

    if( !_at_end )  {
        if( _log )  LOG( "lseek(" << _file << ",0,SEEK_END)\n" );
        rc = lseek( _file, 0, SEEK_END );
        if( rc == -1 )  {
            if( errno != ESPIPE ) throw_errno( errno, "lseek", this );
        }
        _at_end = true;
    }

    _eof = false;
    write_record( area );
}

//-----------------------------------------------------------------------Std_file::write_record

void Std_file::write_record( const Const_area& area )
{
    if( _sam3 ) {
        Byte l [ 3 ];
        l[ 0 ] = Byte( area.length() >> 16 );
        l[ 1 ] = Byte( area.length() >> 8  );
        l[ 2 ] = Byte( area.length()       );

        buffered_write( l, 3 );
    }

    buffered_write( area.char_ptr(), area.length() );

    if( _nl ) {
        if( _record_end_char == '\n' ) { // Normalfall
            buffered_write( _nl_string.char_ptr(), _nl_string.length() );
        } else {
            char buf[2]; buf[1] = '\0'; buf[0] = _record_end_char;
            buffered_write( buf, 1 );
        }
    }

    if( !_write_buffered )  flush_write_buffer();
}

//-----------------------------------------------------------------------------Std_file::insert

void Std_file::insert( const Const_area& record )
{
    put_record( record );
}

//-----------------------------------------------------------------------------Std_file::udpate

void Std_file::update( const Const_area& record )
{
    if( !_update_allowed )  throw_xc( "SOS-1233" );
    _update_allowed = false;
    _current_key.length( 0 );
    flush_read_buffer();

    if( record.length() != _get_length  )  throw_xc( "SOS-1234", record.length(), _get_length );

    if( _epipe )  throw_errno( EPIPE, "Std_file::update", this );

    _at_end = false;
    if( _log )  LOG( "lseek(" << _file << ',' << _record_pos << ",SEEK_SET)\n" );
    int rc = lseek( _file, _record_pos, SEEK_SET );
    if( rc == -1 )  {
        if( errno == EPIPE )  _epipe = true;  // Solaris sagt das nur einmal, danach crash
        throw_errno( errno, "lseek", this );
    }

    write_record( record );
}

//---------------------------------------------------------------------Std_file::buffered_write

void Std_file::buffered_write( const void* ptr, int len )
{
    //jz 14.4.97 if( ( _write_buffered | _at_end | _sam3 )  &&  len < _write_buffer_size )
    if( _write_buffered  &&  ( _at_end | _sam3 )  &&  len < _write_buffer_size )
    {
        flush_read_buffer();

        if( !_write_buffer_filled ) {
            _buffer.allocate_min( _write_buffer_size );
            _buffer.length( 0 );
        }

        uint rest_len = len;

        while(1) {
            int l = min( rest_len, uint( _buffer.size() - _buffer.length() ) );
            _buffer.append( ptr, l );
            _write_buffer_filled = true;
            rest_len -= l;
            if( rest_len == 0 )  break;
            flush_write_buffer();
        }
    }
    else
    {
        if( _write_buffer_filled )  flush_write_buffer();
        if( _log )  LOG( "write(" << _file << ",," << len << ")\n" );
        int ret = write( _file, ptr, len );
        if( ret < len )  throw_errno( errno, "write", this );
    }
}

//------------------------------------------------------------------------------Std_file::flush

void Std_file::flush()
{
    flush_write_buffer();
}

//-----------------------------------------------------------------Std_file::flush_write_buffer

void Std_file::flush_write_buffer()
{
    if( _write_buffer_filled ) {
        if( _log )  LOG( "write(" << _file << ",," << _buffer.length() << ")\n" );
        int ret = write( _file, _buffer.char_ptr(), _buffer.length() );
        if( ret < _buffer.length() )  throw_errno( errno, "write", this );
        _write_buffer_filled = false;
        _buffer.length( 0 );
    }
}

//-------------------------------------------------------------------------------Std_file::info
/*
File_info Std_file::info()
{
    File_info info;

    //info._text = "(text)";
    return info;
}
*/
//---------------------------------------------------------------------Std_file::norm_filename

/*
void norm_filename ( char* input_name,
                     char  normed_name[ _MAX_PATH ] )
{
   exceptions
}
*/

//---------------------------------------------------------------------------------is_filename

Bool is_filename( const char* filename )
{
    // filename ist ein Dateiname, wenn keine Blanks drin sind.
    // (Man kˆnnte auf Anf¸hrungszeichen pr¸fen).

    uint len = length_without_trailing_spaces( filename );      // Blanks am Ende ignorieren

    if( memchr( filename, ' ', len ) )  return false;

    return true;
}

//------------------------------------------------------------------------is_absolute_filename

Bool is_absolute_filename( const char* filename )
{
    if( !is_filename( filename ) )  return false;

    uint len = length_without_trailing_spaces( filename );

#   if defined SYSTEM_WIN
        if( len >= 3  
         && isalpha( filename[ 0 ] )
         && filename[1] == ':' 
         && ( filename[ 2 ] == '/' || filename[ 2 ] == '\\' )  )  return true;

        if( len >= 2  
         && ( filename[0] == '/' || filename[0] == '\\' )  
         && ( filename[1] == '/' || filename[1] == '\\' ) )  return true;
#    else
        if( len >= 1  &&  filename[0] == '/' )  return true;
#   endif

    return false;
}

//--------------------------------------------------------------------------------file_exists

bool file_exists( const Sos_string& filename )
{
#   if defined SYSTEM_WIN
        struct ::_stat st;
#    else
        struct stat st;
#   endif

    memset( &st, 0, sizeof st );

#   if defined SYSTEM_WIN
        int err = ::_stat( c_str(filename), &st );
#    else
        int err = ::stat( c_str(filename), &st );
#   endif
    
    if( err ) {
        if( errno == 2 )  return false;
                    else  throw_errno( errno, "stat", c_str(filename) );
    }

    return true;
}

//----------------------------------------------------------------------------------make_path

void make_path( const Sos_string& path_par )
{
    Sos_string  path = path_par;

#   if defined SYSTEM_WIN

        for( int i = 0; i < length(path); i++ )  if( path[i] == '\\' )  path[i] = '/';

        const char* p  = c_str(path);
        const char* p1 = strchr( p, ':' );      // Laufwerksbuchstabe?
        if( p1 ) 
        {
            p = p1 + 1;
        }
        else
        {
            if( p[0] == '/' && p[1] == '/' )   // UNC
            {
                p += 2;
                while( p[0]  &&  p[0] != '/' )  p++;    // Rechnername
                while( p[0] == '/' )  p++;
                while( p[0]  &&  p[0] != '/' )  p++;    // Freigabe (share)
            }
        }

#    else

        const char* p = c_str(path);

#   endif

    // Auf Unix scheint es eine Fehlermeldung zu geben (nicht EEXIST), wenn ein mit mkdir anzulegendes Verzeichnis bereits da ist,
    // aber man kein Schreibrecht hat, weil es jemand anderem gehˆrt.
    // Besser w‰re also, erstmal mit stat() zu pr¸fen, ob es existiert.
/* Z.B. so:
#   ifdef Z_WINDOWS    

        if( GetFileAttributes( path.c_str() ) != -1 )  return;

#    else

        struct stat s;
        int err = ::stat( path.c_str(), &s );
        if( !err )  return;

#   endif
*/

    while( p[0] )
    {
        while( p[0] == '/' )  p++;
        if( !p[0] )  break;
        while( p[0]  &&  p[0] != '/' )  p++;

        Sos_string part = as_string( c_str(path), p - c_str(path) );

#       if defined SYSTEM_WIN
            int err = mkdir( c_str(part) );
#       else
            int err = mkdir( c_str(part), (uint)-1 );
#       endif

        if( err ) 
        {
            if( errno != EEXIST )  throw_errno( errno, "mkdir", c_str(part) );
        }
        else
            LOG( "mkdir " << part << '\n' );
    }
}


} //namespace sos
