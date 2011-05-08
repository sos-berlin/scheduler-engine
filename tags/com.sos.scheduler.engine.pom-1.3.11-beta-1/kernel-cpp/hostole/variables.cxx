// $Id$          Joacim Zschimmer

// §1701 
// §1727

#include "precomp.h"

#include "../kram/sos.h"
#include "../kram/com_simple_standards.h"
#include "../kram/thread_semaphore.h"
#include "../zschimmer/z_com_server.h"

#include "hostole2.h"
#include "variables.h"

#ifdef SYSTEM_WIN
#   include <initguid.h>
#endif

namespace zschimmer
{
    namespace xml
    {
        using namespace libxml2;
    }
}

//-------------------------------------------------------------------------------------------------

namespace sos {

using namespace ::zschimmer;
using namespace ::zschimmer::com;

const int                       dispid_offset           = 1000;
const char                      xml_element_name[]      = "sos.hostware.Variables";
const char                      xml_subelement_name[]   = "variable"; 

//--------------------------------------------------------------------Hostware_variables_enumerator

struct Hostware_variables_enumerator : Ivariables_enumerator, Sos_ole_object
{ 
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware.Variables_enumerator" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


    STDMETHODIMP                QueryInterface          ( REFIID, void** );
    
    USE_SOS_OLE_OBJECT_ADDREF_RELEASE
  //USE_SOS_OLE_OBJECT_GETTYPEINFO
  //USE_SOS_OLE_OBJECT_INVOKE           

                                Hostware_variables_enumerator();

    HRESULT STDMETHODCALLTYPE   Next                    ( ULONG celt, VARIANT* rgvar, ULONG* pceltFetched );
    HRESULT STDMETHODCALLTYPE   Skip                    ( ULONG celt );
    HRESULT STDMETHODCALLTYPE   Reset                   ();
    HRESULT STDMETHODCALLTYPE   Clone                   ( IEnumVARIANT** ppenum );
  //HRESULT STDMETHODCALLTYPE   Get_next                ( IDispatch** );

    void                        initialize              ( Hostware_variables* );

    ptr<Hostware_variables>             _variables;
    Hostware_variables::Map::iterator   _iterator;
  //IID                                 _iid;
};

//-------------------------------------------------------------------Hostware_variables2_enumerator

struct Hostware_variables2_enumerator : Hostware_variables_enumerator
{
                                Hostware_variables2_enumerator()                                    {}; //{ _iid = IID_Ivariable2; }

    ptr<Hostware_variables>             _variables;
    Hostware_variables::Map::iterator   _iterator;
};

//----------------------------------------------------------Hostware_variables2_idispatch_enumerator

struct Hostware_variables2_idispatch_enumerator : Ivariables2_idispatch_enumerator, Sos_ole_object
{
                                Hostware_variables2_idispatch_enumerator( Hostware_variables* );
                               ~Hostware_variables2_idispatch_enumerator()                          {}

    STDMETHODIMP                QueryInterface          ( REFIID, void** );

    USE_SOS_OLE_OBJECT_ADDREF_RELEASE
    USE_SOS_OLE_OBJECT_GETTYPEINFO
    USE_SOS_OLE_OBJECT_INVOKE           

    STDMETHODIMP                Next                    ( Ivariable2** );
    STDMETHODIMP            get_Has_next                ( VARIANT_BOOL* );

    ptr<Hostware_variables>             _variables;
    Hostware_variables::Map::iterator   _iterator;
};

//------------------------------------------------------------------------------------Typbibliothek

// Einzig erlaubte Implementierung von Ivariables  {11DFCF81-324F-4edb-9E6F-BDD647CE391D}
DEFINE_GUID( IID_Variables, 0x11dfcf81, 0x324f, 0x4edb, 0x9e, 0x6f, 0xbd, 0xd6, 0x47, 0xce, 0x39, 0x1d );

DESCRIBE_CLASS          ( &hostole_typelib, Hostware_variable , variable , CLSID_Variable , "hostWare.Variable" , "1.0" );
DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware_variables, variables, CLSID_Variables, "hostWare.Variables", "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_variables_enumerator, variables_enumerator, CLSID_Variables_enumerator, "hostWare.Variables_enumerator", "1.0" );

DESCRIBE_CLASS          ( &hostole_typelib, Hostware_variable2 , variable2 , CLSID_Variable2 , "hostWare.Variable2" , "1.0" );
DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware_variables2, variables2, CLSID_Variables2, "hostWare.Variables2", "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_variables2_enumerator, variables2_enumerator, CLSID_Variables2_enumerator, "hostWare.Variables2_enumerator", "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_variables2_idispatch_enumerator, variables2_idispatch_enumerator, CLSID_Variables2_idispatch_enumerator, "hostWare.Variables2_idispatch_enumerator", "1.0" );

Variant empty_variant;

//----------------------------------------------------------------------Hostware_variable::_methods
#ifdef Z_COM

