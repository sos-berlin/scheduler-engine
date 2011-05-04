//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "sqlfield"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosctype.h"
#include "../kram/log.h"
#include "../file/anyfile.h"
#include "../kram/soslimtx.h"
#include "../kram/sosarray.h"
#include "../kram/stdfield.h"
#include "../kram/sqlfield.h"
#include "../kram/tabucase.h"
#include "../kram/sossql2.h"

using namespace std;
namespace sos {

/*
Record_type dynamisch aus einem String mit folgender SQL ähnlichen Syntax aufbauen:

record_descr:   ( field_descr {, field_descr } ).
field_descr:    name ield_type [position].
code:           "ebcdic"; "iso".
field_type:     "ebcdic char" [ size ];
                "ebcdic packed" ["with zero"]( no of digits );
                "ebcdic number" ( no of digits );
                "iso char" [ size ];
                "iso number" [ size ];
                "iso string0" size;
                ["unsigned"] endian integer_type.
endian:         "little endian"; "big endian".
integer_type:   "int4".
size:           ( number ).
position:       "at" number.


Beispiel srp:
( ident_nr              ebcdic char(6),
  ident_nr_x            ebcdic char(9),
  ident_nr_y            ebcdic char(3),
  klassifikation        ebcdic char(5),
  benennung_1           ebcdic char(30),
  benennung_2           ebcdic char(30),
  werkst_div            ebcdic char(30),
  werkst_div_kz         ebcdic char,
  beschaff_doku         ebcdic char(15),
  anlage_datum          ebcdic packed(7),
  ...
  sel                   ebcdic number(6)
  ...
  auftragab_vg_glj      ebcdic packed with zero(3),
  ...
*/

const Sql_parser::Token empty_token;

//Sos_simple_array<Sos_string> Sql_parser::Token::_token_array;
const char* Sql_parser::Token::_token_array [ Sql_parser::k__last ];

/*
static void error( const char* e )
{
    LOG( "error: " << e << "\n" );
    SHOW_ERR( e );

    Syntax_error x ( "syntax" );
    x.insert( e );
    throw x;
}
*/


const Sql_token_table_entry sql_token_table [] =
{
    { (int)Sql_parser::k_eof           , "<Ende>" },
    { (int)Sql_parser::k_comma         , "," },
    { (int)Sql_parser::k_semikolon     , ";" },
    { (int)Sql_parser::k_colon         , ":" },
    { (int)Sql_parser::k_punkt         , "." },
    { (int)Sql_parser::k_klammer_auf   , "(" },
    { (int)Sql_parser::k_klammer_zu    , ")" },
    { (int)Sql_parser::k_eckklammer_auf, "[" },
    { (int)Sql_parser::k_eckklammer_zu , "]" },
    { (int)Sql_parser::k_star          , "*" },
    { (int)Sql_parser::k_slash         , "/" },
    { (int)Sql_parser::k_plus          , "+" },
    { (int)Sql_parser::k_minus         , "-" },
    { (int)Sql_parser::k_lt            , "<" },
    { (int)Sql_parser::k_le            , "<=" },  // !>
    { (int)Sql_parser::k_eq            , "=" },   // ==
    { (int)Sql_parser::k_ne            , "!=" },  // <>
    { (int)Sql_parser::k_ge            , ">=" },  // !<
    { (int)Sql_parser::k_gt            , ">" },
    { (int)Sql_parser::k_regex_match   , "=~" },
    { (int)Sql_parser::k_star_eq       , "*=" },
    { (int)Sql_parser::k_eq_star       , "=*" },
    { (int)Sql_parser::k_colon_eq      , ":=" },
    { (int)Sql_parser::k_eq_colon      , "=:" },
    { (int)Sql_parser::k_plus_in_klammern, "(+)" },
    { (int)Sql_parser::k_ausr_in_klammern, "(!)" },
    { (int)Sql_parser::k_question_mark , "?" },
    { (int)Sql_parser::k_pipe          , "|" },
    { (int)Sql_parser::k_doppelstrich  , "||" },
    { (int)Sql_parser::k_and           , "AND" },
    { (int)Sql_parser::k_all           , "ALL" },
    { (int)Sql_parser::k_any           , "ANY" },
    { (int)Sql_parser::k_as            , "AS" },
    { (int)Sql_parser::k_asc           , "ASC" },
    { (int)Sql_parser::k_assert        , "ASSERT" },
    { (int)Sql_parser::k_avg           , "AVG" },
  //{ (int)Sql_parser::k_at            , "AT" },
    { (int)Sql_parser::k_between       , "BETWEEN" },
  //{ (int)Sql_parser::k_big           , "BIG" },
    { (int)Sql_parser::k_by            , "BY" },
  //{ (int)Sql_parser::k_char          , "CHAR" },           // ingres
  //{ (int)Sql_parser::k_create        , "CREATE" },
    { (int)Sql_parser::k_createobject  , "CREATEOBJECT" },
    { (int)Sql_parser::k_count         , "COUNT" },
  //{ (int)Sql_parser::k_date          , "DATE" },           // ingres
    { (int)Sql_parser::k_delete        , "DELETE" },
    { (int)Sql_parser::k_desc          , "DESC" },
    { (int)Sql_parser::k_distinct      , "DISTINCT" },
  //{ (int)Sql_parser::k_ebcdic        , "EBCDIC" },         // /370
  //{ (int)Sql_parser::k_endian        , "ENDIAN" },
    { (int)Sql_parser::k_exists        , "EXISTS" },
    { (int)Sql_parser::k_from          , "FROM" },
    { (int)Sql_parser::k_group         , "GROUP" },
    { (int)Sql_parser::k_having        , "HAVING" },
    { (int)Sql_parser::k_in            , "IN" },
    { (int)Sql_parser::k_into          , "INTO" },
    { (int)Sql_parser::k_insert        , "INSERT" },
  //{ (int)Sql_parser::k_int4          , "INT4" },
  //{ (int)Sql_parser::k_integer4      , "INTEGER4" },       // ingres
    { (int)Sql_parser::k_is            , "IS" },
  //{ (int)Sql_parser::k_iso           , "ISO" },
    { (int)Sql_parser::k_let           , "LET" },
  //{ (int)Sql_parser::k_long          , "LONG" },           // ingres
    { (int)Sql_parser::k_like          , "LIKE" },
  //{ (int)Sql_parser::k_little        , "LITTLE" },
    { (int)Sql_parser::k_loop          , "LOOP" },
    { (int)Sql_parser::k_max           , "MAX" },
    { (int)Sql_parser::k_min           , "MIN" },
    { (int)Sql_parser::k_not           , "NOT" },
    { (int)Sql_parser::k_null          , "NULL" },
  //{ (int)Sql_parser::k_number        , "NUMBER" },
  //{ (int)Sql_parser::k_numeric       , "NUMERIC" },        // oracle?
    { (int)Sql_parser::k_or            , "OR" },
    { (int)Sql_parser::k_order         , "ORDER" },
  //{ (int)Sql_parser::k_packed        , "PACKED" },         // /370
    { (int)Sql_parser::k_rowid         , "ROWID" },
    { (int)Sql_parser::k_select        , "SELECT" },
    { (int)Sql_parser::k_set           , "SET" },
  //{ (int)Sql_parser::k_stddev        , "STDDEV" },
  //{ (int)Sql_parser::k_store         , "STORE" },
    { (int)Sql_parser::k_sum           , "SUM" },
  //{ (int)Sql_parser::k_table         , "TABLE" },
  //{ (int)Sql_parser::k_text          , "TEXT" },
    { (int)Sql_parser::k_update        , "UPDATE" },
    { (int)Sql_parser::k_values        , "VALUES" },
  //{ (int)Sql_parser::k_varchar       , "VARCHAR" },        // ingres
  //{ (int)Sql_parser::k_variance      , "VARIANCE" },
    { (int)Sql_parser::k_where         , "WHERE" },
    { (int)Sql_parser::k_identifier    , "<Name>" },
    { (int)Sql_parser::k_string        , "<String>" },
    { (int)Sql_parser::k_number        , "<natürliche Zahl>" },
    { (int)Sql_parser::k_decimal       , "<Festpunktzahl>" },
    { (int)Sql_parser::k_float         , "<Fließkommazahl>" }
};

const int sql_token_table_size = NO_OF( sql_token_table );

//--------------------------------------------------------------------------Sql_parser::name_of
/*
const char* Sql_parser::name_of( Kinde k )
{
    for( const Sql_token_table_entry* t = sql_token_table; t < sql_token_table + NO_OF( sql_token_table ); t++ ) 
    {
        if( t->_kind )  return t->_name;
    }

    return "?";
}
*/
//----------------------------------------------------------------------Sql_parser::Token::init

void Sql_parser::Token::init()
{
    //LOG( "Sql_parser::Token::init() sos_isspace('|')=" << (int)sos_isspace( (Byte)'|') << '\n' );
    if( _token_array[ k_select ] )  return;

    //_token_array.last_index( (int)k__last - 1 );

    for( const Sql_token_table_entry* t = sql_token_table; t < sql_token_table + NO_OF( sql_token_table ); t++ ) {
        _token_array[ (Sql_parser::Kind)t->_kind ] = t->_name;
    }
}

//---------------------------------------------------------------------Sql_parser::Token::Token

Sql_parser::Token::Token()
:
    _number ( 0 )
{
}

//--------------------------------------------------------------------------Sql_parser::get_char

int Sql_parser::get_char()
{
    _pos._col++;
    int c = _next_char;
    _next_char = _input->get();
//LOG( "Sql_parser::get_char(): '" << (char)c << "'" << (int)c <<"\n" );
    return c;
}

//------------------------------------------------------------------------Sql_parser::Sql_parser

Sql_parser::Sql_parser( istream* s, const Source_pos& pos  )
:
    _zero_     ( this+1 ),
    _input     ( s ),
    _pos       ( pos ),
    _expected_tokens_set( k__last )
{
/*
    _static = sos_static_ptr()->_sossql;
    if( !_static ) {
        _static = SOS_NEW_PTR( Sossql_static );
        sos_static_ptr()->_sossql = +_static;
    }
*/
    _hilfspuffer.allocate_min( 1024 );      // Für Identifier und String
    _pos._line = 0;   // 1. Zeile
    Token::init();
    get_char();
    read_token();
}

//----------------------------------------------------------------------Sql_parser::~Sql_parser

Sql_parser::~Sql_parser()
{
}

//----------------------------------------------------------------------Sql_parser::check_token

void Sql_parser::check_token( Kind kind )
{
    //if( next_token().kind() != kind ) {
    if( next_token_is( kind ) )  return;

    //Sos_string k = Token::repr( kind );
    //throw_syntax_error( "SOS-SQL-11", c_str( k ), _token._pos );
    Sos_string tokens;
    Bool       first = true;
    int        i     = 0;
    int        next_i = _expected_tokens_set.scan( 0, true );

    while(1) {
        i = next_i;
        if( i == -1 )  break;
        next_i = _expected_tokens_set.scan( i + 1, true );
        if( !first )  tokens += next_i >= 0? ", " : " oder ";
        first = false;
        tokens += Token::repr( (Kind)i );
    }

    Syntax_error x ( "SOS-SQL-11" );
    x.insert( c_str( tokens ) );
    //x.insert( c_str( next_token()._name ) );
    Sos_string rest = next_token()._name + ", rest: " + parse_rest();
    x.insert( c_str( rest ) );
    x.pos( _token._pos );
    throw x;
}

//-----------------------------------------------------------------------Sql_parser::read_token

void Sql_parser::read_token()
{
    int c;

    _expected_tokens_set.clear();

    _token = empty_token;

    while(1)
    {
        while( sos_isspace( _next_char ) ) {
//LOG( "sos_isspace('" << _next_char << "')!\n" );
            if( _next_char == '\n' )  { _pos._col = -1; _pos._line++; }
            get_char();
        }

        _token._pos = _pos;

        c = _next_char;
        if( c != '/' )  break;
        get_char();

        if( _next_char == '*' ) {        //  /* ... */
            get_char();
            while(1) {
                if( _next_char == '\n' )  { _pos._col = -1; _pos._line++; }
                if( _next_char == EOF )  throw_syntax_error( "SOS-SQL-1", _token._pos );
                if( _next_char == '*' ) {
                    get_char();
                    if( _next_char == '/' )  break;
                }
                get_char();
            }
            get_char();
            c = _next_char;
            _token._pos = _pos;
        }
        else
        if( _next_char == '/' ) {
            get_char();
            while(1) {
                if( _next_char == '\n' )  break;
                if( _next_char == EOF )  break;
                get_char();
            }
            get_char();
            _pos._col = -1;
            _pos._line++;
            c = _next_char;
            _token._pos = _pos;
        }
        else { _token._kind = k_slash; goto ENDE; }
    }

    _token._pos = _pos;

    switch( c ) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
        char  buffer [ 50 ];
        char* p     = buffer;
        char* p_end = buffer + sizeof buffer - 1;

        while( p < p_end  &&  sos_isdigit( _next_char ) )  *p++ = (char)get_char();

        _token._kind = k_number;

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

        *p = '\0';

        _expected_tokens_set.include( k_number );
        _expected_tokens_set.include( k_decimal );
        _expected_tokens_set.include( k_float );

        switch( _token._kind )
        {
            case k_number : _token._number  = as_big_int( buffer );  break;
            case k_decimal: _token._decimal = as_decimal( buffer );  break;
            case k_float  : _token._float   = as_double( buffer );   break;
            default       : throw_xc( "Sql_parser::read_token" );
        }

        break;
    }

