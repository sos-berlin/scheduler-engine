// ole_parser.cxx
// $Id$

#include "precomp.h"

#if !defined STRICT
#   define STRICT
#endif

#if defined __BORLANDC__
#   include <windows.h>
#   include <ole2.h>
#   include <variant.h>
#   include <cstring.h>
#endif


#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sysxcept.h"
#include "../kram/sosprof.h"
#include "../kram/sosfield.h"
#include "../kram/sosdate.h"
#include "../file/anyfile.h"
#include "../file/flstream.h"
#include "../kram/stdfield.h"
#include "../kram/sosctype.h"
#include "../kram/tabucase.h"
#include "../kram/licence.h"
#include "../kram/sleep.h"

#include <ole2ver.h>

#include "../kram/oleserv.h"
#include "../kram/olestd.h"
#include "hostole.h"
#define INITGUIDS
#include "hostole2.h"

namespace sos {

#if defined SYSTEM_WIN
    extern HINSTANCE _hinstance;
#endif

struct Hostware_parser;
struct Hostware_token;

const int max_sgml_tag_length = 32;

//-------------------------------------------------------------------------------Hostware_token

struct Hostware_token : Itoken, 
                        Sos_ole_object
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware_token" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Hostware_token          ( IUnknown* = NULL, Ole_class_descr* = token_class_ptr );
                               ~Hostware_token          ();

    USE_SOS_OLE_OBJECT

    /* Ihostware_field_type methods */
    STDMETHODIMP                get_Filename            ( BSTR* o )                             { *o = SysAllocString_string( _pos.filename() ); return NOERROR; }
    STDMETHODIMP                get_Line_no             ( int* o )                              { *o = _pos._line + 1; return NOERROR; }
    STDMETHODIMP                get_Column_no           ( int* o )                              { *o = _pos._col + 1;  return NOERROR; }
    STDMETHODIMP                get_Id                  ( int* o )                              { *o = _id;            return NOERROR; }
  //STDMETHODIMP                get_Repr                ( BSTR* o );
    STDMETHODIMP                get_Value               ( VARIANT* o );

    Fill_zero                  _zero_;
    Hostware_parser*           _parser;
    Source_pos                 _pos;
    int                        _id;
    VARIANT                    _value;
};

//--------------------------------------------------------------------------Hostware_parser_word

struct Hostware_parser_word
{
                                Hostware_parser_word    ( const char* repr = "" )               : _repr(repr)  { VariantInit( &_extra ); }
                               ~Hostware_parser_word    ()                                      { VariantClear( &_extra ); }

    Sos_string                 _repr;
    VARIANT                    _extra;
    Sos_string                 _extra_string;           // Nur wenn _repr == "&...;"
};

//-------------------------------------------------------------------------------Hostware_parser

struct Hostware_parser : Iparser, 
                         Sos_ole_object
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware_parser" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Hostware_parser         ( IUnknown* pUnkOuter = NULL, Ole_class_descr* cls = parser_class_ptr );
                             //~Hostware_parser         ();

    USE_SOS_OLE_OBJECT

    SOS_OLE_MEMBER_BOOL       ( Ignore_case       )
  //SOS_OLE_MEMBER_STRING     ( source_filename   )
  //STDMETHODIMP    ident_chars             ( BSTR o )                              { set_chars( &_ident_chars, o ); }
  //STDMETHODIMP    special_chars           ( BSTR o )                              { set_chars( &_special_chars, o ); }

    STDMETHODIMP    Open_source             ( BSTR filename );

    STDMETHODIMP    Add_word                ( int ident, BSTR representation, VARIANT* extra );
    STDMETHODIMP    Word_repr               ( int ident, BSTR* representation );
    STDMETHODIMP    Word_extra              ( int ident, VARIANT* extra );
	STDMETHODIMP    Get_next_token          ( Itoken** token );

  //void                        set_chars               ( Bool*, BSTR );
  //void                        set_chars               ( Bool*, const char* );

    int                         get_char                ();
    void                        read_token              ();

    Source_pos                 _pos;
    Any_file_stream            _input;
    int                        _next_char;
    Hostware_token             _token;
    Bool                       _sgml;

    Sos_simple_array<Hostware_parser_word> _word_array;

    int                        _eof_id;
    int                        _word_id;
    int                        _string_id;
    int                        _number_id;
    int                        _text_id;                    // Text zwischen SGML-Tags
    int                        _double_quote_id;
    int                        _single_quote_id;
    int                        _back_quote_id;

    Dynamic_area               _hilfspuffer;
  //Bool                       _ident_chars   [ 256 ];
  //Bool                       _special_chars [ 256 ];
};

