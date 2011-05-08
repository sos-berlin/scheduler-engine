// $Id$

#include "precomp.h"
//                                                      (c) Joacim Zschimmer

#include "sosstrng.h"               // zieht windows.h ein (MSC++)

#if defined __WIN32__
#   include <windows.h>              // GetModuleFileName
#endif

#if defined SYSTEM_UNIX
#   include <limits.h> // wg. PATH_MAX -> sollte eigentlich in sos.h rein !
#endif

#include "sysxcept.h"

#include "sosstrg0.h"         // empty()
#include "sos.h"
#include "log.h"
#include "sosstat.h"
#include "soslimtx.h"
#include "sosdate.h"
#include "soserror.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>                  // für throw_errno()
#include <ctype.h>
#include <time.h>

#include "sysdep.h"

using namespace std;
namespace sos {


const Source_pos std_source_pos;

//-------------------------------------------------------------------------------Msg_code::Msg_code

Msg_code::Msg_code( const char* error_code )
{
    int length = min( strlen( error_code ), sizeof _error_code - 1 );
    memcpy( _error_code, error_code, length );
    _error_code[ length ] = 0;
}

//-------------------------------------------------------------------------------Msg_code::Msg_code

Msg_code::Msg_code( const char* code_prefix, const char* code_suffix )
{
    int prefix_length = min( strlen( code_prefix ), sizeof _error_code - 1 );
    memcpy( _error_code, code_prefix, prefix_length );

    int suffix_length = min( strlen( code_suffix ), sizeof _error_code - 1 - prefix_length );
    memcpy( _error_code + prefix_length, code_suffix, suffix_length );
    _error_code[ prefix_length + suffix_length ] = 0;
}

//-------------------------------------------------------------------------------Msg_code::Msg_code

Msg_code::Msg_code( const char* code_prefix, int numeric_suffix, int suffix_width )
{
    if( strchr( code_prefix, '%' ) )
    {
        char error_code [ 100 ];
        sprintf( error_code, code_prefix, numeric_suffix );
        strncpy( _error_code, error_code, sizeof _error_code );
        _error_code[ sizeof _error_code - 1 ] = '\0';
    }
    else
    {
        ostrstream s ( _error_code, sizeof _error_code - 1 );
        s << code_prefix
          << setw( suffix_width ) << setfill( '0' ) << numeric_suffix;
        _error_code[ s.pcount() ] = 0;
    }
}

//-------------------------------------------------------------------------Msg_code::numeric_suffix

int Msg_code::numeric_suffix() const
{
    const char* p = _error_code + strlen( _error_code ) - 1;
    while( p > _error_code  &&  *p != '-' )  p--;
    return atoi( p + 1 );
}

//-------------------------------------------------------------------Msg_insertions::Msg_insertions

Msg_insertions::Msg_insertions() THROW_NONE
{
    _init();
}

//-------------------------------------------------------------------Msg_insertions::Msg_insertions

Msg_insertions::Msg_insertions( const char* text ) THROW_NONE
{
    _init();

    assign( 0, text );
}

//-------------------------------------------------------------------Msg_insertions::Msg_insertions

Msg_insertions::Msg_insertions( const char* text_1, const char* text_2, const char* text_3 ) THROW_NONE
{
    _init();

    assign( 0, text_1 );
    assign( 1, text_2 );
    assign( 2, text_3 );
}

//-------------------------------------------------------------------Msg_insertions::Msg_insertions

Msg_insertions::Msg_insertions( const Sos_object_base* o1, const char* text_2, const char* text_3 )
THROW_NONE
{
    _init();

    assign( 0, o1     );
    assign( 1, text_2 );
    assign( 2, text_3 );
}

//-------------------------------------------------------------------Msg_insertions::Msg_insertions

Msg_insertions::Msg_insertions( const Sos_object_base* o1, const Sos_object_base* o2, const char* text_3 )
THROW_NONE
{
    _init();

    assign( 0, o1     );
    assign( 1, o2     );
    assign( 2, text_3 );
}

//-------------------------------------------------------------------Msg_insertions::Msg_insertions

Msg_insertions::Msg_insertions( const Msg_insertions& m )
THROW_NONE
{
    _init();

    for( int i = 0; i < NO_OF( _insertion_array ); i++ )  _insertion_array[i] = m._insertion_array[i];
}

//-----------------------------------------------------------------------Msg_insertions::operator =

Msg_insertions& Msg_insertions::operator= ( const Msg_insertions& m )
THROW_NONE
{
    //LOG( "Msg_insertions::operator=(" << m << ");\n"); //test

    for( int i = 0; i < NO_OF( _insertion_array ); i++ )  _insertion_array[i] = m._insertion_array[i];

    return *this;
}

//------------------------------------------------------------------------Msg_insertions::_init

void Msg_insertions::_init()
THROW_NONE
{
    for( int i = 0; i < NO_OF( _insertion_array ); i++ )  _insertion_array[ i ] = "";
}

//------------------------------------------------------------------Msg_insertions::~Msg_insertions

Msg_insertions::~Msg_insertions()
THROW_NONE
{
}

//---------------------------------------------------------------------------Msg_insertions::assign

void Msg_insertions::assign( int i, const char* text, int len, Xc_base* x )
THROW_NONE
{
  //if( len )  LOG( "[xc.insert \"" << Const_area( text, len ) << "\"]\n" );
    assign_nolog( i, text, len );   //text? strlen( text ) : 0 );

    if( !_insertion_array[i].empty()  &&  ( !x ||  log_category_is_set( "exception." + z::lcase( x->code() ) ) ) )
        LOG( "[xc.insert \"" << _insertion_array[i] << "\"]\n" );
}

//---------------------------------------------------------------------Msg_insertions::assign_nolog

void Msg_insertions::assign_nolog( int i, const char* text_par, int len_par, Xc_base* x )
THROW_NONE
{
    string text = zschimmer::remove_password( make_string( text_par, len_par ) );
    while( text.length() >= 1  && ( *text.rbegin() == '\n' || *text.rbegin() == '\r' ) )  text.erase( text.length()-1 );
    _insertion_array[i] = text;
}

//-----------------------------------------------------------------------Msg_insertions::assign

void Msg_insertions::assign( int i, const char* text, Xc_base* x )
THROW_NONE
{
    assign( i, text, text? strlen( text ) : 0, x );
}

//-----------------------------------------------------------------------Msg_insertions::assign

void Msg_insertions::assign( int i, const Sos_object_base* o, Xc_base* x )
THROW_NONE
{
    char text [ 80+1 ];

    if( o ) {
        ostrstream s ( text, sizeof text );
        s << *o;

        if( s.pcount() <= sizeof text )  text[ s.pcount() ] = '\0';
                                   else  memcpy( text + sizeof text - 4, "...", 4 );
    } else {
        strcpy( text, "NULL-Object" );
    }

    assign( i, text, x );
}

//-----------------------------------------------------------------------Msg_insertions::append

void Msg_insertions::append( const char* text, int len, Xc_base* x )
THROW_NONE
{
	int i;
    for( i = 0; i < NO_OF( _insertion_array ); i++ )  if( _insertion_array[i].empty() )  break;

    if( i < NO_OF( _insertion_array ) ) {
        assign( i, text, len, x );
    }
}

//-----------------------------------------------------------------------Msg_insertions::append

void Msg_insertions::append( const Const_area& area, Xc_base* x )
THROW_NONE
{
    append( area.char_ptr(), area.length(), x );
}

//-----------------------------------------------------------------------Msg_insertions::append

void Msg_insertions::append( const Sos_string& s, Xc_base* x )
THROW_NONE
{
    append( c_str( s ), length( s ), x );
}

//-------------------------------------------------------------------Msg_insertions::append_hex

void Msg_insertions::append_hex( const void* text, int len, Xc_base* x )
THROW_NONE
{
#define MAX_HEX_CNT 16
    char h [ MAX_HEX_CNT*2 + 3 + 1 ];
    memset( h, 0, sizeof h );
    ostrstream s ( h, sizeof h );
    s << hex << Const_area( text, len > MAX_HEX_CNT ? MAX_HEX_CNT : len ) << dec;
    //jz 9.12.97 Das liefert drei Schmierzeichen: if ( len > MAX_HEX_CNT ) s << "...";
    if( len > MAX_HEX_CNT ) {
        memcpy( h + MAX_HEX_CNT*2, "...", 3 );
    }
/*
    s << hex << setw(2) << setfill( '0' );
    s.setf( ios::uppercase );
    for( int i = 0; i < len; i++ ) {
        if( i % 4 == 0  &&  i > 0 )  s << ' ';
        s << (uint)((Byte*)text)[ i ];
    }
*/
    //jzappend( h, s.pcount() );
    append( h, x );
#undef MAX_HEX_CNT  // Ob Jörg schon von was von "const" gehört hat? jz
}

//-----------------------------------------------------------------------Msg_insertions::append

void Msg_insertions::append( const char* text, Xc_base* x )
THROW_NONE
{
    if( text )  append( text, strlen( text ), x );
}

//-----------------------------------------------------------------------Msg_insertions::append

void Msg_insertions::append( const Sos_object_base* o, Xc_base* x )
THROW_NONE
{
    char buffer [ 250+1 ];

    try {
        ostrstream s ( buffer, sizeof buffer );
        s << *o << '\0';
    }
    catch( const exception& ) {}

    memcpy( buffer + sizeof buffer - 4, "...", 4 );
    append( buffer, x );
}

//-----------------------------------------------------------------------Msg_insertions::append

void Msg_insertions::append( int4 value, Xc_base* x )
THROW_NONE
{
    char buffer [ 20];
    ostrstream( buffer, sizeof buffer ) << value << '\0';
    append( buffer, x );
}

//-------------------------------------------------------------------Msg_insertions::_obj_print

void Msg_insertions::_obj_print( ostream* s ) const
THROW_NONE
{
    for( int i = 0; i < NO_OF( _insertion_array ); i++ )
    {
        if( i > 0 )  *s << ", ";
        *s << '[' << _insertion_array[ i ] << ']';
    }
}

//---------------------------------------------------------------------------------------------

ostream& operator<< ( ostream& s, const Source_pos& pos )
THROW_NONE
{
    Bool first = true;

    if( !empty( pos.filename() ) ) {
        s << pos.filename();
        first = false;
    }

    if( pos._line != -1 ) {
        if( !first ) s << ", ";
        s << "line " << ( pos._line + 1 );
        first = false;
    }

    if( pos._col  != -1 ) {
        if( !first ) s << ", ";
        s << "column " << ( pos._col + 1 );
    }

    return s;
}

//-----------------------------------------------------------------------------Xc_base::Xc_base

Xc_base::Xc_base( const char* error_code )
THROW_NONE
:
    _error_code( error_code )
{
    _what = "";
    _name[ 0 ] = '\0';

    log_error();
}

//-----------------------------------------------------------------------------Xc_base::Xc_base

Xc_base::Xc_base( const char* error_code, const Msg_insertions& insertions )
THROW_NONE
:
    _error_code( error_code ),
    _insertions ( insertions )
{
    _what = "";
    _name[ 0 ] = '\0';

    log_error();
}

//-----------------------------------------------------------------------------Xc_base::Xc_base

Xc_base::Xc_base( const Xc_base& x )
THROW_NONE
{
    assign( x );
}

//----------------------------------------------------------------------------Xc_base::~Xc_base
#ifdef SYSTEM_GNU

Xc_base::~Xc_base() throw()
{
    // Damit es polymorph wird
}

#endif

//-------------------------------------------------------------------------------Xc_base::log_error

void Xc_base::log_error()
{
    // _name ist hier noch nicht bekannt.

    if( log_category_is_set( "exception." + string( _error_code ) ) )
    {
        Log_ptr log;
        if( log )
        {
            *log << "[ERROR ";
            get_text( log );
            *log << "]\n";
        }
    }
}

//----------------------------------------------------------------------------------Xc_base::assign

void Xc_base::assign( const Xc_base& x ) throw()
{
    _pos = x._pos;
    memcpy( _name, x._name, sizeof _name );
    _error_code = x._error_code;
    _insertions = x._insertions;
    _what = x._what;
}

//----------------------------------------------------------------------------Xc_base::set_code

void Xc_base::set_code( const char* code )
{
    _error_code = code;
}

//--------------------------------------------------------------------------------Xc_base::name

void Xc_base::name( const char* n ) throw()
{
    int len = min( strlen( n ), sizeof _name - 1 );
    memcpy( _name, n, len );
    _name[ len ] = '\0';
}

//------------------------------------------------------------------------------Xc_base::insert

Xc_base& Xc_base::insert( const Const_area& a ) throw()
{
    _insertions.append( a, this );
    return *this;
}

//----------------------------------------------------------------------------Xc_base::set_name

void Xc_base::set_name( const char* n )
{
    strncpy( _name, n , sizeof _name - 1 );
    _name[ sizeof _name - 1 ] = '\0';
}

//--------------------------------------------------------------------------Xc_base::_obj_print

void Xc_base::_obj_print( ostream* s ) const
{
#   if defined SYSTEM_RTTI
        const char* type_name = typeid( *this ).name();
        if( memcmp( type_name, "const ", 6 ) == 0 )  type_name += 6;
      //if( memcmp( type_name, "struct sos::", 12 ) == 0 )  type_name += 12;
        //*s << type_name << "( ";
#       if defined __BORLANDC__
            if( strncmp( type_name, "sos::", 5 ) != 0 )
#       elif defined SYSTEM_MICROSOFT
            if( strncmp( type_name, "struct sos::", 12 ) != 0 )
#       elif defined SYSTEM_GNU
            if( strncmp( type_name, "N3sos2XcE", 9 ) != 0 )
#       endif
        *s << type_name << ": ";
#   else
        //*s << "Xc(";
#   endif

    print_text( *s );
    //*s << ')'; //"in " << _constructions_module_name;

#   if 0 /* defined( __BORLANDC__ ) */
        if( __throwFileName  &&  __throwFileName[ 0 ] ) {
            const char* f = __throwFileName + strlen( __throwFileName ) - 1;
            while( f >= __throwFileName  &&  *f != '/'  &&  *f != '\\'  &&  *f != ':' )  f--;
            *s << " [" << (f+1) << '/' << __throwLineNumber << ']';
        }
#   endif
}

//--------------------------------------------------------------------------Xc_base::print_text

void Xc_base::print_text( ostream& s ) const
THROW_NONE
{
    //Bool text_found =
    get_text( &s );
    //if( !text_found  &&  _insertions[ 0 ] ) s  << ' ' << _insertions;
}

//----------------------------------------------------------------------------Xc_base::get_text

Bool Xc_base::get_text( Area* area_ptr ) const
THROW_NONE
{
    if( area_ptr->resizable() ) {
        try {
            area_ptr->allocate_min( 1000 );
        }
        catch( const Xc& ) {}
    }

    if( area_ptr->size() == 0 )  return false;

    ostrstream s ( area_ptr->char_ptr(), area_ptr->size() );

    Bool erg = get_text( &s );
    int  len = min( (int)s.pcount(), (int)area_ptr->size() - 1 );
    area_ptr->char_ptr()[ len ] = '\0';
    area_ptr->length( len + 1 );
    return erg;
}

//--------------------------------------------------------------------------------Xc_base::what
/*
Sos_string Xc_base::what() const
{
    Dynamic_area text;
    get_text( &text );
    return as_string( text );
}
*/
//--------------------------------------------------------------------------------Xc_base::what

const char* Xc_base::what() const throw()
{
    if( _what.empty() )
    {
        Dynamic_area text;
        get_text( &text );
        ((Xc_base*)this)->_what = as_string( text );
    }

    return _what.c_str();
}

//--------------------------------------------------------------------------------------get_hex
//ALLGEMEINE ROUTINE!!!

inline int1 get_hex( char hex_char )
{
    int1 n;

    if ( hex_char >= '0' && hex_char <= '9' ) {
        n = hex_char - '0';
    } else if ( hex_char >= 'A' && hex_char <= 'F' ) {
        n = hex_char - 'A' + 10;
    } else if ( hex_char >= 'a' && hex_char <= 'f' ) {
        n = hex_char - 'a' + 10;
    } else {
        char str [ 2 ];
        str[0] = hex_char;
        str[1] = '\0';
        throw_xc( "SOS-1392", str );
    }

    return n;
}

//----------------------------------------------------------------------------Xc_base::get_text

Bool Xc_base::get_text( ostream* s ) const
THROW_NONE
{
    const char* text;

    if( !_what.empty() )  // jz 15.3.2003
    {
        *s << _what;
        return false;  //?
    }

    Sos_limited_text<32> error_code = code();
    Bool found = false;
    uint set   = ~0u << max_msg_insertions;        // nicht ausgegebene Einfügungen sind 0-Bits
    const Soserror_text* t = NULL;

    if( strncmp( code(), "ERRNO-", 6 ) == 0 )
    {
        const char* e = strerror( atoi( code() + 6 ) );// << '\0';
        if( e ) {
            *s << code();                         // Fehlercode einstellen
            *s << "  ";
            int l = strlen( e );
            if( l > 0 && e[ l-1 ] == '\n' )  l--;
            s->write( e, l );
            found = true;
            goto ENDE;
        }
    }

    if( strncmp( code(), "MSWIN-", 6 ) == 0
     || strncmp( code(), "OLE-"  , 4 ) == 0
     || strncmp( code(), "COM-"  , 4 ) == 0 )
    {
        Sos_limited_text<500> text;
        ulong error = 0;
        const char* p = strchr(code(),'-')+1;
        while( isxdigit( *p ) )  error = ( error << 4 ) + get_hex( *p++ );

        get_mswin_msg_text( &text, error );
        if( text.length() > 0 )
        {
            *s << code();
            *s << "  ";
            *s << text;
            goto ENDE;
        }
    }

    if( error_code[ 0 ] == 'D'  &&  error_code[ 1 ] == '0'  &&  error_code[ 4 ] == ' ' ) {  // Rapid?
        error_code[ 1 ] = '1';
    }


    c_str( error_code );
    t = soserror_texts;
    while( t->_code  &&  strcmp( t->_code, error_code.char_ptr() ) != 0 )  t++;


    *s << code();                         // Fehlercode einstellen
    *s << "  ";


    text = t->_text;
    if( !text )
    {
        text = zschimmer::get_error_text( error_code.char_ptr() );
    }

    if( text  &&  text[0] )
    {
        const char* p     = text;
        const char* p_end = p + strlen(p);

        while( p < p_end ) {
            if( *p == '$'  &&  p+1 < p_end ) {
                uint n = *(p+1) - '1';
                if( n < max_msg_insertions ) {
                    set |= 1 << n;
                    const char* ptr = _insertions[ n ];
                    if( ptr ) {
                        *s << ptr;
                        p += 2;
                        continue;
                    }
                }
            }
            *s << *p++;
        }
                                            // Nicht benutzte Einfügungen der Meldung anhängen:
    }

ENDE:
    if( set != ~0u ) {
        uint b = 1;
        for( int i = 0; i < max_msg_insertions; i++ ) {
            if( !( set & b )  &&  _insertions[ i ] &&  _insertions[ i ][ 0 ] ) {
                *s << " [" << _insertions[ i ] << ']';
            }
            b <<= 1;
        }
    }

    found = true;

    if( !_pos.empty() ) *s << " in " << _pos;

    if( !found  &&  _insertions[ 0 ] ) *s  << ' ' << _insertions;
    return found;
}

//---------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const char* error_code )
THROW_NONE
:
    Xc_base ( error_code )
{
}

//---------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const Msg_code& error_code )
THROW_NONE
:
    Xc_base ( error_code )
{
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const Msg_code& error_code, const Msg_insertions& insertions )
THROW_NONE
:
    Xc_base ( error_code, insertions )
{
}

//---------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const Msg_code& error_code, const Msg_insertions& insertions, const Source_pos& pos )
THROW_NONE
:
    Xc_base ( error_code, insertions )
{
    _pos = pos;
}

//---------------------------------------------------------------------------------------Xc::Xc
/*
Xc::Xc( const Msg_code& error_code, const Source_pos& pos )
:
    Xc_base( error_code )
{
    _pos = pos;
}
*/
//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const exception& x )
THROW_NONE
{

#ifndef DYNAMIC_CAST_CRASHES	
    if( dynamic_cast< const zschimmer::Xc* >( &x ) )
#else
    if( z::name_of_type( x ) == "zschimmer::Xc" )
#endif
    {
        const zschimmer::Xc* z = (const zschimmer::Xc*)&x;
        _error_code = z->code().c_str();
        _what = z->what();

        log_error();
    }
    else

#ifndef DYNAMIC_CAST_CRASHES	
    if( dynamic_cast< const Xc* >( &x ) )
#else
    if( z::name_of_type( x ) == "sos::Xc" )
#endif
    {
        assign( *(const Xc*)&x );
    }

    else
    {
        _error_code = exception_name( x ).c_str();
        _what = x.what();
    }
}

//-------------------------------------------------------------------------------------------Xc::Xc
/*
Xc::Xc( const zschimmer::Xc& x ) throw()
:
    Xc_base( x.code().c_str() )
{
    _what = x.what();
}
*/
//---------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const Xc& x )
THROW_NONE
:
    Xc_base ( x )
{
    //LOG( "Xc::Xc(" << x << ")\n"); //test
}

//---------------------------------------------------------------------------------------------

Xc::~Xc()
THROW_NONE
{
}

//---------------------------------------------------------------------------------Xc_copy::set

void Xc_copy::set( const Xc* x )
{
    delete _xc;
    _xc = NULL;
    _time = 0;

    if( x )  set( *x );
}

//---------------------------------------------------------------------------------Xc_copy::set

void Xc_copy::set( const Xc& x )
{
    delete _xc;

    _time = (double)::time(NULL);

    _xc = new Xc(x);
}

//---------------------------------------------------------------------------------Xc_copy::set

void Xc_copy::set( const exception* x )
{
    delete _xc;
    _xc = NULL;
    _time = 0;

    if( x )  set( *x );
}

//---------------------------------------------------------------------------------Xc_copy::set

void Xc_copy::set( const exception& x )
{
    delete _xc;

    _time = (double)::time(NULL);

    _xc = new Xc(x);
}

//-------------------------------------------------------------------------operator<<( Xc_copy )

ostream& operator<<( ostream& s, const Xc_copy& x )
{
    if( x._xc )
    {
        char  buff [30];

        //char* old_locale = setlocale( LC_NUMERIC, "C" );
        char* bruch = buff + sprintf( buff, "%0.3lf", x._time ) - 4;
        //setlocale( LC_NUMERIC, old_locale );

        s << Sos_optional_date_time( uint(x._time) );
        s << bruch;
        s << ' ';
        s << *x._xc;
    }
    else
    {
        s << "Kein Fehler";
    }

    return s;
}

//------------------------------------------------------------------------------No_memory_error

No_memory_error::No_memory_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "NOMEMORY" );
}

