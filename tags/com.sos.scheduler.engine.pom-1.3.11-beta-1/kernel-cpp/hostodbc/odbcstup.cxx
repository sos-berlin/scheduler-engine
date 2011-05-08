#define MODULE_NAME "odbcstup"

#ifndef _WIN32
#   define USE_CTL3D
#endif
/*
    TODO:
        - Unterscheidung temporäre und normale DSN (nicht speichern)
        - Rückgabewerte abhg. vom Aufruf bzw. Modus: DSN= oder CATALOG=

*/

/*
** SETUP.C - This is the ODBC sample driver code for
** setup.
**
**	This code is furnished on an as-is basis as part of the ODBC SDK and is
**	intended for example purposes only.
**
*/
/*--------------------------------------------------------------------------
  setup.c -- Sample ODBC setup

  This code demonstrates how to interact with the ODBC Installer.  These
  functions may be part of your ODBC driver or in a separate DLL.

  The ODBC Installer allows a driver to control the management of
  data sources by calling the ConfigDSN entry point in the appropriate
  DLL.  When called, ConfigDSN receives four parameters:

    hwndParent ---- Handle of the parent window for any dialogs which
                    may need to be created.  If this handle is NULL,
                    then no dialogs should be displayed (that is, the
                    request should be processed silently).

    fRequest ------ Flag indicating the type of request (add, configure
                    (edit), or remove).

    lpszDriver ---- Far pointer to a null-terminated string containing
                    the name of your driver.  This is the same string you
                    supply in the ODBC.INF file as your section header
                    and which ODBC Setup displays to the user in lieu
                    of the actual driver filename.  This string needs to
                    be passed back to the ODBC Installer when adding a
                    new data source name.

    lpszAttributes- Far pointer to a list of null-terminated attribute
                    keywords.  This list is similar to the list passed
                    to SQLDriverConnect, except that each key-value
                    pair is separated by a null-byte rather than a
                    semicolon.  The entire list is then terminated with
                    a null-byte (that is, two consecutive null-bytes
                    mark the end of the list).  The keywords accepted
                    should be those for SQLDriverConnect which are
                    applicable, any new keywords you define for ODBC.INI,
                    and any additional keywords you decide to document.

  ConfigDSN should return TRUE if the requested operation succeeds and
  FALSE otherwise.  The complete prototype for ConfigDSN is:

  BOOL FAR PASCAL ConfigDSN(HWND    hwndParent,
                            WORD    fRequest,
                            LPSTR   lpszDriver,
                            LPCSTR  lpszAttributes)

  Your setup code should not write to ODBC.INI directly to add or remove
  data source names.  Instead, link with ODBCINST.LIB (the ODBC Installer
  library) and call SQLWriteDSNToIni and SQLRemoveDSNFromIni.
  Use SQLWriteDSNToIni to add data source names.  If the data source name
  already exists, SQLWriteDSNToIni will delete it (removing all of its
  associated keys) and rewrite it.  SQLRemoveDSNToIni removes a data
  source name and all of its associated keys.

  For NT compatibility, the driver code should not use the
  Get/WritePrivateProfileString windows functions for ODBC.INI, but instead,
  use SQLGet/SQLWritePrivateProfileString functions that are macros (16 bit) or
  calls to the odbcinst.dll (32 bit).

--------------------------------------------------------------------------*/
#if defined __BORLANDC__
#   pragma option -Od          // Optimierung macht sonst mist?
#endif

// ODBCSTUP -> sosdb
#define ONE_ODBC_DLL 1

#ifdef WIN32
#if defined ONE_ODBC_DLL
#include "../kram/sosstrng.h"
#endif
#include "../kram/sos.h"
#endif

#define STRICT
#include <windows.h>


#if defined WIN16
#   include "w16macro.h"
#endif

#include "sql.h"
#include "sqlext.h"

#ifdef USE_CTL3D
#   include "ctl3d.h"
#endif

#include "odbcinst.h"				// ODBC installer prototypes


#include <stdlib.h>
#include <string.h>					// C include files

#ifdef WIN16
#if defined ONE_ODBC_DLL
#include <sosstrng.h>
#endif
#include <sos.h>
#endif


#if defined ONE_ODBC_DLL
#include "../kram/sosappl.h"
#include "../kram/sosprof.h"
#include "../kram/log.h"
#else
#define LOG(X) do{ char _buffer_[300];ostrstream _s_(_buffer_,sizeof _buffer_); _s_ << X << '\0';_buffer_[sizeof _buffer_-1]='\0';OutputDebugString(_buffer_);}while(0)
#endif
#include "sosodbc.hrc"
//#include "sosodbc.h" 				// Local include files

// nach sysdep.h oder sosmwwin.h:
#if defined SYSTEM_WIN32  &&  !defined GET_WM_COMMAND_ID
#   define GET_WM_COMMAND_ID( wParam, lParam )   LOWORD( wParam )
#   define GET_WM_COMMAND_CMD( wParam, lParam )  HIWORD( wParam )
# //?define GET_WM_COMMAND_WND( wParam, lParam )  ( (HWND)(UINT)lParam )
#endif


