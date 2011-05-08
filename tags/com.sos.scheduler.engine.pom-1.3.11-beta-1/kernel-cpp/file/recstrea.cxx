#define MODULE_NAME "recstrea"
// recstrea.cpp
//                                                      Joacim Zschimmer

#if 0   // Modul auﬂer Betrieb


#include <precomp.h>
#include <stdlib.h>
#include <sos.h>
#include <xception.h>

#include <recstrea.h>
#include <absfile.h>

#if defined USE_SOS_STREAM

//------------------------------------------Record_streambuf::~Record_streambuf

Record_streambuf::~Record_streambuf()
{
    // zu sp‰t: sync();
}

//-------------------------------------------------------Record_streambuf::sync

int Record_streambuf::sync()
BEGIN
    int rc = overflow();
    setg( 0, 0, 0 );
    return rc;
END

//-------------------------------------------------Record_streambuf::underflow

int _Cdecl Record_streambuf::underflow()
BEGIN
    setp( 0, 0 );
    if( _get_next_record ) {
        _buffer.allocate_min( _buffer_size );  xc;

        _f->get_record( _buffer );
        ignore_exception( "TOOSHORT" ); xc;

        setg( _buffer.char_ptr(), _buffer.char_ptr(),
              _buffer.char_ptr() + _buffer.length() );

        _get_next_record = false;

        return _buffer.char_ptr()[ 0 ];
    } else {
        _buffer.length( 0 );
      //_get_next_record = true;
        return EOF;                            // "Satzende"
    }

  exception_handler:
    return EOF;
END

//--------------------------------------------------Record_streambuf::overflow

int _Cdecl Record_streambuf::overflow( int b )
BEGIN
    _get_next_record = true;

    if (b != EOF) {
        _buffer.allocate_min( _buffer_size );  xc;
        _buffer.char_ptr()[ 0 ] = b;
        _buffer.length( 1 );
        setp( _buffer.char_ptr() + 1, _buffer.char_ptr() + _buffer.size() );
    } else {
        _buffer.set_length( _buffer.length() + pptr() - pbase() );
        if( _buffer.length() ) {
            _f->put_record( _buffer );  xc;
        }
        setp( _buffer.char_ptr(), _buffer.char_ptr() + _buffer.size() );
        _buffer.length( 0 );
    }

    return 0;

  exception_handler:
    _buffer.length( 0 );
    return EOF;         // Exception bleibt stehen
END

//-------------------------------------------------Record_stream::Record_stream

Record_stream::Record_stream()
  : _record_streambuf ( this ),
    Streambuf_stream  ( &_record_streambuf )
{}

//---------------------------------------------------Record_stream::skip_record

uint Record_stream::skip_record()
BEGIN
    uint  len = 0;

    while( get() >= 0 )  len++;
    return len;
END

#endif

#endif
