#include "precomp.h"
//#define MODULE_NAME "cobfield"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/*
    7.11.99: [SIGN IS] LEADING|TRAILING [SEPARATE CHARACTER] eingebaut.  J. Zschimmer
*/

#include <ctype.h>

#include "sosstrng.h"
#include "sos.h"
#include "log.h"
#include "xception.h"
#include "soslimtx.h"
#include "sosarray.h"
#include "sosfield.h"
#include "ebcdifld.h"
#include "sosopt.h"
#include "sosprof.h"
#include "../file/anyfile.h"

using namespace std;
namespace sos {


extern ostream& operator<< ( ostream& s, const Source_pos& pos );   // Ab nach xception.h!


//---------------------------------------------------------------------------------------------

enum Field_kind { k_none, k_alpha, k_numeric, k_binary, k_packed, k_float/*nicht implementiert*/, k_mixed };

//---------------------------------------------------------------------------------------------

//static Bool german_profile_read = false;
//static Bool german              = false;

//---------------------------------------------------------------------------------------------

struct Cobol_field : Sos_object_base
{
                                Cobol_field             ();
                              //Cobol_field             ( const Cobol_field& o )        { _assign( o ); }
    virtual                    ~Cobol_field             ();

  //Cobol_field&                operator =              ( const Cobol_field& o )        { _assign( o ); }

    Sos_ptr<Field_type>         field_type              ( Bool ignore_vlen, Ebcdic_type_flags, Bool unsigned_packed_f ) const;
    Sos_ptr<Field_descr>        field_descr             () const;

    void                       _obj_print               ( ostream* s ) const;

    Fill_zero                  _zero_;
    Source_pos                 _pos;
    int                        _stufennr;
    uint                       _offset;
    Bool                       _group;
    Field_kind                 _kind;
    uint                       _length;
    uint                       _byte_count;
    int                        _precision;
    int                        _scale;
    Bool                       _with_sign;
    Bool                       _leading_sign;           // SIGN IS LEADING
    Bool                       _separate_sign;          // SEPARATE CHARACTER
    int                        _occurs;
    int                        _occurs_level;
    Bool                       _redefined;
    Bool                       _filler; 
    Sos_limited_text<100>      _name;
    Sos_limited_text<100>      _original_name;           // when _redefined };
    Sos_ptr<Field_type>        _type;
    Sos_limited_text<65>       _remark;
};


//ostream& operator << ( ostream&, const Cobol_field& );

//---------------------------------------------------------------------------------------------

struct Cobol_synthesizer : Sos_self_deleting
{
                                Cobol_synthesizer       ( const Sos_string& type_name );

    void                        stufennr                ( int, const Source_pos& );
    void                        filler                  ()                        { _current_field._filler = true; }
    void                        identifier              ( const String0_area& s ) { _current_field._name = s; }
    void                        field_precision         ( int i )                 { _current_field._precision = i; }
    void                        field_kind              ( Field_kind k )          { _current_field._kind      = k; }
    void                        field_length            ( int len      )          { _current_field._length    = len; }
    void                        field_scale             ( int scale    )          { _current_field._scale     = scale; }
    void                        update_group            ( Cobol_field* );
    void                        field_ok                ( Cobol_field* );
    void                        with_sign               ( Bool b )                { _current_field._with_sign = b; }
    void                        leading_sign            ( Bool b )                { _current_field._leading_sign = b; }
    void                        separate_sign           ( Bool b )                { _current_field._separate_sign = b; }
    void                        occurs                  ( uint i )                { _current_field._occurs    = i; }
    void                        redefines               ( const String0_area& );
    void                        remark                  ( const String0_area& str ) { _current_field._remark = str; }
    void                        point                   ();
    Record_type*                record_type             ()                        { return SOS_CAST( Record_type, +_field_stack[ 0 ]._type ); }
    Bool                        record_type_exists      ()                        { return +_field_stack[ 0 ]._type != NULL; }