using namespace std;
namespace sos {

//----------------------------------------------------------------------------

#ifdef WIN32
#   define INTFUNC  __stdcall
#   define EXPFUNC  __stdcall
//#   define EXPFUNC  __declspec(dllexport)
#else
#   define INTFUNC PASCAL
#   define EXPFUNC __export CALLBACK
#endif

//extern "C"
extern HINSTANCE _hinstance;

// Constants ---------------------------------------------------------------

#define MAXPATHLEN      (255+1)           // Max path length
#define MAXKEYLEN       (25+1)            // Max keyword length
#define MAXDESC         (255+1)           // Max description length
#define MAXDSNAME       (32+1)            // Max data source name length

const char EMPTYSTR  []= "";
//const char OPTIONON  []= "Yes";
//const char OPTIONOFF []= "No";

// ODBC.INI keywords
const char ODBC_INI    []="ODBC.INI";     // ODBC initialization file
const char INI_KDESC   []="Description";  // Data source description
const char INI_SDEFAULT[] = "Default";    // Default data source name
;const char INI_SERVER  []="Server";       // winmsql.ini entry
;const char INI_CATALOG  []="Catalog";   // MSQL Database name
const char szTranslateName[] = "TranslationName";
const char szTranslateDLL[] = "TranslationDLL";
const char szTranslateOption[] = "TranslationOption";
const char szUnkTrans[] = "Unknown Translator";

// Attribute key indexes (into an array of Attr structs, see below)
#define KEY_DSN 		0
#define KEY_DESC		1
#define KEY_TRANSNAME	3
#define KEY_TRANSOPTION 4
#define KEY_TRANSDLL	5
#define KEY_CATALOG   	6
//#define KEY_INIFILE     7
//#define KEY_INISECTION  8
#define NUMOFKEYS		9				// Number of keys supported

// Attribute string look-up table (maps keys to associated indexes)

struct Lookup_table {
  char   szKey[MAXKEYLEN];
  int    iKey;
};
/*
Lookup_table s_aLookup_near[] =
{
                  { "DSN",			KEY_DSN,            },
				  { "DESC",			KEY_DESC,           },
				  { "Description",	KEY_DESC,           },
//                { "INIFILE",      KEY_INIFILE,        },
//                { "INISECTION",   KEY_INISECTION,     },
				//{ "Server",		KEY_SERVER,         },
				  { "CATALOG",		KEY_CATALOG,        },
				  { "TranslateName",KEY_TRANSNAME,      },
				  { "TranslateDLL",	KEY_TRANSDLL,       },
				  { "TranslateOption",KEY_TRANSOPTION,  },
				  { "",				0                   }
};
*/
Lookup_table s_aLookup[] =
{
                  { "DSN",			KEY_DSN,            },
				  { "DESC",			KEY_DESC,           },
				  { "Description",	KEY_DESC,           },
//                { "INIFILE",      KEY_INIFILE,        },
//                { "INISECTION",   KEY_INISECTION,     },
				//{ "Server",		KEY_SERVER,         },
				  { "CATALOG",		KEY_CATALOG,        },
				  { "TranslateName",KEY_TRANSNAME,      },
				  { "TranslateDLL",	KEY_TRANSDLL,       },
				  { "TranslateOption",KEY_TRANSOPTION,  },
				  { "",				0                   }
};


// Types -------------------------------------------------------------------

typedef struct tagAttr
{
	BOOL  fSupplied;
	char  szAttr[MAXPATHLEN];
} Attr, FAR * LPAttr;


// Globals -----------------------------------------------------------------
// NOTE:  All these are used by the dialog procedures

typedef struct tagSETUPDLG {
	HWND	hwndParent; 					// Parent window handle
    LPCSTR	lpszDrvr;						// Driver description
	Attr	aAttr[NUMOFKEYS];				// Attribute array
	char	szDSN[MAXDSNAME];				// Original data source name
	BOOL	fNewDSN;						// New data source flag
	BOOL	fDefault;						// Default data source flag
    Bool    something_changed;
    Bool    empty_dsn_allowed;              // temporäre DSN
} SETUPDLG, FAR *LPSETUPDLG;



// Prototypes --------------------------------------------------------------

static void INTFUNC  center_dialog     ( HWND );
static int  CALLBACK config_dlg_proc   ( HWND, /*WORD jz*/uint wMsg, WPARAM, LPARAM );
static void INTFUNC  parse_attributes  ( LPCSTR lpszAttributes, LPSETUPDLG );
static BOOL CALLBACK set_dsn_attributes( HWND, LPSETUPDLG );

/* ConfigDSN ---------------------------------------------------------------
  Description:  ODBC Setup entry point
                This entry point is called by the ODBC Installer
                (see file header for more details)
  Input      :  hwnd ----------- Parent window handle
                fRequest ------- Request type (i.e., add, config, or remove)
                lpszDriver ----- Driver name
                lpszAttributes - data source attribute string
  Output     :  TRUE success, FALSE otherwise
--------------------------------------------------------------------------*/
#if defined ONE_ODBC_DLL
#define ODBC_DRIVERCONNECT_DSN 4
BOOL SosConfigDSN( HWND hwnd, WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes, Sos_string* dsn_ptr, Sos_string* cat_ptr );

extern "C"
BOOL EXPFUNC ConfigDSN( HWND hwnd, WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes  )
{
    return SosConfigDSN( hwnd, fRequest, lpszDriver, lpszAttributes, NULL, NULL );
}

BOOL SosConfigDSN( HWND hwnd, WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes, Sos_string* dsn_ptr, Sos_string* cat_ptr  )
{
	BOOL        fSuccess;						   // Success/fail flag
    Sos_appl    appl;

    try {
        appl.init();
        if( !read_profile_bool( "", "sosodbc", "ConfigDSN_allowed", true ) ) {
            Xc x( "SOS-1376", "ConfigDSN" );
            SHOW_MSG( "Fehler aufgetreten: " << x );
            return FALSE;
        }
    } catch ( const Xc& x ) {
        if ( hwnd ) {
            SHOW_MSG( "Fehler aufgetreten: " << x );
        } else {
            LOG( "Fehler aufgetreten: " << x );
        }
        return FALSE;
    }
    LOG( "SosConfigDSN called\n" );

//LOG("ds="<<hex<<__ds<<", &s_aLookup="<<(void*)s_aLookup<<"\n");

/*
int a; __asm mov a,ds;
do{ char _buffer_[300];ostrstream _s_(_buffer_,sizeof _buffer_); _s_ << "ds="<<hex<<a<<", &s_aLookup="<<(void*)s_aLookup<<"\n" << '\0';_buffer_[sizeof _buffer_-1]='\0';OutputDebugString(_buffer_);}while(0);
//LOG("_x_=" << (void*)&_x_ << " " << _x_ << "\n" );
//LOG("_y_=" << (void*)&_y_ << " " << _y_ << "\n" );
//LOG("_z_" << (void*)&_z_ << " " << _z_ << "\n" );
//LOG("_zz_=" << (void*)&_zz_ << " " << _zz_ << "\n" );
OutputDebugString( "s_aLookup[0].szKey=\"");OutputDebugString( s_aLookup[0].szKey);OutputDebugString("\"\n");
    OutputDebugString( "hostODBC setup ConfigDSN(\"" );
    OutputDebugString( lpszDriver );
    OutputDebugString( "\",\"" );
    OutputDebugString( lpszAttributes );
    OutputDebugString( "\")\n" );
OutputDebugString( "ODBC_INI=\"");OutputDebugString( ODBC_INI);OutputDebugString("\"\n");
*/
//DebugBreak();

	GLOBALHANDLE hglbAttr;
	LPSETUPDLG   lpsetupdlg;

	// Allocate attribute array
	hglbAttr = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(SETUPDLG) );
	if( !hglbAttr )  return FALSE;
	lpsetupdlg = (LPSETUPDLG)GlobalLock( hglbAttr );
    memset( lpsetupdlg, 0, sizeof (SETUPDLG) );