//--------------------------------------------------------------------------------DESCRIBE_CLASS

DESCRIBE_CLASS          ( &hostole_typelib, Hostware_token        , token  , CLSID_Token   , "hostWare.Token"  , "1.0" );
DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware_parser       , parser , CLSID_Parser  , "hostWare.Parser" , "1.0" );

//----------------------------------------------------------------Hostware_token::Hostware_token

Hostware_token::Hostware_token( IUnknown* pUnkOuter, Ole_class_descr* cls )   
: 
    Sos_ole_object( cls, this, pUnkOuter ), 
    _zero_(this+1)  
{ 
    VariantInit( &_value ); 
}

//---------------------------------------------------------------Hostware_token::~Hostware_token

Hostware_token::~Hostware_token()   
{ 
    if( _parser ) {
        _parser->Release();
        _parser = NULL;
    }

    VariantClear( &_value );
}

//---------------------------------------------------------------------Hostware_token::get_value

STDMETHODIMP Hostware_token::get_Value( VARIANT* o )
{ 
    HRESULT hr = NOERROR;
    
    VariantInit( o );

    if( _id == -1 )  return E_FAIL; // Vorsichtshalber

    if( V_VT( &_value ) == VT_EMPTY ) 
    {
        V_VT( o ) = VT_BSTR;
        V_BSTR( o ) = SysAllocString_string( _parser->_word_array[ _id ]._repr ); 
    } 
    else 
    {
        hr = VariantCopy( o, &_value ); 
    }

    return hr; 
}

//--------------------------------------------------------------Hostware_parser::Hostware_parser

Hostware_parser::Hostware_parser( IUnknown* pUnkOuter, Ole_class_descr* cls )   
: 
    Sos_ole_object( cls, this, pUnkOuter ) 
{
    //set_chars( _ident_chars, "ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ"
    //                         "abcdefghijklmnopqrstuvwxyzäöüß" );
    //set_chars( _special, "!\"$%&/()=?^\\`+*#';:_,.-<>|{}[]" );
}   

//------------------------------------------------------------------Hostware_parser::open_source

STDMETHODIMP Hostware_parser::Open_source( BSTR filename_bstr )
{
    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try {
        Sos_string filename = bstr_as_string(filename_bstr);

        // Erstmal einige besondere Ids herausfinden:

        _eof_id    = -1;
        _word_id   = -1;
        _string_id = -1;
        _number_id = -1;
        _double_quote_id = -1;
        _single_quote_id = -1;
        _back_quote_id   = -1;

        for( int i = _word_array.first_index(); i <= _word_array.last_index(); i++ ) 
        {
            Hostware_parser_word* w = &_word_array[ i ];
            
            if( stricmp( c_str(w->_repr), "(eof)"    ) == 0 )  _eof_id    = i;
            else                                 
            if( stricmp( c_str(w->_repr), "(word)"   ) == 0 )  _word_id = i;
            else   
            if( stricmp( c_str(w->_repr), "(string)" ) == 0 )  _string_id = i;
            else   
            if( stricmp( c_str(w->_repr), "(number)" ) == 0 )  _number_id = i;
            else   
            if( stricmp( c_str(w->_repr), "(text)"   ) == 0 )  _text_id = i;           // Nur bei _sgml
            else   
            if( strcmp( c_str(w->_repr), "\"\""      ) == 0 )  _double_quote_id = i;
            else   
            if( strcmp( c_str(w->_repr), "''"        ) == 0 )  _single_quote_id = i;
            else   
            if( strcmp( c_str(w->_repr), "``"        ) == 0 )  _back_quote_id = i;
        }


        _pos.filename( c_str(filename) );
        _pos._line = 0;

        _input.open( filename, Any_file::in_seq );
        get_char();
    }
    catch( const Xc& x )  { hr = _set_excepinfo( x, "hostWare.Parser::open_source" ); }

    return hr;
}                                                                     

