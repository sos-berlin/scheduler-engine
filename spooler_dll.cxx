// $Id$

#include "spooler.h"


namespace sos
{
    extern HINSTANCE       _hinstance;
    Sos_appl                application ( false );
}


//------------------------------------------------------------------------------------------DllMain

extern "C" BOOL WINAPI DllMain( HANDLE hinstance, DWORD ul_reason_being_called, LPVOID )
{
    switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH: 
            sos::_hinstance = (::HINSTANCE)hinstance; 
            sos::application.init();
            sos::spooler::typelib.set_hinstance( (HINSTANCE)hinstance );
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

extern "C" void __declspec(dllexport) WINAPI/*CALLBACK*/ spooler( HWND, HINSTANCE, LPTSTR command_line, int )
{
    sos::Sos_appl my_application;

    sos::spooler_main( NULL, NULL, command_line );
}

//-------------------------------------------------------------------------------------------------

namespace sos 
{
    extern int sos_main0( int argc, char** argv );
}


extern "C" int __declspec(dllexport) spooler_program( int argc, char** argv )
{
    return sos::sos_main0( argc, argv );
    //sos::_argc = argc;
    //sos::_argv = argv;

    //return sos::spooler_main( argc, argv, "" );
}

