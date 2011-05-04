#include "precomp.h"
//#define MODULE_NAME "frmfield"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include <errno.h>      // errno
#include <ctype.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/soslimtx.h"
#include "../kram/sosarray.h"
#include "../kram/sosfield.h"
#include "../kram/sosdate.h"
#include "../kram/ebcdifld.h"
#include "../kram/tabucase.h"
#include "../kram/frmfield.h"
#include "../file/anyfile.h"
#include "../file/flstream.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/sosfld2.h"
#include "../kram/soslimtx.h"

using namespace std;
namespace sos {


#if defined SYSTEM_WIN || defined SYSTEM_DOS
    const ios::open_mode ios_binary = ios::binary;
# else
#    if defined SYSTEM_GNU
        const ios::openmode ios_binary = (ios::openmode) 0;
#     else
        const ios::open_mode ios_binary = (ios::open_mode) 0;
#    endif
#endif

/*
Punkt im Record-Namen ist nicht erlaubt.

.df RECORD
.im &VARIABLE[.](dateiname)

.*

feldname { attribute }
.df off

...


Folgende Attribute werden ausgewertet:
ini=intialwert
zwb ?  Leeres Zeichenfeld als 0 interpretieren?
zib ?
zif   Numeric
jan   Boolean
dat   Date
tag, mon, jah: Numeric
pos
len
typ=C/B/P


Attribute:

hou=jn   Stundenwert 0..23
min=jn   Minutenwert 0..59
ass=name    von Frame nicht implementiert, für Filter DF2ASS
cob=name    von Frame nicht implementiert, für Filter DF2COB
ini=initialwert
tlx=jn   Nur Telex-Zeichen
fut=jn   future date
pas=jn   past date
wdy=jn   Datum eines Wochentags
ful=jn   Feld vollständig ausfüllen
dag[=n]  default 1  date group  date group
zwb=jn   zero when blank
zib=jn   zero is blank
aex=jn   arithmetischer Ausdruck
cen=jn  zentrieren (Maske)
flg=jn  Fehlerflag "==>"
one=n   default 1  one of us 
oon=n   default 1  one or none
mof=n   default 1  mindestens ein Feld
aon=n   default 1 all or none
err=jn  Fehlervariable
zif=jn  Nur Ziffern
mus=jn  Muss-Feld
jan=jn  'J' oder 'N'
dat=jn  Datum "tt.mm.jj"
tag=jn
mon=jn
jah=jn
alp=jn   Alphanumerischer Wert erlaubt
gkl=jn   Umwandeln in Groß-/Kleinschreibung beim Lesen aus einer Datei
adr=jn   Variable, deren Wert indirekt gespeichert wird
tau=jn   Tausenderpunkte einfügen
pos=n    Position (ab 1)
len=n    Länge (in Bytes?)
typ=C/B/P/U  character (default), binär, gepackt, gepackt ohne Vorzeichen (um 4 Bit nach rechts verschoben)
msk=xx   von Frame nicht implementiert
pvi=jn   .pv ignorieren: Variable nicht mit .pv schreiben
bli=jn   blinken
int=jn   intensified
nor=jn   normal intensity
key=jn
dar=jn   dark
pro=jn|n   protected
vis=jn|n   visible
num=jn   numeric
und=jn   underline
upp=jn   uppercase  bei Terminaleingabe
low=jn   lowercase  bei Terminaleingabe
lef=jn   left adjust
rig=jn   right adjust

*/

//-----------------------------------------------------------------------------------Frame_df_file

struct Frame_df_file : Abs_file
{
                                Frame_df_file           ();
                               ~Frame_df_file           ();

    void                        open                    ( const char*, Open_mode, const File_spec& );

 protected:
    void                        get_record              ( Area& area );

    Fill_zero                  _zero_;
    Frame_parser               _parser;
    Sos_ptr<Record_type>       _column_type;
    Sos_ptr<Record_type>       _record_type;
};


//-----------------------------------------------------------------------------Frame_df_file_type

struct Frame_df_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "frame_df"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Frame_df_file> f = SOS_NEW( Frame_df_file );
        return +f;
    }
};

