//#define MODULE_NAME "convfile"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/soslimtx.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/cobfield.h"
#include "../kram/frmfield.h"
#include "../kram/sqlfield.h"
#include "../kram/typereg.h"
#include "../file/absfile.h"

/* Konvertiert mit Field_converter (sosfield.h) Datensätze.
   Z.B. C-Objekt des Aufrufers zu EBCDIC-Datensatz in der Datei u.u.
*/

namespace sos {

struct Convert_file : Abs_file
{
                                Convert_file         ();
                               ~Convert_file         ();

    void                        prepare_open           ( const char*, Open_mode, const File_spec& );
    void                        open                   ( const char*, Open_mode, const File_spec& );
    virtual void                close                  ( Close_mode );
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

  private:
    const Area&                _obj_to_rec             ( const Const_area& );
    void                       _rec_to_obj             ( Area* o )                              { o->allocate_min( _object_size ); _convert.read( o->ptr(), _record ); o->length( _object_size ); }
    const Area&                _key_to_rec             ( const Const_area& );

    Fill_zero                  _zero_;
    Sos_ptr<Record_type>       _object_type;
    int                        _object_size;
    Sos_ptr<Record_type>       _record_type;
  //Sos_ptr<Field_descr>       _key_object_descr;
    Field_converter            _convert;
    Field_converter            _key_convert;
    Any_file                   _file;
    Dynamic_area               _record;
    Bool                       _key_da;
};

//----------------------------------------------------------------------statics

struct Convert_file_type : Abs_file_type
{
    virtual const char* name() const { return "convert"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Convert_file> f = SOS_NEW_PTR( Convert_file );
        return +f;
    }
};

const Convert_file_type  _convert_file_type;
const Abs_file_type&      convert_file_type = _convert_file_type;

//----------------------------------------------------------------------------get_field_numbers
/*
void get_field_numbers( const Sos_ptr<Record_type>& record_type,
                        const Sos_ptr<Record_type>& key_type,
                        Sos_array<int>* number_array )
{
    if( !record_type )  throw_xc( "SOS-1193", "get_field_numbers" );

    int first = number_array->first_index();

    number_array->last_index( key_type.field_count() - first  + 1 );

    for( int i = 0; i < key_type->field_count(); i++ ) {
        (*number_array)[ first + i ] =
            record_type->field_index( key_type->field_descr_ptr( i )->name() );
    }
}
*/
//----------------------------------------------------------------------------get_field_numbers
/* Vielleicht brauchen wir diese Funktion noch mal:

void get_field_numbers( const Sos_ptr<Record_type>& record_type, const Sos_string& fields,
                        Sos_array<int>* number_array )
{
    number_array->last_index( number_array->first_index() - 1 );
    if( !record_type )  throw_xc( "SOS-1193", "get_field_numbers" );

    Bool        geklammert = false;
    const char* p0 = c_str( fields );
    const char* p = p0;

    if( *p == '(' )  { p++; geklammert = true; }

    while(1) {
        Sos_limited_text<max_field_name_length> name;
        while( *p == ' ' )  p++;
        while( *p  &&  *p != ',' )  name += *p++;

        if( length( name ) == 0 )  throw_syntax_error( "SOS-1184", "<feldname>", p - p0 );

        number_array->add( record_type->field_index( c_str( name ) ) );

        if( *p != ',' )  break;
        p++;
    }

    while( *p == ' ' )  p++;

    if( geklammert )  {
        if( *p != ')' )  throw_syntax_error( "SOS-1184", ")", p - p0 );
        p++;
        while( *p == ' ' )  p++;
    }

    if( *p )  throw_syntax_error( "SOS-1184", "Parameterende", p - p0 );
}
*/
//---------------------------------------------------------------record_type_of_selected_fields
/*
Sos_ptr<Record_type> record_type_of_selected_fields( const Sos_ptr<Record_type>& record_type,
                                                     const Sos_array<int>& field_numbers,
                                                     const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    if( !record_type )  throw_xc( "SOS-1193", "record_type_of_selected_fields" );

    Sos_ptr<Dyn_record_type> t = SOS_NEW_PTR( Dyn_record_type );;

    if( new_type_name != "" ) t->name( new_type_name );
    else
    if( length( record_type->name() ) < 100 )  t->name( record_type->name() + "/selected" );

    for( int i = field_numbers.first_index(); i <= field_numbers.last_index(); i++ )
    {
        Field_descr* f = record_type->field_descr_ptr( field_numbers[ i ] );
        t->add_field( f );
    }

    return +t;
}
*/
//---------------------------------------------------------------Convert_file::Convert_file

Convert_file::Convert_file()
:
    _zero_ ( this+1 )
{
}

//--------------------------------------------------------------Convert_file::~Convert_file

Convert_file::~Convert_file()
{
}

//--------------------------------------------------------------------Convert_file::_key_to_rec

const Area& Convert_file::_key_to_rec( const Const_area& k )
{
    if( !_key_da )  throw_xc( "SOS-1214", "convert" );
    _record.length( 0 );
    _key_convert.write( k.ptr(), &_record );
    return _record;
}

//-------------------------------------------------------------------Convert_file::prepare_open

void Convert_file::prepare_open( const char* name, Open_mode mode, const File_spec& spec_ )
{
    Sos_ptr<Field_descr> key_object_descr;
    Sos_ptr<Record_type> key_object_type;
    Sos_ptr<Field_descr> key_record_descr;
    Sos_ptr<Record_type> key_record_type;
    Sos_string           fields;
    Sos_string           key_names;
    Bool                 empty_is_null = false;
    Bool                 use_names     = false;
    Sos_string           filename;

    for( Sos_option_iterator o ( name ); !o.end(); o.next() )
    {
        if( o.flag( "empty-is-null"  ) )  empty_is_null = o.set();
        else
        if( o.flag( "use-names"      ) )  use_names = o.set();          // Nicht Tupel, sondern Feldnamen verwenden
        else
        if( o.pipe() )                    { filename = o.rest();  break; }
        else
        throw_sos_option_error( o );
    }

    File_spec  spec = spec_;

    /// Satzbeschreibung des Aufrufers
    if( !spec._field_type_ptr )  throw_xc( "SOS-1191" );
    _object_type = +spec._field_type_ptr;
    spec._field_type_ptr = 0;
    _object_size = _object_type->field_size();

    /// Schlüsselbeschreibung des Aufrufers
    if( spec._key_specs._key_spec._field_descr_ptr ) {
        key_object_descr = +spec._key_specs._key_spec._field_descr_ptr;
        key_object_type = SOS_CAST( Record_type, +key_object_descr->type_ptr() );
        spec._key_specs._key_spec._field_descr_ptr = 0;
    }


    _file.prepare_open( filename, mode, spec );

    _key_pos = _file.key_position();
    _key_len = _file.key_length();


    /// Satzbeschreibung
    _record_type = +_file.spec()._field_type_ptr;
    if( !_record_type )  throw_xc( "SOS-1188" );    // Keine mit Option zugewiesen?

    if( use_names ) {
        _convert.prepare_for_equal_names( _object_type, _record_type );
    } else {
        _convert.prepare_for_tuples( _object_type, _record_type );
    }

    _convert.empty_is_null( empty_is_null );


    /// Schlüsselbeschreibung
    if( _file.spec()._key_specs._key_spec._field_descr_ptr ) {  // Schlüsselbeschreibung der Datei
        key_record_descr = +_file.spec()._key_specs._key_spec._field_descr_ptr;
        key_record_type  = SOS_CAST( Record_type, +key_record_descr->type_ptr() );

        if( !key_object_type ) {
            // Key-Typ für die Objekt-Seite aufbauen: Über Feldpositionen zuordnen.

            key_object_type = Record_type::create();
            key_object_type->allocate_fields( key_record_type->field_count() );
            key_object_type->name( "key" );

            for( int i = 0; i < key_record_type->field_count(); i++ ) {
                key_object_type->add_field(
                    _object_type->field_descr_ptr(
                        _record_type->field_index(
                            key_record_type->field_descr_ptr( i )->name() ) ) );
            }
        }

        _key_convert.prepare_for_tuples( key_object_type, key_record_type );
        _key_convert.empty_is_null( empty_is_null );    // ?
        _key_da = true;
    }

    _record.allocate_min( _record_type->field_size() );
    _record_type->clear( _record.byte_ptr() );       // if use_names: Nicht alle Felder müssen berücksichtigt sein
}

//---------------------------------------------------------------------------Convert_file::open

void Convert_file::open( const char*, Open_mode, const File_spec& )
{
    _file.open();
}

//-------------------------------------------------------------------------Convert_file::close

void Convert_file::close( Close_mode mode )
{
    _file.close( mode );
}

void Convert_file::get_record( Area& o )
{
    _file.get( &_record );
    _rec_to_obj( &o );
}
/*
void Convert_file::get_record_lock( Area& o, Record_lock lock )
{
    _file.get_lock( &_record, lock );
    _rec_to_obj( &o );
}
*/
void Convert_file::get_record_key( Area& o, const Key& k )
{
    _file.get_key( &_record, _key_to_rec( k ) );
    _rec_to_obj( &o );
}

void Convert_file::put_record( const Const_area& o )
{
    _file.put( _obj_to_rec( o ) );
}

void Convert_file::insert( const Const_area& o )
{
    _file.insert( _obj_to_rec( o ) );
}

void Convert_file::store( const Const_area& o )
{
    _file.store( _obj_to_rec( o ) );
}

void Convert_file::update( const Const_area& o )
{
    _file.update( _obj_to_rec( o ) );
}


void Convert_file::set( const Key& k )
{
    _file.set( _key_to_rec( k ) );
}


void Convert_file::del()
{
    _file.del();
}

void Convert_file::del( const Key& k )
{
    _file.del( _key_to_rec( k ) );
}

//--------------------------------------------------------------------Convert_file::_obj_to_rec

const Area& Convert_file::_obj_to_rec( const Const_area& o )                  
{ 
    _convert.write( o.ptr(), &_record ); 
    return _record; 
}

} //namespace
