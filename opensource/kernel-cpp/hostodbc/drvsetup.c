// drvsetup.c                 ©1997 SOS Software GmbH


/*--------------------------------------------------------------------------
  DRVSETUP.C -- ODBC Setup

  (c) Microsoft Corp., 1990-1994
--------------------------------------------------------------------------*/
#define ODBCVER 0x0210

// Includes ----------------------------------------------------------------
#include	<windows.h>
#include	<windowsx.h>
#ifndef WIN32
#    include	<w16macro.h>
#endif
#include	<ctl3d.h>

#include	<stdlib.h>
#include    <string.h>

#include	<odbcinst.h>
#include	<stdlib.h>
#include	"drvsetup.h"

#ifdef WIN32
#define EXPFUNC	__stdcall
#define INTFUNC	__stdcall
#else
#ifndef EXPORT
#define EXPORT		_export
#endif

#define EXPFUNC	EXPORT CALLBACK
#define INTFUNC	PASCAL
#endif


// Constants ---------------------------------------------------------------
const char ODBCCLASS[]	  = ODBCSETUPCLASS;

const char ODBC_INF[]     = "ODBC.INF";
const char ODBC_INI[]     = "ODBC.INI";
const char ODBCINST_INI[] = "ODBCINST.INI";

const char SW_AUTO[]      = "/AUTO";

const char INI_DEFAULT[]  = "Default";
const char INI_DSOURCES[] = "ODBC Data Sources";
const char INI_DRIVERS[]  = "ODBC Drivers";
const char INI_KDRIVER[]  = "Driver";

const char EMPTYSTR[]     = "";
const char DSNKEY_FMT[]   = "DSN=%s";
const char KEY_FMT[]      = "%s=%s";

#define BUFSIZE		4096
#define STRLEN			256
#define DELAY			5000L

#define WMU_WELCOME	(WM_USER+1000)
#define WMU_INSTALL	(WM_USER+1001)
#define WMU_DSOURCE	(WM_USER+1002)
#define WMU_EXIT		(WM_USER+1003)

#define WMU_DELAY		(WM_USER+2000)

#define cxDEF			620
#define cyDEF			460

#define cPALETTESIZE	256

#define ISBLANK(x)	   	((x) == ' ')
#define ISSLASH(x)		((x) == '\\')
#define ISTAB(x)	   	((x) == '\t')
#define ISWHITE(x)	   	(ISBLANK(x) || ISTAB(x))

#define CANCELOK		((LPARAM)0x00000001L)
#define CANCELNOTOK	((LPARAM)0x00000000L)

#define WIN32S			0x80000000


// Types -------------------------------------------------------------------
typedef struct tagGLOBALS {               // Main window global variables
	HBITMAP		hbitmap;                   //   Bitmap handle
	BITMAP		bm;                        //   Bitmap size
	HPALETTE	hpal;                      //   Palette handle
} GLOBALS, FAR *LPGLOBALS;


// Prototypes --------------------------------------------------------------
BOOL     EXPFUNC DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT  EXPFUNC WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL     INTFUNC AddDSources(HWND);
BOOL     INTFUNC AutoInstall(HWND);
void     INTFUNC CenterWindow(HWND);
void     INTFUNC Delay(void);
HPALETTE INTFUNC MakePalette(void);


// Globals -----------------------------------------------------------------
HINSTANCE	hinst;                         // Instance handle
char		szINF[_MAX_PATH];              // INF path
char		szSrc[_MAX_PATH];              // Source path
char		szODBC_INI[_MAX_PATH];         // ODBC.INI path
char		szODBCINST_INI[_MAX_PATH];     // ODBCINST.INI path
char		szTitle[STRLEN];               // Window title
BOOL		fAuto;                         // /AUTO requested
BOOL		fAutoCtl3d;                    // Ctl3d in auto-subclass mode


