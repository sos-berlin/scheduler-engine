// $Id: cache_file.cxx 11837 2006-01-25 08:19:41Z jz $

// s.a. ältere cachefl.cxx, cachsqfl.cxx

#include "precomp.h"
#include "../kram/sysdep.h"
#include <limits.h>

#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY

#if defined SYSTEM_WIN
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#else
#   include <stdio.h>               // fileno
#   include <unistd.h>              // read(), write(), close()
#endif

#include <errno.h>
#include <time.h>
#include <map>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"

#include "../kram/log.h"


namespace sos {

//---------------------------------------------------------------------------------------const

const int max_cached_files = 100;

//---------------------------------------------------------------------------------Cached_file

struct Cached_file : Sos_self_deleting
{
                                    Cached_file         () : _zero_(this+1), _birth(time(NULL)), _tmp_file(-1)  {}
                                   ~Cached_file         ();

    void                            open                ( const string& filename, Abs_file::Open_mode, const File_spec& );
    void                            get                 ( int64 pos, Area* buffer );


    Fill_zero                      _zero_;
    Any_file                       _file;
    string                         _tmp_filename;
    int                            _tmp_file;
  //Area                           _current_key;        
  //Byte                           _current_key_buffer[ sizeof (int64) ];
  //int64                          _pos;                // Position des zuletzt gelesenen Satzes
    int64                          _next_pos;           // Position des nächsten zu lesenden Satzes
    int64                          _write_pos;          // Hochwassermarke == Größe der temporären Datei
    int64                          _eof_pos;            // Absolute Hochwassermarke, temporären Datei mit allen Sätzen
    bool                           _error_in_tmp_file;
    bool                           _do_seek;
    int64                          _number;             // Zähler. Der Eintrag mit der kleinsten Nummer wird zuerst vergessen.
    uint                           _birth;
};

//------------------------------------------------------------------------------Cached_file_map

typedef std::map< string, Sos_ptr<Cached_file> >  Cached_file_map;

struct Cache_file_static : Sos_self_deleting
{
                                    Cache_file_static   ();
                                   ~Cache_file_static   ();

    void                            remove_oldest       ();

    Fill_zero                      _zero_;
    Cached_file_map                _cached_file_map;
    int                            _max_cached_files;
    int                            _max_age;
    int64                          _number;
};

DEFINE_SOS_STATIC_PTR( Cache_file_static );

//-----------------------------------------------------------------------------------Cache_file

struct Cache_file : Abs_file
{
                                    Cache_file          ();
                                   ~Cache_file          ();

    void                            open                ( const char*, Open_mode, const File_spec& );
  //void                            close               ( Close_mode );

    void                            rewind              ( Key::Number );
  //void                            set                 ( const Key& );
  //Record_position                 key_position        ( Key::Number ) { return _file.key_position(); }
  //Record_length                   key_length          ( Key::Number ) { return _file.key_length(); }

  protected:
    void                            get_record          ( Area& area );
  //void                            update_current_key  ( int64 );

  private:
    Fill_zero                      _zero_;
    int64                          _my_pos;             // Position des nächsten zu lesenden Satzes
    Sos_ptr<Cached_file>           _cached_file;
};

//------------------------------------------------------------------------------Cache_file_type

struct Cache_file_type : Abs_file_type
{
    virtual const char*         name                    () const        { return "cache"; }
    virtual const char*         alias_name              () const        { return "cache_seq"; };    // Alter Name

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Cache_file> f = SOS_NEW( Cache_file );
        return +f;
    }
};

const Cache_file_type  _cache_file_type;
const Abs_file_type&    cache_file_type = _cache_file_type;

//--------------------------------------------------------------------------------------cleanup
/*
static void cleanup()
{
    for( Cached_file_map::iterator it = cached_file_map.begin(); it != cached_file_map.end(); it++ )
    {
        it->second = NULL;
    }
}
*/
// -------------------------------------------------------------------Cached_file::~Cached_file

Cached_file::~Cached_file()
{
    LOGI( "~Cached_file close " << _tmp_filename << '\n' );

    if( _tmp_file != -1 )  ::close( _tmp_file );
    // Datei ist jetzt gelöscht. In Unix wegen unlink(), in Windows wegen O_TEMPORARY   

    _file.close();
}

//----------------------------------------------------------------------------Cached_file::open

