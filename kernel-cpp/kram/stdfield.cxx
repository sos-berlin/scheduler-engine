// $Id: stdfield.cxx 13579 2008-06-09 08:09:33Z jz $

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "stdfield"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#pragma implementation

#if defined SYSTEM_BORLAND || defined SYSTEM_SOLARIS
#   include <values.h>         // MAXINT
#endif

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>

#include "../kram/optimize.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/xception.h"
#include "../kram/sosarray.h"
#include "../kram/xlat.h"

#include "../kram/sosfield.h"
#include "../kram/exp10.h"
#include "../kram/stdfield.h"

using namespace std;
namespace sos {

#if defined SYSTEM_NOTALIGNED
    const int short_alignment  = 1;
    const int int_alignment    = 1;
    const int long_alignment   = 1;
    const int big_int_alignment= 1;       
    const int int64_alignment  = 1;       
    const int double_alignment = 1;
#else
    const int short_alignment  = sizeof (short);
    const int int_alignment    = sizeof (int);
    const int long_alignment   = sizeof (long);
    const int big_int_alignment= sizeof (Big_int);
#   if defined SYSTEM_INT64
        const int int64_alignment= sizeof (int64);       
#   endif
    const int double_alignment = sizeof (double);
#endif

int64   as_int64( const char* );  // Gehört nach sosstrg0.h
uint64 as_uint64( const char* );  // Gehört nach sosstrg0.h

//---------------------------------------------------------------------------------------static

Char_type                   char_type;
Bool_type                   bool_type;
Bool_type                   bool_1_type ( 1 );
Int_type                    int_type;
Uint_type                   uint_type;
Int1_type                   int1_type;
Uint1_type                  uint1_type;
Short_type                  short_type;
Ushort_type                 ushort_type;
Long_type                   long_type;
Ulong_type                  ulong_type;
Double_type                 double_type;
//Text_char                   c_char_field_type;
String0_type                const_string0_type( 32767-1 );    // nur zum lesen
Area_type                   area_type;
Null_area_type              null_area_type;
Sos_string_type             sos_string_type;
Void_method_type            void_method_type;

#ifdef SYSTEM_INT64
Int64_type                  int64_type;
Uint64_type                 uint64_type;
Currency_type               currency_type;
Big_int_type&               big_int_type = int64_type;
#else
Big_int_type&               big_int_type = int32_type;
#endif



// Prozessorabhängigkeiten:
//#if defined SYSTEM_INTEL || defined SYSTEM_SOLARIS  
#   ifdef SYSTEM_WIN16
      //Int_type            int2_type;
      //Uint_type           uint2_type;
      //Long_type           int4_type;
      //Ulong_type          uint4_type;
#    else
      //Short_type          int2_type;
      //Ushort_type         uint2_type;
#   endif
    Short_type          int16_type;
    Ushort_type         uint16_type;
    Long_type           int32_type;
    Ulong_type          uint32_type;
//#endif

//---------------------------------------------------------------------------------------------

static Text_type text_string_type_1 ( 1 );

//------------------------------------------------------------------------Char_type::_type_info

Listed_type_info Char_type::_type_info;

SOS_INIT( char )
{
    Char_type::_type_info._std_type      = std_type_varchar;
    Char_type::_type_info._name          = "char";
    Char_type::_type_info._nullable      = true;
    Char_type::_type_info._max_size      = 1;
    Char_type::_type_info._max_precision = 1;
    Char_type::_type_info._exact_char_repr = true;
    Char_type::_type_info.normalize();
}

//------------------------------------------------------------------------Char_type::write_text

void Char_type::write_text( const Byte* p, Area* output, const Text_format& ) const
{
    if( *p == '\x00' )  { output->length(0); return; }    // null

    output->allocate_min( 1 );
    output->length( 1 );
    *output->char_ptr() = *p;
}

//-------------------------------------------------------------------------Char_type::read_text

void Char_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    if( !t[0] && !t[1] )  throw_xc( "SOS-1241", this, t );
    *p = *t;
}

//------------------------------------------------------------------------Char_type::_get_param
/*
void Char_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
}
*/
//-----------------------------------------------------------------------------Char_type::empty

Bool Char_type::empty( const Byte* p ) const
{
    return *p == ' ';
}

//----------------------------------------------------------------------------write_text_inline

inline void write_text_inline( int o, Area* buffer )
{
    buffer->allocate_min( sizeof (int) * 3 + 2 );
    int len = sprintf( buffer->char_ptr(), "%d", o );
    buffer->length( len );
}

//-----------------------------------------------------------------------------------write_text

void write_text( int o, Area* buffer )
{
	write_text_inline( o, buffer );
}

//------------------------------------------------------------------------------------read_text

void read_text( int* p, const char* t )
{
    while( *t == ' ' )  t++;
    *(int*)p = as_int( t );
}

//-------------------------------------------------------------------------Bool_type::_type_info

Listed_type_info Bool_type::_type_info;

SOS_INIT( bool )
{
    Bool_type::_type_info._std_type      = std_type_bool;   // std_type_integer  16.11.97
    Bool_type::_type_info._name          = "bool";
    Bool_type::_type_info._max_size      = sizeof(Bool);
    Bool_type::_type_info._max_precision = 1;  // oder 5 für "false" und "true" ?
    Bool_type::_type_info._display_size  = 1;  // oder 5 für "false" und "true" ?
    Bool_type::_type_info.normalize();
}

//-------------------------------------------------------------------------Bool_type::write_text

void Bool_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    Bool b;

    if( _field_size == sizeof (Bool) )  b = *(Bool*)p != false;
    else
    if( _field_size == sizeof (Byte) )  b = *(Byte*)p != 0;
    else
    if( _field_size == sizeof (int2) )  b = *(int2*)p != 0;
    else
    if( _field_size == sizeof (int4) )  b = *(int4*)p != 0;
    else throw_xc( "SOS-1363", _field_size );

    buffer->allocate_length( 1 );
    buffer->char_ptr()[ 0 ] = '0' + b;
}

//--------------------------------------------------------------------------Bool_type::read_text
#if 1
void Bool_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    Bool b = ::sos::as_bool( t );

    if( _field_size == sizeof (Bool) )  *(Bool*)p = b;
    else
    if( _field_size == sizeof (Byte) )  *(Byte*)p = b;
    else
    if( _field_size == sizeof (int2) )  *(int2*)p = b;
    else
    if( _field_size == sizeof (int4) )  *(int4*)p = b;
    else throw_xc( "SOS-1363", _field_size );
}
#else
void Bool_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    int i = abs( ::sos::as_int( t ) );
    if( i & ~1 )  throw_xc( "SOS-1240", t );

    if( _field_size == sizeof (Bool) )  *(Bool*)p = (Bool)i;
    else
    if( _field_size == sizeof (Byte) )  *(Byte*)p = i;
    else
    if( _field_size == sizeof (int2) )  *(int2*)p = i;
    else
    if( _field_size == sizeof (int4) )  *(int4*)p = i;
    else throw_xc( "SOS-1363", _field_size );
}
#endif
//-------------------------------------------------------------------------Bool_type::_get_param
/*
void Bool_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (Bool);
    param->_info_ptr     = &_type_info;
}
*/
//-------------------------------------------------------------------------Int_type::_type_info

Listed_type_info Int_type::_type_info;