  //private:
    Fill_zero                  _zero_;
  //Cobol_field                _last_field;             // Für REDEFINES
    Cobol_field                _current_field;
    Cobol_field                _field_stack [ 50 ];
    int                        _field_sp;
    Bool                       _ignore_vlen;    
    Bool                       _unsigned_packed_f;      // Gepackte Zahlen ohne Vorzeichen verwenden X'F' statt X'C' als Vorzeichen
    Ebcdic_type_flags          _ebcdic_flags;
};

//---------------------------------------------------------------------Cobol_field::Cobol_field

Cobol_field::Cobol_field()
:
    _zero_(this+1)
{
   //record_type = Record_type::create();
   //_record_type->flat_scope( true );
}

//--------------------------------------------------------------------Cobol_field::~Cobol_field

Cobol_field::~Cobol_field()
{
}

//----------------------------------------------------------------------Cobol_field::field_type

Sos_ptr<Field_type> Cobol_field::field_type( Bool ignore_vlen, Ebcdic_type_flags ebcdic_flags, Bool unsigned_packed_f ) const
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_type> );

    if( !_stufennr )  return 0;

    Sos_ptr<Field_type> t;

    switch( _kind )
    {
        case k_alpha: // PIC X oder Gruppe von Alpha-Feldern
        {
            if( ignore_vlen  &&  _offset + _byte_count <= 4 ) {   
                LOG( "Cobol-Feld " << *this << " wird unterdrückt.\n" );
                // Feld unterdrücken
            } else {
                if( ebcdic_flags & ebc_ascii ) {
                    Sos_ptr<Text_type> u = SOS_NEW( Text_type( _byte_count ) );
                    t = +u;
                } else {
                    Sos_ptr<Ebcdic_text_type> u = SOS_NEW( Ebcdic_text_type( _byte_count, ebcdic_flags ) );
                    t = +u;
                }
            }

            break;
        }

        case k_numeric:
        {
            if( _with_sign ) 
            {
                Sos_ptr<Ebcdic_number_type> u = SOS_NEW( Ebcdic_number_type( _byte_count, ebcdic_flags ) );
                u->scale( _scale );
                u->_leading_sign = _leading_sign;
                u->_separate_sign = _separate_sign;
                t = +u;
            } 
            else 
            {
                Sos_ptr<Ebcdic_unsigned_number_type> u = SOS_NEW( Ebcdic_unsigned_number_type( _byte_count, ebcdic_flags ) );
                u->scale( _scale );
                t = +u;
            }
            break;
        }

        case k_packed:
        {
            Sos_ptr<Ebcdic_packed_type> u = SOS_NEW( Ebcdic_packed_type( _byte_count ) );
            if( unsigned_packed_f )  u->positive_sign( _with_sign? 0x0C : 0x0F );
            u->scale( _scale );
            t = +u;
            break;
        }

        case k_binary:
        {
            if( _byte_count == 2  &&  ignore_vlen  &&  _offset == 0 ) {
                // Feld unterdrücken
                LOG( "Cobol-Feld " << *this << " wird unterdrückt.\n" );
            } 
            else 
            {
                if( _scale ) 
                {
                    if( _with_sign ) {
                        Sos_ptr<Scaled_little_endian_int_type> u = SOS_NEW( Scaled_little_endian_int_type( _byte_count * 8, _scale ) );
                        t = +u;
                    } else {
                        Sos_ptr<Scaled_ulittle_endian_int_type> u = SOS_NEW( Scaled_ulittle_endian_int_type( _byte_count * 8, _scale ) );
                        t = +u;
                    }
                }
                else
                {
                    if( _with_sign ) {
                        Sos_ptr<Little_endian_int_type> u = SOS_NEW( Little_endian_int_type( _byte_count * 8 ) );
                        t = +u;
                    } else {
                        Sos_ptr<Ulittle_endian_int_type> u = SOS_NEW( Ulittle_endian_int_type( _byte_count * 8 ) );
                        t = +u;
                    }
                }
            }
            break;
        }

        case k_mixed: break;        // Gruppe von gemischten Typen: Gruppe ist nicht als Ganzes ansprechbar

        case k_none:
        {
            Sos_ptr<Record_type> u = Record_type::create();
            u->_byte_type = true;
            u->name( as_string( _name ) );
            u->flat_scope( true );
            u->_offset_base = _offset;
            if( ignore_vlen &&  u->_offset_base >= 4 )  u->_offset_base -= 4;    //jz 29.7.97
            t = +u;
            break;
        }

        default:
            throw_syntax_error( "SOS-COBOL-12", c_str( _name ), _pos );
    }

    return t;
}

//--------------------------------------------------------------------Cobol_field::field_descr

Sos_ptr<Field_descr> Cobol_field::field_descr() const
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_descr> );

    Sos_ptr<Field_descr> f;

    if( _occurs_level > 0 ) {
        Sos_ptr<Array_field_descr> af = SOS_NEW( Array_field_descr );
        af->_level = _occurs_level;
        if( _occurs ) {
            af->_dim[ _occurs_level - 1 ]._first_index = 1;
            af->_dim[ _occurs_level - 1 ]._elem_count  = _occurs;
        }
        f = +af;
    } else {
        f = Field_descr::create();
    }

    f->_type_ptr = _type;
    f->offset( _offset );
  //f->name( _kind != k_none? c_str( _name ) : ""  );
    f->name( c_str( _name ) );
    f->_remark = c_str( _remark );
    //LOG( "cob field remark=" << _remark << '\n' );

    //LOG( "cobfield.cxx " << *f << "\n" );
    return +f;
}

//----------------------------------------------------------------------ostream& << Cobol_field
//----------------------------------------------------------------------Cobol_field::_obj_print

//ostream& operator << ( ostream& s , const Cobol_field& cob_field )
void Cobol_field::_obj_print( ostream* s ) const
{
    *s << _stufennr << ' ';
    if( _filler    )  *s << "FILLER";  else *s << _name;
    if( _type      )  *s << ' ' << *_type;
    if( _redefined )  *s << " REDEFINES " << _original_name;
    if( _occurs    )  *s << " OCCURS " << _occurs;
    *s << " (" << _remark << ')';
    *s << ' ' << _pos;
}

