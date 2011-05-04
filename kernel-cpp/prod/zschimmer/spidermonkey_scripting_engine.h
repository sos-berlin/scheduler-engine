// $Id$

#ifndef __SPIDERMONKEY_SCRIPTING_ENGINE_H
#define __SPIDERMONKEY_SCRIPTING_ENGINE_H

#include "scripting_engine.h"

extern const CLSID CLSID_Spidermonkey;

//#ifdef COM_STATIC
    extern "C" BOOL WINAPI      spidermonkey_DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )            COM_LINKAGE;
    extern "C" HRESULT APIENTRY spidermonkey_DllGetClassObject( const CLSID& rclsid, const IID& riid, void** ppv )   COM_LINKAGE;
//#endif


HRESULT Create_spidermonkey_scripting_engine( const CLSID&, const IID&, void** );


#endif
