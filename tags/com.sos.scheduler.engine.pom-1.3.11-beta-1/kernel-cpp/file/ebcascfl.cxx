//#define MODULE_NAME "convfile"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"

/* Konvertiert mit Field_ebcdic_asciier (sosfield.h) Datensätze.
   Z.B. C-Objekt des Aufrufers zu EBCDIC-Datensatz in der Datei u.u.
*/

namespace sos {

struct Ebcdic_ascii_file : Abs_file
{
                                Ebcdic_ascii_file         ();
                               ~Ebcdic_ascii_file         ();

    virtual void                close                  ( Close_mode );
    void                        open                   ( const char*, Open_mode, const File_spec& );
    virtual void                insert                 ( const Const_area& o );
    virtual void                store                  ( const Const_area& o );
    virtual void                update                 ( const Const_area& o );
    virtual void                set                    ( const Key& k );
    virtual void                del                    ();
    virtual void                del                    ( const Key& k );

  //virtual Record_position     key_position           ( Key::Number = 0 )                      { return _file.key_position(); }
  //virtual Record_length       key_length             ( Key::Number = 0 )                      { return _file.key_length(); }

    virtual File_info           info                   ()                                       { return _file.info(); }

  //static void                 erase                  ( const char* filename );

  protected:
    virtual void                get_record             ( Area& o );
  //virtual void                get_record_lock        ( Area& o, Record_lock lock );
    virtual void                get_record_key         ( Area& o, const Key& k );
    virtual void                put_record             ( const Const_area& o );
    void                        any_file_ptr           ( Any_file* p ) { _any_file_ptr = p; }

  private:
    Any_file*                  _any_file_ptr;
    Any_file                   _file;
    Dynamic_area               _buffer;
};

//----------------------------------------------------------------------statics

struct Ebcdic_ascii_file_type : Abs_file_type
{
    virtual const char* name() const { return "ebcdic/ascii"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Ebcdic_ascii_file> f = SOS_NEW_PTR( Ebcdic_ascii_file );
        return +f;
    }
};

const Ebcdic_ascii_file_type  _ebcdic_ascii_file_type;
const Abs_file_type&        ebcdic_ascii_file_type = _ebcdic_ascii_file_type;

//---------------------------------------------------------------Ebcdic_ascii_file::Ebcdic_ascii_file

Ebcdic_ascii_file::Ebcdic_ascii_file()
{
}

//--------------------------------------------------------------Ebcdic_ascii_file::~Ebcdic_ascii_file

Ebcdic_ascii_file::~Ebcdic_ascii_file()
{
}

//-------------------------------------------------------------------------Ebcdic_ascii_file::open

void Ebcdic_ascii_file::open( const char* param, Open_mode mode, const File_spec& spec )
{
    //if( name[ 0 ] == '|' )  name++;     // pipe

    Sos_string filename;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if ( opt.pipe() )                               filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    mode = Open_mode( mode & ~binary );        // -binary von -cobol-type zurücknehmen
    _file.open( filename, mode, spec );

    _key_pos = _file.key_position();
    _key_len = _file.key_length();
}

//-------------------------------------------------------------------------Ebcdic_ascii_file::close

void Ebcdic_ascii_file::close( Close_mode mode )
{
    _file.close( mode );
}

void Ebcdic_ascii_file::get_record( Area& o )
{
    _file.get( &o );
    o.xlat( iso2ebc );
}
/*
void Ebcdic_ascii_file::get_record_lock( Area& o, Record_lock lock )
{
    _file.get_lock( &_buffer, lock );
    _rec_to_obj( &o );
}
*/
void Ebcdic_ascii_file::get_record_key( Area& o, const Key& k )
{
    _buffer.assign( k );
    _buffer.xlat( ebc2iso );
    _file.get_key( &o, _buffer );
    o.xlat( iso2ebc );
}

void Ebcdic_ascii_file::put_record( const Const_area& o )
{
    _buffer.allocate_min( o.length() );
    xlat( _buffer.byte_ptr(), o.byte_ptr(), o.length(), ebc2iso );
    _buffer.length( o.length() );
    _file.put( _buffer );
}

void Ebcdic_ascii_file::insert( const Const_area& o )
{
    _buffer.allocate_min( o.length() );
    xlat( _buffer.byte_ptr(), o.byte_ptr(), o.length(), ebc2iso );
    _buffer.length( o.length() );
    _file.insert( _buffer );
}

void Ebcdic_ascii_file::store( const Const_area& o )
{
    _buffer.allocate_min( o.length() );
    xlat( _buffer.byte_ptr(), o.byte_ptr(), o.length(), ebc2iso );
    _buffer.length( o.length() );
    _file.store( _buffer );
}

void Ebcdic_ascii_file::update( const Const_area& o )
{
    _buffer.allocate_min( o.length() );
    xlat( _buffer.byte_ptr(), o.byte_ptr(), o.length(), ebc2iso );
    _buffer.length( o.length() );
    _file.update( _buffer );
}


void Ebcdic_ascii_file::set( const Key& k )
{
    _buffer.assign( k );
    _buffer.xlat( ebc2iso );
    _file.set( _buffer );
}


void Ebcdic_ascii_file::del()
{
    _file.del();
}

void Ebcdic_ascii_file::del( const Key& k )
{
    _buffer.assign( k );
    _buffer.xlat( ebc2iso );
    _file.del( _buffer );
}


} //namespace sos