    case 'A':  case 'B':  case 'C':  case 'D':  case 'E':  case 'F':  case 'G':  case 'H':
    case 'a':  case 'b':  case 'c':  case 'd':  case 'e':  case 'f':  case 'g':  case 'h':
    case 'I':  case 'J':  case 'K':  case 'L':  case 'M':  case 'N':  case 'O':  case 'P':
    case 'i':  case 'j':  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':  case 'p':
    case 'Q':  case 'R':  case 'S':  case 'T':  case 'U':  case 'V':  case 'W':  case 'X':
    case 'q':  case 'r':  case 's':  case 't':  case 'u':  case 'v':  case 'w':  case 'x':
    case 'Y':  case 'Z':
    case 'y':  case 'z':
    case 'Ä':  case 'Ö':  case 'Ü':
    case 'ä':  case 'ö':  case 'ü':
    case '_':
    {
        //char buffer[ 2 ];
        //buffer[ 1 ] = '\0';
        _hilfspuffer.length( 0 );
        while( sos_isalnum( _next_char )  ||  _next_char == '_' )
        {
            //buffer [ 0 ] = (char)get_char();
            //_token._name += buffer;
            _hilfspuffer += (char)get_char();
        }

        _hilfspuffer += '\0';
        _token._name = _hilfspuffer.char_ptr();
        _token._kind = k_identifier;
/*
        for( int i = Token::_token_array.first_index(); i <= Token::_token_array.last_index(); i++ )
        {
            if( stricmp( c_str( Token::_token_array[ i ] ), c_str( _token._name ) ) == 0 ) {
                _token._kind = Kind( i );
                break;
            }
        }
*/
        int  len = length( _token._name );
        char name [ 30 ];

        if( len <= sizeof name )
        {
            xlat( name, c_str( _token._name ), len, tabucase );

            for( int i = k__keyword_first; i < k__keyword_last; i++ )
            {
                const Sos_string& tok = Token::_token_array[ i ];
                if( length( tok ) == len  &&  memcmp( name, c_str( tok ), len ) == 0 ) {
                    _token._kind = Kind( i );
                    break;
                }
            }
        }

        break;
    }

