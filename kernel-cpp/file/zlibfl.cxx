// $Id: zlibfl.cxx 13795 2009-06-23 11:13:12Z sos $

#include "precomp.h"

#include "../kram/sysdep.h"
#if defined SYSTEM_SOLARIS || defined __hpux || defined MAKE_UBUNTU
//#   include "../zlib/zutil.h"
#   include "zlib.h"
#elif defined SYSTEM_WIN
#   include "../3rd_party/zlib/zutil.h"      // OS_CODE
#   include "../3rd_party/zlib/zlib.h"
#else
#   // Wir verwenden (und setzten voraus) die libz.so
#   include "zutil.h"      // OS_CODE
#   include "zlib.h"
#endif

#ifndef OS_CODE
#  define OS_CODE  0x03  /* assume Unix */
#endif

/*
#ifdef _DEBUG
#   pragma comment( lib, "zlibd.lib" )
#else
#   pragma comment( lib, "zlib.lib" )
#endif
*/

#include <ctype.h>
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"

static const int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */


namespace sos {

//----------------------------------------------------------------------------------Zlib_file

struct Zlib_file : Abs_file
{
                                    Zlib_file           ();
                                   ~Zlib_file           ();

    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );
    void                            rewind              ( Key::Number );

  protected:
    void                            get_record          ( Area& );
    void                            put_record          ( const Const_area& );

  private:
    void                            start               ();
    void                            init_stream         ();
    void                            write_header        ();
    int                             read_header         ();
    void                            check_zlib_error    ( int nReturn );

    Fill_zero                      _zero_;
    Any_file                       _file;
    z_stream                       _z_stream;
  //Dynamic_area                   _input_buffer; 
    int4                           _block_size;
    Dynamic_area                   _buffer;
    uint4                          _crc;
    int                            _method;
    Bool                           _deflate;
    Bool                           _gzip;               // gzip-Kopf verwenden
    Bool                           _auto;               // Prüfen, ob gzip-Kopf da ist, sonst: _do_nothing;
    Bool                           _do_nothing;         // Daten nicht anrühren, sondern durchlassen (nur bei -auto oder -deflate-only)
    Bool                           _writing;
    Bool                           _eof;
};

//---------------------------------------------------------------------------Zlib_file_type

struct Zlib_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "gzip"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Zlib_file> f = SOS_NEW( Zlib_file() );
        return +f;
    }
};

const Zlib_file_type    _zlib_file_type;
const Abs_file_type&     zlib_file_type = _zlib_file_type;

// --------------------------------------------------------------------Zlib_file::Zlib_file

Zlib_file::Zlib_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Zlib_file::~Zlib_file

Zlib_file::~Zlib_file()
{
    _deflate? deflateEnd( &_z_stream )
            : inflateEnd( &_z_stream );
}

// -------------------------------------------------------------------------Zlib_file::open

void Zlib_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Bool        deflated        = true;
    Bool        inflate_only    = false;
    Sos_string  filename;

    if( ( open_mode & inout ) == inout )  throw_xc( "SOS-1236" );

    _method     = Z_DEFAULT_COMPRESSION;
    _block_size = 4096;
    _writing    = !!( open_mode & out );
    _gzip       = true;


    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "block-size" ) )            _block_size = opt.as_uintK();
        else
        if( opt.flag( "gzip" ) )                        _gzip = opt.set();           // gzip-Kopf verwenden
        else
        if( opt.flag( "zlib" ) )                        _gzip = !opt.set();          // gzip-Kopf nicht verwenden
        else
        if( opt.flag( "deflated" ) )                    deflated = opt.set();
        else
        if( opt.flag( "inflated" ) )                    deflated = !opt.set();
        else
        if( opt.flag( "deflate" )
         || opt.flag( "d"       ) )                     deflated = !!opt.set() == _writing;
        else
        if( opt.flag( "inflate" ) )                     deflated = !opt.set() != _writing;
        else
        if( opt.flag( opt.option() )  &&  opt.option()[ 1 ] == '\0'  &&  isdigit( opt.option()[ 0 ] ) )  _method = opt.option()[ 0 ] - '0';
        else
        if( opt.flag( "auto" ) )                        _auto = opt.set();           // Beim Lesen nicht gezippte Dateien lesen
        else
        if( opt.flag( "inflate-only" ) )                inflate_only = opt.set();    // Nur entkomprimieren, nicht komprimieren
        else
        if( opt.pipe() )                                filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _deflate = deflated == _writing;

    if( inflate_only  &&  _deflate )  _do_nothing = true;

    _file.obj_owner( this );
    _file.open( filename, Open_mode( open_mode | ( !_do_nothing && deflated? binary : 0 ) ), file_spec );

    _z_stream.opaque = this;

    start();
}

