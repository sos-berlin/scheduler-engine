// $Id$

#ifndef __ZSCHIMMER_SCRIPTING_ENGINE_H
#define __ZSCHIMMER_SCRIPTING_ENGINE_H

#include "../zschimmer/com.h"



#ifdef Z_WINDOWS

#   include <objbase.h>                     // COM
//#   include "../factory/msscript.h"         // GUID für Scripting Engine
#    include <activscp.h>


#else


//extern const CLSID CLSID_Perl;
//DEFINE_GUID( CLSID_Perl, 0x60a0062b, 0x8877, 0x4a83, 0x86, 0x11, 0xd, 0x14, 0x28, 0xe0, 0xd9, 0xda );

#define SCRIPTITEM_ISVISIBLE            0x00000002
#define SCRIPTITEM_ISSOURCE             0x00000004
#define SCRIPTITEM_GLOBALMEMBERS        0x00000008
#define SCRIPTITEM_ISPERSISTENT         0x00000040
#define SCRIPTITEM_CODEONLY             0x00000200
#define SCRIPTITEM_NOCODE               0x00000400

#define SCRIPTTEXT_DELAYEXECUTION       0x00000001
#define SCRIPTTEXT_ISVISIBLE            0x00000002
#define SCRIPTTEXT_ISEXPRESSION         0x00000020
#define SCRIPTTEXT_ISPERSISTENT         0x00000040
#define SCRIPTTEXT_HOSTMANAGESSOURCE    0x00000080      // ?

#define SCRIPTINFO_IUNKNOWN             0x00000001
#define SCRIPTINFO_ITYPEINFO            0x00000002

enum SCRIPTSTATE
{    
    SCRIPTSTATE_UNINITIALIZED           = 0,
    SCRIPTSTATE_INITIALIZED             = 5,
    SCRIPTSTATE_STARTED                 = 1,
    SCRIPTSTATE_CONNECTED               = 2,
    SCRIPTSTATE_DISCONNECTED            = 3,
    SCRIPTSTATE_CLOSED                  = 4
};

enum SCRIPTTHREADSTATE
{    
    SCRIPTTHREADSTATE_NOTINSCRIPT       = 0,
    SCRIPTTHREADSTATE_RUNNING           = 1
};

typedef DWORD SCRIPTTHREADID;

//-------------------------------------------------------------------------------IActiveScriptError

extern const IID IID_IActiveScriptError;
//DEFINE_GUID(IID_IActiveScriptError, 0xeae1ba61, 0xa4ed, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

struct IActiveScriptError : IUnknown
{
    friend inline const GUID& __uuidof_                 ( IActiveScriptError* )                     { return IID_IActiveScriptError; }

    virtual STDMETHODIMP            QueryInterface      ( const IID&, void** ) = 0;
    virtual STDMETHODIMP_( ULONG )  AddRef              () = 0;
    virtual STDMETHODIMP_( ULONG )  Release             () = 0;

    virtual HRESULT             GetExceptionInfo        ( EXCEPINFO* )                              = 0;
    virtual HRESULT             GetSourcePosition       ( DWORD* source_context, ULONG* line, LONG* col ) = 0;
    virtual HRESULT             GetSourceLineText       ( BSTR* )                                   = 0;
};

//--------------------------------------------------------------------------------IActiveScriptSite

extern const IID IID_IActiveScriptSite;
//DEFINE_GUID(IID_IActiveScriptSite, 0xdb01a1e3, 0xa42b, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

struct IActiveScriptSite : IUnknown
{
    friend inline const GUID& __uuidof_                 ( IActiveScriptSite* )                      { return IID_IActiveScriptSite; }

  //virtual STDMETHODIMP            QueryInterface      ( const IID&, void** ) = 0;
  //virtual STDMETHODIMP_( ULONG )  AddRef              () = 0;
  //virtual STDMETHODIMP_( ULONG )  Release             () = 0;

    virtual HRESULT             GetLCID                 ( LCID* plcid )                             = 0;