	// Parse attribute string
    if ( fRequest == ODBC_DRIVERCONNECT_DSN ) { // bei SQLDriverConnect übergeben wir keine Attribute
        if ( dsn_ptr && *dsn_ptr != "" ) {
            lstrcpy( lpsetupdlg->aAttr[KEY_DSN].szAttr, c_str(*dsn_ptr) );
            lpsetupdlg->aAttr[KEY_DSN].fSupplied = TRUE;
        }
        if ( cat_ptr && *cat_ptr != "" ) {
            lstrcpy( lpsetupdlg->aAttr[KEY_CATALOG].szAttr, c_str(*cat_ptr) );
            lpsetupdlg->aAttr[KEY_CATALOG].fSupplied = TRUE;
        }
	} else if( lpszAttributes ) parse_attributes( lpszAttributes, lpsetupdlg ); // parse_attributes kann die geschwungenen Klammern nicht richtig

	// Save original data source name
    lstrcpy( lpsetupdlg->szDSN,
             lpsetupdlg->aAttr[KEY_DSN].fSupplied? lpsetupdlg->aAttr[KEY_DSN].szAttr
                                                 : ""  );
    //LOG( "lpsetupdlg->aAttr[KEY_DSN].fSupplied=" << lpsetupdlg->aAttr[KEY_DSN].fSupplied << '\n' );
    //OutputDebugString( "ConfigDSN: lstrcpy ok\n" );

	// Remove data source
	if( fRequest == ODBC_REMOVE_DSN ) {
		// Fail if no data source name was supplied
		if( !lpsetupdlg->aAttr[KEY_DSN].fSupplied )  { fSuccess = FALSE; goto ENDE; }

        fSuccess = SQLRemoveDSNFromIni( lpsetupdlg->aAttr[KEY_DSN].szAttr );
        if( !fSuccess )  goto ENDE;

        // Eintrag in der SOS.INI löschen:
        WritePrivateProfileString( "sossql catalogs", lpsetupdlg->aAttr[KEY_DSN].szAttr,
                                   NULL, "sos.ini" );

        // Wenn der Katalog selbst in einer .ini steht, bleibt er erhalten.
    }
	else // Add or Configure data source oder temporär
    {
        //OutputDebugString( "ConfigDSN: Add or Configure data source\n" );
		// Save passed variables for global access (e.g., dialog access)
		lpsetupdlg->hwndParent = hwnd;
		lpsetupdlg->lpszDrvr   = lpszDriver;
		lpsetupdlg->fNewDSN	   = (ODBC_CONFIG_DSN != fRequest);
		lpsetupdlg->fDefault   = !lstrcmpi( lpsetupdlg->aAttr[KEY_DSN].szAttr, INI_SDEFAULT );
		lpsetupdlg->something_changed = false;
		lpsetupdlg->empty_dsn_allowed = (ODBC_DRIVERCONNECT_DSN == fRequest);

		// Display the appropriate dialog (if parent window handle supplied)
		if( hwnd ) {
            //LOG( "ConfigDSN: DialogBoxParam => erg=" );
            //OutputDebugString( "ConfigDSN: DialogBoxParam\n" );
              int erg = DialogBoxParam( _hinstance,
                                        MAKEINTRESOURCE( CONFIGDSN ),
                                        hwnd,
                                        config_dlg_proc,
                                        (LONG)(LPSTR)lpsetupdlg );
            //LOG( erg << '\n' );
            if ( erg == -1 ) {
                SHOW_MSG( "Fataler Fehler in SosConfigDSN: DialogBox konnte nicht erzeugt werden" );
                return FALSE;
            }
			  fSuccess = IDOK == erg;
            //OutputDebugString( "ConfigDSN: DialogBoxParam ok\n" );
		}
		else
        if( lpsetupdlg->aAttr[KEY_DSN].fSupplied )
			fSuccess = set_dsn_attributes(hwnd, lpsetupdlg);
		else
			fSuccess = FALSE;
        if ( fSuccess ) {
            // DSN zuweisen
            LOG ( "dsn=" << (const char*)lpsetupdlg->aAttr[KEY_DSN].szAttr << '\n' );
            if ( dsn_ptr ) *dsn_ptr = (const char*)lpsetupdlg->aAttr[KEY_DSN].szAttr;
            LOG ( "catalog=" << (const char*)lpsetupdlg->aAttr[KEY_CATALOG].szAttr << '\n' );
            if ( cat_ptr ) *cat_ptr = (const char*)lpsetupdlg->aAttr[KEY_CATALOG].szAttr;
        }
	}
  ENDE:
	GlobalUnlock(hglbAttr);
	GlobalFree(hglbAttr);
	return fSuccess;
}

#else