//---------------------------------------------------------------------------Zlib_file::start

void Zlib_file::start()
{
    if( _do_nothing )  return;

    if( _writing ) 
    {
        init_stream();

        _buffer.allocate_min( _block_size );

        if( _gzip )  write_header();

    	_z_stream.next_out  = _buffer.byte_ptr() + _buffer.length();
        _z_stream.avail_out = _buffer.size() - _buffer.length();
    }
    else
    {
        int len = 0;

        if( _gzip ) {
            _buffer.allocate_min( _block_size );  // read_header() muss dann nicht so pingelig auf Satzende prüfen
            
            if( _file.eof() )  
            {   
                _eof = true;                    // Datei ist leer
                _do_nothing = true;             // zlib nicht schließen (ist auch nicht gestartet)
                return; 
            }

            _file.get( _buffer );
            len = read_header();                       // Header muss vollständig im ersten Satz sein!
            if( _do_nothing )  return;
        }

        _z_stream.next_in  = _buffer.byte_ptr() + len;
        _z_stream.avail_in = _buffer.length() - len;

        init_stream();
    }

    _crc = crc32( 0, NULL, 0 );
}

//---------------------------------------------------------------------Zlib_file::init_stream

void Zlib_file::init_stream()
{
    int ret;

    if( _deflate ) {
        if( _gzip ) {
            ret = deflateInit2( &_z_stream, _method, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY );
        } else {
            ret = deflateInit( &_z_stream, _method );
        }
    } else {
        if( _gzip ) {
            ret = inflateInit2( &_z_stream, -MAX_WBITS );
        } else {
            ret = inflateInit( &_z_stream );
        }
    }
     
    check_zlib_error( ret );
}

// -------------------------------------------------------------------Zlib_file::write_header

void Zlib_file::write_header()
{
    Byte* p = _buffer.byte_ptr();

    *p++ = gz_magic[0];
    *p++ = gz_magic[1];
    *p++ = Z_DEFLATED;          // Methode
    *p++ = 0;                   /*flags*/
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;                   /*time*/
    *p++ = 0;                   /*xflags*/
    *p++ = OS_CODE;

    _buffer.length( p - _buffer.byte_ptr() );
    //_file.put( _buffer );
}

// -------------------------------------------------------------------Zlib_file::read_header

int Zlib_file::read_header()
{
    const Byte* p     = _buffer.byte_ptr();
    const Byte* p_end = p + _buffer.length();
    int         flags;  /* flags byte */
    uint        len;

    /* Check the gzip magic header */
    if( p+2 >= p_end  ||  p[0] != gz_magic[0]  ||  p[1] != gz_magic[1] ) 
    {
        if( !_auto )  throw_xc( "ZLIB-100" );
        _do_nothing = true;
        LOG( "gzip -auto: Datenstrom hat keinen gzip-Kopf\n" );
        return 0;
    }

    if( _auto )  LOG( "gzip -auto: Datenstrom hat einen gzip-Kopf\n" );

    p += 2;
    _method = *p++;

    flags = *p++;

    if( _method != Z_DEFLATED || (flags & RESERVED) != 0 )  throw_xc( "ZLIB-DATA-ERROR" );

    p += 6;  /* Discard time, xflags and OS code */

    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
        len  =  (uInt)*p++;
        len += ((uInt)*p++ ) << 8;
        p += len;
        if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );
    }

    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
        LOG( "zlib origfile=" << p << '\n' );
        p += strlen( (const char*)p ) + 1;
        if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );
    }

    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
        LOG( "zlib comment=" << p << '\n' );
        p += strlen( (const char*)p ) + 1;
        if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
        p += 2;
    }

    if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );

    return p - _buffer.byte_ptr();
}

// --------------------------------------------------------------------------Zlib_file::close

