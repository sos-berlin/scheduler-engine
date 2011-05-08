//#define MODULE_NAME "inlinefl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"


namespace sos {

//----------------------------------------------------------------------------------Inline_file

struct Inline_file : Abs_file
{
                                Inline_file             ();

    void                        open                    ( const char*, Open_mode, const File_spec& );

  protected:
    void                        get_record              ( Area& area );

  private:
    Fill_zero                  _zero_;
    Sos_string                 _data;
    const char*                _ptr;
    const char*                _end_ptr;
};

//-----------------------------------------------------------------------------Inline_file_type

struct Inline_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "inline"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Inline_file> f = SOS_NEW_PTR( Inline_file() );
        return +f;
    }
};

const Inline_file_type         _inline_file_type;
const Abs_file_type&            inline_file_type = _inline_file_type;

//---------------------------------------------------------------------Inline_file::Inline_file

Inline_file::Inline_file()
:
    _zero_(this+1)
{
}

//----------------------------------------------------------------------------Inline_file::open

void Inline_file::open( const char* fn, Open_mode open_mode, const File_spec& )
{
    if( open_mode & out )  throw_xc( "SOS-INLINE-READONLY" );

    for( Sos_option_iterator opt( fn ); !opt.end(); opt.next() )
    {
      //if( opt.flag( "nl" ) ) _data += '\n';        // Sos_option_iterator erkennt nicht '', jz 8.11.98
      //else
        if( opt.param() ) {
            if( length( _data ) > 0 )  _data += '\n';
            _data += opt.value();
        }
        else throw_sos_option_error( opt );
    }

    _ptr = c_str( _data );
    _end_ptr = _ptr + length( _data );
}

//----------------------------------------------------------------------Inline_file::get_record

void Inline_file::get_record( Area& area )
{
    if( _ptr == _end_ptr )  throw_eof_error();

    const char* p = (char*)memchr( _ptr, '\n', _end_ptr - _ptr );
    if( p ) {
        const char* q = p;
        if( q > _ptr  &&  q[-1] == '\r' )  q--;    // "\r\n" ?
        if( q == _ptr  &&  p + 1 == _end_ptr )  throw_eof_error();   // '\n' am Ende liefert keine neue Zeile
        area.assign( _ptr, q - _ptr );
        _ptr = p + 1;
    } else {
        area.assign( _ptr, _end_ptr - _ptr );
        _ptr = _end_ptr;
    }
}


} //namespace sos