extern "C"
BOOL EXPFUNC ConfigDSN( HWND hwnd, WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes )
{
//LOG("ds="<<hex<<__ds<<", &s_aLookup="<<(void*)s_aLookup<<"\n");

/*
int a; __asm mov a,ds;
do{ char _buffer_[300];ostrstream _s_(_buffer_,sizeof _buffer_); _s_ << "ds="<<hex<<a<<", &s_aLookup="<<(void*)s_aLookup<<"\n" << '\0';_buffer_[sizeof _buffer_-1]='\0';OutputDebugString(_buffer_);}while(0);
//LOG("_x_=" << (void*)&_x_ << " " << _x_ << "\n" );
//LOG("_y_=" << (void*)&_y_ << " " << _y_ << "\n" );
//LOG("_z_" << (void*)&_z_ << " " << _z_ << "\n" );
//LOG("_zz_=" << (void*)&_zz_ << " " << _zz_ << "\n" );
OutputDebugString( "s_aLookup[0].szKey=\"");OutputDebugString( s_aLookup[0].szKey);OutputDebugString("\"\n");
    OutputDebugString( "hostODBC setup ConfigDSN(\"" );
    OutputDebugString( lpszDriver );
    OutputDebugString( "\",\"" );
    OutputDebugString( lpszAttributes );
    OutputDebugString( "\")\n" );
OutputDebugString( "ODBC_INI=\"");OutputDebugString( ODBC_INI);OutputDebugString("\"\n");
*/
//DebugBreak();

	BOOL         fSuccess;						   // Success/fail flag
	GLOBALHANDLE hglbAttr;
	LPSETUPDLG   lpsetupdlg;

	// Allocate attribute array
	hglbAttr = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(SETUPDLG) );
	if( !hglbAttr )  return FALSE;
	lpsetupdlg = (LPSETUPDLG)GlobalLock( hglbAttr );
    memset( lpsetupdlg, 0, sizeof (SETUPDLG) );

	// Parse attribute string
	if( lpszAttributes )  parse_attributes( lpszAttributes, lpsetupdlg );

	// Save original data source name
    lstrcpy( lpsetupdlg->szDSN,
             lpsetupdlg->aAttr[KEY_DSN].fSupplied? lpsetupdlg->aAttr[KEY_DSN].szAttr
                                                 : ""  );
    //OutputDebugString( "ConfigDSN: lstrcpy ok\n" );

	// Remove data source
	if( fRequest == ODBC_REMOVE_DSN ) {
		// Fail if no data source name was supplied
		if( !lpsetupdlg->aAttr[KEY_DSN].fSupplied )  { fSuccess = FALSE; goto ENDE; }

        fSuccess = SQLRemoveDSNFromIni( lpsetupdlg->aAttr[KEY_DSN].szAttr );
        if( !fSuccess )  goto ENDE;

        // Eintrag in der SOS.INI löschen:
        WritePrivateProfileString( "sossql catalogs", lpsetupdlg->aAttr[KEY_DSN].szAttr,
                                   NULL, "sos.ini" );

        // Wenn der Katalog selbst in einer .ini steht, bleibt er erhalten.
    }
	else // Add or Configure data source
    {
        //OutputDebugString( "ConfigDSN: Add or Configure data source\n" );
		// Save passed variables for global access (e.g., dialog access)
		lpsetupdlg->hwndParent = hwnd;
		lpsetupdlg->lpszDrvr   = lpszDriver;
		lpsetupdlg->fNewDSN	   = (ODBC_ADD_DSN == fRequest);
		lpsetupdlg->fDefault   = !lstrcmpi( lpsetupdlg->aAttr[KEY_DSN].szAttr, INI_SDEFAULT );
		lpsetupdlg->something_changed = false;

		// Display the appropriate dialog (if parent window handle supplied)
		if( hwnd ) {
            //OutputDebugString( "ConfigDSN: DialogBoxParam\n" );
			  fSuccess = (IDOK == DialogBoxParam( _hinstance,
                                                  MAKEINTRESOURCE( CONFIGDSN ),
                                                  hwnd,
                                                  config_dlg_proc,
                                                  (LONG)(LPSTR)lpsetupdlg ) );
            //OutputDebugString( "ConfigDSN: DialogBoxParam ok\n" );
		}
		else
        if( lpsetupdlg->aAttr[KEY_DSN].fSupplied )
			fSuccess = set_dsn_attributes(hwnd, lpsetupdlg);
		else
			fSuccess = FALSE;
	}

  ENDE:
	GlobalUnlock(hglbAttr);
	GlobalFree(hglbAttr);
	return fSuccess;
}
#endif


/* center_dialog ------------------------------------------------------------
	Description:  Center the dialog over the frame window
	Input	   :  hdlg -- Dialog window handle
	Output	   :  None
--------------------------------------------------------------------------*/

void INTFUNC center_dialog( HWND hdlg )
{
	HWND	hwndFrame;
	RECT	rcDlg, rcScr, rcFrame;
	int		cx, cy;

	hwndFrame = GetParent( hdlg );

	GetWindowRect(hdlg, &rcDlg);
	cx = rcDlg.right  - rcDlg.left;
	cy = rcDlg.bottom - rcDlg.top;

	GetClientRect(hwndFrame, &rcFrame);
	ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.left));
	ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.right));
	rcDlg.top    = rcFrame.top  + (((rcFrame.bottom - rcFrame.top) - cy) >> 1);
	rcDlg.left   = rcFrame.left + (((rcFrame.right - rcFrame.left) - cx) >> 1);
	rcDlg.bottom = rcDlg.top  + cy;
	rcDlg.right  = rcDlg.left + cx;

	GetWindowRect(GetDesktopWindow(), &rcScr);

	if (rcDlg.bottom > rcScr.bottom) {
		rcDlg.bottom = rcScr.bottom;
		rcDlg.top    = rcDlg.bottom - cy;
	}

	if (rcDlg.right  > rcScr.right)	{
		rcDlg.right = rcScr.right;
		rcDlg.left  = rcDlg.right - cx;
	}

	if (rcDlg.left < 0) rcDlg.left = 0;
	if (rcDlg.top  < 0) rcDlg.top  = 0;

	MoveWindow(hdlg, rcDlg.left, rcDlg.top, cx, cy, TRUE);
}

//-----------------------------------------------------------------------------------------trim
#ifndef ONE_ODBC_DLL

void trim( char* string )
{
    char* q = string + strlen( string );
    while( q > string  &&  q[-1] == ' ' )  q--;
    *q = '\0';

    char* p = string;
    if( *p == ' ' ) {
        while( *p == ' ' )  p++;
        memmove( string, p, q - p + 1 );
    }
}

#endif
//-------------------------------------------------------------------------------enable_disable

static void enable_disable( HWND hdlg, Bool empty_dsn_allowed )
{
    int profile = SendDlgItemMessage( hdlg, IDC_PROFILE, BM_GETSTATE, 0, 0 ) & 3;
    int cobol   = SendDlgItemMessage( hdlg, IDC_COBOL  , BM_GETSTATE, 0, 0 ) & 3;
    int any     = SendDlgItemMessage( hdlg, IDC_ANY    , BM_GETSTATE, 0, 0 ) & 3;

    EnableWindow( GetDlgItem( hdlg, IDC_INIFILE_TEXT    ), profile );
    EnableWindow( GetDlgItem( hdlg, IDC_INIFILE         ), profile );
    EnableWindow( GetDlgItem( hdlg, IDC_INISECTION_TEXT ), profile );
    EnableWindow( GetDlgItem( hdlg, IDC_INISECTION      ), profile );
    EnableWindow( GetDlgItem( hdlg, IDC_COBOLFILE_TEXT  ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_COBOLFILE       ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_IP_TEXT         ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_IP              ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_PORT_TEXT       ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_PORT            ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_BS2NAME_TEXT    ), cobol );
    EnableWindow( GetDlgItem( hdlg, IDC_BS2NAME         ), cobol );
  //EnableWindow( GetDlgItem( hdlg, IDC_CATALOG_TEXT    ), any );
    EnableWindow( GetDlgItem( hdlg, IDC_CATALOG         ), any );

    // OK-Taste:
    char szItem[MAXDSNAME];		// Edit control text
    EnableWindow( GetDlgItem( hdlg, IDOK ),
                  (empty_dsn_allowed || GetDlgItemText( hdlg, IDC_DSNAME, szItem, sizeof(szItem) ) > 0)
                  && ( profile || cobol || any )
                );
}

