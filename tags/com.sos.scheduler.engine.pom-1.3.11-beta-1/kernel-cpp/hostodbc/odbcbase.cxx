//$Id$
//#define MODULE_NAME "odbcbase"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include "precomp.h"

#include <stdio.h>
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/msec.h"
#include "sosodbc.h"


// hat nix mit diesem modul zu tun:

/*  WAS FEHLT:

    Info ob Schlüsselfelder beim Update geändert werden dürfen
    -> ?

    Externer Satzschlüssel (ROWID) von SAM und Std_file.

    Commit und Rollback für Leasy


    C-Datentypen:
        SQL_C_BINARY:
        SQL_C_BIT:
        SQL_C_BOOKMARK: (?)
        SQL_C_FLOAT:
        SQL_C_STINYINT:
        SQL_C_TINYINT:
        SQL_C_TIMESTAMP     Nur mit Datum implementiert, die Zeit fehlt
        SQL_C_TIME:
        SQL_C_UTINYINT:

    SQLBindParameter

    SQLBindCol
        feststehene Länge in *pcbValue zurückgegeben

    SQLColumns
        Qualifier und user := null?
        Öffnet die Datei, obwohl kein Satz gelesen wird.

    SQLGetStmtOptions

    SQLStatistics
        Qualifier und user := null?
        liefert nur eindeutige Indexfelder (im Test)
        Indexfelder werden nicht (nach Feldnamen) geordnet
        Öffnet die Datei, obwohl kein Satz gelesen wird.

    SQL-Grammatik:
        Ausdrücke

*/


using namespace std;
namespace sos {


//static char*        argv0       = "sosodbc";
//char**             _argv        = &argv0;
//int                _argc        = 1;


struct Odbc_func_entry
{
  //Sos_odbc_base::Sos_odbc_func_ptr    _func;
    const char*                         _name;
    uint4                               _count;
    uint4                               _msec;
};

// Reihenfolge muss mit den Enums in sosodbc.h übereinstimmen
static Odbc_func_entry odbc_func_table[ odbc_func_code_last ] =
{
    { "SQLAllocStmt"    , 0, 0 },
    { "SQLBindCol"      , 0, 0 },
    { "SQLBindParameter", 0, 0 },
    { "SQLColAttributes", 0, 0 },
    { "SQLColumns"      , 0, 0 },
    { "SQLConnect"      , 0, 0 },
    { "SQLDisconnect"   , 0, 0 },
    { "SQLDescribeCol"  , 0, 0 },
    { "SQLDriverConnect", 0, 0 },
    { "SQLExecDirect"   , 0, 0 },
    { "SQLExecute"      , 0, 0 },
    { "SQLFetch"        , 0, 0 },
    { "SQLFreeStmt"     , 0, 0 },
    { "SQLGetConnectOption", 0, 0 },
    { "SQLGetData"      , 0, 0 },
    { "SQLGetInfo"      , 0, 0 },
    { "SQLGetStmtOption", 0, 0 },
    { "SQLGetTypeInfo"  , 0, 0 },
    { "SQLNumResultCols", 0, 0 },
    { "SQLParamData"    , 0, 0 },
    { "SQLPutData"      , 0, 0 },
    { "SQLPrepare"      , 0, 0 },
    { "SQLRowCount"     , 0, 0 },
    { "SQLSetConnectOption", 0, 0 },
    { "SQLSetStmtOption", 0, 0 },
    { "SQLSpecialColumns",0, 0 },
    { "SQLStatistics"   , 0, 0 },
    { "SQLTables"       , 0, 0 }
};

int LoadByOrdinal;      // Für .def

Bool odbc_make_timing = false;
Sos_string timing_filename;

#if defined SYSTEM_WIN32

    static CRITICAL_SECTION critical_section;
    