const Frame_df_file_type       _frame_df_file_type;
const Abs_file_type&            frame_df_file_type = _frame_df_file_type;



static const char* repr_tab[] =
{
    "",
    "<eof>",
    "<newline>",
    "','",
    "';'",
    "'='",
    "'.'",
    "'('",
    "')'",
    "'+'",
    "'-'",
    "'/'",
    "<string>",
    "<bezeichner>",
    "<zahl>"
};

Sos_simple_array<Frame_parser::Token::Token_entry> Frame_parser::Token::_token_array;


Frame_parser::Token::Token()
:
    _number ( 0 )
{
    _token_array.obj_const_name( "Frame_parser::_token_array" );
}

void Frame_parser::Token::init()
{
    if( _token_array.size() )  return;

  //_token_array.add( Token_entry( k_numeric       , "NUMERIC"          ) );  // oracle?
}



const char* Frame_parser::Token::repr( Kind k )
{
    return repr_tab[ k ];
}


Frame_parser::Stmt::Stmt() 
: 
    _zero_(this+1)
{
    _input_sp = -1;
}

/* Any_file = Any_file stürtzt ab. jz 7.8.97
void Frame_parser::Stmt::init( const Any_file& file, const Source_pos& pos ) 
{ 
    _input_stack[ 0 ].init( file );
    _input = &_input_stack[ 0 ]; 

    _pos = pos; 
}
*/


void Frame_parser::Stmt::im( const Sos_string& filename )
{ 
    // Gegenstück in read_next_stmt() bei EOF

    if( _input_sp == NO_OF( _input_stack ) - 1 )  throw_xc( "Frame: Zuviele .im" );

    Any_file_stream* inp = &_input_stack[ _input_sp + 1 ];
    inp->~Any_file_stream();
    memset( inp, 0, sizeof *inp );
    new( (void*)inp ) Any_file_stream;

    inp->open( filename, Any_file::Open_mode( Any_file::in | Any_file::seq ) );
    _input_sp++;
    _input = inp;

    _pos_stack[ _input_sp ] = _pos;
    _pos = Source_pos( c_str( filename ) );
}


void Frame_parser::Stmt::read_next_stmt()
{
    int c;


    _pos._line++; _pos._col = -1;
    _stmt.length( 0 );

    while(1)   // Für .im
    {
        c = _input->get();

        while( c == '.'  &&  _input->peek() == '*' ) {     // .* Kommentar
            _input->get();
            while(1) {
                c = _input->get();
                while( c == '\r' )  c = _input->get();
                if( c == EOF  )  break;
                if( c == '\n' )  {
                    _pos._line++; 
                    break;
                }
            }
            c = _input->get();
        }

        if( c != EOF )  break;  //ok

        if( _input_sp == 0 )  { _eof = true; return; }    // .im-Stack abgeräumt?

        _input_stack[ _input_sp ].close();
        _input_sp--;
        _input = &_input_stack[ _input_sp ];
        _pos = _pos_stack[ _input_sp ];
    }

    if( c == '.' )
    {
        while(1) {
            _stmt += (char)c;

            c = _input->get();
            while( c == '\r' )  c = _input->get();
            if( c == EOF  )  break;
            if( c == '\n' )  break;
            if( c == ';' ) {
                if( _input->peek() != ';' )  break;
                _input->get();
            }
        }
    }
    else
    {
        while( c != '\n' ) {
            _stmt += (char)c;

            c = _input->get();
            while( c == '\r' )  { c = _input->get(); }
            if( c == EOF  )  break;
            //if( c == '\n' )  break;
        }
    }

    _ptr0 = c_str( _stmt );
    _ptr  = _ptr0;
}


Frame_parser::Scanner::Scanner()
:
    _multi_line_stmt ( false )
{
}


/*
void Frame_parser::Scanner::init( const Any_file& file, const Source_pos& pos )
{
    _stmt.init( file, pos );
    _pos = pos;
    _stmt.read_next_stmt();
    read_next_token();
}
*/