SOS_INIT( int )
{
    Int_type::_type_info._std_type      = std_type_integer;
    Int_type::_type_info._name          = "int";
    Int_type::_type_info._max_size      = sizeof(int);
    Int_type::_type_info._max_precision = sizeof(int) * 8;
    Int_type::_type_info._alignment     = int_alignment;
    Int_type::_type_info._radix         = 2;
    Int_type::_type_info._display_size  = sizeof (int) == 2? 1+5 : sizeof (int) == 4? 1+9 : 1+19;
    Int_type::_type_info.normalize();
}

//-------------------------------------------------------------------------Int_type::write_text

void Int_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    ::sos::write_text_inline( *(int*)p, buffer );
}

//--------------------------------------------------------------------------Int_type::read_text

void Int_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    //::read_text( (int*)p, t );
    while( *t == ' ' )  t++;
    *(int*)p = ::sos::as_int( t );
}

//-------------------------------------------------------------------------Int_type::_get_param
/*
void Int_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (int) == 2? 1+5 : sizeof (int) == 4? 1+9 : 1+19;
  //param->_info_ptr     = &_type_info;
}
*/
//------------------------------------------------------------------------Uint_type::_type_info

Listed_type_info Uint_type::_type_info;

SOS_INIT( uint )
{
    Uint_type::_type_info._std_type      = std_type_integer;
    Uint_type::_type_info._name          = "unsigned_int";
    Uint_type::_type_info._max_size      = sizeof(uint);
    Uint_type::_type_info._max_precision = sizeof(uint) * 8;
    Uint_type::_type_info._alignment     = int_alignment;
    Uint_type::_type_info._radix         = 2;
    Uint_type::_type_info._unsigned      = true;
    Uint_type::_type_info._display_size  = sizeof (uint) == 2? 1+5 : sizeof (uint) == 4? 1+9 : 1+19;
    Uint_type::_type_info.normalize();
}

//------------------------------------------------------------------------Uint_type::write_text

void Uint_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    buffer->allocate_min( sizeof (uint) * 3 + 1 );
    int len = sprintf( buffer->char_ptr(), "%u", *(uint*)p  );
    buffer->length( len );
}

//-------------------------------------------------------------------------Uint_type::read_text

void Uint_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    *(int*)p = as_uint( t );
}

//------------------------------------------------------------------------Uint_type::_get_param
/*
void Uint_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (uint) == 2? 1+5 : sizeof (uint) == 4? 1+9 : 1+19;
  //param->_info_ptr     = &_type_info;
}
*/
//-----------------------------------------------------------------------------------write_text

void write_text( short o, Area* buffer )
{
    buffer->allocate_min( sizeof (short) * 3 + 2 );
    int len = sprintf( buffer->char_ptr(), "%d", (int)o );
    buffer->length( len );
}

//------------------------------------------------------------------------------------read_text

void read_text( short* p, const char* t )
{
    while( *t == ' ' )  t++;
    *(short*)p = as_short( t );
}

//------------------------------------------------------------------------Int1_type::_type_info

Listed_type_info Int1_type::_type_info;

SOS_INIT( int1 )
{
    Int1_type::_type_info._std_type      = std_type_integer;
    Int1_type::_type_info._name          = "int1";
    Int1_type::_type_info._max_size      = sizeof(int1);
    Int1_type::_type_info._alignment     = 1;
    Int1_type::_type_info._max_precision = 8;
    Int1_type::_type_info._radix         = 2;
    Int1_type::_type_info._display_size  = 1+3;
    Int1_type::_type_info.normalize();
}

//------------------------------------------------------------------------Int1_type::write_text

void Int1_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    ::sos::write_text_inline( *(int1*)p, buffer );
}

//-------------------------------------------------------------------------Int1_type::read_text

void Int1_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    int i = ::sos::as_int( t );
    if( i > 127  ||  i < -128 )  throw_overflow_error( "SOS-1107", t, "int1" );
    *(int1*)p = i;
}

//-----------------------------------------------------------------------Uint1_type::_type_info

Listed_type_info Uint1_type::_type_info;

SOS_INIT( uint1 )
{
    Uint1_type::_type_info._std_type      = std_type_integer;
    Uint1_type::_type_info._name          = "unsigned int1";
    Uint1_type::_type_info._max_size      = 1;
    Uint1_type::_type_info._max_precision = 8;
    Uint1_type::_type_info._alignment     = 1;
    Uint1_type::_type_info._radix         = 2;
    Uint1_type::_type_info._unsigned      = true;
    Uint1_type::_type_info._display_size  = 3;
    Uint1_type::_type_info.normalize();
}

//-----------------------------------------------------------------------Uint1_type::write_text

void Uint1_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    buffer->allocate_min( sizeof (uint) * 3 + 1 );
    int len = sprintf( buffer->char_ptr(), "%u", *(uint*)p  );
    buffer->length( len );
}

//------------------------------------------------------------------------Uint1_type::read_text

void Uint1_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    uint i = ::sos::as_uint( t );
    if( i > 0xFF )  throw_overflow_error( "SOS-1107", "uint1", t );
    *(uint1*)p = i;
}

//-----------------------------------------------------------------------Short_type::_type_info

Listed_type_info Short_type::_type_info;

SOS_INIT( short )
{
    Short_type::_type_info._std_type      = std_type_integer;
    Short_type::_type_info._name          = "short";
    Short_type::_type_info._max_size      = sizeof(short);
    Short_type::_type_info._alignment     = short_alignment;
    Short_type::_type_info._max_precision = sizeof(short) * 8;
    Short_type::_type_info._radix         = 2;
    Short_type::_type_info._display_size  = 1+5;
    Short_type::_type_info.normalize();
}

//-----------------------------------------------------------------------Short_type::write_text

void Short_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    ::sos::write_text( *(short*)p, buffer );
}

//------------------------------------------------------------------------Short_type::read_text

void Short_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    ::sos::read_text( (short*)p, t );
}

//-----------------------------------------------------------------------Short_type::_get_param
/*
void Short_type::_get_param( Type_param* param ) const
{
    param->_display_size = 1+5;
  //param->_info_ptr     = &_type_info;
}
*/
//----------------------------------------------------------------------Ushort_type::_type_info

Listed_type_info Ushort_type::_type_info;

SOS_INIT( ushort )
{
    Ushort_type::_type_info._std_type      = std_type_integer;
    Ushort_type::_type_info._name          = "unsigned short";
    Ushort_type::_type_info._max_size      = sizeof(unsigned short);
    Ushort_type::_type_info._alignment     = short_alignment;
    Ushort_type::_type_info._max_precision = sizeof(unsigned short) * 8;
    Ushort_type::_type_info._radix         = 2;
    Ushort_type::_type_info._unsigned      = true;
    Ushort_type::_type_info._display_size  = 5;
    Ushort_type::_type_info.normalize();
}

//----------------------------------------------------------------------Ushort_type::write_text

void Ushort_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    char buff [ 1+10+1 ];
    int len = sprintf( buff, "%u", (uint)*(unsigned short*)p );
    buffer->assign( buff, len );

    //buffer->allocate_min( sizeof (unsigned short) * 3 + 1 );
    //int len = sprintf( buffer->char_ptr(), "%u", (uint)*(unsigned short*)p  );
    //buffer->length( len );
}

//-----------------------------------------------------------------------Ushort_type::read_text

void Ushort_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    *(unsigned short*)p = as_ushort( t ); 
}