//------------------------------------------------------------------------------------Eof_error

Eof_error::Eof_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "EOF" );
}

//------------------------------------------------------------------------------Not_exist_error

Not_exist_error::Not_exist_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "NOTEXIST" );
}

//----------------------------------------------------------------------------------Exist_error

Exist_error::Exist_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "EXIST" );
}

//------------------------------------------------------------------------------Not_found_error

Not_found_error::Not_found_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "NOFIND" );
}

//------------------------------------------------------------------------------Duplicate_error

Duplicate_error::Duplicate_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "DUPLICAT" );
}

//-------------------------------------------------------------------------------Too_long_error

Too_long_error::Too_long_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "TRUNCATE" );
}

//-------------------------------------------------------------------------------No_space_error

No_space_error::No_space_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "NOSPACE" );
}

//-----------------------------------------------------------------------------Wrong_type_error

Wrong_type_error::Wrong_type_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "WRONGTYP" );
}

//------------------------------------------------------------------------Connection_lost_error

Connection_lost_error::Connection_lost_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "CONNLOST" );
}

//-----------------------------------------------------------------------------------Data_error

Data_error::Data_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "DATA" );
}

//---------------------------------------------------------------------------------Locked_error

Locked_error::Locked_error( const Msg_code& e )
THROW_NONE
:
    Xc( e )
{
    name( "LOCKED" );
}

