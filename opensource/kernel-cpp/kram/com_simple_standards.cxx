// $Id: com_simple_standards.cxx 11931 2006-03-04 10:56:32Z jz $

#include "precomp.h"
#include "sos.h"
#include <vector>

#include "../zschimmer/zschimmer.h"
using zschimmer::ptr;


#ifdef SYSTEM_WIN
#   include <ios>           // ios_base::failure
#else
#   include <typeinfo>
#endif

#include <stdio.h>          // sprintf

#ifdef SYSTEM_WIN
#   ifdef SYSTEM_MFC
#       include <afxdisp.h>
#    else
#       include <windows.h>
#   endif

#   include <shellapi.h>
#   include <ole2ver.h>
#endif

#include "log.h"
#include "sysxcept.h"
#include "sosprof.h"
#include "sosfield.h"
#include "sosdate.h"
#include "stdfield.h"
#include "licence.h"
#include "../file/anyfile.h"


#define INITGUIDS
#include "com_simple_standards.h"

#include "../zschimmer/regex_class.h"


using namespace std;
using zschimmer::Regex;
using zschimmer::Regex_submatches;


//------------------------------------------------------------------------------VariantChangeTypeEx
/*#ifndef SYSTEM_HAS_COM

HRESULT VariantChangeTypeEx( VARIANT* dest, VARIANT* src, LCID lcid, unsigned short flags, VARTYPE vt )
{
    using namespace sos;

    HRESULT hr = S_OK;
    VARIANT result;

    VariantInit( &result );

    try
    {
        string str = sos::variant_as_string( *src, lcid );

        switch( vt )
        {
          //case VT_EMPTY:
          //case VT_NULL: 
            case VT_I2:         V_I2  (&result) = as_int16 ( str.c_str() );  break;
            case VT_I4:         V_I4  (&result) = as_int32 ( str.c_str() );  break;
            case VT_R4:         V_R4  (&result) = as_double( str.c_str() );  break;
            case VT_R8:         V_R8  (&result) = as_double( str.c_str() );  break;
          //case VT_CY: 
          //case VT_DATE:       
          //case VT_ERROR:
            case VT_BOOL:       V_BOOL(&result) = as_bool  ( str.c_str() );  break;
          //case VT_I1:         V_I1  (&result) = as_int8  ( str.c_str() );  break;
          //case VT_UI1:        V_UI1 (&result) = as_uint8 ( str.c_str() );  break;
            case VT_UI2:        V_UI1 (&result) = as_uint16( str.c_str() );  break;
            case VT_UI4:        V_UI4 (&result) = as_uint32( str.c_str() );  break;
          //case VT_I8:         V_I8  (&result) = as_int64 ( str.c_str() );  break;
          //case VT_UI8:        V_I8  (&result) = as_uint64( str.c_str() );  break;
            case VT_INT:        V_INT (&result) = as_int   ( str.c_str() );  break;
            case VT_UINT:       V_UINT(&result) = as_uint  ( str.c_str() );  break;
          //case VT_VOID:
          //case VT_HRESULT:
          //case VT_PTR:
          //case VT_FILETIME:
            case VT_BSTR:       V_BSTR(&result) = SysAllocString_string( str );  break;
          //case VT_DISPATCH:
          //case VT_VARIANT:
          //case VT_UNKNOWN:
          //case VT_DECIMAL:
          //case VT_SAFEARRAY:
          //case VT_CARRAY:
          //case VT_USERDEFINED:
          //case VT_LPSTR:
          //case VT_LPWSTR:
          //case VT_RECORD:
          //case VT_BLOB:
          //case VT_STREAM:
          //case VT_STORAGE:
          //case VT_STREAMED_OBJECT:
          //case VT_STORED_OBJECT:
          //case VT_BLOB_OBJECT:
          //case VT_CF:
          //case VT_CLSID:
            default:            return DISP_E_BADVARTYPE;
        }
    }
    catch( const Overflow_error& ) { return DISP_E_OVERFLOW; }
    catch( const Xc&             ) { return DISP_E_TYPEMISMATCH; }

    VariantCopy( dest, &result );
    VariantClear( &result );

    return hr;
}

#endif*/
//-------------------------------------------------------------------------------------------------