//----------------------------------------------------------------------Ushort_type::_get_param
/*
void Ushort_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (unsigned short) == 2? 1+5 : 1+9;
  //param->_info_ptr     = &_type_info;
}
*/
//------------------------------------------------------------------------Long_type::_type_info

Listed_type_info Long_type::_type_info;

SOS_INIT( long )
{
    Long_type::_type_info._std_type      = std_type_integer;
    Long_type::_type_info._name          = "long int";
    Long_type::_type_info._max_size      = sizeof(long);
    Long_type::_type_info._alignment     = long_alignment;
    Long_type::_type_info._max_precision = sizeof(long) * 8;  // bits
    Long_type::_type_info._radix         = 2;
    Long_type::_type_info._display_size  = sizeof (long) == 4? 1+9 : 1+19;
    Long_type::_type_info.normalize();
}

//------------------------------------------------------------------------Long_type::write_text

void Long_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    char buff [ 1+40+1 ];
    int len = sprintf( buff, "%ld", *(long*)p );
    buffer->assign( buff, len );

    //buffer->allocate_min( sizeof (long) == 4? 1+10 : sizeof (long) == 8? 1+20 : 1+40 );
    //int len = sprintf( buffer->char_ptr(), "%ld", *(long*)p );
    //buffer->length( len );
}

//-------------------------------------------------------------------------Long_type::read_text

void Long_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    *(long*)p = as_int4( t );
}

//------------------------------------------------------------------------Long_type::_get_param
/*
void Long_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (long) == 4? 1+9 : 1+19;
  //param->_info_ptr     = &_type_info;
}
*/
//-----------------------------------------------------------------------Ulong_type::_type_info

Listed_type_info Ulong_type::_type_info;

SOS_INIT( ulong )
{
    Ulong_type::_type_info._std_type      = std_type_integer;
    Ulong_type::_type_info._name          = "unsigned long int";
    Ulong_type::_type_info._max_size      = sizeof(unsigned long);
    Ulong_type::_type_info._alignment     = long_alignment;
    Ulong_type::_type_info._max_precision = sizeof(unsigned long) * 8;
    Ulong_type::_type_info._radix         = 2;
    Ulong_type::_type_info._unsigned      = true;
    Ulong_type::_type_info._display_size  = sizeof (long) == 4? 9 : 19;
    Ulong_type::_type_info.normalize();
}

//-----------------------------------------------------------------------Ulong_type::write_text

void Ulong_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    char buff [ 1+40+1 ];
    int len = sprintf( buff, "%lu", *(ulong*)p );
    buffer->assign( buff, len );

    //buffer->allocate_min( sizeof (ulong) * 3 + 1 );
    //int len = sprintf( buffer->char_ptr(), "%lu", *(ulong*)p  );
    //buffer->length( len );
}

//------------------------------------------------------------------------Ulong_type::read_text

void Ulong_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;
    *(ulong*)p = as_uint4( t );
}

//-----------------------------------------------------------------------Ulong_type::_get_param
/*
void Ulong_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (unsigned long) == 4? 1+9 : 1+19;
  //param->_info_ptr     = &_type_info;
}
*/
//------------------------------------------------------------------------Int64_type::_type_info
#ifdef SYSTEM_INT64

Listed_type_info Int64_type::_type_info;

SOS_INIT( __int64 )
{
    Int64_type::_type_info._std_type      = std_type_integer;
    Int64_type::_type_info._name          = "int64";
    Int64_type::_type_info._max_size      = sizeof(__int64);
    Int64_type::_type_info._alignment     = int64_alignment;
    Int64_type::_type_info._max_precision = sizeof(__int64) * 8;  // bits
    Int64_type::_type_info._radix         = 2;
    Int64_type::_type_info._display_size  = 1+19;
    Int64_type::_type_info.normalize();
}

//------------------------------------------------------------------------Int64_type::write_text

void Int64_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    char buff [ 1+40+1 ];

    int len = sprintf( buff, "%" PRINTF_LONG_LONG "d", *(__int64*)p );
    buffer->assign( buff, len );
}

//-------------------------------------------------------------------------Int64_type::read_text

void Int64_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;

    *(int64*)p = ::sos::as_int64( t );
}

//-----------------------------------------------------------------------Uint64_type::_type_info

Listed_type_info Uint64_type::_type_info;

SOS_INIT( uint64 )
{
    Uint64_type::_type_info._std_type      = std_type_integer;
    Uint64_type::_type_info._name          = "uint64";
    Uint64_type::_type_info._max_size      = sizeof(__int64);
    Uint64_type::_type_info._alignment     = int64_alignment;
    Uint64_type::_type_info._max_precision = sizeof(__int64) * 8;  // bits
    Uint64_type::_type_info._radix         = 2;
    Uint64_type::_type_info._display_size  = 1+19;
    Uint64_type::_type_info._unsigned      = true;
    Uint64_type::_type_info.normalize();
}

//------------------------------------------------------------------------Uint64_type::write_text

void Uint64_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    char buff [ 1+40+1 ];

    int len = sprintf( buff, "%" PRINTF_LONG_LONG "u", *(__int64*)p );
    buffer->assign( buff, len );
}

//-------------------------------------------------------------------------Uint64_type::read_text

void Uint64_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    while( *t == ' ' )  t++;

    *(uint64*)p = as_uint64( t );
}

#endif
//------------------------------------------------------------------Scaled_int64_type::_type_info
#ifdef SYSTEM_INT64

Listed_type_info Scaled_int64_type::_type_info;

SOS_INIT( scaled_int64 )
{
    Scaled_int64_type::_type_info._std_type      = std_type_decimal;
    Scaled_int64_type::_type_info._name          = "scaled int64";
    Scaled_int64_type::_type_info._max_size      = sizeof(__int64);
    Scaled_int64_type::_type_info._alignment     = int64_alignment;
    Scaled_int64_type::_type_info._max_precision = sizeof(__int64) * 8;  // bits
    Scaled_int64_type::_type_info._radix         = 2;
    Scaled_int64_type::_type_info._max_scale     = 19;
    Scaled_int64_type::_type_info._display_size  = 1+19+2;
    Scaled_int64_type::_type_info.normalize();
}

//------------------------------------------------------------------Scaled_int64_type::write_text

void Scaled_int64_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    char buff [ 1+40+1 ];

    int64 a = *(int64*)p;

    if( a < 0 ) 
    {
        buff[0] = '-';
        Area buff_area ( buff + 1, sizeof buff - 1 );
        if( a != UI64( 0x8000000000000000 ) )  a = -a;
        Scaled_uint64_type( _scale ).write_text( (Byte*)&a, &buff_area, format );

        buffer->assign( buff, 1 + buff_area.length() );
    }
    else
    {
        Scaled_uint64_type( _scale ).write_text( (Byte*)&a, buffer, format );
    }
}

//-------------------------------------------------------------------------Scaled_int64_type::read_text

void Scaled_int64_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    while( *t == ' ' )  t++;

    uint64  a;
    int     sign = +1;

    if( *t == '+' )  t++;
    else
    if( *t == '-' )  { t++; sign = -1; }

    Scaled_uint64_type( _scale ).read_text( (Byte*)&a, t, format );

    *(int64*)p = a * sign;
}

//------------------------------------------------------------------------Scaled_int64_type::_get_param

void Scaled_int64_type::_get_param( Type_param* param ) const
{
    param->_scale = _scale;
}

