// $Id: rectab.cxx 13579 2008-06-09 08:09:33Z jz $
// #rev 3#

//#define MODULE_NAME "rectab"
//#define AUTHOR      "Jörg Schwiemann"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

// Tabbed2Sosfield-Zugriffe

// Datum: 01.08.1995
// Stand: 01.08.1995

//#define CHECK_STACK_OVERFLOW
//#include "../kram/optimize.h"

#include "precomp.h"

#undef JZ_TEST
#if defined __WIN32__   &&  defined __BORLANDC__
#   pragma option -Od
#endif


#include <stdio.h>          // sprintf
#include <ctype.h>
#include "../kram/sosctype.h"
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
#include "../file/flstream.h"

#define LOGBLOCK(e) //Log_block qq(e)

namespace sos {
using namespace std;



//----------------------------------------------------------------------------------------const

const int max_tabbed_record_length = 32767;     // Für mehrzeilige Sätze zusammen
const int max_field_size           = 1024;

//---------------------------------------------------------------------------Record_tabbed_file

struct Record_tabbed_file : Abs_file
{
                                Record_tabbed_file();
                               ~Record_tabbed_file();

    void                        open                ( const char*, Open_mode, const File_spec& );
    void                        close               ( Close_mode );
    void                        insert              ( const Const_area& record )  { put_record( record ); }
    void                        del                 ();
    void                        update              ( const Const_area& );
/*

    void                            set( const Key& key );
    void                            del( const Key& key );
    void                            del();

    Record_position                 key_position (Key::Number = 0) { return _key_position; };
    Record_length                   key_length   (Key::Number = 0) { return _key_length; };

    static const Abs_file_type&     static_file_type();
*/

  protected:

    void                        get_record              ( Area& area );
  //void                        get_record_key          ( Area& area, const Key& key );
    void                        put_record              ( const Const_area& );
    void                        rewind                  ( Key::Number );

  private:
    bool                        is_valid_separator      ( char c );
    void                        detect_separator        ();
    void                        put_field_names         ();
    void                        get_field_names         ();
    Sos_ptr<Record_type>        record_type_of_name_list( Byte separator );
    void                        make_tabbed_record      ( const Const_area& );
    int                         get_value               ( Byte** result );
    int                         get_char                ( bool get_line_allowed = false );
    void                        get_line                ();

    Fill_zero                  _zero_;
    Bool                       _first_record_read;
    Any_file                   _file;
    Sos_ptr<Record_type>       _callers_type;
    Sos_ptr<Record_type>       _record_type;
    Dynamic_area               _tabbed_record;
    Byte*                      _char_ptr;
    Byte*                      _char_end_ptr;
    Bool                       _with_field_names;
    Bool                       _transparent_write;          // ?
    int                        _base_record_size;
    int                        _record_number;
    int                        _line_nr;
    Byte                       _separator;
    bool                       _separator_determined;
    Byte                       _quote;
    Text_format                _text_format;
    Bool                       _update_allowed;
    Bool                       _rtrim;
    int                        _max_field_size;
    int                        _field_size;
    Sos_string                 _field_names;
    Bool                       _only_tabbed_field_type; // _tabbed_set enthält nur true
    Dynamic_area               _tabbed_set;             // !_callers_type && byte_ptr(i) == true? Tabbed_field_type : Text_type 
    Any_file_stream            _error_log;
    Bool                       _error_logging;
    long                       _count;
    bool                       _eof;
};

//----------------------------------------------------------------------Record_tabbed_file_type

struct Record_tabbed_file_type : Abs_file_type
{
    virtual const char*         name                    () const  { return "record/tabbed"; }
    virtual const char*         alias_name              () const  { return "tab"; };

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Record_tabbed_file> f = SOS_NEW_PTR( Record_tabbed_file() );
        return +f;
    }
};

const Record_tabbed_file_type  _record_tabbed_file_type;
const Abs_file_type&            record_tabbed_file_type = _record_tabbed_file_type;

//---------------------------------------------------------------------------------Tabbed_field

