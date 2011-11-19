//#define MODULE_NAME "odbcenv"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif


#include <sql.h>
#include <sqlext.h>
#include "precomp.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "sosodbc.h"

#if defined __WIN32__
#   include <stdarg.h>
#   include <wingdi.h>
#   include <winbase.h>
#   include <winuser.h>        // OutputDebugString
#endif

using namespace std;
namespace sos {

//----------------------------------------------------------------------------------SQLAllocEnv

extern "C"
RETCODE SQL_API SQLAllocEnv( HENV FAR *phenv )
{
#   if defined SYSTEM_WIN
        OutputDebugString( "*** hostODBC SQLAllocEnv()\n" );
#   endif

    try {
        sos_static_ptr()->add_ref();    // alt: in SQLConnect, weil add_remove() in SQLDisconnect ist
 
        //try {
            Sos_string log_filename = read_profile_string( "", "sosodbc", "log" );
            if( !empty( log_filename ) )   log_start( log_filename == "*dbwin"? "" : c_str( log_filename ) );
            //else log_start();
    
            timing_filename = read_profile_string( "", "sosodbc", "timing" );
            odbc_make_timing = !empty( timing_filename );
        //}
        //catch( const Xc& ) {}
    
        ODBC_LOG( "SQLAllocEnv()\n" );
        LOG( "sos_static_ptr()->_ref_count=" << sos_static_ptr()->_ref_count << '\n' );

        Sos_odbc_env* env = new Sos_odbc_env;
        *phenv = (HENV*)env;
    }
    catch( const Xc& x )
    {
        SHOW_MSG( "hostODBC: Fehler bei der Initialisierung in SQLAllocEnv():\n" << x );
        return SQL_ERROR;
    }

#   if defined SYSTEM_WIN
        if( read_profile_bool( "", "sosodbc", "debugbreak", false ) )  {
            //show_msg( "Wegen der Einstellung DebugBreak=yes im Abschnitt [sosodbc]\n"
            //          "der Datei sos.ini wird hostODBC gleich unterbrochen." );
            int rc = MessageBox( NULL,  "Wegen der Einstellung DebugBreak=yes im Abschnitt [sosodbc]\n"
                                        "der Datei sos.ini wird hostODBC gleich unterbrochen.\n"
                                        "Einverstanden?",
                                 "hostODBC", 
                                 MB_TASKMODAL | MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
            if( rc == IDYES )  DebugBreak();
        }
#   endif

	return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------------SQLFreeEnv

extern "C"
RETCODE SQL_API SQLFreeEnv( HENV henv )
{
    ODBC_LOG( "SQLFreeEnv()\n" );
    //sos_static_ptr()->_log_indent++;
    Log_indent _indent_;

    if( odbc_make_timing )  {
        ofstream f ( c_str( timing_filename ) );
        print_odbc_func_timings( &f );
    }

    LOG( "sos_static_ptr()->_ref_count=" << sos_static_ptr()->_ref_count << '\n' );
    sos_static_ptr()->remove_ref();

#   if defined SYSTEM_WIN
        OutputDebugString( "hostODBC: SQLFreeEnv ok\n" );
#   endif

    //sos_static_ptr()->_log_indent--;
	return SQL_SUCCESS;
}

//-------------------------------------------------------------------------------odbc_as_string

Sos_string odbc_as_string( const void* ptr, SWORD len )
{
    if( ptr ) return as_string( (const char*)ptr,
                                len == SQL_NTS? strlen( (const char*)ptr ) : len );
        else  return empty_string;
}


} //namespace sos