//------------------------------------------------------------------------Scaled_int64_type::_obj_print

void Scaled_int64_type::_obj_print( ostream* s ) const
{
    *s << "int64";
    *s << "(scale=" << _scale << ')';
}

//-----------------------------------------------------------------------Scaled_uint64_type::_type_info

Listed_type_info Scaled_uint64_type::_type_info;

SOS_INIT( scaled_uint64 )
{
    Scaled_uint64_type::_type_info._std_type      = std_type_decimal;
    Scaled_uint64_type::_type_info._name          = "scaled uint64";
    Scaled_uint64_type::_type_info._max_size      = sizeof(__int64);
    Scaled_uint64_type::_type_info._alignment     = int64_alignment;
    Scaled_uint64_type::_type_info._max_precision = sizeof(__int64) * 8;  // bits
    Scaled_uint64_type::_type_info._radix         = 2;
    Scaled_uint64_type::_type_info._display_size  = 19+2;
    Scaled_uint64_type::_type_info._unsigned      = true;
    Scaled_uint64_type::_type_info.normalize();
}

//------------------------------------------------------------------------Scaled_uint64_type::write_text

void Scaled_uint64_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    char buff [ 1+40+1 ];

    if( _scale ) 
    {
        int len = sprintf( buff, "%0*" PRINTF_LONG_LONG "u", 1 + _scale, *(uint64*)p );

        buffer->allocate_min( len + 1 );
        char* b = buffer->char_ptr();

        int l = len - _scale;
        memcpy( b, buff, l );                   // Ziffern vor dem Komma
        b += l;
        *b = format.decimal_symbol();
        memcpy( b + 1, buff + l, _scale );      // Ziffern nach dem Komma

        buffer->length( len + 1 );
    }
    else
    {
        int len = sprintf( buff, "%" PRINTF_LONG_LONG "u", *(__int64*)p );
        buffer->assign( buff, len );
    }
}

//-------------------------------------------------------------------------Scaled_uint64_type::read_text

void Scaled_uint64_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    while( *t == ' ' )  t++;

    char        buff [ 100+1 ];       // Nimmt die Zahl ohne Komma auf; Das Komma ist um _scale Stellen nach rechts vorschoben
    char*       b = buff;
    const char* q = t;
   
    while( isdigit( *q ) ) {    // Ziffern vor dem Komma
        if( b >= buff + sizeof buff - 1 )  goto THROW_1107;
        *b++ = *q++;
    }

    if( *q == format.decimal_symbol() )     // Komma
    {
        q++;
        int s = _scale;                     
        if( b + s >= buff + sizeof buff - 1 )  goto THROW_1107;

        while( s > 0  &&  isdigit( *q ) ) {   // Ziffern nach dem Komma
            *b++ = *q++;
            s--;
        }

        while( *q == '0' )  q++;        // Überflüssige Nullen hinter dem Komma

        while( s > 0 ) {                // Fehlende Nullen auffüllen
            *b++ = '0';     
            s--;
        }
    }

    while( *q == ' ' )  q++;

    if( *q != '\0' )  {
        if( isdigit( *q ) )  throw_xc( "SOS-1107", this, t );
                       else  throw_xc( "SOS-1105", t );
    }

    *b++ = '\0';

    *(uint64*)p = as_uint64( buff );

    return;

  THROW_1107:
    throw_xc( "SOS-1107", "uint64", t );
}

//----------------------------------------------------------------------Scaled_uint64_type::_get_param

void Scaled_uint64_type::_get_param( Type_param* param ) const
{
    param->_scale = _scale;
}

//----------------------------------------------------------------------Scaled_uint64_type::_obj_print

void Scaled_uint64_type::_obj_print( ostream* s ) const
{
    *s << "uint64";
    *s << "(scale=" << _scale << ')';
}

#endif
//---------------------------------------------------------------------Big_int_type::_type_info
/*
#if defined SYSTEM_INT64

Listed_type_info Big_int_type::_type_info;

SOS_INIT( big_int )
{
    Big_int_type::_type_info._std_type      = std_type_integer;
    Big_int_type::_type_info._name          = "Big_int";
    Big_int_type::_type_info._max_size      = sizeof(Big_int);
    Big_int_type::_type_info._alignment     = big_int_alignment;
    Big_int_type::_type_info._max_precision = sizeof(Big_int) * 8;
    Big_int_type::_type_info._radix         = 2;
    Big_int_type::_type_info.normalize();
}

//---------------------------------------------------------------------Big_int_type::write_text

void Big_int_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    buffer->allocate_min( sizeof (Big_int) * 3 + 1 );
    ostrstream s ( buffer->char_ptr(), buffer->size() );
    s << *(Big_int*)p;
    buffer->length( s.pcount() );

    //? int len = sprintf( buffer->char_ptr(), "%lli", *(Big_int*)p  );
    //buffer->length( len );
}

//----------------------------------------------------------------------Big_int_type::read_text

void Big_int_type::read_text( Byte* p, const char* t, const Text_format& f ) const
{
    while( *t == ' ' )  t++;
    *(Big_int*)p = as_big_int( t );
}

#endif
*/
//------------------------------------------------------------------------Currency_type::_type_info
#ifdef SYSTEM_INT64

Listed_type_info Currency_type::_type_info;

SOS_INIT( currency )                                                // max 922.337.203.685.478,0000
{
    Currency_type::_type_info._std_type      = std_type_decimal;
    Currency_type::_type_info._name          = "Currency";
    Currency_type::_type_info._max_size      = sizeof(int64);
    Currency_type::_type_info._alignment     = int64_alignment;
    Currency_type::_type_info._max_precision = 19;                  // Nicht 18, weil u.a. ODBC-Treiber 19 liefert. Sonst passt's nicht.
    Currency_type::_type_info._min_scale     = 4;
    Currency_type::_type_info._max_scale     = 4;
    Currency_type::_type_info._radix         = 10;
    Currency_type::_type_info._display_size  = 1+15+1+4;
    Currency_type::_type_info.normalize();
}

//------------------------------------------------------------------------Currency_type::write_text

void Currency_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    char buff [ 1+15+1+4+1 ];
    char* b = buff;

    int64 n = *(int64*)p;

    if( n < 0 )  *b++ = '-', n = -n;

    int len = sprintf( b, "%" PRINTF_LONG_LONG "d%c%04d", n / 10000, format.decimal_symbol(), (int)( n % 10000 ) );

    len += b - buff;
    
    if( !format.raw() ) 
    {
        while( buff[ len-1 ] == '0' )  len--;
        if( buff[ len-1 ] == format.decimal_symbol() )  len--;
    }

    buffer->assign( buff, len );
}

//-------------------------------------------------------------------------Currency_type::read_text

void Currency_type::read_text( Byte* p, const char* text, const Text_format& format ) const
{
    const char* t = text;
    char        buffer [ 100 ];
    char*       b = buffer;
    Big_int     n;
    int         e;
    int         sign = +1;

    while( *t == ' ' )  t++;

    if( *t == '-' )  sign = -1, *t++;

    while( isdigit( *t ) ) {
        if( b >= buffer + sizeof buffer - 1 )  throw_conversion_error( "SOS-1140", text );
        *b++ = *t++;
    }
    *b++ = '\0';

    if( buffer[0] == '\0' )  n = 0;
                       else  n = sos::as_big_int( buffer );

    if( n > INT64_MAX / 10000 )  throw_overflow_error( "SOS-1107", "Currency", text );
    n *= 10000;

    if( *t == format.decimal_symbol() ) 
    {
        t++;

        e = 10000;
        while( isdigit( *t ) ) {
            e /= 10;
            if( e == 0  &&  *t != '0' )   throw_xc( "SOS-1107", "Currency", text );
            n += ( *t++ - '0' ) * e;
        }
    }

    *(int64*)p = sign * n;

    if( !::sos::empty( t )  ||  t == text )  throw_conversion_error( "SOS-1140", text );
}

