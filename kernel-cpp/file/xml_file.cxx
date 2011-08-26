// xml_file.cxx, Joacim Zschimmer
// $Id: xml_file.cxx 11394 2005-04-03 08:30:29Z jz $

#include "precomp.h"


#include <stdio.h>          // sprintf
#include <ctype.h>
#include "../kram/sosctype.h"
#include "../kram/sysdep.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"

#include "../kram/log.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../kram/sosdate.h"
#include "../file/flstream.h"

#include "../kram/sosxml.h"

namespace sos {

//-------------------------------------------------------------------------------------Xml_file

struct Xml_file : Abs_file, 
                  Xml_processor
{
                                Xml_file                ();
                               ~Xml_file                ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

  protected:
    void                        get_record              ( Area& area );
    void                        put_record              ( const Const_area& );

  private:
    void                        put_xml_line            ( const Const_area& );
    void                        get_xml_line            ( Area* );

    Fill_zero                  _zero_;

    Open_mode                  _open_mode;

    Any_file                   _file;

    // XML schreiben:
    Sos_ptr<Record_type>       _record_type;
    Dynamic_area               _record_buffer;

    // XML lesen:
};


//----------------------------------------------------------------------Xml_file_type

struct Xml_file_type : Abs_file_type
{
    virtual const char*         name                    () const  { return "xml"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Xml_file> f = SOS_NEW( Xml_file() );
        return +f;
    }
};

const Xml_file_type            _xml_file_type;
const Abs_file_type&            xml_file_type = _xml_file_type;

//-----------------------------------------------------------Xml_file::Entity::Entity

Xml_file::Entity::Entity( const Sos_string& name, const Sos_string& value ) 
: 
    _name(name), 
    _value(value) 
{
}

//----------------------------------------------------------Xml_file::Entity::~Entity

Xml_file::Entity::~Entity() 
{
}

// ---------------------------------------------------------------Xml_file::Xml_file

Xml_file::Xml_file()
:
    _zero_(this+1)
{
}

//----------------------------------------------------------------Xml_file::~Xml_file

Xml_file::~Xml_file()
{
}

//---------------------------------------------------------------------Xml_file::open

void Xml_file::open( const char* fn, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename = fn;

    Sos_string doctype;
    Sos_string style_type;
    Sos_string style_href;
    Sos_string encoding   = "iso-8859-1";
    
  //_date_tag      = "date";
    _enclosing_tag = "hostxml";
    _suppress_null = true;

    if( ( ( open_mode & in ) != 0 ) == ( ( open_mode & out ) != 0 ) )  throw_xc( "SOS-1236" );  // Entweder -in oder -out!


    for( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "encoding" ) )      encoding = opt.value();
        else
        if( opt.with_value( "doctype" ) )       doctype = opt.value();
        else
        if( opt.with_value( "style-type" ) )    style_type = opt.value();
        else
        if( opt.with_value( "style-href" ) )    style_href = opt.value();
        else
        if( opt.with_value( "tag" ) )           _enclosing_tag = opt.value();
        else
        if( opt.with_value( "record-tag" ) )    _record_tag = opt.value();
        else
	    if( opt.with_value( "date-tag" ) )      _date_tag = opt.value();
	    else
        if( opt.flag( "upper-tags" )  )         { _ucase = opt.set(); _lcase = !_ucase; }
        else                                                                                                         
        if( opt.flag( "lower-tags" )  )         { _lcase = opt.set(); _ucase = !_lcase; }
        else
        if( opt.flag( "suppress-null" ) )       _suppress_null = opt.set();
        else
        if( opt.flag( "suppress-empty" ) )      _suppress_empty = opt.set();
        else
	    if( opt.with_value( "indent" ) )        _indent_string = as_string( "        ", min( 8, opt.as_int() ) );
	    else
        if( opt.flag( "indent" ) )              _indent_string = "\t";
        else
        if( opt.flag( "ignore-unknown-fields" )
         || opt.flag( "i"                     ) )  _ignore_unknown_fields = opt.set();
        else
        if( opt.pipe()                    )     { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    get_encoding( encoding );

    _record_type = +file_spec._field_type_ptr;
    if( !_record_type ) throw_xc( "SOS-1193" );

    _file.obj_owner( this );
    _file.open( filename, Open_mode( open_mode & ~binary ), file_spec );

    _open_mode = open_mode;

    _next_token._pos.filename( c_str( filename ) );

    if( _open_mode & in ) 
    {
        _char_pos.filename( c_str( filename ) );
        init_read();
        parse_header();
    }
    else
    if( _open_mode & out ) 
    {
        init_write();

        _buffer = "<?xml version=\"1.0\"";
        
        if( !empty( encoding ) ) 
        {
            _buffer += " encoding=\"";
            _buffer += encoding;
            _buffer += '\"';
        }
        _buffer += "?>";
        _file.put( _buffer );


        if( !empty( style_type )  ||  !empty( style_href ) ) 
        {
            _buffer = "<?xml:stylesheet";

            if( !empty( style_type ) )
            {
                _buffer += " type=";
                _buffer += quoted_string( c_str( style_type ), '"', '"' );
            }
            
            if( !empty( style_href ) )
            {
                _buffer += " href=";
                _buffer += quoted_string( c_str( style_href ), '"', '"' );
            }
            
            _buffer += "?>";

            _file.put( _buffer );
        }

        
        if( !empty( doctype ) ) 
        {
            _buffer = "<!DOCTYPE ";
            _buffer += doctype;
            _buffer += '>';
            _file.put( _buffer );
        }


        // Start-Tag
        _buffer = "<";
        _buffer += _enclosing_tag;
        _buffer += '>';
        _file.put( _buffer );


        if( !empty( _date_tag ) )
        {
            _buffer = "<";
            _buffer += _date_tag;
            _buffer += '>';

            _buffer += as_string( Sos_optional_date_time::now(), std_date_time_format_iso );

            _buffer += "</";
            _buffer += _date_tag;
            _buffer += '>';

            _file.put( _buffer );
        }
        else
        {
            _buffer = "<!--Generated ";
            _buffer += as_string( Sos_optional_date_time::now(), std_date_time_format_iso );
            _buffer += "-->";

            _file.put( _buffer );
        }

        _buffer.length( 0 );
    }
}

//--------------------------------------------------------------------Xml_file::close

void Xml_file::close( Close_mode close_mode )
{
    if( _open_mode & out ) 
    {
        // Ende-Tag
        _buffer = "</";
        _buffer += _enclosing_tag;
        _buffer += '>';
        _file.put( _buffer );
    }

    _file.close( close_mode );
}

//---------------------------------------------------------------Xml_file::get_record

void Xml_file::get_record( Area& area )
{
    if( _next_token._symbol == sym_end_tag )
    {
        parse_end_tag( _enclosing_tag );

        if( _next_token._symbol != sym_eof )  throw_syntax_error( "SOS-1407", _next_token._pos );
        throw_eof_error();
    }
             
    if( _next_token._symbol == sym_eof )  throw_eof_error();

    area.allocate_min( _record_type->field_size() );
    area.length( _record_type->field_size() );
    area.fill( '\0' );


    // Falls _record_tag leer ist, wird er auf den ersten gelesenen Record-Tag gesetzt.
    // D.h. alle folgenden Records müssen den selben Tag wie der erste haben.
    _record_tag = eat_token_as_string( sym_standard_tag, c_str( _record_tag ) );

    if( _next_token._symbol == sym_slash_gt )   // Leer?
    {
        eat_token();
    }
    else
    {
        eat_token( sym_gt );
        parse_record_fields( _record_type, area.byte_ptr() );
        parse_end_tag( _record_tag );
    }
}

//--------------------------------------------------------------Xml_file::put_xml_line

void Xml_file::put_xml_line( const Const_area& record )
{
    _file.put( record );
    reset_buffer();
}

//--------------------------------------------------------------Xml_file::put_record

void Xml_file::put_record( const Const_area& record )
{
    put_start_tag( c_str( _record_tag ) );

    if( record.length() < _record_type->field_size() ) 
    {
        _record_buffer.allocate_min( _record_type->field_size() );
        _record_buffer.length( _record_type->field_size() );
        _record_buffer.fill( '\x00' );
        _record_buffer = record;
        write_record_fields( _record_type, _record_buffer.byte_ptr(), _record_type->field_size() );
    }
    else
    {
        write_record_fields( _record_type, record.byte_ptr(), record.length() );
    }

    put_end_tag( c_str( _record_tag ) );
}

//--------------------------------------------------------------Xml_file::get_xml_line

void Xml_file::get_xml_line( Area* record )
{
    _file.get( record );
}


} //namespace sos
