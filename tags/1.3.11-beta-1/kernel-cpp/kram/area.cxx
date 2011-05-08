#include "precomp.h"
//#define MODULE_NAME "area"
// area.cpp
// 10.10.92                                                    Joacim Zschimmer


#include <stdlib.h>                 // abort()
#include <stdio.h>
#include <assert.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosalloc.h"
#include "../kram/tabucase.h"
#include "../kram/log.h"
#include "../kram/sosprof.h"
#include "../kram/area.h"

#if defined SYSTEM_WIN32
#   include <windows.h>
#endif

using namespace std;
namespace sos {

//---------------------------------------------------------------------------------------hex_tab

static char                     hex_tab [ 256 ] [2];

//---------------------------------------------------------------------------Const_area::compare

int Const_area::compare( const Const_area& b ) const
{
    int len = min( this->length(), b.length() );
    int cmp = len == 0? 0 : memcmp( this->ptr(), b.ptr(), len );
    return   cmp < 0? -1
           : cmp > 0? +1
           : this->length() < b.length() ? -1
           : this->length() > b.length() ? +1
                                         : 0;
}

//------------------------------------------------------------------------------Area::_size_min

void Area::_resize_min( unsigned int size )
{
    _allocate_min( size );
}

//--------------------------------------------------------------------------Area::_allocate_min

void Area::_allocate_min( unsigned int size )
{
    if( size > _size )  
    {
#       if defined SYSTEM_WIN32
            static Bool profile_read = false;
            static Bool brk;
    
            if( !profile_read ) {
                brk = read_profile_string( "", "debug", "xc-break" ) == "SOS-1113";
                profile_read = true;
            }

            if( brk )  {
                int rc = MessageBox( NULL,  "Wegen der Einstellung xc-break=SOS-1113 im Abschnitt [debug]\n"
                                            "der Datei sos.ini wird gleich unterbrochen.\n"
                                            "Einverstanden?",
                                     "hostODBC", 
                                     MB_TASKMODAL | MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
                if( rc == IDYES )  DebugBreak();
            }
#       endif

        throw_too_long_error( "SOS-1113", size, _size );
    }
}

//-----------------------------------------------------------------------------------Area::xlat
/*
void Area::xlat( const Byte* table )
{
    ::xlat( byte_ptr(), byte_ptr(), length(), table );
}
*/
//-----------------------------------------------------------------------------Area::lower_case

void Area::lower_case()
{
    xlat( tablcase );
}

//-----------------------------------------------------------------------------Area::upper_case

void Area::upper_case()
{
    xlat( tabucase );
}

//-----------------------------------------------------------------------------------Area::fill
/*
void Area::fill( char c, int len )
{
    allocate_length( len );
    memset( ptr(), length(), c );
}
*/
//----------------------------------------------------------------------------------operator <<

ostream& operator<< ( ostream& s, const Const_area& area )
{
    const char hex_t[] = "0123456789ABCDEF";

    if( s.flags() & ios::hex ) 
    {
        if( hex_tab[0][0] == 0 ) 
        {
            Z_MUTEX( hostware_mutex )
            {
                if( hex_tab[0][0] == 0 ) {
                    for (int i = 0; i < 16; i++) {
                        for (int j = 0; j < 16; j++) {
                            hex_tab [i * 16 + j] [0] = hex_t [i];
                            hex_tab [i * 16 + j] [1] = hex_t [j];
                        }
                    }
                }
            }
        }

        const Byte* p     = area.byte_ptr();
        const Byte* p_end = p + area.length();

        while( p < p_end ) {
            const char* x = hex_tab[ *p++ ];
            s.put( x[ 0 ] );
            s.put( x[ 1 ] );
        }
    } else {
        s.write( area.char_ptr(), area.length() );
    }

    return s;
}

//-----------------------------------------------------------------Area::assign
/*
void Area::assign( const char* ptr )
{
    unsigned int len = strlen( ptr );

    assign( (Byte*)ptr, len );
    length( len );
}
*/
//--------------------------------------------------------------------------------Area::_assign

void Area::_assign( const void* ptr, unsigned int len )
{
    _allocate_min( len );               // evtl. neue Puffer bei Dynamic_area und _ref_count > 1
    memcpy( char_ptr(), ptr, len );
    length( len );
}

//---------------------------------------------------------------------------------Area::append

void Area::append( const void* ptr, unsigned int len )
{
    if( (int4)len < 0 )  throw_no_memory_error( len );

    uint new_length = length() + len;
    resize_min( new_length );
    memcpy( byte_ptr() + length(), ptr, len );
    length( new_length );
}

//----------------------------------------------------------------------------Area::operator +=
#if !defined AREA_APPEND_CHAR_INLINE

Area& Area::operator +=( char c )
{
    append_inline( c );
    return *this;
}

#endif
//-------------------------------------------------------------------------------Area::length_error

void Area::length_error( uint len )
{
    //LOG( "Area::length( " << len << " ), aber _size=" << _size << '\n' );
    throw_xc( "SOS-1120", len, _size );
}

//----------------------------------------------------------------------------------operator >>

istream& operator>> ( istream& s, Area& area )
{
    s.read( area.char_ptr(), area.size() );
    area.length( s.gcount() );
    return s;
}

//----------------------------------------------------------convert_from_string

void convert_from_string( Area* area_ptr, const Sos_string& str )
{
/*
    area_ptr->allocate_min( length( str ) );  xc;
    area_ptr->length( length( str ) );

    memcpy( area_ptr->char_ptr(), c_str(str), area_ptr->length() );
  //area_ptr->char_ptr()[ area_ptr->length() ] = '\0';
*/
    area_ptr->assign( c_str( str ), length( str ) );
}

//----------------------------------------------------------Collectable_const_area::operator new

inline void* Collectable_const_area::operator new( size_t, void* ptr )
{
    return ptr;
}

//-------------------------------------------------------Collectable_const_area::operator delete

void Collectable_const_area::operator delete( void* ptr )
{
  //LOG( "Collectable_const_area::operator delete(" << (void*)ptr << "," << size << ")\n" );
    sos_free( ptr );
}

//-------------------------------------------------------Collectable_const_area::operator delete
#ifdef SYSTEM_DELETE_WITH_PARAMS

void Collectable_const_area::operator delete( void* ptr, void* )
{
  //LOG( "Collectable_const_area::operator delete(" << (void*)ptr << "," << size << ")\n" );
    sos_free( ptr );
}

#endif
//------------------------------------------------Collectable_const_area::Collectable_const_area

inline Collectable_const_area::Collectable_const_area( uint size )
:
    _size      ( size ),
    _ref_count ( 1 )
{
    //LOG( "sizeof (Collectable_const_area) == " << sizeof (Collectable_const_area) << "\n" );
}

//-------------------------------------------------------Collectable_const_area::operator delete

Collectable_const_area::~Collectable_const_area()
{
    //LOG( "~Collectable_const_area: " << (void*)this << " _size=" << _size << "\n" );
/*
    if( memcmp( buffer() + _size, "SLUT", 4 ) != 0 ) {   // s.a. Collectable_const_area::create()
        SHOW_ERR( "Fatal: Collectable_const_area(" << _size << ") (Datensatzpuffer) geplatzt\n" );
        abort();
    }
*/
}

//----------------------------------------------------------------Collectable_const_area::create

Collectable_const_area* Collectable_const_area::create( uint size )
{
    //LOG( "new Collectable_const_area( " << size << " )\n" );
    //Byte* p = new Byte [ Collectable_const_area::base_size() + size + 4 ];
    //Byte* p = (Byte*)malloc( Collectable_const_area::base_size() + size + 4 );
    //if( !p )  throw No_memory_error();
    //memcpy( p + Collectable_const_area::base_size() + size, "SLUT", 4 );
    if( (int4)size < 0 )  throw_no_memory_error( size );
    Byte* p = (Byte*)sos_alloc( Collectable_const_area::base_size() + size, "Dynamic_area" );
    //LOG( "liefert " << (void*)p << "\n" );

    return new ( p ) Collectable_const_area( size );
}

//----------------------------------------------------------Const_area_handle::Const_area_handle

Const_area_handle::Const_area_handle( Dynamic_area& dynamic_area )
:
    Const_area ( dynamic_area )
{
/*
    if( !dynamic_area._ptr ) {
        dynamic_area.allocate( 1 );
    }

    dynamic_area.collectable_area_ptr()->_ref_count++;
*/
    if( _ptr )  {
        collectable_area_ptr()->_ref_count++;
        //LOG( "Const_area_handle:copy ref=" << collectable_area_ptr()->_ref_count << "\n" );
    }
}

//----------------------------------------------------------Const_area_handle::Const_area_handle

Const_area_handle::Const_area_handle( const Const_area& area )
{
    _ptr  = Collectable_const_area::create( area.length() )->buffer();
    memcpy( _ptr, area.ptr(), area.length() );
    _length = area.length();
}

//---------------------------------------------------------Const_area_handle::~Const_area_handle

Const_area_handle::~Const_area_handle()
{
    //LOG( "Const_area_handle::~Const_area_handle(): ref_count=" << ( _ptr? collectable_area_ptr()->_ref_count : 0 ) << "\n" );
    if( _ptr  &&  --collectable_area_ptr()->_ref_count == 0 ) {
        del();
    }
}

//------------------------------------------------------------------Const_area_handle::operator=

Const_area_handle& Const_area_handle::operator= ( const Const_area_handle& h )
{
    if( _ptr && --collectable_area_ptr()->_ref_count == 0 ) {
        del();
    }

    _length = h._length;
    _ptr    = h._ptr;
    if( _ptr )  {
        collectable_area_ptr()->_ref_count++;
        //LOG( "Const_area_handle::operator=(): ref_count=" << collectable_area_ptr()->_ref_count << '\n' );
    }

    return *this;
}

//------------------------------------------------------------------Const_area_handle::operator=

Const_area_handle& Const_area_handle::operator= ( const Const_area& area )
{
    if( length() == area.length()  &&  ref_count() == 1 )
    {
        memcpy( _ptr, area.char_ptr(), area.length() );
    }
    else
    {
        *this = Const_area_handle( area );
    }

    return *this;
}

//-----------------------------------------------------------------------Const_area_handle::del

void Const_area_handle::del()
{
    //LOG( "delete Collectable_const_area( " << collectable_area_ptr()->_size << ", addr=" << (void*)collectable_area_ptr()<< " )\n" );
    delete collectable_area_ptr();
    _ptr = 0;
}

//----------------------------------------------------------------------------------Dynamic_area

Dynamic_area::Dynamic_area( unsigned int size )
{
    allocate( size );
}

//-------------------------------------------------------------------Dynamic_area::Dynamic_area

Dynamic_area::Dynamic_area( const Dynamic_area& o )
{
    allocate_min( o.size() );
    _assign( o.ptr(), o.length() );
}

//------------------------------------------------------------------Dynamic_area::~Dynamic_area

Dynamic_area::~Dynamic_area()
{
    if( _ptr ) {
        //LOG( "Dynamic_area::~Dynamic_area(): ref_count=" << collectable_area_ptr()->_ref_count << '\n' );
        if( collectable_area_ptr()->_ref_count == 1 ) {
            free();
        } else {
            collectable_area_ptr()->_ref_count--;
        }
    }
}

//--------------------------------------------------------------------Dynamic_area::_resize_min

void Dynamic_area::_resize_min( unsigned int size )
{
    if( _length == 0 ) _allocate_min( size );
    else {
        Const_area_handle old = *this;
        _allocate_min( size );
        if( _ptr != old.ptr() ) {
            assign( old.byte_ptr(), min( _size, old.length() ) );
        }
    }
}

//------------------------------------------------------------------------Dynamic_area::allocate

void Dynamic_area::allocate( unsigned int size )
{
    if( _ptr && collectable_area_ptr()->_ref_count > 1 )  free(); //throw Xc( "SOS-1119" );

    if (size != _size)
    {
        if( _size )  free();

        _ptr  = Collectable_const_area::create( size )->buffer();
        _size = size;
    }
}

//--------------------------------------------------Dynamic_area::_allocate_min

void Dynamic_area::_allocate_min( unsigned int size )
{
    //LOG( "Dynamic_area::_allocate_min()\n" );
    if( _ptr && collectable_area_ptr()->_ref_count > 1 )  free(); //throw Xc( "SOS-1119" );

    if( size > _size) {
        allocate( size );
    }
}

//----------------------------------------------------------------------------Dynamic_area::free

void Dynamic_area::free()
{
    if( _ptr )
    {
        //LOG( "Dynamic_area::free() ref_count=" << collectable_area_ptr()->_ref_count << '\n' );
        collectable_area_ptr()->_ref_count--;

        if( collectable_area_ptr()->_ref_count == 0 )  {//return;//throw Xc( "SOS-1119" );
          //delete collectable_area_ptr();
            Const_area_handle( *this );  // Referenz legen und wieder entfernen, löscht den Puffer und protokolliert das
        }

        _ptr    = 0;
        _size   = 0;
        _length = 0;
    }
}

//---------------------------------------------------------------------------Dynamic_area::take

void Dynamic_area::take( Dynamic_area* geber )
{
    if( _ptr )  free(); 

    _ptr    = geber->_ptr;     geber->_ptr    = NULL;
    _size   = geber->_size;    geber->_size   = 0;
    _length = geber->_length;  geber->_length = 0;
}

//-------------------------------------------------------------------------------------exchange

void exchange_dynamic_area( Dynamic_area* a, Dynamic_area* b )
{
    exchange( &a->_ptr   , &b->_ptr    );
    exchange( &a->_length, &b->_length );
    exchange( &a->_size  , &b->_size   );
}

//-------------------------------------------------------------------------Dynamic_area::assign
/* Auskommentiert, weil noch nicht getestet. Lohnt sich noch nicht.

void Dynamic_area::assign( Const_area_handle handle )   // nicht als Referenz übergeben!
{
    if( handle.ref_count() == 1 ) {     // der letzte Mohikaner?
        free();
        _ptr    = handle._ptr;
        _length = handle._length;
        _size   = handle._length;        // ?
        handle._ptr    = 0;
        handle._length = 0;
    }
    else
    {
        Area::assign( handle.ptr(), handle.length() );
    }
}

//-------------------------------------------------------------------------Dynamic_area::append

void Dynamic_area::append( Const_area_handle handle )   // nicht als Referenz übergeben!
{
    if( length() == 0 )
    {
        assign( handle );
    }
    else
    {
        Area::append( handle.ptr(), handle.length() );
    }
}
*/
//--------------------------------------------------------------------------Dynamic_area::handle
/*
Const_area_handle Dynamic_area::handle()
{
    if( !_ptr ) {
        allocate( 1 );
    }

    Collectable_const_area* a = collectable_area_ptr();
    a->_length = _length;
    return Const_area_handle( a );
}
*/
//------------------------------------------------------String_area::operator+=
/*
String_area& String_area::operator+= ( const String_area& str )
{
    allocate_min( this->length() + str.length() );  xc;
    memcpy( this->char_ptr() + this->length(), str.char_ptr(), str.length() + 1 );
    length( this->length() + str.length() );
    return *this;

  exception_handler:
    return *this;
}
*/
//------------------------------------------------------------String_area::_set
/*
void String_area::_set( const char* str )
{
    int len = strlen( str );
    allocate_min( len + 1 );  xc;
    length( len );
    memcpy( char_ptr(), str, len + 1 ); // ??? js : autom. Cast von str nach void  char* ???

  exceptions
}
*/
//-----------------------------------------------------Dynamic_string::allocate
/*
void Dynamic_string::allocate( unsigned int size )
{
    Dynamic_area::allocate( size );
}

//-------------------------------------------------Dynamic_string::allocate_min

void Dynamic_string::allocate_min( unsigned int size )
{
    Dynamic_area::allocate_min( size );
}
*/
//-----------------------------------------------------------------------------------------incr

void incr( Area* a )            // Als vorzeichenlose Binärzahl um eins erhöhen
{
    Byte* p     = a->byte_ptr() + a->length();
    int   carry = 1;

    while( carry && p-- > a->byte_ptr() ) {
        int m = *p + carry;
        carry =  m & 0x100? 1 : 0;
        *p = (Byte)m;
    }

    if( carry )  throw_overflow_error( "SOS-1227" );
}

//-----------------------------------------------------------------------------------------decr

void decr( Area* a )            // Als vorzeichenlose Binärzahl um eins erniedrigen
{
    Byte* p      = a->byte_ptr() + a->length();
    int   borrow = 1;

    while( borrow && p-- > a->byte_ptr() ) {
        int m = *p - borrow;
        borrow =  m & 0x100? 1 : 0;
        *p = (Byte)m;
    }

    if( borrow )  throw_overflow_error( "SOS-1227" );
}

//----------------------------------------------------------------------------------------rtrim

void rtrim( Area* area )           
{ 
    area->length( length_without_trailing_spaces( area->char_ptr(), area->length() ) ); 
}

/*
struct Area_ostrstream : ostrstream
{
    Area_ostrstream( Area* area_ptr ) : _area_ptr( area_ptr ), ostrstream( area_ptr->char_ptr(), area_ptr->size()-1 ) {}
   ~Area_ostrstream()
    {
        _area_ptr->length(pcount());
        //jz das knallt: _area_ptr->char_ptr()[_area_ptr->length()]='\0'; // null-Byte implizit, gehört aber nicht zum Satz
    }

  private:
     Area* _area_ptr;
};

*/


//---------------------------------------------------------------Area_streambuf::Area_streambuf

Area_streambuf::Area_streambuf( Area* area )
:
    _area ( area? area : &_buffer )
{
}

//---------------------------------------------------------------Area_streambuf::Area_streambuf

Area_streambuf::~Area_streambuf()
{
}

//-------------------------------------------------------------------------Area_streambuf::sync

int Area_streambuf::sync()
{
    setg( 0, 0, 0 );
    return overflow();
}

//--------------------------------------------------------------------Area_streambuf::underflow

int _Cdecl Area_streambuf::underflow()
{
    if( gptr() < egptr() ) {
        int c = *gptr();
        return c;
    }

    setg( _area->char_ptr(),
          _area->char_ptr(),
          _area->char_ptr() + _area->length() );

    return *_buffer.char_ptr();
}

//---------------------------------------------------------------------Area_streambuf::overflow

int _Cdecl Area_streambuf::overflow( int b )
{
    if( b == EOF ) {
       _area->length( _area->length() + pptr() - pbase() );
    } else {
        if( _area->size() == 0 )  _area->allocate_min( 256 );
                            else  _area->resize_min( _buffer.length() + 4096 );
        *_area += (char)b;
    }

    setp( _area->char_ptr() + _area->length(),
          _area->char_ptr() + _area->size() - _area->length() );

    return 0;
}

//---------------------------------------------------------------------Area_stream::Area_stream

Area_stream::Area_stream( Area* area )
: 
    Area_streambuf ( area ),
    iostream       ( (Area_streambuf*)this ) 
{
}

//--------------------------------------------------------------------Area_stream::~Area_stream

Area_stream::~Area_stream() 
{ 
    iostream::sync(); 
}

//-----------------------------------------------------------------------------------write_char

void write_char( char c, Area* output, char quote, char quote_quote )
{
    if( !quote  &&  c == '\0' )  throw_xc( "SOS-1111" );

    output->allocate_min( 4 );
    if( quote )  *output += quote;

    if( c == '\0' )  *output += "\0";
    else {
        if( c == quote || c == quote_quote )  *output += quote_quote;
        *output += c;
    }

    if( quote )  *output += quote;
}

//------------------------------------------------------------------------------------read_char
/*
void read_char( char* char_ptr, const char* input, char quote, char quote_quote )
{
    read_char( char_ptr, Const_area( input ), quote, quote_quote );
}

//------------------------------------------------------------------------------------read_char

void read_char( char* char_ptr, const Const_area& input, const Text_format& format )
{
    const char* q     = input.char_ptr();
    const char* q_end = q + input.length();

    if( q == q_end ) {
        *char_ptr = '\0';
    } else {
        if( q < q_end && *q == quote )  q++;
                                  else  throw_xc( "SOS-1115", input );

        if( q < q_end && *q == quote_quote )  q++;

        *char_ptr = *q++;

        if( q < q_end && *q == quote )  q++;
                                  else  throw_xc( "SOS-1115", input );

        if( q != q_end )  throw_xc( "SOS-1169" );
    }
}
*/

//---------------------------------------------------------------------------------write_string

void write_string( const char* p, int len, Area* output, char quote, char quote_quote )
{
    if( (int4)len < 0 )  throw_no_memory_error( len );
    if( memchr( p, '\0', len ) )  throw_xc_hex( "SOS-1111", p, len );

    output->allocate_min( 2 + len );    // + 1 für jedes Anführungszeichen im Text
    output->length( 0 );
    *output += quote;

    const char* p_end = p + len;

    if( quote == quote_quote ) {
        while( p < p_end ) {
            const char* q = (char*)memchr( p, quote, p_end - p );
            if( !q )  break;

            output->append( p, q - p + 1 );
            *output += quote;
            p = q + 1;
        }
        output->append( p, p_end - p );
    } else {
        while( p < p_end ) {
            if( *p == quote  ||  *p == quote_quote )  *output += quote_quote;
            *output += *p++;
        }
    }

    *output += quote;
}

//---------------------------------------------------------------------------------print_string

void print_string( const char* p, int len, ostream* s, char quote, char quote_quote )
{
    if( !quote )  if( memchr( p, '\0', len ) )  throw_xc_hex( "SOS-1111", p, len );

    if( quote )  *s << quote;

    const char* p_end;
    
    //p_end = memchr( p, '\0', len );
    p_end = p + len;

    while( p < p_end ) {
        if( *p == '\0' )  *s << "\\0";
        else {
            if( *p == quote  ||  *p == quote_quote )  *s << (char) quote_quote;
            *s << *p;
        }
        p++;
    }

    if( quote )  *s << quote;

    if( s->fail() )  throw_xc( "SOS-1170" );
}

//---------------------------------------------------------------------------------------format

void format( Area* buffer, double a, const char* form, char decimal_symbol )
{
    format( buffer, Const_area( as_string( a ) ), form, decimal_symbol );
    //char text [ 100+1 ];

    //char* old_locale = setlocale( LC_NUMERIC, "C" );
    //int  len = sprintf( text, "%f", (double)a );
    //setlocale( LC_NUMERIC, old_locale );


    //format( buffer, Const_area( text, len ), form, decimal_symbol );

/*
    char  buff [ 50+1 ];    // Zwischenformatierung von sprintf()
    char  buff2 [ 50+1 ];   // Ergebnis, rechtbündig
    int   dig   = 0;        // Zähler für Ziffern (von rechts)
    int   scale = 0;        // Anzahl Nachkommastellen

    const char* fe = form + strlen( form );
    const char* f  = fe;

    // Anzahl der Nachkommastellen (scale) herausfinden:
    while( f > form ) {
        if( f[-1] == '9'  ||  f[-1] == '0' )  dig++;
        else
        if( f[-1] == decimal_symbol )  { scale = dig; break; }
        f--;
    }

    sprintf( buff, "%.*f", scale, a );

          f = fe;
    char* b = buff + strlen( buff );
    char* p = buff2 + sizeof buff2;

    while( p > buff2  &&  b > buff )
    {
        if( f > form ) {
            if( f[-1] == '9'  ||  f[-1] == '0' )  { *--p = *--b; --f; }
            else
            if( f[-1] == decimal_symbol )  { *--p = decimal_symbol; --b; --f; }
            else
            if( f[-1] == ' '  &&  ( b[-1] == '-' || b[-1] == '+' ) )  { *--p = *--b; --f; }
            else
                *--p = *--f;
        } else {
            *--p = *--b;
        }
    }

    const char* p1   = p;      // Beginn relevanter Ziffern
    const char  taus = decimal_symbol == '.'? ','
                                            : '.';

    while( f > form  &&  p > buff2 )
    {
        if( f[-1] == '9' )  --f;
        else
        if( f[-1] == '0' )  { *--p = '0'; p1 = p; --f; }
        else
        if( f[-1] == taus ) { *--p = *--f; }
        else {
            *--p = *--f;
            p1 = p;
        }
    }

    if( f > form  ||  b > buff )  throw_xc( "format()-Fehler" );

    buffer->assign( p1, buff2 + sizeof buff2 - p1 );
*/
}

//---------------------------------------------------------------------------------------format

void format( Area* buffer, const Const_area& zahl, const char* form, char decimal_symbol )
{
{
    // zahl: Keine Tausenderzeichen, Dezimalzeichen ist Punkt, Vorzeichen direkt vor der Zahl,
    //       Blanks am Anfang und am Ende werdeb ignoriert.
    // format: [+-]999.999.990,00[+-]

    const char  taus    = decimal_symbol == '.'? ',' : '.';
    char        buff  [ 100+1 ];  // Ziffern der Zahl, ohne Vorzeichen oder Dezimalpunkt
    char*       b;
    char        buff2 [ 100+1 ];  // Ergebnis, rechtbündig
    int         dig     = 0;      // Zähler für Ziffern (von rechts)
    int         scale   = 0;      // Anzahl Nachkommastellen
    const char* fe      = form + strlen( form );
    const char* f;
    const char* z;
    Bool        neg = false;
    char*       k = NULL;


    f = fe;
    
    // Anzahl der Nachkommastellen (scale) herausfinden:
    while( f > form ) {
        if( f[-1] == '9'  ||  f[-1] == '0' )  dig++;
        else
        if( f[-1] == decimal_symbol )  { scale = dig; break; }
        f--;
    }



    b = buff;
    *b++ = '0';      // Platz für Überlauf beim Aufrunden
    z = zahl.char_ptr();

    while( z < zahl.char_ptr() + zahl.length()  &&  *z == ' ' )  z++;
 
    while( z < zahl.char_ptr() + zahl.length() ) 
    {
        if( b >= buff + sizeof buff )  goto FEHLER;
        if( *z == ' ' )  break;
        else
        if( *z == '+' )  z++;
        else
        if( *z == '-' )  { neg = true; z++; }
        else
        if( *z == '.' )  { k = b; z++; }
        else
        if( *z >= '0'  &&  *z <= '9' )  *b++ = *z++;
        else goto FEHLER;
    }
    while( z < zahl.char_ptr() + zahl.length()  &&  *z == ' ' )  z++;
    if( z != zahl.char_ptr() + zahl.length() )  goto FEHLER;

    if( !k )  k = b;

    if( b - k > scale ) {       // Zu viele Nachkommastellen?
        b = k + scale;
        if( *b >= '5' ) {           // Aufrunden?
            char* bb = b - 1;
            while(1) {
                if( ++bb[0] <= '9' )  break;
                bb[0] = '0';
                bb--;
            }
        }
    }

    while( b - k < scale ) {    // Zu wenige Nachkommastellen?
        if( b >= buff + sizeof buff )  goto FEHLER;
        *b++ = '0';
    }

    char* b0 = buff;            // Vornullen ignorieren
    while( b0 < k  &&  b0[0] == '0' )  b0++;


    Bool  zero = true;
    char* p    = buff2 + sizeof buff2;
    f = fe;

    while( b > b0  &&  p > buff2 + 1 )
    {
        if( f > form ) 
        {
            if( f[-1] == '9'  ||  f[-1] == '0' )  
            {
                if( zero  &&  b >= k  &&  f[-1] == '9'  &&  b[-1] == '0' ) {
                    // Nachnull ignorieren
                } else {
                    *--p = b[-1];
                    zero = false;
                }
                --b;  
                --f; 
            }
            else
            if( f[-1] == decimal_symbol ) 
            {
                *--p = decimal_symbol; 
                --f; 
            }
            else
            if( f[-1] == '-' )
            { 
                if( neg )  *--p = '-';
                     else  if( f > form )  *--p = ' ';
                --b; 
                --f; 
            }
            else
            if( f[-1] == '+' )
            { 
                *--p = neg? '-' : '+';
                --b; 
                --f; 
            }
            else
            {
                //if( b == b0  &&  f[-1] == taus ) {
                    // Format-Zeichen unterdrücken
                //} else {
                    *--p = *--f;
                //}
            }
        } 
        else 
        {
            *--p = *--b;
        }
    }

    char* p1 = p;      // Beginn relevanter Ziffern

    // Kann folgende Schleife in der vorangehenden aufgehen? Hier ist einiges redundant!
    while( f > form  &&  p > buff2 )
    {
        if( f[-1] == '9' )  --f;
        else
        if( f[-1] == '0' )  { *--p = '0'; p1 = p; --f; }
        else
        if( f[-1] == taus ) {
            if( f-1 == form  ||  f[-2] != '9' )  *--p = f[-1];
            --f;
        }
        else
        if( f[-1] == '-' )
        { 
            if( neg )  *--p1 = '-';
                 else  if( f >= form )  *--p1 = ' ';
            --f; 
        }
        else
        if( f[-1] == '+' )
        { 
            *--p = neg? '-' : '+';
            --b; 
            --f; 
        }
        else {
            *--p = *--f;
            p1 = p;
        }
    }

    if( f > form  ||  b > b0 )  goto FEHLER;

    buffer->assign( p1, buff2 + sizeof buff2 - p1 );
    return;
}
  FEHLER:
    throw_xc( "format()-Fehler", zahl );
}

} //namespace sos