    SOS_INIT( odbcbase_critical_sesion )
    {
        InitializeCriticalSection( &critical_section );
    }

#endif

//-------------------------------------------------------------------print_odbc_func_timings

void print_odbc_func_timings( ostream* s )
{
    if( !s )  return;

    char buffer [ 100 ];
    int4 total_msec = 0;

    sprintf( buffer, "%-23s\t%7s\t%13s\t%13s\t%4s\n\n",
            "function", "count", "total time", "time per call", "%" );
    *s << buffer;

    Odbc_func_entry* e;

    for( e = odbc_func_table; e < odbc_func_table + NO_OF( odbc_func_table ); e++ ) {
        total_msec += e->_msec;
    }

    for( e = odbc_func_table; e < odbc_func_table + NO_OF( odbc_func_table ); e++ ) {
        if( e->_count ) {
            sprintf( buffer, "%-23s\t%7ld\t%11ldms\t%11.3lfms\t%3.0lf%%\n",
                     e->_name, (long)e->_count, (long)e->_msec, (double)e->_msec / e->_count, 
                     100 * (double)e->_msec / total_msec );
            *s << buffer;
        }
    }

    sprintf( buffer, "%-23s\t%7s\t%11ldms\t%13s\t100%%\n",
             "total", "", (long)total_msec, "" );
    *s << buffer;
}

//-----------------------------------------------------------------------------throw_odbc_error

void throw_odbc_error( const char* sqlstate, const char* a, int b )
{
    Odbc_error x ( "SOS-ODBC" );
    x._sqlstate = sqlstate;
    x.insert( a );
    x.insert( b );
    throw x;
}

//--------------------------------------------------------------------------Sos_odbc_base::call
// Hier gehen alle ODBC-Aufruf durch, die mit DEFINE_ODBC_CALL_n definiert sind.
// Der Aufruf wird mit Parametern (hex) protokolliert ( WORD-Reihenfolge ist etwas konfus)
// Die entsprechende Methode eines ODBC-Objekts wird aufgerufen.
// Eine Exception werden abgefangen und für SQLError aufgehoben

RETCODE Sos_odbc_base::call( void* handle, ... )
{
    uint4                start_msec;
    RETCODE              rc;
    Odbc_func_code       fc = _func_code;           // wg delete this
    const Unknown_param* p = (Unknown_param*)( &handle );

#   if defined SYSTEM_WIN32
        EnterCriticalSection( &critical_section );
#   endif

    clear_error();

    {
        Log_ptr log;
        if( log )
        {
            *log << odbc_func_table[ fc ]._name << "( ";

            for( int i = 0; i < ( _param_byte_count + sizeof(Unknown_param)-1 ) / sizeof(Unknown_param); i++ )  {
                *log << p[i] << ' ';
            }
            *log << ")\n" << flush;
        }
    }
    //sos_static_ptr()->_log_indent++; // BC 4.53: Schneller als LOGI
    Log_indent _indent_;

    if( odbc_make_timing )  start_msec = elapsed_msec();
    if( _active )  { rc = sql_error( "S1000", "SOSODBC-10" ); goto RETURN; }
    _active = true;

    try {
        // Sos_odbc_conn::SQLFreeConnect und Sos_odbc_stmt::SQLFreeStmt
        // entfernen sich selbst mit delete this!
        rc = (_func_ptr)( p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],p[10],p[11],p[12],p[13] );
    }
    catch( const Odbc_error& x )
    {
        rc = sql_error( c_str( x._sqlstate ), x );
    }
    catch( const No_memory_error& x )
    {
        rc = sql_error( "S1001", x );  // "Communication link failure"
    }
  //catch( const Eof_error& x )
  //{
  //    //sql_error( "S1000", x );
  //    rc = SQL_NO_DATA_FOUND;
  //}
    catch( const Connection_lost_error& x )
    {
        rc = sql_error( "08S01", x ); // "Communication link failure"
    }
  //catch( const Timeout_error& x )
  //{
  //    rc = sql_error( "S1T00", x ); // "Timeout expired"
  //}
    catch( const Overflow_error& x )
    {
        rc = sql_error( "22003", x ); // "Numeric value out of range"
    }
    catch( const Syntax_error& x )
    {
        rc = sql_error( "37000", x ); // "Syntax error or access violation"
    }
    catch( const Xc_base& x )
    {
        rc = sql_error( "S1000", x ); // "General error"
    }
    catch( const exception& xm )
    {
        rc = sql_error( "S1000", xm ); // "General error"
    }
    catch( const char* text )
    {
        Sos_string msg = "(const char*) ";
        msg += text;
        rc = sql_error( "S1000", c_str(msg) ); // "General error"
    }
/*
    catch( ... )
    {
        rc = sql_error( "S1000", "UNKNOWN" ); // "General error"
    }
*/

    if( odbc_make_timing ) {
        odbc_func_table[ fc ]._count++;
        odbc_func_table[ fc ]._msec += elapsed_msec() - start_msec;
    }

    _active = false;

