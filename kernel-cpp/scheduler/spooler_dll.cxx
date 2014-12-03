// $Id: spooler_dll.cxx 14110 2010-10-27 12:58:32Z jz $

#include "spooler.h"
#include "spooler_dll.h"

#if defined Z_WIN64 || defined _DEBUG       // Win32: Nur die Debug-Variante wird als DLL erzeugt

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
            sos::scheduler::typelib.set_hinstance( (HINSTANCE)hinstance );
            break;

        case DLL_THREAD_ATTACH:     
            break;

        case DLL_PROCESS_DETACH: 
            //2010-10-27  Absturz in statisch aufgerufenem ~Sos_client(), weil Java-VM bereits weg ist: sos::application.exit();
            break;

        case DLL_THREAD_DETACH: 
            break;

        default: 
            break;
    }

    return TRUE;
}

#endif
//-------------------------------------------------------------------------------------------------

namespace sos 
{
    extern int sos_main0( int argc, char** argv );
}

//----------------------------------------------------------------------------------spooler_program

extern "C" int SCHEDULER_EXPORT spooler_program( int argc, char** argv )
{
    return sos::sos_main0( argc, argv );
}