//---------------------------------------------------------------------------parse_catalog_name

static void parse_catalog_name( HWND hdlg, const char* catalog )
{
    char        text [ 500+1 ];
    char*       t   = text;
    const char* p   = catalog;
    char        quote;

    SendDlgItemMessage( hdlg, IDC_PROFILE, BM_SETCHECK, FALSE, 0 );
    SendDlgItemMessage( hdlg, IDC_COBOL  , BM_SETCHECK, FALSE, 0 );
    SendDlgItemMessage( hdlg, IDC_ANY    , BM_SETCHECK, FALSE, 0 );

    SetDlgItemText( hdlg, IDC_INISECTION, "" );
    SetDlgItemText( hdlg, IDC_INIFILE   , "" );
    SetDlgItemText( hdlg, IDC_COBOLFILE , "" );
    SetDlgItemText( hdlg, IDC_IP        , "" );
    SetDlgItemText( hdlg, IDC_PORT      , "" );
    SetDlgItemText( hdlg, IDC_BS2NAME   , "" );

    //OutputDebugString( "if( strncmp( p, \"sossql_profile_catalog \", 23 ) == 0 )\n");
    if( strncmp( p, "sossql_profile_catalog ", 23 ) == 0 )
    {
        // sossql_profile_catalog SECTION INIFILE
        p += 23;

        while( *p == ' ' )  p++;
        if( *p == '\'' || *p == '"' )  { quote = *p; p++; }  else quote = ' ';
        while( *p  &&  *p != quote )  *t++ = *p++;
        if( *p == quote )  p++;
        *t++ = '\0';
        SetDlgItemText( hdlg, IDC_INISECTION, text );

        while( *p == ' ' )  p++;
        t = text;
        while( *p  &&  *p != ' ' )  *t++ = *p++;
        *t++ = '\0';
        SetDlgItemText( hdlg, IDC_INIFILE, text );

        while( *p == ' ' )  p++;
        if( !*p ) {
            SendDlgItemMessage( hdlg, IDC_PROFILE, BM_SETCHECK, TRUE, 0 );
            //enable_disable( hdlg, IDC_PROFILE );
        }
    }

    if( strncmp( p, "-cobol-type=", 12 ) == 0 )
    {
        //-cobol-type=COBOLFILE -binary fs -server=IP/PORT | com:CATALOG
        p += 12;
        if( *p == '\'' || *p == '"' )  { quote = *p; p++; }  else quote = ' ';
        while( *p  &&  *p != quote )  *t++ = *p++;
        if( *p == quote )  p++;
        *t++ = '\0';
        SetDlgItemText( hdlg, IDC_COBOLFILE, text );

        while( *p == ' ' )  p++;
        //if( strncmp( p, "-in -out -key=TABLE_NAME -binary fs -server=", 44 ) != 0 )  goto ERR;
        //p += 44;
        if( strncmp( p, "fs -server=", 11 ) != 0 )  goto ERR;
        p += 11;

        t = text;
        while( *p  &&  *p != '/' )  *t++ = *p++;
        *t++ = '\0';
        SetDlgItemText( hdlg, IDC_IP, text );

        if( *p != '/' )  goto ERR;
        p++;
        t = text;
        while( *p  &&  *p != ' ' )  *t++ = *p++;
        *t++ = '\0';
        SetDlgItemText( hdlg, IDC_PORT, text );

        while( *p == ' ' )  p++;
        if( *p != '|' )  goto ERR;
        p++;

        while( *p == ' ' )  p++;
        if( strncmp( p, "com:", 4 ) != 0 )  goto ERR;
        p += 4;

        t = text;
        while( *p )  *t++ = *p++;
        *t++ = '\0';
        SetDlgItemText( hdlg, IDC_BS2NAME, text );

        if( !*p ) {
            SendDlgItemMessage( hdlg, IDC_COBOL, BM_SETCHECK, TRUE, 0 );
            //enable_disable( hdlg, IDC_COBOL );
        }

      ERR: ;
    }

    SetDlgItemText( hdlg, IDC_CATALOG, catalog );

    if( *p )  {
        SendDlgItemMessage( hdlg, IDC_ANY, BM_SETCHECK, TRUE, 0 );
        //enable_disable( hdlg, IDC_ANY );
    }
}


/* config_dlg_proc -----------------------------------------------------------
  Description:  Manage add data source name dialog
  Input      :  hdlg --- Dialog window handle
                wMsg --- Message
                wParam - Message parameter
                lParam - Message parameter
  Output     :  TRUE if message processed, FALSE otherwise
--------------------------------------------------------------------------*/

