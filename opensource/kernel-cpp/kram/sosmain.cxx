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

#ifdef SYSTEM_WIN
    HINSTANCE _hinstance = 0; 
#endif

#ifdef Z_LINK_STATIC
    extern const bool _dll = false;
#endif

extern int sos_main0( int argc, char** argv );

//---------------------------------------------------------------------------------------------main

extern "C" int __cdecl main( int argc, char** argv )
{
    _argc = argc;
    _argv = argv;

    int rc = sos_main0( argc, argv );

    return rc;
}

} //namespace sos