void Zlib_file::close( Close_mode close_mode )
{
    int ret;

    if( !_do_nothing ) 
    {
        if( _writing ) {
            while(1) {
                ret = _deflate? deflate( &_z_stream, Z_FINISH )
                              : inflate( &_z_stream, Z_FINISH );
                check_zlib_error( ret );

                int len = _buffer.size() - _z_stream.avail_out;
                if( len ) {
                    _buffer.length( len );
                    _file.put( _buffer );
                    _z_stream.next_out = _buffer.byte_ptr();
                    _z_stream.avail_out = _buffer.size();
                }

                if( ret == Z_STREAM_END )  break;
            }


            int   i;
            uint4 x;

            _buffer.allocate_min( 2 * 4 );
            Byte* p = _buffer.byte_ptr();

            x = _crc;
            for( i = 0; i < 4; i++ )  { *p++ = (Byte)x; x >>= 8; }
            x = _z_stream.total_in;
            for( i = 0; i < 4; i++ )  { *p++ = (Byte)x; x >>= 8; }

            _buffer.length( p - _buffer.byte_ptr() );
            _file.put( _buffer );
        }
        else
        {
            // _crc und _z_total_in prüfen. 
            // Die acht Bytes stehen in _z_stream und _file.get()
        }
    }

    _file.close( close_mode );

    if( !_do_nothing ) {
        ret = _deflate? deflateEnd( &_z_stream )
                      : inflateEnd( &_z_stream );
        check_zlib_error( ret );
    }
}

// --------------------------------------------------------------------------Zlib_file::rewind

void Zlib_file::rewind( int )
{
    _file.rewind();
    start();
    _eof = false;
}

// ----------------------------------------------------------------------Zlib_file::put_record

void Zlib_file::put_record( const Const_area& record )
{
    int ret;

    if( record.length() == 0 )  return;

    if( _do_nothing ) 
    {
        _file.put( record );
    }
    else
    {
        _z_stream.next_in = (Byte*)record.byte_ptr();
        _z_stream.avail_in = record.length();

        while( _z_stream.avail_in > 0)
        {
            ret = _deflate? deflate( &_z_stream, Z_NO_FLUSH )
                          : inflate( &_z_stream, Z_NO_FLUSH );
            if( ret )  check_zlib_error( ret );
                    
            int len = _buffer.size() - _z_stream.avail_out;
            if( len ) {
                _buffer.length( len );
                _file.put( _buffer );
                _z_stream.next_out = _buffer.byte_ptr();
                _z_stream.avail_out = _buffer.size();
            }
        }

        _crc = crc32( _crc, record.byte_ptr(), record.length() );
    }
}

// ---------------------------------------------------------------------Zlib_file::get_record

void Zlib_file::get_record( Area& buffer )
{
    if( _eof )  throw_eof_error();

    if( _do_nothing ) 
    {
        if( _buffer.length() == 0 ) {
            _file.get( &_buffer );
        }
        
        buffer.assign( _buffer );
        _buffer.length( 0 );
    } 
    else 
    {
        int ret;

        if( buffer.resizable() )  buffer.allocate_min( _block_size );

        _z_stream.next_out = (Byte*)buffer.byte_ptr();
        _z_stream.avail_out = buffer.size();

        while( _z_stream.avail_out )        // Solange _buffer nicht voll
        {
            while( _z_stream.avail_in == 0  &&  !_eof ) 
            {
                try {
                    _file.get( _buffer );

                    _z_stream.next_in = _buffer.byte_ptr();
                    _z_stream.avail_in = _buffer.length();
                }
                catch( const Eof_error& ) { 
                    _eof = true; 
                }
            }

            ret = _deflate? deflate( &_z_stream, Z_NO_FLUSH )
                          : inflate( &_z_stream, Z_NO_FLUSH );
            if( ret == Z_STREAM_END )  break;
            if( ret )  check_zlib_error( ret );
        }

        int len = buffer.size() - _z_stream.avail_out;
        if( len == 0 )  throw_eof_error();

        buffer.length( len );

        _crc = crc32( _crc, buffer.byte_ptr(), buffer.length() );
    }
}

//------------------------------------------------------------------Zlib_file::check_zlib_error

void Zlib_file::check_zlib_error( int nReturn )
{
    switch( nReturn )
    {
        case Z_OK:           return;
        case Z_STREAM_END:   return;  
        case Z_ERRNO:        throw_errno( errno, "zlib", _z_stream.msg );
        case Z_STREAM_ERROR: throw_xc( "ZLIB-STREAM-ERROR", _z_stream.msg );
        case Z_MEM_ERROR:    throw_xc( "ZLIB-MEM-ERROR", _z_stream.msg );
        case Z_DATA_ERROR:   throw_xc( "ZLIB-DATA-ERROR", _z_stream.msg );
        case Z_BUF_ERROR:    throw_xc( "ZLIB-BUF-ERROR", _z_stream.msg );
        default:             throw_xc( "ZLIB-ERROR", _z_stream.msg );
    }
}


} //namespace sos