int CALLBACK config_dlg_proc( HWND hdlg, uint wMsg, WPARAM wParam, LPARAM lParam )
{
//OutputDebugString( "config_dlg_proc()\n");
	LPSETUPDLG lpsetupdlg = (LPSETUPDLG)GetWindowLong(hdlg, DWL_USER);

	switch (wMsg) {

	case WM_INITDIALOG:  // Initialize the dialog
	{
//OutputDebugString( "INITDIALOG\n");
		LPSETUPDLG lpsetupdlg;
		LPCSTR	   lpszDSN;

#       ifdef USE_CTL3D
		    Ctl3dRegister( _hinstance );
#       endif
		SetWindowLong( hdlg, DWL_USER, lParam );

#       ifdef WIN32
		    //? Ctl3dSubclassDlg(hdlg, CTL3D_ALL);
#        else
		    Ctl3dSubclassDlgEx(hdlg, CTL3D_ALL);
#       endif

		center_dialog(hdlg); 				// Center dialog

		lpsetupdlg = (LPSETUPDLG) lParam;
		lpszDSN    = lpsetupdlg->aAttr[KEY_DSN].szAttr;


		// Initialize dialog fields
		// NOTE: Values supplied in the attribute string will always
		//		 override settings in ODBC.INI
		SetDlgItemText( hdlg, IDC_DSNAME, lpszDSN );
        if( !lpsetupdlg->aAttr[KEY_DSN].fSupplied ) {
            EnableWindow( GetDlgItem( hdlg, IDC_DSNAME ), TRUE );
        }

		if (!lpsetupdlg->aAttr[KEY_DESC].fSupplied) {
		    SQLGetPrivateProfileString( lpszDSN, "Description", "",
                                        lpsetupdlg->aAttr[1].szAttr,
                                        sizeof(lpsetupdlg->aAttr[1].szAttr),
                                        ODBC_INI );
        }

		SetDlgItemText(hdlg, IDC_DESC, lpsetupdlg->aAttr[KEY_DESC].szAttr);

		if (!lpsetupdlg->aAttr[KEY_CATALOG].fSupplied)
        {
            GetPrivateProfileString( "sossql catalogs", lpszDSN, "",
                                     lpsetupdlg->aAttr[KEY_CATALOG].szAttr,
                                     sizeof(lpsetupdlg->aAttr[KEY_CATALOG].szAttr),
                                     "sos.ini" );
        }

        parse_catalog_name( hdlg, lpsetupdlg->aAttr[KEY_CATALOG].szAttr );

		//SetDlgItemText(hdlg, IDC_CATALOG, lpsetupdlg->aAttr[KEY_CATALOG].szAttr);
/*
		if (lpsetupdlg->aAttr[KEY_INISECTION].fSupplied) {
    		SetDlgItemText(hdlg, IDC_INISECTION, lpsetupdlg->aAttr[KEY_INISECTION].szAttr);
            SendDlgItemMessage( hdlg, IDC_PROFILE, BM_SETCHECK, TRUE, 0 );
        }

		if (lpsetupdlg->aAttr[KEY_INIFILE].fSupplied) {
    		SetDlgItemText(hdlg, IDC_INIFILE, lpsetupdlg->aAttr[KEY_INIFILE].szAttr);
            SendDlgItemMessage( hdlg, IDC_PROFILE, BM_SETCHECK, TRUE, 0 );
        }
*/
        // Set Translation fields
		if (!lpsetupdlg->aAttr[KEY_TRANSDLL].fSupplied)
        {
			SQLGetPrivateProfileString( lpsetupdlg->aAttr[KEY_DSN].szAttr,
                                        szTranslateDLL,
                                        EMPTYSTR,
                                        lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr,
                                        sizeof(lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr),
                                        ODBC_INI );
		}

		if (!lpsetupdlg->aAttr[KEY_TRANSOPTION].fSupplied)
        {
			SQLGetPrivateProfileString( lpsetupdlg->aAttr[KEY_DSN].szAttr,
                                        szTranslateOption,
                                        EMPTYSTR,
                                        lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr,
                                        sizeof(lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr),
                                        ODBC_INI);
		}

		if (!lpsetupdlg->aAttr[KEY_TRANSNAME].fSupplied)
		{
			if( !SQLGetPrivateProfileString( lpsetupdlg->aAttr[KEY_DSN].szAttr,
                                             szTranslateName,
                                             EMPTYSTR,
                                             lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr,
                                             sizeof(lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr),
                                             ODBC_INI))
			{
				if (*lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr) {
					lstrcpy (lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr, szUnkTrans);
				}
			}
		}

		SetDlgItemText( hdlg, IDC_TRANS_NAME, lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr);

		if (lpsetupdlg->fDefault) {
			EnableWindow(GetDlgItem(hdlg, IDC_DSNAME), FALSE);
			EnableWindow(GetDlgItem(hdlg, IDC_DSNAMETEXT), FALSE);
		}
		else {
			SendDlgItemMessage(hdlg, IDC_DSNAME, EM_LIMITTEXT, (WPARAM)(MAXDSNAME-1), 0L);
        }

		SendDlgItemMessage(hdlg, IDC_DESC, EM_LIMITTEXT, (WPARAM)(MAXDESC-1), 0L);
        enable_disable( hdlg, lpsetupdlg->empty_dsn_allowed );
		return TRUE;						// Focus was not set
    }

#ifdef USE_CTL3D
#   ifdef WIN32
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
		return (BOOL)Ctl3dCtlColorEx(wMsg, wParam, lParam);

	case WM_SETTEXT:
	case WM_NCPAINT:
	case WM_NCACTIVATE:
		SetWindowLong(hdlg, DWL_MSGRESULT,
			Ctl3dDlgFramePaint(hdlg, wMsg, wParam, lParam));
		return TRUE;
#   endif

	case WM_SYSCOLORCHANGE:
        return Ctl3dColorChange();
#endif

    // Process buttons
    case WM_COMMAND:
//OutputDebugString( "WM_COMMAND\n");
        uint2 resid = GET_WM_COMMAND_ID(wParam, lParam);

        if( resid != IDC_DSNAME
         && GET_WM_COMMAND_CMD(wParam, lParam) == EN_SETFOCUS )
        {
            lpsetupdlg->something_changed = true;
        }

//OutputDebugString( "WM_COMMAND: switch\n");
		switch( resid ) {

        case IDC_DSNAME:  // Ensure the OK button is enabled only when a data source name is entered
//OutputDebugString( "WM_COMMAND: IDC_DSNAME\n");
			if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
			{
                //Datenquellenname geändert? (nur bei ADD): sos.ini lesen:
                if( !lpsetupdlg->aAttr[KEY_CATALOG].fSupplied
                  && !lpsetupdlg->something_changed  )
                {
                    GetDlgItemText( hdlg, IDC_DSNAME,
                                    lpsetupdlg->szDSN,
                                    sizeof lpsetupdlg->szDSN);
//OutputDebugString( "WM_COMMAND: IDC_DSNAME trim\n");
                    trim( lpsetupdlg->aAttr[KEY_DSN].szAttr );

                    GetPrivateProfileString( "sossql catalogs", lpsetupdlg->szDSN, "",
                                            lpsetupdlg->aAttr[KEY_CATALOG].szAttr,
                                            sizeof(lpsetupdlg->aAttr[KEY_CATALOG].szAttr),
                                            "sos.ini" );
//OutputDebugString( "WM_COMMAND: IDC_DSNAME parse_catalog_name \"");
//OutputDebugString( lpsetupdlg->aAttr[KEY_CATALOG].szAttr );
//OutputDebugString( "\"\n" );
                    parse_catalog_name( hdlg, lpsetupdlg->aAttr[KEY_CATALOG].szAttr );
                }

                enable_disable( hdlg, lpsetupdlg->empty_dsn_allowed );
//OutputDebugString( "WM_COMMAND: IDC_DSNAME return TRUE\n");
				return TRUE;
			}
//OutputDebugString( "WM_COMMAND: IDC_DSNAME ok\n");
			break;

		case IDC_SELECT:  //	Translator selection
		{
//OutputDebugString( "WM_COMMAND: IDC_SELECT\n");
			WORD cbNameOut, cbPathOut;
			uint4 vOption;
			LPSETUPDLG lpsetupdlg = (LPSETUPDLG)GetWindowLong(hdlg, DWL_USER);
			GetDlgItemText( hdlg, IDC_TRANS_NAME, lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr,
                            sizeof (lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr));
			vOption = atol( lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr );

			if( SQLGetTranslator( hdlg,
                                  lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr,
                                  sizeof (lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr)-1,
                                  &cbNameOut,
                                  lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr,
                                  sizeof (lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr)-1,
                                  &cbPathOut, &vOption ) )
			{
				if (cbNameOut == 0)
				{	//	No translator selected
					*lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr = 0;
					*lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr = 0;
					vOption = 0;
				}
				SetDlgItemText (hdlg, IDC_TRANS_NAME, lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr);
				ltoa (vOption, lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr, 10);
			}
			return TRUE;
		}

		case IDC_PROFILE:
        case IDC_COBOL:
        case IDC_ANY:
        {
//OutputDebugString( "WM_COMMAND: IDC_PROFILE,COBOL,ANY\n");
            //if( GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED ) {
            enable_disable( hdlg, lpsetupdlg->empty_dsn_allowed );
          //if( SendDlgItemMessage( hdlg, IDC_PROFILE, BM_GETCHECK, 0, 0 ) )  enable_disable( hdlg, IDC_PROFILE );
            if( SendDlgItemMessage( hdlg, IDC_COBOL  , BM_GETCHECK, 0, 0 ) ) {
                //enable_disable( hdlg, IDC_COBOL   );
                char port [ 100+1 ];
     			GetDlgItemText( hdlg, IDC_PORT, port, sizeof port );
                trim( port );
                if( port[0] == '\0' )  SetDlgItemText( hdlg, IDC_PORT, "4001" );
            }
          //if( SendDlgItemMessage( hdlg, IDC_ANY    , BM_GETCHECK, 0, 0 ) )  enable_disable( hdlg, IDC_ANY     );

  		    return TRUE;
        }

        // Accept results
		case IDOK:
		{
//OutputDebugString( "WM_COMMAND: IDC_OK\n");
            Bool isprofile = SendDlgItemMessage(hdlg, IDC_PROFILE, BM_GETCHECK, 0, 0) != 0;
            Bool iscobol   = SendDlgItemMessage(hdlg, IDC_COBOL  , BM_GETCHECK, 0, 0) != 0;
            Bool isany     = SendDlgItemMessage(hdlg, IDC_ANY    , BM_GETCHECK, 0, 0) != 0;

			LPSETUPDLG lpsetupdlg;

			lpsetupdlg = (LPSETUPDLG)GetWindowLong(hdlg, DWL_USER);
			// Retrieve dialog values

            char* cat = lpsetupdlg->aAttr[KEY_CATALOG].szAttr;

            if( isprofile ) {
                char ini_file [ 100+1 ];
    			GetDlgItemText( hdlg, IDC_INIFILE, ini_file, sizeof ini_file );
                trim( ini_file );

                char section [ 100+1 ];
    			GetDlgItemText( hdlg, IDC_INISECTION, section, sizeof section );
                trim( section );

                strcpy( cat, "sossql_profile_catalog " );
                if( strchr( section, ' ' ) )  strcat( cat, "'" );
                strcat( cat, section );
                if( strchr( section, ' ' ) )  strcat( cat, "'" );
                strcat( cat, " " );
                strcat( cat, ini_file );
            }
            if( iscobol ) {
                char cobol_file [ 100+1 ];
    			GetDlgItemText( hdlg, IDC_COBOLFILE, cobol_file, sizeof cobol_file );
                trim( cobol_file );

                char ip[ 100+1 ];
    			GetDlgItemText( hdlg, IDC_IP, ip, sizeof ip );
                trim( ip );

                char port [ 20+1 ];
    			GetDlgItemText( hdlg, IDC_PORT, port, sizeof port );
                trim( port );

                char bs2_file [ 100+1 ];
    			GetDlgItemText( hdlg, IDC_BS2NAME, bs2_file, sizeof bs2_file );
                trim( bs2_file );

                //-cobol-type=COBOLFILE -in -out -key=TABLE_NAME -binary fs -server=IP/PORT | com:CATALOG
                strcpy( cat, "-cobol-type=" );
                if( strchr( cobol_file, ' ' ) )  strcat( cat, "'" );
                strcat( cat, cobol_file );
                if( strchr( cobol_file, ' ' ) )  strcat( cat, "'" );
                strcat( cat, " fs -server=" );
                strcat( cat, ip );
                strcat( cat, "/" );
                strcat( cat, port );
                strcat( cat, " | com:" );
                strcat( cat, bs2_file );
            }
            if( isany ) {
    			GetDlgItemText( hdlg, IDC_CATALOG, cat,
                                                   sizeof lpsetupdlg->aAttr[KEY_CATALOG].szAttr );
                trim( cat );
            }
			if (!lpsetupdlg->fDefault) {
                GetDlgItemText( hdlg, IDC_DSNAME,
                                lpsetupdlg->aAttr[KEY_DSN].szAttr,
                                sizeof(lpsetupdlg->aAttr[KEY_DSN].szAttr));
                trim( lpsetupdlg->aAttr[KEY_DSN].szAttr );
            }
			GetDlgItemText( hdlg, IDC_DESC,
                            lpsetupdlg->aAttr[KEY_DESC].szAttr,
                            sizeof(lpsetupdlg->aAttr[KEY_DESC].szAttr));
            trim( lpsetupdlg->aAttr[KEY_DESC].szAttr );
			// Update ODBC.INI
			set_dsn_attributes(hdlg, lpsetupdlg);
        }

        // Return to caller
        case IDCANCEL:
//OutputDebugString( "WM_COMMAND: IDCANCEL\n");
#           ifdef USE_CTL3D
			    Ctl3dUnregister (_hinstance);
#           endif
            EndDialog(hdlg, wParam);
			return TRUE;

        default: ;//OutputDebugString( "hostODBC setup WM_COMMAND: Unbekannte resid\n" );
		}
		break;

	}

	// Message not processed
	return FALSE;
}


