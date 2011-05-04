/******************************************************************************
 *
 *  _BOOTSTP.C
 *  Copyright (C) 1993, Microsoft Corp.
 *  All Rights Reserved.
 *
 ******************************************************************************
 *
 *  Description:    Extremely simple 16-bit bootstrapper for the 32-bit
 *                  setup executable.  This is only necessary so that the
 *                  DOS setup bootstrapper can detect when the program is
 *                  done so that it can clean up the temporary directory.
 *
 ******************************************************************************
 *
 *  Contains:       WinMain
 *                  WndProc
 *
 ******************************************************************************
 *
 *  Known Bugs:     none
 *
 ******************************************************************************
 *
 *  Possible Improvements:
 *
 ******************************************************************************
 *
 *  Revision History:
 *
 *  none
 *
 *****************************************************************************/



    //
    //  Headers
    //

#define STRICT
#include <windows.h>
#include <stdlib.h>
#include "drvsetup.h"



    //
    //  Constants
    //

const char	ODBCCLASS[]		= ODBCSETUPCLASS;
const char	*BOOTCLASS      = "ODBC_BOOTSTRAP";
const char	*BOOTWINDOW     = "Bootstrapper";
#define		ID_TIMER        1
#define		TIMER_GRAN		100


	//
	//	Globals
	//

BOOL	fStarted			= FALSE;
UINT	nTicks				= 0;




//
//  WndProc
//
//      Simple windows procedure to process the timer messages.
//
LRESULT CALLBACK WndProc(HWND   hwnd,
                         UINT   wmsg,
                         WPARAM wParam,
                         LPARAM lParam)
    {
    switch( wmsg )
        {
        case WM_TIMER:

			if( !fStarted )
				{
					//
					//	If we've waited for more than approx. 5 seconds
					//	and the app hasn't started yet, we'll assume that
					//	it's never going to and that we should abort
					//

				if( nTicks * TIMER_GRAN >= 5000 )
					{
					PostMessage(hwnd, WM_CLOSE, 0, 0L);
					break;
					}
					
				if( FindWindow(ODBCCLASS, NULL) )
					fStarted = TRUE;
				else
					nTicks++;
				}
			else if( !FindWindow(ODBCCLASS, NULL) )
                PostMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, wmsg, wParam, lParam);
        }

    return 0;
    }




//
//  WinMain
//
//      Windows entry point -- start up the 32-bit executable
//      and wait for it to finish.
//
//
int PASCAL WinMain(HINSTANCE  hInstance,
                   HINSTANCE  hPrevInstance,
                   LPSTR      lpszCmdLine,
                   int        nCmdShow)
    {
    WNDCLASS    wc;
    HWND        hwnd;
    MSG         msg;

        //
        //  Don't continue if there's another instance already running;
        //  simply exit silently -- there's no need for TWO bootstrappers
        //  to be running, executing, and waiting on the same program!
        //

    if( hPrevInstance )
        return FALSE;

        //
        //  Make sure that the program we were planning on executing is
        //  not already running for some reason.
        //

    if( FindWindow(ODBCCLASS, NULL) )
        return FALSE;

        //
        //  Fill out the class structure
        //

    wc.style            =   CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      =   WndProc;
    wc.cbClsExtra       =   0;
    wc.cbWndExtra       =   0;
    wc.hInstance        =   hInstance;
    wc.hIcon            =   NULL;
    wc.hCursor          =   LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    =   (HBRUSH)(COLOR_BACKGROUND + 1);
    wc.lpszMenuName     =   NULL;
    wc.lpszClassName    =   BOOTCLASS;

        //
        //  Register the class
        //

    if( !RegisterClass(&wc) )
        return FALSE;

        //
        //  Create the window
        //

    if( !(hwnd = CreateWindow(BOOTCLASS, BOOTWINDOW,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              HWND_DESKTOP,
                              NULL,
                              hInstance,
                              NULL)) )
        return FALSE;

        //
        //  Register a timer
        //

    if( !SetTimer(hwnd, ID_TIMER, TIMER_GRAN, NULL) )
        return FALSE;

        //
        //  Execute the new program
        //

    if( WinExec(lpszCmdLine, SW_SHOW) < 32 )
        return FALSE;

        //
        //  Get and dispatch messages
        //

    while( GetMessage(&msg, NULL, 0, 0) )
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }

        //
        //  Once the program has finished, we can return
        //

    return TRUE;
    }