    case EOF:   _token._kind = k_eof;                          break;
    case '.':   _token._kind = k_punkt;           get_char();  break;
    case ',':   _token._kind = k_comma;           get_char();  break;
    case ';':   _token._kind = k_semikolon;       get_char();  break;
    case ')':   _token._kind = k_klammer_zu;      get_char();  break;
    case '[':   _token._kind = k_eckklammer_auf;  get_char();  break;
    case ']':   _token._kind = k_eckklammer_zu;   get_char();  break;
    case '?':   _token._kind = k_question_mark;   get_char();  break;
    case '+':   _token._kind = k_plus;            get_char();  break;
    case '-':   _token._kind = k_minus;           get_char();  break;

    case '*':   get_char();
                if( _next_char == '=' )  { _token._kind = k_star_eq; get_char(); }
                                  else  _token._kind = k_star;
                break;

    case '(':   get_char();
                if( _next_char == '+' ) {
                    get_char();
                    if( _next_char == ')' ) { _token._kind = k_plus_in_klammern; get_char(); }
                    else {
                        // k_klammer_auf liefern und beim nächsten read_token() k_plus
                        goto FEHLER;
                    }
                }
                else
                if( _next_char == '!' ) {
                    get_char();
                    if( _next_char == ')' ) { _token._kind = k_ausr_in_klammern; get_char(); }
                    else goto FEHLER;
                }
                else _token._kind = k_klammer_auf;
                break;

