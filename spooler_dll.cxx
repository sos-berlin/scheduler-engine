// $Id: spooler_dll.cxx,v 1.1 2003/09/04 15:56:11 jz Exp $

#include "spooler.h"


namespace sos
{
    extern HINSTANCE       _hinstance;
    Sos_appl                application ( false );
}


//------------------------------------------------------------------------------------------DllMain

extern "C" BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, LPVOID )
{
    switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH: 
            sos::_hinstance = (::HINSTANCE)hInst; 
            sos::application.init();
            break;

		case DLL_THREAD_ATTACH:     
            break;

    	case DLL_PROCESS_DETACH: 
            sos::application.exit();
            break;

    	case DLL_THREAD_DETACH: 
            break;

        default: 
            break;
	}

    return TRUE;
}

//--------------------------------------------------------------------------------------------start

extern "C" void __declspec(dllexport) WINAPI/*CALLBACK*/ spooler( HWND window, HINSTANCE, LPTSTR command_line, int show )
{
    sos::Sos_appl my_application;

    sos::spooler_main( NULL, NULL, command_line );
}

//-------------------------------------------------------------------------------------------------

extern "C" int __declspec(dllexport) spooler_program( int argc, char** argv )
{
    return sos::spooler_main( argc, argv, "" );
}