//----------------------------------------------------------------------------------Abort_error

Abort_error::Abort_error()
THROW_NONE
:
    Xc_base( "SOS-1002" )
{
}

//-------------------------------------------------------------------------------Overflow_error

Overflow_error::Overflow_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "OVERFLOW" );
}

//-----------------------------------------------------------------------------------Null_error

Null_error::Null_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "NULL" );
}

//------------------------------------------------------------------------------Conversion_error

Conversion_error::Conversion_error( const char* e )
THROW_NONE
:
    Xc( e )
{
    name( "CONV" );
}

//-----------------------------------------------------------------------------------Usage_error

Usage_error::Usage_error( const char* usage, const char* e )
THROW_NONE
:
    Xc( e )
{
    int len = MIN( strlen( usage ), sizeof _usage - 1 );
    memcpy( _usage, usage, len );
    _usage[len] = '\0';
    insert( usage );
    name( "USAGE" );
}


//----------------------------------------------------------------------------throw_right_typed
#if 1
void Xc::throw_right_typed() const
{
    //int INSERTIONS_WERDEN_NICHT_UEBERNOMMEN;        // operator= noch nicht implementiert.

    if( strcmp( _name, "EOF"      ) == 0 )  throw_eof_error( code() );
    if( strcmp( _name, "NOFIND"   ) == 0 )  throw_not_found_error( code() );
    if( strcmp( _name, "NOMEMORY" ) == 0 )  throw_no_memory_error( code() );
    if( strcmp( _name, "NOTEXIST" ) == 0 )  throw_not_exist_error( code() );
    if( strcmp( _name, "EXIST"    ) == 0 )  throw_exist_error( code() );
    if( strcmp( _name, "DUPLICAT" ) == 0 )  throw_duplicate_error( code() );
    if( strcmp( _name, "TRUNCATE" ) == 0 )  throw_too_long_error( code() );
    if( strcmp( _name, "NOSPACE"  ) == 0 )  throw_no_space_error( code() );
    if( strcmp( _name, "WRONGTYP" ) == 0 )  throw_wrong_type_error( code() );
    if( strcmp( _name, "CONNLOST" ) == 0 )  throw_connection_lost_error( code() );
    if( strcmp( _name, "DATA"     ) == 0 )  throw_data_error( code() );
    if( strcmp( _name, "CONV"     ) == 0 )  throw_conversion_error( code() );
    if( strcmp( _name, "SYNTAX"   ) == 0 )  throw_syntax_error( code() );
    if( strcmp( _name, "OVERFLOW" ) == 0 )  throw_overflow_error( code() );
    if( strcmp( _name, "LOCKED" ) == 0 )  throw Locked_error( code() );
    throw *this;
}

