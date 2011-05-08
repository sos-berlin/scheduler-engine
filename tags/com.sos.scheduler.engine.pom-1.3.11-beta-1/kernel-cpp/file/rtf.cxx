//#define MODULE_NAME "rtf"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"


#define CHECK_STACK_OVERFLOW
#include <optimize.h>

#include <precomp.h>      // MSVC++ 4.2: fatal error C1001: INTERNAL COMPILER ERROR

#include <sysdep.h>

#include <sosstrng.h>
#include <sos.h>
#include <absfile.h>
#include <sosfield.h>
#include <log.h>
#include <sosopt.h>
#include <sosprof.h>

#define LOGBLOCK(e) //Log_block qq(e)

const int max_tabbed_record_length = 4096;

//---------------------------------------------------------------------------Rtf_file

struct Rtf_file : Abs_file
{
                                Rtf_file();
                               ~Rtf_file();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

  protected:
    void                        put_record              ( const Const_area& );

  private:
    void                        put_field_names         ();

    Fill_zero                  _zero_;
    Any_file                   _file;
    Sos_ptr<Record_type>       _record_type;
    Bool                       _with_field_names;
    Text_format                _text_format;
};

//----------------------------------------------------------------------Rtf_file_type

struct Rtf_file_type : Abs_file_type
{
    virtual const char*         name                    () const  { return "rtf"; }
  //virtual const char*         alias_name              () const  { return "rtf"; };

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Rtf_file> f = SOS_NEW( Rtf_file() );
        return +f;
    }
};

const Rtf_file_type    _rtf_file_type;
const Abs_file_type&    rtf_file_type = _rtf_file_type;

// ------------------------------------------------------Rtf_file::Rtf_file

Rtf_file::Rtf_file()
:
    _zero_(this+1),
#   if defined SYSTEM_WIN16
        _max_field_size ( 256 )   // ca. 250 Felder bis 64KB erreicht sind
#   else
        _max_field_size ( 1024 )
#   endif
{
    _with_field_names  = false;  // mit Feldname im ersten Satz
}

//------------------------------------------------------Rtf_file::~Rtf_file

Rtf_file::~Rtf_file()
{
}

//---------------------------------------------------------------Record_tabbed::put_field_names

void Rtf_file::put_field_names()
{
    _tabbed_record.allocate_min( max_tabbed_record_length );

    ostrstream s ( _tabbed_record.char_ptr(), _tabbed_record.size() );

    for( int i = 0; i < _record_type->field_count(); i++ ) {
        if( i > 0 )  s << _separator;
        s << _record_type->field_descr_ptr( i )->name();
    }

    _tabbed_record.length( s.pcount() );
    _file.put( _tabbed_record );
}

//---------------------------------------------------------------Record_tabbed::get_field_names

void Rtf_file::get_field_names()
{
    // Eof_error wird vom Aufrufer abgefangen.

    Dynamic_area field_names;

    if( length( _field_names ) )
    {
        Sos_string  field_names = _field_names;

        if( _field_names[ 0 ] == '('  &&  _field_names[ (int)length( _field_names ) - 1 ] == ')' ) {
            // Klammern entfernen
            field_names = as_string( c_str( _field_names ) + 1, length( _field_names ) - 2 );
        }

        _record_type = record_type_of_name_list( c_str( field_names ), ',' );
    }
    else
    {
        if( _with_field_names ) {
            _file.get( &field_names );      // Feldnamen stehen im ersten Satz
        }
        else
        {
            // Felder des ersten Satzes zählen und Feldnamen f1,f2,...,fn erzeugen:
            _file.get( &_tabbed_record );
            _first_record_read = true;
            const char* p     = _tabbed_record.char_ptr();
            const char* p_end = p  + _tabbed_record.length();
            field_names.allocate_min ( _max_field_size );
            char name [ 10 ];
            int  i = 1;
            while(1) {
                name[ 0 ] = _separator;
                sprintf( name+1, "f%i", i );
                field_names.append( i == 1? name + 1 : name );
                i++;
                p = (char*)memchr( p, _separator, p_end - p );
                if( !p )  break;
                p++;
            }
        }

        field_names += '\0';
        _record_type = record_type_of_name_list( field_names.char_ptr(), _separator );
    }

    _base_record_size = _record_type->field_size();
  //_record.allocate_min( _base_record_size + max_tabbed_record_length );
    _any_file_ptr->_spec._field_type_ptr = +_record_type;
}