/* parse_attributes ---------------------------------------------------------
  Description:  Parse attribute string moving values into the aAttr array
  Input      :  lpszAttributes - Pointer to attribute string
  Output     :  None (global aAttr normally updated)
--------------------------------------------------------------------------*/

void INTFUNC parse_attributes ( LPCSTR lpszAttributes, LPSETUPDLG lpsetupdlg )
{
//OutputDebugString( "parse_attributes begin\n" );
//OutputDebugString( "s_aLookup[0].szKey=\"");OutputDebugString( s_aLookup[0].szKey);OutputDebugString("\"\n");
	LPCSTR	lpsz;
	LPCSTR	lpszStart;
	char	aszKey[MAXKEYLEN];
	int		iElement;
	int		cbKey;

	for( lpsz = lpszAttributes; *lpsz; lpsz++ )
	{   //  Extract key name (e.g., DSN), it must be terminated by an equals
//OutputDebugString( lpsz );
//OutputDebugString( "\\0" );
		lpszStart = lpsz;
		for (;; lpsz++)
		{
			if (!*lpsz) goto ENDE;		// No key was found
			else if (*lpsz == '=')	break;		// Valid key found
		}
		// Determine the key's index in the key table (-1 if not found)
		iElement = -1;
		cbKey	 = lpsz - lpszStart;
		if (cbKey < sizeof(aszKey))
		{
			register int j;

			memcpy(aszKey, lpszStart, cbKey);
			aszKey[cbKey] = '\0';
			for (j = 0; *s_aLookup[j].szKey; j++)
			{
                //OutputDebugString(aszKey );
                //OutputDebugString("=" );
                //OutputDebugString(s_aLookup[j].szKey );
                //OutputDebugString("?\n" );
				if (!lstrcmpi(s_aLookup[j].szKey, aszKey))
				{
					//OutputDebugString( "iElement = s_aLookup[j].iKey;\n");
					iElement = s_aLookup[j].iKey;
					break;
				}
			}
		}

		// Locate end of key value
		lpszStart = ++lpsz;  // lpsz zeigt auf den Wert nach '='
		for (; *lpsz; lpsz++);

		// Save value if key is known
		// NOTE: This code assumes the szAttr buffers in aAttr have been
		//	   zero initialized
        //OutputDebugString( "if iElement >= 0\n");
		if (iElement >= 0)
		{
			lpsetupdlg->aAttr[iElement].fSupplied = TRUE;
			memcpy(lpsetupdlg->aAttr[iElement].szAttr,
				lpszStart,
				MIN(lpsz-lpszStart+1, sizeof(lpsetupdlg->aAttr[0].szAttr)-1));
            lpsetupdlg->aAttr[0].szAttr[ sizeof(lpsetupdlg->aAttr[0].szAttr)-1 ] = '\0';
//OutputDebugString( lpsetupdlg->aAttr[iElement].szAttr );
//OutputDebugString( ".\n" );
		}
	}
ENDE: ;
//OutputDebugString( "parse_attributes end\n" );
}