  RETURN:
    if( rc != SQL_SUCCESS ) {
        ODBC_LOG( "retcode=" << rc << ", SqlState=" << _sqlstate << '\n' );
    }

    //sos_static_ptr()->_log_indent--;

#   if defined SYSTEM_WIN32
        LeaveCriticalSection( &critical_section );
#   endif

    return rc;
}

//---------------------------------------------------------------------Sos_odbc_base::delete_xc

void Sos_odbc_base::delete_xc()
{
    SOS_DELETE( _xc );
}

//-------------------------------------------------------------------Sos_odbc_base::clear_error

void Sos_odbc_base::clear_error()
{
    if( _xc )  delete_xc();
    _sqlstate = "00000";
}

//---------------------------------------------------------------------Sos_odbc_base::sql_error

RETCODE Sos_odbc_base::sql_error( const char* sqlstate )
{
    delete_xc();
    _sqlstate = sqlstate;
    return SQL_ERROR;
}

//---------------------------------------------------------------------Sos_odbc_base::sql_error

RETCODE Sos_odbc_base::sql_error( const char* sqlstate, const Xc_base& x )
{
    sql_error( sqlstate );
    _xc = new Xc_base( x );      // No_memory_error?
    return SQL_ERROR;
}

//---------------------------------------------------------------------Sos_odbc_base::sql_error

RETCODE Sos_odbc_base::sql_error( const char* sqlstate, const exception& x )
{
    return sql_error( sqlstate, Xc( x ) );
}

//---------------------------------------------------------------------Sos_odbc_base::sql_error

RETCODE Sos_odbc_base::sql_error( const char* sqlstate, const char* e )
{
    return sql_error( sqlstate, Xc( e ) );
}

//-------------------------------------------------------------------------------------SQLError

extern "C"
RETCODE SQL_API SQLError(
    HENV       henv,
    HDBC       hdbc,
	HSTMT	   hstmt,
	UCHAR  FAR *szSqlState,
	SDWORD FAR *pfNativeError,
	UCHAR  FAR *szErrorMsg,
	SWORD	   cbErrorMsgMax,
	SWORD  FAR *pcbErrorMsg)
{
    try {
        ODBC_LOG( "SQLError " );

        if( pcbErrorMsg )  *pcbErrorMsg = 0;
        if( pfNativeError )  *pfNativeError = 0;        // ?

        Xc_base*       x = 0;
        Sos_odbc_base* odbc_obj = hstmt != SQL_NULL_HSTMT? (Sos_odbc_base*)hstmt
                                : hdbc  != SQL_NULL_HDBC?  (Sos_odbc_base*)hdbc
                                : henv  != SQL_NULL_HENV?  (Sos_odbc_base*)henv : 0;

        if( !odbc_obj      )  { ODBC_LOG( "return SQL_ERROR\n" );  return SQL_ERROR; }
        if( !odbc_obj->_xc )  { ODBC_LOG( "return SQL_NO_DATA_FOUND\n" ); return SQL_NO_DATA_FOUND; }

        ODBC_LOG( "SqlState=" << odbc_obj->_sqlstate );
        if( szSqlState ) {
            memcpy( szSqlState, odbc_obj->_sqlstate.ptr(), 5 );
#           if !defined SYSTEM_WIN
                szSqlState[5] = '\0';    // Für unixODBC ist das besser. Vielleicht auch für Windows? jz 23.7.01
#           endif
        }
        odbc_obj->_sqlstate = "00000";

        x = odbc_obj->_xc;
        odbc_obj->_xc = 0;

        if( szErrorMsg  &&  cbErrorMsgMax ) {
            const char msg_prefix[] = "[" VENDOR_FULL_NAME "][" DRIVER_FULL_NAME "]";
            int l = min( strlen( msg_prefix ), (uint)cbErrorMsgMax );
            memcpy( szErrorMsg, msg_prefix, l );
            Area buffer ( szErrorMsg + l, cbErrorMsgMax - l );
            x->get_text( &buffer );
            ODBC_LOG( ' ' << buffer << '\n' );
          //if( pcbErrorMsg )  *pcbErrorMsg = l + strlen( buffer.char_ptr() );
            if( pcbErrorMsg )  *pcbErrorMsg = l + buffer.length();
        }
        else ODBC_LOG( '\n' );

        delete x;
        return SQL_SUCCESS;
    }
    catch( ... )
    {
        return SQL_ERROR;
    }
}

} //namespace sos