void Cached_file::open( const string& filename, Abs_file::Open_mode open_mode, const File_spec& file_spec )
{
    int     ret;

    if( _tmp_file != -1 )  throw_xc( "Cache_file::open" );

    _file.open( filename, open_mode, file_spec );
    _eof_pos = INT64_MAX;

#   ifdef SYSTEM_WIN

        char tmp_filename [1024+1];

        ret = GetTempPath( sizeof tmp_filename, tmp_filename );
        if( ret == 0 )  throw_mswin_error( "GetTempPath" );

        ret = GetTempFileName( tmp_filename, "sos", 0, tmp_filename );
        if( ret == 0 )  throw_mswin_error( "GetTempFileName" );

        _tmp_filename = tmp_filename;

        int oflag = O_TEMPORARY | _O_SHORT_LIVED | O_SEQUENTIAL;
        int pmode = S_IREAD | S_IWRITE;

        LOG( "Cached_file create " << _tmp_filename << '\n' );

        _tmp_file = ::sopen( _tmp_filename.c_str(), oflag | O_CREAT | O_RDWR | O_APPEND | O_BINARY, _SH_DENYRW, pmode );
        if( _tmp_file == -1 )  throw_errno( errno, "open", this );

#    else

        char* tmp = getenv( "TMP" );
        if( !tmp || !tmp[0] )  tmp = "/tmp";

        _tmp_filename = tmp;
        _tmp_filename += "/hostware.cache.XXXXXX";

        _tmp_file = mkstemp( (char*)_tmp_filename.c_str() );        // Öffnet die Datei, aber ohne O_APPEND
        LOG( "Cached_file create " << _tmp_filename << '\n' );

        ret = unlink( _tmp_filename.c_str() );                    // Namen sofort wieder löschen
        if( ret == -1 )  throw_errno( errno, "unlink", this );
#   endif
}

//----------------------------------------------------------------------------Cached_file::get

void Cached_file::get( int64 pos, Area* buffer )
{
    Byte ln [3];
    int  ret;

    if( _error_in_tmp_file )  throw_xc( "SOS-1414" );

    if( pos == _eof_pos )  throw_eof_error();
    else
    if( pos == _write_pos )
    {
        try {
            _file.get( buffer );
        }
        catch( const Eof_error& )
        {
            _eof_pos = _write_pos;
            _file.close();
            throw;
        }

        ln[0] = (Byte)( buffer->length() >> 16 );
        ln[1] = (Byte)( buffer->length() >>  8 );
        ln[2] = (Byte)( buffer->length()       );

#       if defined SYSTEM_UNIX
            ret = lseek( _tmp_file, 0, SEEK_END );
            if( ret < 0 )  { _error_in_tmp_file = true; throw_errno( errno, "lseek", this ); }
#       endif

        _do_seek = true;

        ret = write( _tmp_file, ln, 3 );
        if( ret < 3 )  throw_errno( errno, "write", this );

        ret = write( _tmp_file, buffer->byte_ptr(), buffer->length() );
        if( ret < buffer->length() )  throw_errno( errno, "write", this );

        _write_pos += 3 + buffer->length();
        _next_pos = _write_pos;
    }
    else
    {
        if( _do_seek || pos != _next_pos )
        {
            int64 ret = lseek( _tmp_file, pos, SEEK_SET );
            if( ret < 0 )  { _error_in_tmp_file = true; throw_errno( errno, "lseek", this ); }

            _do_seek = false;
        }

        ret = read( _tmp_file, ln, 3 );
        if( ret < 3 )  throw_errno( errno, "write", this );
        
        int len = ( uint(ln[0]) << 16 ) + ( uint(ln[1]) << 8 ) + ln[2];

        buffer->allocate_min( len );
        buffer->length( len );

        ret = read( _tmp_file, buffer->byte_ptr(), len );
        if( ret < len ) { _error_in_tmp_file = true; throw_errno( errno, "read", this ); }

        _next_pos = pos + 3 + len;
    }
}

//---------------------------------------------------------Cache_file_static::Cache_file_static

Cache_file_static::Cache_file_static() 
: 
    _zero_(this+1), 
    _max_cached_files(max_cached_files),
    _max_age(INT_MAX)
{
    _max_cached_files = read_profile_uint( "", "cache", "files", _max_cached_files );
}

//---------------------------------------------------------Cache_file_static::Cache_file_static

Cache_file_static::~Cache_file_static() 
{
    if( !_cached_file_map.empty() )
    {
        LOGI( "Dateityp cache: Dateien werden geschlossen ...\n" );
        _cached_file_map.clear();
    }
}

//-------------------------------------------------------------Cache_file_static::remove_oldest

void Cache_file_static::remove_oldest()
{
    if( _cached_file_map.size() == 0 )  return;

    LOGI( "Cache_file_static::remove_oldest\n" );

    Cached_file_map::iterator oldest = _cached_file_map.begin();

    for( Cached_file_map::iterator it = _cached_file_map.begin(); it != _cached_file_map.end(); it++ )
    {
        if( it->second->_number < oldest->second->_number )  oldest = it;
    }

    _cached_file_map.erase( oldest );
}

//---------------------------------------------------------------------- Cache_file::Cache_file

