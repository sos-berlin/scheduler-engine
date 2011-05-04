#include "precomp.h"
//#define MODULE_NAME "sosmain"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/* Applikation, die main() ruft.
   Ohne Fenster.

   Nicht in einer Bibliothek halten. Explizit einbinden!
*/

#include "sysdep.h"
#include "sysxcept.h"
#include "sosstrng.h"
#include "sos.h"
#include "sosappl.h"    // Sos_static
#include "sosprog.h"    // MFC
#include "log.h"
#include "sosctype.h" //test

#ifdef SYSTEM_WIN
#   include <windows.h>
#   define _CRTDBG_MAPALLOC
#   include <crtdbg.h>
#endif

namespace sos {

extern Bool sos_gui;


#ifdef SYSTEM_WIN
    HINSTANCE _hinstance = 0; 
#endif

#ifdef Z_LINK_STATIC
    extern const bool _dll = false;
#endif

extern int sos_main0( int argc, char** argv );


#if defined __BORLANDC__


    struct Sos_standard_application : Sos_application
    {
        virtual void _main( int, char*[] );
    };


    void Sos_standard_application::_main( int argc, char* argv[] )
    {
        sos_gui = true;
        sos_main0( argc, argv );
        //sos_static_ptr()->close();
    }

    Sos_standard_application sos_application;

#else

//---------------------------------------------------------------------------------------------main

    //int    _argc;
    //char** _argv;

    extern "C" int __cdecl main( int argc, char** argv )
    {
        _argc = argc;
        _argv = argv;

        sos_gui = false;

        int rc = sos_main0( argc, argv );

        return rc;
    }

#endif


//--------------------------------------------------------------------Sos_program::InitInstance
#ifdef SYSTEM_MFC

BOOL Sos_program::InitInstance()  
{
    return FALSE;
}

#endif

} //namespace sos