//------------------------------------------------------------------------Currency_type::_get_param
/*
void Currency_type::_get_param( Type_param* param ) const
{
    param->_display_size = sizeof (long) == 4? 1+9 : 1+19;
  //param->_info_ptr     = &_type_info;
}
*/
#endif
//----------------------------------------------------------------------Double_type::_type_info

Listed_type_info Double_type::_type_info;

SOS_INIT( double )
{
    Double_type::_type_info._std_type      = std_type_float;
    Double_type::_type_info._name          = "double";
    Double_type::_type_info._max_size      = sizeof(double);
    Double_type::_type_info._alignment     = double_alignment;
    Double_type::_type_info._max_precision = 15;
    Double_type::_type_info._radix         = 10;
    Double_type::_type_info._min_scale     = -308;
    Double_type::_type_info._max_scale     = +308;
    Double_type::_type_info.normalize();
}

//----------------------------------------------------------------------Double_type::write_text

void Double_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    char buff [ 70+1 ];
    int  len;

    //char* old_locale = setlocale( LC_NUMERIC, "C" );

    if( (int)p & ( double_alignment-1 ) )  throw_xc( "SOS-1356", this );  // Ausgerichtet?
  //if( *(double*)p == NAN )  return;   // Not A Number

    if( _scale_null  &&  abs( *(double*)p ) < 1e14 ) 
    {
        len = sprintf( buff, "%-.14lg", *(double*)p );     // 14 signifikante Ziffern ausgeben, evtl. mit Exponent
    } 
    else
    if( abs( *(double*)p ) < 1e30 ) 
    {
        len = sprintf( buff, "%-.*lf", (int)max( _scale, 0 ), *(double*)p );  // "12345.789"
    } 
    else 
    {
        len = sprintf( buff, "%-.14le", *(double*)p );  // "+1.234567890123456e+00"
    }

    //setlocale( LC_NUMERIC, old_locale );

    if( format.decimal_symbol() != '.'  &&  format.decimal_symbol() != '\0' ) {
        char* d = (char*)memchr( buff, '.', len );
        if( d )  *d = format.decimal_symbol();
    }

    buffer->assign( buff, len );
}

//-----------------------------------------------------------------------Double_type::read_text

void Double_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    if( (int)p & ( double_alignment-1 ) )  throw_xc( "SOS-1356", this );   // Ausgerichtet?
    *(double*)p = sos::as_double( t );
}

//----------------------------------------------------------------------Double_type::_get_param

void Double_type::_get_param( Type_param* param ) const
{
  //param->_display_size = 15;
    param->_scale        = _scale;
    param->_scale_null   = _scale_null;
  //param->_info_ptr     = &_type_info;
}

//-------------------------------------------------------------------Xlat_char_type::_type_info

Listed_type_info Xlat_char_type::_type_info;

SOS_INIT( xlat_char )
{
    Xlat_char_type::_type_info._std_type      = std_type_char;
    Xlat_char_type::_type_info._name          = "Xlat_char";
    Xlat_char_type::_type_info._max_size      = sizeof(char);
    Xlat_char_type::_type_info._max_precision = 1;
    Xlat_char_type::_type_info.normalize();
    Xlat_char_type::_type_info._exact_char_repr = true;
}

//------------------------------------------------------------------------Xlat_char::write_text

void Xlat_char::write_text( const Byte* p, Area* text, const Text_format& ) const
{
    if( *p == 0x00 )  { text->length(0); return; }    // NULL

    text->allocate_min( 1 );
    text->length( 1 );
    *text->char_ptr() = write_table()[ *p ];
}

//-------------------------------------------------------------------------Xlat_char::read_text

void Xlat_char::read_text( Byte* p, const char* t, const Text_format& ) const
{
    if( t[0] && t[1] )  throw_xc( "SOS-1241", this, t );
    *p = read_table()[ *t ];

    //char c;
    //read_char( &c, text, format );
    //*ptr = read_table()[ (Byte)c ];
}

//------------------------------------------------------------------------------Xlat_char::empty

Bool Xlat_char::empty( const Byte* p ) const
{
    return *p == read_table()[' '];
}

//-------------------------------------------------------------------Xlat_char_type::_get_param
/*
void Xlat_char_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
}
*/
//-------------------------------------------------------------------Xlat_text_type::_type_info

Listed_type_info Xlat_text_type::_type_info;

SOS_INIT( xlat_text )
{
    Xlat_text_type::_type_info._std_type      = std_type_char;
    Xlat_text_type::_type_info._name          = "Xlat_text";
    Xlat_text_type::_type_info._max_size      = 1024; //32767
    Xlat_text_type::_type_info._max_precision = 1024; //32767
    Xlat_text_type::_type_info._exact_char_repr = true;
    Xlat_text_type::_type_info.normalize();
}

//---------------------------------------------------------------Xlat_text_type::Xlat_text_type

Xlat_text_type::Xlat_text_type( int field_size, const char* write_table, const Byte* r_table )
:
    Field_type    ( &_type_info, field_size ),
    _write_table  ( write_table )
{
    if( r_table )  read_table( r_table );
    _rtrim = true;
}

//-------------------------------------------------------------------------Xlat_text_type::null

Bool Xlat_text_type::null( const Byte* p ) const
{
    const Byte* p_end = p + _field_size;
    while( p < p_end )  if( *p++ != 0x00 )  return false;
    return true;
}

//--------------------------------------------------------------Xlat_text_type::_v_field_length

uint Xlat_text_type::_v_field_length( const Byte* p0, const Byte* p ) const
{
    uint len = p - p0;

    while(1) {
        len = length_without_trailing_char( (const char*)p0, len, _space );
        if( len == 0  || (char)p0[ len ] != '\0' )  break;
        len = length_without_trailing_char( (const char*)p0, len, '\0' );
        if( len == 0  || (char)p0[ len ] != ' ' )  break;
    }

    return len;
}

//------------------------------------------------------------------------Xlat_text_type::empty

Bool Xlat_text_type::empty( const Byte* p ) const
{
    const Byte* p_end = p + _field_size;
    if( *p == 0x00 )  {
    	while( p < p_end )  if( *p++ != 0x00   )  return false;
    } else {
    	Byte space = _space;
        while( p < p_end )  if( *p++ != space )  return false;
    }

    return true;
}

//-------------------------------------------------------------------Xlat_text_type::write_text

void Xlat_text_type::write_text( const Byte* p, Area* text, const Text_format& ) const
{
    if( p[0] == '\0' )  if( null( p ) )  { text->length(0); return; }

    uint len = _field_size;

    if( _rtrim ) {
        while(1) {
            len = length_without_trailing_char( (const char*)p, len, _space );
            if( len == 0  || (char)p[ len-1 ] != '\0' )  break;
            len = length_without_trailing_char( (const char*)p, len, '\0' );
            if( len == 0  || (char)p[ len-1 ] != ' ' )  break;
        }
    }

    text->allocate_length( len );
    xlat( text->ptr(), p, len, write_table() );
}