#else
#define THROW_RIGHT_TYPED( NAME_STRING, NAME )                                              \
    if( strcmp( _name, NAME_STRING ) == 0 )                                                 \
    {                                                                                       \
        NAME z ( _error_code );                                                             \
      /*z._insertions = _insertions;*/                                                      \
        throw z;                                                                            \
      throw_##NAME()                                                                        \
    }

void Xc::throw_right_typed() const
{
    int INSERTIONS_WERDEN_NICHT_UEBERNOMMEN;        // operator= noch nicht implementiert.

    THROW_RIGHT_TYPED( "EOF"     , eof_error        )
    THROW_RIGHT_TYPED( "NOFIND"  , not_found_error  )
    THROW_RIGHT_TYPED( "NOMEMORY", no_memory_error  )
    THROW_RIGHT_TYPED( "NOTEXIST", not_exist_error  )
    THROW_RIGHT_TYPED( "EXIST"   , exist_error      )
    THROW_RIGHT_TYPED( "DUPLICAT", duplicate_error  )
    THROW_RIGHT_TYPED( "TRUNCATE", too_long_error   )
    THROW_RIGHT_TYPED( "NOSPACE" , no_space_error   )
    THROW_RIGHT_TYPED( "WRONGTYP", wrong_type_error )
    THROW_RIGHT_TYPED( "CONNLOST", connection_lost_error )
    THROW_RIGHT_TYPED( "DATA"    , data_error            )
    THROW_RIGHT_TYPED( "SYNTAX"  , syntax_error          )
    THROW_RIGHT_TYPED( "OVERFLOW", overflow_error        )
    THROW_RIGHT_TYPED( "LOCKED"  , locked_error        )
    throw *this;
}
#endif

