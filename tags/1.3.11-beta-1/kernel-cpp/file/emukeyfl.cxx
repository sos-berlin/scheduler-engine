//#define MODULE_NAME "emukeyfl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

#include "precomp.h"
#include <limits.h>
#include <time.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
//#include <sosdate.h> // wg. std_date_format_ingres
//#include <soslist.h>
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"

#include "../kram/log.h"

#if 0

/*
    Zwei Anwendungen:

    1)  Basisdatei ist hat Schlüssel. Die Satzschlüssel werden gespeichert.

    2)  Basisdatei ist streng sequentiell. Die ganzen Sätze werden gespeichert.
*/


struct Emulate_key_file : Abs_file
{
                                    Emulate_key_file    ();
                                   ~Emulate_key_file    ();

    void                            prepare_open        ( const char*, Open_mode, const File_spec& );
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

    void                            insert              ( const Const_area& area );
    void                            update              ( const Const_area& area );
  //void                            store               ( const Const_area& area );

    void                            set                 ( const Key& );
  //void                            del                 ( const Key& key );
  //void                            del                 ();

  //Record_position                 key_position        (Key::Number ) { return _key_pos; }
  //Record_length                   key_length          (Key::Number ) { return _key_len; }

  protected:
    void                            get_record          ( Area& area );
    void                            get_record_key      ( Area& area, const Key& key );
    void                            put_record          ( const Const_area& area );

  private:
    void                           _cache_update        ( const Const_area& );
    void                           _cache_insert        ( const Const_area& );
    Bool                           _cache_get           ( Area*, const Const_area& key );
    void                           _cache_delete_key    ( const Const_area& key );

    int                            _cache_find_record   ( const Const_area& record );
    int                            _cache_find_key      ( const Const_area& key );
    void                           _cache_change_record ( int, const Const_area& );
    void                           _cache_delete        ( int );

    Fill_zero                      _zero_;
    Any_file                       _file;
    Dynamic_area                   _key;
    Bool                           _sort_key;
    Bool                           _set_key;
    int                            _key_pos;
    int                            _key_len;
    int                            _file_key_pos;       // == _file.key_position()
    int                            _file_key_len;       // == _file.key_length()
    Bool                           _key_in_record;      // == _file.key_in_record()
    int                            _file_record_no;
    int                            _record_no;          // 1. Satz == 1
    Sos_simple_array<Byte*>        _record_array;       // Satz/keylänge + Emulierter Schlüssel + ( Satz oder Schlüssel )
};

//------------------------------------------------------------------------Emulate_key_file_type

struct Emulate_key_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "emulate_key"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
    	Sos_ptr<Emulate_key_file> f = SOS_NEW( Emulate_key_file );
    	return +f;
    }
};

const Emulate_key_file_type    _emulate_key_file_type;
const Abs_file_type&            Emulate_key_file_type = _emulate_key_file_type;

// ----------------------------------------------------------Emulate_key_file::Emulate_key_file

Emulate_key_file::Emulate_key_file()
:
    _zero_ ( this+1 )
{
    _record_array.obj_const_name( "Emu_key_file::_record_array" );
     _record_array.first_index( 1 );
}

//----------------------------------------------------------Emulate_key_file::~Emulate_key_file

Emulate_key_file::~Emulate_key_file()
{
}

//---------------------------------------------------------------Emulate_key_file::prepare_open

void Emulate_key_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
	  //if( opt.with_value( "sort-key" ) )      _sort_key = opt.set();
	  //else
	    if( opt.pipe() )                      { filename = opt.rest(); break; }
	    else throw_sos_option_error( opt );
    }

    _file.prepare_open( filename, open_mode, file_spec );

    _any_file_ptr->_spec._field_type_ptr = _file.spec()._field_type_ptr;
}

//-----------------------------------------------------------------------Emulate_key_file::open