//---------------------------------------------------------Cobol_synthesizer::Cobol_synthesizer

Cobol_synthesizer::Cobol_synthesizer( const Sos_string& type_name )
:
    _zero_(this+1)
{
    Sos_ptr<Record_type> t = Record_type::create();
    t->name( type_name );
    t->flat_scope( true );
    t->_byte_type = true;
    _current_field._type = +t;
    _field_sp = 0;
}

//---------------------------------------------------------------------------------------------

void Cobol_synthesizer::stufennr( int nr, const Source_pos& pos )
{
    _field_stack[ _field_sp ] = _current_field;     // Nur für REDEFINES-Klause

    Cobol_field new_field;
    new_field._stufennr = nr;
    new_field._pos      = pos;
    new_field._offset   = _current_field._offset + _current_field._byte_count * max( 1, _current_field._occurs );
    //jz 22.4.97new_field._occurs_level = _current_field._occurs_level;  

    if( nr == _current_field._stufennr  ||  nr >= 50 )
    {
        // nix
        new_field._occurs_level = _field_stack[ _field_sp - 1 ]._occurs_level;  //jz 22.4.97
    }
    else
    if( nr > _current_field._stufennr )
    {
        Field_type* t = _field_stack[ _field_sp ]._type;
        if( !t  ||  !t->obj_is_type( tc_Record_type ) )  throw_xc( "SOS-COBOL-19", &_current_field );

        new_field._occurs_level = _current_field._occurs_level;  //jz 22.4.97
        new_field._offset = _current_field._offset;

        // SIGN IS LEADING|TRAILING SEPARATE erben:
        new_field._leading_sign = _current_field._leading_sign;     // SIGN IS LEADING
        new_field._separate_sign = _current_field._separate_sign;   // SIGN IS SEPARATE

        _field_stack[ _field_sp++ ] = _current_field;
    }
    else
    {
        while( _field_sp > 0  &&  _field_stack[ _field_sp - 1 ]._stufennr >= nr ) {
            _field_sp--;
            Cobol_field* cf = &_field_stack[ _field_sp ];
            cf->_byte_count = new_field._offset - _field_stack[ _field_sp ]._offset;

            Record_type* r = SOS_CAST( Record_type, _field_stack[ _field_sp ]._type );
            r->_field_size = cf->_byte_count;
            //filler nicht mitgezählt: assert( r->_field_size == cf->_byte_count );  //jz 16.5.97

            Sos_ptr<Field_type> type = cf->field_type( _ignore_vlen, _ebcdic_flags, _unsigned_packed_f );
            if( type                  // d.h. nicht k_mixed
             && length( cf->_name ) )     // Nicht FILLER?
            {  
                if( _ignore_vlen  
               //|| ( type->obj_is_type( tc_Record_type )  &&  ((Record_type*)+type)->field_count() > 0 )  // ??
                 && cf->_offset < 4 ) 
                {
                    // Feld unterdrücken
                    LOG( "Cobol-Feld " << *this << " wird unterdrückt.\n" );
                }
                else
                {
                    r->_group_type = type;   // Typ, der die ganze Gruppe beschreibt
                }
            }

            update_group( &_field_stack[ _field_sp ] );

            if( cf->_occurs )  new_field._offset = cf->_offset + cf->_byte_count * cf->_occurs;
            new_field._occurs_level = _field_stack[ _field_sp - 1 ]._occurs_level;
        }
    }

    _current_field = new_field;
}

//---------------------------------------------------------------------------------------------

void Cobol_synthesizer::redefines( const String0_area& name )
{
    Cobol_field* last_field = &_field_stack[ _field_sp ];

    if( name != ( last_field->_redefined? last_field->_original_name : last_field->_name ) ) {
         throw_syntax_error( "SOS-COBOL-10", _current_field._pos );
    }

    if( _current_field._stufennr != last_field->_stufennr )  throw_syntax_error( "SOS-COBOL-11", _current_field._pos );

    _current_field._offset        = last_field->_offset;
    _current_field._redefined     = true;
    _current_field._original_name = name;
}

//--------------------------------------------------------------Cobol_synthesizer::update_group