void Frame_parser::Scanner::init( const Sos_string& filename )
{
    _stmt.im( filename );
    _stmt.read_next_stmt();
    read_next_token();
}


void Frame_parser::Scanner::read_next_token()
{
    Bool begin_of_line = false;

    if( _token._kind == Token::k_new_line ) {       // s. if( c == '\0' ) ...
        _stmt.read_next_stmt();
        begin_of_line = true;
    }

    _token = Token();
    _token._pos = _stmt.pos();
    _stmt._last_ptr = _stmt._ptr;

    if( _stmt._eof ) { _token._kind = Token::k_eof; return; }

    char c = _stmt.peek();

    if( begin_of_line  &&  !_multi_line_stmt  &&  *_stmt._ptr != '.' )  goto NO_TOKEN;

    if( !begin_of_line || _multi_line_stmt ) {
        while( c == ' ' || c == '\t' || c == '\r' )  { _stmt.get(); c = _stmt.peek(); }
    }

    if( c == '\0' ) {
        _token._kind = Token::k_new_line;
        //_stmt.read_next_stmt();  s.o.
    }
    else
    if( isdigit( c ) )
    {
        while( isdigit( _stmt.peek() ) )
        {
            _token._number *= 10; _token._number += _stmt.get() - '0';
        }
        _token._kind = Token::k_number;
    }
    else
    if( isalpha( c ) )
    {
        while( isalnum( _stmt.peek() )  ||  _stmt.peek() == '-'  ||  _stmt.peek() == '_' ||  _stmt.peek() == '.' )  // Punkt ist nicht erlaubt (warum nicht? jz 4.3.98)
        {
            _token._name += (char)_stmt.get();
        }

        _token._kind = Token::k_identifier;

        for( int i = Token::_token_array.first_index(); i <= Token::_token_array.last_index(); i++ )
        {
            if( stricmp( c_str( Token::_token_array[ i ]._name ), c_str( _token._name ) ) == 0 ) {
                _token._kind = Token::_token_array[ i ]._kind;
                break;
            }
        }
    }
    else
  //if( c == '&' ) Variable, "[A-Z$][A-Z0-9_]*"
    if( c == '.' ) { _stmt.get();  _token._kind = Token::k_point;  }
    else
    if( c == ',' ) { _stmt.get();  _token._kind = Token::k_comma;  }
    else
    if( c == ';' ) { _stmt.get();  _token._kind = Token::k_semikolon;  }
    else
    if( c == '=' ) { _stmt.get();  _token._kind = Token::k_gleich;  }
    else
    if( c == '(' ) { _stmt.get();  _token._kind = Token::k_klammer_auf;  }
    else
    if( c == ')' ) { _stmt.get();  _token._kind = Token::k_klammer_zu;  }
    else
    if( c == '+' ) { _stmt.get();  _token._kind = Token::k_plus;  }
    else
    if( c == '-' ) { _stmt.get();  _token._kind = Token::k_minus;  }
    else
    if( c == '\'' ) {
        _stmt.get();
        while(1) {
            char c = _stmt.peek();
            if( !c )  break;
            //if( c == EOF )  throw_syntax_error( "SOS-1181", _stmt.pos() );
            if( c == '\'' ) {
                _stmt.get();
                if( _stmt.peek() != '\'' )  break;
            }
            _token._name += c;
            _stmt.get();
        }
        _token._kind = Token::k_string;
    }
    else
    {
  NO_TOKEN:
        // Ungültig, vielleicht ist es Text
        _stmt.get();
        _token._kind = Token::k_none;
        //throw_syntax_error( "SOS-FRAME-1", _stmt.pos()  );
    }
}


Frame_parser::Field::Field()
:
    _zero_(this+1)
{
    _len_null = true;
    _for_vorkomma_null = true;
    _for_nachkomma_null = true;
    _ini_null = true;
    _typ_null = true;
    _one_null = true;
    _aon_null = true;
}

Frame_parser::Frame_parser()
:
    _zero_( this+1 )
{
    Token::init();
    //_next_token = Token( _input );
}