//--------------------------------------------------------------------Xlat_text_type::read_text

void Xlat_text_type::read_text( Byte* ptr, const char* text, const Text_format& ) const
{
    uint len = strlen( text );

    if( len > _field_size ) {                               // zu groß
        len = length_without_trailing_spaces( text, len );
        if( len > _field_size )  throw_xc( "SOS-1221", this, text );
    }

    xlat( ptr, text, len, read_table() );
    memset( ptr + len, 0x40, _field_size - len );  // Auffüllen
}

//-----------------------------------------------------------------Xlat_text_type::_get_param
/*
void Xlat_text_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
}
*/
//-------------------------------------------------------------------Xlat_text_type::_obj_print

void Xlat_text_type::_obj_print( ostream* s ) const
{
    *s << "xlat_text(" << field_size() << ')';
}

//------------------------------------------------------------------------Text_type::_type_info

Listed_type_info Text_type::_type_info;

SOS_INIT( text_type )
{
    Text_type::_type_info._std_type      = std_type_char;
    Text_type::_type_info._name          = "Text";
    Text_type::_type_info._max_size      = 32767;
    Text_type::_type_info._max_precision = 32767;
    Text_type::_type_info._exact_char_repr = true;
    Text_type::_type_info.normalize();
}

//----------------------------------------------------------------------Text_type::write_text

void Text_type::write_text( const Byte* ptr, Area* text, const Text_format& ) const
{
    if( ptr[ 0 ] == '\0' )  if( null( ptr ) )  { text->length(0); return; }  // Null wie leerer Wert (?)

    uint len = _field_size;

    if( _rtrim ) {
        while(1) {
            len = length_without_trailing_spaces( (const char*)ptr, len );
            if( len == 0  || (char)ptr[ len-1 ] != '\0' )  break;
            len = length_without_trailing_char( (const char*)ptr, len, '\0' );
            if( len == 0  || (char)ptr[ len-1 ] != ' ' )  break;
        }
    }

    text->assign( ptr, len );
}

//-----------------------------------------------------------------------Text_type::read_text

void Text_type::read_text( Byte* ptr, const char* text, const Text_format& ) const
{
    uint len = strlen( text );

    if( len > _field_size ) {                               // zu groß
        len = length_without_trailing_spaces( text, len );
        if( len > _field_size )  throw_xc( "SOS-1221", this, text );
    }

    memcpy( ptr, text, len );
    memset( ptr + len, ' ', _field_size - len );  // Auffüllen
}

//--------------------------------------------------------------------------Text_type::set_null

void Text_type::set_null( Byte* p ) const
{
    memset( p, '\0', _field_size );
}

//------------------------------------------------------------------------Text_type::_get_param
/*
void Text_type::_get_param( Type_param* param ) const
{
    param->_info_ptr = &_type_info;
}
*/
//------------------------------------------------------------------------Text_type::_obj_print

void Text_type::_obj_print( ostream* s ) const
{
    *s << "Text(" << _field_size << ")";
}

//-------------------------------------------------------------------Text_type::_v_field_length

uint Text_type::_v_field_length( const Byte* ptr, const Byte* end_ptr ) const
{
    uint len = end_ptr - ptr;

    while(1) {
        len = length_without_trailing_spaces( (const char*)ptr, len );
        if( len == 0  || (char)ptr[ len ] != '\0' )  break;
        len = length_without_trailing_char( (const char*)ptr, len, '\0' );
        if( len == 0  || (char)ptr[ len ] != ' ' )  break;
    }

    return len;
}

//-----------------------------------------------------------------------------Text_type::empty

Bool Text_type::empty( const Byte* p ) const
{
    return _v_field_length( p, p + field_size() ) == 0;    
}

//---------------------------------------------------------------------String0_type::_type_info

Listed_type_info String0_type::_type_info;

SOS_INIT( string0 )
{
    String0_type::_type_info._std_type      = std_type_varchar;
    String0_type::_type_info._name          = "String0";
    String0_type::_type_info._max_size      = 1024; //MAX_INT
    String0_type::_type_info._max_precision = 1024; //MAX_INT
    String0_type::_type_info._exact_char_repr = true;
    String0_type::_type_info.normalize();
}

//-------------------------------------------------------------------------String0_type::length

uint String0_type::length( const Byte* s ) const
{
    const char* z = (char*)memchr( s, '\0', _field_size );
    return z? z - (const char*)s : _field_size;   // Ohne 0-Byte (d.h. ein Zeichen mehr!) wegen SQLBindParameter cbValue. jz 28.7.97
}

//---------------------------------------------------------------------String0_type::field_copy

void String0_type::field_copy( Byte* p, const Byte* s ) const
{
    const char* z = (char*)memchr( s, '\0', _field_size );
    //if( !z )  throw_xc( "SOS-1221", this );
    //int len = z - (const char*)s;
    //jz 8.10.97 int len = z? z - (const char*)s : _field_size;   // Ohne 0-Byte (d.h. ein Zeichen mehr!) wegen SQLBindParameter cbValue. jz 28.7.97
    int len = z? z - (const char*)s + 1 : _field_size;   // Ohne 0-Byte (d.h. ein Zeichen mehr!) wegen SQLBindParameter cbValue. jz 28.7.97 //jz 8.10.97

    memcpy( (char*)p, (const char*)s, len );
    //jz 8.10.97  p[ len ] = '\0';  //jz 17.7.97

    //strncpy( (char*)p, (const char*)s, field_size() );  // Ruft strlen(), auch wenn '\0' fehlt!
    // kein memcpy wegen const_string0_type (mit unbekannter field_size)
}

//---------------------------------------------------------------------String0_type::write_text

void String0_type::write_text( const Byte* p, Area* text, const Text_format& ) const
{
    int len = field_size();
    const char* z = (const char*)memchr( p, '\0', len );
    if( z )  len = z - (const char*)p;

    if( _rtrim )  len = length_without_trailing_spaces( (const char*)p, len );

    text->assign( p, len );
}

//----------------------------------------------------------------------String0_type::read_text

void String0_type::read_text( Byte* ptr, const char* text, const Text_format& ) const
{
    int len = strlen( text );
    if( _rtrim )  len = length_without_trailing_spaces( text, len );
    //jz 8.10.97 if( len + 1 > _field_size )  throw_xc( "SOS-1221", this, text );
    if( len >= _field_size )  throw_xc( "SOS-1221", this, text );

    memcpy( ptr, text, len + 1 );
    //memset( ptr + len, '\0', _field_size - len );  // Auffüllen
    //if( len < _field_size )*/  ptr[ len ] = '\0';
}

//---------------------------------------------------------------String0_type::read_other_field

void String0_type::read_other_field( Byte* ptr, const Field_type* type, const Byte* q,
                                     Area*, const Text_format& format ) const
{
    Area area ( ptr, _field_size - 1 );
    type->write_text( q, &area, format );
    ptr[ area.length() ] = '\0';
}

//----------------------------------------------------------------String0_type::_v_field_length

uint String0_type::_v_field_length( const Byte* ptr, const Byte* end_ptr ) const
{
    int len = end_ptr - ptr;
    const char* z = (const char*)memchr( ptr, '\0', len );
    if( z )  len = z - (const char*)ptr;

    return len;
}