void Cobol_synthesizer::update_group( Cobol_field* cob_field )
{
    // Typ des Gruppenfeldes bestimmen:
    if( _field_sp > 0
     && cob_field->_kind )
    {
        Field_kind* k = &_field_stack[ _field_sp - 1 ]._kind;
        if( *k != k_mixed ) {
            if( cob_field->_kind == k_alpha  
             || cob_field->_kind == k_numeric && cob_field->_separate_sign )  *k = k_alpha;
            else
            if( cob_field->_kind == k_numeric
             && !cob_field->_with_sign
             && !cob_field->_scale )
            {
                if( !*k )  *k = k_numeric;   // numeric ohne Vorzeichen oder Komma
            } else {
                *k = k_mixed;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------

void Cobol_synthesizer::field_ok( Cobol_field* cob_field )
{
    Sos_ptr<Field_descr> f;

    update_group( cob_field );

    switch( cob_field->_kind )
    {
        case k_alpha:   cob_field->_byte_count = cob_field->_length; break;
        case k_numeric: if( cob_field->_separate_sign )  cob_field->_length++;
                        cob_field->_byte_count = cob_field->_length; break;
        case k_binary:  cob_field->_byte_count = //cob_field->_length <=  2? 1 :      // 2008-06-23 für Avendata (nicht Cobol-gemäß)
                                                 cob_field->_length <=  4? 2 
                                               : cob_field->_length <=  9? 4 
                                               : cob_field->_length <= 18? 8        // EBO Eichenauer
                                                                     : 99; 
                        //jz 16.2.00 Wird doch nicht ausgerichtet: if( _ebcdic_flags & ebc_ascii )  cob_field->_offset = ( cob_field->_offset + 1 ) & ~1;
                        break;
        case k_packed:  cob_field->_byte_count = ( cob_field->_length + 1 + 1 ) / 2; break;
        default:        cob_field->_byte_count = cob_field->_length;
    }

    if( cob_field->_occurs )  cob_field->_occurs_level++;

    if( !cob_field->_filler  
     || cob_field->_kind == k_none )   // Record_type doch (aber ohne _group_type)
    {
        cob_field->_type = cob_field->field_type( _ignore_vlen, _ebcdic_flags, _unsigned_packed_f );
        
        if( cob_field->_type ) {
            f = cob_field->field_descr();
     
            if( _ignore_vlen ) // &&  !cob_field->_type->obj_is_type( tc_Record_type ) ) 
            {
                if( f->offset() < 4 ) {
                    if( !cob_field->_type->obj_is_type( tc_Record_type ) ) {
                        if( f->offset() + f->type_ptr()->field_size() > 4 )  {
                            Xc x ( "SOS-COBOL-17" );
                            x.insert( f );
                            x.pos( cob_field->_pos );
                            throw x;
                        }
                        LOG( "Cobol-Feld " << *cob_field << " wird unterdrückt.\n" );
                        f = NULL;   // Feld unterdrücken
                    }
                } else {
                    f->offset( f->offset() - 4 );
                }
            }
    
            if( f )  SOS_CAST( Record_type, _field_stack[ _field_sp - 1 ]._type )->add_field( f );
        }
    }
}

//---------------------------------------------------------------------------------------------

void Cobol_synthesizer::point()
{
    field_ok( &_current_field );
}

//---------------------------------------------------------------------------------------------

struct Cobol_input_stream
{
                                Cobol_input_stream      ( Any_file* f, const Source_pos& pos ) : _file ( f ), _next_pos(pos)  { _pos._col = 0; _c = read_char(); }

    int                         get                     ();
    int                         peek                    ()  { _pos._col++; return _c; }
    void                        back                    ( char c )  { _c = c; --_pos._col; }

    Source_pos                 _pos;

  //private:
    int                         read_char               ();

    Any_file*                  _file;
    Source_pos                 _next_pos;
    Dynamic_area               _line;
    int                        _c;
    Sos_limited_text<65>       _remark;
};

//---------------------------------------------------------------------------------------------

struct Cobol_parser
{
    struct Token
    {
        enum Kind
        {
            k_none = 0,
            k_eof = 1,

            k_binary,
            k_blank,
            k_character,
            k_comma,
            k_comp,
            k_comp_1,
            k_comp_2,
            k_comp_3,
            k_external,
            k_filler,
            k_identifier,
            k_indexed,
            k_by,
            k_is,
            k_justified,
            k_leading,
            k_occurs,
            k_packed_decimal,
            k_picture,
            k_point,
            k_redefines,
            k_separate,
            k_sign,
            k_synchronized,
            k_times,
            k_trailing,
            k_usage,
            k_value,
            k_when,
            k_zero,

            k_number,
            k_string,
            k__last
        };

        static void             init                    ();

                                Token                   ();

        Kind                    kind                    () const  { return _kind; }
        const String0_area&     name                    () const  { return _name; }
        uint4                   number                  () const  { return _number; }

        Source_pos             _pos;

        Kind                   _kind;
        Sos_limited_text<100>  _name;
        uint4                  _number;

        struct Token_entry
        {
            Kind               _kind;
            const char*        _name;
        };


        //static Sos_simple_array<Token_entry> token_table;
        static void             add_token               ( Kind, const char* );
        static const char*      repr                    ( Token::Kind );
    };


                                Cobol_parser            ( Any_file*, const Source_pos&, Cobol_synthesizer* );

    const Token&                next_token              ()                  { return _next_token; }
    void                        check_token             ( Token::Kind );
    void                        parse                   ( Token::Kind );
    void                        parse_token             ();
    uint4                       parse_number            ();
    void                        parse_identifier        ();
    void                        parse_string            ();
    void                        parse_literal           ();
    void                        parse_picture_clause    ();
    void                        parse_field_decl        ();
    void                        parse_data_decl         ();
    void                        parse                   ();

  //private:
    Cobol_input_stream         _input;
    Cobol_synthesizer* const   _synth_ptr;
    Token                      _next_token;
    Bool                       _remark_before_decl;
};

//---------------------------------------------------------------------------------------------

//Sos_simple_array<Cobol_parser::Token::Token_entry> Cobol_parser::Token::_token_array;
//static Cobol_parser::Token::Token_entry Cobol_parser::Token::token_table [ Cobol_parser::Token::k__last ];

//---------------------------------------------------------------------------------------------

int Cobol_input_stream::read_char()
{
    while( _next_pos._col >= (int)_line.length() )
    {
        try {
            _file->get( &_line );
        }
        catch( const Eof_error& )
        {
            return EOF;
        }

        if( _line.length() > 72 )  _line.length( 72 ); // Alles ab Spalte 73 ignorieren

        _next_pos._line++; _next_pos._col = 6;

        if( _line.char_ptr()[ 6 ] == '*'               // Kommentar
         || _line.char_ptr()[ 6 ] == '/' )             // Kommentar mit Seitenvorschub
        {
            if( length( _remark ) == 0 ) {
                _line.resize_min( _line.length() + 1 );
                _line.char_ptr()[ _line.length() ] = '\0';
                const char* p = _line.char_ptr( 7 );
                while( *p == _line.char_ptr()[ 6 ] )  p++;
                while( *p == ' ' )  p++;
                int l = _line.char_ptr( min( 71u, _line.length() ) ) - p;
                if( l > 0 ) _remark.assign( p, length_without_trailing_spaces( p, l ) );
                //LOG( "cob remark=" << _remark << '\n' );
            }
            _next_pos._col = _line.length();
        }
        else
        if( _line.char_ptr()[ 6 ] == '-' ) {           // Verkettung ohne Blank
            _next_pos._col = 7;
        }
        else {                                         // Verkettung mit Blank
            _line.char_ptr()[ 6 ] = ' ';
            _next_pos._col = 6;
        }
    }

    if( _line.char_ptr()[ _next_pos._col ] == ' ' ) {
        while( _next_pos._col < (int)_line.length() && _line.char_ptr()[ _next_pos._col ] == ' ' )  _next_pos._col++;
        return ' ';
    }

    return _line.char_ptr()[ _next_pos._col++ ];
}

//---------------------------------------------------------------------------------------------

int Cobol_input_stream::get()
{
    int c = _c;
    _pos = _next_pos;
    _c = read_char();
    return c;
}

//---------------------------------------------------------------------------------------------

static const Cobol_parser::Token::Token_entry token_table [] =
{
    { Cobol_parser::Token::k_binary        , "BINARY"           },
    { Cobol_parser::Token::k_blank         , "BLANK"            },
    { Cobol_parser::Token::k_by            , "BY"               },
    { Cobol_parser::Token::k_character     , "CHARACTER"        },
    { Cobol_parser::Token::k_comma         , ","                },
    { Cobol_parser::Token::k_comp          , "COMP"             },
    { Cobol_parser::Token::k_comp          , "COMPUTATIONAL"    },
    { Cobol_parser::Token::k_comp_1        , "COMP-1"           },
    { Cobol_parser::Token::k_comp_1        , "COMPUTATIONAL-1"  },
    { Cobol_parser::Token::k_comp_2        , "COMP-2"           },
    { Cobol_parser::Token::k_comp_2        , "COMPUTATIONAL-2"  },
    { Cobol_parser::Token::k_comp_3        , "COMP-3"           },
    { Cobol_parser::Token::k_comp_3        , "COMPUTATIONAL-3"  },
    { Cobol_parser::Token::k_external      , "EXTERNAL"         },
    { Cobol_parser::Token::k_filler        , "FI"               },  // Gesehen in $e.xaok.lib(ceabdb)
    { Cobol_parser::Token::k_filler        , "FILLER"           },
    { Cobol_parser::Token::k_indexed       , "INDEXED"          },
    { Cobol_parser::Token::k_is            , "IS"               },
    { Cobol_parser::Token::k_justified     , "JUSTIFIED"        },
    { Cobol_parser::Token::k_leading       , "LEADING"          },
    { Cobol_parser::Token::k_occurs        , "OCCURS"           },
    { Cobol_parser::Token::k_picture       , "PIC"              },
    { Cobol_parser::Token::k_picture       , "PICTURE"          },
    { Cobol_parser::Token::k_packed_decimal, "PACKED-DECIMAL"   },
    { Cobol_parser::Token::k_point         , "."                },
    { Cobol_parser::Token::k_redefines     , "REDEFINES"        },
    { Cobol_parser::Token::k_separate      , "SEPARATE"         },
    { Cobol_parser::Token::k_sign          , "SIGN"             },
    { Cobol_parser::Token::k_synchronized  , "SYNC"             },
    { Cobol_parser::Token::k_synchronized  , "SYNCHRONIZED"     },
    { Cobol_parser::Token::k_times         , "TIMES"            },
    { Cobol_parser::Token::k_trailing      , "TRAILING"         },
    { Cobol_parser::Token::k_usage         , "USAGE"            },
    { Cobol_parser::Token::k_value         , "VALUE"            },
    { Cobol_parser::Token::k_when          , "WHEN"             },
    { Cobol_parser::Token::k_zero          , "ZERO"             },

    { Cobol_parser::Token::k_identifier    , "<bezeichner>"     },
    { Cobol_parser::Token::k_number        , "<natürliche zahl>"},
    { Cobol_parser::Token::k_string        , "<string>"         }
};

//---------------------------------------------------------------------------------------------

void Cobol_parser::Token::init()
{
    //if( token_table.size() )  return;

    //for( int i = 0; i < NO_OF( token_table ); i++ )  token_table.add( token_table[ i ] );

}

//---------------------------------------------------------------------------------------------

const char* Cobol_parser::Token::repr( Cobol_parser::Token::Kind kind )
{
    //for( int i = token_table.first_index(); i <= token_table.last_index(); i++ ) {
    for( int i = 0; i < NO_OF( token_table ); i++ ) {
        if( token_table[ i ]._kind == kind )  return c_str( token_table[ i ]._name );
    }
    return "<?>";
}

//---------------------------------------------------------------------------------------------

Cobol_parser::Token::Token()
:
    _number ( 0 )
{
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_token()
{
    Cobol_input_stream* s = &_input;

    _next_token = Token();
    _next_token._pos = s->_pos;

    while( s->peek() == ' ' )  s->get();
    
    int c = s->peek();

    if( isdigit( c ) )
    {
        while( isdigit( s->peek() ) )
        {
            _next_token._number *= 10; _next_token._number += s->get() - '0';
        }
        _next_token._kind = Token::k_number;
    }
    else
    if( isalpha( c ) )
    {
        while( isalnum( s->peek() )  ||  s->peek() == '-' )
        {
            _next_token._name += (char)s->get();
        }

        _next_token._kind = Token::k_identifier;

        //for( int i = Token::token_table.first_index(); i <= Token::token_table.last_index(); i++ )
        for( int i = 0; i < NO_OF( token_table ); i++ ) 
        {
            if( stricmp( c_str( token_table[ i ]._name ), c_str( _next_token._name ) ) == 0 ) {
                _next_token._kind = token_table[ i ]._kind;
                break;
            }
        }

        if( _next_token._kind == Token::k_picture )
        {
            _next_token._name += ' ';
            while( s->peek() == ' ' )  s->get();

            if( s->peek() == 'I' ) {
                s->get();
                if( s->peek() != 'S' )  throw_syntax_error( "SOS-COBOL-13", s->_pos );
                s->get();
                while( s->peek() == ' ' )  s->get();
            }

            while( s->peek() != EOF  &&  strchr( "ABPSVXYZ90/,.+-CRDB*$(12345678)", s->peek() ) )
            {
                if( s->peek() == '.' ) {
                    s->get();
                    if( s->peek() == ' '  ||  s->peek() == EOF )  { s->back( '.' ); break; }
                    _next_token._name += '.';
                } else {
                    _next_token._name += s->get();
                }
            }
        }
    }
    else
    if( c == '.' ) {
        s->get();
        _next_token._kind = Token::k_point;
    }
    else
    if( c == ',' ) {
        s->get();
        _next_token._kind = Token::k_comma;
    }
    else
    if( c == EOF ) {
        _next_token._kind = Token::k_eof;
    }
    else
    if( c == '"' ) {
        s->get();
        while(1) {
            int c = s->peek();
            if( c == EOF )  throw_syntax_error( "SOS-COBOL-13", s->_pos );
            if( c == '"' ) {
                s->get();
                if( s->peek() != '"' )  break;
            }
            _next_token._name += c;
            s->get();
        }
    }
    else {
        throw_syntax_error( "SOS-COBOL-1", s->_pos );
    }
}

//---------------------------------------------------------------------------------------------

Cobol_parser::Cobol_parser( Any_file* f, const Source_pos& pos, Cobol_synthesizer* s )
:
    _input     ( f, pos ),
    _synth_ptr ( s ),
    _remark_before_decl ( false )
{
    Token::init();
    parse_token();
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::check_token( Token::Kind kind )
{
    if( next_token().kind() != kind ) throw_syntax_error( "SOS-COBOL-15", Token::repr( kind ), next_token()._pos );
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse( Token::Kind kind )
{
    check_token( kind );
    parse_token();
}

//---------------------------------------------------------------------------------------------

uint4 Cobol_parser::parse_number()
{
    check_token( Token::k_number );

    uint4 i = next_token().number();
    parse_token();
    return i;
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_identifier()
{
    check_token( Token::k_identifier );
    _synth_ptr->identifier( next_token().name() );
    parse_token();
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_string()
{
    check_token( Token::k_string );
    parse_token();
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_literal()
{
    if( next_token().kind() == Token::k_number ) {
        parse_number();
    } else
    if( next_token().kind() == Token::k_string ) {
        parse_string();
    }
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_picture_clause()
{
    const char* p0    = strchr( c_str( next_token().name() ), ' ' ) + 1;
    const char* p     = p0;

    Bool  alpha     = false;
    int   length    = 0;
    int   komma_pos = -1;   // Stelle des Kommas von links gezählt (-1: kein Komma)

    while( *p )
    {
        switch( *p++ )
        {
            case '0':   // 0
            case '9':   // 0-9
            {
                length++;
                break;
            }

            case 'A':   // A-Z
            case 'B':   // Blank oder zweites Zeichen von DB
            case 'X':
            case 'Z':   // Führende 0 wird Blank
            case '/':   // /
            case ',':   // , Stellung des Dezimalpunktes bei DECIMAL-POINT IS COMMA
            case '.':   // . Stellung des Dezimalpunktes
            case '+':
            case '-':
            case '*':
            case '$':
            case 'C':   // CR
            case 'R':   // CR
            case 'D':   // DB
            {
                alpha = true;
                length++;
            }

          //case 'P':
            case 'S':   // Mit Vorzeichen
            {
                _synth_ptr->with_sign( true );
                break;
            }

            case 'V':   // Stellung des Dezimalpunkts
            {
                komma_pos = length;
                break;
            }

            case '(':   // Letztes Zeichen wiederholen
            {
                if( p-1 == p0 )  {
                    Source_pos pos = _next_token._pos;
                    pos._col += p - p0;
                    char str[2]; str[0]=*(p-1); str[1]='\0';
                    throw_syntax_error( "SOS-COBOL-3", str, pos );
                }

                int n = 0;
                while( isdigit( *p ) ) {
                    n *= 10;  n += *p++ - '0';
                }
                length += n - 1;
                if( *p != ')' )  {
                    Source_pos pos;
                    pos._col += p - p0;
                    throw_syntax_error( "SOS-COBOL-2", pos );
                }
                p++;
                break;
            }

            default: {
                Source_pos pos = _next_token._pos;
                pos._col += p - p0;
                char str[2]; str[0]=*(p-1); str[1]='\0';
                throw_syntax_error( "SOS-COBOL-3", str, pos );
            }
        }
    }

    _synth_ptr->field_kind( alpha? k_alpha : k_numeric  ); 
    _synth_ptr->field_length( length );
    if( komma_pos != -1 )  _synth_ptr->field_scale( length - komma_pos );
    parse_token();
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_field_decl()
{
    Bool wrong = false;

    _synth_ptr->stufennr( parse_number(), _next_token._pos );

    if( next_token().kind() == Token::k_identifier ) {
        parse_identifier();
    }
    else {
        if( next_token().kind() == Token::k_filler )  parse( Token::k_filler );
        _synth_ptr->filler();
    }

    while( !wrong )
    {
        switch( next_token().kind() )
        {
            case Token::k_redefines:
            {
                parse_token();
                check_token( Token::k_identifier );
                _synth_ptr->redefines( next_token().name() );
                parse_token();
                break;
            }

            case Token::k_picture:
            {
                parse_picture_clause();
                break;
            }

            case Token::k_usage:
            {
                parse_token();
                if( next_token().kind() == Token::k_is )  parse_token();
                break;
            }

            case Token::k_binary:
            case Token::k_comp:
            {
                parse_token();
                _synth_ptr->field_kind( k_binary );
                break;
            }

            case Token::k_packed_decimal:
            case Token::k_comp_3:
            {
                parse_token();
                _synth_ptr->field_kind( k_packed );
                break;
            }

            case Token::k_sign:
            {
                parse_token();
                check_token( Token::k_is );
                parse_token();
                if( next_token().kind() != Token::k_leading )  check_token( Token::k_trailing );
                _synth_ptr->leading_sign( false );   // Nicht von der Gruppe erben
                _synth_ptr->separate_sign( false );  // Nicht von der Gruppe erben
                break;
            }

            case Token::k_leading:
            case Token::k_trailing:
            {
                _synth_ptr->leading_sign( next_token().kind() == Token::k_leading );
                parse_token();

                _synth_ptr->separate_sign( false );
                if( next_token().kind() == Token::k_separate ) {
                    _synth_ptr->separate_sign( true );
                    parse_token();
                    if( next_token().kind() == Token::k_character )  parse_token();
                }
                break;
            }

            case Token::k_value:
            {
                parse_token();
                if( next_token().kind() == Token::k_is )  parse_token();
                parse_token();  // irgendwas
                break;
            }

            case Token::k_blank:        throw_syntax_error( "SOS-COBOL-18", "BLANK", _next_token._pos );
            case Token::k_justified:    throw_syntax_error( "SOS-COBOL-18", "JUSTIFIED", _next_token._pos );
            case Token::k_synchronized: throw_syntax_error( "SOS-COBOL-18", "SYNCHRONIZED", _next_token._pos );

            case Token::k_occurs:
            {
                parse_token();
                _synth_ptr->occurs( parse_number() );
                if( next_token().kind() == Token::k_times )  parse_token();

                if( next_token().kind() == Token::k_indexed ) {   // INDEXED BY ident  ignorieren
                    parse_token();
                    parse( Token::k_by );
                    parse( Token::k_identifier );
                }
                break;
            }

          //case Token::k_external:
            default: wrong = true;
        }
    }

    if( _remark_before_decl )  _synth_ptr->remark( _input._remark );
    parse( Token::k_point );
    if( !_remark_before_decl )  _synth_ptr->remark( _input._remark );
    _input._remark.length( 0 );
    _synth_ptr->point();
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse_data_decl()
{
    while( next_token().kind() == Token::k_number )
    {
        parse_field_decl();
    }

    _synth_ptr->stufennr( 0, _next_token._pos );
}

//---------------------------------------------------------------------------------------------

void Cobol_parser::parse()
{
    while( next_token().kind() == Token::k_number )
    {
        parse_data_decl();
    }

    if( next_token().kind() != Token::k_eof ) throw_syntax_error( "SOS-COBOL-16", _next_token._pos );
}

//--------------------------------------------------------------------------read_profile_german
/*
static void read_profile_german()
{
    german_profile_read = true;
    Sos_string code;
    code = read_profile_string( "", "sosfield", "code" );
    german = code == "german" || code == "ebcdic-german";
}
*/
//---------------------------------------------------------------------------------------------

Sos_ptr<Record_type> cobol_record_type( const Sos_string& param )
{
    Sos_string          filename;
    Any_file            file;
    Bool                ignore_vlen = false;   // Die ersten 4 Bytes ignorieren (Satzlängenfeld im BS2000)    
    Ebcdic_type_flags   flags       = ebc_none;
    Sos_ptr<Cobol_synthesizer> synth;   // sehr groß
    Bool                remark_first = false;
    Bool                unsigned_packed_f = false;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() ) 
    {
        if( opt.flag( "remark-first" ) )   remark_first = opt.set();
        else
        if( opt.flag( "vlen" ) )   ignore_vlen = opt.set();
        else
        // s.a. cobfield.cxx!
        if( opt.flag( "mvs"    ) )  flags = Ebcdic_type_flags( opt.set()? flags & ~ebc_bs2000 | ebc_mvs : flags & ~ebc_mvs   );
        else
        if( opt.flag( "bs2000" ) )  flags = Ebcdic_type_flags( opt.set()? flags & ~ebc_mvs | ebc_bs2000 : flags & ~ebc_bs2000 );
        else
        if( opt.flag( "german" ) )  flags = Ebcdic_type_flags( opt.set()? flags & ~ebc_international | ebc_german : flags & ~ebc_german | ebc_international );
        else
        if( opt.flag( "unsigned-packed-f" ) )  unsigned_packed_f = opt.set();
        else
        if( opt.flag( "ascii" ) )   flags = Ebcdic_type_flags( opt.set()? flags | ebc_ascii : flags & ~ebc_ascii );
        else
        filename = opt.rest();
    }

    synth = SOS_NEW( Cobol_synthesizer ( filename ) );

    synth->_ignore_vlen = ignore_vlen;
    synth->_ebcdic_flags = flags;
    synth->_unsigned_packed_f = unsigned_packed_f;


    file.open( c_str(filename), File_base::Open_mode( File_base::in | File_base::seq ) );
    Cobol_parser        parser ( &file, Source_pos( c_str( filename ), 0, 0 ), synth );

    parser._remark_before_decl = remark_first;

    parser.parse();

    if( !synth->record_type_exists() )  throw_xc( "SOS-1248", c_str( filename ) );

    Sos_ptr<Record_type> t = synth->record_type();

    if( t->field_count() == 1 )   // Auf der kleinsten Stufennummer nur ein Feld?
    {
        Field_type* u = t->field_descr_ptr( 0 )->type_ptr();
        if( u->obj_is_type( tc_Record_type ) ) {              // Und das Feld ist ein Record?
            t = (Record_type*)u;                                 // Dann ist das unser Record
        }
    }

    if( ignore_vlen ) {
        Sos_ptr<Record_type> t2 = Record_type::create();
        t2->allocate_fields( t->field_count() );
        t2->name( t->name() );
        t2->flat_scope( true );
        t2->_byte_type = t->_byte_type;

        for( int i = 0; i < t->field_count(); i++ ) {
            Field_descr* f = t->field_descr_ptr( i );
            if( f->type_ptr()->obj_is_type( tc_Record_type )        // Gruppe
             && ((Record_type*)f->type_ptr())->field_count() == 0 )   // ohne Felder (alle unterdrückt)
            {
                // Feld unterdrücken
                LOG( "Leeres Cobol-Gruppenfeld " << *f << " wird unterdrückt.\n" );
            } 
            else 
            {
              //if( f->offset() < 4 )  throw_xc( "SOS-COBOL-17", f );
              //f->offset( f->offset() - 4 );
                t2->add_field( f );
            }
        }

        //jz 16.5.97  t2->_field_size = t->_field_size - 4;
        //filler nicht mitgezählt: assert( t2->_field_size == t->_field_size - 4 );   //jz 16.5.97
        t = t2;
    }

    t->set_array_distances();
/*
    if( t->name() == "" ) {
        t->name( filename );
    }
*/
    return t;
}

} //namespace sos