    case '|':   get_char();
                if( _next_char == '|' )  { _token._kind = k_doppelstrich;  get_char(); }
                                   else  _token._kind = k_pipe;
                break;

    case '<':   get_char();
                if( _next_char == '=' )  { _token._kind = k_le; get_char(); }
                else
                if( _next_char == '>' )  { _token._kind = k_ne; get_char(); }
                else                       _token._kind = k_lt;
                break;

    case '>':   get_char();
                if( _next_char == '=' )  { _token._kind = k_ge; get_char(); }
                                   else    _token._kind = k_gt;
                break;

    case '!':   get_char();
                if( _next_char == '=' )  { _token._kind = k_ne; get_char(); }
                else
                if( _next_char == '<' )  { _token._kind = k_ge; get_char(); }
                else
                if( _next_char == '>' )  { _token._kind = k_le; get_char(); }
                                   else  throw_syntax_error( "SOS-SQL-2", "!", _token._pos );
                break;

    case '=':   get_char();
                if( _next_char == '*' )  { _token._kind = k_eq_star; get_char(); }
                else
                if( _next_char == '~' )  { _token._kind = k_regex_match; get_char(); }
                else
                if( _next_char == ':' )  { _token._kind = k_eq_colon; get_char(); }
                else {
                    if( _next_char == '=' )  get_char();
                    _token._kind = k_eq;
                }
                break;

