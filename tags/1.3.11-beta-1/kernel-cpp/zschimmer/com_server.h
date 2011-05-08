// $Id$

#ifndef __COM_SERVER_H
#define __COM_SERVER_H

#include "z_com.h"
#include "mutex.h"

#ifdef Z_COM


#ifdef INTERFACE
#   undef INTERFACE             // Aus C:\Programme\Microsoft Visual Studio .NET 2003\Vc7\PlatformSDK\Include\CommDlg.h(920)
#endif


namespace zschimmer {
namespace com {

//-------------------------------------------------------------------------------------------------

struct                          Com_class_descriptor;   // in z_com_server.h
struct                          Com_method;

//-------------------------------------------------------------------------------------------------

//typedef HRESULT              (* Com_class_factory_creator )( const CLSID&, const IID&, IUnknown** );    // Für perl_scripting_engine.cxx

//HRESULT                         com_get_class_object    ( Com_class_factory_creator, const CLSID&, const IID&, void** result );
HRESULT                         Com_get_class_object    ( const Com_class_descriptor*, const CLSID&, const IID&, void** result );
HRESULT                         Cet_error_info          ( HRESULT, EXCEPINFO* );

//--------------------------------------------------------------------------------Com_module_params

struct Com_module_params
{
    int                        _version;
    int                        _subversion;
    const Com_context*         _com_context;
    ostream**                  _log_stream__deprecated;
    Log_context::Log_mutex*    _log_system_mutex__deprecated;
    Log_context**              _log_context;
};

HRESULT                         Apply_com_module_params ( Com_module_params* );

//--------------------------------------------------------------------------------Com_class_factory
/*
struct Com_class_factory : IClassFactory
{
                                Com_class_factory       ( const CLSID& clsid, Com_class_factory_creator creator )  : _ref_count(0), _clsid(clsid), _creator(creator) {}
    virtual                    ~Com_class_factory       ()                                          {}

    STDMETHODIMP                QueryInterface          ( const IID&, void** );
    STDMETHODIMP_(ULONG)        AddRef                  ()                                          { _ref_count++; return S_OK; }
    STDMETHODIMP_(ULONG)        Release                 ()                                          { --_ref_count; int r = _ref_count; if( r == 0 )  delete this;  return r; }

    STDMETHODIMP                CreateInstance          ( IUnknown* outer, const IID&, void** );

  private:
    int                        _ref_count;
    CLSID                      _clsid;
    Com_class_factory_creator  _creator;
};
*/
HRESULT         Com_get_dispid              ( const Com_method* methods, REFIID, LPOLESTR* rgszNames, UINT cNames, LCID, DISPID* );

HRESULT         Com_invoke                  ( IDispatch*, const Com_method* methods, DISPID dispid, 
                                              REFIID, LCID, WORD flags, DISPPARAMS*, uint* argnr,
                                              VARIANT* result, EXCEPINFO* excepinfo );

HRESULT         Com_get_type_info_count     ( IDispatch*, const Com_method*, unsigned int* result );
HRESULT         Com_get_type_info           ( IDispatch*, const Com_method*, unsigned int info_number, LCID, ITypeInfo** );

//-----------------------------------------------------------------------Z_GETIDSOFNAMES_AND_INVOKE

#define Z_DEFINE_GETIDSOFNAMES_AND_INVOKE                                                               \
                                                                                                        \
    HRESULT GetIDsOfNames( REFIID iid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId )  \
    {                                                                                                   \
        return ::zschimmer::com::Com_get_dispid( _methods, iid, rgszNames, cNames, lcid, rgDispId );    \
    }                                                                                                   \
                                                                                                        \
    HRESULT Invoke( DISPID dispid, REFIID iid, LCID lcid, WORD flags,                                   \
                    DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* argnr )        \
    {                                                                                                   \
        return ::zschimmer::com::Com_invoke( static_cast<IDispatch*>( this), _methods,                  \
                                             dispid, iid, lcid, flags,                                  \
                                             dispparams, argnr, result, excepinfo );                    \
    }                                                                                                   \
                                                                                                        \
    static const ::zschimmer::com::Com_method _methods[];

//-----------------------------------------------------------------------Z_GETIDSOFNAMES_AND_INVOKE
/*
#define Z_DEFINE_GETIDSOFNAMES_AND_INVOKE_INHERITED                                                     \
                                                                                                        \
    HRESULT GetIDsOfNames( REFIID iid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId )  \
    {                                                                                                   \
        HRESULT hr = ::zschimmer::com::Com_get_dispid( _methods, iid, rgszNames, cNames, lcid, rgDispId ); \
        if( hr == DISP_E_UNKNOWNNAME )                                                                  \
            hr = Base_class::GetIDsOfNames( iid, rgszNames, cNames, lcid, rgDispId );                   \
        return hr;
    }                                                                                                   \
                                                                                                        \
    HRESULT Invoke( DISPID dispid, REFIID iid, LCID lcid, WORD flags,                                   \
                    DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* argnr )        \
    {                                                                                                   \
        HRESULT hr = ::zschimmer::com::Com_invoke( (IDispatch*)this, _methods, dispid, iid, lcid, flags,\
                                                   dispparams, argnr, result, excepinfo );              \
        if( hr == DISP_E_MEMBERNOTFOUND )                                                               \
            hr = Base_class::Invoke( dispid, iid, lcid, flags, dispparanms, result, excepinfo, argnr ); \
        return hr;                                                                                      \
    }                                                                                                   \
                                                                                                        \
    static const ::zschimmer::com::Com_method _methods[];
*/
//------------------------------------------------------------------------------Z_DEFINE_GETTYPEINFO

#define Z_DEFINE_GETTYPEINFO                                                                            \
                                                                                                        \
    HRESULT GetTypeInfoCount( unsigned int* result )                                                    \
    {                                                                                                   \
        return ::zschimmer::com::Com_get_type_info_count( (IDispatch*)this, _methods, result );         \
    }                                                                                                   \
                                                                                                        \
    HRESULT GetTypeInfo( unsigned int info_number, LCID lcid, ITypeInfo** result )                      \
    {                                                                                                   \
        return ::zschimmer::com::Com_get_type_info( (IDispatch*)this, _methods, info_number, lcid, result );   \
    }

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer

#endif
#endif