//---------------------------------------------------------------------Rtf_file::open

void Rtf_file::open( const char* fn, Open_mode open_mode, const File_spec& file_spec )
{
    Bool       write_field_names;
    Bool       eof      = false;
    Sos_string filename = fn;

    _separator = '\t';
    _text_format = std_text_format;

    if( !( open_mode & out ) )  throw_xc( "D127" );

    for( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( opt.flag( "field-names" ) )        _with_field_names = opt.set();       // 1. Zeile enthält Feldnamen
        else
        if( opt.flag( "decimal-comma" ) )      _text_format.decimal_symbol( opt.set()? ',' : '.' );
        else
	    if( opt.with_value( "date-format" ) )  _text_format.date( c_str( opt.value() ) );
	    else
	  //if( opt.with_value( "max-field-size" ) )  _max_field_size = opt.as_int();
	  //else
        if( opt.flag( "raw" ) )                _text_format.raw( opt.set() );
        else
        if( opt.pipe()                    )   { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    write_field_names = _with_field_names && ( open_mode & out );
    _record_type  = +file_spec._field_type_ptr;

    _file.obj_owner( this );
    _file.open( filename, open_mode, file_spec );

    if( write_field_names ) {
        put_field_names();
    }
}

//--------------------------------------------------------------------Rtf_file::close

void Rtf_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//-------------------------------------------------------Rtf_file::make_tabbed_record

void Rtf_file::make_tabbed_record( const Const_area& record )
{
    if( !_callers_type )  throw_xc( "SOS-1193", "record/tabbed.insert" );

    _tabbed_record.allocate_min( max_tabbed_record_length );

    char* p     = _tabbed_record.char_ptr();
    char* p_end = p + _tabbed_record.size();

    for( int i = 0; i < _record_type->field_count(); i++ ) 
    {
        if( i > 0 )  *p++ = _separator;

        Field_descr* f = _record_type->field_descr_ptr( i );
        if( f ) {
            Area area ( p, p_end - p );  area.length( 0 );

            if( _quote ) {
                ostrstream s ( area.char_ptr(), area.size() );
                f->print( record.byte_ptr(), &s, _text_format, _quote, _quote );
                area.length( s.pcount() );
            } else {
                f->write_text( record.byte_ptr(), &area, _text_format );
            }

            if( memchr( area.ptr(), _separator, area.length() ) )  throw_xc( "SOS-1231", f );
            p += area.length();
        }
    }

    _tabbed_record.length( p - _tabbed_record.char_ptr() );
}

//--------------------------------------------------------------Rtf_file::put_record

void Rtf_file::put_record( const Const_area& record )
{
    _update_allowed = false;

    if ( _transparent_write ) {
        _file.put( record );
        return;
    }

    make_tabbed_record( record );
    _file.put( _tabbed_record );
}

//-------------------------------------------------------------Rtf_file::rewind

void Rtf_file::rewind( Key::Number )
{
    _file.rewind();

    if( _with_field_names )
    {
        Dynamic_area buffer;
        _file.get( &buffer );      // Feldnamen stehen im ersten Satz
    }
}

//-------------------------------------------------------------------Rtf_file::update

void Rtf_file::update( const Const_area& record )
{
    if( !_update_allowed )  throw_xc( "SOS-1233" );

    _update_allowed = false;

    int orig_len = _tabbed_record.length();

    make_tabbed_record( record );

    if( _tabbed_record.length() <= orig_len ) {
        memset( _tabbed_record.char_ptr() + orig_len, ' ', orig_len - _tabbed_record.length() );
        _file.update( _tabbed_record );
    } else {
        del();
        insert( _tabbed_record );
    }
}

//----------------------------------------------------------------------Rtf_file::del

void Rtf_file::del()
{
    if( !_update_allowed )  throw_xc( "SOS-1233" );

    _update_allowed = false;

    int orig_len = _tabbed_record.length();

    Dynamic_area blanks ( orig_len );
    blanks.length( orig_len );
    memset( blanks.ptr(), ' ', orig_len );

    _file.update( blanks );
}