/* AddDSources -------------------------------------------------------------
	Description: add data sources by reading information from ODBC.INI

--------------------------------------------------------------------------*/
BOOL INTFUNC AddDSources(HWND hdlg)
{
	BOOL	fSuccess = 0;
	HCURSOR	hcur = 0;
    LPSTR   memory_ptr = NULL;
	LPSTR	lpsz = 0;
	LPSTR	lpszT = 0;

	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

	memory_ptr =  GlobalAllocPtr(GHND, (3 * BUFSIZE) + (2 * STRLEN));
    if( !memory_ptr )  return FALSE;

    memset( memory_ptr, 0, (3 * BUFSIZE) + (2 * STRLEN) );
	lpsz = memory_ptr;

	// to see if there is a default data source
	if (GetPrivateProfileString(INI_DEFAULT, NULL, EMPTYSTR, lpsz, BUFSIZE, szODBC_INI)) {
		lstrcpy(lpsz, INI_DEFAULT);
		lpszT = lpsz + lstrlen(lpsz) + 1;
	}
	else lpszT = lpsz;

	// retrieve the list of data sources
	if (!GetPrivateProfileString(INI_DSOURCES, NULL, EMPTYSTR,
                                 lpszT, BUFSIZE-(lpszT-lpsz), szODBC_INI))
    {
/* jz 9.9.96  Keine Datenquellen? Dann werden eben keine installiert. Kein Fehler
		LPSTR	lpszFmt;
		LPSTR	lpszMsg;

		lpszFmt = lpsz;
		lpszMsg = lpszFmt + _MAX_PATH;

		LoadString(hinst, IDS_BADODBC, lpszFmt, _MAX_PATH);
		wsprintf(lpszMsg, lpszFmt, (LPSTR)szODBC_INI);
		MessageBox(hdlg, lpszMsg, szTitle, MB_ICONEXCLAMATION | MB_OK);

		fSuccess = FALSE;
*/
	}

//jz 9.9.96 else
{
		LPSTR	lpszBuf = NULL;
		LPSTR	lpszAttr = NULL;
		LPSTR	lpszDriver = NULL;
		LPSTR	lpszKey = NULL;
		LPSTR	lpszValue = NULL;
		int		cb = 0;

		lpszBuf    = lpsz;
		lpszDriver = lpszBuf + BUFSIZE;
		lpszValue  = lpszDriver + STRLEN;

		fSuccess = TRUE;

		lpszAttr = lpszValue + STRLEN;
		// walk through all data sources
		for (; *lpsz; lpsz += lstrlen(lpsz) + 1) {

			if (lstrcmpi(lpsz, INI_DEFAULT))
				// read the driver information about this data source
				GetPrivateProfileString(INI_DSOURCES, lpsz, EMPTYSTR,
										lpszDriver, STRLEN, szODBC_INI);
			else
				// read the driver information on the default data source
				GetPrivateProfileString(INI_DEFAULT, INI_KDRIVER, EMPTYSTR,
										lpszDriver, STRLEN, szODBCINST_INI);

			lpszKey = lpszAttr + BUFSIZE;

			// list of keys for this data source
			GetPrivateProfileString(lpsz, NULL, EMPTYSTR, lpszKey, BUFSIZE, szODBC_INI);

			cb = wsprintf(lpszAttr, DSNKEY_FMT, lpsz);
			lpszAttr += cb + 1;

			for (; *lpszKey; lpszKey += lstrlen(lpszKey) + 1)
            {
				if (!lstrcmpi(lpszKey, INI_KDRIVER))  continue;

				// value for lpszKey on data source lpsz
				GetPrivateProfileString(lpsz, lpszKey, EMPTYSTR, lpszValue, STRLEN, szODBC_INI);

				cb = wsprintf(lpszAttr, KEY_FMT, lpszKey, lpszValue);
				lpszAttr += cb + 1;
			}
			*lpszAttr = '\0';

			lpszAttr = lpszValue + STRLEN;

			fSuccess = SQLConfigDataSource(NULL, ODBC_ADD_DSN, lpszDriver, lpszAttr);
			if (!fSuccess) break;
		}

		if (!fSuccess) {
			LPSTR	lpszFmt;
			LPSTR	lpszMsg;

			lpszFmt = lpsz + lstrlen(lpsz) + 1;
			lpszMsg = lpszFmt + _MAX_PATH;

			LoadString(hinst, IDS_BADDS, lpszFmt, _MAX_PATH);
			wsprintf(lpszMsg, lpszFmt, lpsz);
			MessageBox(hdlg, lpszMsg, szTitle, MB_ICONEXCLAMATION | MB_OK);
		}
	}

	GlobalFreePtr( memory_ptr );
	SetCursor(hcur);

	return fSuccess;
}