    case ':':   get_char();
                if( _next_char == '=' )  { _token._kind = k_colon_eq; get_char(); }
                                   else    _token._kind = k_colon;
                break;

    case '\'':
  //case '"':
    case sql_identifier_quote_char:
    case '`': // kompatibel zur alten Version - 21.5.97
    {
        int quote = c;
        _hilfspuffer.length( 0 );
        get_char();

        while(1) {
            c = _next_char;
            if( c == EOF )  throw_syntax_error( "SOS-SQL-3", _token._pos );
            if( c == quote ) {
                get_char();
                if( _next_char != quote )  break;
            }
            _hilfspuffer += (char)c;
            get_char();
        }

        _hilfspuffer += '\0';
        _token._name = _hilfspuffer.char_ptr();

        if( quote == sql_identifier_quote_char
         || quote == '`'                       )  _token._kind = k_identifier;
                                            else  _token._kind = k_string;
        break;
    }

    default:
        FEHLER:
        throw_syntax_error( "SOS-SQL-2",  " ", _token._pos );
    }

  ENDE: ;
    //LOG( "sqlfield.cxx: read_token: " << Token::repr( _token._kind ) << ": \"" << _token._name << "\"\n" );
}

//----------------------------------------------------------------------------Sql_parser::parse
/*
void Sql_parser::parse( Token::Kind kind )
{
    check_token( kind );
    parse_token();
}
*/
//----------------------------------------------------------------------Sql_parser::parse_token
/*
void Sql_parser::parse_token()
{
    read_token();
    //_next_token = Token( _input );
}
*/
//-----------------------------------------------------------------------Sql_parser::parse_rest