/*
Frame_parser::Frame_parser( const Any_file& file, const Source_pos& pos )
:
    _offset    ( 0 )
{
    init( file, pos );
}


void Frame_parser::init( const Any_file& file, const Source_pos& pos )
{
    _scanner.init( file, pos );
    Token::init();
}
*/

void Frame_parser::init( const Sos_string& filename )
{
    _scanner.init( filename );
    Token::init();
}

Sos_string Frame_parser::text()
{
    Sos_string stmt = as_string( _scanner._stmt._stmt );
    // _ptr ans Ende setzen:
    _scanner._stmt._ptr = c_str( _scanner._stmt._stmt ) + length( _scanner._stmt._stmt );
    _scanner._token._kind = Frame_parser::Token::k_none;      // Für read_next_token()
    _scanner.read_next_token();   // Muß k_new_line sein
    return stmt;
}

void Frame_parser::skip_stmt()
{
    _scanner._stmt._ptr = c_str( _scanner._stmt._stmt ) + length( _scanner._stmt._stmt );
    parse_stmt_end();
}

Sos_string Frame_parser::parse_rest()
{
    Sos_string stmt = _scanner._stmt._last_ptr;
    // _ptr ans Ende setzen:
    _scanner._stmt._ptr = c_str( _scanner._stmt._stmt ) + length( _scanner._stmt._stmt );
    _scanner.read_next_token();
    return stmt;
}

void Frame_parser::expect( Token::Kind kind )
{
    if( next_token().kind() != kind )  throw_syntax_error( "SOS-FRAME-2", Token::repr( kind ), next_token()._pos );
}

void Frame_parser::parse( Token::Kind kind )
{
    expect( kind );
    parse_token();
}


void Frame_parser::parse_token()
{
    _scanner.read_next_token();
}


uint4 Frame_parser::parse_number()
{
    expect( Token::k_number );
    uint4 i = next_token().number();
    parse_token();
    return i;
}

Sos_string Frame_parser::parse_identifier()
{
    expect( Token::k_identifier );
    Sos_string s = as_string( next_token().name() );
    parse_token();
    return s;
}

Sos_string Frame_parser::parse_string()
{
    expect( Token::k_string );
    Sos_string s = as_string( next_token().name() );
    parse_token();
    return s;
}

void Frame_parser::parse( const char* identifier )
{
    Sos_string str = parse_identifier();
    if( stricmp( c_str( str ), c_str( identifier ) ) != 0 )  throw_syntax_error( "SOS-FRAME-2", identifier, next_token()._pos );
}


Bool Frame_parser::next_token_is( const char* identifier )
{
    return next_token_is( Token::k_identifier )  &&  next_token().name() == identifier;
}


Bool Frame_parser::is_keyword( const char* word, const char* looking_for )
{
    return memcmp( word, looking_for, 3 ) == 0;
}


