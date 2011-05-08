//#define MODULE_NAME "odbcdll"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


/*
** DLL.C - This is the ODBC sample driver code for
** LIBMAIN processing.
**
**	This code is furnished on an as-is basis as part of the ODBC SDK and is
**	intended for example purposes only.
**
*/

//	-	-	-	-	-	-	-	-	-

#ifndef STRICT
#	define STRICT
#endif

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif


namespace sos {


HINSTANCE _hinstance;    // Saved module handle.

#ifdef _WIN32

#if defined SYSTEM_MFC 
//#if defined _AFXDLL || defined _USRDLL  // js 8.6.99 AfxGetInstanceHandle eingebaut //jz 6.10.98  _hinstance wird hier nicht gesetzt!?!?   //#if defined _AFXDLL || defined _USRDLL


struct Sosodbc_mfcapp : CWinApp
{
                               ~Sosodbc_mfcapp          ();
	virtual BOOL                InitInstance            ();

  //DECLARE_MESSAGE_MAP()
};


Sosodbc_mfcapp sosodbc_mfcapp;


BOOL Sosodbc_mfcapp::InitInstance()
{
    //? AfxEnableControlContainer();
    // js 8.6.99: _hinstance = m_hInstance;
    _hinstance = AfxGetInstanceHandle(); // js 8.6.99
    return TRUE;  //oder FALSE?
}


Sosodbc_mfcapp::~Sosodbc_mfcapp()
{
    //sosodbc_appl.exit();
}


#else

extern "C"      
BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, LPVOID )
{
//DebugBreak(); 
    switch (ul_reason_being_called)
    {
        case DLL_PROCESS_ATTACH:	// case of libentry call in win 3.x
            _hinstance = (HINSTANCE)hInst;
            break;
        case DLL_THREAD_ATTACH:
            break;
    	case DLL_PROCESS_DETACH:	// case of wep call in win 3.x
            break;
    	case DLL_THREAD_DETACH:
            break;
        default:
            break;
	} /* switch */

    return TRUE;
}
#endif

#else
extern "C"
int FAR PASCAL __export LibMain (HINSTANCE hInstance, WORD wDataSeg, WORD cbHeapSize, LPSTR lpCmdLine)
{
    OutputDebugString( "hostODBC LibMain()\n" );

    _hinstance = (HINSTANCE)hInstance;
    return TRUE;
}

#endif

/*
void EXPFUNC FAR PASCAL LoadByOrdinal(void);
//	Entry point to cause DM to load using ordinals
void EXPFUNC FAR PASCAL LoadByOrdinal(void)
{
}
*/

} //namespace sos