namespace sos {

//--------------------------------------------------------------------------------------------const

const VARIANT                   empty_variant               = Variant();
const LCID                      std_lcid                    = (LCID)0;   //LOCALE_SYSTEM_DEFAULT;
const bool                      true_value                  = true;
const bool                      false_value                 = false;

//-------------------------------------------------------------------------------------------static

//static Mutex                    last_lcid_mutex             ( "last_lcid", Mutex::kind_nonrecursive );
//static Text_format              last_lcid_text_format;
//static LCID                     last_lcid                   = (LCID)-1;  // leer

//-------------------------------------------------------------------------------------print_dispid

void print_dispid( ostream* s, const DISPID& dispID ) 
{
    switch( dispID ) {
        case DISPID_VALUE       : *s << "DISPID_VALUE"; break;
        case DISPID_UNKNOWN     : *s << "DISPID_UNKNOWN"; break;
        case DISPID_PROPERTYPUT : *s << "DISPID_PROPERTYPUT"; break;
        case DISPID_NEWENUM     : *s << "DISPID_NEWENUM"; break;
        case DISPID_EVALUATE    : *s << "DISPID_EVALUATE"; break;
        case DISPID_CONSTRUCTOR : *s << "DISPID_CONSTRUCTOR"; break;
        case DISPID_DESTRUCTOR  : *s << "DISPID_DESTRUCTOR"; break;
        case DISPID_COLLECT     : *s << "DISPID_COLLECT"; break;
        default                 : *s << hex << (ulong)dispID << dec;
    }
}

//------------------------------------------------------------------------------ostream << DISPID
/*
ostream& operator << ( ostream& s, const DISPID& dispid ) 
{
    if( wFlags & ~( DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF ) ) {
        s << ' ' << hex << wFlags << dec;
    }

    if( wFlags & DISPATCH_METHOD )          s << "DISPATCH_METHOD";
    if( wFlags & DISPATCH_PROPERTYGET )     s << "DISPATCH_PROPERTYGET";
    if( wFlags & DISPATCH_PROPERTYPUT )     s << "DISPATCH_PROPERTYPUT";
    if( wFlags & DISPATCH_PROPERTYPUTREF )  s << "DISPATCH_PROPERTYPUTREF";

    return s;
}
*/

//------------------------------------------------------------------------------ostream << DISPID

ostream& operator << ( ostream& s, const DISPPARAMS* pDispParams ) 
{
    int i;

    if( !pDispParams ) 
    {
        s << "NULL";
    } 
    else 
    {
        s << '{';
        for( i = pDispParams->cArgs - 1; i >= 0; i-- ) 
        {
            if( i < (int)pDispParams->cArgs - 1 )  s << ',';
            if( i < (int)pDispParams->cNamedArgs )  {
                print_dispid( &s, pDispParams->rgdispidNamedArgs[ i ] );
                s << ":=";
            }

            s << pDispParams->rgvarg[ i ];
        }
        s << '}';
    }

    return s;
}

//-----------------------------------------------------------------------------------as_string

string as_string( const IID& clsid ) 
{
    OLECHAR clsid_text [ 38+1 ];

    clsid_text[ 0 ] = 0;
    StringFromGUID2( clsid, clsid_text, sizeof clsid_text );

    return w_as_string(clsid_text);
}

//-----------------------------------------------------------------------------int_from_variant

int int_from_variant( const VARIANT& variant )
{
    Variant v = variant;
    HRESULT hr = v.ChangeType( VT_I4 );
    if( FAILED(hr) ) {
        string text;
        v.ChangeType(VT_BSTR);
        if( v.vt == VT_BSTR )  text = variant_as_string(v);
        throw_ole( hr, "ChangeType(int)", text.c_str() );
    }
    return v.lVal;
}

//-----------------------------------------------------------------------------int_from_variant
/*
int int_from_variant( const VARIANT& variant, int deflt )
{
    if( variant.vt == VT_NULL )  return deflt;
    if( variant.vt == VT_EMPTY )  return deflt;
    if( variant.vt == VT_BSTR && SysStringLen(V_BSTR(&variant)) == 0 )  return deflt;

    return int_from_variant( variant );
}
*/
//------------------------------------------------------------------------------string_as_clsid

CLSID string_as_clsid( const string& class_name )
{
    HRESULT hr;
    OLECHAR class_namew [ 38+1 ];
  //ulong   class_namew_len = NO_OF( class_namew );
    CLSID   clsid;

    if( length(class_name) > NO_OF(class_namew) - 1 )  throw_xc( "string_as_clsid", "Klassenname zu lang", c_str(class_name) );

    MultiByteToWideChar( CP_ACP, 0, c_str(class_name), length(class_name), class_namew, length(class_name) );
    class_namew[ length(class_name) ] = '\0';


    if( class_name[0] == '{' ) 
    {
        hr = CLSIDFromString( class_namew, &clsid );
        if( FAILED( hr ) )  throw_ole( hr, class_name.c_str(), "CLSIDFromString" );
    }
    else
    {
        hr = CLSIDFromProgID( class_namew, &clsid );
        if( FAILED( hr ) )  throw_ole( hr, class_name.c_str(), "CLSIDFromProgID" );
    }

    return clsid;
}

//----------------------------------------------------------------------------------------name

string name( const IID& clsid ) 
{
#   ifdef SYSTEM_HAS_COM
        string  clsid_string = as_string( clsid );
        char        value      [ 100+1 ];
        long        value_len;         
        long        err;
        string  key;

        key = "CLSID\\";  key += clsid_string;
        value_len = sizeof value;
        err = RegQueryValue( HKEY_CLASSES_ROOT, c_str( key ), value, &value_len );
        if( !err )  return "CLSID " + string( value );

        value_len = sizeof value;
        key = "Interface\\";  key += clsid_string;
        err = RegQueryValue( HKEY_CLASSES_ROOT, c_str( key ), value, &value_len );
        if( !err )  return "Interface " + string( value );

        value_len = sizeof value;
        key = "Typelib\\";  key += clsid_string;
        err = RegQueryValue( HKEY_CLASSES_ROOT, c_str( key ), value, &value_len );
        if( !err )  return "Typelib " + string( value );
#   endif

    return "(unknown)";
}

//----------------------------------------------------------------------------operator << (IID)

ostream& operator << ( ostream& s, const IID& clsid )
{
    try
    {
        s << as_string( clsid ) << ' ' << name( clsid );
    }
    catch( const exception& x  ) { s << x.what(); }

    return s;
}

//--------------------------------------------------------------------------ostream << OLECHAR*

ostream& operator << ( ostream& s, const OLECHAR* ole_string )
{
    try
    {
        s << w_as_string( ole_string );
    }
    catch( const exception& x  ) { s << x.what(); }

    return s;
}

//---------------------------------------------------------------------------ostream << VARIANT

ostream& operator<< ( ostream& s, const VARIANT& v )
{
    HRESULT hr;
    VARIANT w;

    VariantInit( &w );


    hr = VariantChangeTypeEx( &w, (VARIANT*)&v, STANDARD_LCID, 0, VT_BSTR );  //MAKELCID(MAKELANGID(0x09, 0x01),SORT_DEFAULT)
  //hr = VariantChangeTypeEx( &w, (VARIANT*)&v, (LCID)0, 0, VT_BSTR );  //MAKELCID(MAKELANGID(0x09, 0x01),SORT_DEFAULT)
    if( FAILED( hr ) )  return s;

    s << V_BSTR( &w );

    VariantClear( &w );

    return s;
}

//-----------------------------------------------------------------SysAllocStringLen_char(char*)

BSTR SysAllocStringLen_char( const char* single_byte_text, uint len )
{
    OLECHAR* ptr = SysAllocStringLen( (OLECHAR*)NULL, len );

    if( ptr ) {
        int count = 0;
        if( len ) {
            count = MultiByteToWideChar( CP_ACP, 0, single_byte_text, len, ptr, len );
            if( !count )  throw_mswin_error( "MultiByteToWideChar" );
        }
        ptr[ count ] = L'\0';
    }

    return ptr;
}

//------------------------------------------------------------------------------bstr_to_area

uint bstr_to_area( BSTR wchar_string, Area* char_string )
{
    uint wlen = SysStringLen( wchar_string );
    char_string->allocate_min( wlen + 1 );
    uint len = 0;
    if( wlen ) {
        len = WideCharToMultiByte( CP_ACP, 0, wchar_string, wlen, char_string->char_ptr(), char_string->size(), NULL, NULL );
        if( !len )  throw_mswin_error( "WideCharToMultiByte" );
    }

    char_string->length( len );
    return len;
}

//----------------------------------------------------------------------------olechar_to_char
/*
void olechar_to_char( BSTR wchar_string, Area* char_string )
{
    wchar_to_char( wchar_string, char_string );
}
*/
//---------------------------------------------------------------------------as_string(short*)
#ifdef _WIN32
/*
string as_string( const unsigned short* wstr )
{
    assert( sizeof(wchar_t) == sizeof(unsigned short) );

    int    n = wmemchr( (wchar_t*)wstr, 0, (uint)-1 ) - (wchar_t*)wstr;
    return w_as_string( (wchar_t*)wstr, n );
}
*/
#endif
//---------------------------------------------------------------------------------w_as_string

string w_as_string( const OLECHAR* wstr )
{
    if( !wstr )  return "";

    return w_as_string( wstr, wmemchr( wstr, 0, (uint)-1 ) - wstr );
}

//---------------------------------------------------------------------------------w_as_string

string w_as_string( const OLECHAR* wstr, int len )
{
    if( len == 0 )  return "";

    char* buffer = new char[ len+1 ];

    len = WideCharToMultiByte( CP_ACP, 0, wstr, len, buffer, len, NULL, NULL );
    if( !len ) { delete[] buffer; throw_mswin_error("WideCharToMultiByte"); }

    string result ( buffer, len );
    delete[] buffer;

    return result;
}

//------------------------------------------------------------------------------bstr_as_string

string bstr_as_string( const BSTR bstr )
{
    if( !bstr )  return "";
    return w_as_string( bstr, SysStringLen( bstr ) );
}

//----------------------------------------------------------------------------variant_to_dynobj

void variant_to_dynobj( const VARIANT& variant, Dyn_obj* dyn_obj, LCID lcid )
{
    // s.a. variant_to_char!

    switch( V_VT( &variant ) ) 
    {
      //case VT_EMPTY:
        case VT_I2             : dyn_obj->assign( &int16_type, &V_I2   ( &variant ) );  break;
        case VT_I2   | VT_BYREF: dyn_obj->assign( &int16_type,  V_I2REF( &variant ) );  break;
        case VT_I4             : dyn_obj->assign( &int32_type, &V_I2   ( &variant ) );  break;
        case VT_I4   | VT_BYREF: dyn_obj->assign( &int32_type,  V_I2REF( &variant ) );  break;
        case VT_R4             : { double a = V_R4( &variant ); dyn_obj->assign( &double_type, &a );  break; }
        case VT_R4   | VT_BYREF: { double a = V_R4( &variant ); dyn_obj->assign( &double_type, &a );  break; }
        case VT_R8             : { double a = V_R8( &variant ); dyn_obj->assign( &double_type, &a );  break; }
        case VT_R8   | VT_BYREF: { double a = V_R8( &variant ); dyn_obj->assign( &double_type, &a );  break; }
#   ifdef SYSTEM_INT64
        case VT_CY             : dyn_obj->assign( &currency_type, &V_CY   ( &variant ) );  break;
        case VT_CY   | VT_BYREF: dyn_obj->assign( &currency_type,  V_CYREF( &variant ) );  break;
#   endif
        case VT_BSTR           : 
        case VT_BSTR | VT_BYREF: 
        {
            string text = bstr_as_string( V_VT( &variant ) & VT_BYREF? *V_BSTRREF( &variant ) : V_BSTR( &variant ) );
            *dyn_obj = text;
            break;
        }

        case VT_NULL           : dyn_obj->set_null();  break;
        case VT_BOOL           : dyn_obj->assign( &bool_type, V_BOOL   ( &variant )? &true_value : &false_value );  break;
        case VT_BOOL | VT_BYREF: dyn_obj->assign( &bool_type, V_BOOLREF( &variant )? &true_value : &false_value );  break;

#ifdef SYSTEM_HAS_COM
        case VT_DATE:
        case VT_DATE | VT_BYREF: 
        {
            SYSTEMTIME systemtime;
            VariantTimeToSystemTime( ( V_VT(&variant) & VT_BYREF )? *V_DATEREF( &variant ) : V_DATE( &variant ), &systemtime ); ;

            Sos_optional_date_time  datetime;
            
            datetime.assign_date( systemtime.wYear, systemtime.wMonth, systemtime.wDay );
            datetime.set_time( systemtime.wHour, systemtime.wMinute, systemtime.wSecond );  //systemtime.wMilliseconds

            dyn_obj->assign( &sos_optional_date_time_type, &datetime );
            break;
        }
#endif

      //case VT_DISPATCH           :
      //case VT_DISPATCH | VT_BYREF: {
      //    if Hostole_dynobj ...
      //}

        default: 
        {
            Dynamic_area buffer ( 1024 );
            variant_to_char( variant, &buffer, lcid );
            buffer.append( '\0' );
            *dyn_obj = buffer.char_ptr();
        }
    }
}

//----------------------------------------------------------------------------variant_to_char

void variant_to_char( const VARIANT& variant, Area* area, LCID lcid )
{
    //s.a. variant_to_dynobj!

    //char buffer [ 100 ];

    switch( V_VT( &variant ) ) 
    {

  //case VT_EMPTY:
    case VT_NULL:   
    {
        area->length( 0 ); 
        break;
    }

    case VT_I2:             int16_type.write_text( (const Byte*)&V_I2   ( &variant ), area, std_text_format );  break;
    case VT_I2 | VT_BYREF:  int16_type.write_text( (const Byte*) V_I2REF( &variant ), area, std_text_format );  break;
    case VT_I4:             int32_type.write_text( (const Byte*)&V_I4   ( &variant ), area, std_text_format );  break;
    case VT_I4 | VT_BYREF:  int32_type.write_text( (const Byte*) V_I4REF( &variant ), area, std_text_format );  break;
    case VT_I8:             int64_type.write_text( (const Byte*)&V_I8   ( &variant ), area, std_text_format );  break;
    case VT_I8 | VT_BYREF:  int64_type.write_text( (const Byte*) V_I8REF( &variant ), area, std_text_format );  break;
    case VT_R4:             { double a =  V_R4   ( &variant );  double_type.write_text( (const Byte*)&a, area, std_text_format );  break; }
    case VT_R4 | VT_BYREF:  { double a = *V_R4REF( &variant );  double_type.write_text( (const Byte*)&a, area, std_text_format );  break; }
    case VT_R8:             double_type.write_text( (const Byte*)&V_R8   ( &variant ), area, std_text_format );  break;
    case VT_R8 | VT_BYREF:  double_type.write_text( (const Byte*) V_R8REF( &variant ), area, std_text_format );  break;

#if defined SYSTEM_MFC
    case VT_CY:             area->assign( COleCurrency(  V_CY   ( &variant ) ).Format( 0, MAKELCID(MAKELANGID(0x09, 0x01),SORT_DEFAULT) ) );  break;
    case VT_CY | VT_BYREF:  area->assign( COleCurrency( *V_CYREF( &variant ) ).Format( 0, MAKELCID(MAKELANGID(0x09, 0x01),SORT_DEFAULT) ) );  break;
#else
    case VT_CY:             
    case VT_CY | VT_BYREF:  
    {
        int64 cy = V_VT(&variant) & VT_BYREF? V_CYREF(&variant)->int64
                                            : V_CY(&variant).int64;
        currency_type.write_text( (const Byte*)&cy, area, raw_text_format );
      //sprintf( buffer, "%" PRINTF_LONG_LONG "d.%04d", cy / 10000, (long)( cy % 10000 ) );
      //area->assign( buffer );  
        break;
    }
#endif

//#ifdef SYSTEM_HAS_COM
    case VT_DATE:
    case VT_DATE | VT_BYREF:
    {
        Sos_optional_date_time datetime;
        SYSTEMTIME  systemtime;
        double      variant_time = ( V_VT( &variant ) & VT_BYREF )? *V_DATEREF( &variant ) : V_DATE( &variant );

        VariantTimeToSystemTime( variant_time, &systemtime );

        datetime.assign_date( systemtime.wYear, systemtime.wMonth, systemtime.wDay );
        datetime.set_time( systemtime.wHour, systemtime.wMinute, systemtime.wSecond );
      //systemtime.wMilliseconds;

        datetime.write_text( area );
        break;
    }
//#endif

    case VT_BSTR:            bstr_to_area(  V_BSTR   ( &variant ), area );  break;
    case VT_BSTR | VT_BYREF: bstr_to_area( *V_BSTRREF( &variant ), area );  break;
  //case VT_DISPATCH:
  //case VT_ERROR:
    case VT_BOOL:            area->assign(  V_BOOL   ( &variant )? "1" : "0" );  break;
    case VT_BOOL | VT_BYREF: area->assign( *V_BOOLREF( &variant )? "1" : "0" );  break;
  //case VT_VARIANT:
  //case VT_UNKNOWN:
  //case VT_DECIMAL:
    case VT_I1:             int1_type.write_text( (const Byte*)&V_I1   ( &variant ), area, std_text_format );  break;
    case VT_I1 | VT_BYREF:  int1_type.write_text( (const Byte*) V_I1REF( &variant ), area, std_text_format );  break;
    case VT_UI1:            uint1_type.write_text( (const Byte*)&V_UI1   ( &variant ), area, std_text_format );  break;
    case VT_UI1 | VT_BYREF: uint1_type.write_text( (const Byte*) V_UI1REF( &variant ), area, std_text_format );  break;
    case VT_UI2:            uint16_type.write_text( (const Byte*)&V_UI2   ( &variant ), area, std_text_format );  break;
    case VT_UI2 | VT_BYREF: uint16_type.write_text( (const Byte*) V_UI2REF( &variant ), area, std_text_format );  break;
    case VT_UI4:            uint32_type.write_text( (const Byte*)&V_UI4   ( &variant ), area, std_text_format );  break;
    case VT_UI4 | VT_BYREF: uint32_type.write_text( (const Byte*) V_UI4REF( &variant ), area, std_text_format );  break;
    case VT_UI8:            uint64_type.write_text( (const Byte*)&V_UI8   ( &variant ), area, std_text_format );  break;
    case VT_UI8 | VT_BYREF: uint64_type.write_text( (const Byte*) V_UI8REF( &variant ), area, std_text_format );  break;
    case VT_INT:            int_type.write_text( (const Byte*)&V_INT   ( &variant ), area, std_text_format );  break;
    case VT_INT | VT_BYREF: int_type.write_text( (const Byte*) V_INTREF( &variant ), area, std_text_format );  break;
    case VT_UINT:            uint_type.write_text( (const Byte*)&V_UINT   ( &variant ), area, std_text_format );  break;
    case VT_UINT | VT_BYREF: uint_type.write_text( (const Byte*) V_UINTREF( &variant ), area, std_text_format );  break;
  //case VT_VOID:
  //case VT_HRESULT:
  //case VT_PTR:
  //case VT_SAFEARRAY:
  //case VT_CARRAY:
  //case VT_USERDEFINED:
  //case VT_LPSTR:
  //case VT_LPWSTR:
  //case VT_FILETIME:
  //case VT_BLOB:
  //case VT_STREAM:
  //case VT_STORAGE:
  //case VT_STREAMED_OBJECT:
  //case VT_STORED_OBJECT:
  //case VT_BLOB_OBJECT:
  //case VT_CF:
  //case VT_CLSID:

    default:
    {
        VARIANT vt;
        VariantInit( &vt );
        try {
            HRESULT hr = VariantChangeTypeEx( &vt, (VARIANT*)&variant, lcid, 0, VT_BSTR );
            if( FAILED( hr ) )  throw_ole( hr, "VariantChangeTypeEx" );
            bstr_to_area( V_BSTR( &vt ), area );
            VariantClear( &vt );
        }
        catch( const Xc& )
        {
            VariantClear( &vt );
            throw;
        }
    }

    }
}

//----------------------------------------------------------------------------variant_to_string

HRESULT variant_to_string( const VARIANT& variant, string* string, LCID lcid )
{
    if( V_VT( &variant ) == VT_BSTR ) 
    {
        *string = bstr_as_string( V_BSTR( &variant ) );
    } 
    else 
    {
        Dynamic_area buffer ( 1024 );
        variant_to_char( variant, &buffer, lcid );
        buffer.append( '\0' );
        *string = buffer.char_ptr();
    }

    return NOERROR;
}

//----------------------------------------------------------------------------variant_as_string

string variant_as_string( const VARIANT& variant, LCID lcid )
{
    string  string;

    variant_to_string( variant, &string, lcid );

    return string;
}

//----------------------------------------------------------------------------field_to_variant

void field_to_variant( Field_type* field_type, const Byte* p, VARIANT* variant, Area* hilfspuffer, LCID lcid )
{
    VariantInit( variant );

    Field_type* type = field_type->simple_type();

    if( !type  ||  type->null( p ) ) 
    {
        V_VT( variant ) = VT_NULL;
    } 
    else 
    {
        Type_param type_param;
        type->get_param( &type_param );

        switch( type->info()->_std_type )
        {
            case std_type_integer:
            {
                if( type->info()->_radix == 2  &&  !type->info()->_unsigned ) {
                    if( type_param._precision == 16 ) {
                        V_VT( variant ) = VT_I2;
                        V_I2( variant ) = as_int( type, p );
                    }
                    else
                    if( type_param._precision == 32 ) {
                        V_VT( variant ) = VT_I4;
                        V_I4( variant ) = as_int( type, p );
                    }
                    else goto TEXT;
                }
                else goto TEXT;
                break;
            }

            case std_type_bool:
            {
                V_VT( variant ) = VT_BOOL;
                V_BOOL( variant ) = as_bool( type, p )? VARIANT_TRUE : VARIANT_FALSE;
                break;
            }

            case std_type_decimal:
            {
                if( type == &currency_type ) 
                {
                    V_VT( variant ) = VT_CY;
                    V_CY( variant ).int64 = *(int64*)p;
                }
                else
                if( type_param._precision < 10  &&  type_param._scale == 0 )
                {
                    V_VT( variant ) = VT_I4;
                    V_I4( variant ) = type->as_int(p);
                }
                else
                if( type_param._scale <= 4  &&  type_param._precision - type_param._scale <= 15 )   // 15 muss übereinstimmen mit Currency_type::_type_info._max_precision - Currency_type::_scale )
                {
                    Sos_limited_text<100> text;
                    type->write_text( p, &text );
                    V_VT( variant ) = VT_CY;
                    currency_type.read_text( (Byte*)&V_CY( variant ), c_str(text), raw_text_format );
                }
                else
                if( type_param._precision <= 14 )  goto AS_FLOAT;
                else 
                    goto TEXT;
                break;
            }
            case std_type_float:
            {
        AS_FLOAT:
                V_VT( variant ) = VT_R8;
                V_R8( variant ) = type->as_double( p );
                break;
            }

#ifdef SYSTEM_HAS_COM
            case std_type_date_time:
            case std_type_date:
            case std_type_time:
            {
                Sos_optional_date_time  datetime   = as_date_time( type, p );
                SYSTEMTIME              systemtime;

                systemtime.wYear        = datetime.year();
                systemtime.wMonth       = datetime.month();
                systemtime.wDayOfWeek   = 0;
                systemtime.wDay         = datetime.day();
                systemtime.wHour        = datetime.hour();
                systemtime.wMinute      = datetime.minute();
                systemtime.wSecond      = datetime.second();
                systemtime.wMilliseconds= 0;

                V_VT( variant ) = VT_DATE;
                SystemTimeToVariantTime( &systemtime, &V_DATE( variant ) );

                break;
            }
#endif

            case std_type_char:
            case std_type_varchar:
            default:
            {
        TEXT:
                Text_format text_format;

#               ifdef SYSTEM_WIN
                    //static LCID        last_lcid    = (LCID)-1;  // leer

                    //if( lcid != last_lcid )
                    {
                        //CCriticalSection !
                        char buffer [ 2 ];
                        //last_lcid = lcid;
                        int ret = GetLocaleInfo( lcid, LOCALE_SDECIMAL, buffer, sizeof buffer );
                        if( ret > 0 )  text_format.decimal_symbol( buffer[0] );
                    }
#               endif

                hilfspuffer->length( 0 );
                type->write_text( p, hilfspuffer, text_format );
                V_VT( variant )   = VT_BSTR;
                V_BSTR( variant ) = SysAllocStringLen_char( hilfspuffer->char_ptr(), hilfspuffer->length() );
            }
        }
    }
}

//---------------------------------------------------------------------------------field_to_variant

void field_to_variant( Field_descr* field, const Byte* p, VARIANT* variant, Area* hilfspuffer, LCID lcid )
{
    if( !field  ||  !field->type_ptr()  ||  field->has_null_flag()  &&  field->null_flag( p ) ) 
    {
        VariantInit( variant );
        V_VT( variant ) = VT_NULL;
    } 
    else 
    {
        field_to_variant( field->type_ptr(), field->const_ptr( p ), variant, hilfspuffer, lcid );
    }
}

//--------------------------------------------------------------------------------dynobj_to_variant

void dynobj_to_variant( const Dyn_obj& dyn_obj, VARIANT* variant, Area* hilfspuffer, LCID lcid )
{
    field_to_variant( dyn_obj._type, dyn_obj._ptr, variant, hilfspuffer, lcid );
}

//-------------------------------------------------------------------------Ole_error::Ole_error

Ole_error::Ole_error( const char* error_code, HRESULT hr, const char* function, const char* ins1, const char* ins2 )
:
    Xc              ( error_code ),
    _hresult        ( hr )
{
    strcpy( _name, "OLE" );
/*
    Dynamic_area text ( 500 );
    text.char_ptr()[ 0 ] = '\0';

#   ifdef SYSTEM_WIN
        text.length( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL,
                                    hr,
                                    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), //The user default language
                                    text.char_ptr(), text.size(), NULL ) );
#    else
        text.length( 0 );
#   endif                

    char* p = text.char_ptr() + text.length();
    if( p > text.char_ptr()  &&  p[-1] == '\n' )  *--p = '\0';
    if( p > text.char_ptr()  &&  p[-1] == '\r' )  *--p = '\0';
*/
    if( hr == DISP_E_EXCEPTION )
    {
        ptr<IErrorInfo> error_info;
        if( SUCCEEDED(GetErrorInfo( 0, error_info.pp() ))  &&  error_info )
        {
            Bstr description_bstr, source_bstr;
            error_info->GetDescription( &description_bstr._bstr );
            error_info->GetSource     ( &source_bstr._bstr );

            string description = bstr_as_string(description_bstr);
            string source      = bstr_as_string(source_bstr);

            // Prüfen, ob description mit einem Fehlercode anfängt:  (s.a. zschimmer/z_com.cxx)
            // Hostware: XXX-XXX-X9A
            // Rapid:    D999
            Regex regex = "^((([A-Z]+-)+[0-9A-Z]+)|(D[0-9]{3}))( | *$)";       
            Regex_submatches matches = regex.match_subresults( description );
            if( matches )
            {
                string error_code = matches[1];
                description = ltrim( description.substr( error_code.length() ) );

                _error_code = error_code.c_str();
                insert( description );
                if( source != "" )  insert( source );
                if( ins1 )          insert( ins1 );
                if( ins2 )          insert( ins2 );
                if( source != "" )  set_name( source.c_str() );
                return;
            }

            insert( description.c_str() );
            if( source_bstr.length() > 0 )  insert( bstr_as_string(source_bstr).c_str() );
        }
    }

    if( function &&  function[0] )  insert( function );
  //insert( text );

    insert( ins1 );
    insert( ins2 );
}

//-------------------------------------------------------------------------Ole_error::Ole_error

Ole_error::Ole_error( const _com_error& x )
:
    Xc              ( "_com_error" ),
    _hresult        ( x.Error() )
{
    strcpy( _name, "OLE" );

    char code  [ 20 ];

    sprintf( code, "COM-%08lX", (long)x.Error() );
    set_code( code );
}

//------------------------------------------------------------------------Ole_error::~Ole_error

Ole_error::~Ole_error() throw()
{
}

//------------------------------------------------------------------------------------throw_ole

void throw_ole( HRESULT hr, const char* function, const char* ins1, const char* ins2 )
{
    char code  [ 20 ];

    sprintf( code, "COM-%08lX", (long)hr );
    throw Ole_error( code, hr, function, ins1, ins2 );
}

//------------------------------------------------------------------------------------throw_ole

void throw_ole( HRESULT hr, const char* function, const char* ins1, const char* ins2, const Source_pos& = std_source_pos )
{
    char code  [ 20 ];

    sprintf( code, "COM-%08lX", (long)hr );
    throw Ole_error( code, hr, function, ins1, ins2 );
}

//------------------------------------------------------------------------------------throw_ole

void throw_ole( HRESULT hr, const char* function, const OLECHAR* ins1_w )
{
    char code  [ 20 ];

    sprintf( code, "COM-%08lX", (long)hr );

    string ins1 = w_as_string( ins1_w );

    throw Ole_error( code, hr, function, ins1.c_str() );
}

//------------------------------------------------------------------------------throw_com_error

void throw_com_error( const _com_error& com_error, const char* insertion )
{
    string d = bstr_as_string( com_error.Description() );
    throw_ole( com_error.Error(), d.c_str(), insertion );
}

//-------------------------------------------------------------------------throw_ole_excepinfo

void throw_ole_excepinfo( HRESULT hresult, EXCEPINFO* excepinfo, const char* function, const char* ins )
{
    if( excepinfo->pfnDeferredFillIn ) (*excepinfo->pfnDeferredFillIn)( excepinfo );

    string source = bstr_as_string( excepinfo->bstrSource );
    string descr  = bstr_as_string( excepinfo->bstrDescription );

    HRESULT hr = excepinfo->scode? excepinfo->scode : hresult;

    SysFreeString( excepinfo->bstrSource );         excepinfo->bstrSource      = NULL;
    SysFreeString( excepinfo->bstrDescription );    excepinfo->bstrDescription = NULL;
    SysFreeString( excepinfo->bstrHelpFile );       excepinfo->bstrHelpFile    = NULL;

    throw_ole( hr, function, ins, descr.c_str(), Source_pos( c_str(source) ) );
}

//------------------------------------------------------------------------------------com_call

Variant com_call( IDispatch* object, const string& method )
{
    vector<Variant> variant_array ( 0 );
    return com_invoke( DISPATCH_METHOD, object, method, &variant_array );
}

//--------------------------------------------------------------------------com_call_if_exists

Variant com_call_if_exists( IDispatch* object, const string& method )
{
    if( com_name_exists( object, method ) )  return com_call( object, method );
                                       else  return empty_variant;
}

//------------------------------------------------------------------------------------com_call

Variant com_call( IDispatch* object, const string& method, const Variant& par1 )
{
    vector<Variant> variant_array ( 1 );
    variant_array[0] = par1;
    return com_invoke( DISPATCH_METHOD, object, method, &variant_array );
}

//--------------------------------------------------------------------------com_call_if_exists

Variant com_call_if_exists( IDispatch* object, const string& method, const Variant& par1 )
{
    if( com_name_exists( object, method ) )  return com_call( object, method, par1 );
                                       else  return empty_variant;
}

//------------------------------------------------------------------------------------com_call

Variant com_call( IDispatch* object, const string& method, Variant* par1 )
{
    vector<Variant> variant_array ( 1 );
    variant_array[0].vt = VT_VARIANT | VT_BYREF;
    variant_array[0].pvarVal = par1;
    return com_invoke( DISPATCH_METHOD, object, method, &variant_array );
}

//------------------------------------------------------------------------------------com_call

Variant com_call( IDispatch* object, const string& method, const Variant& par1, const Variant& par2 )
{
    vector<Variant> variant_array ( 2 );
    variant_array[0] = par2;
    variant_array[1] = par1;
    return com_invoke( DISPATCH_METHOD, object, method, &variant_array );
}

//--------------------------------------------------------------------------com_call_if_exists

Variant com_call_if_exists( IDispatch* object, const string& method, Variant* par1 )
{
    if( com_name_exists( object, method ) )  return com_call( object, method, par1 );
                                       else  return empty_variant;
}

//----------------------------------------------------------------------------com_property_get

Variant com_property_get( IDispatch* object, const string& property )
{
    vector<Variant> variant_array ( 0 );
    return com_invoke( DISPATCH_PROPERTYGET, object, property, &variant_array );
}

//----------------------------------------------------------------------------com_property_put

void com_property_put( IDispatch* object, const string& property, const Variant& value )
{
    vector<Variant> variant_array ( 1 );
    variant_array[0] = value;
    com_invoke( DISPATCH_PROPERTYPUT, object, property, &variant_array );
}

//-------------------------------------------------------------------------com_property_putref

void com_property_putref( IDispatch* object, const string& property, const Variant& value )
{
    vector<Variant> variant_array ( 1 );
    variant_array[0] = value;
    com_invoke( DISPATCH_PROPERTYPUTREF, object, property, &variant_array );
}

//----------------------------------------------------------------------------------com_invoke

Variant com_invoke( DWORD flags, IDispatch* dispatch, const string& method, vector<Variant>* params  )
{
    HRESULT     hr;
    DISPID      dispid = 0;
    Variant     result;
    DISPID      dispid_propertyput = DISPID_PROPERTYPUT;
    string      clean_method_name;
    bool        is_optional = false;

    if( !dispatch )  throw_xc( "SOS-1455", method );

    is_optional = method[0] == '?';

    int pos = method.find( '(' );  if( pos == string::npos )  pos = method.length();        // Java-Signatur abschneiden, die ignorieren wir (s. spooler_module_java.cxx)
    clean_method_name.assign( method.c_str() + is_optional, pos - is_optional );

    if( flags == DISPATCH_METHOD  &&  method == "()" )
    {
        dispid = DISPID_VALUE;  // So für Aufruf eines Funktionsobjekts in ECMAScript
    }
    else
    {
        Bstr method_bstr = clean_method_name;
        hr = dispatch->GetIDsOfNames( IID_NULL, &method_bstr._bstr, 1, std_lcid, &dispid );
        if( FAILED(hr) )  
        {
            if( is_optional  &&  ( hr == DISP_E_UNKNOWNNAME  ||  hr == DISP_E_MEMBERNOTFOUND ) )  return Variant();
            throw_ole( hr, "IDispatch::GetIDsOfNames", clean_method_name.c_str() );
        }
    }

    DISPPARAMS  dispparams;
    EXCEPINFO   excepinfo;
    uint        error_arg_no = (uint)-1;

    memset( &excepinfo, 0, sizeof excepinfo );

    dispparams.rgvarg            = params->size()? &(*params)[0] : NULL;               // Array of arguments.
    dispparams.cArgs             = params->size();              // Number of arguments.
    dispparams.rgdispidNamedArgs = flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF? &dispid_propertyput  // Dispatch IDs of named arguments.
                                                                                                    : NULL; 
    dispparams.cNamedArgs        = flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF? 1 : 0;  // Number of named arguments.

    hr = dispatch->Invoke( dispid, IID_NULL, std_lcid, (WORD)flags, &dispparams, &result, &excepinfo, &error_arg_no );
    
    if( FAILED(hr) ) 
    {
        if( hr == DISP_E_EXCEPTION )  throw_ole_excepinfo( hr, &excepinfo, clean_method_name.c_str() );
        
        Sos_string a = clean_method_name;
        if( ( hr == DISP_E_TYPEMISMATCH || hr == DISP_E_PARAMNOTFOUND ) && (int)error_arg_no >= 0 )  
            a += ", " + as_string( params->size() - (int)error_arg_no ) + ". Parameter";
        throw_ole( hr, "IDispatch::Invoke", a.c_str() );
    }

    if( result.vt == VT_ERROR ) 
    {
        if( result.scode != DISP_E_PARAMNOTFOUND )    // Das liefert PerlScript, wenn Funktion keine Rückgabe hat.
            throw_ole( result.scode, "IDispatch::Invoke (result)", clean_method_name.c_str() );

        result.Clear();
    }

    return result;
}

//-------------------------------------------------------------------------------com_name_exists

bool com_name_exists( const ptr<IDispatch>& dispatch, const string& name )
{
    HRESULT     hr;
    DISPID      dispid;
    Bstr        name_bstr;

    if( name == "()" )  throw_xc( "com_name_exists(\"()\") ???" );
    if( name.empty() )  throw_xc( "com_name_exists", "Leerer Name" );

    int pos = name.find( '(' );  if( pos == string::npos )  pos = name.length();        // Java-Signatur abschneiden, die ignorieren wir (s. spooler_module_java.cxx)
    name_bstr.assign( name.c_str(), pos );
    
    hr = dispatch->GetIDsOfNames( IID_NULL, &name_bstr._bstr, 1, std_lcid, &dispid );

    if( FAILED( hr ) ) 
    {
        if( hr == DISP_E_UNKNOWNNAME )  return false;
        throw_ole( hr, "GetIDsOfNames", bstr_as_string( name_bstr ).c_str() );
    } 

    return true;
}

//---------------------------------------------------------------------------------com_param_given

bool com_param_given( VARIANT* param )
{
    if( param->vt == VT_EMPTY )  return false;
    if( param->vt == VT_ERROR  &&  param->scode == DISP_E_PARAMNOTFOUND )  return false;
    return true;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
