// $Id: sosscrpt.h 11570 2005-07-01 17:41:48Z jz $

#ifndef __SOSSCRPT_H
#define __SOSSCRPT_H

#include "../zschimmer/scripting_engine.h"

#include "sosalloc.h"
#include "sosarray.h"
//#include "olestd.h"
#include "com_simple_standards.h"
#include <map>

#include "../zschimmer/z_com.h"
using zschimmer::ptr;
using zschimmer::com::Bstr;

namespace sos
{

enum Scripttext_flags
{
    scripttext_isvisible = 2,
    scripttext_isexpression = 32,
    scripttext_ispersistent = 64
};

//----------------------------------------------------------------------------------Script_site

struct Script_site : IActiveScriptSite
{                                      
    typedef stdext::hash_map< Bstr, ptr<IDispatch> >  Obj_map;


    void*                       operator new            ( size_t size )                                 { return sos_alloc( size, "Script_site" ); }
    void                        operator delete         ( void* ptr )                                   { sos_free( ptr ); }

    void*                       operator new            ( size_t, void* ptr )                           { return ptr; }
    void                        operator delete         ( void*, void* )                                {}

                                Script_site             ();
    virtual                    ~Script_site             ();

    static void                 clear                   ();

    // IUnknown:
    STDMETHODIMP_( ULONG )      AddRef                  ()                                              { return InterlockedIncrement( &_ref_count ); }
    STDMETHODIMP_( ULONG )      Release                 ()                                              { long n = InterlockedDecrement( &_ref_count );  if( n == 0 )  delete this; return n; }
    STDMETHODIMP_( HRESULT )    QueryInterface          ( REFIID, void** );

    // IActiveScriptSite:
    STDMETHODIMP_( HRESULT )    GetDocVersionString     ( BSTR *pbstrVersion ) 	                        { *pbstrVersion = SysAllocString( L"Software- und Organisations-Service GmbH, Berlin" ); return S_OK; }
    STDMETHODIMP_( HRESULT )    GetItemInfo             ( LPCOLESTR, DWORD, IUnknown**, ITypeInfo** );
    STDMETHODIMP_( HRESULT )    OnScriptError           ( IActiveScriptError* );
    STDMETHODIMP_( HRESULT )    GetLCID                 ( LCID *plcid )	                                { *plcid = 9; return S_OK; }
    STDMETHODIMP_( HRESULT )    OnScriptTerminate       ( const VARIANT *, const EXCEPINFO * )          { return S_OK; }
    STDMETHODIMP_( HRESULT )    OnStateChange           ( SCRIPTSTATE )                                 { return S_OK; }
    STDMETHODIMP_( HRESULT )    OnEnterScript           ()                                              { return S_OK; }
    STDMETHODIMP_( HRESULT )    OnLeaveScript           ()                                              { return S_OK; }

    void                        init                    ();
    void                        init_engine             ();
    void                        close_engine            ();
    void                        reset_engine            ();
    void                        add_obj                 ( IDispatch*, const OLECHAR* name, uint4 flags );
    void                        add_obj                 ( IDispatch* d, const OLECHAR* name );
    void                        set_engine              ( const Sos_string& name )                      { _engine_name = name; }
    Variant                     parse                   ( const string& script_text, Scripttext_flags = scripttext_isvisible, int linenr_offset = 0 );
    Variant                     parse                   ( const string& script_text, int linenr_offset )   { return parse( script_text, scripttext_isvisible, linenr_offset ); }
    IDispatch*                  dispatch                () const                                        { return _dispatch; }
    void                        interrupt               ();

    Variant                     call                    ( const string& name );
    Variant                     call                    ( const string& name, const Variant& par );
    Variant                     call                    ( const string& name, Variant* par );
    Variant                     property_get            ( const string& name );
    void                        property_put            ( const string& name, const Variant& );
    void                        property_putref         ( const string& name, const Variant& );
    bool                        name_exists             ( const string& name );

    bool                        init_engine_called      () const                                        { return _init_engine_called; }

    Fill_zero                  _zero_;
    zschimmer::long32          _ref_count;
    Sos_string                 _engine_name;            // "VBScript", "JavaScript", "PerlScript", ...
    Sos_string                 _source;                 // Wird dem Quellennamen in der Fehlermeldung vorangestellt.
    int                        _first_lineno;           // Wird auf die Zeilennummer der Fehlermeldung der Scripting Engine addiert
    IActiveScriptParse*        _script_parse;
    IActiveScript*             _script;
    ptr<IDispatch>             _dispatch;
    Xc*                        _script_exception;
    Obj_map                    _obj_map;
    bool                       _init_engine_called;
    std::map<string,bool>      _names;                  // Optimierung für name_exists()
};


string                          detect_script_language  ( const string& script_text );

} //namespace sos

#endif

