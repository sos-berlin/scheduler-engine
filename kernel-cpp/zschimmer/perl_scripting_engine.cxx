// $Id: perl_scripting_engine.cxx 13795 2009-06-23 11:13:12Z sos $

#include "zschimmer.h"
#include "z_com.h"
#include "com_server.h"
#include "z_com_server.h"
#include "scripting_engine.h"
#include "perl_scripting_engine.h"
#include "perl.h"

using namespace std;
using namespace zschimmer;
using namespace zschimmer::com;

//#ifdef COM_STATIC
    extern "C" BOOL WINAPI      sosperl_DllMain         ( HANDLE hInst, DWORD ul_reason_being_called, void* )  COM_LINKAGE;
//#endif

//----------------------------------------------------------------------------Perl_scripting_engine

struct Perl_scripting_engine : creatable_iunknown_implementation< CLSID_PerlScript, Perl_scripting_engine, IActiveScript >,
                               IActiveScriptParse
{
                                Perl_scripting_engine   ()                                          : _zero_(this+1), Iunknown_implementation(NULL) {}
    virtual                    ~Perl_scripting_engine   ()                                          {}


    virtual STDMETHODIMP        QueryInterface          ( const IID&, void** );
    virtual ULONG               AddRef                  ()                                          { return Iunknown_implementation::AddRef(); }
    virtual ULONG               Release                 ()                                          { return Iunknown_implementation::Release(); }


    // IActiveScript
    virtual HRESULT             SetScriptSite           ( IActiveScriptSite* );
    virtual HRESULT             GetScriptSite           ( REFIID iid, void** result )               { if( _site )  return _site->QueryInterface( iid, result );  
                                                                                                      *result = NULL; return S_OK; }
    virtual HRESULT             SetScriptState          ( SCRIPTSTATE );
    virtual HRESULT             GetScriptState          ( SCRIPTSTATE* result )                     { *result = _script_state; return S_OK; }
    virtual HRESULT             Close                   ();
    virtual HRESULT             AddNamedItem            ( LPCOLESTR, DWORD );
    virtual HRESULT             AddTypeLib              ( REFGUID, DWORD major, DWORD minor, DWORD ){ return E_NOTIMPL; }
    virtual HRESULT             GetScriptDispatch       ( LPCOLESTR, IDispatch** );
    virtual HRESULT             GetCurrentScriptThreadID( SCRIPTTHREADID* )                         { return E_NOTIMPL; }
    virtual HRESULT             GetScriptThreadID       ( DWORD, SCRIPTTHREADID* )                  { return E_NOTIMPL; }
    virtual HRESULT             GetScriptThreadState    ( SCRIPTTHREADID, SCRIPTTHREADSTATE* )      { return E_NOTIMPL; }
    virtual HRESULT             InterruptScriptThread   ( SCRIPTTHREADID, const EXCEPINFO*, DWORD ) { return E_NOTIMPL; }
    virtual HRESULT             Clone                   ( IActiveScript** )                         { return E_NOTIMPL; }


    // IActiveScriptParse
    virtual HRESULT             InitNew                 ();
        
    virtual HRESULT             AddScriptlet            ( LPCOLESTR pstrDefaultName,
                                                          LPCOLESTR pstrCode,
                                                          LPCOLESTR pstrItemName,
                                                          LPCOLESTR pstrSubItemName,
                                                          LPCOLESTR pstrEventName,
                                                          LPCOLESTR pstrDelimiter,
                                                          DWORD     dwSourceContextCookie,
                                                          ULONG     ulStartingLineNumber,
                                                          DWORD     dwFlags,
                                                          BSTR*     pbstrName,
                                                          EXCEPINFO*pexcepinfo )                    { return E_NOTIMPL; }
        
    virtual HRESULT             ParseScriptText         ( LPCOLESTR pstrCode,
                                                          LPCOLESTR pstrItemName,
                                                          IUnknown* punkContext,
                                                          LPCOLESTR pstrDelimiter,
                                                          DWORD     dwSourceContextCookie,
                                                          ULONG     ulStartingLineNumber,
                                                          DWORD     dwFlags,
                                                          VARIANT*  pvarResult,
                                                          EXCEPINFO*pexcepinfo );

  private:
    friend struct               Perl_idispatch;

    Fill_zero                  _zero_;
    ptr<IActiveScriptSite>     _site;
    SCRIPTSTATE                _script_state;
    ptr<Perl>                  _perl;
};

//-----------------------------------------------------------------------------------Perl_idispatch

struct Perl_idispatch : idispatch_base_implementation<IDispatch>
{
                                Perl_idispatch          ( Perl_scripting_engine* );
    virtual                    ~Perl_idispatch          ()                                          {}

    virtual STDMETHODIMP        QueryInterface          ( const IID&, void** );

    virtual HRESULT             GetIDsOfNames           ( REFIID, LPOLESTR*, UINT, LCID, DISPID* );
    virtual HRESULT             Invoke                  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );

    Fill_zero                  _zero_;
    ptr<Perl_scripting_engine> _engine;
    vector<string>             _names;                  // Hier steckt GetIDsOfNames() die Namen rein, um eine dispid zu ermitteln
};

