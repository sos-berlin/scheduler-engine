#define MODULE_NAME "odbcinst"
#define COPYRIGHT   "©1997 SOS GmbH Berlin"
#define AUTHOR      "Jörg Schwiemann"

// ODBCINST-SQL-Zugriffe

// Datum: 14. 9.1994
// Stand:  4.12.1995

#include <precomp.h>
#include <stdio.h>
#include <sysdep.h>

#if defined SYSTEM_WIN       // gesamtes odbcinst.cxx

#include <string.h>

#if defined SYSTEM_MICROSOFT
#   error
#   include <afx.h>
#else
#   define STRICT
#   include <windows.h>
#endif

// ODBCINST-Interface
extern "C" {
#   include <sqlext.h>
#   include <sql.h>
#   include <odbcinst.h>
}

#include <sosstrng.h>
#include <sos.h>

#include <sosprof.h>
#include <sosopt.h>
#include <sosarray.h>
#include <stdfield.h>
#include <log.h>
#include <sosdate.h>
#include <absfile.h>
#include <sosdll.h>
#include <sosdb.h>
#include <odbctype.h>

// ----------------------------------------------------------------------------- Odbcinst_functions

struct Odbcinst_functions : Sos_self_deleting, Sos_dll
{
                                DECLARE_AUTO_LOADING_DLL_PROC_4(    BOOL INSTAPI, SQLConfigDataSource,
                                                                    HWND       /*hwndParent*/,
                                                                    WORD       /*fRequest*/,
                                                                    LPCSTR     /*lpszDriver*/,
                                                                    LPCSTR     /*lpszAttributes*/ );
};

#define ODBCINST_LIB _odbcinst_static._lib.


//----------------------------------------------------------------------------------Odbcinst_static

struct Odbcinst_static
{
                                Odbcinst_static          ();
                               ~Odbcinst_static          ();

    Odbcinst_static*            static_ptr               ()                                     { return this; }
    void                        init();

    Odbcinst_functions          _lib;
};

static Odbcinst_static _odbcinst_static;

//---------------------------------------------------------------------Odbcinst_static::Odbcinst_static

Odbcinst_static::Odbcinst_static()
{
}

//---------------------------------------------------------------------Odbc_static::~Odbc_static

Odbcinst_static::~Odbcinst_static()
{
}

//---------------------------------------------------------------------Odbcinst_static::Odbcinst_static

void Odbcinst_static::init()
{
#   if defined SYSTEM_WIN16
        _lib.init( "ODBCINST.DLL" );
#   else
#error
        _lib.init( "ODBCINST32.DLL" );
#   endif
}


Bool repair_access_db( HWND parent, const Sos_string& pfad )
{
    _odbcinst_static.init();
    Sos_string attr = "REPAIR_DB=";
    attr += pfad;
    BOOL b = ODBCINST_LIB SQLConfigDataSource( parent, ODBC_CONFIG_DSN, "ODBCJT16.DLL", c_str(attr) );
    return b ? true : false;
}


#endif // defined SYSTEM_WIN
