// $Id$

#ifndef __PERL_SCRIPTING_ENGINE_H
#define __PERL_SCRIPTING_ENGINE_H

#include "scripting_engine.h"

extern const CLSID CLSID_PerlScript;

//#ifdef COM_STATIC
    extern "C" BOOL WINAPI      sosperl_DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )            COM_LINKAGE;
    extern "C" HRESULT APIENTRY sosperl_DllGetClassObject( const CLSID& rclsid, const IID& riid, void** ppv )   COM_LINKAGE;
//#endif


HRESULT Create_perl_scripting_engine( const CLSID&, const IID&, void** );


#endif
