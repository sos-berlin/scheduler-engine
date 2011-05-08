// $Id: spidermonkey.h,v 1.2 2004/12/05 16:14:26 jz Exp $

#ifndef __SPIDERMONKEY_H
#define __SPIDERMONKEY_H

extern const CLSID CLSID_Spidermonkey;

//#ifdef COM_STATIC
//    extern "C" BOOL WINAPI      sosperl_DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )            COM_LINKAGE;
//    extern "C" HRESULT APIENTRY sosperl_DllGetClassObject( const CLSID& rclsid, const IID& riid, void** ppv )   COM_LINKAGE;
//#endif


HRESULT Create_spidermonkey_scripting_engine( const CLSID&, const IID&, void** );


#endif
