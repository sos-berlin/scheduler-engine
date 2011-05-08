//#define MODULE_NAME "progfile"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"
#if defined SYSTEM_UNIX

#include <values.h>                 // MAXINT
#include <limits.h>                 // MAX_PATH
#include <stdlib.h>
#include <string.h>
#include <stdio.h>                  // S_IREAD
#include <errno.h>

#include "../kram/sosstrg0.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"
#include "../file/absfile.h"
#include "../kram/env.h"



#if defined SYSTEM_WIN || defined SYSTEM_DOS
//    const int O_BINARY = 0;
#endif

namespace sos {


struct Program_file : Abs_file
{
                                Program_file            ();
                               ~Program_file            ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );

  protected:
    virtual void                get_record              ( Area& );
    virtual void                put_record              ( const Const_area& );

  private:
    Fill_zero                  _zero_;
    FILE*                      _file;
    uint                       _fixed_length;
    Bool                       _crlf;
    Dynamic_area               _buffer;
    unsigned char              _record_separator[1];
};


//----------------------------------------------------------------------statics

struct Program_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "program"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Program_file> f = SOS_NEW_PTR( Program_file );
        return +f;
    }
};

const Program_file_type   _program_file_type;
const Abs_file_type&       program_file_type = _program_file_type;


//-------------------------------------------------------------------Program_file::Program_file

Program_file::Program_file()
:
    _zero_ (this+1)
{
    set_environment_from_sos_ini();

    _crlf = true;
    _record_separator[0] = '\n';
}

//------------------------------------------------------------------Program_file::~Program_file

Program_file::~Program_file()
{
}

//---------------------------------------------------------------------------Program_file::open

void Program_file::open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_option_iterator opt ( filename );
    Sos_string rs;

    for(; !opt.end(); opt.next() ) {
        if( opt.with_value( "fixed-length" ) )  { _fixed_length = as_int( opt.value() ); _crlf = false; }
        else
        if( opt.with_value( "rs" ) )            { rs = opt.value(); _crlf = true; }     // record separator
        else
        break;
    }

    if ( rs != empty_string ) {
        int val = as_int( rs );
        if ( val > 255 ) throw_xc( "Progfile", "rs" );
        _record_separator[0] = (unsigned char) val;
    }

    _file = popen( c_str( opt.rest() ), open_mode & out? "w" : "r" );
    if( !_file )  throw_errno( errno, filename );
}

//--------------------------------------------------------------------------Program_file::close

void Program_file::close( Close_mode )
{
    pclose( _file );
    _file = NULL;
}

//-------------------------------------------------------------------------Program_file::get_record

void Program_file::get_record( Area& area )
{
    if( _fixed_length )
    {
        area.allocate_min( _fixed_length );
        int len = fread( area.ptr(), _fixed_length, 1, _file );

        if( len < area.size() ) {
            if( feof( _file ) )  throw_eof_error( "D310", this );    // Der letzte, zu kurze Satz geht verloren
            throw_errno( errno, this );
        }

        area.length( _fixed_length );
    }
    else
    {
        _buffer.allocate_min( 1024 + 2 );

        if( !fgets( _buffer.char_ptr(), _buffer.size(), _file ) )  {
            if( feof( _file ) )  throw_eof_error( "D310", this );    // Der letzte, zu kurze Satz geht verloren
            throw_errno( errno, this );
        }
        int len = strlen( _buffer.char_ptr() );
        if( len > 0  &&  _buffer.char_ptr()[ len - 1 ] == _record_separator[0] )  len--;
        area.assign( _buffer.char_ptr(), len );
    }
}


//-------------------------------------------------------------------------Program_file::put_record

void Program_file::put_record( const Const_area& area )
{
    int rc;

    if( area.length() > (uint)MAXINT)  throw_too_long_error( "D420", this );

    if( _fixed_length ) {
        if( area.length() != _fixed_length )  throw_xc( "D422", area.length(), _fixed_length );

        rc = fwrite( area.ptr(), 1, area.length(),  _file );
        if( rc != area.length() )  throw_errno( errno, this );
    } else {
        if ( area.length() > 0 ) {
            rc = fwrite( area.ptr(), 1, area.length(),  _file );
            if( rc != area.length() )  throw_errno( errno, this );
        }

        if ( _crlf ) {
            rc = fwrite( _record_separator, 1, 1, _file );
            if( rc != 1 )  throw_errno( errno, this );
        }
    }
}

} //namespace sos
#endif