const Com_method Hostware_variable::_methods[] =
{ 
   // _flags              ,    _name      , _method                                   , _result_type, _types                    , _default_arg_count
    { DISPATCH_PROPERTYPUT, 0, "obj_value", (Com_method_ptr)&Ivariable2::put_Obj_value, VT_EMPTY    , { VT_VARIANT|VT_BYREF, VT_VARIANT|VT_BYREF }, 1 },
    { DISPATCH_PROPERTYGET, 0, "obj_value", (Com_method_ptr)&Ivariable2::get_Obj_value, VT_VARIANT  , { VT_VARIANT|VT_BYREF   }, 1 },
    { DISPATCH_METHOD     , 1, "obj_dim"  , (Com_method_ptr)&Ivariable2::Obj_dim      , VT_INT }, 
    { DISPATCH_PROPERTYGET, 2, "obj_name" , (Com_method_ptr)&Ivariable2::get_Obj_name , VT_BSTR }, 
    {}
};

#endif
//-------------------------------------------------------------Hostware_variable::Hostware_variable
// Zweimal implementiert! s.u.

Hostware_variable::Hostware_variable( const BSTR name, const VARIANT& v, Ivariable* )
:
    Sos_ole_object( variable_class_ptr, static_cast<Ivariable*>( this ) ),
    _lock( "Hostware_variable" ),
    _dispid(0)
{
    if( SysStringLen( name ) == 0 )  throw_xc( "SOS-EMPTY-VAR" );

    _name   = name;
    assign( v );
}

//-------------------------------------------------------------Hostware_variable::Hostware_variable

Hostware_variable::Hostware_variable( const BSTR name, const VARIANT& v, Ivariable2* )
:
    Sos_ole_object( variable2_class_ptr, static_cast<Ivariable2*>( this ) ),
    _lock( "Hostware_variable" ),
    _dispid(0)
{
    if( SysStringLen( name ) == 0 )  throw_xc( "SOS-EMPTY-VAR" );

    _name   = name;
    assign( v );
}

//------------------------------------------------------------Hostware_variable::~Hostware_variable

Hostware_variable::~Hostware_variable()
{
}

//------------------------------------------------------------------------Hostware_variable::assign

void Hostware_variable::assign( const VARIANT& v )
{
    Variant         helper;

    _value = *normalized_value( &v, &helper );
}

//---------------------------------------------------------------Hostware_variable::normalize_value

const VARIANT* Hostware_variable::normalized_value( const VARIANT* value, VARIANT* helper )
{
    const VARIANT* result = value;

    if( value->vt == VT_DISPATCH  ||  value->vt == VT_UNKNOWN )    // Wert ist eine Variable? Dann nehmen wir deren Wert!
    {
        ptr<Ivariable2> p = (Ivariable2*)com_query_interface_or_null( V_UNKNOWN( value ), __uuidof( Ivariable2 ) );
        if( p )
        {
            Variant missing ( Variant::vt_missing );
            HRESULT hr = p->get_Obj_value( &missing, helper );
            if( FAILED(hr) )  throw_com( hr, "Variable::Obj_value" );  // Sollte nicht passieren

            result = helper;
        }
    }

    return result;
}

//----------------------------------------------------------------Hostware_variable::QueryInterface

STDMETHODIMP Hostware_variable::QueryInterface( const IID& iid, void** result )
{                                     
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ivariable , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ivariable2, result );

    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//--------------------------------------------------------------Hostware_variable::GetTypeInfoCount
/*

HRESULT Hostware_variable::GetTypeInfoCount( UINT* result )
{
    *result = 0;
    return E_NOTIMPL;
}

*/
//-------------------------------------------------------------------Hostware_variable::GetTypeInfo
/*

HRESULT Hostware_variable::GetTypeInfo( UINT, LCID, ITypeInfo** typeinfo )
{
    return E_NOTIMPL;
}

*/
//-----------------------------------------------------------------Hostware_variable::GetIDsOfNames
#ifndef SYSTEM_HAS_COM
/*
HRESULT Hostware_variable::GetIDsOfNames( REFIID, LPOLESTR* name_w, UINT, LCID, DISPID* dispid )
{
    return DISP_E_UNKNOWNNAME;
}
*/
#endif
//-----------------------------------------------------------------Hostware_variable::GetIDsOfNames