/*
void throw_xc( Exception_code, const char* error_code );
void throw_xc( Exception_code, const char* error_code );
void throw_xc( Exception_code, const char* error_code, const char* insertion );
void throw_xc( Exception_code, const char* error_code, const Const_area& insertion );
void throw_xc( Exception_code, const char* error_code, const Sos_object_base* );
void throw_xc( Exception_code, const char* error_code, const Msg_insertions& );
void throw_xc( Exception_code, const char* error_code, const Msg_insertions&, const Source_pos& );
*/

void throw_abort_error           ()                { throw Abort_error(); }
void throw_connection_lost_error ( const char* e ) { throw Connection_lost_error( e ); }
void throw_data_error            ( const char* e ) { throw Data_error           ( e ); }
void throw_duplicate_error       ( const char* e ) { throw Duplicate_error      ( e ); }
void throw_eof_error             ( const char* e ) { throw Eof_error            ( e ); }
void throw_eof_error             ( const char* e, const Sos_object_base* o ) { Eof_error x ( e ); x.insert( o ); throw x; }
void throw_eof_error             ( const char* e, int o ) { Eof_error x ( e ); x.insert( o ); throw x; }
void throw_errno                 ( int e, const char* func ) { throw_errno( e, Msg_insertions( func ) ); }
void throw_errno                 ( int e, const Msg_insertions& );
void throw_errno                 ( int e, const char* func , const char* t ) { throw_errno( e, Msg_insertions( func, t ) ); }
void throw_exist_error           ( const char* e ) { throw Exist_error          ( e ); }
void throw_no_memory_error       ( const char* e ) { throw No_memory_error      ( e ); }
void throw_no_memory_error       ( unsigned long size )  { No_memory_error x; x.insert( size ); throw x; }
void throw_no_space_error        ( const char* e ) { throw No_space_error       ( e ); }
void throw_not_exist_error       ( const char* e ) { throw Not_exist_error      ( e ); }
void throw_not_exist_error       ( const char* e, const Sos_object_base* o ) { Not_exist_error x ( e ); x.insert( o ); throw x; }
void throw_not_exist_error       ( const char* e, const char* o ) { Not_exist_error x ( e ); x.insert( o ); throw x; }
void throw_not_found_error       ( const char* e, const Sos_object_base* o ) { Not_found_error x ( e ); x.insert( o ); throw x; }
void throw_overflow_error        ( const char* e ) { throw Overflow_error       ( e ); }
void throw_overflow_error        ( const char* e, const char* a, const char* b ) { Overflow_error x ( e ); x.insert( a ); if(b) x.insert(b); throw x; }
//void throw_syntax_error          ( const char* e ) { throw Syntax_error         ( e ); }
void throw_syntax_error          ( const char* e, int column )                              { throw_syntax_error( e, NULL, column ); }
void throw_syntax_error          ( const char* e, const Source_pos& pos )                   { throw_syntax_error( e, NULL, pos ); }
void throw_syntax_error          ( const char* e, const char* ins, int column )             { throw_syntax_error( e, ins, Source_pos( column ) ); }
void throw_syntax_error          ( const char* e, const char* ins, const Source_pos& pos )  { throw Syntax_error( e, ins, pos ); }
void throw_too_long_error        ( const char* e ) { throw Too_long_error       ( e ); }
void throw_too_long_error        ( const char* e, const Sos_object_base* o ) { Too_long_error x ( e ); x.insert( o ); throw x; }
void throw_too_long_error        ( const char* e, long a, long b ) { Too_long_error x ( e ); x.insert( a ); x.insert( b ); throw x; }
void throw_wrong_type_error      ( const char* e ) { throw Wrong_type_error     ( e ); }
void throw_null_error            ( const char* e, const Sos_object_base* o )  { Null_error x ( e ); x.insert( o ); throw x; }
void throw_null_error            ( const char* e, const char* o )             { Null_error x ( e ); x.insert( o ); throw x; }
void throw_conversion_error      ( const char* e, const char* o )             { Conversion_error x ( e ); x.insert( o ); throw x; }

