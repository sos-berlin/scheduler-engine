//#define MODULE_NAME "tabrec"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosfield.h"

#include "../kram/log.h"
#include "../kram/sosopt.h"


namespace sos {
using namespace std;


//---------------------------------------------------------------------------Tabbed_record_file

struct Tabbed_record_file : Abs_file
{
                                    Tabbed_record_file  ();
                                   ~Tabbed_record_file  ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );
    void                            insert              ( const Const_area& );
    void                            update              ( const Const_area& );
    void                            store               ( const Const_area& );
    static const Abs_file_type&     static_file_type    ();

  protected:
    void                            get_record          ( Area& area );
    void                            put_record          ( const Const_area& );

  private:
    void                            print_field_names   ( ostream*, const Field_type&, const char* = "" );

    Fill_zero                      _zero_;
    Any_file                       _file;
    Sos_ptr<Record_type>           _record_type_ptr;
    Dynamic_area                   _record;
    Dynamic_area                   _buffer;
  //Sos_ptr<Field_descr>           _key_descr_ptr;
    Text_format                    _format;
    Bool                           _field_names;
    char                           _separator;
  //Bool                           _transparent_write;

};

//----------------------------------------------------------------------Tabbed_record_file_type

struct Tabbed_record_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "tabbed/record"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Tabbed_record_file> f = SOS_NEW_PTR( Tabbed_record_file() );
        return +f;
    }
};

const Tabbed_record_file_type  _tabbed_record_file_type;
const Abs_file_type&            tabbed_record_file_type = _tabbed_record_file_type;

// --------------------------------------------------------------------Tabbed_record_file::Tabbed_record_file

Tabbed_record_file::Tabbed_record_file()
:
    _zero_(this+1)
{
    _field_names       = false;  // mit Feldnamen im ersten Satz

    _separator = '\t';
  //_format.char_quote(0);
  //_format.string_quote(0);
  //_format.with_nesting(false);
    _format.decimal_symbol('.');

    //current_key_ptr( &_current_key );
}

// -----------------------------------------------------Tabbed_record_file::~Tabbed_record_file

Tabbed_record_file::~Tabbed_record_file()
{
}

// --------------------------------------------------------------------Tabbed_record_file::open

void Tabbed_record_file::open( const char* parameter, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    // Dateityp-Optionen parsieren
    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
        if ( opt.flag( "field-names" ) )        _field_names = opt.set();
        else
      //if ( opt.flag( "transparent-write" ) )  _transparent_write = opt.set();
      //else
        if ( opt.pipe()                      )   { filename = opt.rest();  break; }
        else throw_sos_option_error( opt );
    }

    _file.obj_owner( this );
    _file.open( filename, open_mode, file_spec );

    _record_type_ptr = (Record_type*)_file.spec().field_type_ptr();
    if( !_record_type_ptr )  throw_xc( "SOS-1193" );

    if( _file.spec().key_count() ) {
        //_key_descr_ptr   = _file.spec().key_specs()[ 0 ].field_descr_ptr();
    }
}

// -----------------------------------------------------------------Tabbed_record_file::print_field_names

void Tabbed_record_file::print_field_names( ostream* s, const Field_type& record_type, const char* name )
{
    if( !record_type.obj_is_type( tc_Record_type ) ) {
        if ( name[0] == 0 ) throw_xc( "Tabbed_record_file::print_field_names" );
        *s << name;
    } else
    {
        const Record_type* t = (const Record_type*)&record_type;
        for( int i = 0; i < t->field_count(); i++ )
        {
            const Field_descr& field_descr = t->field_descr( i );

            if( i > 0 )  *s << _separator;
            print_field_names( s, field_descr.type(), field_descr.name() );
        }
    }
}

// --------------------------------------------------------------------------Tabbed_record_file::close

void Tabbed_record_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//---------------------------------------------------------------Tabbed_record_file::get_record

void Tabbed_record_file::get_record( Area& area )
{
    if( _field_names ) {
        _field_names = false;
        _buffer.allocate_min( 4096 ); // vielleicht noch groesser ?
        ostrstream s ( _buffer.char_ptr(), _buffer.size() );
        print_field_names( &s, *_record_type_ptr );
        _buffer.length( s.pcount() );
    } else {
        _file.get( &_record );
        _buffer.allocate_min( 4096 );
        ostrstream s ( _buffer.char_ptr(), _buffer.size() );
        if( _record.length() < _record_type_ptr->field_size() )  throw_xc( "tabbed/record", "Satz zu kurz" );
        //_record_type_ptr->print( _record.byte_ptr(), &s, _format );

        for( int i = 0; i < _record_type_ptr->field_count(); i++ )
        {
            if( i > 0 )  s << _separator;
            const Field_descr& f = _record_type_ptr->field_descr( i );
            f.print( _record.byte_ptr(), &s, _format );
        }

        _buffer.length( s.pcount() );
    }
    if ( _buffer.length() == _buffer.size() )
    {
        throw_too_long_error();
    }
    area.assign( _buffer );
}

// ---------------------------------------------------------------------Tabbed_record_file::put

void Tabbed_record_file::put_record( const Const_area& area )
{
    _record.allocate_length( _record_type_ptr->field_size() );
    memset( _record.ptr(), 0, _record_type_ptr->field_size() );

    if( _field_names ) {
        _field_names = false;
    } else {
        istrstream s ( (char*)area.char_ptr(), area.length() );
        //_record_type_ptr->input( _record.byte_ptr(), &s, _format );


        for( int i = 0; i < _record_type_ptr->field_count(); i++ )
        {
            const Field_descr& f = _record_type_ptr->field_descr( i );
            if( i > 0 ) {
                if( s.peek() != _separator )  throw_xc( "SOS-1168", this );
                s.get();
            }
  
            if( f.nullable()  &&  ( s.peek() == EOF  ||  s.peek() == _separator ) ) {
                f.set_null( _record.byte_ptr() );
            } else {
                f.input( _record.byte_ptr(), &s, _format );
            }
        }

        _file.put( _record );
    }
}

// ------------------------------------------------------------------Tabbed_record_file::insert

void Tabbed_record_file::insert( const Const_area& area )
{
    istrstream s ( (char*)area.char_ptr(), area.length() );
    _record_type_ptr->input( _record.byte_ptr(), &s, _format );
    _file.insert( _record );
}

// -------------------------------------------------------------------Tabbed_record_file::store

void Tabbed_record_file::store( const Const_area& area )
{
    istrstream s ( (char*)area.char_ptr(), area.length() );
    _record_type_ptr->input( _record.byte_ptr(), &s, _format );
    _file.store( _record );
}

// -------------------------------------------------------------------Tabbed_record_file::upate

void Tabbed_record_file::update( const Const_area& area )
{
    istrstream s ( (char*)area.char_ptr(), area.length() );
    _record_type_ptr->input( _record.byte_ptr(), &s, _format );
    _file.update( _record );
}

} //namespace sos