Frame_parser::Field Frame_parser::parse_df_field()
{
    Field               f;
    Sos_string          string;

    string = parse_identifier();
    strncpy( f._name, c_str( string ), sizeof f._name - 1 );

    f._offset = _offset;

    while( !end_of_stmt() ) {
        const Sos_string key_word = parse_identifier();
        if( length( key_word ) < 3 )  throw_syntax_error( "SOS-FRAME-3", c_str( key_word ), next_token()._pos );
        char kw [ 3 ];
        xlat( kw, c_str( key_word ), 3, tabucase );

        if( is_keyword( kw, "AON" ) ) {
            parse( Token::k_gleich );
            f._aon = parse_number();
            f._aon_null = false;
        }
      //else if( is_keyword( kw, "ASS" ) ) {
      //}
      //else if( is_keyword( kw, "COB" ) ) {
      //}
        else if( is_keyword( kw, "DAG" ) ) {
            parse( Token::k_gleich );
            parse_number();
        }
        else if( is_keyword( kw, "DAT" ) ) {
            f._dat = true;
        }
        else if( is_keyword( kw, "LEN" ) ) {
            parse( Token::k_gleich );
            f._len = parse_number();
            f._len_null = false;
        }
        else if( is_keyword( kw, "FOR" ) ) {
            parse( Token::k_gleich );
            f._for_vorkomma = parse_number();
            f._for_vorkomma_null = false;
            if( next_token_is( Token::k_point ) ) {
                parse_token();
                f._for_nachkomma = parse_number();
                f._for_nachkomma_null = false;
            }
        }
      //else if( is_keyword( kw, "FUL" ) ) {
      //}
        else if( is_keyword( kw, "FUT" ) ) {
            f._fut = true;
        }
        else if( is_keyword( kw, "HOU" ) ) {
            f._numeric = true;
        }
        else if( is_keyword( kw, "INI" ) ) {
            parse( Token::k_gleich );
            if( next_token_is( Token::k_string     ) )  string = parse_string();
            if( next_token_is( Token::k_identifier ) )  string = parse_identifier();
                                                  else  string = as_string( parse_number() );
            strncpy( f._ini, c_str( string ), sizeof f._ini );
            f._ini_null = false;
        }
        else if( is_keyword( kw, "JAH" ) ) {
            f._jah = true;
            f._numeric = true;
        }
        else if( is_keyword( kw, "JAN" ) ) {
            f._jan = true;
        }
        else if( is_keyword( kw, "IGN" ) ) {
            f._ign = true;
        }
        else if( is_keyword( kw, "MIN" ) ) {
        }
        else if( is_keyword( kw, "MON" ) ) {
            f._mon = true;
            f._numeric = true;
        }
        else if( is_keyword( kw, "MUS" ) ) {
            f._mus = true;
        }
        else if( is_keyword( kw, "LOW" ) ) {
            f._low = true;
        }
        else if( is_keyword( kw, "ONE" ) ) {
            parse( Token::k_gleich );
            f._one = parse_number();
            f._one_null = false;
        }
        else if( is_keyword( kw, "PAS" ) ) {
            f._pas = true;
        }
        if( is_keyword( kw, "POS" ) ) {
            parse( Token::k_gleich );
            f._offset = parse_number() - 1;
        }   
        else if( is_keyword( kw, "TAG" ) ) {
            f._tag = true;
            f._numeric = true;
        }
      //else if( is_keyword( kw, "TLX" ) ) {
      //}
        else if( is_keyword( kw, "TYP" ) ) {
            parse( Token::k_gleich );
            string = parse_identifier();
            strncpy( f._typ, c_str( string ), sizeof f._typ - 1 );
            f._typ_null = false;

            if( f._typ[0] == 'p'  ||  f._typ[0] == 'P' )  f._numeric = true;
        }
        else if( is_keyword( kw, "UPP" ) ) {
            f._upp = true;
        }
      //else if( is_keyword( kw, "WDY" ) ) {
      //}
      //else if( memcmp( kw, "ZIB" ) ) {
      //}
        else if( is_keyword( kw, "ZIF" ) ) {
            f._zif = true;
            f._numeric = true;
        }
      //else if( is_keyword( kw, "ZWB" ) ) {
      //}
        else {
            if( next_token_is( Token::k_gleich ) ) {
                parse_token();
                parse_token();
            }
        }
    }

    parse_stmt_end();

    return f;
}


