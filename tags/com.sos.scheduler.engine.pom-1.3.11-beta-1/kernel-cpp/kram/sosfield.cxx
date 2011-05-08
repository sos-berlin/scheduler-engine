// $Id$

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

#include "precomp.h"

#include <stdio.h>          // sprintf in Array_type
#include <limits.h>
#include <sstream>

#include "sysdep.h"

#include "sosstrng.h"
#include "sos.h"
#include "log.h"
#include "ctype.h"
#include "sosopt.h"         // Sos_token_iterator
#include "sosarray.h"
#include "soslimtx.h"
#include "sosdate.h"
#include "xlat.h"

#include "stdfield.h"       // Int_type
#include "dynobj.h"

using namespace std;
namespace sos {


DEFINE_SOS_STATIC_PTR( Field_type )
DEFINE_SOS_STATIC_PTR( Record_type )
//DEFINE_SOS_STATIC_PTR( Field_descr )
typedef Sos_ptr<Field_descr> _instantiate_Field_descr_;

//----------------------------------------------------------------------------------------const

const Type_info                 default_type_info;
const extern Text_format        std_text_format         = Text_format();

//---------------------------------------------------------------------------------------static

//?const Field_descr* current_field_descr_ptr = 0;  // Dummy

Text_format                     raw_text_format;
Listed_type_info*               Listed_type_info::_head;
static char                     names_equal_table       [ 256 ];

//---------------------------------------------------------------------------------------------

SOS_INIT( sosfield )
{
    int i;
    for( i = 0; i < 256; i++ )  names_equal_table[ i ] = (char)i;
    for( i = 'a'; i <= 'z'; i++   )  names_equal_table[ i ] = 'A' - 'a' + (char)i;
    names_equal_table[ (int)'-' ] = '_';

  //raw_text_format.string_quote( 0 );
  //raw_text_format.char_quote  ( 0 );
  //raw_text_format.separator   ( 0 );
    raw_text_format.raw         ( true );
}

//------------------------------------------------------------------------field_names_are_equal

Bool field_names_are_equal( const char* a, const char* b )
{
    if( *a == '_' )  a++;   // Unterstrich am Anfang ignorieren
    if( *b == '_' )  b++;

    while( *a && names_equal_table[ (uint)*a ] == names_equal_table[ (uint)*b ] ) { a++; b++; }
    return *a == *b;  // nur bei *a = *b = '\0'
}

//-------------------------------------------------------------------------Type_info::Type_info

Type_info::Type_info()
:
    _zero_ ( this + 1 )
{
    //*this = default_type_info;
    _std_type            = std_type_none;
    _name                = "Field_type";
    _quote               = true;
    _unsigned            = false;
    _nullable            = false;
    _max_size            = 0;
    _max_precision       = 0;
    _min_scale           = 0;
    _max_scale           = 0;
    _exact_char_repr     = false;
    _field_copy_possible = true;
    _alignment           = 1;
}

//-------------------------------------------------------------------------Type_info::Type_info
/*
Type_info::Type_info( const Type_info& o )
:
    _zero_(this+1)
{
    *this = o;
}
*/
//------------------------------------------------------------------------Type_info::~Type_info

Type_info::~Type_info()
{
}

//------------------------------------------------------------------------Type_info::operator =
/*
Type_info& Type_info::operator = ( const Type_info& o )
{
    _std_type               = o._std_type;
    _name                   = o._name;
    _unsigned               = o._unsigned;
    _nullable               = o._nullable;
    _max_size               = o._max_size;
    _max_precision          = o._max_precision;
    _radix                  = o._radix;
    _min_scale              = o._min_scale;
    _max_scale              = o._max_scale;
    _quote                  = o._quote;
    _exact_char_repr        = o._exact_char_repr;
    _field_copy_possible    = o._field_copy_possible;
    _tail                   = NULL;

    return *this;
}
*/
//------------------------------------------------------------------Type_info::max_precision_10

int Type_info::max_precision_10() const
{
    return _radix == 2? (   _max_precision <= 1 * 8? 3
                          : _max_precision <= 2 * 8? 5
                          : _max_precision <= 4 * 8? 10
                          : _max_precision * 3
                        ) + !_unsigned
                      : _max_precision;
}

//-------------------------------------------------------------------------Type_info::normalize

void Type_info::normalize()
{
    if( _std_type == std_type_char
     || _std_type == std_type_varchar
     || _std_type == std_type_date    )
    {
        _quote = true;
    }

    if( is_numeric( _std_type ) ) {
        _quote = false;
        if( !_radix )  _radix = 10;
    }
}

//-----------------------------------------------------------Listed_type_info::Listed_type_info

Listed_type_info::Listed_type_info()
:
    _tail ( NULL )
{
}

//----------------------------------------------------------Listed_type_info::~Listed_type_info

Listed_type_info::~Listed_type_info()
{
/*  Soll die Liste doch bleiben, wo sie ist.
    if( this == _head ) {
        _head = _tail;
    }
    else
    {
        Listed_type_info* t = _head;
        while( t  &&  t->_tail != this )  t = t->_tail;   // Vorgänger suchen
        if( t )  t->_tail = _tail;
    }
*/
}

//------------------------------------------------------------------Listed_type_info::normalize

void Listed_type_info::normalize()
{
    Type_info::normalize();

    if( !_tail  &&  _head != this ) {
        _tail = _head;
        _head = this;
    }
}

//---------------------------------------------------------------------Type_param::precision_10

int Type_param::precision_10() const
{
    return _radix == 2? (   _precision <= 1 * 8? 3
                          : _precision <= 2 * 8? 5
                          : _precision <= 4 * 8? 10
                          : _precision * 3
                        )
                      : _precision;
}

//------------------------------------------------------------------------Field_type::get_param

void Field_type::get_param( Type_param* param ) const
{
    Bool is_record = obj_is_type( tc_Record_type );

    param->_std_type     = std_type_none;
    param->_size         = field_size();
    param->_precision    = is_record? 0 : _info->_radix? _info->_max_precision : param->_size;    // wird in der Regel von _get_param() überschrieben
    param->_radix        = _info->_radix;
    param->_display_size = _info->_display_size;
    param->_scale        = _info->_min_scale;  // oder _max_scale?
    param->_unsigned     = _info->_unsigned;
    param->_info_ptr     = _info;//&default_type_info;

    _get_param( param );

    if( !param->_std_type  )  param->_std_type = param->_info_ptr->_std_type;

    if( !param->_display_size  &&  param->_precision )
    {
        if( param->_radix == 2 ) {
            if( param->_precision <= 1*8 )  param->_display_size = 3;
            else
            if( param->_precision <= 2*8 )  param->_display_size = 5;
            else
            if( param->_precision <= 4*8 )  param->_display_size = 10;
            else
            if( param->_precision <= 8*8 )  param->_display_size = 20;
            else                            param->_display_size = 100;
        } else
        if( !is_record ) {
            param->_display_size = param->_precision;
        }

        if( sos::is_numeric( param->_std_type ) ) {
            if( !param->_unsigned )  param->_display_size++;    // Vorzeichen
            if( param->_scale     )  param->_display_size++;    // Dezimalkomma
        }
    }
    else
    if( !param->_precision && param->_display_size && !param->_radix ) {
        param->_precision = param->_display_size;
    }
}

//-----------------------------------------------------------------------Field_type::_get_param

void Field_type::_get_param( Type_param* ) const
{
    //LOG( *this << "::_get_param nicht implementiert\n" );
}

//----------------------------------------------------------------------Field_type::~Field_type
#if defined SYSTEM_SOLARIS

Field_type::~Field_type()
{
    // Für Solaris 4.0.1 (?) jz 8.11.95
}

#endif
//------------------------------------------------------------------------------Field_type::sql

//Bool Field_type::sql() const   { return false; }   //??????????????

//-------------------------------------------------------------------------Field_type::nullable

Bool Field_type::nullable() const
{
    return false;
}

//-----------------------------------------------------------------------------Field_type::null

Bool Field_type::null( const Byte* ) const
{
    return false;
}

//-------------------------------------------------------------------------Field_type::set_null

void Field_type::set_null( Byte* ) const
{
    throw_null_error( "SOS-1186", this );
}

//----------------------------------------------------------------------------Field_type::empty

Bool Field_type::empty( const Byte* p ) const
{
    if( null( p ) )  return true;

    Std_type std_type = info()->_std_type;
    if( std_type == std_type_char  ||  std_type == std_type_varchar )  return _empty( p );

    return false;
}

//---------------------------------------------------------------------------Field_type::_empty

Bool Field_type::_empty( const Byte* p ) const
{
    Dynamic_area text ( max_display_field_size );
    write_text( p, &text );
    return length_without_trailing_spaces( text.char_ptr(), text.length() ) == 0;
}

//--------------------------------------------------------------------Field_type::empty_is_null

Bool Field_type::empty_is_null() const
{
    return false;
}

//------------------------------------------------------------------------Field_type::construct

void Field_type::construct( Byte* p ) const
{
    clear( p );
}

//-------------------------------------------------------------------Field_type::field_destruct

void Field_type::destruct( Byte* ) const
{
}

//-----------------------------------------------------------------------Field_type::field_copy

void Field_type::field_copy( Byte* p, const Byte* s ) const
{
    memcpy( p, s, field_size() );
}

//----------------------------------------------------------------------Field_type::field_equal

Bool Field_type::field_equal( const Byte* a, const Byte* b ) const
{
    return memcmp( a, b, field_size() ) == 0;
}

//----------------------------------------------------------------------------Field_type::clear

void Field_type::clear( Byte* p ) const
{
    if( nullable() )  set_null( p );
                else  memset( p, 0, field_size() );   // VORSICHT!
}

//------------------------------------------------------------------------Field_type::alignment

int Field_type::alignment() const
{
    //return 1;
    return _info->_alignment;
}

//-----------------------------------------------------------------------Field_type::op_compare

int Field_type::op_compare( const Byte* a, const Byte* b ) const
{
    switch( _info->_std_type )
    {
        case std_type_bool:
        case std_type_decimal:  
        case std_type_integer:
        {
            if( _info->_min_scale == 0 )
            {
                Big_int A = this->as_big_int(a);
                Big_int B = this->as_big_int(b);
                return A < B? -1 :
                       A > B? +1 : 0;
            }
        }

        case std_type_float:
        {
            double A = this->as_double(a);
            double B = this->as_double(b);
            return A < B? -1 :
                   A > B? +1 : 0;
        }

        case std_type_date:
        case std_type_time:
        case std_type_date_time:
        {
            Sos_optional_date_time A = this->as_string(a);
            Sos_optional_date_time B = this->as_string(b);
            return A.compare(B);
        }
        
        case std_type_char:
        case std_type_varchar:
        {
            Dynamic_area A;
            Dynamic_area B;
            write_text( a, &A );
            write_text( b, &B );
            return A.compare(B);
        }

        default: 
            return memcmp( a, b, field_size() );
    }
}

//---------------------------------------------------------------------------Field_type::op_add

void Field_type::op_add( Byte* a, const Byte* b ) const
{
    switch( _info->_std_type )
    {
        case std_type_decimal:  
        case std_type_integer:
        {
            if( _info->_min_scale == 0 )
            {
                char text[100];
                sprintf( text, "%" PRINTF_LONG_LONG "d", this->as_big_int(a) + this->as_big_int(b) );
                read_text( a, text );
            }
        }

        case std_type_bool:
        case std_type_date:
        case std_type_time:
        case std_type_date_time:
        {
            throw_xc( "SOS-1432", _info->_name );
        }

        case std_type_float:
        default:
        {
            char text[100];
            sprintf( text, "%g", this->as_double(a) + this->as_double(b) );
            read_text( a, text );
        }
    }
}

//-----------------------------------------------------------------------Field_type::write_text

void Field_type::write_text( const Byte*, Area*, const Text_format& ) const
{
    throw_xc( "Field_type::write_text", this );
/*
    Dynamic_area buffer ( max_display_field_size );
    //?Sos_limited_text<max_display_field_size>
    strstream s ( buffer.char_ptr(), buffer.size() );
    print( p, s, f );
    a->assign( a, s->pcount() );
*/
}

//----------------------------------------------------------------------------Field_type::print

void Field_type::print( const Byte* p, ostream* s, const Text_format& f,
                        char quote, char quote_quote ) const
{
    Dynamic_area buffer;
    write_text( p, &buffer, f );
    if( _info->_quote  &&  quote )  print_string( buffer, s, quote, quote_quote );
                              else  *s << buffer;
}

//--------------------------------------------------------------------Field_type::write_variant
#if 0 // defined SYSTEM_WIN  //SYSTEM_OLE

void Field_type::write_variant( const Byte* p, VARIANT* v, Area* hilfspuffer, uint4 flags ) const
{
    write_text( p, hilfspuffer, f );

    VariantInit( v );

    switch( _info->_std_type )
    {
        case std_type_decimal:
        case std_type_integer:
        case std_type_float:
        case std_type_date:
        case std_type_time:
        case std_type_date_time:
        case std_type_bool:
        case std_type_char:
        case std_type_varchar:
        default:
        {
            V_VT( v ) = VT_BSTR;
            V_BSTR( v ) = SysAllocStringLen( NULL, _hilfspuffer.length() );
            if( !V_BSTR( v ) )  throw_no_memory_error();
            int wlen = MultiByteToWideChar( CP_ACP, 0,
                                            _hilfspuffer.char_ptr(), _hilfspuffer.length(),
                                            V_BSTR( v ), _hilfspuffer.length() );
            if( wlen == 0 )  throw_mswin_error( "MultiByteToWideChar" );

            if( wlen != _hilfspuffer.length() ) {
                HRESULT hr = SysReAllocStringLen( v, V_BSTR( v ), _hilfspuffer.length() );
                if( FAILED( hr ) ) throw_hresult_error( hr, "SysReAllocStringLen" );
            }
        }
    }
}
/*
            VB-Typen:
    VT_EMPTY	= 0,
	VT_NULL	= 1,
	VT_I2	= 2,
	VT_I4	= 3,
	VT_R4	= 4,
	VT_R8	= 5,
	VT_CY	= 6,
	VT_DATE	= 7,
	VT_BSTR	= 8,
*/
#endif
//----------------------------------------------------------------------------Field_type::input

void Field_type::input( Byte* p, istream* s, const Text_format& f ) const
{
    Dynamic_area buffer ( max_display_field_size );

    if( f.separator() ) {
        while( s->peek() != f.separator()  &&  s->peek() != EOF )  buffer += (char)s->get();
    } else {
        s->read( buffer.char_ptr(), buffer.size() );
        buffer.length( s->gcount() );
        if( s->peek() != EOF ) {
            char c [30];
            s->read( c, sizeof c );
            Syntax_error x ( "SOS-1196" );
            x.insert( c, s->gcount() );
            throw_xc( x );
        }
    }

    buffer += '\0';

    read_text( p, buffer.char_ptr(), f );
}

//---------------------------------------------------------------------------Field_type::assign

void Field_type::assign( Byte* p, const Dyn_obj& o, Area* hilfspuffer ) const
{
    if( o.null() )  set_null( p );
              else  copy_value( this, p, o.type(), o.ptr(), hilfspuffer );
}

//----------------------------------------------------------------------------Field_type::as_string

string Field_type::as_string( const Byte* p ) const
{
    Dynamic_area buffer;
    write_text( p, &buffer );
    return ::sos::as_string( buffer.char_ptr(), buffer.length() );
}

//---------------------------------------------------------------------------Field_type::as_int

int Field_type::as_int( const Byte* p ) const
{
/*
    Sos_limited_text<50> text;

    write_text( p, &text );
    return sos::as_int( text.c_str() );
*/
    Big_int i = as_big_int(p);
    if( i >  (Big_int)INT_MAX   )  goto OVERFLW;
    if( i < -(Big_int)INT_MAX-1 )  goto OVERFLW;
    return (int)i;

  OVERFLW:
    string str = sos::as_string(i);
    throw_overflow_error( "SOS-1107", "int", str.c_str() );
    return 0;
}

//-----------------------------------------------------------------------------Field_type::as_int64

int64 Field_type::as_int64( const Byte* p ) const
{
    Sos_limited_text<50> text;

    write_text( p, &text );
    return sos::as_int64( c_str(text) );
}

//-----------------------------------------------------------------------Field_type::as_big_int

Big_int Field_type::as_big_int( const Byte* p ) const
{
    Sos_limited_text<50> text;

    write_text( p, &text );
    return sos::as_big_int( c_str(text) );
}

//---------------------------------------------------------------------Field_type::as_date_time
/*
Sos_optional_date_time Field_type::as_date_time( const Byte* p ) const
{
    Sos_limited_text<50> text;

    write_text( p, &text );
    return text.c_str();
}
*/
//------------------------------------------------------------------------Field_type::as_double

double Field_type::as_double( const Byte* p ) const
{
    Sos_limited_text<100> buffer;
    write_text( p, &buffer );
    return c_str_as_double( c_str( buffer ) );
}

//-----------------------------------------------------------------Field_type::read_other_field

void Field_type::read_other_field( Byte* p, const Field_type* type, const Byte* q,
                                   Area* hilfspuffer, const Text_format& format ) const
{
    // Text_format = std_text_format für MS Access 2.0, weil dort (char*)"00" != (short)0

    if( !type  ||  type->null( q ) ) {         // ist NULL nicht schon behandelt?
        set_null( p );
    } else {
        type->write_text( q, hilfspuffer, format );
        *hilfspuffer += '\0';
        read_text( p, hilfspuffer->char_ptr(), format );
    }
}

//-----------------------------------------------------------------Field_subtype::Field_subtype

Field_subtype::Field_subtype( const Sos_ptr<Field_type>& base_type )
:
    Field_type     ( base_type->info(), base_type->field_size() ),
    _base_type     ( base_type ),
    _type_info     ( *base_type->info() ),
    _empty_is_null ( false )
{
    _type_info._field_copy_possible = _base_type->info()->_field_copy_possible;
}

//-----------------------------------------------------------------Field_subtype::Field_subtype

Field_subtype::Field_subtype( const Type_info* info, const Sos_ptr<Field_type>& base_type )
:
    Field_type     ( info, base_type->field_size() ),
    _base_type     ( base_type ),
  //_type_info     ( *info ),
    _empty_is_null ( false )
{
    _type_info._field_copy_possible = _base_type->info()->_field_copy_possible;   //?
}

//-----------------------------------------------------------------------Field_type::_obj_print

void Field_type::_obj_print( ostream* s ) const
{
    *s << info()->_name;
}

//----------------------------------------------------------------------Field_subtype::nullable

Bool Field_subtype::nullable() const
{
    return _empty_is_null  ||  _base_type->nullable();
}

//--------------------------------------------------------------------------Field_subtype::null

Bool Field_subtype::null( const Byte* p ) const
{
    if( _base_type->null( p ) )  return true;
    if( _empty_is_null )  return _empty( p );
    return false;
}

//----------------------------------------------------------------------Field_subtype::set_null

void Field_subtype::set_null( Byte* p ) const
{
    if( _empty_is_null ) {
        _base_type->read_text( p, "" );
    } else {
        _base_type->set_null( p );
    }
}

//-----------------------------------------------------------------Field_subtype::empty_is_null

Bool Field_subtype::empty_is_null() const
{
    return _empty_is_null;
}

void Field_subtype::construct       ( Byte* p ) const       { _base_type->construct( p ); }
void Field_subtype::destruct        ( Byte* p ) const       { _base_type->destruct( p ); }
void Field_subtype::clear           ( Byte* p ) const       { _base_type->clear( p ); }
Bool Field_subtype::field_equal     ( const Byte* a, const Byte* b ) const { return _base_type->field_equal( a, b ); }
int  Field_subtype::alignment       () const                { return _base_type->alignment(); }

//--------------------------------------------------------------------Field_subtype::_obj_print

void Field_subtype::_obj_print( ostream* s ) const
{
    *s << info()->_name << '(' << *_base_type << ')';
}

//---------------------------------------------------------------------Field_descr::Field_descr

Field_descr::Field_descr()
:
    _offset   ( -1 ),
    _null_flag_offset( -1),
    _flags    (0),
    _type_ptr ( 0 ),
    _read_only( false ),
    _write_null_as_empty ( false ),
    _precision(0)
{
    obj_const_name( "Field_descr" );
}

//---------------------------------------------------------------------Field_descr::Field_descr

Field_descr::Field_descr( const Sos_ptr<Field_type>& type,
                          const char*                name_ptr,
                          long                       offset,
                          long                       null_flag_offset,
                          Field_descr_flags          flags )
:
    _name     (name_ptr),
    _offset   (offset),
    _null_flag_offset(null_flag_offset),
    _flags    (flags),
    _type_ptr ( type ),
    _read_only( false ),
    _write_null_as_empty ( false ),
    _precision(0)
{
    obj_const_name( "Field_descr" );
    if( !type )  throw_xc( "SOS-1194", name_ptr );
}

//---------------------------------------------------------------------Field_descr::Field_descr

Field_descr::Field_descr( const Field_descr& o )
:
    _offset             ( o._offset ),
    _null_flag_offset   ( o._null_flag_offset ),
    _flags              ( o._flags ),
    _type_ptr           ( o._type_ptr ),
    _read_only          ( o._read_only ),
    _name               ( o._name ),
    _write_null_as_empty ( o._write_null_as_empty ),
    _precision          ( o._precision )
{
    obj_const_name( "Field_descr" );
}

//--------------------------------------------------------------------Field_descr::~Field_descr

Field_descr::~Field_descr()
{
}

//--------------------------------------------------------------------------Field_descr::create

Sos_ptr<Field_descr> Field_descr::create()
{
    return SOS_NEW( Field_descr );
}

//--------------------------------------------------------------------------Field_descr::create

Sos_ptr<Field_descr> Field_descr::create( const Field_descr& field )
{
    return SOS_NEW( Field_descr( field ) );
}

//---------------------------------------------------------------------Field_descr::Field_descr

Field_descr& Field_descr::operator = ( const Field_descr& o )
{
    _offset              = o._offset;
    _null_flag_offset    = o._null_flag_offset;
    _flags               = o._flags;
    _type_ptr            = o._type_ptr;
    _read_only           = o._read_only;
    _name                = o._name;
    _write_null_as_empty = o._write_null_as_empty;
    return *this;
}

//----------------------------------------------------------------------------Field_descr::null

Bool Field_descr::null( const Byte* p ) const         
{ 
    if( has_null_flag() && null_flag( p ) )  return true;
    if( !type_ptr()  )  return true;

    // jz 6.6.01: Wenn der Aufrufer (Microsoft Access) "" als NULL interpretiert, dann soll auch "" = NULL gelten. (Für Fehrmann, Dia-Nielsen)
    if( _write_null_as_empty )  return simple_type()->empty( const_ptr( p ) );

    return type().null( const_ptr( p ) );
}

//------------------------------------------------------------------------Field_descr::set_null

void Field_descr::set_null( Byte* p ) const
{
    if( has_null_flag() )  set_null_flag( p, true );
    else
    if( _write_null_as_empty )  read_text( p, "" );
    else {
        if( !type().nullable() )  throw_null_error( "SOS-1186", this );  // Hier wird der Feldname mit angezeigt
        type().set_null( ptr( p ) );
    }
}

//--------------------------------------------------------------------------Field_descr::add_to

void Field_descr::add_to( Record_type* record_type_ptr )
{
    if( _offset == -1 ) 
    {
        int4 offs = round_up( record_type_ptr->field_size(), type().alignment() );
        
        if( offs + type().field_size() >= (uint4)INT_MAX ) {
            //LOG( "Field_descr::add_to size=" << record_type_ptr->field_size() << " field_size=" << type().field_size() << '\n' );
            throw_xc( "SOS-1343", this, record_type_ptr );
        }

        _offset = offs;
    }

    record_type_ptr->add_field( this );
}

//----------------------------------------------------------------Field_descr::add_null_flag_to

void Field_descr::add_null_flag_to( Record_type* record_type_ptr )
{
    if( type().nullable() )  throw_xc( "SOS-1187", this );

    _null_flag_offset = round_up( record_type_ptr->field_size(), sizeof (Bool)/*alignment*/ );
    record_type_ptr->_field_size = _null_flag_offset + sizeof (Bool);
}

//---------------------------------------------------------------------------Field_descr::print
// s.a. write_text

void Field_descr::print( const Byte* p, ostream* s, const Text_format& f,
                         char quote, char quote_quote ) const
{
    if( has_null_flag()  &&  null_flag( p ) )  return;

    try {
        type().print( const_ptr( p ), s, f, quote, quote_quote );
    }
    catch( Xc& x )
    {
        _xc_insert_field_name( &x );
        throw;
    }
}

//-------------------------------------------------------------------Field_descr::print_shorted
// s.a. write_text_shorted

void Field_descr::print_shorted( const Const_area& a, ostream* s, const Text_format& f,
                                 char quote, char quote_quote ) const
{
    //if( has_null_flag()  &&  null_flag( a.byte_ptr() ) )  return;

    try {
        Dynamic_area value;

        int len = a.length() - offset();

        if( len <= 0  ||  (uint)len >= type().field_size() )  throw_xc( "print_shorted" );

        value.allocate_min( type().field_size() );
        memcpy( value.ptr(), const_ptr( a.byte_ptr() ), len );
        memset( value.byte_ptr() + len, '\0', type().field_size() - len );

        type().print( value.byte_ptr(), s, f, quote, quote_quote );
    }
    catch( Xc& x )
    {
        _xc_insert_field_name( &x );
        throw;
    }
}

//---------------------------------------------------------------------------Field_descr::print
// s.a. write_text

void Field_descr::print( const Const_area& a, ostream* s, const Text_format& f,
                         char quote, char quote_quote ) const
{
    if( !type_ptr() )  return;  // NULL

    if( offset() + type().field_size() > a.length() )
    {
        if( a.length() <= offset() )  return;  // NULL
        if( !type().info()->_exact_char_repr )  throw_xc( "SOS-1246", c_str( name() ), a.length() );
        print_shorted( a, s, f, quote, quote_quote );
    }
    else
    {
        print( a.byte_ptr(), s, f, quote, quote_quote );
    }
}

//-----------------------------------------------------------Field_descr::_xc_insert_field_name

void Field_descr::_xc_insert_field_name( Xc* x ) const
{
/*
    ostrstream s;
    s << *this;
    x->insert( "Feld " + string( s.str(), s.pcount() ) );
*/
    ostringstream s;
    s << *this;
    x->insert( "Feld " + s.str() );
}

//---------------------------------------------------------------------------Field_descr::input

void Field_descr::input( Byte* p, istream* s, const Text_format& f ) const
{
    try {
        if( has_null_flag() )  set_null_flag( p, false );

        if( ( s->peek() == EOF || s->peek() == f.separator() ) && nullable() ) {
            try {
                type().input( ptr( p ), s, f );
            }
            catch( const Xc& ) {
                set_null( p );
            }
        } else {
          //type().input( (Byte*)( (long)p + offset() ), s, f );
            type().input( ptr( p ), s, f );
        }
    }
    catch( Xc& x ) {
        _xc_insert_field_name( &x );
        throw;
    }
}

//---------------------------------------------------------------------------Field_descr::input

void Field_descr::input( Area* a, istream* s, const Text_format& f ) const
{
    //if( offset() + type().field_size() > a->size() )   throw_xc( "SOS-1347",  c_str( name() ), a->size() );
    uint needed_length = offset() + type().field_size();
    if( needed_length > a->length() ) {
        if( offset() < a->length() )  throw_xc( "SOS-1342", this, a->length() );
        if( needed_length < a->size() )  throw_xc( "SOS-1347", this, a->size() );
        input( a->byte_ptr(), s, f );
        a->length( type().field_length( a->byte_ptr(), a->byte_ptr() + type().field_size() ) );
    } else {
        input( a->byte_ptr(), s, f );
    }
}

//----------------------------------------------------------------------Field_descr::write_text
// s.a. print

void Field_descr::write_text( const Byte* p, Area* buffer, const Text_format& f  ) const
{
    if( has_null_flag()  &&  null_flag( p ) )  return;
    if( !type_ptr() )  return;

    try {
        type().write_text( const_ptr( p ), buffer, f );
    }
    catch( Xc& x )
    {
        _xc_insert_field_name( &x );
        throw;
    }
}

//--------------------------------------------------------------Field_descr::write_text_shorted
// s.a. print_shorted

void Field_descr::write_text_shorted( const Const_area& a, Area* buffer, const Text_format& f ) const
{
    //nicht def.: if( has_null_flag()  &&  null_flag( a.byte_ptr() ) )  return;
    if( !type_ptr() )  return;

    try {
        Dynamic_area value ( type().field_size() );

        int len = a.length() - offset();

        if( len <= 0  ||  len >= type().field_size() )  throw_xc( "write_text_shorted", this );

        memcpy( value.ptr(), const_ptr( a.byte_ptr() ), len );
        memset( value.byte_ptr() + len, '\0', type().field_size() - len );

        type().write_text( value.byte_ptr(), buffer, f );
    }
    catch( Xc& x )
    {
        _xc_insert_field_name( &x );
        throw;
    }
}

//----------------------------------------------------------------------Field_descr::write_text
// s.a. print

void Field_descr::write_text( const Const_area& a, Area* buffer, const Text_format& f  ) const
{
    if( !type_ptr() )  return;

    //if( offset() + type().field_size() > a.length() )  throw_xc( "SOS-1246", c_str( name() ),a.length() );
    if( offset() + type().field_size() > a.length() )
    {
        if( a.length() <= offset() )  return;  // NULL
        if( !type().info()->_exact_char_repr )  throw_xc( "SOS-1246", c_str( name() ), a.length() );
        write_text_shorted( a, buffer, f );
    }
    else
    {
        write_text( a.byte_ptr(), buffer, f );
    }
}

//-----------------------------------------------------------------------Field_descr::read_text

void Field_descr::read_text( Byte* p, const char* text, const Text_format& f ) const
{
    try {
        if( has_null_flag() )  set_null_flag( p, false );

        if( text[ 0 ] == '\0'  &&  nullable() ) {  // Leerstring?
            try {
                type().read_text( ptr( p ), text/*leer!*/, f );
            }
            catch( const Null_error& ) 
            {
                set_null( p );
            }
        } else {
            type().read_text( ptr( p ), text, f );
        }
    }
    catch( Xc& x ) {
        _xc_insert_field_name( &x );
        throw;
    }
}

//-----------------------------------------------------------------------Field_descr::read_text

void Field_descr::read_text( Area* a, const char* text, const Text_format& f ) const
{
  //if( offset() + type().field_size() > a->size() )  throw_xc( "SOS-1347", c_str( name() ), a->size() );
    uint needed_length = offset() + type().field_size();
    if( needed_length > a->length() ) {
        if( offset() < a->length() )  throw_xc( "SOS-1342", this, a->length() );
        if( needed_length < a->size() )  throw_xc( "SOS-1347", this, a->size() );
        read_text( a->byte_ptr(), text, f );
        a->length( type().field_length( a->byte_ptr(), a->byte_ptr() + type().field_size() ) );
    } else {
        read_text( a->byte_ptr(), text, f );
    }
}

//--------------------------------------------------------------------------Field_descr::assign

void Field_descr::assign( Byte* p, const Dyn_obj& o, Area* hilfspuffer ) const
{
    if( o.null() )  set_null( p );
    else {
        //else  _type_ptr->assign( ptr( p ), o );
        if( has_null_flag() )  set_null_flag( p, false );
		copy_value( type_ptr(), ptr( p ), o.type(), o.ptr(), hilfspuffer );
	}
}

//--------------------------------------------------------------------------Field_descr::assign

void Field_descr::assign( Byte* p, const Field_descr* field, const Byte* q, Area* hilfspuffer ) const
{
    if( !field  ||  field->null( q ) ) {
        set_null( p );
    } else {
        if( has_null_flag() )  set_null_flag( p, false );

        if( hilfspuffer ) {
            type_ptr()->read_other_field( ptr( p ), field->type_ptr(), field->const_ptr( q ), hilfspuffer );
        } else {
            copy_value( type_ptr(), ptr( p ), field->type_ptr(), field->const_ptr( q ), hilfspuffer );
        }
    }
}

//--------------------------------------------------------------------------Field_descr::assign

void Field_descr::assign( Byte* p, const Field_type* type, const Byte* q, Area* hilfspuffer ) const
{
    if( !type  ||  type->null( q ) ) {
        set_null( p );
    } else {
        if( has_null_flag() )  set_null_flag( p, false );

        if( hilfspuffer ) {
            type_ptr()->read_other_field( ptr( p ), type, q, hilfspuffer );
        } else {
            copy_value( type_ptr(), ptr( p ), type, q, hilfspuffer );
        }
    }
}

//-----------------------------------------------------------------------Field_descr::construct

void Field_descr::construct( Byte* record_ptr ) const
{
    try {
        if( has_null_flag() )  set_null_flag( record_ptr, true );
        type().construct( ptr( record_ptr ) );
    }
    catch( Xc& x )     // Manche Typen sind nicht konstruierbar, z.B. Tabbed_field_type des Dateityps record/tabbed
    {
        _xc_insert_field_name( &x );
        throw;
    }
}

//---------------------------------------------------------------------------Field_descr::clear

void Field_descr::clear( Byte* record_ptr ) const
{
    if( has_null_flag() )  set_null_flag( record_ptr, true );
    type().clear( ptr( record_ptr ) );
}

//-------------------------------------------------------------Field_descr::write_null_as_empty

void Field_descr::write_null_as_empty( Bool w )
{
    if( w ) 
    {
        if( _type_ptr )
        {
            Std_type std_type = _type_ptr->simple_type()->info()->_std_type;
            if( std_type == std_type_char
             || std_type == std_type_varchar )  
            {
                _write_null_as_empty = true;
            }

            _type_ptr->write_null_as_empty( true );     // Für Record_type
        }
    } 
    else 
    {
        _write_null_as_empty = false;
    }
}

//----------------------------------------------------------------------Field_descr::_obj_print

void Field_descr::_obj_print( ostream* s ) const
{
    *s << _name << ' ';

    if( type_ptr() )  *s << *type_ptr();
                else  *s << "(ohne Typ)";

    *s << " offset=";

    if( (ulong)_offset < 0xFFFF )  *s << _offset;
    else
    if( _offset == -1           )  *s << "(ohne Offset?)";
                             else  *s << (void*)_offset;
}

//---------------------------------------------------------Array_field_descr::Array_field_descr

Array_field_descr::Array_field_descr()
:
    _zero_ ( this+1 )
{
    obj_const_name( "Array_field_descr" );
}

//--------------------------------------------------------Array_field_descr::~Array_field_descr

Array_field_descr::~Array_field_descr()
{
}

//--------------------------------------------------------Array_field_descr::_throw_index_error

void Array_field_descr::_throw_index_error( int index ) const
{
    Xc x ( "SOS-1349" );
    x.insert( index );
    //x.insert( _dim[ 0 ]._first_index );
    //x.insert( _dim[ 0 ]._first_index + _dim[ 0 ]._elem_count - 1 );
    x.insert( this );
    throw_xc( x );
}

//---------------------------------------------------------------Array_field_descr::elem_offset

long Array_field_descr::elem_offset( int index, int dim ) const
{
    uint idx = index - _dim[dim]._first_index;

    if( idx >= _dim[dim]._elem_count )  _throw_index_error( index );

    return _offset + idx * _dim[dim]._distance;
}

//-----------------------------------------------------Array_field_descr::_xc_insert_field_name

void Array_field_descr::_xc_insert_field_name( Xc* x, int i ) const
{
    ostrstream s;
    s << *this;

    Sos_string ins = string("Feld ") + string( s.str(), s.pcount() );
  //ins += name();
    ins += '[';
    ins += ::sos::as_string( i );
    ins += ']';
    x->insert( ins );
}

//----------------------------------------------------------------Array_field_descr::elem_print

void Array_field_descr::elem_print( int i, const Byte* p, ostream* s, const Text_format& f,
                                    char quote, char quote_quote  ) const
{
    //KEIN null_flag!
    //if( _null_flag_offset != (uint)-1  &&  *(Bool*)( p + _null_flag_offset ) )  return;

    try {
        if( type_ptr() )  type().print( (const Byte*)( (long)p + elem_offset( i ) ), s, f,
                                        quote, quote_quote );
    }
    catch( Xc& x )
    {
        _xc_insert_field_name( &x, i );
        throw;
    }
}

//-----------------------------------------------------------Array_field_descr::elem_write_text

void Array_field_descr::elem_write_text( int i, const Byte* p, Area* buffer, const Text_format& f ) const
{
    //KEIN null_flag!
    //if( _null_flag_offset != -1  &&  *(Bool*)( (long)p + _null_flag_offset ) )  return;

    try {
        if( type_ptr() )  type().write_text( (const Byte*)( (long)p + elem_offset( i ) ), buffer, f );
    }
    catch( Xc& x )
    {
        _xc_insert_field_name( &x, i );
        throw;
    }
}

//-------------------------------------------------------Array_field_descr::set_array_distances

void Array_field_descr::set_array_distances( Array_field_descr* outer )
// Für Cobol
{
    if( outer ) {   // äußeres Array?
        for( int i = 0; i < _level; i++ ) {
            if( _dim[i]._elem_count )  break;   // 19.4.00  Verschachtelte OCCURS-Klausel nicht überschreiben!
            _dim[ i ] = outer->_dim[ i ];
        }
    }

    if( !outer  ||  _level > outer->_level ) {
        int a = _type_ptr->alignment();
        _dim[ _level - 1 ]._distance = ( _type_ptr->field_size() + a - 1 ) / a * a;
    }

    if( _type_ptr->obj_is_type( tc_Record_type ) ) {
        ((Record_type*)+_type_ptr)->set_array_distances( this );
    }
}

//----------------------------------------------------------------Array_field_descr::_obj_print

void Array_field_descr::_obj_print( ostream* s ) const
{
    Field_descr::_obj_print( s );

    for( int i = 0; i < _level; i ++ ) {
        const Dim* d = &_dim[ i ];
        *s << '['
           << d->_first_index << ".." << ( d->_first_index + d->_elem_count - 1 );
        if( d->_distance != _type_ptr->field_size() )  *s << ",distance=" << d->_distance;
        *s << ']';
    }
}

//-------------------------------------------------------------------Method_descr::Method_descr

Method_descr::Method_descr( const Sos_ptr<Method_type>& t, const char* name,
                            Any_method_ptr m, Field_descr_flags flags )
:
    _type ( t ),
    _name ( name ),
    _method( m ),
    _flags(flags)
{
    obj_const_name( "Method_descr" );
}

//----------------------------------------------------------------------Method_descr::_obj_print

void Method_descr::_obj_print( ostream* s ) const
{
    *s << _name << ' ' << type()/* << " at " << _offset*/;
}

//---------------------------------------------------------------------Record_type::Record_type

Record_type::Record_type( int field_count )
:
    Field_type(&default_type_info,0),
    _zero_(this+1)
{
    obj_const_name( "Record_type" );
    _method_descr_array.obj_const_name( "Record_type::_method_descr_array" );
    _field_descr_array.obj_const_name( "Record_type::_field_descr_array" );

    if ( field_count > 0 ) _field_descr_array.size( field_count );
}

//--------------------------------------------------------------------Record_type::~Record_type

Record_type::~Record_type()
{
}

//--------------------------------------------------------------------------Record_type::create

Sos_ptr<Record_type> Record_type::create()
{
    return SOS_NEW( Record_type );
}

//-----------------------------------------------------------------------Record_type::add_field

void Record_type::add_field( const Sos_ptr<Field_type>& type, const char* name,
                             long offset, long null_offset, Field_descr_flags flags )
{
    //LOG( "count=" << _field_count << "  add_field( " << name << " );\n");
    Sos_ptr<Field_descr> field_ptr = SOS_NEW( Field_descr( type, name, offset, null_offset, flags ) );
    add_field( field_ptr );
}

//-----------------------------------------------------------------------Record_type::add_field

void Record_type::add_field( Field_type* type, const char* name, long offset,
                             long null_offset, Field_descr_flags flags  )
{
    add_field( Sos_ptr<Field_type>( type ), name, offset, null_offset, flags );
}

//-----------------------------------------------------------------------Record_type::add_field

Field_descr* Record_type::add_field( const char* type_definition, const char* field_name, bool nullable )
{
    Sos_ptr<Field_type>  type  = make_type( type_definition );
    Sos_ptr<Field_descr> field = SOS_NEW( Field_descr( type, field_name ) );

    field->add_to( this );

    if( nullable  &&  !field->type().nullable() )  field->add_null_flag_to( this );

    return field;
}

//-----------------------------------------------------------------------Record_type::add_field

void Record_type::add_field( Field_descr* f )
{
    _field_descr_array.add( f );

    if( f ) {
        //f->offset() darf nicht absolut sein!!
        _field_size = max( _field_size, (uint)( f->offset() + f->type().field_size() - _offset_base ) );
        if( f->has_null_flag() )  _field_size = max( _field_size, (uint)( f->_null_flag_offset + sizeof (Bool) - _offset_base ) );
        //LOG( *this << ".add_field offset=" << f->offset() << "  " << *f << "  size=" << f->type().field_size() << ", _field_size=" << _field_size << '\n' );
    }

    _field_count++;
}

//-------------------------------------------------------------Record_type::set_array_distances

void Record_type::set_array_distances( Array_field_descr* af )
{
    int n = field_count();

    for( int i = 0; i < n; i++ )
    {
        Field_descr* f = field_descr_ptr( i );
        if( f ) {
            if( f->obj_is_type( tc_Array_field_descr ) ) {
                ((Array_field_descr*)f)->set_array_distances( af );
            } else
            if( f->type_ptr()
             && f->type_ptr()->obj_is_type( tc_Record_type ) )
            {
                ((Record_type*)f->type_ptr())->set_array_distances( af );
            }
        }
    }
}

//-------------------------------------------------------------------Record_type::append_fields

void Record_type::append_fields( const Sos_ptr<Record_type>& record_type )
{
    int n = record_type->field_count();
    //allocate_fields( field_count() + n );

    for( int i = 0; i < n; i++ )
    {
        Field_descr* field = record_type->field_descr_ptr( i );
        if( field ) {
            Sos_ptr<Field_descr> f = Field_descr::create( *field );
            f->_offset = -1;
            f->_null_flag_offset = -1;  // Läßt sich aber auch ausrechnen mit add_null_flag()
            f->add_to( this );
        }
        //else ?
    }
}

//----------------------------------------------------------------------Record_type::add_method

void Record_type::add_method( Method_type* t, const char* name, const Any_method_ptr& m )
{
    add_method( Sos_ptr<Method_type>( t ), name, m );
}

//----------------------------------------------------------------------Record_type::add_method

void Record_type::add_method( const Sos_ptr<Method_type>& t, const char* name, const Any_method_ptr& m )
{
    add_method( SOS_NEW( Method_descr( t, name, m, 0 ) ) );
}

//-----------------------------------------------------------------------Record_type::construct

void Record_type::construct( Byte* p ) const
{
    //if( field_size() < 0 )  throw_xc( "field_size undefiniert", this );   //? jz 29.7.97

    memset( p, 0, field_size() );       // Für die Zahnlücken

    //Erst was anderes testen, dann folgendes freigeben: 6.10.97
    //if( _group_type )
    //{
    //    _group_type->construct( p );
    //}
    //else
    if( !_byte_type ) 
    {
        for( int i = 0; i < field_count(); i++ )
        {
            const Field_descr* f = field_descr_ptr( i );
            if( f )  f->construct( p - _offset_base );
        }
    }
}

//------------------------------------------------------------------------Record_type::destruct

void Record_type::destruct( Byte* p ) const
{
    if( !_byte_type ) 
    {
        if( _group_type )
        {
            _group_type->destruct( p );
        }
        else
        {
            for( int i = field_count() - 1; i >= 0; i-- )
            {
                const Field_descr* f = field_descr_ptr( i );
                if( f ) {
                    if( f->type_ptr() )  f->type_ptr()->destruct( f->ptr( p - _offset_base ) );
                    if( f->has_null_flag() )  f->set_null_flag( p - _offset_base, true );
                }
            }
        }
    }
}

//---------------------------------------------------------------------------Record_type::clear

void Record_type::clear( Byte* p ) const
{
    memset( p, 0, field_size() );       // Für die Zahnlücken

    //Erst was anderes testen, dann folgendes freigeben: 6.10.97
    //if( _group_type ) {
    //    _group_type->clear( p );
    //}
    //else
    if( !_byte_type )
    {
        for( int i = 0; i < field_count(); i++ )
        {
            const Field_descr* f = field_descr_ptr( i );
            if( f )  f->clear( p - _offset_base );
        }
    }
}

//---------------------------------------------------------------------Record_type::simple_type

Field_type* Record_type::simple_type()
{ 
    if( _group_type )  return _group_type;

    if( field_count() == 1 )
    {
        Field_descr* f = field_descr_ptr(0);
        if( f->offset() == 0 )  return f->simple_type();
    }

    return this;
}

//-------------------------------------------------------------Record_type::write_null_as_empty

void Record_type::write_null_as_empty( Bool w )
{
    for( int i = 0; i < field_count(); i++ )
    {
        Field_descr* f = field_descr_ptr( i );
        if( f )  f->write_null_as_empty( w );
    }

    if( _group_type )  _group_type->write_null_as_empty( w );
}

//----------------------------------------------------------------------Record_type::field_copy

void Record_type::field_copy( Byte* p, const Byte* s ) const
{
    if( !p || !s )  throw_xc( "Record_Type::field_copy", "offset ist absolut" );

    if( _group_type )
    {
        _group_type->field_copy( p, s );
    }
    else
    {
        if( _byte_type )        //jz 6.12.97
        {
            memcpy( p, s, field_size() );
        } 
        else 
        {
            for( int i = 0; i < field_count(); i++ )
            {
                const Field_descr* f = field_descr_ptr( i );
                if( f  &&  f->_type_ptr ) 
                {
                    f->_type_ptr->field_copy( f->ptr( p - _offset_base ), f->const_ptr( s - _offset_base ) );
                    //NULL-Behandlung?? 28.10.96
                
                    if( f->has_null_flag() )    //2006-12-06
                    {
                        f->set_null_flag( p - _offset_base, f->null_flag( s - _offset_base ) );  //2006-12-06
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------Field_type::field_equal

Bool Record_type::field_equal( const Byte* a, const Byte* b ) const
{
    for( int i = 0; i < field_count(); i++ )
    {
        Field_descr* f = field_descr_ptr( i );
        Field_type*  t = f->type_ptr();
        if( !t->field_equal( (const Byte*)( (long)a + f->offset() - _offset_base ),
                             (const Byte*)( (long)b + f->offset() - _offset_base ) ) )  return false;
    }

    return true;
}

//----------------------------------------------------------------------Field_type::field_count

int Field_type::field_count() const
{
    return obj_is_type( tc_Record_type )? ((Record_type*)this)->field_count() : 0;
}

//----------------------------------------------------------------------Field_type::field_descr

const Field_descr& Field_type::field_descr( int i ) const
{
    if( !obj_is_type( tc_Record_type ) )  throw_xc( "Field_type::field_descr" );
    return ((Record_type*)this)->field_descr( i );
}

//------------------------------------------------------------------Field_type::field_descr_ptr

Field_descr* Field_type::field_descr_ptr( int i )
{
    if( !obj_is_type( tc_Record_type ) )  throw_xc( "SOS-1210", info()->_name, i );
    return ((Record_type*)this)->field_descr_ptr( i );
}

//------------------------------------------------------------------Field_type::field_descr_ptr

Field_descr* Field_type::field_descr_ptr( const char* name )
{
    if( !obj_is_type( tc_Record_type ) )  throw_xc( "SOS-1210", info()->_name, name );
    return ((Record_type*)this)->field_descr_ptr( name );
}

//-------------------------------------------------------------Field_type::field_descr_by_name_or_0

Field_descr* Field_type::field_descr_by_name_or_0( const char* name )
{
    if( !obj_is_type( tc_Record_type ) )  throw_xc( "SOS-1210", info()->_name, name );
    return ((Record_type*)this)->field_descr_by_name_or_0( name );
}

//----------------------------------------------------------------Field_type::named_field_descr

const Field_descr& Field_type::named_field_descr( int i ) const
{
    return field_descr( i );
}

//-------------------------------------------------------------------Field_type::print_selected
/*
void Field_type::print_selected( const Byte*, ostream*, const Text_format&,
                                 const Sos_array<int>& ) const
{
    throw_xc( "Field_type::print_selected" );
}

//-------------------------------------------------------------------Field_type::input_selected

void Field_type::input_selected( Byte*, istream*, const Text_format&,
                                 const Sos_array<int>&  ) const
{
    throw_xc( "Field_type::input_selected" );
}
*/
//-----------------------------------------------------------------------Record_type::alignment

int Record_type::alignment() const
{
    int a = 1;

    for( int i = field_count() - 1; i >= 0; i-- )
    {
        Field_descr* f = field_descr_ptr( i );
        if( f  &&  f->type_ptr() )  a = max( a, f->type_ptr()->alignment() );
    }

    return a;
}

//-----------------------------------------------------------------------Record_type::read_text

void Record_type::read_text( Byte* ptr, const char* text, const Text_format& format ) const
{
    if( /* jz 970526 format.raw()  &&*/  _group_type ) 
    {
        _group_type->read_text( ptr, text, format );
    } 
    else 
    {
        //jz 23.11.97? if( !empty( ptr ) )  throw_xc( "SOS-1368", this );

        if( field_count() == 0 )  {
            if( !::sos::empty( text ) )  throw_xc( "SOS-1368", this );  ////jz 23.11.97
            return;
        }

        if( field_count() != 1 )  throw_xc( "SOS-1364", this );

        Field_descr* f = field_descr_ptr( 0 );
        if( f )  f->read_text( ptr, text, format );
    }
}

//-----------------------------------------------------------------------Record_type::read_text
/*
void Record_type::read_text( Byte* ptr, const Const_area& text, const Text_format& format ) const
{
    if( _group_type ) {
        Sos_string string = as_string( text );
        _group_type->read_text( ptr, c_str( string ), format );
    } else {
        if( field_count() == 0 )  return;
        if( field_count() != 1 )  throw_xc( "SOS-1364" );
        Field_descr* f = field_descr_ptr( 0 );
        if( f )  f->read_text( ptr, text, format );
        //read_text_selected( ptr, text, format, 0 );
    }
}
*/
//----------------------------------------------------------------------Record_type::write_text

void Record_type::write_text( const Byte* ptr, Area* buffer, const Text_format& format ) const
{
    if( /*jz 961101 format.raw()  &&*/  _group_type ) 
    {
        _group_type->write_text( ptr, buffer, format );
    } 
    else 
    {
        if( field_count() == 0 )  return;

        if( field_count() != 1 )  throw_xc( "SOS-1364" );

        Field_descr* f = field_descr_ptr( 0 );
        if( f )  f->write_text( ptr, buffer, format );
    }
}

//---------------------------------------------------------------------------Record_type::print
/*
void Record_type::print( const Byte* p, ostream*s, const Text_format& f,
                         char quote, char quote_quote ) const
{
    _print_selected( p, s, f, 0 );
}

//---------------------------------------------------------------------------Record_type::input

void Record_type::input( Byte* p, istream* s, const Text_format& f ) const
{
    _input_selected( p, s, f, 0 );
}

//------------------------------------------------------------------Record_type::print_selected

void Record_type::print_selected( const Byte* p, ostream*s, const Text_format& f,
                                 const Sos_array<int>& a ) const
{
    _print_selected( p, s, f, &a );
}

//------------------------------------------------------------------Record_type::input_selected

void Record_type::input_selected( Byte* p, istream* s, const Text_format& f,
                                 const Sos_array<int>& a ) const
{
    _input_selected( p, s, f, &a );
}
*/
//-----------------------------------------------------------------Record_type::input_selected
/*
void Record_type::_input_selected( Byte* ptr, istream* s, const Text_format& format,
                                   const Sos_array<int>* field_numbers ) const
{
  try {
    uint i_end = field_count();

    if ( field_numbers &&
         field_numbers->last_index()-field_numbers->first_index()+1 != field_count() )
    {
            // korrekte Permutation ?  (Bed. f.a. i ex. j => field_numbers[j] == i)
            throw_xc( "SOS-1169",  // ???
                      field_numbers->last_index()-field_numbers->first_index()+1, field_count() );
    }

    for( int i = 0; i < i_end; i++ )
    {
        const Field_descr& n = field_descr( field_numbers? (*field_numbers)[ i ]
                                                         : i );

        if( format.has_separator()  &&  i > 0 ) {
            // auch feste Feldlänge möglich?
            if( s->peek() != format.separator() )  throw_xc( "SOS-1168", this );
            s->get();
        }

//      if( format.with_name()
//       && ( format.with_nesting() || n.type().field_count() == 0 )
//       && *field_descr( i ).name() )
//      {
//          *s << n.name() << "=";
//      }

// js 27.12.95
        if( n.nullable() &&
            ( s->peek() == EOF || ( format.has_separator() && s->peek() == format.separator() ) ) )
        {
            n.set_null( ptr );
        } else {
            n.input( ptr, s, format );
        }
        //n.input( ptr, s, format );
// js 27.12.95 Ende
    }
  }
  catch( Xc& x ) {
     Sos_string ins = "im Record-Typ ";
     ins += name();
     x.insert( ins );
     throw;
  }
}
*/
//----------------------------------------------------------------------Record_type::print_selected
/*
void Record_type::_print_selected( const Byte* ptr, ostream* s, const Text_format& format,
                                   const Sos_array<int>* field_numbers ) const
{
    int i_start;
    int i_end;

    Text_format format2 = format;
    if( !format.separator() )  format2.raw( true );     // Felder möglichst Orignal ausgeben (Cobol-Gruppe)

    if( field_numbers ) {
        i_start = field_numbers->first_index();
        i_end = field_numbers->last_index() + 1;
    } else {
        i_start = 0;
        i_end = field_count();
    }

    for( int i = i_start; i < i_end; i++ )
    {
        uint j = i;

        if( field_numbers ) {
            j = (*field_numbers)[ i ];
            if( j >= field_count() )  throw_xc( "Record_type::print" );
        }

        if( i > i_start  &&  format.separator() )  *s << format.separator();
        const Field_descr& f = field_descr( j );
        //if( format.with_name() )  if( *f.name() )  *s << f.name() << "=";
        f.print( ptr, s, format2 );
    }
}
*/
//-------------------------------------------------------------Record_type::read_text_selected
/*
void Record_type::read_text_selected( Byte* ptr, const Const_area& text, const Text_format& format,
                                      const Sos_array<int>* field_numbers ) const
{
  const char* t     = text.char_ptr();
  const char* t_end = t + text.length();

  try {
    uint i_end = field_count();

    if ( field_numbers &&
         field_numbers->last_index()-field_numbers->first_index()+1 != field_count() )
    {
            // korrekte Permutation ?  (Bed. f.a. i ex. j => field_numbers[j] == i)
            throw_xc( "SOS-1169", // ???
                      field_numbers->last_index()-field_numbers->first_index()+1,
                      field_count() );
    }

    for( int i = 0; i < i_end; i++ )
    {
        const Field_descr& n = field_descr( field_numbers? (*field_numbers)[ i ]
                                                         : i );

        if( format.has_separator()  &&  i > 0 ) {
            // auch feste Feldlänge möglich?
            if( t == t_end || *t != format.separator() )  throw_xc( "SOS-1168", this );
            t++;
        }

// js 27.12.95
        if( n.nullable() &&
            ( t == t_end || ( format.has_separator() && *t == format.separator() ) ) )
        {
            n.set_null( ptr );
        } else {
            const char* t0 = t;
            while( t < t_end && *t != format.separator() )  t++;
            Dynamic_area field_text ( t - t0 + 1 );
            field_text.assign( t0, t - t0 );
            field_text += '\0';
            n.read_text( ptr, field_text.char_ptr(), format );
        }
        //n.input( ptr, s, format );
// js 27.12.95 Ende
    }
  }
  catch( Xc& x ) {
     Sos_string ins = "im Record-Typ ";
     ins += name();
     x.insert( ins );
     throw;
  }
}
*/
//-------------------------------------------------------------------------print_name

static void print_name( ostream* s, const char* n )
{
    //while( c = *n++ )  s->put( (char)( c == '-' ? '_' : c ) );
    while(1) {
        char c = *n++;
        if( !c )  break;
        s->put( (char)( c == '-' ? '_' : c ) );
    }
}


// ---------------------------------------------------------------Record_type::print_field_names

void Record_type::print_field_names( ostream* s, const Text_format& format ) const
{
    // Ohne Nesting !
    for( int i = 0; i < field_count(); i++ )
    {
        const Field_descr& f = field_descr( i );
        if( i > 0 )  *s << format.separator();
        print_name( s, f.name() );
    }
}

//-------------------------------------------------------------------------print_name

static void print_name( Area* area_ptr, const char* n )
{
    area_ptr->allocate_min( strlen( n ) );

    while(1) {
        char c = *n++;
        if( !c )  break;
        *area_ptr += (char)( c == '-' ? '_' : c );
    }
}

// ---------------------------------------------------------------Record_type::append_field_names

void Record_type::append_field_names( Area* area_ptr, const Text_format& format ) const
{
    // Ohne Nesting !
    for( int i = 0; i < field_count(); i++ )
    {
        const Field_descr& f = field_descr( i );
        if( i > 0 )  *area_ptr += format.separator();
        print_name( area_ptr, f.name() );
    }
}


//-------------------------------------------------------------Record_type::write_text_selected
/*
void Record_type::write_text_selected( const Byte* ptr, Area* buffer, const Text_format& format,
                                       const Sos_array<int>* field_numbers ) const
{
    int i_start;
    int i_end;

    Text_format format2 = format;
    if( !format.separator() )  format2.raw( true );     // Felder möglichst Orignal ausgeben (Cobol-Gruppe)

    if( field_numbers ) {
        i_start = field_numbers->first_index();
        i_end = field_numbers->last_index() + 1;
    } else {
        i_start = 0;
        i_end = field_count();
    }

    for( int i = i_start; i < i_end; i++ )
    {
        uint j = i;

        if( field_numbers ) {
            j = (*field_numbers)[ i ];
            if( j >= field_count() )  throw_xc( "Record_type::print" );
        }

        if( i > i_start  &&  format.separator() )  *buffer += format.separator();
        const Field_descr& f = field_descr( j );
        //if( format.with_name() )  if( *f.name() )  { buffer->append( f.name() ); *buffer += '='; }
        f.write_text( ptr, buffer, format2 );
    }
}
*/
//---------------------------------------------------------------------Record_type::field_index

int Record_type::field_index( const char* name ) const
{
    int i = 0;

    while( i < field_count() )
    {
        Field_descr* f = field_descr_ptr( i );
        if( f  &&  field_names_are_equal( name, f->name() ) )  return i;
        i++;
    }

    throw_xc( "SOS-1179", name, c_str( _name ) );
    return 0;
}

//-----------------------------------------------------------Record_type::field_descr_by_offset

Field_descr* Record_type::field_descr_by_offset( long offset ) const
{
    int i = 0;

    while( i < field_count() )
    {
        Field_descr* f = field_descr_ptr( i );
        if( f  &&  f->offset() == offset )  return f;
    }

    throw_xc( "SOS-1192", offset );
    return 0;
}

//------------------------------------------------------------Record_type::_field_descr_by_name

Field_descr* Record_type::_field_descr_by_name( const char* name ) const
{
    if( isdigit( name[ 0 ] ) ) {
        return field_descr_ptr( ::sos::as_int( name ) );
    }

    Field_descr* f = field_descr_by_name_or_0( name );
    if( f )  return f;

    throw_xc( "SOS-1179", name, c_str( _name ) );
    return 0;
}

//--------------------------------------------------------Record_type::field_descr_by_name_or_0

Field_descr* Record_type::field_descr_by_name_or_0( const char* name ) const
{
    if( isdigit( name[ 0 ] ) ) {
        try {
            return field_descr_ptr( ::sos::as_int( name ) );
        }
        catch( const Xc& ) {
            return 0;
        }
    }


    Field_descr*  f = 0;
    int           i = 0;

    while( i < field_count() )
    {
        Field_descr* g = field_descr_ptr( i );
        if( g ) {
            if( !flat_scope() ) {
                if( field_names_are_equal( name, g->name() ) )  return g;
            } else {
                if( field_names_are_equal( name, g->name() ) ) {
                    if( f )  throw_xc( "SOS-1183", name );
                    f = g;
                    //g = field_descr_ptr( i );
                }
                else
                if( g->type().obj_is_type( tc_Record_type )
                 && !g->type().obj_is_type( tc_Array_type ) )  // Array_type sollte nicht von Record_type erben
                {
                    g = ((Record_type*)SOS_CAST( Record_type, g->type_ptr() ))->
                        field_descr_by_name_or_0( name );
                    if( g ) {
                        if( f )  throw_xc( "SOS-1183", name );
                        f = g;
                    }
                }
            }
        }

        i++;
    }

    return f;
}

//----------------------------------------------------------------------Record_type::_obj_print

void Record_type::_obj_print( ostream* s ) const
{
    if( !::sos::empty( name() ) )  *s << name();
                             else  *s << "Record_type";
    *s << '(' << field_count() << " Felder)";
}

//-----------------------------------------------------------------------Record_type::print_all
/*
void Record_type::print_all( ostream* s, int nesting ) const
{
    _obj_print( s );

    *s << ' ' << field_size() << " byte\n";

    int max_name_length = 0;

    for( int i = 0; i < field_count(); i++ )
    {
        int name_length = length( field_descr( i ).name() );
        if( name_length > max_name_length )  max_name_length = name_length;
    }

    for( i = 0; i < field_count(); i++ )
    {
        if( i > 0 )  *s << ",\n";
        const Field_descr& n = field_descr( i );
        s->flags( ios::left );
        *s << setw( max_name_length + 1 ) << n.name() << " at " << setw(4) << n.offset() << setw(0) << n.type();
    }
}
*/
//-----------------------------------------------------------------------------make_record_type

Sos_ptr<Record_type> make_record_type(const char* record_type_definition )
{
    Sos_ptr<Record_type> t = SOS_NEW( Record_type );
    const char*          p = record_type_definition;
    Sos_string           name;
    Sos_string           interpretation;
    Sos_ptr<Field_type>  field_type;
    bool                 mit_klammer = false;
    
    if( *p == '(' )  p++, mit_klammer = true;

    while( isspace( *p ) )  p++;

    while(1) 
    {
        int size = default_field_size;

        read_field_name_and_interpretation( &p, &name, &interpretation, ',' );

        if( length( interpretation ) ) 
        {
            const char* q = c_str( interpretation ) + 1;   // ':' überspringen
            while( isspace( *q ) )  q++;
            
            if( isdigit( *q ) ) 
            {
                size = 0;
                while( isdigit( *q ) )  size = size*10 + *q++ - '0';
                while( isspace( *q ) )  q++;
                interpretation = q;
            }

            //if( *q )  throw_xc( "SOS-1217", name, interpretation );
        }

        Sos_ptr<Text_type> text_type = SOS_NEW( Text_type( size ) );
        field_type = +text_type;

        Sos_ptr<Field_descr> field_descr = SOS_NEW( Field_descr( field_type, c_str( name ) ) );
        if( length( interpretation ) )  modify_field( field_descr, interpretation );
        field_descr->add_to( t );
        if( !field_type->nullable() )  field_descr->add_null_flag_to( t );

        while( isspace( *p ) )  p++;
        if( *p == ')' )  break;
        if( *p == '\0' )  break;
        if( *p != ',' )  goto FEHLER;
        p++;
    }

    if( mit_klammer )
    {  
        if( *p != ')' )  goto FEHLER;
        p++;
    }
    if( *p )  goto FEHLER;


    return t;

FEHLER:
    throw_xc( "SOS-1401", record_type_definition, p - record_type_definition + 1 );
    return NULL;
}
    
//---------------------------------------------------------------------Method_type::Method_type

Method_type::Method_type()
:
    Field_type( &default_type_info, 0 )
{
    obj_const_name( "Method_type" );
}

//--------------------------------------------------------------------Method_type::~Method_type

Method_type::~Method_type()
{
}

//----------------------------------------------------------------------Method_type::_obj_print

void Method_type::_obj_print( ostream* s ) const
{
    *s << "method";
}

//----------------------------------------------------------------------Method_type::write_text

void Method_type::write_text( const Byte*, Area*, const Text_format& ) const
{
    throw_xc( "method-print?");
}

//-----------------------------------------------------------------------Method_type::read_text

void Method_type::read_text( Byte*, const char*, const Text_format& ) const
{
    throw_xc( "method-input?");
}

//-----------------------------------------------------------------------------------copy_value
// Quellwert darf nicht nichtig sein!

void copy_value( const Field_type* dest_type,       Byte* dest_ptr,
                 const Field_type* src_type , const Byte* src_ptr,
                 Area* hilfspuffer )
{
    if( hilfspuffer ) {
        dest_type->read_other_field( dest_ptr, src_type, src_ptr, hilfspuffer );
/*
        src_type->write_text( src_ptr, hilfspuffer );
        *hilfspuffer += '\0';
        dest_type->read_text( dest_ptr, hilfspuffer->char_ptr() );
*/
    } else {
        copy_value( dest_type, dest_ptr, src_type, src_ptr );
    }
}

//-----------------------------------------------------------------------------------copy_value
// Quellwert darf nicht nichtig sein!

void copy_value( const Field_type* dest_type,       Byte* dest_ptr,
                 const Field_type* src_type , const Byte* src_ptr  )
{
    Dynamic_area mein_hilfspuffer ( max_display_field_size+1 );
    copy_value( dest_type, dest_ptr, src_type, src_ptr, &mein_hilfspuffer );
}

//-----------------------------------------------------------------------------------copy_value
// Quellwert darf nicht nichtig sein!

void copy_value( const Field_descr* dst_field,       Byte* dst_ptr,
                 const Field_descr* src_field, const Byte* src_ptr,
                 Area* hilfspuffer )
{
    if( hilfspuffer ) {
        //try
        //dst_field->assign( dst_ptr, src_field, src_ptr, hilfspuffer );
        //catch( null_error) dst_field->set_null(); ??

        src_field->write_text( src_ptr, hilfspuffer );
        *hilfspuffer += '\0';
        dst_field->read_text( dst_ptr, hilfspuffer->char_ptr() );
    } else {
        copy_value( dst_field, dst_ptr, src_field, src_ptr );
    }
}

//-----------------------------------------------------------------------------------copy_value
// Quellwert darf nicht nichtig sein!

void copy_value( const Field_descr* dst_field, Byte* dst_ptr,
                 const Field_descr* src_field, const Byte* src_ptr )
{
    Dynamic_area mein_hilfspuffer ( max_display_field_size+1 );
    copy_value( dst_field, dst_ptr, src_field, src_ptr, &mein_hilfspuffer );
}

//-----------------------------------------------------------------------------------copy_field

void copy_field( const Field_descr* dest_field, Byte* dest_ptr,
                 const Field_descr* src_field, const Byte* src_ptr,
                 Area* hilfspuffer, Bool empty_is_null )
{
    const Field_type* dest_type = dest_field->type_ptr();
    const Field_type* src_type  = src_field? src_field->type_ptr() : 0;

    try {
        if( !src_field  ||  src_field->null( src_ptr ) ) {
            if( !dest_field->nullable()
             && (    empty_is_null
                  || ( src_type && src_type->empty_is_null() )
                  || dest_type->empty_is_null() ) )
            {
                dest_field->read_text( (Byte*)dest_ptr, "" );
            } else {
                dest_field->set_null( dest_ptr );
            }
        }
        else
        {
            if( dest_type == src_type ) {   // Typen sind identisch?
                dest_type->field_copy( dest_field->ptr( dest_ptr ),
                                       src_field->const_ptr( src_ptr ) );
                if( dest_field->has_null_flag() )  dest_field->set_null_flag( dest_ptr, false );
            } else {
                copy_value( dest_field, dest_ptr, src_field, src_ptr, hilfspuffer );
            }
        }
    }
    catch( Xc& x )
    {
        //char text [ 100 ];
        //ostrstream s ( text, sizeof text );
        //s << p->_r->name() << " " << p->_r->type() << " at " << p->_r->offset();
        //x.insert( text, s.pcount() );
        x.insert( dest_field );
        x.insert( src_field );
        throw;
    }
}

//-----------------------------------------------------------------------------------copy_field

void copy_field( const Field_descr* dest_field, Byte* dest_ptr,
                 const Field_descr* src_field, const Byte* src_ptr,
                 Bool empty_is_null )
{
    Dynamic_area hilfpuffer;
    copy_field( dest_field, dest_ptr, src_field, src_ptr, &hilfpuffer, empty_is_null );
}

//----------------------------------------------------------------------------------copy_record

void copy_record( const Record_type* dest_type, void* dest_ptr,
                  const Record_type* source_type, const void* source_ptr,
                  Bool empty_is_null )
{
    Dynamic_area hilfspuffer ( max_display_field_size + 1 );

    int n = dest_type->field_count();
    if( n != source_type->field_count() )  throw_xc( "copy_record", "unterschiedliche Anzahl Felder");

    for( int i = 0; i < n; i++ ) {
        copy_field( dest_type->field_descr_ptr( i ), (Byte*)dest_ptr,
                    source_type->field_descr_ptr( i ), (const Byte*)source_ptr,
                    &hilfspuffer, empty_is_null );
    }
}

//--------------------------------------------------------------------------------------as_bool

Bool as_bool( const Field_type* t, const Byte* p )
{
    if( t == &int_type )      return *(int*)p != 0;

    Sos_limited_text<50> buffer;
    t->write_text( p, &buffer );
    return as_bool( c_str( buffer ) );
}

//--------------------------------------------------------------------------------------as_bool

Bool as_bool( const Field_type* t, const Byte* p, Bool deflt )
{
    if( t->null( p ) )  return deflt;
    Sos_limited_text<50> buffer;
    t->write_text( p, &buffer );
    if( empty( buffer ) )  return deflt;
    return as_bool( c_str( buffer ) );
}

//--------------------------------------------------------------------------------------as_bool

Bool as_bool( const Field_descr* f, const Byte* p, Bool deflt )
{
    if( f->null( p ) )  return deflt;
    return as_bool( f->type_ptr(), f->const_ptr( p ), deflt );
}

//---------------------------------------------------------------------------------------as_int

int as_int( const Field_type* t, const Byte* p )
{
    if( t == &int_type )      return *(int*)p;

    Sos_limited_text<50> buffer;
    t->write_text( p, &buffer );
    return as_int( c_str( buffer ) );
}

//--------------------------------------------------------------------------------------as_long

long as_long( const Field_type* t, const Byte* p )
{
    if( t == &long_type )      return *(long*)p;
    if( t == &int_type )       return *(int*)p;

    Sos_limited_text<50> buffer;
    t->write_text( p, &buffer );
    return as_long( c_str( buffer ) );
}

//-----------------------------------------------------------------------------------as_big_int

Big_int as_big_int( const Field_type* t, const Byte* p )
{
    if( t == &big_int_type )  return *(Big_int*)p;
    if( t == &long_type )     return *(long*)p;
    if( t == &int_type )      return *(int*)p;

    Sos_limited_text<50> buffer;
    t->write_text( p, &buffer );
    return as_big_int( c_str( buffer ) );
}

//---------------------------------------------------------------------------------as_text_area

Const_area_handle as_text_area( const Field_type* t, const Byte* p )
{
    Dynamic_area buffer;
    t->write_text( p, &buffer );
    return buffer;
}

//--------------------------------------------------------------------------------------as_char

char as_char( const Field_type* t, const Byte* p )
{
    Sos_limited_text<1> buffer;
    t->write_text( p, &buffer );
    if( buffer.length() == 0 )  throw_xc( "SOS-1345" );
    return buffer[ 0 ];
}

//------------------------------------------------------------------------------------make_type

Sos_ptr<Field_type> make_type( const char* type_definition )
{
    Sos_ptr<Field_type> type;

    if( strcmpi( type_definition, "Bool" ) == 0 ) 
    {
        type = &bool_type;
    } 
    else 
    if( strcmpi( type_definition, "Int" ) == 0 || strcmpi( type_definition, "Integer" ) == 0 ) 
    {
        type = &int_type;
    } 
    else 
    if( strcmpi( type_definition, "Long" ) == 0 ) 
    {
        type = &long_type;
    } 
    else 
    if( strcmpi( type_definition, "Currency" ) == 0 ) 
    {
        type = &currency_type;
    } 
    else 
    if( strcmpi( type_definition, "Number" ) == 0 ) 
    {
        type = &double_type;
    } 
    else 
    if( strcmpi( type_definition, "Date" ) == 0 ) 
    {
        type = &sos_optional_date_type;
    } 
    else 
    if( strcmpi( type_definition, "DateTime" ) == 0 ) 
    {
        type = &sos_optional_date_time_type;
    } 
    else 
    if( strncmpi( type_definition, "Text(", 5 ) == 0 ) 
    {
        const char* p = type_definition + 5;
        uint        n = 0;
        while(1) {
            uint d = *p - '0';
            if( d >= 10 )  break;  
            n = 10*n + d;
            p++;
        }

        if( n == 0 )  goto THROW_1209;

        Sos_ptr<Text_type> t = SOS_NEW( Text_type( n ) );
        type = +t;
    } 
    else 
    if( strncmpi( type_definition, "String(", 6 ) == 0 ) 
    {
        const char* p = type_definition + 6;
        uint        n = 0;
        while(1) {
            uint d = *p - '0';
            if( d >= 10 )  break;  
            n = 10*n + d;
            p++;
        }

        if( n == 0 )  goto THROW_1209;

        Sos_ptr<String0_type> t = SOS_NEW( String0_type( n ) );
        type = +t;
    } 
    else 
    {
THROW_1209:
        throw_xc( "SOS-1209", type_definition );
    }

    return type;
}

//----------------------------------------------------------------------------------------------

} //namespace sos