struct Tabbed_field
{
    const char*                 char_ptr                () const { return (const char*)this + _offset; }
    int                        _offset;
    int                        _length;
};

// Dyn_obj setzt diesen Typ in Text_type(max_field_size) um, weil dessen Werte nur lesbar sind
// (_field_copy_possible = false). Nicht effizient.

//----------------------------------------------------------------------------Tabbed_field_type

struct Tabbed_field_type : Field_type
{
    BASE_CLASS( Field_type )

                                Tabbed_field_type       ( int field_size  )         : Field_type( &_type_info, sizeof (Tabbed_field) ), _max_field_size(field_size) {}

    void                        construct               ( Byte* ) const                  { throw_xc( "SOS-1322", "construct" ); }
    void                        field_copy              ( Byte*, const Byte* ) const     { throw_xc( "SOS-1322", "field_copy" ); }
    int                         alignment               () const                         { return sizeof (int); }

    void                        write_text              ( const Byte* p, Area*, const Text_format& ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& ) const;
    bool                        null                    ( const Byte* ) const;

    void                       _get_param               ( Type_param* ) const;

    Bool                       _obj_is_type             ( Sos_type_code t ) const               { return t == tc_Tabbed_field_type || Base_class::_obj_is_type( t ); }

    int                        _max_field_size;

    static Type_info           _type_info;
};

//extern Tabbed_field_type tabbed_field_type;

//----------------------------------------------------------------Tabbed_field_type::write_text

void Tabbed_field_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    buffer->assign( p + ((Tabbed_field*)p)->_offset, ((Tabbed_field*)p)->_length );
}

//-----------------------------------------------------------------Tabbed_field_type::read_text

void Tabbed_field_type::read_text( Byte*, const char*, const Text_format& ) const
{
    throw_xc( "Tabbed_field", "Schreiben nicht möglich" );
}

//----------------------------------------------------------------------Tabbed_field_type::null

bool Tabbed_field_type::null( const Byte* p ) const
{
    return ((Tabbed_field*)p)->_length == 0;
}

//----------------------------------------------------------------Tabbed_field_type::_type_info

Type_info Tabbed_field_type::_type_info;

SOS_INIT( tabbed_field )
{
    Tabbed_field_type::_type_info._std_type      = std_type_char,      // _std_type
    Tabbed_field_type::_type_info._name          = "Tabbed_field",     // _name
    Tabbed_field_type::_type_info._max_size      = max_field_size;     // _max_size;
    Tabbed_field_type::_type_info._max_precision = max_field_size;     // _max_precision;
    Tabbed_field_type::_type_info._field_copy_possible = false;        // field_copy() scheitert!
    Tabbed_field_type::_type_info.normalize();
};

//----------------------------------------------------------------Tabbed_field_type::_get_param

void Tabbed_field_type::_get_param( Type_param* param ) const
{
	param->_precision = _max_field_size;
}

// ------------------------------------------------------Record_tabbed_file::Record_tabbed_file

Record_tabbed_file::Record_tabbed_file()
:
    _zero_(this+1),
    _max_field_size ( max_field_size )
{
    _with_field_names   = false;  // mit Feldname im ersten Satz
    _transparent_write = false;  // Puts werden durchgereicht (z.B. SELECT-Statements!)
}

//------------------------------------------------------Record_tabbed_file::~Record_tabbed_file

Record_tabbed_file::~Record_tabbed_file()
{
}

//-------------------------------------------------Record_tabbed_file::record_type_of_name_list