Cache_file::Cache_file()
:
    _zero_(this+1)
  //_current_key( _current_key_buffer, sizeof _current_key_buffer )
{
}

//----------------------------------------------------------------------Cache_file::~Cache_file

Cache_file::~Cache_file()
{
}

//-----------------------------------------------------------------------------Cache_file::open

void Cache_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string  filename;

    if( !sos_static_ptr()->_cache_file ) {
        Sos_ptr<Cache_file_static> s = SOS_NEW( Cache_file_static );
        sos_static_ptr()->_cache_file = +s;
    }
    Cache_file_static* stat = sos_static_ptr()->_cache_file;

    bool    immediately = false;
    bool    new_cache   = false;
    bool    flush_all   = false;
    int     max_age     = stat->_max_age;


    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if( opt.flag      ( "immediately" ) 
         || opt.flag      ( "i"           ) )           immediately = opt.set();        // Schon beim open() den Cache füllen
        else
        if( opt.flag      ( "new"         ) )           new_cache = opt.set();          // Datei im Cache ignorieren, also neu lesen
        else
        if( opt.flag      ( "flush-all"   ) )           flush_all = opt.set();          // Alle Cache-Einträge löschen
        else
        if( opt.with_value( "files"       ) )           stat->_max_cached_files = opt.as_uint4();
        else
        if( opt.with_value( "age"     ) )               max_age = opt.as_uint4();       // Höchstes Alter in Sekunden
        else
        if( opt.with_value( "default-age" ) )           stat->_max_age = opt.as_uint4(); // Höchstes Alter in Sekunden (Default)
        else
      //if( opt.with_value( "mem" ) )                   _mem = opt.as_uintK();
      //else
        if( opt.pipe() )                                { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    if( flush_all )  stat->_cached_file_map.clear();

    if( !empty( filename ) )
    {
        if( new_cache )  stat->_cached_file_map[ filename ] = NULL;
        _cached_file = stat->_cached_file_map[ filename ];

        if( _cached_file ) {
            if( (uint64)INT_MAX + (uint64)_cached_file->_birth <= (uint64)INT_MAX + (uint64)time(NULL) - max_age )  _cached_file = NULL;
        }

        if( _cached_file ) 
        {
            LOG( "Cache_file: " << _cached_file->_tmp_filename << " wird benutzt\n" );
        }
        else
        {
            _cached_file = SOS_NEW( Cached_file );
            _cached_file->open( filename, Abs_file::Open_mode( open_mode | seq ), file_spec );

            stat->_cached_file_map[ filename ] = _cached_file;
        }

        _cached_file->_number = ++stat->_number;
        while( stat->_cached_file_map.size() > stat->_max_cached_files )  stat->remove_oldest();

        _any_file_ptr->_spec._field_type_ptr = _cached_file->_file.spec()._field_type_ptr;
        _any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = _cached_file->_file.spec()._key_specs._key_spec._field_descr_ptr;

        _key_pos = _cached_file->_file.key_position();
        _key_len = _cached_file->_file.key_length();

      //current_key_ptr( &_current_key );
      //_key_len = _current_key.length();

        if( immediately && _cached_file->_eof_pos == INT64_MAX )
        {
            Dynamic_area buffer;

            try {
                while(1) { get_record( buffer ); }
            }
            catch( const Eof_error& ) {}

            rewind(0);
            //_cached_file->_file.close();
            //LOG( "Cache_file: Datei wieder geschlossen\n" );
        }
    }
}

// --------------------------------------------------------------------- Cache_file::rewind

void Cache_file::rewind( Key::Number )
{
    _my_pos = 0;
}

// --------------------------------------------------------------------------Cache_file::set
/*
void Cache_file::set( const Key& key )
{
    if( key.length() != sizeof _next_pos )  throw_xc("SOS-1228", key.length(), sizeof _next_pos );

    _do_seek = true;
    _next_pos = 0;

    for( int i = 0; i < sizeof _next_pos; i++ )
    {
        _next_pos |= _current_key.byte_ptr()[i];
        _next_pos <<= 8;
    }
}
*/
// -----------------------------------------------------------Cache_file::update_current_key
/*
void Cache_file::update_current_key( int64 k )
{
    uint64 p = k;

    for( int i = sizeof p - 1; i >= 0; i-- )
    {
        _current_key.byte_ptr()[i] = (Byte)p;
        p >>= 8;
    }
}
*/
// ------------------------------------------------------------------Cache_file::get_record

void Cache_file::get_record( Area& area )
{
    if( !_cached_file )  throw_xc( "SOS-1416" );

  //_pos = _next_pos;
  //update_current_key( _pos );

    _cached_file->get( _my_pos, &area );

    _my_pos = _cached_file->_next_pos;
}


} //namespace sos
