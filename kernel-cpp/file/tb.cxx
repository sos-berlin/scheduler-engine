// $Id: tb.cxx 11394 2005-04-03 08:30:29Z jz $
// tb.cxx                                     ©1997 SOS Software Gmbh
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

//------------------------------------------------------------------------------------Tb_file

struct Tb_file : Abs_file
{
                                    Tb_file             ();
                                   ~Tb_file             ();

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
};

//--------------------------------------------------------------------------------Tb_file_type

struct Tb_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "tb"; }
  //virtual const char*         alias_name              () const { return "nl"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Tb_file> f = SOS_NEW( Tb_file() );
        return +f;
    }
};

const Tb_file_type      _tb_file_type;
const Abs_file_type&     tb_file_type = _tb_file_type;

// --------------------------------------------------------------------Tb_file::Tb_file

Tb_file::Tb_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Tb_file::~Tb_file

Tb_file::~Tb_file()
{
}

// -------------------------------------------------------------------------Tb_file::open

void Tb_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    if( ( open_mode & inout ) == inout )  throw_xc( "SOS-1236" );

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.pipe() )                                filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _file.obj_owner( this );
    _buffer.allocate_min( 2000 );

    _file.open( filename, open_mode, file_spec );
}

// --------------------------------------------------------------------------Tb_file::close

void Tb_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// -------------------------------------------------------------------------Tb_file::rewind

void Tb_file::rewind( Key::Number )
{
    _file.rewind();
}

// ----------------------------------------------------------------------Tb_file::put_record

void Tb_file::put_record( const Const_area& record )
{
    int         soll_spalte = 0;
    int         ist_spalte  = 0;
    const char* p           = record.char_ptr();
    const char* p_end       = p + record.length();

    _buffer.allocate_min( record.length() + 1000 );
    _buffer.length( 0 );

    while( p < p_end )
    {
        switch( *p )
        {
            case ' ':
            {
                soll_spalte++;
                p++;
                break;
            }

            case '\r':
            case '\n':
            {
                soll_spalte = 0;
                _buffer.append( *p++ );
                ist_spalte = 0;
                break;
            }

            case '\t':
            {
                soll_spalte = ( soll_spalte + 8 ) & ~7;
                p++;
                break;
            }

            default:
            {
                while( ( soll_spalte & ~7 ) > ist_spalte )  
                {
                    _buffer.append( '\t' );  
                    ist_spalte = ( ist_spalte + 8 ) & ~7; 
                }
                _buffer.append( "        ", soll_spalte - ist_spalte );
                _buffer.append( *p++ ); 
                soll_spalte++;
                ist_spalte = soll_spalte;
            }
        }
    }

    // Blanks und Tabs am Ende werden ignoriert!

    _file.put( _buffer );
}

// ---------------------------------------------------------------------Tb_file::get_record

void Tb_file::get_record( Area& buffer )
{
    _file.get( &_buffer );

    buffer.allocate_min( _buffer.length() );

    const char* p = _buffer.char_ptr();
    const char* z = p + _buffer.length();

    while(1) 
    {
        const char* p2 = (char*)memchr( p, '\t', z - p );
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
}

} //namespace sos