Sos_ptr<Record_type> Record_tabbed_file::record_type_of_name_list( Byte separator )
{
    Sos_ptr<Record_type> t = SOS_NEW( Record_type );
    int                  i = 0;
    Sos_string           name;
    Sos_string           interpretation;
    Sos_ptr<Field_type>  field_type;

    if( !_quote )  _only_tabbed_field_type = true;

    while(1) 
    {
        int size = 0;

        if( i >= _tabbed_set.size() ) { _tabbed_set.resize_min( i + 200 ); }

        const char* p = NULL;
        /*int len =*/ get_value( (Byte**)&p );
        if( !p )  break;
        read_field_name_and_interpretation( &p, &name, &interpretation, separator );

        if( length( interpretation ) ) 
        {
            const char* q = c_str( interpretation ) + 1;   // ':' überspringen
            while( sos_isspace( *q ) )  q++;
            //if( !_rtrim )  throw_xc( "SOS-1317", c_str( name ) );
            if( sos_isdigit( *q ) ) {
                while( sos_isdigit( *q ) ) size = size*10 + *q++ - '0';
                //if( size == 0 )  throw_xc( "SOS-1318", c_str( name ) );
                while( sos_isspace( *q ) )  q++;
                Sos_string str = interpretation;  // Anker für nächste Zeile
                interpretation = q;
            }
        }

        if( size == 0 )  size = _field_size;    // Text_type statt Tabbed_field_type benutzen, 22.3.2002

        if( size ) {
            Sos_ptr<Text_type> text_type = SOS_NEW( Text_type( size ) );
            field_type = +text_type;
            _only_tabbed_field_type = false;
            _tabbed_set += char( false );
        } else {
            Sos_ptr<Tabbed_field_type> tabbed_field_type = SOS_NEW( Tabbed_field_type( _max_field_size ) );
            field_type = +tabbed_field_type;
            _tabbed_set += char( true );
        }

        if( name.empty()  &&  _char_ptr == _char_end_ptr )  break;

        Sos_ptr<Field_descr> field_descr = SOS_NEW( Field_descr( field_type, c_str( name ) ) );
        if( length( interpretation ) )  modify_field( field_descr, interpretation );
        field_descr->add_to( t );
      //field_descr->add_null_flag_to( t );

        i++;
    }

    return +t;
}

//---------------------------------------------------------------Record_tabbed::put_field_names

void Record_tabbed_file::put_field_names()
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

void Record_tabbed_file::get_field_names()
{
    // Eof_error wird vom Aufrufer abgefangen.

    if( !length( _field_names ) )
    {
        Dynamic_area saved_first_record;
        bool         restore_first_record = false;

        if( _with_field_names ) {
            get_line(); // Feldnamen stehen im ersten Satz
        }
        else
        {
            string field_names;

            // Felder des ersten Satzes zählen und Feldnamen f1,f2,...,fn erzeugen:
            _tabbed_record.length(0);
            get_line();
            
            saved_first_record = _tabbed_record;  restore_first_record = true;
            
            _first_record_read = true;
            
            int  i = 1;

            while( *_char_ptr != '\n' ) 
            {
                Byte* p;
                get_value( &p );
                if( !p )  break;
                if( i > 1 )  field_names += (char)_separator;
                field_names += 'f';
                field_names.append( as_string( i ) );
                i++;
            }

            _tabbed_record = field_names + "\n";
        }

        _char_ptr = _tabbed_record.byte_ptr();  _char_end_ptr = _char_ptr + _tabbed_record.length();
        _record_type = record_type_of_name_list( _separator );

        _base_record_size = _record_type->field_size();
      //_record.allocate_min( _base_record_size + max_tabbed_record_length );
        if( !_callers_type )  _any_file_ptr->_spec._field_type_ptr = +_record_type;

        if( restore_first_record )  
        {
            _tabbed_record = saved_first_record;
            _char_ptr = _tabbed_record.byte_ptr();  
            _char_end_ptr = _char_ptr + _tabbed_record.length();
        }
    }
}

//-------------------------------------------------------Record_tabbed_file::is_valid_separator

bool Record_tabbed_file::is_valid_separator( char c )
{
    return c == ';'  
       ||  c == ','  
       ||  c == '\t';
}

//---------------------------------------------------------Record_tabbed_file::detect_separator