Sos_ptr<Field_descr> Frame_parser::parse_df_field_descr()
{
    Sos_ptr<Field_type> t;

    Field f = parse_df_field();
    if( f._typ[0] == '\0'  || f._typ[0] == 'c'  ||  f._typ[0] == 'C' ) {
        if( f._numeric ) {
            Sos_ptr<Ebcdic_number_type> p = SOS_NEW( Ebcdic_number_type( f._len ) );
            p->scale( f._for_nachkomma );
            t = +p;
        }
        else {
            Sos_ptr<Ebcdic_text_type> p = SOS_NEW( Ebcdic_text_type( f._len, _ebcdic_flags ) );
            t = +p;
        }

        if( f._dat || f._fut || f._pas ) {
            Sos_ptr<As_date_type> p = SOS_NEW( As_date_type( t, "" ) );
            t = +p;
        }

        if( f._jan ) {
            Sos_ptr<As_bool_type> p = SOS_NEW( As_bool_type( t, "N", "J" ) );
            t = +p;
        }
    }
/*
    else
    if( typ[0] == 'b'  ||  typ[0] == 'B' )
    {
        //t = SOS_NEW( Little_endian_integer( len ) );
        //t->scale( nach_komma );
        //numeric = true;
    }
*/
    else
    if( f._typ[0] == 'p'  ||  f._typ[0] == 'P' )
    {
        if( f._len == 0 )  f._len = ( f._for_vorkomma + f._for_nachkomma + 1 + 1 ) / 2;
        Sos_ptr<Ebcdic_packed_type> p = SOS_NEW( Ebcdic_packed_type( f._len ) );
        p->scale( f._for_nachkomma );
        t = +p;
    }
    else
    throw_syntax_error( "SOS-FRAME-4", next_token()._pos );

    _offset = MAX( _offset, int( f._offset + t->field_size() ) );

    return SOS_NEW( Field_descr( t, c_str( f._name ), f._offset ) );


}



Bool Frame_parser::end_of_stmt()
{
    return next_token_is( Token::k_semikolon )  ||  next_token_is( Token::k_new_line );
}


void Frame_parser::parse_stmt_end()
{
    if( next_token_is( Token::k_semikolon ) )  parse_token();
                                         else  parse( Token::k_new_line );
}

Bool Frame_parser::parse_on_off()
{
    if( next_token_is( "on" )
     || next_token_is( "ja" )
     || next_token_is( "j"  )
     || next_token_is( "1"  )  )  { parse_token(); return true; }

    if( next_token_is( "off"  )
     || next_token_is( "nein" )
     || next_token_is( "n"    )
     || next_token_is( "0"    )  )  { parse_token(); return false; }

    throw_syntax_error( "SOS-FRAME-5", next_token()._pos );
    return false;
}

//-----------------------------------------------------------Frame_parser::parse_field()
/*
Sos_ptr<Record_type> Frame_parser::parse_field()
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t          = SOS_NEW( Record_type );

    //parse_stmt_begin();
    parse( Token::k_point );
    parse( "df" );
    Sos_string record_name = parse_identifier();
    _scanner._multi_line_stmt = true;
    parse_stmt_end();


    int df_nesting = 1;

    while(1)  {
        if( next_token_is( Token::k_point ) ) {
            parse_token();
            parse( "df" );
            if( next_token_is( "off" ) ) {
                parse_token();
                parse_stmt_end();
                if( --df_nesting == 0 )  break;
            }
            else {
                parse_identifier();
                parse_stmt_end();
                df_nesting++;
            }
        }
        else 
        if( end_of_stmt() )  parse_stmt_end();
                       else  t->add_field( parse_df_field_descr() );
    }
    _scanner._multi_line_stmt = false;

    return t;
}
*/
//---------------------------------------------------------------------Frame_parser::begin()

void Frame_parser::begin()
{
    parse( Token::k_point );
    parse( "df" );
    Sos_string record_name = parse_identifier();
    _scanner._multi_line_stmt = true;
    parse_stmt_end();

    _df_nesting = 1;
}

//---------------------------------------------------------------------Frame_parser::end()

void Frame_parser::end()
{
    _scanner._multi_line_stmt = false;
}

//------------------------------------------------------------Frame_parser::parse_until_field()

void Frame_parser::parse_until_field()
{
    while(1) {
        if( next_token_is( Token::k_point ) ) {
            parse_token();
            parse( "df" );
            if( next_token_is( "off" ) ) {
                parse_token();
                parse_stmt_end();
                if( --_df_nesting == 0 )  break;
            }
            else {
                parse_identifier();
                parse_stmt_end();
                _df_nesting++;
            }
        }
        else 
        if( end_of_stmt() )  parse_stmt_end();
                       else  break;  //t->add_field( parse_df_field() );
    }
}

//------------------------------------------------------------Frame_parser::parse_complete_df()