/* AutoInstall -------------------------------------------------------------
	Description: read from ODBCINST.INI and install the ODBC components
				silently (no dialog boxes)
--------------------------------------------------------------------------*/
BOOL INTFUNC AutoInstall(HWND hdlg)
{
	BOOL	fSuccess = FALSE;
	HCURSOR	hcur;
	LPSTR	lpsz = "?"; //jz 26.12.98

	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
/*
	lpsz = GlobalAllocPtr(GHND, BUFSIZE);
    if( !lpsz )  return FALSE;
    memset( lpsz, 0, BUFSIZE );

	// retrieve a list of drivers from ODBCINST.INI
	if (!GetPrivateProfileString(INI_DRIVERS, NULL, EMPTYSTR,
								lpsz, BUFSIZE, szODBCINST_INI)) {
		LPSTR	lpszFmt;
		LPSTR	lpszMsg;

		lpszFmt = lpsz;
		lpszMsg = lpszFmt + _MAX_PATH;

		LoadString(hinst, IDS_BADODBCI, lpszFmt, _MAX_PATH);
		wsprintf(lpszMsg, lpszFmt, (LPSTR)szODBCINST_INI);
		MessageBox(hdlg, lpszMsg, szTitle, MB_OK);

		fSuccess = FALSE;
	}
	else 
    {
		// lpsz contains the list of drivers
*/
#       if ODBCVER < 0x0300
		    //fSuccess = SQLInstallODBC(hdlg, szINF, szSrc, lpsz);
		    fSuccess = SQLInstallODBC( hdlg, szINF, szSrc, "hostODBC\0" );
#        else
		    //?jz fSuccess = SQLInstallDriverEx(hdlg, szINF, szSrc, lpsz);
			DWORD usage_count;
			fSuccess = SQLInstallDriverEx( "hostODBC\0", NULL /*= Windows system dir*/,
				                           install_dir, sizeof install_dir, 
										   ODBC_INSTALL_INQUIRY, &usage_count );


#       endif
//    }

	if (!fSuccess) {
		LPSTR  lpszMsg = lpsz;
		LoadString(hinst, IDS_BADINST, lpszMsg, _MAX_PATH);
		MessageBox(hdlg, lpszMsg, szTitle, MB_ICONEXCLAMATION | MB_OK);
	}

	GlobalFreePtr(lpsz);
	SetCursor(hcur);

	return fSuccess;
}


/* CenterWindow ------------------------------------------------------------
	Description: place a window to the center of its parent window.
				 if parent window does not exist, place the to the
				 center of desktop window
--------------------------------------------------------------------------*/
void INTFUNC CenterWindow(HWND hwnd)
{
	HWND	hwndParent;
	RECT	rc, rcScr, rcParent;
	int		cx, cy;

	hwndParent = GetParent(hwnd);
	if (!hwndParent) hwndParent = GetDesktopWindow();

	GetWindowRect(hwnd, &rc);
	cx = rc.right  - rc.left;
	cy = rc.bottom - rc.top;

	GetWindowRect(hwndParent, &rcParent);
	rc.top    = rcParent.top  + (((rcParent.bottom - rcParent.top) - cy) >> 1);
	rc.left   = rcParent.left + (((rcParent.right - rcParent.left) - cx) >> 1);
	rc.bottom = rc.top  + cy;
	rc.right  = rc.left + cx;

	GetWindowRect(GetDesktopWindow(), &rcScr);
	if (rc.bottom > rcScr.bottom) {
		rc.bottom = rcScr.bottom;
		rc.top    = rc.bottom - cy;
	}
	if (rc.right  > rcScr.right) {
		rc.right = rcScr.right;
		rc.left  = rc.right - cx;
	}

	if (rc.left < 0) rc.left = 0;
	if (rc.top  < 0) rc.top  = 0;

	MoveWindow(hwnd, rc.left, rc.top, cx, cy, TRUE);
	return;
}


