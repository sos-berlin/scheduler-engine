//#define MODULE_NAME "vflfile"                       // Frame variable field length
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

//#define CHECK_STACK_OVERFLOW
//#include <optimize.h>
#include "precomp.h"

#include <stdio.h>          // sprintf
#include "../kram/sysdep.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"
//#include <strstrea.h>

#include "../kram/log.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/tabbed.h"

namespace sos {


const int max_frame_record_length = 8192;

//-------------------------------------------------------------------------------Frame_vfl_file

struct Frame_vfl_file : Abs_file
{
                                Frame_vfl_file          ();
                               ~Frame_vfl_file          ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );
    void                        insert                  ( const Const_area& record )  { put_record( record ); }
    void                        del                     ();
    void                        update                  ( const Const_area& );


    void                        set                     ( const Key& key )      { _file.set( key ); }
    void                        del                     ( const Key& key )      { _file.del( key ); }

  protected:

    void                        get_record              ( Area& area );
    void                        get_record_key          ( Area& area, const Key& key );
    void                        put_record              ( const Const_area& );

    void                        set_key                 ( const Key& key );

  //Record_position             key_position            ( Key::Number )         { return 0; }
  //Record_length               key_length              ( Key::Number )         { return _file.key_length(); }
    void                        get_position            ( Area* buffer )        { _file.get_position( buffer ); }

    void                        frame_to_record         ( Area* );

  private:
    void                        make_frame_record       ( const Const_area& );

    Fill_zero                  _zero_;
    Any_file                   _file;
    int                        _key_len;
    Sos_ptr<Record_type>       _record_type;
    Dynamic_area               _frame_record;
    Dynamic_area               _record;
    Bool                       _update_allowed;
    Bool                       _frame;
    //Field_descr*               _select_field;
    //Sos_string                 _select_field_value;
};

//----------------------------------------------------------------------Frame_vfl_file_type

struct Frame_vfl_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "framevfl"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Frame_vfl_file> f = SOS_NEW( Frame_vfl_file );
        return +f;
    }
};

const Frame_vfl_file_type      _frame_vfl_file_type;
const Abs_file_type&            frame_vfl_file_type = _frame_vfl_file_type;

// --------------------------------------------------------------Frame_vfl_file::Frame_vfl_file

Frame_vfl_file::Frame_vfl_file()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------Frame_vfl_file::~Frame_vfl_file

Frame_vfl_file::~Frame_vfl_file()
{
}

//-------------------------------------------------------------------------Frame_vfl_file::open

