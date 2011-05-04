// $Id$

#include "zschimmer.h"
#include "z_gzip.h"

#if defined Z_WINDOWS
#   include "../3rd_party/zlib/zutil.h"      // OS_CODE
#   include "../3rd_party/zlib/zlib.h"
#elif defined Z_SOLARIS || defined Z_HPUX || defined MAKE_UBUNTU
#   include "zlib.h"
#   ifndef OS_CODE
#       define OS_CODE  0x03  /* assume Unix */
#   endif
#else
#   // Wir verwenden (und setzten voraus) die libz.so
#   include "zutil.h"      // OS_CODE
#   include "zlib.h"
#endif


#include <ctype.h>

using namespace std;

//-------------------------------------------------------------------------------------------------

static const int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */


namespace zschimmer {

//--------------------------------------------------------------------------------------------const

const int Simple_byte_queue::default_chunk_size = 10000;

//---------------------------------------------------------------------------------------------Gzip

struct Gzip : Object
{
                                Gzip                    ( Simple_byte_queue*, bool deflate );
                               ~Gzip                    ();

    void                        write                   ( const Byte*, int length );
    void                        close                   ();
    int                         read_header             ( const Byte*, int length );
    void                        write_header            ();

  private:
    void                        start                   ();
    void                        write_gzip_header       ();
    int                         process                 ( int flag );
    void                        check_zlib_error        ( int nReturn );
    void                        write_int32             ( Byte*, int32 );

