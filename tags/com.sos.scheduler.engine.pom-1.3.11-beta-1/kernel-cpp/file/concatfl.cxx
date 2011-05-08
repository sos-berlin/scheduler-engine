//#define MODULE_NAME "concatfl"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann"

#include "precomp.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"

/*
    Nur schreiben: hängt nehrere Sätze zu einem Satz zusammen (Zeilenende angebbar)
*/


namespace sos {


struct Concat_file : Abs_file
{
                                Concat_file             ();
                               ~Concat_file             ();

    virtual void                close                  ( Close_mode );
    void                        open                   ( const char*, Open_mode, const File_spec& );
    virtual void                insert                 ( const Const_area& o );

    virtual File_info           info                   ()                                       { return _file.info(); }

  //static void                 erase                  ( const char* filename );

  protected:
    virtual void                get_record             ( Area& area );
    virtual void                put_record             ( const Const_area& o );
    void                        any_file_ptr           ( Any_file* p ) { _any_file_ptr = p; }

  private:
    Bool                        end_of_line( const Const_area& area );

    char                       _zeilentrenner;
    Sos_string                 _eol_string;
    Any_file*                  _any_file_ptr;
    Any_file                   _file;
    Dynamic_area               _buffer;
    Bool                       _eof;
};

//----------------------------------------------------------------------statics

struct Concat_file_type : Abs_file_type
{
    virtual const char* name() const { return "concat"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Concat_file> f = SOS_NEW_PTR( Concat_file );
        return +f;
    }
};

const Concat_file_type  _concat_file_type;
const Abs_file_type&    concat_file_type = _concat_file_type;

//---------------------------------------------------------------Concat_file::Concat_file

Concat_file::Concat_file() :
    _zeilentrenner(' '),
    _eol_string( "EOL" ),
    _eof( false )
{
}

//--------------------------------------------------------------Concat_file::~Concat_file

Concat_file::~Concat_file()
{
}

//-------------------------------------------------------------------------Concat_file::open

void Concat_file::open( const char* param, Open_mode mode, const File_spec& spec )
{
    if ( mode & in && mode & out ) throw_xc( "D049" );
    _buffer.allocate_min(8192);
    //if( name[ 0 ] == '|' )  name++;     // pipe

    Sos_string filename;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "eol"   ))         _eol_string = opt.value();
        else
        if( opt.flag( "nl"   ))                _eol_string = "";
        else
        if ( opt.pipe() )                       filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _file.open( filename, mode, spec );
}

//-------------------------------------------------------------------------Concat_file::end_of_line

Bool Concat_file::end_of_line( const Const_area& area )
{
    return Bool( ( length(area) == 0 && length(_eol_string) == 0 ) ||
                 ( length(area) >= length(_eol_string) &&
                   strncmpi( area.char_ptr(), c_str(_eol_string), length(_eol_string) ) == 0 ) );
 }

//-------------------------------------------------------------------------Concat_file::close

void Concat_file::close( Close_mode mode )
{
    if ( _buffer.length() > 0 ) _file.put( _buffer );
    _buffer.length(0);
    _file.close( mode );
}

void Concat_file::put_record( const Const_area& o )
{
    if ( end_of_line(o) ) {
        _file.put( _buffer );
        _buffer.length(0);
    } else {
        _buffer.append( o.char_ptr(), o.length() );
        _buffer += _zeilentrenner; // Satztrenner
    }
}

void Concat_file::insert( const Const_area& o )
{
    put_record( o );
}

void Concat_file::get_record( Area& area )
{
    if ( _eof ) throw_eof_error();
    area.length(0);
    while (1) {
        try { _file.get( &_buffer ); } catch ( const Eof_error& ) { _eof = true; break; }
        if ( end_of_line( _buffer ) ) break;
        area.append( _buffer.char_ptr(), _buffer.length() );
        area += _zeilentrenner; // Satztrenner
    }
}


} //namespace
