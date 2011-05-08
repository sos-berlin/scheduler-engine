// dllstd.cxx
// Standard-Modul für eine DLL

#include "precomp.h"
#include "../kram/sos.h"

#if defined SYSTEM_WIN

namespace sos {

Bool      _dll       = true;
HINSTANCE _hinstance;

//------------------------------------------------------------------------------------Dll_start
#if defined __BORLANDC__

    extern HINSTANCE _hInstance;       // von Borland definiert

    struct Dll_start
    {
        Dll_start()
        {
            _hinstance = _hInstance;     // von Borland definiert
        }
    };

    static Dll_start dll_start;

#endif
//--------------------------------------------------------------------------------------DllMain
#if defined SYSTEM_MFC  && ( defined _AFXDLL || defined _USRDLL )

#include <afxwin.h>         // MFC core and standard components

struct Hostole_mfcapp : CWinApp
{
                               ~Hostole_mfcapp          ();
	virtual BOOL                InitInstance            ();

  //DECLARE_MESSAGE_MAP()
};


Hostole_mfcapp hostole_mfcapp;


BOOL Hostole_mfcapp::InitInstance()
{
    //? AfxEnableControlContainer();
    _hinstance = m_hInstance;
    return TRUE;  //oder FALSE?
}


Hostole_mfcapp::~Hostole_mfcapp()
{
}


#else

#include <windows.h>

extern "C"
BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, LPVOID )
{
    switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH: _hinstance = (::HINSTANCE)hInst; break;
		case DLL_THREAD_ATTACH : break;
    	case DLL_PROCESS_DETACH: break;
    	case DLL_THREAD_DETACH : break;
                        default: break;
	}

    return TRUE;
}

#endif
//--------------------------------------------------------------------------------------LibMain
#ifdef SYSTEM_WIN16

extern "C"
int FAR PASCAL LibMain( HINSTANCE hInstance, WORD, WORD wHeapSize, LPSTR )
{
	if( wHeapSize >= 0 )  UnlockData( 0 );

	_hinstance = hInstance;

	return 1 ;
}

#endif
//------------------------------------------------------------------------------------------WEP
#if 0 //def SYSTEM_WIN16

int CALLBACK WEP( int /*exit_type*/ )
{
    OutputDebugString( "hostapi.dll beendet mit WEP()\n" );
    if( hostapi_count ) {
        char buffer [ 100 ];
        sprintf( buffer, "*** %d MAL sos_init() OHNE sos_exit() GERUFEN ***\n", (int)hostapi_count );
        OutputDebugString( buffer );
    }
    return 1;
}

#endif

} //namespace sos

#endif