//-------------------------------------------------------------------------------------------static

static creatable_com_class_descriptor< Perl_scripting_engine, IActiveScript >   class_descriptor ( (Typelib_ref*)NULL, "PerlScript" );

//---------------------------------------------------------------------Create_perl_scripting_engine

HRESULT Create_perl_scripting_engine( const CLSID& clsid, const IID& iid, void** result )
{
    if( clsid != CLSID_PerlScript )  return CLASS_E_CLASSNOTAVAILABLE;

    return Com_get_class_object( &class_descriptor, clsid, iid, result );
}

//---------------------------------------------------------------------create_perl_scripting_engine
/*
HRESULT create_perl_scripting_engine( const CLSID& clsid, const IID& iid, IUnknown** result )
{
    if( clsid != CLSID_PerlScript )  return CLASS_E_CLASSNOTAVAILABLE;

    ptr<Perl_scripting_engine> engine = new Perl_scripting_engine;
    return engine->QueryInterface( iid, (void**)result );
}
*/
//-------------------------------------------------------------------Perl_idispatch::Perl_idispatch

Perl_idispatch::Perl_idispatch( Perl_scripting_engine* e )
: 
    _zero_(this+1),
    _engine(e) 
{ 
    _names.push_back( "" );      // dispid 0 ist reserviert (DISPID_VALUE)
}

//-------------------------------------------------------------------Perl_idispatch::QueryInterface

STDMETHODIMP Perl_idispatch::QueryInterface( const IID& iid, void** result )
{ 
    if( iid == IID_IUnknown  )  *result = (IUnknown*)(IDispatch*)this;
    else
    if( iid == IID_IDispatch )  *result = (IDispatch*)this;
    else 
    { 
        *result = NULL; 
        return E_NOINTERFACE; 
    }

    AddRef();

    return S_OK;
}

//--------------------------------------------------------------------Perl_idispatch::GetIDsOfNames

STDMETHODIMP Perl_idispatch::GetIDsOfNames( REFIID, LPOLESTR* names, UINT names_count, LCID, DISPID* dispid )
{ 
    if( names_count != 1 )  return DISP_E_NONAMEDARGS;

    string name = string_from_ole( names[0] );

    int idx;

    for( idx = 0; idx < _names.size(); idx++ )
    {
        if( _names[idx] == name )  { *dispid = idx;  return S_OK; }
    }

    try
    {
        Variant result;

        _engine->_perl->eval( "defined &" + name, &result );

        if( result.vt == VT_EMPTY )  return DISP_E_UNKNOWNNAME;
        if( result.vt == VT_BSTR &&  SysStringLen( result.bstrVal ) == 0 )  return DISP_E_UNKNOWNNAME;

        result.change_type( VT_BOOL );
        if( !V_BOOL( &result ) )  return DISP_E_UNKNOWNNAME;
    }
    catch( const exception& ) { return DISP_E_UNKNOWNNAME; }
    
    _names.push_back( name );

    *dispid = idx;

    return S_OK; 
}

//---------------------------------------------------------------------------Perl_idispatch::Invoke

STDMETHODIMP Perl_idispatch::Invoke( DISPID dispid, REFIID, LCID, WORD, DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* ) 
{ 
    HRESULT hr = S_OK;

    memset( excepinfo, 0, sizeof *excepinfo );

    if( dispparams->cNamedArgs != 0 )  return E_NOTIMPL;

    try
    {
        std::list<Variant> params;

        for( int i = dispparams->cArgs - 1; i >= 0; --i )
        {
            params.push_back( dispparams->rgvarg[ i ] );
        }

        _engine->_perl->call( _names[dispid], params, result );
    }
    catch( const exception&  x ) 
    { 
        if( excepinfo )  
        {
            excepinfo->bstrDescription = bstr_from_string( x.what() );       
            excepinfo->bstrSource      = bstr_from_string( "perl" );
        }

        hr = DISP_E_EXCEPTION; 
    }
    catch( const _com_error& x ) 
    { 
        if( excepinfo )  
        {
            excepinfo->scode           = x.Error();
            excepinfo->bstrDescription = SysAllocString( x.Description() );  
            excepinfo->bstrSource      = bstr_from_string( "perl" );
        }

        hr = DISP_E_EXCEPTION; 
    }

    return hr;
}

//------------------------------------------------------------Perl_scripting_engine::QueryInterface

STDMETHODIMP Perl_scripting_engine::QueryInterface( const IID& iid, void** result )
{ 
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IActiveScript     , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IActiveScriptParse, result );

    return Iunknown_implementation::QueryInterface( iid, result ); 
}

//-------------------------------------------------------------Perl_scripting_engine::SetScriptSite