void Emulate_key_file::open( const char*, Open_mode, const File_spec& file_spec )
{
    _file_key_pos       = _file.key_position();
    _file_key_len       = _file.key_length();
    _key_in_record      = _file.key_in_record();

    if( file_spec.key_specs().key_length() ) {
        _key_len = file_spec.key_specs().key_length();
        _key_pos = _key_len? file_spec.key_specs().key_position() : -1;
    } else {
        Field_descr* f = file_spec.key_specs()._key_spec._field_descr_ptr;
        if( f ) {
            _key_len = f->type().field_size();
            _key_pos = f->offset();
        }
    }

    _file.open();
}

// ---------------------------------------------------------------------Emulate_key_file::close

void Emulate_key_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// ----------------------------------------------------------------Emulate_key_file::put_record

void Emulate_key_file::put_record( const Const_area& record )
{
    _file.put( record );
}

//-----------------------------------------------------------------Emulate_key_file::get_record

void Emulate_key_file::get_record( Area& buffer )
{
    if( _set_key ) {
        _find_key( _key );
        _set_key = false;
    }
    else
    if( _record_no == _file_record_no ) {
        _file.get( &buffer );
        _file_record_no++;
        _record_no++;
        _add_record( buffer );
    }

    _read_buffer( &buffer );
}

//----------------------------------------------------------------Emulate_key_file::_add_record

void Emulate_key_file::_add_record( const Const_area& buffer )
{
    Byte* p = sos_alloc( sizeof( uint ) + _key_len + buffer.length(), "Emulate_key_file" );
    _record_array.add( p );

    *(uint*)p = buffer.length();
    p += sizeof (uint);

    memcpy( p, buffer.byte_ptr() + _key_pos, _key_len );
    p += _key_len;

    if( _key_in_record ) {
        if( buffer.length() < _file_key_pos + _file_key_len )  throw_xc( "SOS-EMUKEY", "buffer.length() < _file_key_pos + _file_key_len" );
        memcpy( p, buffer.ptr() + _key_pos, _file_key_len );
    } else {
        Const_buffer k = _file.current_key();
        if( k.length() != _key_length )  throw_xc( "SOS-EMUKEY", "k.length() != _key_length" );
        memcpy( p, k.ptr(), _key_len );
    }
}

//-------------------------------------------------------------Emulate_key_file::get_record_key

void Emulate_key_file::get_record_key( Area& area, const Key& key )
{
    Bool ok = _cache_get( &area, key );
    if( ok ) {
        _key = key;
        incr( &_key );
        _set_key = true;
    } else {
        _file.get_key( &area, key );
        _cache_update( area );
    }
}

//---------------------------------------------------------------------Emulate_key_file::insert

void Emulate_key_file::insert( const Const_area& record )
{
    if( !_key_in_record )  throw_xc( "Emulate_key_file::insert" );
    if( record.length() < _key_pos + _key_len )  throw_xc( "RECORD-LENGTH" );

    Const_area key ( record.byte_ptr() + _key_pos, _key_len );

    int i = _find_key( key, false );


    _file.put( record );
    _add_record( record );
}

//---------------------------------------------------------------------Emulate_key_file::update

void Emulate_key_file::update( const Const_area& record )
{
    _file.update( record );
    _change_record( record );
}

//------------------------------------------------------------------------Emulate_key_file::set

void Emulate_key_file::set( const Key& key )
{
    int i = _find_key( key, false );

    _file_key.assign( _record_array[ i ],
    _set_key = true;
}

//------------------------------------------------------------------Emulate_key_file::_find_key

int Emulate_key_file::_find_key( const Const_area& key, Bool exact )
{
    if( key.length() != _key_len )  throw_xc( "emulate_key: invalid keylength", key.length(), _key_len );

    int i;

    for( i = _record_array.first_index(); i <= _record_array.last_index(); i++ ) {
        Byte* p = _record_buffer[ i ];
        int cmp = memcmp( p + sizeof (uint), key.ptr(), _key_len );
        if( cmp == 0 )  return i;
        if( exact  &&  cmp > 0 )  return i;
    }

    return 0;
}

#endif