void Record_tabbed_file::detect_separator()
{
    const char* p     = _tabbed_record.char_ptr();
    const char* p_end = p + _tabbed_record.length();
    int         counter[256];  memset( counter, 0, sizeof counter );

    // Quotes vorhanden? Dann gilt das Zeichen nach einem Quote:   "xxx";"yyy"

    while( p < p_end )
    {
        if( _quote && *p == _quote )
        {
            p++;  if( p >= p_end )  return;

            while(1)
            {
                if( *p == _quote )
                {
                    p++;  if( p >= p_end )  return;
                    if( *p != _quote )  break;
                }
                p++;
            }

            while( p < p_end  &&  *p == ' ' )  p++;
            if( p >= p_end )  return;

            if( is_valid_separator(*p) )  { _separator = *p; _separator_determined = true; return; }
        }
        else
            counter[ (Byte)*p++ ]++;
    }

    // Keine Quotes? Dann gilt das häufigste Zeichen

    int sep = _separator;

    for( int i = 0; i < 256; i++ )  if( is_valid_separator(i)  &&  counter[i] > counter[sep] )  sep = i;

    _separator = sep;
    _separator_determined = true;
}

//---------------------------------------------------------------------Record_tabbed_file::open

void Record_tabbed_file::open( const char* fn, Open_mode open_mode, const File_spec& file_spec )
{
    Bool       write_field_names;
    bool       csv      = false;
    Sos_string filename = fn;

    _text_format = std_text_format;

    for( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( opt.flag( "field-names" ) )        _with_field_names = opt.set();       // 1. Zeile enthält Feldnamen
        else
        if( opt.with_value( "field-names" ) )  _field_names = opt.value();
        else
        if( opt.flag( "rtrim" ) )              _rtrim = opt.set();              // Blanks rechts abschneiden
        else
        if( opt.flag( "write-transparently" ) )_transparent_write = opt.set();  // Beim Schreiben nicht konvertieren (für SQL)
        else
        if( opt.flag( "transparent-write" ) )  _transparent_write = opt.set();  // Beim Schreiben nicht konvertieren (für SQL)
        else
        if( opt.flag( "decimal-comma" )  )      _text_format.decimal_symbol( opt.set()? ',' : '.' );
        else
        if( opt.with_value( "decimal-symbol" ) ) _text_format.decimal_symbol( opt.as_char() );
        else
        if( opt.with_value( "quote" ) )        _quote = opt.as_char();
        else
	    if( opt.with_value( "date-format" ) )  _text_format.date( c_str( opt.value() ) );
	    else
	    if( opt.with_value( "date-time-format" ) )  _text_format.date_time( c_str( opt.value() ) );
	    else
	    if( opt.with_value( "max-field-size" ) )  _max_field_size = opt.as_int();
	    else
	    if( opt.with_value( "field-size" ) )   _field_size = opt.as_int();      // Text_type statt Tabbed_field_type nutzen, dann läuft's auch mit sossql. 22.3.2002
	    else
        if( opt.with_value( "tab" )  )         _separator = opt.as_char(),  _separator_determined = true;
        else
        if( opt.flag( "raw" ) )                _text_format.raw( opt.set() );
        else
        if( opt.flag( "csv" ) )                csv = opt.set();
        else
        if( opt.with_value( "error-log" ) )    { _error_log.open( opt.value(), Any_file::out ); _error_logging = true; }
        else
        if( opt.pipe()                    )    { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    if( !_separator )       _separator = csv? ',' : '\t';
    if( !_quote &&  csv )  _quote = '"';

    write_field_names = _with_field_names && ( open_mode & out );
    _callers_type = +file_spec._field_type_ptr;
    _record_type  = _callers_type;

    _file.obj_owner( this );
    _file.open( filename, Open_mode( open_mode & ~binary ), file_spec );
    //_open_mode = open_mode;

    _tabbed_record.allocate( max_tabbed_record_length );

    if( length( _field_names ) )
    {
        if( _record_type )  throw_xc( "SOS-1211", c_str( _record_type->name() ) );

        Sos_string  field_names = _field_names;

        if( _field_names[ 0 ] == '('  &&  _field_names[ (int)length( _field_names ) - 1 ] == ')' ) {
            // Klammern entfernen
            field_names = as_string( c_str( _field_names ) + 1, length( _field_names ) - 2 );
        }

        _tabbed_record = field_names;  _char_ptr = _tabbed_record.byte_ptr();  _char_end_ptr = _char_ptr + _tabbed_record.length();
        _record_type = record_type_of_name_list( ',' );
        _base_record_size = _record_type->field_size();
        if( !_callers_type )  _any_file_ptr->_spec._field_type_ptr = +_record_type;
    }

    if( open_mode & in ) 
    {
        if( _callers_type )
        {
            if( _with_field_names )  
            {
                Dynamic_area line;
                _file.get( &line );
            }
        }
        else
        {
            try {
                get_field_names();
                write_field_names = false;          // Datei enthält eine Zeile
            }
            catch( const Eof_error& ) { _eof = true; }
        }
    }

    if( ( (open_mode & out) && !_transparent_write ) ||
        ( (open_mode & in)  && !_eof               )    ) 
    {
        if( !_record_type ) throw_xc( "SOS-1193" );
    }

    if( write_field_names ) {
        put_field_names();
    }
}

//--------------------------------------------------------------------Record_tabbed_file::close

void Record_tabbed_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//------------------------------------------------------------------------------------get_line

void Record_tabbed_file::get_line()
{
    _char_ptr = _tabbed_record.byte_ptr() + _tabbed_record.length();

    Area rest = _tabbed_record.rest( -1 );
    
    _file.get( &rest );

    rest.append( '\n' );
    _tabbed_record.length( _tabbed_record.length() + rest.length() );
    _char_end_ptr = _tabbed_record.byte_ptr() + _tabbed_record.length();

    if( !_separator_determined )  detect_separator(), _separator_determined = true;

    _line_nr++;
}

//-------------------------------------------------------------------------------------get_char

inline int Record_tabbed_file::get_char( bool get_line_allowed )
{
    if( _char_ptr == _char_end_ptr )  
    {
        if( !get_line_allowed )  return EOF;
        if( _eof )  return EOF;

        try
        {
            get_line();
        }
        catch( const Eof_error& ) { _eof = true; return EOF; }
    }
    
    return (Byte)*_char_ptr++;
}

//------------------------------------------------------------------------------------get_value
// Liefert Wert bis zum _separator in result.
// Liefert Länge als Funktionsergebnis.
// Liefert neuen Pointer in pp zurück.

int Record_tabbed_file::get_value( Byte** result )
{
    Byte* p = _char_ptr;
    int   c = get_char();
    int   l;
    Byte* null_byte;

    if( _quote  &&  c == _quote ) 
    {
        Byte  quote = c;
        Byte* q = _char_ptr;    // Ergebnis
        *result = q;

        while(1)
        {
            c = get_char(true);
            if( c == EOF )  break;
            if( c == quote ) 
            {
                c = get_char();
                if( c != quote )  break;
            }

            *q++ = c;
        }

        while( c == ' ' )  c = get_char();
        if( c != '\n'  &&  c != EOF  &&  c != _separator )  throw_xc( "SOS-1431", _line_nr );

        l = q - *result;
        null_byte = q;

        p = q;
        while( *p == ' ' )  p++;
    }
    else
    {
        // Alter, schneller Algorithmus, nur für einzeilige Datensätze:

        if( c == EOF )  { *result = NULL; return 0; }

        --_char_ptr;

        if( _separator == ' ' ) {
            p = _char_ptr;
            while( p < _char_end_ptr  &&  !sos_isspace( p[0] ) )  p++;
        } else {
            p = (Byte*)memchr( p, _separator, _char_end_ptr - p );
            if( !p )  p = _char_end_ptr;
        }

        if( _char_ptr < _char_end_ptr  &&  p == _char_end_ptr  &&  _char_end_ptr[-1] == '\n' )  _char_end_ptr--,  p--;

        l = p - _char_ptr;
    
        if( _rtrim  &&  l  &&  _char_ptr[ l-1 ] == ' ' )  l = length_without_trailing_spaces( (char*)_char_ptr, l );

        *result = _char_ptr;
        null_byte = _char_ptr + l;

        if( _separator == ' ' ) {
            while( p < _char_end_ptr  &&  sos_isspace( p[0] ) )  p++;
            _char_ptr = p;
        } else {
            if( p == _char_end_ptr )  _char_ptr = _char_end_ptr;
            else  
            if( *p == _separator )  _char_ptr = p+1;
            else
                _char_ptr = p;
        }
    }

    *null_byte = '\0';
    return l;
}

//---------------------------------------------------------------Record_tabbed_file::get_record

void Record_tabbed_file::get_record( Area& area )
{
    while(1) {
        if( _first_record_read )  _first_record_read = false;
        else {
            _tabbed_record.length(0);
            get_line();
        }
        _record_number++;

        // Nur Zeilen, die ein Zeichen verschieden von Blank (also auch tab) enthalten,
        // werden akzeptiert:
        for( int i = 0; i < _tabbed_record.length(); i++ ) {
            if( _tabbed_record.char_ptr()[ i ] != ' ' )  goto LINE_OK;
        }
    }

  LINE_OK:
    //_tabbed_record.resize_min( _tabbed_record.length() + 1 );   // Ein '\0' wird angehängt. jz 2.2.98

    int   i = 0;    // Feldnummer

    if( _only_tabbed_field_type )
    {
        // Nur Tabbed_field_type und Text_type

        int result_length = _base_record_size + _tabbed_record.length();
        area.allocate_min( result_length );
        area.length( result_length );

        Tabbed_field* q     = (Tabbed_field*)area.ptr();     // Ziel
        Tabbed_field* q_end = (Tabbed_field*)( area.byte_ptr() + _base_record_size );
        
        _char_ptr     = (Byte*)q_end;
        _char_end_ptr = area.byte_ptr() + area.length();

        memcpy( _char_ptr, _tabbed_record.ptr(), _tabbed_record.length() );

        while( q < q_end ) 
        {
            Byte* value_ptr;
            q->_length = get_value( &value_ptr );
            if( !value_ptr )  break;
            q->_offset = (Byte*)value_ptr - (Byte*)q;
            q++;
        }

        while( q < q_end )  q->_length = 0, q++;      // Weniger Felder als deklariert?
    }
    else
    {
        area.allocate_length( _record_type->field_size() );

        _char_ptr     = _tabbed_record.byte_ptr();
        _char_end_ptr = _char_ptr + _tabbed_record.length();

        if( !_callers_type )
        {
            // Tabbed_field_type mit Text_type gemischt

            while(1)
            {
                Byte* value_ptr;

                if( i >= _record_type->field_count() )  break;
                int len = get_value( &value_ptr );
                if( !value_ptr )  break;
                Field_descr* f = _record_type->field_descr_ptr( i );

                if( _tabbed_set.byte_ptr()[ i ] ) 
                {
                    Tabbed_field* q = (Tabbed_field*)f->ptr( area.byte_ptr() );
                    q->_length = len;
                    q->_offset = area.length() - f->offset();
                    if( len )  area.append( value_ptr, len );
                } 
                else  // Text_type 
                {   
                    int size = f->type().field_size();
                    if( len > size )  { LOG( "_tabbed_record=" << _tabbed_record << '\n' ); throw_xc( "SOS-1320", f, (char*)value_ptr ); }
                    Byte* dest = f->ptr( area.byte_ptr() );
                    memcpy( dest, value_ptr, len );
                    memset( dest + len, ' ', size - len );
                }
    
                i++;
            }

            while( i < _record_type->field_count() )    // Restliche Felder löschen
            {
                Field_descr* f = _record_type->field_descr_ptr( i );
                if( _only_tabbed_field_type  ||  _tabbed_set.byte_ptr()[ i ] ) 
                {
                    Tabbed_field* q = (Tabbed_field*)f->ptr( area.byte_ptr() );
                    q->_length = 0;
                    q->_offset = 0;
                } 
                else 
                {
                    if( f->nullable() )  f->set_null( area.byte_ptr() );
                                   else  f->read_text( area.byte_ptr(), "" );
                }
                i++;
            }
        }
        else
        {
            // record_type des Aufrufers
            while( _char_ptr < _char_end_ptr ) 
            {
                Byte* value_ptr;
        
                if( i >= _record_type->field_count() )  break;
                /*int len =*/ get_value( &value_ptr );
                if( !value_ptr )  break;
                Field_descr* f = _record_type->field_descr_ptr( i++ );
                f->read_text( area.byte_ptr(), (char*)value_ptr );
            }

            // Restliche Felder löschen:
            while( i < _record_type->field_count() ) 
            {
                Field_descr* f = _record_type->field_descr_ptr( i );
                if( f->nullable() )  f->set_null( area.byte_ptr() );
                               else  f->read_text( area.byte_ptr(), "" );
                i++;
            }
        }
    } 

    _update_allowed = true;
}

//-------------------------------------------------------Record_tabbed_file::make_tabbed_record

void Record_tabbed_file::make_tabbed_record( const Const_area& record )
{
    _tabbed_record.allocate_min( max_tabbed_record_length );

    char* p     = _tabbed_record.char_ptr();
    char* p_end = p + _tabbed_record.size();

    for( int i = 0; i < _record_type->field_count(); i++ ) 
    {
        if( i > 0 )  *p++ = _separator;

        Field_descr* f = _record_type->field_descr_ptr( i );
        if( f ) 
        {
            try
            {
                Area area ( p, p_end - p );  area.length( 0 );

                Field_type* t = f->type_ptr();
                if( !t->is_numeric()  &&  t->obj_is_type( tc_Field_subtype ) )  
                    t = ((Field_subtype*)t)->base_type();   // jz 13.11.01: Date("dd.mm.yyyy") soll dd.mm.yyyy, nicht yyyy-mm-dd liefern

                if( _quote ) {
                    ostrstream s ( area.char_ptr(), area.size() );
                    t->print( f->const_ptr(record.byte_ptr()), &s, _text_format, _quote, _quote );
                    area.length( s.pcount() );
                } else {
                    t->write_text( f->const_ptr(record.byte_ptr()), &area, _text_format );

                    // Sichergehen, dass kein Separator im Wert enthalten ist:
                    if( _separator == ' ' ) {
                        for( const char* q = area.char_ptr(); q < area.char_ptr() + area.length(); q++ ) {
                            if( sos_isspace( *q ) )  throw_xc( "SOS-1231", f );
                        }
                    } else {
                        if( memchr( area.ptr(), _separator, area.length() ) )  throw_xc( "SOS-1231", f );
                    } 
                }

                p += area.length();
            }
            catch( Xc& x )
            {
                x.insert( f );
                throw;
            }
        }
    }

    _tabbed_record.length( p - _tabbed_record.char_ptr() );
}

//--------------------------------------------------------------Record_tabbed_file::put_record

void Record_tabbed_file::put_record( const Const_area& record )
{
    _update_allowed = false;
    _count++;

    if ( _transparent_write ) {
        _file.put( record );
        return;
    }

    if( _error_logging ) {
        try {
            make_tabbed_record( record );
        }
        catch( const Xc& x )
        {
            _error_log << "tab " << _count << ". Satz: " << x << '\n';
            return;
        }
    } else {
        make_tabbed_record( record );
    }

    _file.put( _tabbed_record );
}

//-------------------------------------------------------------Record_tabbed_file::rewind

void Record_tabbed_file::rewind( Key::Number )
{
    _file.rewind();

    if( _with_field_names )
    {
        Dynamic_area buffer;
        _file.get( &buffer );      // Feldnamen stehen im ersten Satz
    }

    _line_nr = 0;
}

//-------------------------------------------------------------------Record_tabbed_file::update

void Record_tabbed_file::update( const Const_area& record )
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

//----------------------------------------------------------------------Record_tabbed_file::del

void Record_tabbed_file::del()
{
    if( !_update_allowed )  throw_xc( "SOS-1233" );

    _update_allowed = false;

    int orig_len = _tabbed_record.length();

    Dynamic_area blanks ( orig_len );
    blanks.length( orig_len );
    memset( blanks.ptr(), ' ', orig_len );

    _file.update( blanks );
}

} //namespace sos