HRESULT Perl_scripting_engine::SetScriptSite( IActiveScriptSite* site )
{ 
    HRESULT hr = S_OK;

    try
    {
        _site = site; 
        _perl->parse( "$" Z_PERL_IDISPATCH_PACKAGE_NAME "::site=" + as_string((int)(IActiveScriptSite*)_site) + ";" );
    }
    catch( const exception&  x ) { hr = Com_set_error( x, "Perl_scripting_engine::SetScriptSite" ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "Perl_scripting_engine::SetScriptSite" ); }

    return hr;
}

//------------------------------------------------------------Perl_scripting_engine::SetScriptState

HRESULT Perl_scripting_engine::SetScriptState( SCRIPTSTATE state )
{
    HRESULT hr = S_FALSE;

    try
    {
        switch( state )
        {
            case SCRIPTSTATE_UNINITIALIZED:
                if( _perl )  _perl->close(),  hr = S_OK;
                break;

            case SCRIPTSTATE_INITIALIZED:
                if( !_perl )  hr = E_UNEXPECTED;
                break;

            case SCRIPTSTATE_STARTED:
                if( !_perl )  return E_UNEXPECTED;
                _perl->start();
                hr = S_OK;
                break;

            case SCRIPTSTATE_CONNECTED:
                hr = S_FALSE;
                break;

            case SCRIPTSTATE_DISCONNECTED:
                hr = S_FALSE;
                break;

            case SCRIPTSTATE_CLOSED:
                if( _perl )  _perl->close(), hr = S_OK;

            default: ;
                hr = E_UNEXPECTED;
        }

        if( hr == S_OK )  _script_state = state;
    }
    catch( const exception&  x ) { hr = Com_set_error( x ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "Perl_scripting_engine::SetScriptState" ); }

    return hr;
}

//---------------------------------------------------------------------Perl_scripting_engine::Close

HRESULT Perl_scripting_engine::Close()
{
    HRESULT hr = S_OK;

    try
    {
        if( _perl )  _perl->close();
    }
    catch( const exception&  x ) { hr = Com_set_error( x, "Perl_scripting_engine::Close" ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "Perl_scripting_engine::Close" ); }

    return hr;
}

//--------------------------------------------------------------Perl_scripting_engine::AddNamedItem

HRESULT Perl_scripting_engine::AddNamedItem( LPCOLESTR name_w, DWORD flags )
{
    HRESULT hr = NOERROR;
    string name = string_from_ole( name_w );

    try
    {
        if( flags != SCRIPTITEM_ISVISIBLE )  return E_INVALIDARG;

        _perl->parse( "my $" + name + "= new " + _perl->com_class_name() + "; $" + name + "->{__name}='" + name + "';" );
    }
    catch( const exception&  x ) { hr = Com_set_error( x, "Perl_scripting_engine::AddNamedItem" ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "Perl_scripting_engine::AddNamedItem" ); }

    return hr;
}

//-------------------------------------------------------------------Perl_scripting_engine::InitNew

HRESULT Perl_scripting_engine::InitNew()
{
    HRESULT hr = NOERROR;

    try
    {
        ptr<Perl> perl = Z_NEW( Perl );
        perl->init();
        perl->parse( perl_com_pm );
        _perl = perl;
    }
    catch( const exception&  x ) { hr = Com_set_error( x, "Perl_scripting_engine::InitNew" ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "Perl_scripting_engine::InitNew" ); }

    if( FAILED(hr) )  fprintf( stderr, "Perl_scripting_engine::InitNew() hr=%X\n", (int)hr );
    return hr;
}

//-----------------------------------------------------------Perl_scripting_engine::ParseScriptText

HRESULT Perl_scripting_engine::ParseScriptText( LPCOLESTR pstrCode,
                                                LPCOLESTR pstrItemName,
                                                IUnknown* punkContext,
                                                LPCOLESTR pstrDelimiter,
                                                DWORD     dwSourceContextCookie,
                                                ULONG     ulStartingLineNumber,
                                                DWORD     dwFlags,
                                                VARIANT*  pvarResult,
                                                EXCEPINFO*pexcepinfo )
{
    //fprintf( stderr, "Perl_scripting_engine::ParseScriptText\n" );

    HRESULT hr = NOERROR;

    if( !_perl )  return E_UNEXPECTED;

    try
    {
        if( pvarResult )  VariantInit( pvarResult );

        _perl->parse( string_from_ole( pstrCode ), pvarResult );
    }
    catch( const exception&  x ) { hr = Com_set_error( x, "Perl_scripting_engine::ParseScriptText" ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "Perl_scripting_engine::ParseScriptText" ); }

    return hr;
}

//---------------------------------------------------------Perl_scripting_engine::GetScriptDispatch

HRESULT Perl_scripting_engine::GetScriptDispatch( LPCOLESTR name_w, IDispatch** result )
{
    if( name_w  &&  name_w[0] )  return E_FAIL;

    ptr<Perl_idispatch> perl_idispatch = Z_NEW( Perl_idispatch( this ) );
    *result = perl_idispatch.take();
    //*result = new Perl_idispatch( this );
    //(*result)->AddRef();

    //fprintf( stderr, "Perl_scripting_engine::GetScriptDispatch OK\n" );

    return S_OK;
}

//-------------------------------------------------------------------------------------------------
