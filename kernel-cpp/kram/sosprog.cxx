#include "precomp.h"
#if 0

#include <sysdep.h>

#if defined SYSTEM_MICROSOFT
#   include <afxwin.h>
#endif

#include <sos.h>
#include <sosprog.h>

using namespace std;

//---------------------------------------------------------------------------------------------

#if defined SYSTEM_WIN
    extern HINSTANCE _hinstance;
#endif

extern int sos_main0( int argc, char** argv );  // soswnmai.cxx


extern int      _argc;
extern char**  _argv;

//--------------------------------------------------------------------Sos_program::InitInstance
#if !defined SYSTEM_MICROSOFT

BOOL Sos_program::InitInstance()
{
    return FALSE;
}

#else
    // InitInstance() ist Einstieg in soswnmai.cxx
#endif
//--------------------------------------------------------------------Sos_program::ExitInstance

BOOL Sos_program::ExitInstance()
{
    //_sos_appl.exit();
    return TRUE;
}

//-------------------------------------------------------------------------------------sos_main
/*
int sos_main( int argc, char** argv ) 
{
#   if !defined SYSTEM_MICROSOFT  //MFC
        app.InitInstance();
#   endif

    app.main( argc, argv );

#   if !defined SYSTEM_MICROSOFT  //MFC
        app.ExitInstance();
#   endif

    return 0;
}
*/
#endif

