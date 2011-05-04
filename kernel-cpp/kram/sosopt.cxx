#include "precomp.h"
//#define MODULE_NAME "sosopt"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sosstrng.h"
#include "sos.h"
#include "sosctype.h"
#include "sosopt.h"
#include "log.h"

using namespace std;
namespace sos {

//-----------------------------------------------------Sos_option_iterator::Sos_option_iterator

Sos_option_iterator::Sos_option_iterator( const char* string )
:
    _zero_( this+1),
    _alt_option_char ( '-' )
{
    _max_params = -1;
    _string = string;
    _ptr = c_str( string );
    next();
}

//-----------------------------------------------------Sos_option_iterator::Sos_option_iterator

Sos_option_iterator::Sos_option_iterator( const Sos_string& string )
:
    _zero_( this+1 ),
    _alt_option_char ( '-' )
{
    _max_params = -1;
    _string = string;
    _ptr = c_str( _string );
    next();
}

//-----------------------------------------------------Sos_option_iterator::Sos_option_iterator

Sos_option_iterator::Sos_option_iterator( int argc, char** argv, Bool ignore_first  )
:
    _zero_( this+1 ),
    _alt_option_char ( '-' )
{
#   if defined SYSTEM_WIN
        alt_option_char( '/' );
        ignore_case( true );
#   endif

    _max_params = -1;
    if( ignore_first )  { argc--; argv++; }

    _argc = argc;
    _argv = (const char**)argv;
    _use_argv = true;
    _arg_i = -1;

    next();
}

//-----------------------------------------------------Sos_option_iterator::Sos_option_iterator

Sos_option_iterator::Sos_option_iterator( int argc, char** argv, const string& params )
:
    _zero_( this+1 ),
    _alt_option_char ( '-' )
{
    const bool ignore_first = true;

#   if defined SYSTEM_WIN
        alt_option_char( '/' );
        ignore_case( true );
#   endif

    _max_params = -1;

    if( argc == 0 && argv == NULL )
    {
        _string = params;
        _ptr = c_str( _string );
    }
    else
    {
        if( ignore_first )  { argc--; argv++; }
        _argc = argc;
        _argv = (const char**)argv;
        _use_argv = true;
        _arg_i = -1;
    }

    next();
}

//----------------------------------------------------Sos_option_iterator::~Sos_option_iterator

Sos_option_iterator::~Sos_option_iterator()
{
}

//-------------------------------------------------------------Sos_option_iterator::read_option

void Sos_option_iterator::read_option()
{
    char*       q = _option;
    const char* p = _ptr;

    if( _no_more_options )  goto FERTIG;  // "--"      

    if( ( *p == '-'  ||  *p == _alt_option_char )  &&  p[1] != '\0'  &&  !sos_isspace( p[1] ) ) 
	{
		if( *p == '/' ) {	// Windows-Schalterzeichen?
			const char* pp = p;
			while( *pp  &&  !sos_isspace( *pp ) ) {
				if( *p == '.' )  goto FERTIG;			  // Dateinamenszeichen? Keine Option
				if( *p == '/' )  goto FERTIG;			  // Dateinamenszeichen? Keine Option
				pp++;
			}
		}

        //if( _use_argv  &&  strchr( p, ' ' ) )  goto FERTIG;  
        if( _use_argv ) {
            // Prüfen, ob das nicht doch ein Stellungsparameter ist;  soscopy a "-append b"
            // Option: "-option", "-option=xx", "-option=' '"
            // Stellungsparamter: "-append b"
            const char* r     = p;
            char        quote = 0;
            while( *r ) {
                if( !quote  && ( *r == '\'' || *r == '"' ) )  quote = *r;
                else
                if( *r == quote )  quote = 0;
                else
                if( !quote  &&  *r == ' ' )  goto FERTIG;   // "-append b"  Stellungsparameter!
                else
                if( !quote  &&  *r == '=' )  break;         // "-spooler_descr=ein text"  (Windows entfernt Anführungszeichen)
                r++;
            }
        }

        p++;
        if( *p == p[-1]                            //  "-- " ?  Optionen nicht mehr erkennen
         && ( p[1] == '\0' || sos_isspace( p[1] ) ) )
        {
            p++;
            _no_more_options = true;
            if( _use_argv  &&  *p == '\0' )  _arg_i++;    // sollte bei _argv_used immer so sein!
        }
        else
        {
            while( sos_isalnum( *p )  ||  *p == '-'  ||  *p == '.'  ||  *p == '?'  ||  *p == '#'  ||  *p == '$' ) {
                if( q == _option + sizeof _option - 1  )  {
                    Sos_string ins = as_string( _option, q - _option );
                    ins += "...";
                    throw_xc( "SOS-1300", c_str( ins ) );
                }
                *q++ = *p++;
            }
        }
        if( q > _option  &&  q[-1] == '-' )  {  q--; _set = false; }
                                       else  _set = true;
    }

  FERTIG:
    *q = '\0';
    _ptr = p;
}

//-------------------------------------------------------------Sos_option_iterator::value_debracked

string Sos_option_iterator::value_debracked()
{
    string val = value();
    if( val.length() >= 2  &&  *val.begin() == '('  &&  *val.rbegin() == ')' )  return val.substr( 1, val.length() - 2 );
                                                                          else  return val;
}

//--------------------------------------------------------------Sos_option_iterator::read_value

void Sos_option_iterator::read_value()
{
    const char* p = _ptr;

    _value_read = true;

    //while( sos_isspace( *p ) )  p++;
    _ptr = p;

    if( _use_argv )
    {
        _value = p;
        p += strlen(p);
    }        
    else
    if( *p == '"' || *p == '\'' )
    {
        _buffer.allocate_min( 256 );
        char* q     = _buffer.char_ptr();
        char* q_end = q + _buffer.size();
        char quote = *p++;
        while(1) {
            while( sos_isctype( *p, SOS_IS_ALNUM | SOS_IS_SP ) &&  q < q_end )  *q++ = *p++;
            if( q == q_end )  {
                _buffer.length( q - _buffer.char_ptr() );
                _buffer.resize_min( _buffer.size() + 30000 );
                q     = _buffer.char_ptr() + _buffer.length();
                q_end = _buffer.char_ptr() + _buffer.size();
            }
            if( p[0] == '\\' )  {
                if( p[1] == quote  ||  sos_isspace( p[1] ) )  p++;
                if( !*p )  throw_xc( "SOS-1302", _option );
                else *q++ = *p++;
            }
            else
            if( p[0] == quote ) {
                p++;
                if( p[0] == quote )  *q++ = *p++;
                               else  break;
            }
            else
            if( !*p )  {
                char q[2];q[0] = quote; q[1]=0;
                throw_xc( "SOS-1302", q,  _option );
            }
            else
               *q++ = *p++;
        }
        _buffer.length( q - _buffer.char_ptr() );
        _value = as_string( _buffer );
    }
    else
    {
        int klammern = 0;     // In Klammern dürfen Blanks sein
        while( *p ) {
            if( *p == '(' )  klammern++;
            if( *p == ')' )  klammern--;
            if( !klammern && ( sos_isspace( *p ) || *p == '|' ) )  break;
            p++;
        }
        if( klammern )  throw_xc( "SOS-1302", ")", _option );
        _value = as_string( _ptr, p - _ptr );
    }

    if( *p && !sos_isspace( *p )  &&  *p != '|' )  throw_xc( "SOS-1239", p );

    _ptr = p;
}

//--------------------------------------------------------------------Sos_option_iterator::next

void Sos_option_iterator::next()
{
// js 3.9.: ???  _pipe = false;
    _parameter_start = _ptr;
    _value_read = false;

    if( _use_argv )
    {
        _arg_i++;
        if( _arg_i >= _argc )  { _end = true; return; }
        _ptr = _argv[ _arg_i ];
        read_option();
        if( _option[0]  &&  *_ptr != '\0'  &&  *_ptr != '=' )  throw_xc( "SOS-1308", _option );
    }
    else
    {
        if( !_ptr )  throw_xc( "Sos_option-NULL" );  // Falls dem Konstruktor NULL übergeben worden ist
        while( sos_isspace( *_ptr ) )  _ptr++;
        _rest_ptr = _ptr;
        if( !*_ptr )  { _end = true; return; }

        if( *_ptr == '|' && _ptr[1] != '|' ) {      // |, aber nicht || (wg. SQL)
            _ptr++;
            while( sos_isspace( *_ptr ) )  _ptr++;
            _pipe = true;
            _option[0] = '\0';
            _rest_ptr = _ptr;
        } else {
            read_option();
        }
    }

    // _ptr zeigt auf '=' oder auf den Parameter
}

//------------------------------------------------------------------Sos_option_iterator::pipe()

Bool Sos_option_iterator::pipe()
{
    if( !_pipe && param() )  _pipe = true;   // zur Kompatibilität

    if( _pipe )  _end = true;

    if( _log && _pipe )  LOG( " | " );

    return _pipe;
}

//--------------------------------------------------------------------Sos_option_iterator::flag

Bool Sos_option_iterator::flag( const char* name )
{
    if( ( _ignore_case? stricmp( _option, name ) : strcmp( _option, name ) ) != 0 )  return false;
    if( *_ptr == '=' )  return false;

    Log_ptr log;

    if( _log  &&  log ) {
        *log << '-' << _option;
        if( !_set )  *log << '-';
        *log << '\n';
    }

    return true;
}

//--------------------------------------------------------------------Sos_option_iterator::flag

Bool Sos_option_iterator::flag( char n, const char* name )
{
    if( flag( name ) )  return true;

    char str[2];
    str[0] = n;
    str[1] = '\0';

    return flag( str );
}

//--------------------------------------------------------------Sos_option_iterator::with_value

Bool Sos_option_iterator::with_value( const char* name )
{
    if( ( _ignore_case? stricmp( _option, name ) : strcmp( _option, name )  ) != 0 )  return false;

    if( _use_argv )
    {
        if( *_ptr == '=' )  {                     // [-option=x]
            _ptr++;
            read_value();
        }
        else
        if( _compatible ) {                       // [-option] [x]
            if( _arg_i >= _argc )  throw_xc( "SOS-1301", _option );
            _value = _argv[ ++_arg_i ];
        }
        else return false;
    }
    else
    {
        if( *_ptr == '=' ) {
            _ptr++;
            read_value();
        }
        else
        if( _compatible ) {
            while( *_ptr == ' ' )  _ptr++;
            if( *_ptr == '=' )  _ptr++;
            while( *_ptr == ' ' )  _ptr++;
            read_value();
        }
        else return false;
    }

    return true;
}

//--------------------------------------------------------------Sos_option_iterator::with_value

Bool Sos_option_iterator::with_value( char n, const char* name )
{
    if( with_value( name ) )  return true;

    char str[2];
    str[0] = n;
    str[1] = '\0';

    return with_value( str );
}

//--------------------------------------------------------------Sos_option_iterator::skip_param
// Überspring Programmdateinamen in einer Kommandozeile

void Sos_option_iterator::skip_param()
{
    if( param() )  
    {
        _param_count--;     // Zählung rückgängig machen
        next();
    }
}

//-------------------------------------------------------------------Sos_option_iterator::param

Bool Sos_option_iterator::param( int nr )
{
    if( _option[0] != '\0' )     return false;         // "-option" ?
    if( nr != -1  &&  nr != _param_count + 1 ) return false;

    if( _max_params >= 0 && _param_count == _max_params )  throw_xc( "SOS-1305", _param_count );
    _param_count++;

    if( _use_argv )  _value = _argv[ _arg_i ];
               else  read_value();

    return true;
}

//------------------------------------------------------Sos_option_iterator::complete_parameter

string Sos_option_iterator::complete_parameter( char quote, char quote_quote )
{
    // quote_quote == '\\' => Unix
    // quote_quote == '"'  => Windows

    string opt;
    string val;

    if( _value_read )  opt = string("-") + option() + "=", val = _value;
    else 
    if( with_value( _option ) )  opt = string("-") + option() + "=", val = value();
    else 
    if( _option[0]  &&  flag( _option ) )  opt = string("-") + option() + ( set()? "" : "-" );
    else
    if( param() )  val = value();
    else
        throw_sos_option_error( *this );

    // Das kann hier noch verfeinert werden. Bisher sind nicht alle Zeichen immer möglich. Aber normale Dateinamen gehen. jz 16.2.01
    string dirty_chars = " <>|()\t\n";          // Shell-Meta-Zeichen
    if( quote == '\'' )  dirty_chars += "`";    // Backtick 
    dirty_chars += quote;
    dirty_chars += quote_quote;

    if( val.find_first_of(dirty_chars) != string::npos )  val = quoted_string( val, quote, quote_quote );

    return opt + val;
}

//-----------------------------------------------------------Sos_option_iterator::is_sos_option

Bool Sos_option_iterator::is_sos_option()
{
    return false;
}

//-------------------------------------------------------Sos_option_iterator::handle_sos_option

void Sos_option_iterator::handle_sos_option()
{
}

//--------------------------------------------------------------------Sos_option_iterator::rest

Sos_string Sos_option_iterator::rest()
{
    ZERO_RETURN_VALUE( Sos_string );

    if( _use_argv )
    {
        Sos_string rest;
        for( int i = _arg_i; i < _argc; i++ )
        {
            if( i > _arg_i )  rest += ' ';
            const char* a = _argv[ i ];

            if( strchr( a, ' ' ) ) {       // Blank im Parameter (?)
                rest += '"';                  // In Anführungszeichen setzen
                while( *a ) {
                    if( *a == '"' )  a += '"';
                    rest += *a++;
                }
                rest += '"';
            } else {
                rest += a;
            }
        }
        return rest;
    }
    else {
        if( !_rest_ptr )  return "";   // Falls keine Parameter angegeben sind
        const char* q = _rest_ptr + strlen( _rest_ptr );
        while( q > _rest_ptr  &&  sos_isspace( q[-1] ) )  q--;
        _ptr += strlen( _ptr );
        Sos_string result = as_string( _rest_ptr, q - _rest_ptr );
//LOG("Sos_option_iterator::rest() = " << result << '\n' );
        return result;
    }
}

//---------------------------------------------------------------------Sos_option_iterator::pos

Source_pos Sos_option_iterator::pos() const
{
    if( _use_argv ) {
        int col = 0;
        for( int i = 0; i < _arg_i; i++ )  col += length( _argv[ i ] ) + ( i > 0 );
        return col;
    } else {
        Source_pos  pos;
        if( !_ptr )
        {
            const char* p = c_str( _string );

            while( p < _ptr ) {
                if( *p == '\n' )  { pos._line++; pos._col = 0; }
                            else  pos._col++;
                p++;
            }
            if( pos._line >= 0 )  pos._line++;
        }

        return pos;
    }
}

//-------------------------------------------------------------Sos_option_iterator::log_option

void Sos_option_iterator::log_option()
{ 
    if( _log )  LOG( '-' << _option << '=' );
}

//------------------------------------------------------------------Sos_option_iterator::value

const Sos_string& Sos_option_iterator::value()
{ 
    if( _log )  { log_option(); LOG( _value << '\n' );  }
    return _value;
}

//------------------------------------------------------------------Sos_option_iterator::as_int

int Sos_option_iterator::as_int()
{ 
    try {
        int result = ::sos::as_int( c_str( _value ) ); 
        if( _log )  { log_option(); LOG( result << '\n' );  }
        return result;
    }
    catch( const Xc& )
    {
        throw_xc( "SOS-1328", c_str( _option ), c_str( _value ) );
        return 0;
    }
}

//------------------------------------------------------------------Sos_option_iterator::as_int

uint Sos_option_iterator::as_uint()
{ 
    return as_uint4();
}

//----------------------------------------------------------------Sos_option_iterator::as_uintK

uint Sos_option_iterator::as_uintK()
{ 
    try {
        uint result = ::sos::as_uintK( c_str( _value ) ); 
        if( _log )  { log_option(); LOG( result << '\n' );  }
        return result;
    }
    catch( const Xc& )
    {
        throw_xc( "SOS-1329", c_str( _option ), c_str( _value ) );
        return 0;
    }
}

//----------------------------------------------------------------Sos_option_iterator::as_uint4

uint4 Sos_option_iterator::as_uint4()
{
    try {
        uint result = ::sos::as_uint4( c_str( _value ) );
        if( _log )  { log_option(); LOG( result << '\n' );  }
        return result;
    }
    catch( const Xc& )
    {
        throw_xc( "SOS-1329", c_str( _option ), c_str( _value ) );
        return 0;
    }
}

//---------------------------------------------------------------Sos_option_iterator::as_double

double Sos_option_iterator::as_double()
{ 
    try {
        double result = ::sos::as_double( c_str( _value ) ); 
        if( _log )  { log_option(); LOG( result << '\n' );  }
        return result;
    }
    catch( const Xc& )
    {
        throw_xc( "SOS-1420", c_str( _option ), c_str( _value ) );
        return 0.0;
    }
}

//-----------------------------------------------------------------Sos_option_iterator::as_char

char Sos_option_iterator::as_char()
{
    if( length( value() ) != 1 )  throw_xc( "SOS-1327", c_str( _option ), c_str( _value ) );

    char result = _value[ 0 ];
    if( _log )  { log_option(); LOG( result << '\n' );  }
    return result;
}

//-----------------------------------------------------------Sos_option_error::Sos_option_error

Sos_option_error::Sos_option_error( const Sos_option_iterator& opt )
:
    Xc( "SOS-1300" )
{
    pos( opt.pos() );
    insert( opt._option );
}

//-----------------------------------------------------------------------throw_sos_option_error

void throw_sos_option_error( const Sos_option_iterator& it )
{
  //Log_block qq ( "throw_sos_option_error" );
    throw Sos_option_error( it );
}


//=============================================================================================


// ------------------------------------------------------------------------- Sos_token_iterator::

void Sos_token_iterator::_init()
{
    _eof            = false;
    _istream_ptr    = 0;
    _value          = "";
    _first          = true;
}

Sos_token_iterator::Sos_token_iterator( const Const_area& area, const char sep )
:
    _sep(sep)
{
    _init();
    _istream_ptr = new istrstream( (char*)area.char_ptr(), (int)area.length() );
        if ( !_istream_ptr ) throw_no_memory_error();
    next();
}

Sos_token_iterator::Sos_token_iterator( const char* text, const char sep )
:
    _sep(sep)
{
    _init();
    _istream_ptr = new istrstream( (char*)text, strlen( text ) );
        if ( !_istream_ptr ) throw_no_memory_error();
    next();
}

Sos_token_iterator::Sos_token_iterator( const Sos_string& str, const char sep )
:
    _sep(sep)
{
    _init();
    _istream_ptr = new istrstream( (char*)c_str(str), length( str ) );
        if ( !_istream_ptr ) throw_no_memory_error();
    next();
}

Sos_token_iterator::~Sos_token_iterator()
{
    SOS_DELETE(_istream_ptr);
}

void Sos_token_iterator::next()
{
    if ( _istream_ptr->eof() ) { _eof = true; _value = ""; return; }

    Dynamic_area buffer;
    char*                 p     = buffer.char_ptr();
    char*                 p_end = p + buffer.size();
    int c;

    buffer.allocate_min(1024);
    while(1)
    {
            if ( p == p_end )  break;
            if ( _istream_ptr->peek() == _sep ) { c = _istream_ptr->get(); break; }
            c = _istream_ptr->get();
            if ( c == EOF )  break;
            *p++ = c;
    }

    if( p == p_end )  throw_too_long_error();
    if( !_istream_ptr->eof() && _istream_ptr->fail() )   throw_xc( "SOS-1169" );

    buffer.length( p - buffer.char_ptr() );
    _value = buffer.char_ptr();
}


} //namespace sos
