//#define MODULE_NAME "hostolea"
//#define COPYRIGHT   "(C)1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/*
    hostOLE als EXE (LocalServer)
*/

#include <precomp.h>

#if defined __BORLANDC__
#   include <windows.h>
#   include <ole2.h>
#   include <variant.h>
#endif

#if defined __BORLANDC__
#   include <windows.h>
#   include <ole2.h>
#   include <variant.h>
#endif

#include <sosstrng.h>
#include <sos.h>
#include <sosprog.h>
#include <sosopt.h>
#include <log.h>
//#define INITGUIDS
#include "hostole.h"
#include "oleserv.h"
#include "olereg.h"


//Make window handle global so other code can cause a shutdown
//HWND        g_hWnd=NULL;

//HINSTANCE   _hinstance;
extern const Bool _dll = false;

//----------------------------------------------------------------------------------Hostole_app

struct Hostole_app : Sos_program
{
                       
    int                         main                    ( int, char** );
};

//---------------------------------------------------------------------------------------------

Hostole_app     app;
Sos_program*    app_ptr = &app;

//------------------------------------------------------------------------------ObjectDestroyed
#if 0
void ObjectDestroyed()
{
    com_object_count--;

    if( com_object_count == 0  &&  com_lock == 0 )
    {
        //No more objects and no locks, shut the app down.
        /*if( IsWindow( g_hWnd ) )  PostMessage( g_hWnd, WM_CLOSE, 0, 0L );
                            else*/  PostQuitMessage( 0 );
    }
}
#endif
//----------------------------------------------------------------------------Hostole_app::main

int Hostole_app::main( int argc, char** argv )
{
    Ole_appl*   hostole_appl = 0;
    Bool        automation   = false;


    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "log" ) )    log_start( opt.value() );
        else
        if( opt.flag( "automation" ) )   { automation = opt.set(); }
        else
        if( opt.flag( "Embedding" ) )    { automation = opt.set(); }
        else
        if( opt.flag( "REGSERVER" ) )    { hostole_register_server(); show_msg( "hostOLE ist registriert\n" ); return 0; }
        else
        if( opt.flag( "UNREGSERVER" ) )  { hostole_unregister_server(); show_msg( "hostOLE ist nicht mehr registriert\n" ); return 0; }
        else throw_sos_option_error( opt );
    }
     
    if( !automation ) {
        HRESULT hr = hostole_register_server();
        if( !FAILED( hr ) ) {
            show_msg( "Der OLE-Automations-Server hostOLE.exe ist registriert.\n"
                      "hostOLE.exe ist keine Anwendung." );
        }
        return 1;
    }

    hostole_appl = new Ole_appl;//(hInst, hInstPrev, pszCmdLine, nCmdShow);
    if( !hostole_appl ) return -1;

    hostole_appl->init();
    hostole_appl->message_loop();

    SOS_DELETE( hostole_appl );

    return 0;
}

//-------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    return app.main( argc, argv );
}