Sos_ptr<Record_type> Frame_parser::parse_complete_df()
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t          = SOS_NEW( Record_type );

    //parse_stmt_begin();
    begin();

    while(1) {
        parse_until_field();
        if( _df_nesting == 0 )  break;
        t->add_field( parse_df_field_descr() );
/*
        if( next_token_is( Token::k_point ) ) {
            parse_token();
            parse( "df" );
            if( next_token_is( "off" ) ) {
                parse_token();
                parse_stmt_end();
                if( --_df_nesting == 0 )  break;
            }
            else {
                parse_identifier();
                parse_stmt_end();
                _df_nesting++;
            }
        }
        else 
        if( end_of_stmt() )  parse_stmt_end();
                       else  t->add_field( parse_df_field() );
*/
    }
    end();

    return t;
}


#if 0
Sos_ptr<Record_type> frame_record_type( istream* s, const Source_pos& pos, Bool check_eof )
{
    Frame_parser parser ( s, pos );

    Sos_ptr<Record_type> record = parser.parse_complete_df();

    parser.expect( Frame_parser::Token::k_eof );
/*    while( isspace( s.peek() ) )  s.get();
    if( s.peek() != EOF )  throw_syntax_error( "SOS-1182", next_token()._pos );
*/
    return +record;
}
#endif


Sos_ptr<Record_type> frame_record_type( const char* param )
{
    Sos_string          filename;
    Frame_parser        parser;
    Ebcdic_type_flags   flags       = ebc_none;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() ) 
    {
        // s.a. cobfield.cxx!
        if( opt.flag( "mvs"    ) )  flags = Ebcdic_type_flags( opt.set()? flags & ~ebc_bs2000 | ebc_mvs : flags & ~ebc_mvs   );
        else
        if( opt.flag( "bs2000" ) )  flags = Ebcdic_type_flags( opt.set()? flags & ~ebc_mvs | ebc_bs2000 : flags & ~ebc_bs2000 );
        else
        if( opt.flag( "german" ) )  flags = Ebcdic_type_flags( opt.set()? flags & ~ebc_international | ebc_german : flags & ~ebc_german | ebc_international );
        else
        filename = opt.rest();
    }

    
    parser._ebcdic_flags = flags;
    parser.init( filename );

    Sos_ptr<Record_type> record = parser.parse_complete_df();

    parser.expect( Frame_parser::Token::k_eof );

    return +record;
}




Frame_df_file::Frame_df_file()
:
    _zero_(this+1)
{
}


Frame_df_file::~Frame_df_file()
{
}


void Frame_df_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    Sos_string filename;

    for( Sos_option_iterator opt = param; !opt.end(); opt.next() )
    {
        if( opt.pipe()  ||  opt.param() )  filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    //_parser._ebcdic_flags = ;
    _parser.init( filename );

    _column_type = Record_type::create();
    Record_type* t = _column_type;
    Frame_parser::Field* o = 0;

    t->name( "Frame_df_field" );
    t->allocate_fields( 22 );

    RECORD_TYPE_ADD_CHAR        ( name          , 0 );
    RECORD_TYPE_ADD_FIELD       ( offset        , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( len           , 0 );
    RECORD_TYPE_ADD_CHAR_NULL   ( typ           , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( for_vorkomma  , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( for_nachkomma , 0 );
    RECORD_TYPE_ADD_CHAR_NULL   ( ini           , 0 );
    RECORD_TYPE_ADD_FIELD_AS    ( upp           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( low           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( zif           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( mus           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( jan           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( dat           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( fut           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( pas           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( tag           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( mon           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( jah           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( numeric       , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_AS    ( ign           , 0, &bool_type );
    RECORD_TYPE_ADD_FIELD_NULL  ( one           , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( aon           , 0 );

    _any_file_ptr->_spec._field_type_ptr = +_column_type;

    _parser.begin();
}

void Frame_df_file::get_record( Area& buffer )
{
    _parser.parse_until_field();
    if( _parser._df_nesting == 0 )  throw_eof_error();

    Frame_parser::Field f = _parser.parse_df_field();

    buffer.assign( &f, sizeof f );
}


} //namespace sos