//---------------------------------------------------------------------Hostware_parser::add_word

STDMETHODIMP Hostware_parser::Add_word( int ident, BSTR representation, VARIANT* extra )
{
    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try {
        if( ident <= 0 )  ident = _word_array.last_index() + 1;
        if( _word_array.last_index() < ident )  _word_array.last_index( ident );
        Hostware_parser_word* w = &_word_array[ ident ];
        w->_repr = bstr_as_string( representation );
        hr = VariantCopyInd( &w->_extra, extra );

        if( length( w->_repr ) >= 3  &&  w->_repr[0] == '<'  &&  w->_repr[(int)length(w->_repr)-1] == '>' )  _sgml = true;

        if( length( w->_repr ) >= 3  &&  w->_repr[0] == '&'  &&  w->_repr[(int)length(w->_repr)-1] == ';' ) {
            _sgml = true;
            //w->_repr.MakeUpper();
            for( int i = 0; i < w->_repr.length(); i++ )  w->_repr[i] = toupper(w->_repr[i]);
            w->_extra_string = variant_as_string( w->_extra );
            //w->_extra_string.MakeUpper();
            for( int j = 0; j < w->_repr.length(); j++ )  w->_repr[j] = toupper(w->_repr[j]);
        }

    }
    catch( const Xc& x )  { hr = _set_excepinfo( x, "hostWare.Parser::add_word" ); }

    return hr;
}

//-------------------------------------------------------------------Hostware_parser::word_repr

STDMETHODIMP Hostware_parser::Word_repr( int ident, BSTR* representation )
{
    HRESULT hr = NOERROR;

    try {
        Hostware_parser_word* w = &_word_array[ ident ];
        *representation = SysAllocString_string( w->_repr );
    }
    catch( const Xc& x )  { hr = _set_excepinfo( x, "hostWare.Parser::word_repr" ); }

    return hr;
}

//------------------------------------------------------------------Hostware_parser::word_extra

STDMETHODIMP Hostware_parser::Word_extra( int ident, VARIANT* extra )
{
    HRESULT hr = NOERROR;

    try {
        Hostware_parser_word* w = &_word_array[ ident ];
        hr = VariantCopy( extra, &w->_extra );
    }
    catch( const Xc& x )  { hr = _set_excepinfo( x, "hostWare.Parser::word_extra" ); }

    return hr;
}

//---------------------------------------------------------------------Hostware_parser::get_char

int Hostware_parser::get_char()
{
    _pos._col++;
    int c = _next_char;
    _next_char = _input.get();

    if( _next_char == '\n' )  { _pos._col = -1; _pos._line++; }
    else
    if( _next_char == '\t' )  { _pos._col = ( _pos._col + 8 ) & ~7; }

    return c;
}

//-------------------------------------------------------------------Hostware_parser::read_token

