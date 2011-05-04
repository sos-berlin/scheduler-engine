//#define MODULE_NAME "flstream"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sos.h"
#include "anyfile.h"
#include "flstream.h"


namespace sos {
using namespace std;


//-------------------------------------------------------Any_file_streambuf::Any_file_streambuf

Any_file_streambuf::Any_file_streambuf()
:
    _close_it ( false )
{
}

//-------------------------------------------------------Any_file_streambuf::Any_file_streambuf

Any_file_streambuf::Any_file_streambuf( const Any_file& file )
:
    _any_file ( SOS_NEW( Any_file( file ) ) ),
    _close_it ( false )
{
}

//-------------------------------------------------------Any_file_streambuf::Any_file_streambuf

Any_file_streambuf::~Any_file_streambuf()
{
    if( _close_it ) {
        try {
            close();
            _close_it = false;
        }
        catch( const Xc& ) {}
    }

    SOS_DELETE( _any_file );
}

//---------------------------------------------------------------------Any_file_streambuf::init

void Any_file_streambuf::init( const Any_file& file )
{
    _any_file = SOS_NEW( Any_file( file ) );
}

//---------------------------------------------------------------------Any_file_streambuf::open

void Any_file_streambuf::open( const Sos_string& filename, Any_file::Open_mode mode  )
{
    _close_it = true;
    _any_file = SOS_NEW( Any_file );
    _any_file->open( filename, mode );
}

//--------------------------------------------------------------------Any_file_streambuf::close

void Any_file_streambuf::close( Close_mode mode )
{
    if( _any_file ) 
    {
        overflow();
        _any_file->close( mode );
        SOS_DELETE( _any_file );
    }
}

//---------------------------------------------------------------------Any_file_streambuf::sync

int Any_file_streambuf::sync()
{
    if( !_any_file )  return 0;
    
    setg( 0, 0, 0 );
    return overflow();
}

//-------------------------------------------------------------Any_file_streambuf::underflow

int _Cdecl Any_file_streambuf::underflow()
{
    if( gptr() < egptr() ) {
        int c = *gptr();
        //stossc();
        return c;
    }

    try {
        _any_file->get( &_read_buffer );
        _read_buffer += '\n';
        setg( _read_buffer.char_ptr(),
              _read_buffer.char_ptr(),
              _read_buffer.char_ptr() + _read_buffer.length() );
        return *_read_buffer.char_ptr();
    }
  //catch( const Xc& ) {
    catch( const Eof_error& ) {
        return EOF;
    }
}

//-----------------------------------------------------------------Any_file_streambuf::overflow

int _Cdecl Any_file_streambuf::overflow( int b )
{
    //try {
        if( pptr() > pbase() )  _write_buffer.length( pptr() - _write_buffer.char_ptr() );

        if( b != EOF ) {
            if( !_write_buffer.size() )  _write_buffer.allocate_min( 128+1 );  // Nicht zu groß wegen memmove()
            _write_buffer += (char)b;
        }

        while( _write_buffer.length() )
        {
            char* nl = (char*)memchr( _write_buffer.char_ptr(), '\n', _write_buffer.length() );
            if( !nl )  break;

            _any_file->put( Const_area( _write_buffer.char_ptr(), nl - _write_buffer.char_ptr() ) );

            int len = _write_buffer.char_ptr() + _write_buffer.length() - ( nl + 1 );
            memmove( _write_buffer.char_ptr(), nl + 1, len );
            _write_buffer.length( len );
        }

        if( b == EOF  &&  _write_buffer.length() )  {
            _any_file->put( _write_buffer );
            _write_buffer.length( 0 );
        }

        if( _write_buffer.length() == _write_buffer.size() )  _write_buffer.resize_min( _write_buffer.size() + 128 );

        setp( _write_buffer.char_ptr() + _write_buffer.length(),
              _write_buffer.char_ptr() + _write_buffer.size()  );

        return 0;
    //}
    //catch( ... ) {
    //    return EOF;
    //}
}

//-------------------------------------------------------------Any_file_stream::Any_file_stream

Any_file_stream::Any_file_stream()
: 
    iostream           ( (Any_file_streambuf*)this ) 
{
}

//-------------------------------------------------------------Any_file_stream::Any_file_stream

Any_file_stream::Any_file_stream( const Any_file& file )
: 
    Any_file_streambuf ( file ),
    iostream           ( (Any_file_streambuf*)this ) 
{
}

//------------------------------------------------------------Any_file_stream::~Any_file_stream

Any_file_stream::~Any_file_stream() 
{ 
    iostream::sync(); 
}


} //namespace sos