/* Delay -------------------------------------------------------------------
	Description: Delay for DELAY tickcounts

--------------------------------------------------------------------------*/
void INTFUNC Delay(void)
{
	DWORD	cStart;
	HCURSOR	hcur;

	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

	for (cStart=GetTickCount(); (GetTickCount()-cStart) < DELAY; );

	SetCursor(hcur);
	return;
}


/* DlgProc -----------------------------------------------------------------
	Description: a dialog procedure handles OK, CANCEL etc

--------------------------------------------------------------------------*/
BOOL EXPFUNC DlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
//    static BOOL in_update = FALSE;  // jz 9.9.96
	DWORD	fCancelOK;
    BOOL    ret;

	if (msg > WM_USER) {
//OutputDebugString( "drvsetup DlgProc UpdateWindow\n" );
//        if( !in_update )          // jz 9.9.96
        {
//            in_update = TRUE;           // jz 9.9.96
//jz    		UpdateWindow(hdlg);
//            in_update = FALSE;          // jz 9.9.96
        }
//OutputDebugString( "drvsetup DlgProc UpdateWindow ok\n" );
    }

	fCancelOK = (DWORD)GetWindowLong(hdlg, DWL_USER);

	switch (msg) {
		case WM_INITDIALOG:
			CenterWindow(hdlg);

			SetWindowLong(hdlg, DWL_USER, (LONG)lparam);

			if (!fAutoCtl3d)
#ifdef WIN32
				Ctl3dSubclassDlg(hdlg, CTL3D_ALL);
#else
				Ctl3dSubclassDlgEx(hdlg, CTL3D_ALL);
#endif

			if (fAuto) PostMessage(hdlg, WMU_DELAY, 0, 0L);
			{ret=TRUE;goto RETURN;}

		case WM_SYSCOLORCHANGE:
			if (!fAutoCtl3d) {ret=Ctl3dColorChange();goto RETURN;}
			break;

#ifdef WIN32
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			if (!fAutoCtl3d) {ret= (BOOL)Ctl3dCtlColorEx(msg, wparam, lparam);goto RETURN; }
			break;

		case WM_SETTEXT:
		case WM_NCPAINT:
		case WM_NCACTIVATE:
			if (!fAutoCtl3d)
			{
				SetWindowLong(hdlg, DWL_MSGRESULT,
					Ctl3dDlgFramePaint(hdlg, msg, wparam, lparam));
				{ret = TRUE; goto RETURN;}
			}
			break;
#endif

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wparam, lparam)) {
				case IDCANCEL:
					if (!fCancelOK) {
						if (IDOK == DialogBoxParam(hinst,
												MAKEINTRESOURCE(ASKQUIT),
						  						hdlg, DlgProc, CANCELOK))
							{ret = TRUE; goto RETURN;}
					}

				case IDX:
				case IDOK:
					EndDialog(hdlg, GET_WM_COMMAND_ID(wparam ,lparam));
					ret = TRUE; goto RETURN;
			}
			break;

		case WMU_DELAY:
			Delay();
			PostMessage(hdlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK,0,0));
			ret = TRUE; goto RETURN;
	}
	ret = FALSE; goto RETURN;

RETURN:
    return ret;
}


/* MakePalette -------------------------------------------------------------
	Description: Make color palette for the setup background

--------------------------------------------------------------------------*/
HPALETTE INTFUNC MakePalette(void)
{
	LPLOGPALETTE	ppal;
	LPPALETTEENTRY	pent;
	HPALETTE		hpal;
	int	i;

	ppal = (LOGPALETTE FAR *)GlobalAllocPtr(GHND,
								sizeof(LOGPALETTE) +
									(sizeof(PALETTEENTRY) * cPALETTESIZE));

	ppal->palVersion    = 0x300;
	ppal->palNumEntries = cPALETTESIZE;

	pent = ppal->palPalEntry;
	for (i=cPALETTESIZE-1; i >= 0; i--, pent++) {
		pent->peRed   = 0;
		pent->peGreen = 0;
		pent->peBlue  = i;
		pent->peFlags = 0;
	}

	hpal = CreatePalette(ppal);

	GlobalFreePtr(ppal);

	return hpal;
}