void Hostware_parser::read_token()
{
  //_expected_tokens_set.clear();
    VariantClear( &_token._value );

    while( sos_isspace( _next_char ) ) {
        get_char();
    }

    _token._pos = _pos;
    _token._id  = -1;

    _hilfspuffer.allocate_min( 10000 );

    if( _sgml && _next_char == '<'  
    || !_sgml && ( sos_isalpha( _next_char )  ||  _next_char == '_' ) ) 
    {
        _hilfspuffer.length( 0 );
        _hilfspuffer.append( get_char() );
        

        if( _sgml && _next_char == '/' )  _hilfspuffer.append( get_char() );

        while( sos_isalnum( _next_char )  ||  _next_char == '_' )  _hilfspuffer.append( (char)get_char() );

        if( _hilfspuffer.char_ptr()[0] == '<' ) {
            if( _next_char != '>' )  {
                _hilfspuffer.append( '\0' );
                throw_syntax_error( "SOS-PARSER-201", _hilfspuffer.char_ptr(), _token._pos );
            }
            _hilfspuffer.append( get_char() );
        }

        if( _Ignore_case )  _hilfspuffer.upper_case();

        int i;
        for( i = _word_array.first_index(); i <= _word_array.last_index(); i++ ) 
        {
            Hostware_parser_word* w = &_word_array[ i ];
            if( memcmp( c_str( w->_repr ), _hilfspuffer.char_ptr(), _hilfspuffer.length() ) == 0 )  break;
        }

        if( i <= _word_array.last_index() ) {
            _token._id = i;
        } else {
            V_VT( &_token._value ) = VT_BSTR;
            V_BSTR( &_token._value ) = SysAllocStringLen_char( _hilfspuffer.char_ptr(), _hilfspuffer.length() );
            _token._id = _word_id;
        }
    }
    else
    if( _next_char == EOF ) {
        _token._id = _eof_id;
    }
    else
    if( _sgml ) 
    {
        // Gegenstück hierzu Hostware::as_parser_string( ..., "SGML")
        _hilfspuffer.length( 0 );

        //schon erledigt: while( _next_char == ' '  ||  _next_char == '\t' )  get_char();     // Blanks am Anfang der ersten Zeile wegschneiden

        while( _next_char != '<'  &&  _next_char != EOF ) 
        {
            //while( _next_char == '\r'  ||  _next_char == '\n' )  get_char();

            if( _next_char == '&' ) 
            {
                get_char();

                if( _next_char == '#' ) {
                    get_char();
                    char c = 0;
                    while( sos_isdigit( _next_char ) )  c = 10*c + get_char() - '0';
                    if( _next_char != ';' )  throw_syntax_error( "SOS-PARSER-202", _token._pos );
                    get_char();
                    _hilfspuffer.append( c );
                }
                else
                if( _next_char == '&' )             // SGML-Konform?
                {
                    _hilfspuffer.append( get_char() );
                }
                else
                {
                    Sos_limited_text<max_sgml_tag_length>  txt;
                    
                    txt.append( '&' );

                    while( sos_isalnum( _next_char ) ) {
                        if( txt.length() == txt.size() )  throw_syntax_error( "SOS-PARSER-203", c_str( txt ), _token._pos );
                        txt.append( get_char() );
                    }
                    if( _next_char != ';' )  throw_syntax_error( "SOS-PARSER-202", _token._pos );
                    txt.append( get_char() );

                    if( _Ignore_case )  txt.upper_case();

                    Hostware_parser_word* w = NULL;
                    for( int i = _word_array.first_index(); i <= _word_array.last_index(); i++ ) 
                    {
                        if( strcmp( c_str(_word_array[i]._repr), c_str( txt ) ) == 0 )  { w = &_word_array[ i ];  break; }
                    }

                    if( !w )  throw_syntax_error( "SOS-PARSER-204", c_str( txt ), _token._pos );
                    _hilfspuffer.append( w->_extra_string );
                }
            }
            else
            {   
                if( _hilfspuffer.length() + 2 > _hilfspuffer.size() )   _hilfspuffer.resize_min( _hilfspuffer.length() + ( _hilfspuffer.length() < 1000000? 100000 : 1000000  ) );
#               if NL_IS_CRLF
                    if( _next_char == '\n' ) {
                        if( _hilfspuffer.length() > 0  &&  _hilfspuffer.char_ptr()[ _hilfspuffer.length() - 1 ] != '\r' )  _hilfspuffer.append( '\r' );
                    }
#               endif
                _hilfspuffer.append( get_char() );
            }
        }

        char* p = _hilfspuffer.char_ptr() + _hilfspuffer.length();
        while( p[-1] == ' ' || p[-1] == '\t' )  p--;
        if( p[-1] == '\n' )  p--;
        if( p[-1] == '\r' )  p--;
        _hilfspuffer.length( p - _hilfspuffer.char_ptr() );

        V_VT( &_token._value ) = VT_BSTR;
        V_BSTR( &_token._value ) = SysAllocStringLen_char( _hilfspuffer.char_ptr(), _hilfspuffer.length() );

        _token._id = _text_id;
    }
    else
    if( sos_isdigit( _next_char ) ) 
    {
        char  buffer [ 50 ];
        char* p     = buffer;
        char* p_end = buffer + sizeof buffer - 1;

        while( p < p_end  &&  sos_isdigit( _next_char ) )  *p++ = (char)get_char();

        _token._id = _number_id;
/*
        if( _next_char == '.' ) {
            *p++ = (char)get_char();
            while( p < p_end  &&  sos_isdigit( _next_char ) )  *p++ = (char)get_char();
            _token._kind = k_float; //Skalierung muss fest angegeben werden! k_decimal;
        }

        if( _next_char == 'e' || _next_char == 'E' ) {
            *p++ = (char)get_char();
            if( _next_char == '-' )  *p++ = (char)get_char();
            while( p < p_end  &&  sos_isdigit( _next_char ) )  *p++ = (char)get_char();
            _token._kind = k_float;
        }
*/
        *p = '\0';

      //_expected_tokens_set.include( k_number );
      //_expected_tokens_set.include( k_decimal );
      //_expected_tokens_set.include( k_float );

      //switch( _token._kind )
      //{
      //    case k_number : 
                V_VT( &_token._value ) = VT_UI4;
                V_I4( &_token._value ) = as_uint4( buffer );
      //        break;
      //    case k_decimal: _token._decimal = as_decimal( buffer );  break;
      //    case k_float  : _token._float   = as_double( buffer );   break;
      //}

    }
    else
    if( _next_char == '"'  ||  _next_char == '\''  ||  _next_char == '`' )
    {
        // Gegenstück hierzu Hostware::as_parser_string()
        int quote = _next_char;
        _hilfspuffer.length( 0 );
        get_char();

        while(1) {
            if( _hilfspuffer.length() == _hilfspuffer.size() )   _hilfspuffer.resize_min( _hilfspuffer.length() + ( _hilfspuffer.length() < 1000000? 100000 : 1000000  ) );

            if( _next_char == EOF  ||  _next_char == '\r'  ||  _next_char == '\n' )  throw_syntax_error( "SOS-PARSER-104", _token._pos );
            if( _next_char == quote ) {
                get_char();
                if( _next_char != quote ) {
                    // Zweite Stringkonstante setzt die erste fort? (wie in C, um Zeilenwechsel zu ermöglichen)
                    while( sos_isspace( _next_char ) ) {
                        if( _next_char == '\n' )  { _pos._col = -1; _pos._line++; }
                        get_char();
                    }
                    break;
                    if( _next_char != quote )  break;
                    get_char();
                }
                _hilfspuffer.append( quote );
                get_char();
            }
            else
            if( _next_char == '\\' )
            {
                get_char();
                int c;
                switch( _next_char ) {
                    case 'a': c = '\a'; break;
                    case 'b': c = '\b'; break;
                    case 'f': c = '\r'; break;
                    case 'n': c = '\n'; break;
                    case 'r': c = '\r'; break;
                    case 't': c = '\t'; break;
                    case 'v': c = '\v'; break;
                    case 'x': {
                        get_char();
                        c = 0;
                        
                        if( !sos_isxdigit( _next_char ) )  throw_syntax_error( "SOS-PARSER-105", _token._pos );
                        
                        c = sos_isdigit( _next_char )? _next_char - '0' : tabucase[ _next_char ] - 'A';
                        get_char();

                        if( sos_isxdigit( _next_char ) ) {
                            c <<= 4;
                            c = sos_isdigit( _next_char )? _next_char - '0' : tabucase[ _next_char ] - 'A';
                            get_char();
                        }
                    }
                    default:  c = _next_char; break;
                }

                _hilfspuffer.append( c );
            }
            else
            {
                _hilfspuffer.append( _next_char );
                get_char();
            }
        }

        _hilfspuffer.append( '\0' );
        _word_array.add( _hilfspuffer.char_ptr() );  
        _token._id = _word_array.last_index();

        switch( quote ) {
            case '"' : _token._id = _double_quote_id; break;
            case '\'': _token._id = _single_quote_id; break;
            case '`' : _token._id = _back_quote_id;   break;
        }
    }
    else
    if( sos_isgraph( _next_char ) )
    {
        int   matched_length    = 0;
        Bool  duplicate         = false;
        char  symbol [ 10+1 ];
        char* s                 = symbol;

        memset( symbol, 0, sizeof symbol );

        // Symbole mit gleichem Anfang stehen hintereinander in _word_array!

        int i = _word_array.first_index(); 

        while( sos_isgraph( _next_char )  &&  !sos_isalnum( _next_char )  &&  i <= _word_array.last_index() ) 
        {
            *s = _next_char;                  // Das nächste Zeichen nur für Fehlermeldung
            Hostware_parser_word* w = &_word_array[ i ];

            if( matched_length > 0  &&  memcmp( c_str(w->_repr), symbol, matched_length ) != 0 )  break;   // Anfang wechselt?
            duplicate = true;

            while( matched_length < length( w->_repr )  &&  _next_char == w->_repr[ matched_length ] ) 
            { 
                if( s >= symbol + NO_OF( symbol ) - 1 )  throw_syntax_error( "SOS-PARSER-103",  " ", _token._pos );
                matched_length++;
                *s++ = get_char();
                _token._id = i;
                duplicate = false;
            }

            i++;
        }

        if( duplicate  
        ||  _token._id == -1  
        ||  matched_length != length( _word_array[ _token._id ]._repr ) )  throw_syntax_error( "SOS-PARSER-102",  symbol, _token._pos );
    }

    if( _token._id == -1 ) 
    {
        throw_syntax_error( "SOS-PARSER-101",  " ", _token._pos );
    }


    //LOG( "sqlfield.cxx: read_token: " << Token::repr( _token._kind ) << ": \"" << _token._name << "\"\n" );
}

//--------------------------------------------------------------------Hostware_parser::set_chars
/*
STDMETHODIMP Hostware_parser::set_chars( Bool* set, BSTR o )
{
    Sos_string chars = o;

    memset( set, false, 256 * sizeof (Bool) );

    for( int i = 0; i < length( chars ); i++ ) {
        set[ (Byte)chars[ i ] ] = true;
    }
}
*/
//--------------------------------------------------------------Hostware_parser::get_next_token

STDMETHODIMP Hostware_parser::Get_next_token( Itoken** itoken )
{
    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try {
        read_token();

        Hostware_token* token = new Hostware_token;

        token->AddRef();
        token->_parser = this;
        token->_parser->AddRef();
        token->_pos = _token._pos;
        token->_id = _token._id;

        token->_value = _token._value;  
        VariantInit( &_token._value );
        //hr = VariantCopy( &token->_value, &_token._value ); 
        //if( FAILED( hr ) )  { token->Release(); return hr; }

        *itoken = token;
    }
    catch( const Xc& x )  { hr = _set_excepinfo( x, "hostWare.Parser::get_next_token" ); }

    return hr;
}


} //namespace sos