Sos_string Sql_parser::parse_rest()
{
    ZERO_RETURN_VALUE( Sos_string );

    // Nicht zuvor next_token() rufen!!!!

    Dynamic_area buffer ( 1024 );

    while(1) {
        _input->read( buffer.char_ptr() + buffer.length(),
                      buffer.size() - buffer.length() );
        uint len = _input->gcount();
        buffer.length( buffer.length() + len );
        if( len < buffer.size() )  break;
        buffer.resize_min( buffer.size() + 4000 );
    }

    _token._kind = k_eof;

    return as_string( buffer );
}

//---------------------------------------------------------------------Sql_parser::parse_number

Big_int Sql_parser::parse_number()
{
    check_token( k_number );
    Big_int i = next_token().number();
    parse_token();
    return i;
}

//-----------------------------------------------------------------Sql_parser::parse_identifier

Sos_string Sql_parser::parse_identifier()
{
    ZERO_RETURN_VALUE( Sos_string );

    check_token( k_identifier );
    Sos_string s = next_token().name();
    parse_token();
    return s;
}

//--------------------------------------------------------------------Sql_parser::parse_string

Sos_string Sql_parser::parse_string()
{
    ZERO_RETURN_VALUE( Sos_string );

    check_token( k_string );
    Sos_string s = next_token().name();
    parse_token();
    return s;
}

//------------------------------------------------------Sql_parser::parse_identifier_or_string

Sos_string Sql_parser::parse_identifier_or_string()
{
    ZERO_RETURN_VALUE( Sos_string );

    if( next_token_is( k_string ) )  return parse_string();
                                      else  return parse_identifier();
}

//----------------------------------------------------------------------Sql_parser::parse_size
/*
uint4 Sql_parser::parse_size()
{
    parse( k_klammer_auf );
    uint4 n = parse_number();
    parse( k_klammer_zu );
    return n;
}
*/
//----------------------------------------------------------------Sql_parser::parse_field_type
#if 0

Sos_ptr<Field_type> Sql_parser::parse_field_type()
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_type> );

/*
    if( next_token_is( k_ebcdic ) )
    {
        switch( next_token().kind() )
        {
            case k_char:
            {
                int size = 1;
                parse_token();
                if( next_token().kind() == k_klammer_auf )  size = parse_size();
                Sos_ptr<Ebcdic_string> t = SOS_NEW_PTR( Ebcdic_string( size ) );
                return +t;
            }

            case k_numeric:
            {
                int size = 1;
                parse_token();
                if( next_token().kind() == k_klammer_auf )  size = parse_size();
                Sos_ptr<Ebcdic_number> t = SOS_NEW_PTR( Ebcdic_number( size ) );
                return +t;
            }

            case k_packed:
            {
                int size = 1;
                parse_token();
                if( next_token().kind() == k_klammer_auf )  size = parse_size();
                Sos_ptr<Ebcdic_packed> t = SOS_NEW_PTR( Ebcdic_packed( ( size + 1 + 1 ) / 2 ) );
                return +t;
            }

            default: throw Syntax_error( "syntax" );
        }
    }
    else
*/
    if( next_token_is( k_long ) )
    {
        parse_token();
        Sos_ptr<Long_type> t = SOS_NEW_PTR( Long_type );
        return +t;
    }
    else
