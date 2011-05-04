// nlfile.cxx                                     ©1997 SOS Software Gmbh
// Joacim Zschimmer




#include "precomp.h"

#include <limits.h>

#include <ctype.h>
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/soslimtx.h"

namespace sos {

//------------------------------------------------------------------------------------nl_file

struct nl_file : Abs_file //Filter_file
{
                                    nl_file             ();
                                   ~nl_file             ();

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
    Sos_limited_text<2>            _nl;
    char                           _record_end_char;
    Bool                           _no_tabs;
};

//----------------------------------------------------------------------Nl_file_type

struct Nl_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "nl"; }
  //virtual const char*         alias_name              () const { return "nl"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<nl_file> f = SOS_NEW( nl_file() );
        return +f;
    }
};

const Nl_file_type      _nl_file_type;
const Abs_file_type&     nl_file_type = _nl_file_type;

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

// --------------------------------------------------------------------nl_file::nl_file

nl_file::nl_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------nl_file::~nl_file

nl_file::~nl_file()
{
}

// -------------------------------------------------------------------------nl_file::open

void nl_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    _nl = SYSTEM_NL;
    _record_end_char = '\n';
    _no_tabs = true;

    //jz 24.12.00 (Dateityp p)  if( ( open_mode & inout ) == inout )  throw_xc( "SOS-1236" );

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

// --------------------------------------------------------------------------nl_file::close

void nl_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// -------------------------------------------------------------------------nl_file::rewind

void nl_file::rewind( Key::Number )
{
    _file.rewind();

    _read_ptr   = NULL;
    _read_end   = NULL;
    //_buffer_pos = 0;
    _eof = false;
}

// ----------------------------------------------------------------------nl_file::put_record

void nl_file::put_record( const Const_area& record )
{
    _buffer.allocate_length( record.length() + length( _nl ) );

    memcpy( _buffer.ptr(), record.ptr(), record.length() );
    memcpy( _buffer.char_ptr() + record.length(), _nl.ptr(), length( _nl ) );

    _file.put( _buffer );
}

// ---------------------------------------------------------------------nl_file::get_record

void nl_file::get_record( Area& buffer )
{
    //int  len_count     = 3;   // Soviele Längenbytes noch zu lesen
    //int  rest_length   = 0;   

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

        char* p = _read_ptr;
        char* z = (char*)memchr( p, _record_end_char, _read_end - p );
        if( !z )  z = _read_end;
        _read_ptr = z;

        while(1) {   // bis alle '\t' abgearbeitet
            char* p2 = _no_tabs? 0 : (char*)memchr( p, '\t', z - p );
            if( !p2 )  p2 = z;

            buffer.append( p, p2 - p );

            if( p2 == z )  break;

            uint n = ( buffer.length() + 8 ) & ~7;
            buffer.resize_min( n );
            memset( buffer.char_ptr() + buffer.length(), ' ', n - buffer.length() );
            buffer.length( n );

            p = p2 + 1;  // \t überspringen
            if( p == z )  break;
        }

        _read_ptr = z;
        if( _read_ptr < _read_end )  break;
    }

    if( _read_ptr && *_read_ptr == _record_end_char )  _read_ptr++;  // '\n' überspringen

    if( _record_end_char == '\n' 
     && buffer.length() > 0  &&  buffer.char_ptr()[ buffer.length() - 1 ] == '\r' ) 
    {
        buffer.length( buffer.length() - 1 );
    }

  OK: ;
}




} //namespace sos