void throw_xc                    ( const char* e ) { throw Xc( e ); }
void throw_xc                    ( const char* e, int a, int b, int c ) { Xc x ( e ); x.insert(a); x.insert(b); x.insert(c); throw x; }
void throw_xc                    ( const char* e, int a, int b ) { Xc x ( e ); x.insert(a); x.insert(b); throw x; }
void throw_xc                    ( const char* e, int a ) { Xc x ( e ); x.insert(a); throw x; }
void throw_xc                    ( const char* e, const char* ins )                         { throw Xc( e, ins ); }
void throw_xc                    ( const char* e, const char* a, const char* b )            { Xc x(e); x.insert(a);x.insert(b); throw x; }
void throw_xc                    ( const char* e, const char* a, int b )                    { Xc x(e); x.insert(a);x.insert(b); throw x; }
void throw_xc                    ( const char* error_code, const Const_area& insertion )    { Xc x ( error_code ); x.insert( insertion ); throw x; }
void throw_xc                    ( const char* e, const Sos_object_base* object )           { throw Xc( e, object ); }
void throw_xc                    ( const char* e, const Sos_object_base* object, int i )           { Xc x ( e, object ); x.insert( i ); throw x; }
void throw_xc                    ( const char* e, const Sos_object_base* object, const char* o )           { Xc x ( e, object ); x.insert( o ); throw x; }
void throw_xc                    ( const char* e, const Msg_insertions& ins ) { throw Xc( e, ins ); }
void throw_xc                    ( const char* e, const Sos_object_base* a, const Sos_object_base* b )           { Xc x ( e ); x.insert(a); x.insert(b); throw x; }
void throw_xc                    ( const char* e, const Msg_insertions& ins, const Source_pos& pos ) { throw Xc( e, ins, pos ); }
void throw_xc                    ( const Xc& x )   { throw x; }
void throw_xc                    ( const exception& x )   { throw Xc( x ); }
void throw_xc                    ( const char* e, const Source_pos& pos, const char* a, int b, int c )  { Xc x(e);x.pos(pos);x.insert(a);x.insert(b);x.insert(c); throw x;}