STDMETHODIMP Hostware_variable::GetIDsOfNames( const IID& iid, OLECHAR** names, UINT names_count, LCID lcid, DISPID* result )
{
    //Z_LOG( __PRETTY_FUNCTION__ << '\n' );
    
    HRESULT hr = S_OK;
    
#   ifdef Z_COM
        hr = ::zschimmer::com::Com_get_dispid( _methods, iid, names, names_count, lcid, result );
#    else
        hr = Sos_ole_object::GetIDsOfNames( iid, names, names_count, lcid, result );                                                    
#   endif


    if( hr == DISP_E_UNKNOWNNAME
     || hr == DISP_E_MEMBERNOTFOUND )  // Dieser Fehlercode kommt, wenn der Name nicht bekannt ist.
    {
        if( _value.vt == VT_DISPATCH )
        {
            ptr<Ivariables> variables;

            HRESULT hr2 = V_DISPATCH(&_value)->QueryInterface( IID_Ivariables, variables.void_pp() );
            if( !FAILED(hr2) )
            {
                hr = variables->GetIDsOfNames( iid, names, names_count, lcid, result );
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------Hostware_variable::Invoke
    
STDMETHODIMP Hostware_variable::Invoke( DISPID dispid, const IID& iid, LCID lcid, unsigned short wFlags, DISPPARAMS* dispparams, 
                                        VARIANT* result, EXCEPINFO* excepinfo, UINT* arg_nr )
{
    //Z_LOG( __PRETTY_FUNCTION__ << '\n' );
    
    HRESULT hr = S_OK;

    if( dispid < dispid_offset )
    {
#       ifdef Z_COM
            hr = ::zschimmer::com::Com_invoke( static_cast<Ivariable2*>( this ), _methods, dispid, iid, lcid, wFlags, dispparams, arg_nr, result, excepinfo );
#        else
            hr = Sos_ole_object::Invoke( dispid, iid, lcid, wFlags, dispparams, result, excepinfo, arg_nr );
#       endif
    }
    else
    {
        if( _value.vt == VT_DISPATCH )
        {
            ptr<Ivariables> variables;

            HRESULT hr2 = V_DISPATCH(&_value)->QueryInterface( IID_Ivariables, variables.void_pp() );
            if( !FAILED(hr2) )
            {
                hr = variables->Invoke( dispid, iid, lcid, wFlags, dispparams, result, excepinfo, arg_nr );
            }
        }
    }

    return hr;
}

//----------------------------------------------------------------------Hostware_variable::by_index

HRESULT Hostware_variable::copy_array_element( VARIANT* index, VARIANT* param, bool write )
{
#ifndef SYSTEM_HAS_COM
    return E_NOTIMPL;
#else
    HRESULT         hr     = NOERROR;
    long            idx    = int_from_variant( *index );
    SAFEARRAYBOUND  bound; 
    long            ubound = 0;
    VARTYPE         vartype = VT_EMPTY;


    if( !( _value.vt & VT_ARRAY ) )  
    {
        if( _value.vt != VT_EMPTY )  throw_xc( "SOS-1430" );

        hr = Dim( 1 );  if( FAILED(hr) )  return hr;
    }

    if( SafeArrayGetDim( _value.parray ) != 1 )  throw_ole( DISP_E_BADINDEX, "hostware.Variable" );

    hr = z_SafeArrayGetVartype( _value.parray, &vartype );    if( FAILED(hr) )  return hr;
    if( vartype != VT_VARIANT )  return DISP_E_TYPEMISMATCH;

    if( write )
    {
        hr = SafeArrayGetUBound( _value.parray, 1, &ubound );                   if( FAILED(hr) )  return hr;

        if( idx == ubound + 1 ) 
        {
            hr = SafeArrayGetLBound( _value.parray, 1, &bound.lLbound );        if( FAILED(hr) )  return hr;

            bound.cElements = idx - bound.lLbound + 1;
            hr = SafeArrayRedim( _value.parray, &bound );                       if( FAILED(hr) )  return hr;
        }

        Variant        helper;
        const VARIANT* value = normalized_value( param, &helper );
        hr = SafeArrayPutElement( _value.parray, &idx, &value );                if( FAILED(hr) )  return hr;
    }
    else
    {
        hr = SafeArrayGetElement( _value.parray, &idx, param );                 if( FAILED(hr) )  return hr;
    }

    return hr;
#endif
}

//---------------------------------------------------------------------Hostware_variable::put_value

STDMETHODIMP Hostware_variable::put_Value( VARIANT* index, VARIANT* value )
{
    HRESULT hr = NOERROR;

    //fprintf( stderr, "Hostware_variable::put_value %s\n", variant_as_string(*value).c_str() );

    try
    {
        THREAD_LOCK( _lock )  
        {
            if( com_param_given( index ) )
            {
                hr = copy_array_element( index, value, true );
            }
            else
            {
                //LOG( __PRETTY_FUNCTION__ << "() " << (void*)this << " " << _name << " := " << debug_string_from_variant( *value ) << "  (vorher: " << debug_string_from_variant( _value ) << ")\n" );
                //hr = VariantCopy( &_value, value );
                assign( *value );
            }
        }
    }
    catch( const Xc&        x )  { hr = _set_excepinfo( x, "hostWare.Variable::value" ); }
    catch( const exception& x )  { hr = _set_excepinfo( x, "hostWare.Variable::value" ); }
    
    return hr;
}

//---------------------------------------------------------------------Hostware_variable::put_value

STDMETHODIMP Hostware_variable::get_Value( VARIANT* index, VARIANT* result )
{
    HRESULT hr = NOERROR;

    //fprintf( stderr, "Hostware_variable::get_value " );
    //fprintf( stderr, "vt=%x pbstrVal=%x", _value.vt, (int)_value.pbstrVal );
    //fprintf( stderr, "bstr=%c%c", _value.pbstrVal[0], _value.pbstrVal[1]  );
    //fprintf( stderr, "%s\n", variant_as_string(_value).c_str() );

    try
    {
        VariantInit( result );

        THREAD_LOCK( _lock )  
        {
            if( com_param_given( index ) )
            {
                hr = copy_array_element( index, result, false );
            }
            else
            {

                hr = VariantCopy( result, &_value );
            }
        }
    }
    catch( const Xc&        x )  { hr = _set_excepinfo( x, "hostWare.Variable::value" ); }
    catch( const exception& x )  { hr = _set_excepinfo( x, "hostWare.Variable::value" ); }

    return hr;
}

//---------------------------------------------------------------------------Hostware_variable::dim

STDMETHODIMP Hostware_variable::Dim( int size )
{
#ifndef SYSTEM_HAS_COM
    return E_NOTIMPL;
#else
    HRESULT        hr = NOERROR;
    SAFEARRAYBOUND bound;

    bound.cElements = size;
    bound.lLbound   = 0;

    if( _value.vt & VT_ARRAY )
    {
        if( SafeArrayGetDim( _value.parray ) != 1 )  return DISP_E_BADINDEX;

        hr = SafeArrayGetLBound( _value.parray, 0, &bound.lLbound );
        if( FAILED(hr) )  return hr;

        if( bound.lLbound == 0 )  bound.cElements++;      // 0...size

        hr = SafeArrayRedim( _value.parray, &bound );
        if( FAILED(hr) )  return hr;
    }
    else
    {
        _value.Clear();

        bound.cElements++;        // 0...size
        
        SAFEARRAY* a = SafeArrayCreate( VT_VARIANT, 1, &bound );
        if( !a )  return E_OUTOFMEMORY;

        _value.parray = a;
        _value.vt = VT_VARIANT | VT_ARRAY;
    }

    return hr;
#endif
}

//---------------------------------------------------------------------Hostware_variables::_methods
#ifdef Z_COM

const Com_method Hostware_variables::_methods[] =
{ 
   // _flags              ,     _name            , _method                                                    , _result_type, _types        , _default_arg_count
  //{ DISPATCH_PROPERTYGET,  1, "Java_class_name", (Com_method_ptr)&Hostware_variables::get_java_class_name   , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 0, "obj_value"       , (Com_method_ptr)&Ivariables2::put_Obj_value          , VT_EMPTY    , { VT_BSTR, VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYGET, 0, "obj_value"       , (Com_method_ptr)&Ivariables2::get_Obj_value          , VT_VARIANT  , { VT_BSTR             } },
    { DISPATCH_METHOD     , 2, "obj_is_empty"    , (Com_method_ptr)&Ivariables2::get_Obj_is_empty       , VT_BOOL }, 
    { DISPATCH_METHOD     , 3, "obj_clone"       , (Com_method_ptr)&Ivariables2::Obj_clone              , VT_DISPATCH }, 
  //{ DISPATCH_PROPERTYGET, DISPID_NEWENUM, "_NewEnum", (Com_method_ptr)&Ivariables2::get__NewEnum      , VT_UNKNOWN }, 
    { DISPATCH_PROPERTYPUT, 4, "obj_xml"         , (Com_method_ptr)&Ivariables2::get_Obj_xml            , VT_EMPTY    , { VT_BSTR } }, 
    { DISPATCH_PROPERTYGET, 4, "obj_xml"         , (Com_method_ptr)&Ivariables2::get_Obj_xml            , VT_BSTR     }, 
    {}
};

#endif
//-----------------------------------------------------------Hostware_variables::Hostware_variables

Hostware_variables::Hostware_variables( Ivariables* )
:
    Sos_ole_object( variables_class_ptr, static_cast<Ivariables*>( this ) ),
    _lock( "Hostware_variables" )
{
}

//-----------------------------------------------------------Hostware_variables::Hostware_variables

Hostware_variables::Hostware_variables( Ivariables2* )
:
    Sos_ole_object( variables2_class_ptr, static_cast<Ivariables2*>( this ) ),
    _lock( "Hostware_variables" )
{
}

//----------------------------------------------------------Hostware_variables::~Hostware_variables

Hostware_variables::~Hostware_variables()
{
}

//---------------------------------------------------------------Hostware_variables::QueryInterface

STDMETHODIMP Hostware_variables::QueryInterface( REFIID iid, void** result )
{                                     
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ivariables , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ivariables2, result );

    if( iid == IID_Variables )      // Das ist diese Implementierung Hostware_variable
    {
        *result = this;
        AddRef();
        return S_OK;
    }

    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//----------------------------------------------------------------Hostware_variables::GetIDsOfNames

STDMETHODIMP Hostware_variables::GetIDsOfNames( const IID& iid, OLECHAR** names, UINT names_count, LCID lcid, DISPID* result )
{
    //Z_LOG( __PRETTY_FUNCTION__ << '\n' );
    
    HRESULT hr = S_OK;
    
#   ifdef Z_COM
        hr = ::zschimmer::com::Com_get_dispid( _methods, iid, names, names_count, lcid, result );
#    else
        hr = Sos_ole_object::GetIDsOfNames( iid, names, names_count, lcid, result );                                                    
#   endif


    if( hr == DISP_E_UNKNOWNNAME
     || hr == DISP_E_MEMBERNOTFOUND )  // Dieser Fehlercode kommt, wenn der Name nicht bekannt ist.
    {
        if( iid != IID_NULL  )  return DISP_E_UNKNOWNINTERFACE;
        if( names_count != 1 )  return DISP_E_MEMBERNOTFOUND;

        Bstr name = names[0];
        name.to_lower();

        THREAD_LOCK( _lock )
        {
            Map::iterator it = _map.find( name );
            if( it == _map.end() )  return DISP_E_MEMBERNOTFOUND;

            Hostware_variable* var = it->second;

            if( var->_dispid == 0 ) 
            {
                _names.push_back( name );
                var->_dispid = dispid_offset + _names.size() - 1;
            }
            else
            {
                assert( _names.at( var->_dispid - dispid_offset ) == name );
            }


            *result = var->_dispid;
        }

        hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------------Hostware_variables::Invoke
    
STDMETHODIMP Hostware_variables::Invoke( DISPID dispid, const IID& iid, LCID lcid, unsigned short wFlags, DISPPARAMS* dispparams, 
                                         VARIANT* result, EXCEPINFO* excepinfo, UINT* arg_nr )
{
    //Z_LOG( __PRETTY_FUNCTION__ << '\n' );
    
    HRESULT hr = S_OK;

    if( dispid < dispid_offset )
    {
#       ifdef Z_COM
            hr = ::zschimmer::com::Com_invoke( static_cast<Ivariables2*>( this ), _methods, dispid, iid, lcid, wFlags, dispparams, arg_nr, result, excepinfo );
#        else
            hr = Sos_ole_object::Invoke( dispid, iid, lcid, wFlags, dispparams, result, excepinfo, arg_nr );
#       endif
    }
    else
    {
        THREAD_LOCK( _lock )
        {
            Bstr& name = _names.at( dispid - dispid_offset );
            assert( _map[ name ]->_dispid == dispid );

            if( wFlags & ( DISPATCH_PROPERTYGET | DISPATCH_METHOD ) )
            {
                if( dispparams->cNamedArgs > 0 )  return DISP_E_NONAMEDARGS;
                if( dispparams->cArgs      > 0 )  return DISP_E_BADPARAMCOUNT;
                if( !result                    )  return E_INVALIDARG;

                result->vt = VT_DISPATCH;
                hr = get_Value( name, (Ivariable**)&V_DISPATCH( result ) );
            }
            else
            if( wFlags & ( DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF ) )
            {
                if( dispparams->cNamedArgs != 1 )  return DISP_E_BADPARAMCOUNT;
                if( dispparams->cArgs      != 1 )  return DISP_E_BADPARAMCOUNT;
                hr = put_Value( name, &dispparams->rgvarg[ 0 ] );
            }
            else
                return E_FAIL;
        }
    }

    return hr;
}

//------------------------------------------------------------------------Hostware_variables::Clone
// Doppelt implementiert. s.a. Obj_clone

STDMETHODIMP Hostware_variables::Clone( Ivariables** result )
{
    HRESULT hr = NOERROR;

    try
    {
        ptr<Hostware_variables> vs = new Hostware_variables;
        
        Z_MUTEX( _lock )
        {
            Z_FOR_EACH( Map, _map, it )
            {
                Hostware_variable* v = it->second;
                if( v->_value.vt != VT_EMPTY )  vs->_map[ it->first ] = new Hostware_variable( v->_name, v->_value, (Ivariable*)NULL );
            }
        }

        *result = vs;        
        (*result)->AddRef();
    }
    catch( const exception& x )  { hr = _set_excepinfo( x, "hostWare.Variable::clone" ); }

    return hr;
}

//--------------------------------------------------------------------Hostware_variables::Obj_clone
// s.a. Clone

STDMETHODIMP Hostware_variables::Obj_clone( Ivariables2** result )
{ 
    HRESULT hr = NOERROR;

    try
    {
        ptr<Hostware_variables> vs = new Hostware_variables( (Ivariables2*)NULL );
        
        Z_MUTEX( _lock )
        {
            Z_FOR_EACH( Map, _map, it )
            {
                Hostware_variable* v = it->second;
                if( v->_value.vt != VT_EMPTY )  vs->_map[ it->first ] = new Hostware_variable( v->_name, v->_value, (Ivariable2*)NULL );
            }
        }

        *result = static_cast<Ivariables2*>( vs.take() );
    }
    catch( const exception& x )  { hr = _set_excepinfo( x, "hostWare.Variable::clone" ); }

    return hr;
}

//--------------------------------------------------------------------Hostware_variables::put_value

STDMETHODIMP Hostware_variables::put_Value( BSTR name, VARIANT* value )
{
    HRESULT hr = NOERROR;

    try
    {
        if( SysStringLen(name) == 0 )  throw_xc( "SOS-1454", "" );

        Bstr nam = name;

        nam.to_lower();

        THREAD_LOCK( _lock )  
        {
            Map::iterator m = _map.find( nam );

            if( value->vt == VT_EMPTY ) 
            {
                if( m != _map.end() )  
                {
                    if( m->second->_dispid == 0 )  _map.erase( m );
                                             else  hr = m->second->_value.Clear();   // Die Dispid eines Namens darf sich nicht ändern. Also Variable stehenlassen!
                }
            }
            else
            {
                Variant my_value;

                if( m == _map.end() )    // Neu?
                {
                    if( _ole_class_descr == variables_class_ptr )
                        _map[nam] = new Hostware_variable( name, *value, (Ivariable*)NULL );
                    else
                        _map[nam] = new Hostware_variable( name, *value, (Ivariable2*)NULL );
                }
                else                    // Variable ändern
                {
                    Variant index ( Variant::vt_missing );
                    //hr = _map[nam]->put_Value( &index, value );
                    hr = m->second->put_Value( &index, value );
                }
            }
        }
    }
    catch( const exception& x )  { hr = _set_excepinfo( x, "hostWare.Variable::value" ); }

    return hr;
}

//--------------------------------------------------------------------Hostware_variables::get_value

STDMETHODIMP Hostware_variables::get_Value( BSTR name, Ivariable** result )
{
    HRESULT hr = NOERROR;

    Bstr nam = name;
    nam.to_lower();

    THREAD_LOCK( _lock )  
    {
        if( SysStringLen(name) == 0 )  throw_xc( "SOS-1454", "" );

        ptr<Hostware_variable>& v = _map[ nam ];

        if( !v )  
        {
            if( _ole_class_descr == variables_class_ptr )
                v = _map[nam] = new Hostware_variable( name, empty_variant, (Ivariable*)NULL );
            else
                v = _map[nam] = new Hostware_variable( name, empty_variant, (Ivariable2*)NULL );
        }

        *result = v;
        (*result)->AddRef();
    }

    //fprintf( stderr, "Hostware_variables::get_value %s => %s\n", string_from_bstr(name).c_str(), string_from_val

    return hr;
}

//----------------------------------------------------------------Hostware_variables::get_Obj_value

STDMETHODIMP Hostware_variables::get_Obj_value( BSTR name, VARIANT* result )
{
    HRESULT hr = NOERROR;

    Bstr nam = name;
    nam.to_lower();

    THREAD_LOCK( _lock )  
    {
        if( SysStringLen(name) == 0 )  throw_xc( "SOS-1454", "" );

        Map::iterator it = _map.find( nam );
        if( it == _map.end() )
        {
            result->vt = VT_EMPTY;
        }
        else
        {
            hr = VariantCopy( result, &it->second->_value );
        }
    }

    return hr;
}

//--------------------------------------------------------------------Hostware_variables::get_count

STDMETHODIMP Hostware_variables::get_Count( int* result )
{
    THREAD_LOCK( _lock )  *result = _map.size();
    return NOERROR;
}

//------------------------------------------------------------------------Hostware_variables::merge
/*
STDMETHODIMP Hostware_variables::merge( Ivariables* to_add, VARIANT_BOOL overwrite )
{
    for( Map::iterator it = to_add->_map.begin(); it != to_add->_map.end(); it++ )
    {
        bool o = overwrite;

        if( !o )
        {
            Map::iterator m = _map.find( it->primary );
            o |= m == _map.end()  ||  !m->secondary  ||  m->secondary->vt == VT_EMPTY;
        }

        if( o )  map[ it->primary ] = it->second;
    }
}
*/

//------------------------------------------------------------------Hostware_variables::get_variant

Variant Hostware_variables::get_variant( const BSTR name )
{
    Bstr nam = name;
    nam.to_lower();

    THREAD_LOCK( _lock )  
    {
        Map::iterator m = _map.find( nam );
        if( m != _map.end() )  return static_cast<IDispatch*>( static_cast<Ivariable*>( m->second ) );
    }

    return Variant();
}

//-----------------------------------------------------------------Hostware_variables::get__NewEnum

STDMETHODIMP Hostware_variables::get__NewEnum( IUnknown** result )
{
    ptr<Hostware_variables_enumerator> e = new Hostware_variables_enumerator;
    e->initialize( this );

    *result = static_cast< IEnumVARIANT* >( e.take() );

    return NOERROR;
}

//---------------------------------------------------------------Hostware_variables::Obj_enumerator

STDMETHODIMP Hostware_variables::Obj_enumerator( Ivariables2_idispatch_enumerator** result )
{
    ptr<Hostware_variables2_idispatch_enumerator> e = new Hostware_variables2_idispatch_enumerator( this );

    *result = e.take();

    return NOERROR;
}

//-----------------------------------------------------------------Hostware_variables::get__NewEnum
/*
STDMETHODIMP Hostware_variables::get__NewEnum( Ivariables_enumerator** result )
{
    ptr<Hostware_variables_enumerator> e = new Hostware_variables_enumerator;
    e->initialize( this );

    *result = e;
    (*result)->AddRef();

    return S_OK;
}
*/
//----------------------------------------------------------------------Hostware_variables::set_dom
/*
void Hostware_variables::set_dom( const xml::Element_ptr& params )
{
    HRESULT hr;

    THREAD_LOCK( _lock )
    {
        DOM_FOR_EACH_ELEMENT( params, e )
        {
            if( e.nodeName_is( "param" ) ) 
            {
                Bstr    name  = e.getAttribute( "name" );
                Variant value = e.getAttribute( "value" );

                hr = put_var( name, &value );                       if( FAILED(hr) )  throw_ole( hr, "Ivariable_set::put_var" );
            }
        }
    }
}
*/
//--------------------------------------------------------------------------Hostware_variables::dom

xml::Document_ptr Hostware_variables::dom()
{
    xml::Document_ptr doc;
    
    doc.create();
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    doc.appendChild( dom_element( doc, xml_element_name, xml_subelement_name ) );

    return doc;
}

//------------------------------------------------------------------Hostware_variables::dom_element

xml::Element_ptr Hostware_variables::dom_element( const xml::Document_ptr& doc, const string& element_name, const string& subelement_name )
{
    xml::Element_ptr varset = doc.createElement( element_name );

    THREAD_LOCK( _lock )
    {
        for( Map::iterator it = _map.begin(); it != _map.end(); it++ )
        {
            Hostware_variable* v = it->second;
            if( v )
            {
                Bstr    name;
                Variant value;
                Variant missing ( Variant::vt_missing );

                v->get_Name( &name );
                v->get_Value( &missing, &value );

                xml::Element_ptr var = doc.createElement( subelement_name );
                var.setAttribute( "name" , string_from_bstr(name) );

                VARTYPE vt = (VARTYPE)-1;

                if( value.vt == VT_EMPTY
                 || value.vt == VT_NULL
                 || value.vt == VT_I2
                 || value.vt == VT_I4
                 || value.vt == VT_R4       
                 || value.vt == VT_R8       
                 || value.vt == VT_CY       
                 || value.vt == VT_DATE     
              // || value.vt == VT_BSTR          // VT_BSTR müssen wir nicht besonders kennzeichnen. Das ist Default.
              // || value.vt == VT_DISPATCH 
                 || value.vt == VT_ERROR    
                 || value.vt == VT_BOOL     
              // || value.vt == VT_VARIANT  
              // || value.vt == VT_UNKNOWN  
                 || value.vt == VT_DECIMAL  
                 || value.vt == VT_I1       
                 || value.vt == VT_UI1      
                 || value.vt == VT_UI2      
                 || value.vt == VT_UI4      
                 || value.vt == VT_I8       
                 || value.vt == VT_UI8      
                 || value.vt == VT_INT      
                 || value.vt == VT_UINT     
              // || value.vt == VT_VOID     
              // || value.vt == VT_HRESULT 
                )
                {
                    vt = value.vt; 
                }

                if( value.vt == VT_DISPATCH )
                {
                    zschimmer::qi_ptr<Ivariables> v;
                    v.try_assign_qi( V_DISPATCH( &value ) );  if( !v )  throw_xc( "SOS-1462", string_from_bstr(name) );

                    var.appendChild( DYNAMIC_CAST( Hostware_variables*, +v )->dom_element( doc, element_name, subelement_name ) );
                }
                else
                {
                    if( vt != (VARTYPE)-1 )  var.setAttribute( "vt", vartype_name( vt ) );
                                       else  {} // Andere Typen sind nicht rückkonvertierbar. Die werden dann zum String.  

                    string type;  // Für andere Software der SOS
                    if( vt == VT_BOOL            )  type = "Boolean";
                    else
                    if( vt == VT_CY              )  type = "Currency";
                    else
                    if( vt == VT_DATE            )  type = "Date";
                    else
                    if( variant_is_integer( vt ) )  type = "Long";
                    else
                    if( variant_is_double( vt )  )  type = "Double";

                    if( type != "" )  var.setAttribute( "type", type );


                    string text = vt == VT_ERROR  ? as_string( V_ERROR( &value ) )
                                                  : string_from_variant( value );


                    if( !text.empty() )  var.setAttribute( "value", text );
                }

                varset.appendChild( var );
            }
        }
    }

    return varset;
}

//----------------------------------------------------------------------Hostware_variables::put_Xml

STDMETHODIMP Hostware_variables::put_Xml( BSTR xml_text )  
{ 
    HRESULT hr = NOERROR;

    try
    {
        xml::Document_ptr doc = dom();
        doc.load_xml( string_from_bstr( xml_text ) );

        load_dom( doc, doc.documentElement() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------Hostware_variables::load_dom

void Hostware_variables::load_dom( const xml::Document_ptr& doc, const xml::Element_ptr& element )
{ 
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( xml_subelement_name ) ) //|| e.nodeName_is( "param" ) )
        {
            Bstr    name            = e.getAttribute( "name"  );
            string  value_string    = e.getAttribute( "value" );
            string  vartype_string  = e.getAttribute( "vt"    );
            string  type            = e.getAttribute( "type"  );    // falls vt nicht angegeben ist
            Variant value;

            if( value_string.empty()  &&  vartype_string.empty() )
            {
                if( xml::Node_ptr n = e.firstChild() )
                {
                    while( n  &&  n.nodeType() != xml::ELEMENT_NODE )  n = n.nextSibling();
                    if( n )
                    {
                        if( !n.nodeName_is( xml_element_name ) )  throw_xc( "SOS-1461", xml_element_name );

                        ptr<Hostware_variables> v = _ole_class_descr == variables_class_ptr? new Hostware_variables( (Ivariables*)NULL )
                                                                                           : new Hostware_variables( (Ivariables2*)NULL );
                        v->load_dom( doc, xml::Element_ptr( n ) );
                        value = static_cast<IDispatch*>( static_cast<Ivariables*>( v ) );
                    }
                }
            }
            else
            {
                VARTYPE vt = VT_EMPTY;
                
                if( vartype_string != "" )
                {
                    vt = vartype_from_name( vartype_string );
                }
                else
                if( type == "Boolean"  )
                {
                    vt = VT_BOOL;
                    if( value_string == "y"  ||  value_string == "Y" )  value_string = "1";
                    if( value_string == "n"  ||  value_string == "N" )  value_string = "0";
                }
                else
                if( type == "Currency" )  vt = VT_CY;
                else
                if( type == "Date"     )  vt = VT_DATE;
                else
                if( type == "Long"     )  vt = VT_I4;
                else
                if( type == "Double"   )  vt = VT_R8;
                else
                if( type == "String"   )  vt = VT_BSTR;
                else
                                          vt = VT_BSTR;

                value = value_string;
                value.change_type( vt );
            }

            HRESULT hr = put_Value( name, &value );
            if( FAILED( hr ) )  break;
        }
        else
            throw_xc( "SOS-1461", xml_subelement_name );
    }
}

//--------------------------------------------------------------------------Hostware_variables::Xml

STDMETHODIMP Hostware_variables::get_Xml( BSTR* result )  
{ 
    HRESULT hr = NOERROR;

    try
    {
        hr = String_to_bstr( dom().xml(), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------Hostware_variables_enumerator::Hostware_variables_enumerator

Hostware_variables_enumerator::Hostware_variables_enumerator()
:
    Sos_ole_object( variables_enumerator_class_ptr, static_cast< Ivariables_enumerator* >( this ) )
  //_iid(IID_Ivariable)
{
}

//----------------------------------------------------Hostware_variables_enumerator::QueryInterface

STDMETHODIMP Hostware_variables_enumerator::QueryInterface( REFIID iid, void** result )
{                              
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IEnumVARIANT, result );

    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//--------------------------------------------------------Hostware_variables_enumerator::initialize

void Hostware_variables_enumerator::initialize( Hostware_variables* v )
{
    _variables = v;
    Reset();
}

//--------------------------------------------------------------Hostware_variables_enumerator::Next

STDMETHODIMP Hostware_variables_enumerator::Next( ULONG celt, VARIANT* rgvar, ULONG* pceltFetched )
{
    int i = 0;

    while( i < celt  &&  _iterator != _variables->_map.end() )
    {
        VariantInit( rgvar );
        
        rgvar->vt       = VT_DISPATCH;
        rgvar->pdispVal = static_cast< IDispatch* >( static_cast<Ivariable*>( (_iterator++)->second ) );
        if( rgvar->pdispVal )  rgvar->pdispVal->AddRef();

        rgvar++;
        i++;
    }

    if( pceltFetched )  *pceltFetched = i;

    return i < celt? S_FALSE : S_OK;
}

//--------------------------------------------------------------Hostware_variables_enumerator::Skip

STDMETHODIMP Hostware_variables_enumerator::Skip( ULONG celt )
{
    while( celt &&  _iterator != _variables->_map.end() )  _iterator++;

    return celt? S_FALSE : S_OK;
}

//-------------------------------------------------------------Hostware_variables_enumerator::Reset

STDMETHODIMP Hostware_variables_enumerator::Reset()
{
    _iterator = _variables->_map.begin();
    return NOERROR;
}

//-------------------------------------------------------------Hostware_variables_enumerator::Clone

STDMETHODIMP Hostware_variables_enumerator::Clone( IEnumVARIANT** )
{
    return ERROR;
}

//-----------------------------------------------Hostware_variables2_idispatch_enumerator::_methods
#ifdef Z_COM

const Com_method Hostware_variables2_idispatch_enumerator::_methods[] =
{ 
   // _flags              ,    _name             , _method                                                          , _result_type, _types        , _default_arg_count
    { DISPATCH_METHOD     , 1, "Next"            , (Com_method_ptr)&Ivariables2_idispatch_enumerator::Next          , VT_DISPATCH }, 
    { DISPATCH_PROPERTYGET, 2, "Has_next"        , (Com_method_ptr)&Ivariables2_idispatch_enumerator::get_Has_next  , VT_BOOL}, 
    {}
};

#endif
//------------------Hostware_variables2_idispatch_enumerator::Hostware_variables2_idispatch_enumerator

Hostware_variables2_idispatch_enumerator::Hostware_variables2_idispatch_enumerator( Hostware_variables* v )
:
    Sos_ole_object( &variables2_idispatch_enumerator_class, static_cast< Ivariables2_idispatch_enumerator* >( this ) )
{
    _variables = v;
    _iterator = _variables->_map.begin();
}

//------------------------------------------Hostware_variables2_idispatch_enumerator::QueryInterface

STDMETHODIMP Hostware_variables2_idispatch_enumerator::QueryInterface( REFIID iid, void** result )
{                              
    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//----------------------------------------------------Hostware_variables2_idispatch_enumerator::Next

STDMETHODIMP Hostware_variables2_idispatch_enumerator::Next( Ivariable2** result )
{
    HRESULT hr;

    if( _iterator != _variables->_map.end() )
    {
        *result = static_cast< Ivariable2* >( (_iterator++)->second );
        if( *result )  (*result)->AddRef();
        hr = S_OK;
    }
    else
    {
        *result = NULL;
        hr = S_FALSE;
    }

    return hr;
}

//-------------------------------------------Hostware_variables2_idispatch_enumerator::get_Has_next

STDMETHODIMP Hostware_variables2_idispatch_enumerator::get_Has_next( VARIANT_BOOL* result )
{
    *result = _iterator != _variables->_map.end()? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
