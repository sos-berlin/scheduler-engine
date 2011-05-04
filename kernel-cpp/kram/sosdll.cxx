#include "precomp.h"
//#define MODULE_NAME "sosdll"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sysdep.h"
#include <string.h>

#ifdef SYSTEM_WIN
#   include <windows.h>
#else
#   include <dlfcn.h>
#endif

#include <limits.h>

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosdll.h"

#include "../kram/sosmswin.h"

using namespace std;
namespace sos {


//--------------------------------------------------------------------------------_dll_proc_ptr

void* _dll_proc_ptr( Library_handle library_handle, const char* name )
{

#   ifdef SYSTEM_WIN

        FARPROC ptr = GetProcAddress( library_handle, name );
        if( !ptr ) {
            char dll_name [ PATH_MAX + 1 ];
            int len = GetModuleFileName( library_handle, dll_name, sizeof dll_name );
            if( !len )  strcpy( dll_name, "(unbekannte DLL)"  );
            throw_xc( "MSWIN-GetProcAddress", Msg_insertions( name, dll_name ) );
        }
        return ptr;

#    else

        void* ptr = dlsym( library_handle, name );
        if( !ptr )  throw_xc( "DLSYM", dlerror(), name );
        return ptr;


#   endif
}

//-------------------------------------------------------------------Auto_loading_dll_proc::ptr

void* Auto_loading_dll_proc::ptr( const Sos_dll* dll_ptr, const char* name_ptr )
{
    if( !_ptr ) {
        _ptr = dll_ptr->proc_ptr( name_ptr );
    }
    return (void*)_ptr;
}

//------------------------------------------------------------------------Sos_dll::~Sos_dll

Sos_dll::~Sos_dll()
{
    if( _library_handle ) 
    {
        Library_handle library_handle = _library_handle;
        _library_handle = 0;

#       ifdef SYSTEM_WIN

            LOG( "FreeLibrary( " << library_handle << ")\n" );

            BOOL ok = FreeLibrary( library_handle );
            if( ok ) {
                LOG( "FreeLibrary ok\n" );;
            } else {
                try { 
                    throw_mswin_error( "FreeLibrary" );   // Fürs Log
                }
                catch( const Xc& ) {}
            }

#        else

            LOG( "dlclose(" << library_handle << ")\n" );
            dlclose( library_handle );

#       endif
    }
}

//--------------------------------------------------------------------------------Sos_dll::init

void Sos_dll::init( const char* name )
{
    if( _library_handle )  return;

#   ifdef SYSTEM_WIN

        LOG( "LoadLibrary(\"" << name << "\")\n" );
        HINSTANCE hinstance = LoadLibrary( name );

        if( (uint)hinstance <= 32 )  throw_mswin_error( "LoadLibrary", name );

        _library_handle = hinstance;

#    else

        LOG( "dlopen(\"" << name << "\",RTLD_LAZY|RTLD_GLOBAL)\n" );
        _library_handle = dlopen( name, RTLD_LAZY | RTLD_GLOBAL );

        if( !_library_handle )  throw_xc( "DLOPEN", dlerror(), name );

#   endif

    LOG( _library_handle << " geladen\n" );
}

//----------------------------------------------------------------------------Sos_dll::proc_ptr

Sosdll_proc_ptr Sos_dll::proc_ptr( const char* name ) const
{
    return dll_proc_ptr( _library_handle, name );
}

//---------------------------------------------------------------Auto_loading_dll_proc::get_ptr
/*
void Auto_loading_dll_proc::get_ptr( const char* name )
{
    _ptr = dll_ptr->proc_ptr( name_ptr );
}
*/
} //namespace sos
    