void throw_xc_hex                ( const char* e, const void* p, uint len )  { Xc x ( e ); x.insert_hex( p, len ); throw_xc( x ); }

//----------------------------------------------------------------------------------throw_errno

void throw_errno( int e, const char* function, const Sos_object_base* o )
{
    Msg_insertions ins;
    ins.append( function );
    ins.append( o );
    throw_errno( e, ins );
}

//----------------------------------------------------------------------------------throw_errno

void throw_errno( int e, const Msg_insertions& ins )
//throw(Xc)
{
    Msg_code msg_code ( "ERRNO-", e );

    switch( e )
    {
//#     if !defined SYSTEM_UNIX
//        case ENOPATH:
//#     endif
        case ENOENT:
        {
            //Not_exist_error x ( "D140" );  // No such file or dir.
            Not_exist_error x ( msg_code );  // No such file or dir.
            x.insert( ins );
            throw x;
        }

        case EACCES:
        {
            //Locked_error x ( "D142" );  // Permission denied
            Locked_error x ( msg_code );  // Permission denied
            x.insert( ins );
            throw x;
        }

        case ENOMEM:
        {
            No_memory_error x ( msg_code );  // Not enough core
            x.insert( ins );
            throw x;
        }

        case EEXIST:
        {
            //Exist_error x ( "D151" );  // File already exists
            Exist_error x ( msg_code );  // File already exists
            x.insert( ins );
            throw x;
        }

        case EDEADLOCK:
        {
            //Locked_error x ( Msg_code( "ERRNO-", e ) );  // Locking violation
            Locked_error x ( msg_code );  // Locking violation
            x.insert( ins );
            throw x;
        }

        default:
        {
            throw Xc( msg_code, ins );
        }
    }
}

