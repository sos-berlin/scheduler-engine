// $Id$                                (C)1996 SOS GmbH Berlin
// Joacim Zschimmer

#include "zschimmer.h"
#include "z_com.h"
#include "com_server.h"
#include "log.h"
#include "perl_scripting_engine.h"

using namespace zschimmer;
using namespace zschimmer::com;


//------------------------------------------------------------------------------------------DllMain

#ifdef Z_LINK_STATIC
#   define DllMain sosperl_DllMain
#endif

extern "C" BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* param )
{
    BOOL result = TRUE;

    switch( ul_reason_being_called )
    {
        case DLL_PROCESS_ATTACH: 
        {
            try 
            {
                zschimmer_init();
            }
            catch( const Xc& x ) 
            { 
                fprintf( stderr, "%s DLL_PROCESS_ATTACH  ERROR  %s\n", Z_FUNCTION.c_str(), x.what() );
                result = FALSE; 
            }
            break;
        }

        case DLL_PROCESS_DETACH: 
        {
            zschimmer_terminate();
            break;
        }

      //case DLL_THREAD_ATTACH : break;
      //case DLL_THREAD_DETACH : break;

        case Z_DLL_COM_ATTACH: 
        {
            Apply_com_module_params( (Com_module_params*)param );
            result = TRUE;
            break;
        }

        default: break;
    } 

    return result;
}

//----------------------------------------------------------------------------DllGetClassObject

#ifdef Z_LINK_STATIC
#   define DllGetClassObject sosperl_DllGetClassObject
#endif

extern "C" HRESULT APIENTRY DllGetClassObject( const CLSID& clsid, const IID& iid, void** result )
{
    if( CLSID_PerlScript == CLSID_NULL )    {        fprintf( stderr, "*** Die statischen Variablen im PerlScript-Modul sind nicht initialisiert! ***\n" );    }

    //if( clsid == CLSID_PerlScript )  return com_get_class_object( perl_class_descriptor, clsid, iid, result );
    //if( clsid == CLSID_PerlScript )  return com_get_class_object( create_perl_scripting_engine, clsid, iid, result );

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    
    if( clsid == CLSID_PerlScript )  hr = Create_perl_scripting_engine( clsid, iid, result );

    if( FAILED(hr) )  fprintf( stderr, "Perl DllGetClassObject(%s) => %x\n", string_from_clsid( clsid ).c_str(), hr );
    return hr;
}

//------------------------------------------------------------------------------DllCanUnloadNow
/*

extern "C" int WINAPI DllCanUnloadNow()
{
    LOG( "DllCanUnloadNow: com_object_count=" << sos::com_object_count << ", com_lock=" << sos::com_lock << '\n' );

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
        // Trotzdem entladen ...
        return S_OK;
    }
}

*/
