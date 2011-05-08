#include "precomp.h"
//#define MODULE_NAME "soslimtx"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#pragma implementation

#include "sosstrng.h"         // Für storable.h: Storable_as<Sos_string>
#include "sos.h"
#include "sosfield.h"
#include "soslimtx.h"

#if !defined SYSTEM_INCLUDE_TEMPLATES
#   include "soslimtx.tpl"
#endif

using namespace std;
namespace sos {

//-----------------------------------------------------------------------------String0_area::assign
// als inline ist length um 2 zu hoch ( Borland-C++ 4.02 )
/*
void String0_area::assign( const String0_area& str )
{
    assign( str.char_ptr(), str.length() );
}
*/
//-----------------------------------------------------------------------------String0_area::assign
/*
void String0_area::assign( const char* str, uint len )
{
    allocate_min( len );  xc;
    memcpy( ptr(), str, len + 1 );
    length( len );

  exceptions
}
*/

char& String0_area::chr( uint pos )
{
    if( pos >  size() )  { Too_long_error x ( "SOS-1150" ); x.insert( pos ); x.insert( size() ); throw_xc( x ); }
    if( pos == size() )  char_ptr()[ pos ] = '\0';
    return char_ptr()[ pos ];
}


} //namespace sos