//---------------------------------------------------------------------------get_mswin_msg_text

void get_mswin_msg_text( Area* buffer, long msg_code )
{
    buffer->length( 0 );

#   if defined SYSTEM_WIN32
        if( buffer->resizable() )  buffer->allocate_min( 500 );
        buffer->allocate_min( 20 );
        buffer->char_ptr()[ 0 ] = '\0';

        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       msg_code,
                       MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), //The user default language
                       buffer->char_ptr(), buffer->size(), NULL );

        char* p = buffer->char_ptr() + strlen( buffer->char_ptr() );
        if( p > buffer->char_ptr()  &&  p[-1] == '\n' )  p--;
        if( p > buffer->char_ptr()  &&  p[-1] == '\r' )  p--;
        buffer->length( p - buffer->char_ptr() );
#   endif
}

//---------------------------------------------------------------------------get_mswin_msg_text

string get_mswin_msg_text( long msg_code )
{
    Dynamic_area buffer ( 1024 );
    get_mswin_msg_text( &buffer, msg_code );
    return as_string( buffer );
}

//----------------------------------------------------------------------------throw_mswin_error

void throw_mswin_error( int4 error, const char* function_name, const char* ins )
{
    char code[ 20 ];
    sprintf( code, "MSWIN-%08lX", (long)error );

    throw_xc( code, Msg_insertions( function_name, ins ) );
}


void throw_mswin_error( const char* function_name, const char* ins )
{
#   if defined SYSTEM_WIN

        throw_mswin_error( GetLastError(), function_name, ins );

#    else

        throw_mswin_error( 0, function_name, ins );

#   endif
}

//---------------------------------------------------------------------------------Syntax_error

Syntax_error::Syntax_error( const char* code, const Msg_insertions& ins,
                           const Source_pos& pos )
THROW_NONE
:
    Xc( code, ins, pos )
{
}

//--------------------------------------------------------------------check_new

void check_new( void* p, const char* source )
{
    if( !p ) throw_no_memory_error( source );
}

//--------------------------------------------------------------------------Source_pos::_assign

void Source_pos::_assign( const Source_pos& o )
{
    _col = o._col;
    _line = o._line;
    memcpy( _file, o._file, sizeof _file );
}

} //namespace sos