//----------------------------------------------------------------String0_type::_get_param

void String0_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
    param->_precision    = _field_size;  // - 1, wenn '\0' immer da sein muss (nicht ODBC)
}

//---------------------------------------------------------------------String0_type::_obj_print

void String0_type::_obj_print( ostream* s ) const
{
    *s << "String0(" << ( _field_size - 1 ) << ")";
}

//----------------------------------------------------------------------Binary_type::_type_info
/*
Listed_type_info Binary_type::_type_info;

SOS_INIT( binary_type )
{
    Binary_type::_type_info._std_type      = std_type_char;
    Binary_type::_type_info._name          = "Binary";
    Binary_type::_type_info._max_size      = 32767;
    Binary_type::_type_info._max_precision = 32767;
    Binary_type::_type_info._exact_char_repr = true;
    Binary_type::_type_info.normalize();
}

//----------------------------------------------------------------------Binary_type::write_text

void Binary_type::write_text( const Byte* ptr, Area* text, const Text_format& ) const
{
    for( int i = 0; i ...
    text->assign( ptr, length_without_trailing_spaces( (char*)ptr, _field_size ) );
}

//-----------------------------------------------------------------------Binary_type::read_text

void Binary_type::read_text( Byte* ptr, const char* text, const Text_format& ) const
{
    int len = length( text );

    if( len > _field_size ) {                               // zu groß
        len = length_without_trailing_spaces( text, _field_size );
        if( len > _field_size )  throw_xc( "SOS-1221", this, text );
    }

    memcpy( ptr, text, len );
    memset( ptr + len, ' ', _field_size - len );  // Auffüllen
}
*/
//------------------------------------------------------------------------Area_type::_type_info

Listed_type_info Area_type::_type_info;

SOS_INIT( area )
{
    Area_type::_type_info._std_type      = std_type_varchar;  //std_type_binary?
    Area_type::_type_info._name          = "Area";
    Area_type::_type_info._max_size      = sizeof(Area);   //?
    Area_type::_type_info._alignment     = long_alignment;
    Area_type::_type_info._max_precision = INT_MAX;
    Area_type::_type_info.normalize();
}

//-----------------------------------------------------------------------Area_type::field_equal

Bool Area_type::field_equal( const Byte* a, const Byte* b ) const
{
    return *(Area*)a == *(Area*)b;
}

//------------------------------------------------------------------------Area_type::write_text

void Area_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    buffer->assign( *(Area*)p );
}

//-------------------------------------------------------------------------Area_type::read_text

void Area_type::read_text( Byte* p, const char* text, const Text_format& ) const
{
    ((Area*)p)->assign( text );
}

//------------------------------------------------------------------------Area_type::_get_param
/*

void Area_type::_get_param( Type_param* ) const
{
    param->_info_ptr     = &_type_info;
    param->_precision    = 256;             //??????????? ist variable
}
*/
//------------------------------------------------------------------------Area_type::_obj_print

void Area_type::_obj_print( ostream* s ) const
{
    *s << "Area";
}

//-------------------------------------------------------------------Null_area_type::_obj_print

void Null_area_type::_obj_print( ostream* s ) const
{
    *s << "Null_area";
}

//------------------------------------------------------------------Sos_string_type::_type_info

Listed_type_info Sos_string_type::_type_info;

SOS_INIT( sos_string )
{
    Sos_string_type::_type_info._std_type      = std_type_varchar;
    Sos_string_type::_type_info._name          = "Sos_string";
    Sos_string_type::_type_info._max_size      = sizeof(Sos_string);
    Sos_string_type::_type_info._alignment     = long_alignment;
    Sos_string_type::_type_info._max_precision = INT_MAX;
    Sos_string_type::_type_info._field_copy_possible = false;
    Sos_string_type::_type_info.normalize();
}

//------------------------------------------------------------------------Sos_string_type::read

void Sos_string_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    buffer->assign( c_str( *(Sos_string*)p ), length( *(Sos_string*)p ) );
}

//-----------------------------------------------------------------------Sos_string_type::write

void Sos_string_type::read_text( Byte* p, const char* text, const Text_format& ) const
{
    *(Sos_string*)p = text;
}

//------------------------------------------------------------------Sos_string_type::_obj_print

void Sos_string_type::_obj_print( ostream* s ) const
{
    *s << "Sos_string";
}

//------------------------------------------------------------Little_endian_int_type::_type_info

Listed_type_info Little_endian_int_type::_type_info;

SOS_INIT( little_endian_int )
{
    Little_endian_int_type::_type_info._std_type      = std_type_integer;
    Little_endian_int_type::_type_info._name          = "little_endian_int";
    Little_endian_int_type::_type_info._max_size      = 8;
  //Little_endian_int_type::_type_info._alignment     = 8;
    Little_endian_int_type::_type_info._max_precision = 64;
    Little_endian_int_type::_type_info._radix         = 2;
    Little_endian_int_type::_type_info._display_size  = 1+19+2;   // Bei 64 bit mit Nachkommastellen
    Little_endian_int_type::_type_info.normalize();
}

//----------------------------------------------------------Little_endian_int_type::write_text

void Little_endian_int_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    uint64 a = 0;
    int64  b;

    for( int i = 0; i < _field_size; i++ ) {
        a <<= 8;
        a |= p[i];
    }

    // Vorzeichenbit ziehen
    int sign_bits = 64 - _field_size * 8;
    a <<= sign_bits;      
    b = a;
    b >>= sign_bits;
    int64_type.write_text( (Byte*)&b, buffer, format );
}

//-----------------------------------------------------------Little_endian_int_type::read_text

void Little_endian_int_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    uint64 a;

    int64_type.read_text( (Byte*)&a, t, format );

    for( int i = _field_size - 1; i >= 0; i-- ) {
        p[i] = (Byte)a;
        a >>= 8;
    }
}

//------------------------------------------------------------Little_endian_int_type::op_compare

int Little_endian_int_type::op_compare( const Byte* a, const Byte* b ) const
{
    Byte a_sign = a[0] & 0x80;
    Byte b_sign = b[0] & 0x80;
    
    if(  a_sign && !b_sign )  return -1;    // compare( a<0, b>=0 )
    if( !a_sign &&  b_sign )  return +1;    // compare( a>=0, b<0 )

    int result = memcmp( a, b, _field_size );
    return a_sign? -result : result;
}

//-------------------------------------------------------------Little_endian_int_type::_obj_print

void Little_endian_int_type::_obj_print( ostream* s ) const
{
    *s << "Little_endian_int(" << ( _field_size * 8 );
    *s << ')';
}

//------------------------------------------------------------Ulittle_endian_int_type::_type_info

Listed_type_info Ulittle_endian_int_type::_type_info;

SOS_INIT( ulittle_endian_int )
{
    Ulittle_endian_int_type::_type_info._std_type      = std_type_integer;
    Ulittle_endian_int_type::_type_info._name          = "little_endian_int";
    Ulittle_endian_int_type::_type_info._max_size      = 8;
  //Ulittle_endian_int_type::_type_info._alignment     = 8;
    Ulittle_endian_int_type::_type_info._max_precision = 64;
    Ulittle_endian_int_type::_type_info._radix         = 2;
    Ulittle_endian_int_type::_type_info._unsigned      = true;
    Ulittle_endian_int_type::_type_info.normalize();
}