void Frame_vfl_file::open( const char* fn, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename = fn;
    Sos_string select_field_name;

    for( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( opt.flag( "frame" ) )             _frame = opt.set();
        //else
        //if( opt.with_value( "select" ) ) {      // feldname='wert'
        //    Sos_option_iterator o2 = '-' + opt.value();
        //    select_field_name = o2.option();
        //    _select_field_value = o2.value();
        //}
        else
        if( opt.with_value( "key-length" )
         || opt.with_value( "kl"         ) )  _key_len = as_int( opt.value() );
        else
        if( opt.pipe()                     )  { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    _record_type = +file_spec._field_type_ptr;
    if( !_record_type ) throw_xc( "SOS-1193" );

    _file.obj_owner( this );
    _file.open( filename, Any_file::Open_mode( open_mode | Any_file::binary ), file_spec );

    if( !_key_len ) {
        _key_len = _file.key_length();
        _key_pos = 0;

        if( _file.key_position() != 0 )  throw_xc( "SOS-1265", this );
        if( !_key_len )     throw_xc( "SOS-1265", this );
    }

    // Prüfen, ob die ersten Felder genau in den Schlüssel passen:
    int l = 0;
    int i = 0;
    while(1) {
        if( i >= _record_type->field_count() )  throw_xc( "SOS-1266", this );
        l += _record_type->field_descr_ptr( i++ )->type().field_size();
        if( l >= _key_len )  break;
    }
    if( l > _key_len )  throw_xc( "SOS-1268" );

    //if( !empty( select_field_name ) ) {
    //    _select_field = _record_type->field_descr_ptr( select_field_name );
    //}
}

//------------------------------------------------------------------------Frame_vfl_file::close

void Frame_vfl_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//--------------------------------------------------------------Frame_vfl_file::frame_to_record

void Frame_vfl_file::frame_to_record( Area* buffer )
{
    if( _frame_record.length() < _key_len )  throw_xc( "Frame_vfl_file:tooshort", _frame_record.length() );

    buffer->allocate_length( _record_type->field_size() );

    Byte* p     = _frame_record.byte_ptr();
    Byte* p_end = _frame_record.byte_ptr() + _frame_record.length();
    int   i     = 0;

    if( _frame ) {
        // Im Frame-Modus wird der Schlüssel überlesen.
        p += _key_len;
    } 
    else 
    {
        // Schlüsselfelder sind fix gespeichert
        while(1) {
            if( i >= _record_type->field_count() )  throw_xc( "SOS-1266", this );
            Field_descr* f = _record_type->field_descr_ptr( i++ );
            int s = f->type().field_size();
            if( p + s > p_end )  throw_xc( "SOS-1267", this );
            memcpy( f->ptr( buffer->byte_ptr() ), p, s );
            p += s;
            if( p >= _frame_record.byte_ptr() + _key_len )  break;
        }
    }

    // Die weiteren Felder haben ein Längenbyte:
    while( p < p_end ) {
        if( i >= _record_type->field_count() )  throw_xc( "SOS-1266", this );
        int l = *p++;   // Länge
        if( p + l > p_end )  throw_xc( "SOS-1267", this );
        Field_descr* f = _record_type->field_descr_ptr( i++ );
        int s = f->type().field_size();
        if( l > s )  throw_xc( "SOS-1269", f->name(), l );
        memcpy( f->ptr( buffer->byte_ptr() ), p, l );
        p += l;
        memset( f->ptr( buffer->byte_ptr() ) + l, 0x40, s - l );   // EBCDIC-Blanks
    }

    while( i < _record_type->field_count() ) {
        _record_type->field_descr_ptr( i++ )->set_null( buffer->byte_ptr() );
    }

    _update_allowed = true;
}

//-------------------------------------------------------------------Frame_vfl_file::get_record

void Frame_vfl_file::get_record( Area& buffer )
{
    _update_allowed = false;
    _file.get( &_frame_record );
    frame_to_record( &buffer );
    _update_allowed = true;
}

//---------------------------------------------------------------Frame_vfl_file::get_record_key

void Frame_vfl_file::get_record_key( Area& buffer, const Key& key )
{
    _update_allowed = false;
    _file.get_key( &_frame_record, key );
    frame_to_record( &buffer );
    _update_allowed = true;
}

//----------------------------------------------------------------------Frame_vfl_file::set_key

void Frame_vfl_file::set_key( const Key& key )
{
    _update_allowed = false;
    _file.set( key );
}

//-------------------------------------------------------Frame_vfl_file::make_tabbed_record

void Frame_vfl_file::make_frame_record( const Const_area& )
{
    throw_xc( "vflfile-nowrite" );
/*
    if( !_record_type )  throw_xc( "SOS-1193", "vfl.insert" );

    _frame_record.allocate_min( max_frame_record_length );

    Byte* p     = _frame_record.char_ptr();
    Byte* p_end = p + _frame_record.size();

    int i = 0;
    while( i < _record_type->field_count()  &&  p < _frame_record.byte_ptr() + _key_len ) {
        Field_descr* f = _record_type->field_descr_ptr( i );
        memcpy( p, record.byte_ptr(), f->type().field_size() );
        p += area.size();
        i++;
    }

    while( i < _record_type->field_count() ) {
        Field_descr* f = _record_type->field_descr_ptr( i );
        if( f ) {
            p++;
            Area area ( p, p_end - p );  area.length( 0 );
            memcpy( p, record.byte_ptr() .........., &area, _text_format );
            if( area.length() > 255 )  throw_xc( "SOS-1269", f->name(), area.length() );
            p[-1] = (Byte) area.length();
            p += area.length();
        }
        i++;
    }

    _frame_record.length( p - _frame_record.char_ptr() );
*/
}

//--------------------------------------------------------------Frame_vfl_file::put_record

void Frame_vfl_file::put_record( const Const_area& record )
{
    _update_allowed = false;

    make_frame_record( record );
    _file.put( _frame_record );
}

//-------------------------------------------------------------------Frame_vfl_file::update

void Frame_vfl_file::update( const Const_area& record )
{
    if( !_update_allowed )  throw_xc( "SOS-1233" );
    _update_allowed = false;

    make_frame_record( record );

    _file.update( _frame_record );
}

//----------------------------------------------------------------------Frame_vfl_file::del

void Frame_vfl_file::del()
{
    _file.del();
}


} //namespace sos
