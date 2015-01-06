// $Id: loaded_module.cxx 13486 2008-04-11 11:14:25Z jz $

#include "zschimmer.h"
#include "loaded_module.h"
#include "log.h"

#ifdef Z_UNIX
#    include <dlfcn.h>
#endif



namespace zschimmer {

//--------------------------------------------------------------------Loaded_module::~Loaded_module

Loaded_module::~Loaded_module()
{
    if( !_dont_unload )
    {
#       ifdef Z_WINDOWS

            if( _handle )
            {
                Z_LOG( "FreeLibrary " << _filename << '\n' );

                FreeLibrary( _handle );
                _handle = NULL;
            }

#       else

            if( _handle )
            {
                Z_LOG( "dlclose " << _filename << '\n' );

                dlclose( _handle );
                _handle = NULL;
            }

#       endif
    }
}

//----------------------------------------------------------------------Loaded_module::set_filename
    
void Loaded_module::set_filename( const string& filename )
{
    _filename = filename;

    if( _filename.find( '.'  ) == string::npos 
     && _filename.find( '/'  ) == string::npos 
     && _filename.find( '\\' ) == string::npos )
    {
#       ifdef Z_WINDOWS
            _filename += ".dll";
#       elif defined Z_HPUX_PARISC
            _filename = "lib" + _filename + ".sl";
#       else
            _filename = "lib" + _filename + ".so";
#       endif
    }
}

//------------------------------------------------------------------------------Loaded_module::load

void Loaded_module::load()
{
#   ifdef Z_WINDOWS

        Z_LOG( "LoadLibrary " << _filename << '\n' );

        _handle = LoadLibrary( _filename.c_str() );
        if( !_handle )  throw_mswin( "LoadLibrary", _filename.c_str(), _title.c_str() );


        char path[ _MAX_PATH+1 ];

        int len = GetModuleFileName( _handle, (LPSTR)path, sizeof path );
        if( len > 0 )  _filename.assign( path, len );
        
        Z_LOG( "HINSTANCE=" << (void*)_handle << "  " << _filename << '\n' );

#    else

        #if defined Z_HPUX
            Z_LOG( "dlopen " << _filename << ",RTLD_LAZY|RTLD_GLOBAL\n" );
            _handle = dlopen( _filename.c_str(), RTLD_LAZY | RTLD_GLOBAL );
        #else
            Z_LOG( "dlopen " << _filename << ",RTLD_LAZY\n" );
            _handle = dlopen( _filename.c_str(), RTLD_LAZY );
        #endif

        if( !_handle)  throw_xc( "Z-LINK", dlerror(), _filename.c_str() );

#   endif
}

//---------------------------------------------------------------------------Loaded_module::addr_of

void* Loaded_module::addr_of( const char* entry_name )
{
#   ifdef Z_WINDOWS

        Z_LOG( "GetProcAddr " << _filename << ',' << entry_name << '\n' );

        void* entry = GetProcAddress( _handle, entry_name );
        if( !entry)  throw_mswin( "GetProcAddress", entry_name, _filename.c_str() );

        return entry;

#    else

        Z_LOG( "dlsym " << _filename << ',' << entry_name << '\n' );

        void* entry = dlsym( _handle, entry_name );
        if( !entry )  throw_xc( "Z-LINK", dlerror(), entry_name, _filename.c_str() );

        return entry;

#   endif
}

} //namespace zschimmer