//----------------------------------------------------------Ulittle_endian_int_type::write_text

void Ulittle_endian_int_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    uint64 a = 0;

    for( int i = 0; i < _field_size; i++ ) {
        a <<= 8;
        a |= p[i];
    }

    uint64_type.write_text( (Byte*)&a, buffer, format );
}

//-----------------------------------------------------------Ulittle_endian_int_type::read_text

void Ulittle_endian_int_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    uint64 a;

    uint64_type.read_text( (Byte*)&a, t, format );

    for( int i = _field_size - 1; i >= 0; i-- ) {
        p[i] = (Byte)a;
        a >>= 8;
    }
}

//-----------------------------------------------------------Ulittle_endian_int_type::op_compare

int Ulittle_endian_int_type::op_compare( const Byte* a, const Byte* b ) const
{
    return memcmp( a, b, _field_size );
}

//------------------------------------------------------------Ulittle_endian_int_type::_obj_print

void Ulittle_endian_int_type::_obj_print( ostream* s ) const
{
    *s << "unsigned Little_endian_int(" << ( _field_size * 8 );
    *s << ')';
}

//------------------------------------------------------Scaled_little_endian_int_type::_type_info

Listed_type_info Scaled_little_endian_int_type::_type_info;

SOS_INIT( scaled_little_endian_int )
{
    Scaled_little_endian_int_type::_type_info._std_type      = std_type_decimal;
    Scaled_little_endian_int_type::_type_info._name          = "scaled_little_endian_int";
    Scaled_little_endian_int_type::_type_info._max_size      = 8;
  //Scaled_little_endian_int_type::_type_info._alignment     = 8;
    Scaled_little_endian_int_type::_type_info._max_precision = 64;
    Scaled_little_endian_int_type::_type_info._radix         = 2;
    Scaled_little_endian_int_type::_type_info._display_size  = 1+19+2;   // Bei 64 bit mit Komma
    Scaled_little_endian_int_type::_type_info._max_scale     = 19;       // Bei 64 bit
    Scaled_little_endian_int_type::_type_info.normalize();
}

//-----------------------------------------------------Scaled_little_endian_int_type::write_text

void Scaled_little_endian_int_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    uint64 a = 0;
    int64  b;

    for( int i = 0; i < _field_size; i++ ) {
        a <<= 8;
        a |= p[i];
    }

    // Vorzeichenbit ziehen
    int sign_bits = 64 - _field_size * 8;
    a <<= sign_bits;      
    b = a;
    b >>= sign_bits;

    Scaled_int64_type( _scale ).write_text( (Byte*)&b, buffer, format );
}

//------------------------------------------------------Scaled_little_endian_int_type::read_text

void Scaled_little_endian_int_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    uint64 a;

    Scaled_int64_type( _scale ).read_text( (Byte*)&a, t, format );

    for( int i = _field_size - 1; i >= 0; i-- ) {
        p[i] = (Byte)a;
        a >>= 8;
    }
}

//-------------------------------------------------------Scaled_little_endian_int_type::op_compare

int Scaled_little_endian_int_type::op_compare( const Byte* a, const Byte* b ) const
{
    Byte a_sign = a[0] & 0x80;
    Byte b_sign = b[0] & 0x80;
    
    if(  a_sign && !b_sign )  return -1;    // compare( a<0, b>=0 )
    if( !a_sign &&  b_sign )  return +1;    // compare( a>=0, b<0 )

    int result = memcmp( a, b, _field_size );
    return a_sign? -result : result;
}

//-------------------------------------------------------Scaled_little_endian_int_type::_get_param

void Scaled_little_endian_int_type::_get_param( Type_param* param ) const
{
    param->_scale = _scale;
}

//-------------------------------------------------------Scaled_little_endian_int_type::_obj_print

void Scaled_little_endian_int_type::_obj_print( ostream* s ) const
{
    *s << "Little_endian_int(" << ( _field_size * 8 );
    *s << " scale=" << _scale;
    *s << ')';
}

//------------------------------------------------------Scaled_ulittle_endian_int_type::_type_info

Listed_type_info Scaled_ulittle_endian_int_type::_type_info;

SOS_INIT( scaled_ulittle_endian_int )
{
    Scaled_ulittle_endian_int_type::_type_info._std_type      = std_type_decimal;
    Scaled_ulittle_endian_int_type::_type_info._name          = "scaled_little_endian_int";
    Scaled_ulittle_endian_int_type::_type_info._max_size      = 8;
  //Scaled_ulittle_endian_int_type::_type_info._alignment     = 8;
    Scaled_ulittle_endian_int_type::_type_info._max_precision = 64;
    Scaled_ulittle_endian_int_type::_type_info._radix         = 2;
    Scaled_ulittle_endian_int_type::_type_info._unsigned      = true;
    Scaled_ulittle_endian_int_type::_type_info.normalize();
}

//----------------------------------------------------------Scaled_ulittle_endian_int_type::write_text

void Scaled_ulittle_endian_int_type::write_text( const Byte* p, Area* buffer, const Text_format& format ) const
{
    uint64 a = 0;

    for( int i = 0; i < _field_size; i++ ) {
        a <<= 8;
        a |= p[i];
    }

    Scaled_uint64_type( _scale ).write_text( (Byte*)&a, buffer, format );
}

//-----------------------------------------------------------Scaled_ulittle_endian_int_type::read_text

void Scaled_ulittle_endian_int_type::read_text( Byte* p, const char* t, const Text_format& format ) const
{
    uint64 a;

    Scaled_uint64_type( _scale ).read_text( (Byte*)&a, t, format );

    for( int i = _field_size - 1; i >= 0; i-- ) {
        p[i] = (Byte)a;
        a >>= 8;
    }
}

//---------------------------------------------------------op_Scaled_ulittle_endian_int_type::op_compare

int Scaled_ulittle_endian_int_type::op_compare( const Byte* a, const Byte* b ) const
{
    return memcmp( a, b, _field_size );
}

//------------------------------------------------------------Scaled_ulittle_endian_int_type::_get_param

void Scaled_ulittle_endian_int_type::_get_param( Type_param* param ) const
{
    param->_scale = _scale;
}

//------------------------------------------------------------Scaled_ulittle_endian_int_type::_obj_print

void Scaled_ulittle_endian_int_type::_obj_print( ostream* s ) const
{
    *s << "unsigned Little_endian_int(" << ( _field_size * 8 );
    *s << " scale=" << _scale;
    *s << ')';
}

//---------------------------------------------------------------------------Method_type::call

void Void_method_type::call( Void_method_ptr f, Sos_object_base* object ) const
{
    (object->*f)();
}

//-----------------------------------------------------------------Void_method_type::_obj_print

void Void_method_type::_obj_print( ostream* s ) const
{
    *s << "simple_method";
}

/*

void Method_type::call( Any_method_ptr f, Sos_object_base* object, void* result, void* params )
{
    //"result = object->f( params )"
    if( _result_type ) {
        throw_xc( "Method_type::call" );
    }
    else
    {
        switch( _param_type->field_count() )
        {
            case 0 : call( f, object ); break;
            case 1 : call( f, object, params ); break;
            default: throw_xc( "Method_type::call" );
        }
    }
}

//void Method_type::call( Any_method_ptr f, Sos_object_base* object )
//{
//    (object->*f)();
//}

*/


} //namespace sos
