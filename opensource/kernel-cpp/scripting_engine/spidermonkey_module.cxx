// $Id: spidermonkey_module.cxx 13509 2008-04-22 13:42:23Z jz $                                (C)1996 SOS GmbH Berlin
// Joacim Zschimmer

#include <zschimmer/zschimmer.h>
#include <zschimmer/z_com.h>
#include <zschimmer/z_com_server.h>
#include <zschimmer/com_server.h>
#include <zschimmer/log.h>
#include <zschimmer/spidermonkey_scripting_engine.h>


using namespace zschimmer;
using namespace zschimmer::com;

//-------------------------------------------------------------------------------------------static

namespace spidermonkey 
{
    Typelib_ref                         typelib;
}

//Has_com_context::Class_descriptor       Has_com_context::class_descriptor  ( &spidermonkey::typelib, "Has_com_context" );

using namespace spidermonkey;

//------------------------------------------------------------------------------------------DllMain

#if defined Z_LINK_STATIC
#   define DllMain spidermonkey_DllMain
#endif

extern "C" BOOL WINAPI DllMain( HANDLE hinstance, DWORD ul_reason_being_called, void* param )
{
    BOOL result = TRUE;

    switch( ul_reason_being_called )
    {
        case DLL_PROCESS_ATTACH: 
        {
            try 
            {
                typelib.set_hinstance( (HINSTANCE)hinstance );
                zschimmer_init();
            }
            catch( const Xc& ) { result = FALSE; }
            break;
        }

        case DLL_PROCESS_DETACH: 
        {
            zschimmer_terminate();
            break;
        }

      //case DLL_THREAD_ATTACH : break;
      //case DLL_THREAD_DETACH : break;


#       ifdef Z_UNIX
        case Z_DLL_COM_ATTACH: 
        {
            Apply_com_module_params( (Com_module_params*)param );
            result = TRUE;
            break;
        }
#       endif

        default: break;
    } 

    return result;
}

//----------------------------------------------------------------------------DllGetClassObject

#if defined Z_LINK_STATIC
#   define DllGetClassObject spidermonkey_DllGetClassObject
#endif

extern "C" HRESULT APIENTRY DllGetClassObject( const CLSID& clsid, const IID& iid, void** result )
{
    HRESULT hr;

    if( CLSID_Spidermonkey == CLSID_NULL )
    {
        fprintf( stderr, "*** Die statischen Variablen im Spidermonkey-Modul sind nicht initialisiert! ***\n" );
    }

    #ifdef Z_WINDOWS
        hr = typelib.Get_class_object( clsid, iid, result );
    #else
        //if( clsid == CLSID_Spidermonkey )  return com_get_class_object( perl_class_descriptor, clsid, iid, result );
        //if( clsid == CLSID_Spidermonkey )  return com_get_class_object( create_perl_scripting_engine, clsid, iid, result );

        hr = CLASS_E_CLASSNOTAVAILABLE;
        
        if( clsid == CLSID_Spidermonkey )  hr = Create_spidermonkey_scripting_engine( clsid, iid, result );
    #endif

    //if( FAILED(hr) )  fprintf( stderr, "Spidermonkey DllGetClassObject(%s,%s) => %s\n", string_from_clsid( clsid ).c_str(), string_from_clsid( iid ).c_str(), string_from_hresult( hr ).c_str() );

    return hr;
}

//------------------------------------------------------------------------------DllCanUnloadNow
#ifdef SYSTEM_HAS_COM

extern "C" HRESULT WINAPI DllCanUnloadNow()
{
    return com_can_unload_now()? S_OK : S_FALSE;
}

//----------------------------------------------------------------------------DllRegisterServer

extern "C" HRESULT WINAPI  DllRegisterServer()
{
#   ifdef _DEBUG
        return typelib.Register_server();
#    else
        return E_NOTIMPL;
#   endif
}

//--------------------------------------------------------------------------DllUnregisterServer

extern "C" HRESULT  WINAPI  DllUnregisterServer()
{
#   ifdef _DEBUG
        return typelib.Unregister_server();
#    else
        return E_NOTIMPL;
#   endif
}

#endif

