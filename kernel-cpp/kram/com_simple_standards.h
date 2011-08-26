// $Id: com_simple_standards.h 11894 2006-02-28 09:42:59Z jz $

#ifndef __SOS_COM_SIMPLE_STANDARDS
#define __SOS_COM_SIMPLE_STANDARDS


#include <string>
#include "com.h"
#include <vector>

#include "../zschimmer/z_com.h"
using zschimmer::ptr;
using zschimmer::com::Bstr;
using zschimmer::com::Variant;

struct Dyn_obj;
struct Field_type;
struct Field_descr;


namespace sos {

//-------------------------------------------------------------------------------------------------

ostream&        operator <<             ( ostream&, const IID& );
ostream&        operator <<             ( ostream&, const OLECHAR* );
ostream&        operator <<             ( ostream&, const VARIANT& );
void            print_dispid            ( ostream*, const DISPID& );

       int      int_from_variant        ( const VARIANT& );
//       int      int_from_variant        ( const VARIANT&, int deflt );
//inline int      int_from_variant        ( const _variant_t& v )                         { return int_from_variant( (const VARIANT&)v ); }
inline int      int_from_variant        ( const Variant& v )                            { return int_from_variant( (const VARIANT&)v ); }
//inline int      int_from_variant        ( const Variant& v, int delft )                 { return int_from_variant( (const VARIANT&)v,  ); }

string          as_string               ( const IID& );
string          name                    ( const IID& );
CLSID           string_as_clsid         ( const string& class_name );                   // ProgId oder ClassId {...}

BSTR            SysAllocStringLen_char  ( const char* single_byte_text, uint len );
inline BSTR     SysAllocString_string   ( const string& str )                           { return SysAllocStringLen_char( str.c_str(), str.length() ); }

string          w_as_string             ( const OLECHAR*, int len );
string          w_as_string             ( const OLECHAR* );

uint            bstr_to_area            ( const BSTR, Area* );
string          bstr_as_string          ( const BSTR );
//void            bstr_to_lower           ( BSTR );
//inline void     bstr_to_lower           ( BSTR* bstr )                                  { bstr_to_lower( *bstr ); }
//void            olechar_to_char         ( BSTR, Area* );

void            variant_to_char         ( const VARIANT&, Area*, LCID = 0 );
HRESULT         variant_to_string       ( const VARIANT&, string*, LCID = 0 );
string          variant_as_string       ( const VARIANT&, LCID = 0 );
string          variant_as_string       ( const VARIANT&, const string& deflt );
void            variant_to_dynobj       ( const VARIANT&, Dyn_obj*, LCID = 0 );

void            field_to_variant        ( Field_type*, const Byte*, VARIANT*, Area* hilfspuffer, LCID = 0 );
void            field_to_variant        ( Field_descr*, const Byte*, VARIANT*, Area* hilfspuffer, LCID = 0 );

//inline string   as_string               ( const VARIANT& v )                            { return variant_as_string( v ); }
//string          as_string               ( const unsigned short* );
//inline string   as_string               ( const OLECHAR* s )                            { return w_as_string(s); }

Variant         com_call                ( IDispatch* object, const string& method );
Variant         com_call_if_exists      ( IDispatch* object, const string& method );
Variant         com_call                ( IDispatch* object, const string& method, const Variant& par1 );
Variant         com_call_if_exists      ( IDispatch* object, const string& method, const Variant& par1 );
Variant         com_call                ( IDispatch* object, const string& method, Variant* par1 );
Variant         com_call_if_exists      ( IDispatch* object, const string& method, Variant* par1 );
Variant         com_call                ( IDispatch* object, const string& method, const Variant& par1, const Variant& par2 );
void            com_property_put        ( IDispatch* object, const string& property, const Variant& );
void            com_property_putref     ( IDispatch* object, const string& property, const Variant& );
Variant         com_property_get        ( IDispatch* object, const string& property );
Variant         com_invoke              ( DWORD flags, IDispatch* object, const string& method, std::vector<Variant>* params );
bool            com_name_exists         ( const ptr<IDispatch>&, const string& name );
bool            com_param_given         ( VARIANT* param );

//----------------------------------------------------------------------------------------Ole_error

struct Ole_error : Xc
{
                                Ole_error               ( const char* error_code, HRESULT, const char* function, const char* ins1 = NULL, const char* ins2 = NULL );
                                Ole_error               ( const _com_error& );
                               ~Ole_error               () throw();

    HRESULT                    _hresult;
};


void            throw_ole_excepinfo     ( HRESULT, EXCEPINFO*, const char* function = NULL, const char* ins = NULL );  // sosole.cxx
void            throw_ole               ( HRESULT, const char* function, const char* ins1 = NULL, const char* ins2 = NULL );  
void            throw_ole               ( HRESULT, const char* function, const OLECHAR* ins1 );  
void            throw_com_error         ( const _com_error&, const char* insertion = NULL );

//-----------------------------------------------------------------------------------Ole_initialize

struct Ole_initialize
{
                                Ole_initialize          ()          { HRESULT hr = CoInitialize(NULL); if(FAILED(hr)) throw_ole( hr, "CoInitialize" ); }
                               ~Ole_initialize          ()          { CoUninitialize(); }
};

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