/*
    if( next_token_is( k_little ) )
    {
        parse_token();
        parse( k_endian );
        parse( k_int4 );
        parse_token();
        Sos_ptr<Int4_field> t = SOS_NEW_PTR( Int4_field );
        return +t;
    }
    else
    if( next_token_is( k_big ) )
    {
        parse_token();
        parse( k_endian );
        parse( k_int4 );
        parse_token();
        Sos_ptr<Big_endian_int4> t = SOS_NEW_PTR( Big_endian_int4 );
        return +t;
    }
    else
*/
    {
        if( next_token_is( k_iso ) )  parse_token();

        _expected_tokens_set.include( k_date );
        _expected_tokens_set.include( k_text );
        //...

        switch( next_token().kind() )
        {
            case k_date:
            case k_text:
            case k_varchar:
            case k_char:
            {
                int size = 1;
                parse_token();
                if( next_token().kind() == k_klammer_auf )  size = parse_size();
                Sos_ptr<Iso_string> t = SOS_NEW_PTR( Iso_string( size ) );
                return +t;
            }

            case k_numeric:
            {
                int vor_komma;
                int nach_komma = 0;

                parse_token();
                parse( k_klammer_auf );
                vor_komma = parse_number();
                if( next_token_is( k_comma ) ) {
                    parse_token();
                    nach_komma = parse_number();
                }
                parse( k_klammer_zu );

                //Sos_ptr<Iso_number> t = SOS_NEW_PTR( Iso_number( vor_komma, nach_komma ) );
                Sos_ptr<Long_type> t = SOS_NEW_PTR( Long_type );
                return +t;
            }

            default: ;
        }
    }

    throw_syntax_error( "SOS-SQL-10", _token._pos );
    return 0;
}
#endif
//------------------------------------------------------------------Sql_parser::parse_at_clause
/*
uint4 Sql_parser::parse_at_clause()
{
    parse( k_at );
    return parse_number();
}
*/
//------------------------------------------------------------------Sql_parser::parse_field_decl
/*
Sos_ptr<Field_descr> Sql_parser::parse_field_decl()
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_descr> );

    Sos_string name = parse_identifier();
    Sos_ptr<Field_type> t;

    t = parse_field_type();

    if( next_token_is( k_not ) ) {
        parse_token();
        parse( k_null );
    }

    int offset = next_token_is( k_at )? offset = parse_at_clause()
                                             : offset = _offset;

    _offset = MAX( _offset, offset + t->field_size() );

    return SOS_NEW_PTR( Field_descr( t, c_str(name), offset ) );
}
*/
//----------------------------------------------------------------Sql_parser::parse_record_decl
/*
Sos_ptr<Dyn_record_type> Sql_parser::parse_record_decl()
{
    ZERO_RETURN_VALUE( Sos_ptr<Dyn_record_type> );

    Sos_ptr<Dyn_record_type> t = SOS_NEW_PTR( Dyn_record_type );

    while( next_token().kind() == k_identifier )
    {
        t->add_field( parse_field_decl() );
        if( !next_token_is( k_comma ) )  break;
        parse_token();
    }

    return t;
    //if( next_token().kind() != k_eof )  error( "Dateiende erwartet" );
}
*/
//------------------------------------------------------------------------------sql_record_type
/*
Sos_ptr<Record_type> sql_record_type( istream* s, Bool check_eof )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sql_parser           parser ( s );
    Sos_ptr<Dyn_record_type> t;

    if( parser.next_token_is( Sql_parser::k_create ) )    // CREATE TABLE ?
    {
        parser.parse_token();
        parser.parse( Sql_parser::k_table );
        parser.parse_identifier();
        parser.parse( Sql_parser::k_klammer_auf );
        t = parser.parse_record_decl();
        parser.parse( Sql_parser::k_klammer_zu );
        if( parser.next_token_is( Sql_parser::k_semikolon ) ) {
            parser.parse_token();
        }
    }
    else
    {
        t = parser.parse_record_decl();
    }

    if( check_eof ) {
        parser.parse( Sql_parser::k_eof );
    }

    return +t;
}
*/
//------------------------------------------------------------------------------sql_record_type
/*
Sos_ptr<Record_type> sql_record_type( const char* filename )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    ifstream s ( filename );
    return sql_record_type( &s, true );
}
*/

} //namespace sos
