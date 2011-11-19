// sam3fl.cxx                                     ©1997 SOS Software Gmbh
// Joacim Zschimmer

#include "precomp.h"
#include <limits.h>

#include <ctype.h>
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"

namespace sos {

//----------------------------------------------------------------------------------Sam3_file

struct Sam3_file : Abs_file //Filter_file
{
                                    Sam3_file           ();
                                   ~Sam3_file           ();

    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );
    void                            rewind              ( Key::Number );

  protected:
    void                            get_record          ( Area& );
    void                            put_record          ( const Const_area& );

  private:
    Fill_zero                      _zero_;
    Any_file                       _file;
    Dynamic_area                   _buffer;
  //size_t                         _buffer_pos;             // Position (tell) von _buffer in der Datei
    uint                           _buffer_size;
    uint                           _min_buffer_size;
    uint                           _max_buffer_size; 
    char*                          _read_ptr;
    char*                          _read_end;
    Bool                           _eof; 
};

//----------------------------------------------------------------------Sam3_file_type

struct Sam3_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "record/sam3"; }
    virtual const char*         alias_name              () const { return "sam3"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Sam3_file> f = SOS_NEW( Sam3_file() );
        return +f;
    }
};

const Sam3_file_type    _sam3_file_type;
const Abs_file_type&     sam3_file_type = _sam3_file_type;

//----------------------------------------------------------------------------------------const

#if defined SYSTEM_WIN16
    const uint default_buffer_size = 16*1024;        // Puffergröße
#elif defined SYSTEM_WIN32
    // Windows 95 Systemsteuerung, Leistungsmerkmale, Dateisysteme:
    // Windows liest voraus, wenn das Programm in Schritten zu max. 64KB sequentiell liest.
    // Optimal sind offenbar 16KB zum Lesen und zum Schreiben (jedenfalls bei Joacims PC)
    // 64KB bringt wenig mehr, darüber wird's wieder langsamer (wg. malloc)
    const uint default_buffer_size = 16*1024;       // Puffergröße
#else
    const uint default_buffer_size = 16*1024;       // Puffergröße
#endif

// --------------------------------------------------------------------Sam3_file::Sam3_file

Sam3_file::Sam3_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Sam3_file::~Sam3_file

Sam3_file::~Sam3_file()
{
}

// -------------------------------------------------------------------------Sam3_file::open

void Sam3_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    //12.11.97 if( ( open_mode & inout ) == inout )  throw_xc( "SOS-1236" );

    _min_buffer_size = default_buffer_size;
    _max_buffer_size = default_buffer_size;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.pipe() )                                filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _file.obj_owner( this );

    _file.open( filename, Any_file::Open_mode( open_mode | binary ), file_spec );
}

// --------------------------------------------------------------------------Sam3_file::close

void Sam3_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// -------------------------------------------------------------------------Sam3_file::rewind

void Sam3_file::rewind( Key::Number )
{
    _file.rewind();

    _read_ptr   = NULL;
    _read_end   = NULL;
    //_buffer_pos = 0;
    _eof = false;
}

// ----------------------------------------------------------------------Sam3_file::put_record

void Sam3_file::put_record( const Const_area& record )
{
    _buffer.allocate_length( 3 + record.length() );

    _buffer.byte_ptr()[ 0 ] = Byte( record.length() >> 16 );
    _buffer.byte_ptr()[ 1 ] = Byte( record.length() >>  8 );
    _buffer.byte_ptr()[ 2 ] = Byte( record.length()       );

    memcpy( _buffer.byte_ptr() + 3, record.ptr(), record.length() );

    _file.put( _buffer );
}

// ---------------------------------------------------------------------Sam3_file::get_record

void Sam3_file::get_record( Area& buffer )
{
    int  len_count     = 3;   // Soviele Längenbytes noch zu lesen
    int  rest_length   = 0;   

    buffer.length( 0 );

    while(1) {   // bis Satzende
        if( _read_ptr == _read_end )
        {
            if( _eof ) {
                _buffer.length( 0 );
            } else {
                if( _buffer_size == 0 )  _buffer_size = _min_buffer_size;
                else
                if( _buffer_size <= _max_buffer_size / 2 /*&& _buffer_size != _pos*/ )  _buffer_size = 2 * _buffer_size; // Damit immer auf 2er-Potenz

                _read_ptr   = NULL;
                _read_end   = NULL;
                _buffer.allocate_min( _buffer_size );

                //if( _log )  LOG( "read(" << _file << ',' << _buffer.ptr() << "," << _buffer_size << ")  _buffer_pos=" << _buffer_pos << "\n" );
                if( buffer.length() == 0 ) {
                    _file.get( &_buffer );
                } else {
                    try {
                        _file.get( &_buffer );
                    }
                    catch( const Eof_error ) { goto OK; }
                }
            }

            _read_ptr = _buffer.char_ptr();
            _read_end = _read_ptr + _buffer.length();
        }

        while( len_count  &&  _read_ptr < _read_end ) {
            rest_length += (int4)*(Byte*)_read_ptr++ << ( --len_count * 8 );
        }

        if( len_count == 0 )   // Satzlängenbytes gelesen?
        {
            int4 l = _read_end - _read_ptr;
            if( l > rest_length )  l = rest_length;
    
            buffer.append( _read_ptr, l );
    
            _read_ptr += l;
            rest_length -= l;
    
            if( rest_length == 0 )  break;  // Fertig
        }
    }

OK: ;
    //if( _buffer_pos > 0 ) {  
    //    while( _min_buffer_size < buffer.length()  &&  _min_buffer_size < INT_MAX / 2 )  _min_buffer_size *= 2;
    //} 
    //else ; // 2er-Potenzen beachten
}

} //namespace sos