    Fill_zero                  _zero_;
    z_stream                   _z_stream;
    Simple_byte_queue*         _result_queue;
  //uint32                     _crc;
    int                        _method;
    bool                       _deflate;
    bool                       _gzip;                   // gzip-Kopf verwenden
    bool                       _is_gzip_header_read ;
  //bool                       _auto;                   // Prüfen, ob gzip-Kopf da ist, sonst: _do_nothing;
  //bool                       _do_nothing;
    bool                       _is_closed;
};

//------------------------------------------------------------------------------string_gzip_deflate

string string_gzip_deflate( const Byte* p, int length )
{
    Simple_byte_queue q;
    Gzip_deflator( &q ).write( p, length );
    return q.string_dequeue_to_end();
}

//---------------------------------------------------------------Simple_byte_queue::Buffer::~Buffer

Simple_byte_queue::Buffer::~Buffer()
{ 
    delete [] _ptr; 
}

//--------------------------------------------------------------Simple_byte_queue::Buffer::allocate

void Simple_byte_queue::Buffer::allocate( int size )
{
    _ptr  = new Byte[ size ];
    _size = size;
}

//-------------------------------------------------------------------------Simple_byte_queue::clear

void Simple_byte_queue::clear()
{
    _buffer_list.clear();

    _read_iterator = _buffer_list.begin();
    _read_position = 0;

    _write_position = 0;
}

//--------------------------------------------------------Simple_byte_queue::request_write_buffer_2

Byte* Simple_byte_queue::request_write_buffer_2( int size, int used_length )
{
    assert( used_length <= size );


    if( !_buffer_list.empty() )
    {
        Buffer& last_buffer = *_buffer_list.rbegin();
        
        if( (size_t)size <= last_buffer._size - _write_position ) 
        {
            Byte* result = last_buffer._ptr + _write_position;
            _write_position += used_length;
            return result;
        }

        last_buffer._size = _write_position;
    }

    _buffer_list.push_back( Buffer() );
    Buffer& last_buffer = *_buffer_list.rbegin();
    last_buffer.allocate( max( size, _chunk_size ) );

    if( _buffer_list.size() == 1 )  _read_iterator = _buffer_list.begin();
    
    _write_position = used_length;
    
    return last_buffer._ptr;
}

//-------------------------------------------------------------Simple_byte_queue::write_buffer_size

int Simple_byte_queue::write_buffer_size()
{
    assert( !_buffer_list.empty() );


    Buffer& last_buffer = *_buffer_list.rbegin();
        
    int result = last_buffer._size - _write_position;

    assert( result > 0 );
    return result;
}

//---------------------------------------------------------Simple_byte_queue::string_dequeue_to_end

string Simple_byte_queue::string_dequeue_to_end()
{
    string result;
    int    length = length_to_end();

    if( length > 0 )
    {
        result.reserve( length );
        
        Buffer_list::iterator last = _buffer_list.end();
        last--;

        while( _read_iterator != last )
        {
            result.append( (const char*)_read_iterator->_ptr + _read_position, _read_iterator->_size - _read_position );
            _read_position = 0;
            _read_iterator = _buffer_list.erase( _read_iterator );
        }

        result.append( (const char*)last->_ptr + _read_position, _write_position - _read_position );
        _read_position = _write_position;
    }

    assert( result.length() == length );

    return result;
}

//-----------------------------------------------------------------Simple_byte_queue::length_to_end

size_t Simple_byte_queue::length_to_end()
{
    size_t result = 0;
    int    position = _read_position;

    for( Buffer_list::iterator r = _read_iterator; r != _buffer_list.end(); r++ )
    {
        result += _read_iterator->_size - position;
        position = 0;
    }

    if( result )
    {
        result -= _buffer_list.rbegin()->_size - _write_position;
    }

    return result;
}

//---------------------------------------------------------------------Gzip_deflator::Gzip_deflator

Gzip_deflator::Gzip_deflator( Simple_byte_queue* q )
: 
    _zero_(this+1),
    _gzip( Z_NEW( Gzip( q, false ) ) )
{
}

//--------------------------------------------------------------------Gzip_deflator::~Gzip_deflator
    
Gzip_deflator::~Gzip_deflator()
{
    _gzip->close();
}

//-----------------------------------------------------------------------------Gzip_deflator::write
    
void Gzip_deflator::write( const Byte* p, int length )
{
    _gzip->write( p, length );
}

//---------------------------------------------------------------------------------------Gzip::Gzip

Gzip::Gzip( Simple_byte_queue* q, bool deflate )
:
    _zero_(this+1),
    _result_queue(q),
    _deflate(deflate),
    _gzip(true),
    _method(Z_DEFAULT_COMPRESSION)
{
    _z_stream.opaque = this;

    start();
}

//--------------------------------------------------------------------------------------Gzip::~Gzip
    
Gzip::~Gzip()
{
    close();

    _deflate? deflateEnd( &_z_stream )
            : inflateEnd( &_z_stream );
}

//--------------------------------------------------------------------------------------Gzip::start

void Gzip::start()
{
    //_crc = crc32( 0, NULL, 0 );

    _z_stream.next_out  = _result_queue->write_buffer( 1 );
    _z_stream.avail_out = _result_queue->write_buffer_size();

    int ret;

    if( _deflate ) 
    {
        if( _gzip )  ret = deflateInit2( &_z_stream, _method, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY );
               else  ret = deflateInit( &_z_stream, _method );
    } 
    else 
    {
        if( _gzip )  ret = inflateInit2( &_z_stream, -MAX_WBITS );
               else  ret = inflateInit( &_z_stream );
    }
     
    check_zlib_error( ret );
}

//--------------------------------------------------------------------------------------Gzip::close

void Gzip::close()
{
    if( !_is_closed )
    {
        //if( _writing ) 
        //{
            while(1) 
            {
                int ret = process( Z_FINISH );
                if( ret == Z_STREAM_END )  break;
            }

        //    Byte* p = _result_queue->request_write_buffer( 2 * 4 );

        //    write_int32( p + 0, _crc );
        //    write_int32( p + 4, _z_stream.total_in );
        //}
        //else
        {
            // _crc und _z_total_in prüfen. 
            // Die acht Bytes stehen in _z_stream und _file.get()
        }

        int ret = ( _deflate? deflateEnd : inflateEnd )( &_z_stream );
        check_zlib_error( ret );
        
        _is_closed = true;
    }
}

//-------------------------------------------------------------------------------Gzip::write_header

void Gzip::write_gzip_header()
{
    Byte* p = _result_queue->request_write_buffer( 8 );

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
}

//--------------------------------------------------------------------------------Gzip::read_header

int Gzip::read_header( const Byte* p0, int length )
{
    const Byte* p     = p0;
    const Byte* p_end = p + length;
    int         flags;
    uint        len;

    if( p+2+1+1+6 >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );

    /* Check the gzip magic header */
    if( p[0] != gz_magic[0]  ||  p[1] != gz_magic[1] ) 
    {
        //if( !_auto )  
            throw_xc( "ZLIB-100" );
        //_do_nothing = true;
        //LOG( "gzip -auto: Datenstrom hat keinen gzip-Kopf\n" );
        //return 0;
    }

    //if( _auto )  LOG( "gzip -auto: Datenstrom hat einen gzip-Kopf\n" );

    p += 2;
    _method = *p++;

    flags = *p++;

    if( _method != Z_DEFLATED || (flags & RESERVED) != 0 )  throw_xc( "ZLIB-DATA-ERROR" );

    p += 6;  /* Discard time, xflags and OS code */

    if( (flags & EXTRA_FIELD) != 0) { /* skip the extra field */
        len  =  (uInt)*p++;
        len += ((uInt)*p++ ) << 8;
        p += len;
        if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );
    }

    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
        //Z_LOG2( "zschimmer", "zlib origfile=" << p << '\n' );
        p += strlen( (const char*)p ) + 1;
        if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );
    }

    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
        //Z_LOG2( "zschimmer", "zlib comment=" << p << '\n' );
        p += strlen( (const char*)p ) + 1;
        if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
        p += 2;
    }

    if( p >= p_end )  throw_xc( "ZLIB-DATA-ERROR" );

    return p - p0;
}

