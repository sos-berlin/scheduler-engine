//#define MODULE_NAME "sleep"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"

#include <math.h>

#include "../kram/sysdep.h"

#if defined SYSTEM_UNIX
#   include <unistd.h>
#endif

#include "../kram/sos.h"
#include "../kram/sleep.h"


#if defined SYSTEM_WIN16

#include <windows.h>

extern HINSTANCE  _hinstance;


using namespace std;
namespace sos {

const char         sos_sleep_window_class_name[] = "SOS_sos_sleep";


static LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return TRUE;   // Dummy
}


void sos_sleep( double seconds )
{
    WNDCLASS    wc;

    if( !GetClassInfo( _hinstance, sos_sleep_window_class_name, &wc ) )
    {
        wc.style         = 0;
        wc.lpfnWndProc   = WindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = _hinstance;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = sos_sleep_window_class_name;

        ATOM atom = RegisterClass( &wc );
#       if defined __WIN32__
            if( !atom )  throw_mswin_error( "RegisterClass", "sos_sleep" );
#       endif
    }

    HWND hwnd = CreateWindow( sos_sleep_window_class_name,
                              "sos_sleep",     // Fenstername
                              DS_NOIDLEMSG | WS_DISABLED,// Style
                              0,                        // x
                              0,                        // y
                              1,                        // Breite
                              1,                        // Höhe
                              HWND_DESKTOP,             // parent window
                              NULL,                     // hmenu
                              _hinstance,               // hinst
                              NULL );                   // lpvParam

    if( !hwnd )  throw_mswin_error( "CreateWindow", "sos_sleep" );

    const uint timer_id = 1;
    uint timer_handle = SetTimer( hwnd, timer_id, (long)ceil( seconds * 1000.0 ), NULL );
    if( !timer_handle )  throw_mswin_error( "SetTimer" );

    MSG msg;
    BOOL ok = GetMessage( &msg, hwnd, WM_TIMER, WM_TIMER );

    ok = KillTimer( hwnd, timer_id );
    if( !ok )  throw_mswin_error( "KillTimer", "sos_sleep" );

    ok = DestroyWindow( hwnd );
    if( !ok )  throw_mswin_error( "DestroyWindow", "sos_sleep" );
}

} //namespace sos

#elif defined SYSTEM_WIN32

#include <windows.h>

using namespace std;
namespace sos {

void sos_sleep( double seconds )
{
    Sleep( (uint4)ceil( seconds * 1000 ) );
}

} //namespace sos

#else

using namespace std;
namespace sos {

void sos_sleep( double seconds )
{
    //sleep( (uint4)ceil( seconds ) );

    struct timespec t;
    t.tv_sec = (time_t)floor( seconds );
    t.tv_nsec = (time_t)max( 0.0, ( seconds - floor( seconds ) ) * 1e9 );

    nanosleep( &t, NULL );
    //? pthread_delay_np( &t );
}

} //namespace sos

#endif