/* WndProc -----------------------------------------------------------------
  Description:  Main window message handler

--------------------------------------------------------------------------*/
LRESULT EXPFUNC WndProc(HWND    hwnd,
                        UINT    msg,
                        WPARAM  wparam,
                        LPARAM  lparam)
{
	LPGLOBALS	lpglb;

#ifdef _WIN32
	DWORD	dwVersion;
#endif	//	_WIN32

	if (msg > WM_USER) UpdateWindow(hwnd);

	lpglb = (LPGLOBALS)GetWindowLong(hwnd, 0);

	switch (msg) {

		case WM_CREATE:
			CenterWindow(hwnd);

			lpglb = (LPGLOBALS)GlobalAllocPtr(GHND, sizeof(GLOBALS));
			SetWindowLong(hwnd, 0, (LONG)lpglb);

			lpglb->hbitmap = LoadBitmap(hinst, MAKEINTRESOURCE(IDI_BITMAP));
			lpglb->hpal    = MakePalette();

			GetObject(lpglb->hbitmap, sizeof(BITMAP), &lpglb->bm);

			PostMessage(hwnd, WMU_WELCOME, 0, 0L);
			break;

		case WMU_WELCOME:
			if (IDOK == DialogBoxParam(hinst,
										(fAuto
											? MAKEINTRESOURCE(AWELCOME)
											: MAKEINTRESOURCE(WELCOME)),
										hwnd,
										DlgProc,
										CANCELNOTOK))
				PostMessage(hwnd, WMU_INSTALL, 0, 0L);
			else
				PostMessage(hwnd, WMU_EXIT, EXITQUIT, 0L);
			return TRUE;

		case WMU_INSTALL:
#ifdef _WIN32
			//	Determine Operating System version
			dwVersion = GetVersion();

			if (LOBYTE(LOWORD(dwVersion)) <= 3) {

				// can not install 32 bit driver under Win32s
				if (dwVersion & WIN32S) {
					LPSTR	lpszMsg;

					lpszMsg = GlobalAllocPtr(GHND, BUFSIZE);
					if( lpszMsg ) {
						LoadString(hinst, IDS_WIN32S, lpszMsg, _MAX_PATH);
						MessageBox(hwnd, lpszMsg, szTitle, MB_ICONEXCLAMATION | MB_OK);
						GlobalFreePtr(lpszMsg);
					}

				   	PostMessage(hwnd, WMU_EXIT, EXITQUIT, 0L);
				}
			}
#endif	//	_WIN32
			// install silently or install interactively
			// if it is successful, add the data source
			if ((fAuto && AutoInstall(hwnd)) ||
				(!fAuto && SQLInstallODBC( hwnd, szINF, szSrc, "hostODBC\0" )))
            {
                int rc = MessageBox( hwnd,
                                     "Möchten Sie, dass die Datenquelle BS2000 mit der\n"
                                     "Tabelle Adressen aus der SOS-Demo eingetragen wird?",
                                     "hostODBC",
                                     MB_TASKMODAL | MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
                if( rc == IDYES ) {
                    char buffer [10];
                    OutputDebugString( "drvsetup.c: SQLConfigDataSource()\n" );
                    rc = SQLConfigDataSource( /*NULL*/hwnd, ODBC_ADD_DSN, "hostODBC",
                                              "DSN=BS2000\0"
                                              "DESC=Tabellen am BS2000\0"
                                              "CATALOG=sossql_profile_catalog BS2000-Tabellen\0" );
                    OutputDebugString( "drvsetup.c: SQLConfigDataSource() ok\n" );

                    if( !GetPrivateProfileString( "BS2000-Tabellen", "adressenbs2000", "",
                                                   buffer, sizeof buffer, "sos.ini" ) )
                    {
                        WritePrivateProfileString( "BS2000-Tabellen", "adressenbs2000",
                                                   "alias adressen_bs2000", "sos.ini" );

                        if( !GetPrivateProfileString( "alias", "adressen_bs2000", "",
                                                      buffer, sizeof buffer, "sos.ini" ) )
                        {
                            WritePrivateProfileString( "alias", "adressen_bs2000",
                              //"-key=adr-kurzname "
                                "-fields=(adr-kurzname,adr-nr,adr-firma,adr-firma2,adr-firma3,adr-postfach,adr-strasse,adr-land,adr-lkz,adr-plz,adr-postfachplz,adr-ort,adr-telefon,adr-telefax,adr-ustidnr,adr-kdnr,adr-erstgruppe,adr-erstdat:date('ddmmyy'),adr-erstsb,adr-aendgruppe,adr-aenddat:date('ddmmyy'),adr-aendsb) "
                                "-cobol-type=c:/sosdemo/cob/adressen.cob "
                                "fs | com:adressen.dat key kl=10 kp=0",
                                "sos.ini" );

                            if( !GetPrivateProfileString( "fs", "server", "",
                                                          buffer, sizeof buffer, "sos.ini" ) )
                            {
                                WritePrivateProfileString( "fs", "server",
                                                           "192.0.0.1/4010", "sos.ini" );
                            }
                        }

                        if( !GetPrivateProfileString( "sosfield", "german", "",
                                                      buffer, sizeof buffer, "sos.ini" ) )
                        {
                            WritePrivateProfileString( "sosfield", "german",
                                                        "yes", "sos.ini" );
                        }
                    }
                }

				PostMessage(hwnd, WMU_DSOURCE, 0, 0L);
            }
            else {
				PostMessage(hwnd, WMU_EXIT, EXITQUIT, 0L);
            }
			return TRUE;

		case WMU_DSOURCE:
			if (fAuto) {
				PostMessage(hwnd, WMU_EXIT, (AddDSources(hwnd)? AEXITSUCCESS : AEXITFAILURE), 0L);
			} else {
				OutputDebugString( "drvsetup.c: SQLManageDataSources();\n");
				SQLManageDataSources(hwnd);
				OutputDebugString( "drvsetup.c: SQLManageDataSources() ok\n");
				PostMessage(hwnd, WMU_EXIT, EXITSUCCESS, 0L);
			}
			return TRUE;

		case WMU_EXIT:
			fAuto = (wparam == AEXITSUCCESS);
			DialogBoxParam(hinst, MAKEINTRESOURCE(wparam), hwnd, DlgProc, CANCELOK);
			PostMessage(hwnd, WM_CLOSE, 0, 0L);
			return TRUE;

		case WM_PAINT: {
			PAINTSTRUCT	ps;

		 	BeginPaint(hwnd, &ps);

			// Paint blue background
			{	HPALETTE	hpal;
				HBRUSH		hbr;
				RECT		rc;
				int			cy;
				int			i;

				hpal = SelectPalette(ps.hdc, lpglb->hpal, FALSE);
				RealizePalette(ps.hdc);

				GetClientRect(hwnd, &rc);

				cy = (rc.bottom - rc.top) / cPALETTESIZE;
				if ((rc.bottom - rc.top) % cPALETTESIZE) cy++;

				rc.bottom = rc.top + cy;

				for (i=0; i < cPALETTESIZE; i++)
                {
					hbr = CreateSolidBrush(PALETTEINDEX(i));
					FillRect(ps.hdc, &rc, hbr);
					DeleteObject(hbr);

					rc.top    += cy;
					rc.bottom += cy;
				}

				SelectPalette(ps.hdc, hpal, FALSE);
			}

			// Paint bitmap
			{	HDC		hdc;
				HBITMAP	hbitmap;

				hdc = CreateCompatibleDC(ps.hdc);
				hbitmap = SelectObject(hdc, lpglb->hbitmap);

				BitBlt(ps.hdc, 4, 4, lpglb->bm.bmWidth, lpglb->bm.bmHeight,	hdc, 0, 0, 0x00220326);
				BitBlt(ps.hdc, 0, 0, lpglb->bm.bmWidth, lpglb->bm.bmHeight,	hdc, 0, 0, SRCPAINT);

				SelectObject(hdc, hbitmap);
				DeleteDC(hdc);
			}

			EndPaint(hwnd, &ps);
			break;
		}

		case WM_PALETTECHANGED:
			if ((HWND)wparam == hwnd) break;

		case WM_QUERYNEWPALETTE:
        {
			HDC			hdc;
			HPALETTE	hpal;
			UINT		i;

			hdc  = GetDC(hwnd);
			hpal = SelectPalette(hdc, lpglb->hpal, FALSE);

			i = RealizePalette(hdc);

			SelectPalette(hdc, hpal, FALSE);
			ReleaseDC(hwnd, hdc);

			if(i)  InvalidateRect(hwnd, NULL, TRUE);

			return TRUE;
		}

		case WM_SYSCOLORCHANGE:
			Ctl3dColorChange();
			break;

		case WM_DESTROY:
			if (lpglb->hbitmap) DeleteObject(lpglb->hbitmap);
			if (lpglb->hpal)    DeleteObject(lpglb->hpal);

			GlobalFreePtr(lpglb);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}


/* WinMain -----------------------------------------------------------------
  Description:  Windows entry point

--------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR  szCommand,
                   int    nCmdShow)
{
	HWND    hwnd;
	MSG		msg;

	hinst = hInstance;

	// Initialize by registering a window class and ensuring that only a
	// single instance is executing
	// If a previous instance is executing, then bring it forward
	if (hwnd = FindWindow(ODBCCLASS, NULL)) {
		hwnd = GetLastActivePopup(hwnd);
		if (IsIconic(hwnd))
			OpenIcon(hwnd);
		else
			BringWindowToTop(hwnd);
		return FALSE;
	}

	// If this is the first instance, register and create the main window
	else {
		WNDCLASS	wc;
		LPSTR		lpszS, lpszD;

		// Get source file path
		for (lpszS=szCommand; ISWHITE(*lpszS); lpszS++);
		for (lpszD=szSrc; *lpszS && !ISWHITE(*lpszS); ) *lpszD++ = *lpszS++;
		*lpszD = '\0';
        //jz 9.9.96 szSrc wird unten mit Pfad dieses Programms überschrieben,
        //jz 9.9.96 denn hier stehen die von setup.exe ausgepackten Installationsdateien!

		// Check for /AUTO switch
		for (; ISWHITE(*lpszS); lpszS++);
		fAuto = !lstrcmpi(lpszS, SW_AUTO);

		if (!fAuto && *lpszS != 0)
		{	// if garbage on command line
			char szText[256], szTitle[256];

			LoadString (hInstance, IDS_FRAMETITLE, szTitle, sizeof(szTitle));
			LoadString (hInstance, IDS_BADOPT, szText, sizeof(szText));
			MessageBox (0, szText, szTitle, MB_OK);
			return FALSE;
		}

		// Get other file paths
		GetModuleFileName(hinst, szINF, _MAX_PATH);
		for (lpszS=lpszD=szINF; *lpszS; lpszS++) {
			if (ISSLASH(*lpszS)) lpszD = lpszS;
        }
		if (ISSLASH(*lpszD)) lpszD++;
		*lpszD = '\0';

		lstrcpy(szODBC_INI, szINF);
		lstrcpy(szODBCINST_INI, szINF);

        //jz 9.9.96: Die von setup.exe ausgepackten Installationsdateien:
        lstrcpy( szSrc, szINF );

		lstrcat(szINF, ODBC_INF);
		lstrcat(szODBC_INI, ODBC_INI);
		lstrcat(szODBCINST_INI, ODBCINST_INI);

		// Register window class
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = sizeof(LPGLOBALS);
		wc.hInstance     = hinst;
		wc.hIcon         = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON));
		wc.hCursor       = NULL;
		wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = ODBCCLASS;
		if (!RegisterClass(&wc)) return FALSE;

		// Create the main window
		LoadString(hinst, IDS_FRAMETITLE, szTitle, sizeof(szTitle));
		if (!(hwnd = CreateWindow(ODBCCLASS,
				  				szTitle,
				  				WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, cxDEF, cyDEF,
				  				HWND_DESKTOP,
				  				NULL,
				  				hinst,
				  				NULL)))
			return FALSE;

		Ctl3dRegister(hinst);
#ifndef WIN32
		fAutoCtl3d = Ctl3dAutoSubclass(hinst);
#endif
	}

	// Show the window
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);


	// Get and dispatch messages
	while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Ctl3dUnregister(hinst);

	return TRUE;
}