/* set_dsn_attributes --------------------------------------------------------
  Description:  Write data source attributes to ODBC.INI
  Input      :  hwnd - Parent window handle (plus globals)
  Output     :  TRUE if successful, FALSE otherwise
--------------------------------------------------------------------------*/

BOOL INTFUNC set_dsn_attributes( HWND hwndParent, LPSETUPDLG lpsetupdlg )
{
	LPCSTR	lpszDSN;						// Pointer to data source name

	lpszDSN = lpsetupdlg->aAttr[KEY_DSN].szAttr;

    if ( strlen(lpszDSN) == 0 ) return TRUE; // ???

	// Validate arguments
	if (lpsetupdlg->fNewDSN && !*lpsetupdlg->aAttr[KEY_DSN].szAttr)
		return FALSE;

	// Write the data source name
	if (!SQLWriteDSNToIni(lpszDSN, lpsetupdlg->lpszDrvr))
	{
		if (hwndParent)
		{
			char  szBuf[MAXPATHLEN];
			char  szMsg[MAXPATHLEN];

			LoadString(_hinstance, IDS_BADDSN, szBuf, sizeof(szBuf));
			wsprintf(szMsg, szBuf, lpszDSN);
			LoadString(_hinstance, IDS_MSGTITLE, szBuf, sizeof(szBuf));
			MessageBox(hwndParent, szMsg, szBuf, MB_ICONEXCLAMATION | MB_OK);
		}
		return FALSE;
	}

	// Update ODBC.INI
	// Save the value if the data source is new, if it was edited, or if
	// it was explicitly supplied
	if (hwndParent || lpsetupdlg->aAttr[KEY_DESC].fSupplied )
    {
        SQLWritePrivateProfileString( lpszDSN,
                                      INI_KDESC,
                                      lpsetupdlg->aAttr[KEY_DESC].szAttr,
                                      ODBC_INI);
    }

	if (hwndParent || lpsetupdlg->aAttr[KEY_CATALOG].fSupplied )
    {
		WritePrivateProfileString( "sossql catalogs",
                                   lpszDSN,
                                   lpsetupdlg->aAttr[KEY_CATALOG].szAttr,
                                   "sos.ini" );
    }

	if (hwndParent || lpsetupdlg->aAttr[KEY_TRANSNAME].fSupplied )
	{
		SQLWritePrivateProfileString( lpszDSN,
                                      szTranslateName,
                                      *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr ?
                                      lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr: NULL,
                                      ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,
                                      szTranslateDLL,
                                      *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr ?
                                      lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr: NULL,
                                      ODBC_INI);
		SQLWritePrivateProfileString( lpszDSN,
                                      szTranslateOption,
                                      *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr ?
                                      lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr: NULL,
                                      ODBC_INI);
	}

	// If the data source name has changed, remove the old name
	if (lpsetupdlg->aAttr[KEY_DSN].fSupplied &&
		lstrcmpi(lpsetupdlg->szDSN, lpsetupdlg->aAttr[KEY_DSN].szAttr))
	{
		SQLRemoveDSNFromIni(lpsetupdlg->szDSN);
	}

	return TRUE;
}


} //namespace sos