//--------------------------------------------------------------------------------Gzip::write_int32

void Gzip::write_int32( Byte* p, int32 v )
{
    for( int i = 0; i < 4; i++ )  *p++ = (Byte)v,  v >>= 8;
}

//--------------------------------------------------------------------------------------Gzip::write

void Gzip::write( const Byte* p, int length )
{
    if( !_is_gzip_header_read )
    {
        int l = read_header( p, length );
        _is_gzip_header_read = true;
        p += l;
        length -= l;
    }

    //_crc = crc32( _crc, p, length );

    _z_stream.next_in  = const_cast<Byte*>( p );
    _z_stream.avail_in = length;

    while( _z_stream.avail_in > 0 )
    {
        int ret = process( Z_NO_FLUSH );
        if( ret == Z_STREAM_END )  break;   // z_stream.avail_in sollte 8 Bytes enthalten (crc+length), oder ein Bruchstück, falls noch ein write() folgt
    }
}

//------------------------------------------------------------------------------------Gzip::process

int Gzip::process( int flag )
{
    int ret = ( _deflate? deflate : inflate ) ( &_z_stream, flag );
    if( ret )  check_zlib_error( ret );
            
    int len = _result_queue->write_buffer_size() - _z_stream.avail_out;
    if( len ) 
    {
        _result_queue->on_write_buffer_written( len );

        _z_stream.next_out  = _result_queue->write_buffer( 1 );   
        _z_stream.avail_out = _result_queue->write_buffer_size();
    }

    return ret;
}

//---------------------------------------------------------------------------Gzip::check_zlib_error

void Gzip::check_zlib_error( int nReturn )
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

//----------------------------------------------------------------------------------------gzip_file

//void gzip_file( const File_path& source_file, const File_path& sink_file )
//{
//    File input  ( source     , "r" );
//    File output ( destination, "w" );
//
//    while( !input.eof() )
//    {
//    }
//
//    namespace io = boost::iostreams;
//
//    io::filtering_ostream out;
//    out.push( io::gzip_compressor() );
//    out.push( io::file_sink( sink_file ) );
//    
//    boost::io::copy( io::file_source( source ), out );
//}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