    virtual HRESULT             GetItemInfo             ( LPCOLESTR, DWORD, IUnknown**, ITypeInfo** ) = 0;
    virtual HRESULT             GetDocVersionString     ( BSTR* )                                   = 0;
    virtual HRESULT             OnScriptTerminate       ( const VARIANT* result, const EXCEPINFO* ) = 0;
    virtual HRESULT             OnStateChange           ( SCRIPTSTATE )                             = 0;
    virtual HRESULT             OnScriptError           ( IActiveScriptError* )                     = 0;
    virtual HRESULT             OnEnterScript           ()                                          = 0;
    virtual HRESULT             OnLeaveScript           ()                                          = 0;
};

//------------------------------------------------------------------------------------IActiveScript

extern const IID IID_IActiveScript;
//DEFINE_GUID(IID_IActiveScript, 0xbb1a2ae1, 0xa4f9, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

struct IActiveScript : IUnknown
{
    friend inline const GUID& __uuidof_                 ( IActiveScript* )                          { return IID_IActiveScript; }

  //virtual STDMETHODIMP            QueryInterface      ( const IID&, void** ) = 0;
  //virtual STDMETHODIMP_( ULONG )  AddRef              () = 0;
  //virtual STDMETHODIMP_( ULONG )  Release             () = 0;

    virtual HRESULT             SetScriptSite           ( IActiveScriptSite* )                      = 0;
    virtual HRESULT             GetScriptSite           ( REFIID, void** )                          = 0;
    virtual HRESULT             SetScriptState          ( SCRIPTSTATE )                             = 0;
    virtual HRESULT             GetScriptState          ( SCRIPTSTATE* )                            = 0;
    virtual HRESULT             Close                   ()                                          = 0;
    virtual HRESULT             AddNamedItem            ( LPCOLESTR, DWORD )                        = 0;
    virtual HRESULT             AddTypeLib              ( REFGUID, DWORD major, DWORD minor, DWORD )= 0;
    virtual HRESULT             GetScriptDispatch       ( LPCOLESTR, IDispatch** )                  = 0;
    virtual HRESULT             GetCurrentScriptThreadID( SCRIPTTHREADID* )                         = 0;
    virtual HRESULT             GetScriptThreadID       ( DWORD, SCRIPTTHREADID* )                  = 0;
    virtual HRESULT             GetScriptThreadState    ( SCRIPTTHREADID, SCRIPTTHREADSTATE* )      = 0;
    virtual HRESULT             InterruptScriptThread   ( SCRIPTTHREADID, const EXCEPINFO*, DWORD ) = 0;
    virtual HRESULT             Clone                   ( IActiveScript** )                         = 0;
};

//-------------------------------------------------------------------------------IActiveScriptParse

extern const IID IID_IActiveScriptParse;
//DEFINE_GUID(IID_IActiveScriptParse, 0xbb1a2ae2, 0xa4f9, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

struct IActiveScriptParse : IUnknown
{
    friend inline const GUID& __uuidof_                 ( IActiveScriptParse* )                     { return IID_IActiveScriptParse; }

  //virtual STDMETHODIMP            QueryInterface      ( const IID&, void** ) = 0;
  //virtual STDMETHODIMP_( ULONG )  AddRef              () = 0;
  //virtual STDMETHODIMP_( ULONG )  Release             () = 0;

    virtual HRESULT             InitNew                 ()                                          = 0;
        
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
                                                          EXCEPINFO*pexcepinfo )                    = 0;
        
    virtual HRESULT             ParseScriptText         ( LPCOLESTR pstrCode,
                                                          LPCOLESTR pstrItemName,
                                                          IUnknown* punkContext,
                                                          LPCOLESTR pstrDelimiter,
                                                          DWORD     dwSourceContextCookie,
                                                          ULONG     ulStartingLineNumber,
                                                          DWORD     dwFlags,
                                                          VARIANT*  pvarResult,
                                                          EXCEPINFO*pexcepinfo )                    = 0;
};

//-------------------------------------------------------------------------------------------------

#endif
#endif
