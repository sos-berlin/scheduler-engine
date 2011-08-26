// $Id: hostoled.cxx 13665 2008-09-22 12:21:50Z jz $                                (C)1996 SOS GmbH Berlin
// Joacim Zschimmer

/*
    hostOLE als DLL (InProc-Server)
*/

#include "precomp.h"


#ifdef SYSTEM_MICROSOFT
#   define _CRTDBG_MAPALLOC
#   include <crtdbg.h>
#endif

#ifdef SYSTEM_WIN
#   include <olectl.h>          // DllRegisterServer
#endif

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/com.h"
#include "../kram/com_simple_standards.h"
#include "../kram/oleserv.h"
#include "../kram/olereg.h"
#include "../kram/licence.h"

#include "dhostole.h"
#include "hostole.hrc"
#include "hostole2.h"


namespace sos {

//-------------------------------------------------------------------------------------------static

static bool         licence_checked = false;

//-------------------------------------------------------------------------------------------------

Ole_appl            hostole_appl ( &hostole_typelib );
HINSTANCE          _hinstance;

#ifndef Z_HPUX_PARISC
extern const bool  _dll        = true;
#endif

Sos_string          module_filename();
Sos_string          directory_of_path( const Sos_string& );

//int                _argc = 0;
//char**             _argv = NULL; 


//---------------------------------------------------------------------------------DEBUG ReportHook
#if defined _DEBUG && defined SYSTEM_WIN

int DebugHook( int nRptType, char* szOutMessage, int* retval )
{
    DWORD written;
    WriteFile( GetStdHandle(STD_ERROR_HANDLE), szOutMessage, strlen( szOutMessage ), &written, NULL );

    *retval = FALSE;
    return FALSE;
}

#endif
//------------------------------------------------------------------------------------------DllMain

#ifdef Z_COM //COM_STATIC
#   define DllMain hostole_DllMain
#endif

extern "C" BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )
{
    BOOL result = TRUE;

    switch( ul_reason_being_called )
    {
        case DLL_PROCESS_ATTACH: 
        {
            try 
            {
#               if defined SYSTEM_MICROSOFT && defined _DEBUG
                    // Speicherprüfung bei _DEBUG
                    _CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF ); 
                    _CrtSetReportHook( DebugHook );
#               endif

                _hinstance = (HINSTANCE)hInst; 

                hostole_appl.init();
            }
            catch( const exception& ) { result = FALSE; }
            break;
        }

    	case DLL_PROCESS_DETACH: 
        {
            //2008-09-22  Folgendes auskommentiert, weil unter hostole.dll am Ende des Prozesses DLLs bereits entladen sein können,
            //2008-09-22  Destruktoren können diese DLLs aufrufen und abstürzen, weil sie ins Leere greifen.
            //
            //LOG( "hostole.DllMain(DLL_PROCESS_DETACH): com_object_count=" << sos::com_object_count << ", com_lock=" << sos::com_lock << '\n' );

            //zschimmer::unloading_module = true;
            //hostole_appl.exit();
            //break;
        }

      //case DLL_THREAD_ATTACH : break;
      //case DLL_THREAD_DETACH : break;
/*
        case Z_DLL_COM_ATTACH: 
        {
            Com_module_params* params = (Com_module_params*)param;
            Log_ptr::set( params->_log_stream, params->_log_mutex );
            set_com_context( params->_com_context );
            break;
        }
*/
        default: break;
    } 

    return result;
}

//----------------------------------------------------------------------------DllGetClassObject

#ifdef Z_COM //COM_STATIC
#   define DllGetClassObject hostole_DllGetClassObject
#endif

extern "C" HRESULT APIENTRY DllGetClassObject( const CLSID& rclsid, const IID& riid, void** ppv )
{
    HRESULT hr = E_FAIL;

    Z_MUTEX( hostware_mutex )
    {
        Ole_factory* factory = NULL;

        try {
            hostole_appl.init();
            LOG( "DllGetClassObject(" << rclsid << ", " << riid << ")\n" );

            try 
            {
                Z_MUTEX( hostware_mutex )
                {
                    if( !licence_checked ) {
                        sos_static_ptr()->_licence->check();    // Lizenzschlüssel werden erst hier geprüft, s. DllMain()
                        licence_checked = true;
                    }
                }

                if( !SOS_LICENCE( licence_hostole ) )  throw_xc( "SOS-1000", "hostOLE" );
            }
            catch( const Xc& )  { hr = CLASS_E_NOTLICENSED; goto FEHLER;}

            if( riid != IID_IUnknown  &&   riid != IID_IClassFactory )  { hr = (HRESULT)E_NOINTERFACE; goto FEHLER; }

            factory = new Ole_factory;

            hr = factory->QueryInterface( riid, ppv );
            if( FAILED( hr ) )  goto FEHLER;

            hr = factory->set_clsid( rclsid );
            if( FAILED( hr ) )  goto FEHLER;
        }
        catch( const Ole_error& x )
        {
            hr = x._hresult;
        }
        catch( const exception& ) 
        {
            //if( strcmp( x.code(), "SOS-1000" ) ==  0 )  hr = CLASS_E_NOTLICENSED;
            //                                      else  
            hr = E_FAIL;
        }

      FEHLER:
        if( FAILED( hr ) ) 
        {
            if( factory )  delete factory;
            if( hr == E_NOINTERFACE )  hr = (HRESULT)CLASS_E_CLASSNOTAVAILABLE;     // Laut Doku, 18.11.97
        }
    }

    return hr;
}

//------------------------------------------------------------------------------DllCanUnloadNow
#ifdef SYSTEM_HAS_COM

extern "C" int WINAPI DllCanUnloadNow()
{
    LOG( "hostole.DllCanUnloadNow: com_object_count=" << sos::com_object_count << ", com_lock=" << sos::com_lock << '\n' );

    try 
    {
        if( sos::com_object_count == 0  &&  sos::com_lock == 0 ) 
        {
            hostole_appl.exit();
            return S_OK;
        }
        else
        {
            sos_alloc_list( log_ptr );   // Zeigt alle angeforderten Datenbereiche, nur bei [debug] check-new=yes
            return S_FALSE;
        }
    } 
    catch(...) 
    {
        return S_FALSE;  // Nicht entladen
    }
}

//----------------------------------------------------------------------------DllRegisterServer

extern "C" int WINAPI DllRegisterServer()
{
    return hostole_typelib.register_server();
}

#endif
//--------------------------------------------------------------------------DllUnregisterServer
#ifdef SYSTEM_HAS_COM

extern "C" int WINAPI DllUnregisterServer()
{
    return hostole_typelib.unregister_server();
}

#endif

} //namespace sos
