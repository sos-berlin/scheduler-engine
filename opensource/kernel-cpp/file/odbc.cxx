// $Id: odbc.cxx 13471 2008-03-31 16:38:12Z jz $
// §927


// ODBC-SQL-Zugriffe

// Datum: 14. 9.1994

#include "precomp.h"
#include <stdio.h>
#include "../kram/sysdep.h"

#if defined SYSTEM_ODBC

#ifdef SYSTEM_WIN
#    include <odbcinst.h>
#endif

#include <string.h>

#if defined SYSTEM_WIN
#   include <windows.h>
#   if _MSC_VER < 1100  // vor VC++ 5.00?
#       define INCLUDE_ODBC_AS_C
#   endif
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#endif

#if defined SYSTEM_UNIX
#   include <stdio.h>               // fileno
#   include <unistd.h>              // read(), write(), close()
#   include <dlfcn.h>     // dlopen()
#   include <errno.h>
#   include <unistd.h>
#endif

// ODBC-Interface
#ifdef INCLUDE_ODBC_AS_C
    extern "C" {
#       include <sqlext.h>
#       include <sql.h>
    }
#else
#   include <sqlext.h>
#   include <sql.h>
#endif

#include "../kram/sosstrng.h"
#include "../kram/sos.h"

#include "../kram/sosdll.h"

#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sosarray.h"
#include "../kram/stdfield.h"
#include "../kram/log.h"
#include "../kram/sosdate.h"
#include "../file/absfile.h"
#include "../file/sosdb.h"
#include "../file/odbctype.h"
#include "../kram/env.h"

using zschimmer::lcase;
using zschimmer::set_split;
using zschimmer::has_regex;
using zschimmer::ucase;
using zschimmer::S;
using zschimmer::string_begins_with;

namespace sos {


#define LOG_AND_IGNORE_XC( rc, fn ) try { check_error( rc, fn ); } catch ( const Xc& x ) {}

Sos_string extension_of_path( const Sos_string& path );  // sysdep.cxx

//----------------------------------------------------------------------------------------const

const uint _odbc_default_max_length = 0;

//---------------------------------------------------------------------------------------static

static Library_handle                   odbc32_dll_handle ;
static void*                            oracle_module           = false;

//------------------------------------------------------------------------------------SQLServer
// Aus \sql\devtools\include\odbcss.h (CD SQLServer):

#define SQL_SOPT_SS_BASE				1225

#define SQL_SOPT_SS_CURSOR_OPTIONS		(SQL_SOPT_SS_BASE+5) // Server cursor options

//	Defines for use with SQL_SOPT_SS_CURSOR_OPTIONS
#define SQL_CO_OFF		0L			//	Clear all cursor options
#define SQL_CO_FFO		1L			//	Fast-forward cursor will be used
#define SQL_CO_AF		2L			//	Autofetch on cursor open
#define SQL_CO_FFO_AF	(SQL_CO_FFO|SQL_CO_AF)	//	Fast-forward cursor with autofetch
#define SQL_CO_DEFAULT	SQL_CO_OFF

//---------------------------------------------------------------------------------Oracle 8.0.5

const int sql_oracle_805_blob = -402;

// -------------------------------------------------------------------- odbc_default_max_length
/*
void odbc_default_max_length( uint len )
{
    _odbc_default_max_length = len;
}
*/
// ----------------------------------------------------------------------------- Odbc_functions


struct Odbc_functions : Sos_self_deleting, Sos_dll
{
/* Core Function Prototypes */


                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLAllocConnect,
                                                                    HENV         /* henv */ ,
                                                                    HDBC       * /* phdbc */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLAllocEnv,
                                                                    HENV       * /* phenv */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLAllocStmt,
                                                                    HDBC         /* hdbc */ ,
                                                                    HSTMT      * /* phstmt */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_6(    RETCODE, SQLBindCol,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* icol */ ,
                                                                    SWORD        /* fCType */ ,
                                                                    PTR          /* rgbValue */ ,
                                                                    SDWORD       /* cbValueMax */ ,
                                                                    SDWORD     * /* pcbValue */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLCancel,
//                                                                  HSTMT        /* hstmt */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_7(    RETCODE, SQLColAttributes,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* icol */ ,
                                                                    UWORD        /* fDescType */ ,
                                                                    PTR          /* rgbDesc */ ,
                                                                    SWORD        /* cbDescMax */ ,
                                                                    SWORD      * /* pcbDesc */ ,
                                                                    SDWORD     * /* pfDesc */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_7(    RETCODE, SQLConnect,
                                                                    HDBC         /* hdbc */ ,
                                                                    UCHAR      * /* szDSN */ ,
                                                                    SWORD        /* cbDSN */ ,
                                                                    UCHAR      * /* szUID */ ,
                                                                    SWORD        /* cbUID */ ,
                                                                    UCHAR      * /* szAuthStr */ ,
                                                                    SWORD        /* cbAuthStr */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_9(    RETCODE, SQLDescribeCol,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* icol */ ,
                                                                    UCHAR      * /* szColName */ ,
                                                                    SWORD        /* cbColNameMax */ ,
                                                                    SWORD      * /* pcbColName */ ,
                                                                    SWORD      * /* pfSqlType */ ,
                                                                    UDWORD     * /* pcbColDef */ ,
                                                                    SWORD      * /* pibScale */ ,
                                                                    SWORD      * /* pfNullable */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLDisconnect,
                                                                    HDBC         /* hdbc */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_8(    RETCODE, SQLError,
                                                                    HENV         /* henv */ ,
                                                                    HDBC         /* hdbc */ ,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UCHAR      * /* szSqlState */ ,
                                                                    SDWORD     * /* pfNativeError */ ,
                                                                    UCHAR      * /* szErrorMsg */ ,
                                                                    SWORD        /* cbErrorMsgMax */ ,
                                                                    SWORD      * /* pcbErrorMsg */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLExecDirect,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UCHAR      * /* szSqlStr */ ,
                                                                    SDWORD       /* cbSqlStr */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLExecute,
                                                                    HSTMT        /* hstmt */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLFetch,
                                                                    HSTMT        /* hstmt */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLFreeConnect,
                                                                    HDBC         /* hdbc */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLFreeHandle,
                                                                    SQLSMALLINT,
                                                                    HDBC         /* hdbc */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLFreeEnv,
                                                                    HENV         /* henv */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLFreeStmt,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* fOption */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_4(    RETCODE, SQLGetCursorName,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UCHAR      * /* szCursor */ ,
                                                                    SWORD        /* cbCursorMax */ ,
                                                                    SWORD      * /* pcbCursor */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLNumResultCols,
                                                                    HSTMT        /* hstmt */ ,
                                                                    SWORD      * /* pccol */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLPrepare,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UCHAR      * /* szSqlStr */ ,
                                                                    SDWORD       /* cbSqlStr */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLRowCount,
                                                                    HSTMT        /* hstmt */ ,
                                                                    SDWORD     * /* pcrow */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLSetCursorName,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szCursor */ ,
//                                                                  SWORD        /* cbCursor */ );
//
                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLTransact,
                                                                    HENV         /* henv */ ,
                                                                    HDBC         /* hdbc */ ,
                                                                    UWORD        /* fType */ );

/* Level 1 Prototypes */
                                DECLARE_AUTO_LOADING_DLL_PROC_9(    RETCODE, SQLColumns,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UCHAR      * /* szTableQualifier */ ,
                                                                    SWORD        /* cbTableQualifier */ ,
                                                                    UCHAR      * /* szTableOwner */ ,
                                                                    SWORD        /* cbTableOwner */ ,
                                                                    UCHAR      * /* szTableName */ ,
                                                                    SWORD        /* cbTableName */ ,
                                                                    UCHAR      * /* szColumnName */ ,
                                                                    SWORD        /* cbColumnName */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_8(    RETCODE, SQLDriverConnect,
                                                                    HDBC         /* hdbc */ ,
                                                                    HWND         /* hwnd */ ,
                                                                    UCHAR      * /* szConnStrIn */ ,
                                                                    SWORD        /* cbConnStrIn */ ,
                                                                    UCHAR      * /* szConnStrOut */ ,
                                                                    SWORD        /* cbConnStrOutMax */ ,
                                                                    SWORD      * /* pcbConnStrOut */ ,
                                                                    UWORD        /* fDriverCompletion */ );


                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLGetConnectOption,
                                                                    HDBC         /* hdbc */ ,
                                                                    UWORD        /* fOption */ ,
                                                                    PTR          /* pvParam */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_6(    RETCODE, SQLGetData,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* icol */ ,
                                                                    SWORD        /* fCType */ ,
                                                                    PTR          /* rgbValue */ ,
                                                                    SDWORD       /* cbValueMax */ ,
                                                                    SDWORD     * /* pcbValue */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLGetFunctions,
                                                                    HDBC         /* hdbc */ ,
                                                                    UWORD        /* fFunction */ ,
                                                                    UWORD      * /* pfExists */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_5(    RETCODE, SQLGetInfo,
                                                                    HDBC         /* hdbc */ ,
                                                                    UWORD        /* fInfoType */ ,
                                                                    PTR          /* rgbInfoValue */ ,
                                                                    SWORD        /* cbInfoValueMax */ ,
                                                                    SWORD      * /* pcbInfoValue */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLGetStmtOption,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* fOption */ ,
                                                                    PTR          /* pvParam */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLGetTypeInfo,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  SWORD        /* fSqlType */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLParamData,
                                                                    HSTMT        /* hstmt */ ,
                                                                    PTR        * /* prgbValue */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLPutData,
                                                                    HSTMT        /* hstmt */ ,
                                                                    PTR          /* rgbValue */ ,
                                                                    SDWORD       /* cbValue */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLSetConnectOption,
                                                                    HDBC         /* hdbc */ ,
                                                                    UWORD        /* fOption */ ,
                                                                    UDWORD       /* vParam */ );

                                DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLSetStmtOption,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* fOption */ ,
                                                                    UDWORD       /* vParam */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC10(    RETCODE, SQLSpecialColumns,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UWORD        /* fColType */ ,
//                                                                  UCHAR      * /* szTableQualifier */ ,
//                                                                  SWORD        /* cbTableQualifier */ ,
//                                                                  UCHAR      * /* szTableOwner */ ,
//                                                                  SWORD        /* cbTableOwner */ ,
//                                                                  UCHAR      * /* szTableName */ ,
//                                                                  SWORD        /* cbTableName */ ,
//                                                                  UWORD        /* fScope */ ,
//                                                                  UWORD        /* fNullable */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_9(    RETCODE, SQLStatistics,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szTableQualifier */ ,
//                                                                  SWORD        /* cbTableQualifier */ ,
//                                                                  UCHAR      * /* szTableOwner */ ,
//                                                                  SWORD        /* cbTableOwner */ ,
//                                                                  UCHAR      * /* szTableName */ ,
//                                                                  SWORD        /* cbTableName */ ,
//                                                                  UWORD        /* fUnique */ ,
//                                                                  UWORD        /* fAccuracy */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_9(    RETCODE, SQLTables,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szTableQualifier */ ,
//                                                                  SWORD        /* cbTableQualifier */ ,
//                                                                  UCHAR      * /* szTableOwner */ ,
//                                                                  SWORD        /* cbTableOwner */ ,
//                                                                  UCHAR      * /* szTableName */ ,
//                                                                  SWORD        /* cbTableName */ ,
//                                                                  UCHAR      * /* szTableType */ ,
//                                                                  SWORD        /* cbTableType */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_6(    RETCODE, SQLBrowseConnect,
//                                                                  HDBC         /* hdbc */ ,
//                                                                  UCHAR      * /* szConnStrIn */ ,
//                                                                  SWORD        /* cbConnStrIn */ ,
//                                                                  UCHAR      * /* szConnStrOut */ ,
//                                                                  SWORD        /* cbConnStrOutMax */ ,
//                                                                  SWORD      * /* pcbConnStrOut */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_9(    RETCODE, SQLColumnPrivileges,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szTableQualifier */ ,
//                                                                  SWORD        /* cbTableQualifier */ ,
//                                                                  UCHAR      * /* szTableOwner */ ,
//                                                                  SWORD        /* cbTableOwner */ ,
//                                                                  UCHAR      * /* szTableName */ ,
//                                                                  SWORD        /* cbTableName */ ,
//                                                                  UCHAR      * /* szColumnName */ ,
//                                                                  SWORD        /* cbColumnName */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_8(    RETCODE, SQLDataSources,
//                                                                  HENV         /* henv */ ,
//                                                                  UWORD        /* fDirection */ ,
//                                                                  UCHAR      * /* szDSN */ ,
//                                                                  SWORD        /* cbDSNMax */ ,
//                                                                  SWORD      * /* pcbDSN */ ,
//                                                                  UCHAR      * /* szDescription */ ,
//                                                                  SWORD        /* cbDescriptionMax */ ,
//                                                                  SWORD      * /* pcbDescription */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_6(    RETCODE, SQLDescribeParam,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UWORD        /* ipar */ ,
//                                                                  SWORD      * /* pfSqlType */ ,
//                                                                  UDWORD     * /* pcbColDef */ ,
//                                                                  SWORD      * /* pibScale */ ,
//                                                                  SWORD      * /* pfNullable */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_5(    RETCODE, SQLExtendedFetch,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UWORD        /* fFetchType */ ,
//                                                                  SDWORD       /* irow */ ,
//                                                                  UDWORD     * /* pcrow */ ,
//                                                                  UWORD      * /* rgfRowStatus */ );


//                              DECLARE_AUTO_LOADING_DLL_PROC13(    RETCODE, SQLForeignKeys,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szPkTableQualifier */ ,
//                                                                  SWORD        /* cbPkTableQualifier */ ,
//                                                                  UCHAR      * /* szPkTableOwner */ ,
//                                                                  SWORD        /* cbPkTableOwner */ ,
//                                                                  UCHAR      * /* szPkTableName */ ,
//                                                                  SWORD        /* cbPkTableName */ ,
//                                                                  UCHAR      * /* szFkTableQualifier */ ,
//                                                                  SWORD        /* cbFkTableQualifier */ ,
//                                                                  UCHAR      * /* szFkTableOwner */ ,
//                                                                  SWORD        /* cbFkTableOwner */ ,
//                                                                  UCHAR      * /* szFkTableName */ ,
//                                                                  SWORD        /* cbFkTableName */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_1(    RETCODE, SQLMoreResults,
//                                                                  HSTMT        /* hstmt */ );



//                              DECLARE_AUTO_LOADING_DLL_PROC_6(    RETCODE, SQLNativeSql,
//                                                                  HDBC         /* hdbc */ ,
//                                                                  UCHAR      * /* szSqlStrIn */ ,
//                                                                  SDWORD       /* cbSqlStrIn */ ,
//                                                                  UCHAR      * /* szSqlStr */ ,
//                                                                  SDWORD       /* cbSqlStrMax */ ,
//                                                                  SDWORD     * /* pcbSqlStr */ );


//                              DECLARE_AUTO_LOADING_DLL_PROC_2(    RETCODE, SQLNumParams,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  SWORD      * /* pcpar */ );


//                              DECLARE_AUTO_LOADING_DLL_PROC_3(    RETCODE, SQLParamOptions,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UDWORD       /* crow */ ,
//                                                                  UDWORD     * /* pirow */ );


//                              DECLARE_AUTO_LOADING_DLL_PROC_7(    RETCODE, SQLPrimaryKeys,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szTableQualifier */ ,
//                                                                  SWORD        /* cbTableQualifier */ ,
//                                                                  UCHAR      * /* szTableOwner */ ,
//                                                                  SWORD        /* cbTableOwner */ ,
//                                                                  UCHAR      * /* szTableName */ ,
//                                                                  SWORD        /* cbTableName */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_9(    RETCODE, SQLProcedureColumns,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szProcQualifier */ ,
//                                                                  SWORD        /* cbProcQualifier */ ,
//                                                                  UCHAR      * /* szProcOwner */ ,
//                                                                  SWORD        /* cbProcOwner */ ,
//                                                                  UCHAR      * /* szProcName */ ,
//                                                                  SWORD        /* cbProcName */ ,
//                                                                  UCHAR      * /* szColumnName */ ,
//                                                                  SWORD        /* cbColumnName */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_7(    RETCODE, SQLProcedures,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szProcQualifier */ ,
//                                                                  SWORD        /* cbProcQualifier */ ,
//                                                                  UCHAR      * /* szProcOwner */ ,
//                                                                  SWORD        /* cbProcOwner */ ,
//                                                                  UCHAR      * /* szProcName */ ,
//                                                                  SWORD        /* cbProcName */ );


//                              DECLARE_AUTO_LOADING_DLL_PROC_4(    RETCODE, SQLSetPos,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UWORD        /* irow */ ,
//                                                                  UWORD        /* fOption */ ,
//                                                                  UWORD        /* fLock */ );

//                              DECLARE_AUTO_LOADING_DLL_PROC_7(    RETCODE, SQLTablePrivileges,
//                                                                  HSTMT        /* hstmt */ ,
//                                                                  UCHAR      * /* szTableQualifier */ ,
//                                                                  SWORD        /* cbTableQualifier */ ,
//                                                                  UCHAR      * /* szTableOwner */ ,
//                                                                  SWORD        /* cbTableOwner */ ,
//                                                                  UCHAR      * /* szTableName */ ,
//                                                                  SWORD        /* cbTableName */ );

/* SDK 2.0 Additions */


#if (ODBCVER >= 0x0200)

//                              DECLARE_AUTO_LOADING_DLL_PROC_8(    RETCODE, SQLDrivers,
//                                                                  HENV         /* henv */ ,
//                                                                  UWORD        /* fDirection */ ,
//                                                                  UCHAR      * /* szDriverDesc */ ,
//                                                                  SWORD        /* cbDriverDescMax */ ,
//                                                                  SWORD      * /* pcbDriverDesc */ ,
//                                                                  UCHAR      * /* szDriverAttributes */ ,
//                                                                  SWORD        /* cbDrvrAttrMax */ ,
//                                                                  SWORD      * /* pcbDrvrAttr */ );


                                DECLARE_AUTO_LOADING_DLL_PROC10(    RETCODE, SQLBindParameter,
                                                                    HSTMT        /* hstmt */ ,
                                                                    UWORD        /* ipar */ ,
                                                                    SWORD        /* fParamType */ ,
                                                                    SWORD        /* fCType */ ,
                                                                    SWORD        /* fSqlType */ ,
                                                                    UDWORD       /* cbColDef */ ,
                                                                    SWORD        /* ibScale */ ,
                                                                    PTR          /* rgbValue */ ,
                                                                    SDWORD       /* cbValueMax */ ,
                                                                    SDWORD     * /* pcbValue */ );
#endif

};



#define ODBC_LIB static_ptr()->_lib.


static void remove_errors( HENV, HDBC, HSTMT, bool debug );

struct Odbc_static;

//------------------------------------------------------------------------------------Odbc_stmt
/*
struct Odbc_stmt, z::Non_clonable
{

    operator HSTMT () { return _handle; }
    HSTMT                      _handle;
};
*/
//---------------------------------------------------------------------------------Odbc_session

struct Odbc_session : Sos_database_session
{
                                Odbc_session            ();
                               ~Odbc_session            ();

    void                       _open                    ( Sos_database_file* );
    void                       _close                   ( Close_mode = close_normal );
    void                       _execute_direct          ( const Const_area& );
    RETCODE                     execute_direct_without_error_check( const Const_area& );
    void                       _commit                  ();
    void                       _rollback                ();
    Sos_string                  name                    ()      { return "odbc"; }
    void                        remove_errors           ()      { sos::remove_errors( NULL, _hdbc, NULL, _debug ); }

    void                       _open_postprocessing     ();

    virtual Bool               _equal_session           ( Sos_database_file* );
  //virtual string              translate               ( const string& word );

    Odbc_static*                static_ptr              ()      { return (Odbc_static*)_static; }

    void                       _obj_print               ( ostream* ) const;

    Fill_zero                  _zero_;
    HDBC                       _hdbc;
    bool                       _hdbc_outside;
    bool                       _connected;
    HSTMT                      _hstmt;                  // Für _execute_direct()
    int                        _odbc_version;           // 0x0100, 0x0200, ...
    string                     _sql_driver_ver;
    Bool                       _auto_commit;
    Bool                       _sqltransact;            // SQLTransact() definiert?
    Bool                       _sql_need_long_data_len; 
    Sos_string                 _driver_name;
    Sos_string                 _dbms_ver;
    bool                       _use_default_resultset;
    UWORD                      _function_array [ 100 ];
};

//----------------------------------------------------------------------------------Odbc_static

struct Odbc_static : Sos_database_static
{
                                Odbc_static              ();
                               ~Odbc_static              ();

    void                        alloc_env                ();
    Odbc_static*                static_ptr               ()                                     { return this; }

    HENV                       _henv;
    Odbc_functions             _lib;
};

//------------------------------------------------------------------------------------Odbc_file

struct Odbc_file : Sos_database_file
{
                                Odbc_file               ();
                               ~Odbc_file               ();

    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode  );
    void                        bind_parameters         ( const Record_type*, const Byte* );

    virtual void                put_record              ( const Const_area&  );
            void                put_blob_record         ( const Const_area&  );
            void                put_blob_file           ();
    virtual void                get_record              ( Area&              );

    void                        remove_errors           ()      { sos::remove_errors( NULL, NULL, _hstmt, _debug ); }


    virtual void               _create_static           ();
    
    virtual Sos_ptr<Sos_database_session> 
                               _create_session          ();

  private:
    void                        prepare_stmt            ( const Sos_string& );
    void                        describe_columns        ();
    void                        bind_columns            ();
    void                        bind_blob_parameter     ();
    void                        execute_stmt            ();

    SQLSMALLINT                 get_blob_sqltype        ();
    SQLSMALLINT                 get_blob_sqltype2       ( HSTMT );
    SQLSMALLINT                 ctype_from_sqltype      ( SQLSMALLINT );
    Odbc_static*                static_ptr              ()   { return (Odbc_static*)+_static; }
    Odbc_session*               session                 ()   { return (Odbc_session*)+_session; }
    void                        check_error             ( RETCODE, const char* function );
    string                      driver_name_from_filename( const string& );


    friend struct               Odbc_session;

    Fill_zero                  _zero_;
    HSTMT                      _hstmt; // für Selects
    Sos_ptr<Record_type>       _type;
    Dynamic_area               _record;
    Sos_simple_array<SDWORD>   _cbValue_array;

    Sos_simple_array<Sos_odbc_binding> _param_bindings;
    Sos_ptr<Record_type>       _param_type;             // Parametertypen des Aufrufers
    const Byte*                _param_base;             // Basisadresse für Parameter des AUfrufers
    Sos_ptr<Record_type>       _odbc_param_type;        // Parametertypen für ODBC
    Dynamic_area               _odbc_param_record;      // Parameterwerte für ODBC

    Sos_simple_array<Sos_odbc_binding> _result_bindings;
    SDWORD                     _column_count;
    Bool                       _stmt_prepared;
    Bool                       _stmt_executed;
    Bool                       _field_as_file;          // Öffnet ein (langes) Feld als Datei
    int4                       _long_data_length;       // Memo-Länge für _field_as_file (sofern der ODBC-Treiber dies verlangt: sql_need_long_data_len)
    uint                       _fixed_length;           // für _field_as_file
    uint                       _max_length;             // Sehr große Felder werden auf diese Größe beschränkt (wenn > 0)
    Bool                       _eof;                    // für _field_as_file
    Bool                       _sql_need_data;          // für _field_as_file
    Bool                       _sqlputdata_called; 
    SDWORD                     _cbValue;                // für SQLBindParameter / SQLPutData
    Bool                       _need_result_set;        // Für Jet
    Bool                       _has_result_set;
    Bool                       _commit;
    Sos_string                 _connection_string;
    HDBC                       _hdbc;
    Sos_string                 _identifier_quote;
    Bool                       _identifier_quote_set;
    Bool                       _info;
    uint                       _min_length;
    uint4                      _blob_data_count;
    Bool                       _single;                 // Für Blobzugriffe als Getkey-Zugriff um Not_found_error melden zu können
    int                        _assert_row_count;       // Ersetzt -single
    Sos_string                 _reset_statement;        // Für Blobzugriffe, um beim Schreiben von 0 Bytes das Feld auf NULL zu setzen
    Bool                       _buffered_blob;          // u.a. für Microsoft SQL Server: Blobs werden zwischegepuffert
    int                        _blob_file;
    uint4                      _blob_filesize;          // Dateigrösse in Bytes (Redundanz mit _blob_data_count)
    bool                       _use_default_resultset;
    bool                       _create;                 // -create: Dateidatenbank anlegen 
    string                     _db_filename;
    string                     _driver_name;
    string                     _table_name;
    string                     _blob;
    SQLSMALLINT                _blob_ctype;
};

//-------------------------------------------------------------------------------Odbc_file_type

struct Odbc_file_type : Abs_file_type
{
    Odbc_file_type() : Abs_file_type() {};

    virtual const char*         name                    () const { return "odbc"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Odbc_file> f = SOS_NEW( Odbc_file );
        return +f;
    }
};

const Odbc_file_type  _odbc_file_type;
const Abs_file_type&  odbc_file_type = odbc_file_type;

//-----------------------------------------------------------------------------------static_ptr

inline Odbc_static* static_ptr()
{
    return sos_static_ptr()->_odbcfile;
}

//----------------------------------------------------------------------------retcode_as_string

static Sos_string retcode_as_string( RETCODE retcode )
{
    const char* p;
    switch( retcode ) {
        case SQL_SUCCESS_WITH_INFO: p = "SQL_SUCCESS_WITH_INFO"; break;
        case SQL_NO_DATA_FOUND    : p = "SQL_NO_DATA_FOUND"    ; break;
        case SQL_ERROR            : p = "SQL_ERROR"            ; break;
        case SQL_INVALID_HANDLE   : p = "SQL_INVALID_HANDLE"   ; break;
        case SQL_NEED_DATA        : p = "SQL_NEED_DATA"        ; break;
        case SQL_STILL_EXECUTING  : p = "SQL_STILL_EXECUTING"  ; break;
                           default: p = NULL;
    }

    if( p )  return p;
       
    return as_string( retcode );
}

//----------------------------------------------------------------------------------log_retcode

static void log_retcode( RETCODE retcode )
{
    LOG( "retcode=" << retcode_as_string( retcode ) << '\n' );
}

//--------------------------------------------------------------------------------call_SQLError
// um Stack einzusparen (wegen Sos_dll):

static RETCODE call_SQLError( HENV henv, HDBC hdbc, HSTMT hstmt, Byte* szSqlState,
                              SDWORD* pfNativeError,
                              Byte* szErrorMsg, SWORD cbErrorMsgMax, SWORD* pcbErrorMsg, Bool debug )
{
    if( debug )  LOG( "  SQLError(" << henv << ',' << hdbc << ',' << hstmt << ")  " );

    if( pcbErrorMsg )  *pcbErrorMsg = 0;
    if( szErrorMsg )  szErrorMsg[0] = '\0';

    RETCODE retcode = ODBC_LIB SQLError( henv, hdbc, hstmt, szSqlState, pfNativeError, szErrorMsg, cbErrorMsgMax, pcbErrorMsg );

    if( debug )  
    {
      //if( szErrorMsg )  while( szErrorMsg == '\r' || szErrorMsg == '\n' )  szErrorMsg++;
        LOG( " => " << retcode_as_string( retcode ) << ", SqlState=" << szSqlState << ", ErrorMsg=" << ( szErrorMsg? (char*)szErrorMsg : "NULL" ) << '\n' );
    }

    return retcode;
}

//-----------------------------------------------------------------------------odbc_error_as_xc

static Xc_copy odbc_error_as_xc( RETCODE retcode, HENV henv, HDBC hdbc, HSTMT hstmt, const char* function, Bool debug )
{
    char         code           [ 40 ];
    UCHAR        szSqlState     [ SQL_SQLSTATE_SIZE+1 ];
    UCHAR        szSqlState2    [ SQL_SQLSTATE_SIZE+1 ];        // Für weitere Fehler
    Dynamic_area text           ( SQL_MAX_MESSAGE_LENGTH );
    Dynamic_area text2          ( SQL_MAX_MESSAGE_LENGTH );     // Für weitere Fehler
    SWORD        cbErrorMsg     = 0;
    SDWORD       NativeError    = 0;
    SDWORD       NativeError2   = 0;                            // Für weitere Fehler
    RETCODE      ret;

    memset( szSqlState, 0, sizeof szSqlState );

    ret = call_SQLError( henv, hdbc, hstmt, szSqlState, &NativeError,
                         text.byte_ptr(), text.size(), &cbErrorMsg, debug );
    if( ret == SQL_SUCCESS  ||  ret == SQL_SUCCESS_WITH_INFO ) {
        if( cbErrorMsg > 0 && text.byte_ptr()[ cbErrorMsg-1 ] == '.' )  cbErrorMsg--;      // unixODBC 27.7.01
        if( cbErrorMsg > 0 && text.byte_ptr()[ cbErrorMsg-1 ] == '\n' )  cbErrorMsg--;
        if( cbErrorMsg > 0 && text.byte_ptr()[ cbErrorMsg-1 ] == '\r' )  cbErrorMsg--;
        text.length( cbErrorMsg );
        text2.assign( text );
        memcpy( szSqlState2, szSqlState, sizeof szSqlState2 );
        NativeError2 = NativeError;

        while( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
        {
            szSqlState2[ SQL_SQLSTATE_SIZE ] = '\0';
            LOG( "  SQLError: pfNativeError=" << NativeError2 << ", " << text2 << '\n' );
            ret = call_SQLError( henv, hdbc, hstmt, szSqlState2, &NativeError2,
                                 text2.byte_ptr(), text2.size(), &cbErrorMsg, debug );
            if( ret == SQL_SUCCESS  ||  ret == SQL_SUCCESS_WITH_INFO ) {
                if( cbErrorMsg > 0 && text2.byte_ptr()[ cbErrorMsg-1 ] == '.' )  cbErrorMsg--;      // unixODBC 27.7.01
                if( cbErrorMsg > 0 && text2.byte_ptr()[ cbErrorMsg-1 ] == '\n' )  cbErrorMsg--;
                if( cbErrorMsg > 0 && text2.byte_ptr()[ cbErrorMsg-1 ] == '\r' )  cbErrorMsg--;
                text2.length( cbErrorMsg );
                text.append( "; " );
                text.append( text2 );
            }
        }

        szSqlState[ SQL_SQLSTATE_SIZE ] = '\0';
        sprintf( code, "ODBC-%s", (const char*)szSqlState );
    }
    else
    {
        //strcpy( code, "ODBC-UNKNOWN" );
        //text.assign( retcode_as_string( retcode ) );
        strcpy( code, c_str( "ODBC-" + retcode_as_string(retcode) ) );
    }

    
    Xc_copy x;

    if ( strcmpi( code, "ODBC-S0021" ) == 0 )  x = Duplicate_error( code );
                                         else  x = Xc( code );
    x->insert( function );
    x->insert( text.char_ptr() );
    return x;
}

//------------------------------------------------------------------------------------remove_errors
            
static void remove_errors( HENV henv, HDBC hdbc, HSTMT hstmt, bool debug )
{                                 
    UCHAR        szSqlState     [ SQL_SQLSTATE_SIZE+1 ];
    Dynamic_area text           ( SQL_MAX_MESSAGE_LENGTH );
    SWORD        cbErrorMsg     = 0;  
    SDWORD       NativeError    = 0; 
    RETCODE      ret;
        
    memset( szSqlState, 0, sizeof szSqlState );
   
    while(1)
    { 
        ret = call_SQLError( henv, hdbc, hstmt, szSqlState, &NativeError, text.byte_ptr(), text.size(), &cbErrorMsg, true );
        if( ret != SQL_SUCCESS  &&  ret != SQL_SUCCESS_WITH_INFO )  break;
        //LOG( "  SQLError: " << Const_area( text.char_ptr(), cbErrorMsg ) << '\n' );
    }
}   
    
//-----------------------------------------------------------------------------throw_and_delete
/*
static void throw_and_delete( Xc* x )
{
    try
    {
        throw *x;
    }
    catch( const Xc& )
    {
        delete x;
        throw;
    }
}
*/
//-----------------------------------------------------------------------------------odbc_error

static void odbc_error( RETCODE retcode, HENV henv, HDBC hdbc, HSTMT hstmt, const char* function, Bool debug )
{
    Xc_copy x = odbc_error_as_xc( retcode, henv, hdbc, hstmt, function, debug );
    
    if( strcmp( x->code(), "ODBC-01S02" ) == 0 )     // Option value changed, z.B. SQLServer in SQLExecute(): Cursortyp geändert
    {
        LOG( "Fehler " << x->code() << " wird ignoriert.\n ");
        //delete x;  
    }
    else
    {
        throw_xc( *x );
    }
}

//-----------------------------------------------------------------------------check_error_henv

void check_error_henv( RETCODE retcode, HENV henv, const char* function, Bool debug )
{
    if( retcode != SQL_SUCCESS ) {
        odbc_error( retcode, henv, 0, 0, function, debug );
    }
}

//-----------------------------------------------------------------------------check_error_hdbc

void check_error_hdbc( RETCODE retcode, HDBC hdbc, const char* function, Bool debug )
{
    if( retcode != SQL_SUCCESS ) {
        odbc_error( retcode, 0, hdbc, 0, function, debug );
    }
}

//----------------------------------------------------------------------------check_error_hstmt

inline void check_error_hstmt( RETCODE retcode, HSTMT hstmt, const char* function, Bool debug )
{
    if( retcode != SQL_SUCCESS ) {
        odbc_error( retcode, 0, 0, hstmt, function, debug );
    }
}

//----------------------------------------------------------------------------------Odbc_static

DEFINE_SOS_STATIC_PTR( Odbc_static )

//---------------------------------------------------------------------Odbc_static::Odbc_static

Odbc_static::Odbc_static()
:
    _henv( SQL_NULL_HENV )
{
    sos_static_ptr()->_odbcfile = this;
    set_environment_from_sos_ini();
}

//---------------------------------------------------------------------Odbc_static::~Odbc_static

Odbc_static::~Odbc_static()
{
#   ifdef SYSTEM_WIN
        LOG( "SQLFreeEnv(" << _henv << ") unterdrückt wegen Absturzes, jz 25.8.2001\n" );
        if(0) {
#   endif


    if( _debug )  LOG( "SQLFreeEnv(" << _henv << ")\n" );

    if( _henv )
    {
        RETCODE retcode = ODBC_LIB SQLFreeEnv( _henv );

        if( retcode != SQL_SUCCESS )  log_retcode( retcode );

/* In Unix Absturz
        try {
            check_error_henv( retcode, _henv, "SQLFreeEnv", _debug );
        } 
        catch ( const Xc& ) {}  // ignorieren
*/
    }


    // hostole.dll stürzt in Win95 (nicht NT) in FreeLibrary() (kernel32.dll) ab. 
    // Das seit 11.97. Darum vermeiden wir den Aufruf von FreeLibrary().

    LOG( "FreeLibrary(" << _lib.handle() << ") unterdrückt wegen Problems in Win95\n" );
    _lib.set_handle( 0 );

#   ifdef SYSTEM_WIN
        }
#   endif
}

//---------------------------------------------------------------------Odbc_static::alloc_env

void Odbc_static::alloc_env()
{
	// hostole.dll stürzt in Win95 (nicht NT) in FreeLibrary() (kernel32.dll) ab. 
	// Das seit 11.97. Darum vermeiden wir den Aufruf von FreeLibrary().
	_lib.set_handle( odbc32_dll_handle );

#   ifdef SYSTEM_WIN
        _lib.init( "ODBC32.DLL" );
#    else
        string libodbc_so = read_profile_string( "", "odbc", "libodbc.so", "libodbc.so" );
        _lib.init( libodbc_so );
#   endif

    odbc32_dll_handle = _lib.handle();

    LOG( "SQLAllocEnv()\n" );
    RETCODE retcode = ODBC_LIB SQLAllocEnv(&_henv);              /* Environment handle */
    if( retcode != SQL_SUCCESS ) throw_xc( "SQLAllocEnv" );
}

//-------------------------------------------------------------------Odbc_session::Odbc_session

Odbc_session::Odbc_session()
:
    _zero_(this+1)
{
    //_identifier_quote_begin = "[";
    //_identifier_quote_end   = "]";
    _identifier_quote_begin = "\"";
    _identifier_quote_end   = _identifier_quote_begin;

  //_date_format            = "'dd-mon-yyyy'";     7.1.98
    _date_format            = std_date_format_odbc;         // {d'yyyy-mm-dd'}
    _date_time_format       = std_date_time_format_odbc;    // {dt'yyyy-mm-dd HH:MM:SS'}
    _use_default_resultset  = false;

    _debug |= log_category_is_set( "odbc" );
}

//------------------------------------------------------------------Odbc_session::~Odbc_session

Odbc_session::~Odbc_session()
{
    try {
        _close();
    }
    catch( const exception& ) {}
}

//-----------------------------------------------------------------Odbc_session::_equal_session

Bool Odbc_session::_equal_session( Sos_database_file* db_file )
{
    Odbc_file*  file = (Odbc_file*)db_file;

    return file->_hdbc? file->_hdbc == _hdbc 
                      : true;
}

//----------------------------------------------------------------------Odbc_session::translate
/*
string Odbc_session::translate( const string& word )
{
    for( const Dbms_word* w = dbms_words; w->_word; w++ )
    {
        if( _dbms == w->_dbms && stricmp( word.c_str(), w->_word ) == 0 )  return w->_dbms_word;
    }

    return word;
}
*/
//--------------------------------------------------------------------------Odbc_session::_open

void Odbc_session::_open( Sos_database_file* db_file )
{
    Odbc_file*  file = (Odbc_file*)db_file;
    char        buffer [ 2 ];
    RETCODE     retcode;

    if( file->_hdbc ) {
        _hdbc = file->_hdbc;
        _hdbc_outside = true;
    }
    else
    {
        if( file ) {
            if( file->_auto_commit )  _auto_commit = true;
            _use_default_resultset = file->_use_default_resultset;
        }

        if( _debug )  *log_ptr << "SQLAllocConnect(" << static_ptr()->_henv << ')' << endl;
        retcode = ODBC_LIB SQLAllocConnect( static_ptr()->_henv, &_hdbc); /* Connection handle */
        check_error_henv( retcode, static_ptr()->_henv, "SQLAllocConnect", _debug );


        if( _open_mode & File_base::out ) 
        {
            if( file->_create  &&  file->_db_filename != "" )
            {
#ifdef SYSTEM_WIN
                if( GetFileAttributes( file->_db_filename.c_str() ) == -1 )
                {
                    string attr = "CREATE_DB=" + file->_db_filename + "\0" ;
                    SQLConfigDataSource( NULL, ODBC_ADD_DSN, file->_driver_name.c_str(), attr.c_str() );
                }
#endif
            }
        }
        else
        {
            // Achtung: SQL_MODE_READ_WRITE ist Default, blockiert aber andere (IM006)
            uint4 access_mode = SQL_MODE_READ_ONLY; //jz 23.11.97 ( _open_mode & File_base::out )? SQL_MODE_READ_WRITE : SQL_MODE_READ_ONLY;
            if( _debug )  *log_ptr << "SQLSetConnectOption(" << _hdbc << ",SQL_ACCESS_MODE,SQL_MODE_READ_ONLY)" << endl;
            retcode = ODBC_LIB SQLSetConnectOption(_hdbc, SQL_ACCESS_MODE, access_mode );
            check_error_hdbc( retcode, _hdbc, "SQLSetConnectOption", _debug );
        }

        if( _debug )  *log_ptr << "SQLSetConnectOption(" <<_hdbc << ",SQL_LOGIN_TIMEOUT,30)" << endl;
        retcode = ODBC_LIB SQLSetConnectOption(_hdbc, SQL_LOGIN_TIMEOUT, 30 );
        check_error_hdbc( retcode, _hdbc, "SQLSetConnectOption", _debug );

        if( _debug )  *log_ptr << "SQLSetConnectOption(" << _hdbc << ",SQL_AUTOCOMMIT," << (_auto_commit? "SQL_AUTOCOMMIT_ON" : "SQL_AUTOCOMMIT_OFF") << ")\n";
        retcode = ODBC_LIB SQLSetConnectOption(_hdbc, SQL_AUTOCOMMIT, _auto_commit? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF);
        check_error_hdbc( retcode, _hdbc, "SQLSetConnectOption", _debug );

        /* Connect to data source */
        if( length( file->_connection_string ) )
        {
            Sos_string conn_str;

            if( length( _user ) ) {
                if( length( conn_str ) )  conn_str += ';';
                conn_str += "UID=";
                conn_str += _user;
            }
            
            if( length( conn_str ) )  conn_str += ';';
            LOG( "SQLDriverConnect(" << _hdbc << ',' << conn_str << file->_connection_string << ")\n" );

            if( length( _password ) ) {
                conn_str += "PWD=";
                conn_str += _password;
            }
            if( length( conn_str ) )  conn_str += ';';
            conn_str += file->_connection_string;

            retcode = ODBC_LIB SQLDriverConnect( _hdbc, NULL, (Byte*)c_str( conn_str ), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
        }
        else
        {
            LOG( "SQLConnect(" << _hdbc<< ',' <<_db_name<<","<<_user<<",<password>)\n");
            retcode = ODBC_LIB SQLConnect( _hdbc,
                                           (Byte*)c_str( _db_name  ), SQL_NTS,
                                           (Byte*)c_str( _user     ), SQL_NTS,
                                           (Byte*)c_str( _password ), SQL_NTS );
        }
        if ( retcode == SQL_SUCCESS_WITH_INFO ) {
            Byte szSqlState [ 5+1 ];
            Dynamic_area text ( 250 );
            call_SQLError( 0, _hdbc, 0, szSqlState, 0, text.byte_ptr(), text.size(), 0, _debug );
            LOG( "SQLConnect: SQL_SUCCESS_WITH_INFO " << szSqlState << ' ' << text.char_ptr() << '\n' );
        }
        else check_error_hdbc( retcode, _hdbc, "SQLConnect", _debug );

        _connected = true;
    }

    if( _open_mode & File_base::out ) {
        UDWORD read_mode = SQL_MODE_READ_WRITE;
        if( _debug )  *log_ptr << "SQLGetConnectOption(" << _hdbc << ",SQL_ACCESS_MODE)" << endl;
        retcode = ODBC_LIB SQLGetConnectOption( _hdbc, SQL_ACCESS_MODE, &read_mode );
        try {
            check_error_hdbc( retcode, _hdbc, "SQLGetConnectOption", _debug );
            if ( read_mode == SQL_MODE_READ_ONLY ) throw_xc( "SOS-1346" );
        } catch ( const Xc& x ) {
            if ( strcmpi( x.code(), "ODBC-IM001" ) != 0 ) throw;
            else {
                LOG( "Fehler ODBC-IM001 (SQLGetConnectOption) wird ignoriert" );
            }
        }
    }

    if( file->_identifier_quote_set ) {
        _identifier_quote_begin = _identifier_quote_end = file->_identifier_quote;
    } else {
        if( _debug )  *log_ptr << "SQLGetInfo("<< _hdbc << ",SQL_IDENTIFIER_QUOTE_CHAR)" << endl;
        retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_IDENTIFIER_QUOTE_CHAR, buffer, sizeof buffer, NULL );
        if( retcode == SQL_SUCCESS ) {
            if( _debug )  *log_ptr << "quote_char=" << buffer << endl;
            _identifier_quote_begin = buffer;
        }
        else remove_errors();
    }
    //if( _identifier_quote_begin == "[" )  _identifier_quote_end = "]";
    _identifier_quote_end = _identifier_quote_begin;

    _open_postprocessing();
}

//---------------------------------------------------------Odbc_session::_open_postprocessing

void Odbc_session::_open_postprocessing()
{
    RETCODE retcode;
     // Funktionen überprüfen

#   define REQUIRED_ODBC_FUNCTION( fcode, fname ) \
    if ( !_function_array[fcode] ) throw_xc( "SOS-1307", #fname )

    if( _debug )  *log_ptr << "SQLGetFunctions(" << _hdbc << ",SQL_API_ALL_FUNCTIONS)" << endl;
    retcode = ODBC_LIB SQLGetFunctions(_hdbc,SQL_API_ALL_FUNCTIONS,_function_array);
    if (retcode != SQL_SUCCESS_WITH_INFO ) check_error_hdbc( retcode, _hdbc, "SQLGetFunctions", _debug );

     REQUIRED_ODBC_FUNCTION( SQL_API_SQLERROR,          "SQLError"          );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLEXECUTE,        "SQLExecute"        );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLEXECDIRECT,     "SQLExecDirect"     );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLPREPARE,        "SQLPrepare"        );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLGETINFO,        "SQLGetInfo"        );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLDESCRIBECOL,    "SQLDescribeCol"    );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLDISCONNECT,     "SQLDisconnect"     );
   //REQUIRED_ODBC_FUNCTION( SQL_API_SQLFREECONNECT,    "SQLFreeConnect"    );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLFREEENV,        "SQLFreeEnv"        );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLFREESTMT,       "SQLFreeStmt"       );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLALLOCSTMT,      "SQLAllocStmt"      );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLBINDCOL,        "SQLBindCol"        );
     //REQUIRED_ODBC_FUNCTION( SQL_API_SQLBINDPARAMETER,  "SQLBindParameter"  );
// ???     REQUIRED_ODBC_FUNCTION( SQL_API_SQLMORERESULTS,    "SQLMoreResults"    );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLCOLATTRIBUTES,  "SQLColAttributes"  );
     REQUIRED_ODBC_FUNCTION( SQL_API_SQLFETCH,          "SQLFetch"          );

     _sqltransact = _function_array[ SQL_API_SQLTRANSACT ] != 0;
#undef REQUIRED_ODBC_FUNCTION

    // SQLGetInfo: Ausgabe einiger Daten zu diesem HDBC
    UWORD uword;
    char  answer [100+1];
    char  string [100+1];


    LOG( "SQLGetInfo(" << _hdbc << ",SQL_DRIVER_NAME)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_DRIVER_NAME,(Byte*)string, sizeof(string), 0 );
    if( retcode == SQL_SUCCESS ) { LOG( string << '\n' );  _driver_name = string; }
                            else  remove_errors();

    LOG( "SQLGetInfo(" << _hdbc << ",SQL_DBMS_NAME)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_DBMS_NAME,(Byte*)string, sizeof(string), 0 );
    if( retcode == SQL_SUCCESS ) 
    { 
        LOG( string );  
        _dbms_name = string; 
        if( _dbms_name == "ACCESS"               )  _dbms = dbms_access;
        else
        if( _dbms_name.substr(0,6) == "Oracle"   )  _dbms = dbms_oracle;
        else
        if( _dbms_name == "MySQL"                )  _dbms = dbms_mysql;
        else
        if( _dbms_name == "Microsoft SQL Server" )  _dbms = dbms_sql_server;
        else
        if( _dbms_name == "PostgreSQL"           )  _dbms = dbms_postgresql;
        else
        if( string_begins_with( _dbms_name, "DB2" ) )  _dbms = dbms_db2;
        else
        if( string_begins_with( lcase( _dbms_name ), "firebird" ) )  _dbms = dbms_firebird;
        else
        if( _dbms_name == "Adaptive Server Enterprise" )  _dbms = dbms_sybase;
        else
        if( _dbms_name == "ASE"                        )  _dbms = dbms_sybase;
        
        if( _dbms == dbms_access )  _concat_operator = "&";
    }
    else remove_errors();

    LOG( "\nSQLGetInfo(" << _hdbc << ",SQL_DBMS_VER)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_DBMS_VER,(Byte*)string, sizeof(string), 0 );
    if( retcode == SQL_SUCCESS ) { LOG( string );  _dbms_ver = string; }
                            else remove_errors();

    LOG( "\nSQLGetInfo(" << _hdbc << ",SQL_DRIVER_ODBC_VER)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_DRIVER_ODBC_VER,(Byte*)string, sizeof(string), 0 );
    if( retcode == SQL_SUCCESS ) {
        LOG( string );
        if( string[0] == '0'  &&  string[2] == '.' ) {
            _odbc_version =   (( (uint)string[1] - '0' ) * 0x100 )
                            + (( (uint)string[3] - '0' ) * 0x10 )   // oder ist das dezimal?
                            +          string[4] - '0';  
            LOG( " _odbc_version=" << hex << _odbc_version << dec );
        }
        LOG( '\n' );
    } else {
        log_retcode( retcode );
        remove_errors();
    }

    LOG( "SQLGetInfo(" << _hdbc << ",SQL_CURSOR_COMMIT_BEHAVIOR)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_CURSOR_COMMIT_BEHAVIOR, (PTR)&uword, sizeof(uword), NULL );
    if( retcode == SQL_SUCCESS )  LOG( uword << '\n' );
                            else  log_retcode( retcode ), remove_errors();

    LOG( "SQLGetInfo(" << _hdbc << ",SQL_DATA_SOURCE_READ_ONLY)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_DATA_SOURCE_READ_ONLY, (PTR)&answer, sizeof(answer), NULL );
    if( retcode == SQL_SUCCESS )  LOG( answer << '\n' );
                            else  log_retcode( retcode ), remove_errors();

    LOG( "SQLGetInfo(" << _hdbc << ",SQL_DRIVER_VER)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_DRIVER_VER, (PTR)&string, sizeof(string)-1, 0 );
    if( retcode == SQL_SUCCESS )  { _sql_driver_ver = string; LOG( string << '\n' ); }
                            else  log_retcode( retcode ), remove_errors();

    LOG( "SQLGetInfo(" << _hdbc << ",SQL_USER_NAME)  " );
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_USER_NAME, (PTR)&string, sizeof(string)-1, 0 );
    if( retcode == SQL_SUCCESS )  LOG( string << '\n' );
                            else  log_retcode( retcode ), remove_errors();

    // Für SQLPutData:
    string[ 0 ] = '\0';
    if( _debug ) *log_ptr << "SQLGetInfo( " << _hdbc << ",SQL_NEED_LONG_DATA_LEN)  ";
    retcode = ODBC_LIB SQLGetInfo( _hdbc, SQL_NEED_LONG_DATA_LEN, (PTR)&string, sizeof string, NULL );
    if( _debug ) if( retcode == SQL_SUCCESS )  *log_ptr << string << '\n';
                                         else  log_retcode( retcode ), remove_errors();
    _sql_need_long_data_len = string[ 0 ] != 'N';






#   if defined SYSTEM_UNIX
        if( _driver_name == "liboraodbc.so" && !oracle_module ) 
        {
            LOG( "*** dlopen(\"libclntsh.so\")  Der Oracle-Treiber ruft atexit(), er darf also nicht entladen werden, sonst stürzt exit() ab.\n" );
            oracle_module = dlopen( "libclntsh.so", RTLD_LAZY );
        }
#   endif

/*
    if( _dbms == dbms_oracle )
    {
        RETCODE retcode;

        // Oracle soll Dezimalpunkt liefern
        retcode = execute_direct_without_error_check( "ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '.,';" );
        if( retcode != SQL_SUCCESS )  remove_errors();

        retcode = execute_direct_without_error_check( "ALTER SESSION SET NLS_DATE_FORMAT = 'YYYY-MM-DD HH24:MI:SS';" );
        if( retcode != SQL_SUCCESS )  remove_errors();
    }
*/
}

//-------------------------------------------------------------------------Odbc_session::_close

void Odbc_session::_close( Close_mode )
{
    if( _hdbc  &&  !_hdbc_outside )
    {
        RETCODE retcode;

        if( _connected )
        {
            LOG( "SQLDisconnect(" << _hdbc << ")\n" );
            retcode = ODBC_LIB SQLDisconnect( _hdbc );
            try {
                check_error_hdbc( retcode, _hdbc, "SQLDisconnect", _debug );
            } catch ( const Xc& ) { /*ignorieren*/ }
        }

        LOG( "SQLFreeConnect(" << _hdbc << ")\n" );
        retcode = ODBC_LIB SQLFreeConnect( _hdbc );
        try {
            check_error_hdbc( retcode, _hdbc, "SQLFreeConnect", _debug );
        } catch ( const Xc& ) { /*ignorieren*/ }

        _hdbc = 0;
    }
}

//----------------------------------------------------------------Odbc_session::_execute_direct

void Odbc_session::_execute_direct( const Const_area& stmt )
{
    RETCODE retcode = execute_direct_without_error_check( stmt );

    check_error_hstmt( retcode, _hstmt, "SQLExecDirect", _debug );

    SQLINTEGER row_count = -1;
    if( _debug )  *log_ptr << "SQLRowCount " << _hstmt << "  ";
    retcode = ODBC_LIB SQLRowCount( _hstmt, &row_count );

    _row_count = row_count;

    if( _debug )  if( retcode == SQL_SUCCESS )  *log_ptr << _row_count << endl;
                                        //else  log_retcode( retcode );

    //check_error_hstmt( retcode, _hstmt, "SQLRowCount" );
    if( retcode != SQL_SUCCESS )  remove_errors();
}

//---------------------------------------------Odbc_session::execute_direct_without_error_check

RETCODE Odbc_session::execute_direct_without_error_check( const Const_area& stmt_area )
{
    RETCODE retcode;

    string stmt ( stmt_area.char_ptr(), stmt_area.length() );  // Oracle 8.0.5 möchte ein 0-Byte am Ende haben

    if ( _hstmt == SQL_NULL_HSTMT ) {
        if( _debug )  *log_ptr << "SQLAllocStmt " << _hdbc << endl;
        retcode = ODBC_LIB SQLAllocStmt( _hdbc, &_hstmt );
        check_error_hdbc( retcode, _hdbc, "SQLAllocStmt", _debug );
    }

    if( _debug )  *log_ptr << "SQLExecDirect " << _hstmt << ',' << stmt << endl;
    retcode = ODBC_LIB SQLExecDirect( _hstmt, (UCHAR*)stmt.c_str(), stmt.length() );

    return retcode;
}

//------------------------------------------------------------------------Odbc_session::_commit

void Odbc_session::_commit()
{
    if( _sqltransact ) {
        RETCODE retcode;
        if( _debug )  *log_ptr << "SQLTransact(" << static_ptr()->_henv << ',' << _hdbc << ",SQL_COMMIT)" << endl;
        retcode = ODBC_LIB SQLTransact( static_ptr()->_henv, _hdbc, SQL_COMMIT );
        check_error_hstmt( retcode, _hstmt, "SQLTransact", _debug );
    }
}

//----------------------------------------------------------------------Odbc_session::_rollback

void Odbc_session::_rollback()
{
    if( _sqltransact  &&  _hdbc ) 
    {
        RETCODE retcode;
        if( _debug )  LOG( "SQLTransact(" << static_ptr()->_henv << ',' << _hdbc << ",SQL_ROLLBACK)\n" );
        retcode = ODBC_LIB SQLTransact( static_ptr()->_henv, _hdbc, SQL_ROLLBACK );

        // Kein throw, weil im catch gerufen (?)
        if( retcode != SQL_SUCCESS &&  retcode != SQL_SUCCESS_WITH_INFO )
        {
            try {
                check_error_hdbc( retcode, _hdbc, "SQLTransact", _debug );
            } catch ( const Xc& ) { /*ignorieren*/ }
            //LOG( "ODBC Sql Rollback FAILED: no exception (driver not capable?)!!!\n" );
        }
        //if( retcode != SQL_SUCCESS )  remove_errors();
    }
    //else ??????????????????????????????????????????????????????????????
}

//---------------------------------------------------------------------Odbc_session::_obj_print

void Odbc_session::_obj_print( ostream* s ) const
{
    Sos_database_session::_obj_print( s );
    *s << " ODBC version " << ( _odbc_version >> 8 ) << '.' << ( _odbc_version & 0xFF );
}

// ------------------------------------------------------------------------Odbc_file::Odbc_file

Odbc_file::Odbc_file()
:
    _zero_(this+1),
    _max_length(_odbc_default_max_length)
{
    _cbValue_array.obj_const_name( "Odbc_file::_cbValue_array" );
    _param_bindings.obj_const_name( "Odbc_file::_param_bindings" );
    _result_bindings.obj_const_name( "Odbc_file::_result_bindings" );
    _use_default_resultset  = false;
}

//------------------------------------------------------------------------Odbc_file::~Odbc_file

Odbc_file::~Odbc_file()
{
    if( _blob_file )  ::close( _blob_file );

    if( _hstmt ) {
        if( _debug )  LOG( "SQLFreeStmt(" << _hstmt << ",SQL_DROP)\n" );
        RETCODE retcode = ODBC_LIB SQLFreeStmt( _hstmt, SQL_DROP );
        try {
            check_error_hstmt( retcode, _hstmt, "SQLFreeStmt", _debug );
        } catch ( const Xc& ) { /*ignorieren*/ }
        _hstmt = 0;
    }

    session_disconnect();
}

//-----------------------------------------------------------------------Odbc_file::check_error

inline void Odbc_file::check_error( RETCODE retcode, const char* function )
{
    check_error_hstmt( retcode, _hstmt, function, _debug );
}

//--------------------------------------------------------------------Odbc_file::_create_static

void Odbc_file::_create_static()
{
    Sos_ptr<Odbc_static> o = SOS_NEW( Odbc_static );
    o->alloc_env();
    _static = +o;
}

//-------------------------------------------------------------------Odbc_file::_create_session

Sos_ptr<Sos_database_session> Odbc_file::_create_session()
{
    Sos_ptr<Odbc_session> o = SOS_NEW( Odbc_session );
    return +o;
}

//----------------------------------------------------------Odbc_file::driver_name_from_filename

string Odbc_file::driver_name_from_filename( const string& filename )
{
# ifndef SYSTEM_WIN
      throw_xc( "SOS-1445", filename );
      return "";
# else

    RETCODE      retcode;
    string       fileextns  = "FileExtns=*." + extension_of_path( filename );
    int          first_next = SQL_FETCH_FIRST;
    Dynamic_area description(1000) , attributes(1000);  
    SQLSMALLINT  description_length, attributes_length;

    while(1)
    {
        retcode = SQLDrivers( static_ptr()->_henv, first_next, 
                              description.byte_ptr(), description.size(), &description_length,
                              attributes .byte_ptr(), attributes .size(), &attributes_length   );
        if( retcode == SQL_NO_DATA )  throw_xc( "SOS-1445", filename );
        check_error_henv( retcode, static_ptr()->_henv, "SQLDrivers", _debug );

        const char* p = attributes.char_ptr();
        while( *p )
        {
            if( stricmp( p, fileextns.c_str() ) == 0 )  return description.char_ptr();
            p += strlen( p ) + 1;
        }

        first_next = SQL_FETCH_NEXT;
    }

# endif
}

// --------------------------------------------------------------------Odbc_file::_prepare_open

void Odbc_file::prepare_open( const char* filename, Open_mode open_mode, const File_spec& )
{
    Sos_string select_statement;
    bool       default_resultset_set = false;

    _fixed_length = 30*1024;    // bei 16bit besser weniger als 32768.

    _open_mode = open_mode;
    _assert_row_count = -1;
    
    get_static( "odbc" );

    for ( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( database_option( opt ) )            { /* do nothing */ }
        else
        if( opt.with_value( "filedsn"       ) ) { 
            _connection_string += "FILEDSN=";
            _connection_string += opt.value();
            if( extension_of_path( opt.value() ) == "" )  _connection_string += ".dsn";
            _connection_string += ';'; 
        }
        else
        if( opt.with_value( 'c', "conn-str" ) ) { _connection_string += opt.value() + ";"; }
        else
        if( opt.flag( "create"              ) ) { _create = opt.set(); }
        else
        if( opt.with_value( "hdbc" ) )          { _hdbc = (HDBC)opt.as_uint4(); }
        else
        if( opt.flag( "field-as-file" ) )       { _field_as_file = opt.set(); }
        else
        if( opt.with_value( "blob" ) ||
            opt.with_value( "clob" ) )          { _blob = opt.value();  _field_as_file = true;  if( _assert_row_count < 0 )  _assert_row_count = 1; }
        else
        if( opt.flag      ( "no-blob-record-allowed" ) )  opt.set();
        else
        if( opt.flag( "single" ) )              { _single = opt.set(); }
      //if( opt.flag( "single" ) )              { _assert_row_count = opt.set()? 1 : -1; }
        else
        if( opt.with_value( "table" ) )         { _table_name = opt.value(); }
        else
        if( opt.with_value( "file" ) )          { _db_filename = opt.value(); }
        else
        if( opt.with_value( "long-data-length" ) )  { _long_data_length = opt.as_uintK(); }
        else
        if( opt.with_value( "fixed-length" ) )  { _fixed_length = opt.as_uintK(); }
        else
        if( opt.with_value( "min-length" ) )    { _min_length = opt.as_uintK(); }
        else
        if( opt.with_value( "max-length" ) )    { _max_length = opt.as_uintK(); }
        else
        if( opt.with_value( "identifier-quote" ) )   { _identifier_quote = opt.value(); _identifier_quote_set = true; }
        else
        if( opt.flag( "commit" ) )              { _commit = opt.set(); }
        else
        if( opt.flag( "info" ) )                { _info = opt.set(); }          // Liefert nur Informationen von SQLInfo()!
        else
        if( opt.flag( "default-resultset" ) )   { _use_default_resultset = opt.set(); 
                                                  default_resultset_set = true; }      // Für SQLServer
        else
        if( _db_name.empty() && _db_filename.empty() && _connection_string.empty() && opt.param(1) )  _db_filename = opt.value();
        else
        if( opt.param() )                       { select_statement = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    //s.u. if( _field_as_file  &&  _fixed_length == 0 )  throw_xc( "SOS-1334" );

    if( !empty( _db_filename ) ) 
    {
        string ext = lcase( extension_of_path( _db_filename ) );
        if( ext == "dsn" )
        {
            _connection_string += "FILEDSN=" + _db_filename + ";" + _connection_string;
        }
        else
        if( ext == ""  &&  _db_name == "")
        {
            _db_name = _db_filename;
            _db_filename = "";
        }
        else
        {
            _driver_name = driver_name_from_filename(_db_filename);
            _connection_string += "DRIVER=" + _driver_name + ";DBQ=" + _db_filename;
        }
    }

    if( length( _connection_string ) > 0 )
    {
        Sos_string conn_str = _connection_string;
        _connection_string = "";
        if( length( _db_name ) > 0 ) {
            _connection_string += "DSN=";
            _connection_string += _db_name;
        }
        if( length( _connection_string ) )  _connection_string += ';';
        _connection_string += conn_str;
    }

    if( length( _db_name ) == 0 )  _db_name = _connection_string;   // Damit die Verbindung einen Namen hat

    // Connection aufbauen, wenn nicht schon passiert
    session_connect( "odbc" );

    if( !default_resultset_set )  _use_default_resultset = session()->_use_default_resultset;

    if( !empty( _blob ) ) 
    {
        if( empty( select_statement ) )  throw_xc( "SOS-1397" );

        if( open_mode & out ) {
            Sos_string s;
            s = select_statement;
            select_statement = "UPDATE " + ucase(_table_name) + " SET \"" + ucase(_blob) + "\"=? " + s;
            _reset_statement = "UPDATE " + ucase(_table_name) + " SET \"" + ucase(_blob) + "\"=NULL " + s;
        } 
        else 
        {
            string s = "SELECT \"" + ucase(_blob) + "\" FROM " + ucase(_table_name) + " ";

            int w = lcase( select_statement ).find( "where" );
            if( session()->_dbms == dbms_oracle  &&  session()->_sql_driver_ver == "8.0.5.0.0"  &&  w != string::npos )   // Oracle-Treiber 8.0.5 liest keine Null-Blobs. 
            {                                                                                                             // Kann vielleicht für alle DBMS so gemacht werden.
                select_statement = s + select_statement.substr( 0, w+5 ) + " ("  + select_statement.substr( w+5 ) + ")"
                                       " and \"" + ucase(_blob) + "\" is not null";
            }
            else
                select_statement =  s + select_statement;
        }

        LOG( "-blob: " << select_statement << '\n' );
    }

    if( _info ) {
/*
        Sos_ptr<Record_type> type;

        type->add_field( .. );
        _any_file_ptr->_spec._field_type_ptr = type;
*/
    }
    else
    {
        if( select_statement != "" ) 
        {
            //Kann auch Update mit SQLPutData sein: if( !(open_mode & in) )  throw_xc( "SOS-1332" );
            // Select-Statement gleich ausführen
            _need_result_set = strncmpi( c_str( select_statement ), "select ", 7 ) == 0;
            Dynamic_area stmt;
            _session->convert_stmt( Const_area( c_str( select_statement ), length( select_statement ) ), &stmt );
            prepare_stmt( as_string( stmt.char_ptr(), length( stmt ) ) );
            _any_file_ptr->_spec._field_type_ptr = +_type;
        }
    }
}

//----------------------------------------------------------------------Odbc_file::prepare_stmt

void Odbc_file::prepare_stmt( const Sos_string& stmt )
{
    RETCODE retcode;

    if( _hstmt == SQL_NULL_HSTMT ) {
        if( _debug )  *log_ptr << "SQLAllocStmt(" << session()->_hdbc << ')' << endl;
        retcode = ODBC_LIB SQLAllocStmt( session()->_hdbc, &_hstmt );
        check_error_hdbc( retcode, session()->_hdbc, "SQLAllocStmt", _debug );
    }

    if( stricmp( c_str(session()->_driver_name), "Sqlsrv32.dll" ) == 0 )   // Microsoft SQL Server?
    {
        if( _need_result_set  &&  !_use_default_resultset )
        {
            // Einen server-basierten Cursor verwenden: (damit können mehrere Anweisungen parallel ausgeführt werden)
            if( _debug )  *log_ptr << "SQLSetStmtOption(" << _hstmt << ",SQL_SOPT_SS_CURSOR_OPTIONS,SQL_CO_FFO)" << endl;
            retcode = ODBC_LIB SQLSetStmtOption( _hstmt, SQL_SOPT_SS_CURSOR_OPTIONS, SQL_CO_FFO );
            check_error( retcode, "SQLSetStmtOption" );
        }
    }

    if( _debug )  *log_ptr << "SQLPrepare(" << _hstmt << ',' << stmt << ')' << endl;
    retcode = ODBC_LIB SQLPrepare( _hstmt, (Byte*)c_str( stmt ), length( stmt ) );
    check_error( retcode, "SQLPrepare" );

/*  Kein RowCount mehr, gibt nur Fehlermeldungen. 10.4.2002

    SQLINTEGER row_count = -1;
    if( _debug )  *log_ptr << "SQLRowCount(" << _hstmt << ")  ";
    retcode = ODBC_LIB SQLRowCount( _hstmt, &row_count );

    _row_count = row_count;
    if( _debug )  if( retcode == SQL_SUCCESS )  *log_ptr << _row_count << endl;
                                          else  log_retcode( retcode );

    if( retcode != SQL_SUCCESS )  remove_errors();
    if ( retcode == SQL_SUCCESS && _single && _row_count == 0 ) throw_not_found_error();
*/

    _stmt_prepared = true;
    _has_result_set = true;
    
    if( _debug )  *log_ptr << "SQLColAttributes(" << _hstmt << ",SQL_COLUMN_COUNT)" << endl;
    retcode = ODBC_LIB SQLColAttributes( _hstmt, 0, SQL_COLUMN_COUNT, 0, 0, 0, &_column_count );
    if( retcode == SQL_ERROR ) 
    {
        Xc_copy x = odbc_error_as_xc( retcode, NULL, NULL, _hstmt, "SQLColAttributes", _debug );

#       if defined SYSTEM_UNIX
        if( strcmp( x->code(), "ODBC-S1002" ) == 0 )  // unixODBC kenn SQL_COLUMN_COUNT nicht, interpretiert das als Spaltennummer jz 25.7.01
        {
            SDWORD usigned;
            _column_count = 0;
            for( int i = 1; true; i++ )
            {
                if( _debug )  *log_ptr << "SQLColAttributes(" << _hstmt << ',' << i << ",SQL_COLUMN_UNSIGNED" << endl;
                retcode = ODBC_LIB SQLColAttributes( _hstmt, i, SQL_COLUMN_UNSIGNED, 0, 0, 0, &usigned );
                if( retcode != SQL_SUCCESS )  break;
                _column_count = i;
            }
            if( retcode != SQL_SUCCESS )  remove_errors();
            if( _column_count > 0 )  x = NULL;
        }
#       endif

        if( x )
        {
            if( strcmp( x->code(), "ODBC-24000" ) == 0    // Jet: Vorbereitete Anweisung ist keine Cursorspezifikation
             || strcmp( x->code(), "ODBC-S2400" ) == 0    // Invalid cursor state. The stmt did not return a result set. (Informix)
             || strcmp( x->code(), "ODBC-S1002" ) == 0    // Invalid column number?
             || strcmp( x->code(), "ODBC-07002" ) == 0  &&  session()->_dbms_name == "MySQL"    // [MySQL][ODBC 3.51 Driver][mysqld-4.0.4-beta-max-nt]SQLBindParameter not used for all parameters
             || strcmp( x->code(), "ODBC-07005" ) == 0  &&  session()->_dbms_name == "MySQL"    // [MySQL][ODBC 3.51 Driver][mysqld-4.0.12-log]No result set
             || strcmp( x->code(), "ODBC-"      ) == 0                                       )  // [Oracle][Odbc], Oracle 8.0.5 liefert keinen Fehlercode
            {
                LOG( "Der Fehler von SQLColAttributes() wird ignoriert (Anweisung ist wohl kein SELECT)\n" );
                _has_result_set = false;
                _column_count = 0;
            }
            else 
                throw_xc( *x );
        }
    } 
    else 
    {
        _has_result_set = _column_count > 0;
        if( _debug )  LOG( "_column_count=" << _column_count << '\n' );
    }

    if( _need_result_set  &&  !_has_result_set )  throw_xc( "SOS-1396" );   // Jet liefert kein Ergebnis, wenn im Select etwas nicht stimmt. jz 8.12.98

    if( _field_as_file )
    {
        _blob_data_count = 0;
        if( _column_count > 1 )  throw_xc( "SOS-1333", _column_count );  // 0 bei UPDATE/INSERT, 1 bei SELECT,

        if( session()->_odbc_version >= 0x0200 ) {   // ODBC 2.0?
            if( session()->_sql_need_long_data_len  &&  _long_data_length == 0 )  {
#               if defined SYSTEM_WIN
                    if ( _open_mode & File_base::out ) {
                        LOG( "Odbc_file::prepare_stmt: Blob wird zwischengespeichert ...\n" );
                        _buffered_blob = true;
                    } else {
                        // js 27.7.00: Lesen aus dem Blob-Feld sollte gehen ...
                    }
                    // js 27.7.00: throw_xc( "SOS-1336", c_str(session()->_dbms_name) );  // Länge des Datums ist unbekannt
#               endif
            } else {
                _cbValue = SQL_LEN_DATA_AT_EXEC( _long_data_length );
            }
        } else {
            _cbValue = SQL_DATA_AT_EXEC;
        }

        if( _open_mode & File_base::out ) 
        {
            if( !_buffered_blob )  bind_blob_parameter();
        }
        else
        {
            _blob_ctype = ctype_from_sqltype( get_blob_sqltype2( _hstmt ) );
        }
    }
    else
    if( _has_result_set )
    {
        if( _column_count == 0 ) {
            LOG( "SQLColAttributes liefert column_count = 0. SQLExecute wird jetzt ausgeführt\n" );

            execute_stmt();

            if( _debug )  *log_ptr << "SQLColAttributes(" << _hstmt << ",SQL_COLUMN_COUNT)" << endl;
            retcode = ODBC_LIB SQLColAttributes( _hstmt, 0, SQL_COLUMN_COUNT, 0, 0, 0, &_column_count );
            check_error( retcode, "SQLColAttributes" );
            if( _column_count == 0 )  throw_xc( "SQLColAttributes", "column_count=0" );
        }

        describe_columns();
    }
}

//-------------------------------------------------------------------Odbc_file::ctype_from_sqltype

SQLSMALLINT Odbc_file::ctype_from_sqltype( SQLSMALLINT sqltype )
{
    if( session()->_dbms == dbms_oracle  &&  session()->_sql_driver_ver == "8.0.5.0.0"  &&  sqltype == sql_oracle_805_blob )       // 2005-01-07
        return SQL_C_BINARY;

    return    sqltype == SQL_LONGVARBINARY 
           || sqltype == SQL_VARBINARY 
           || sqltype == SQL_BINARY        ? SQL_C_BINARY : SQL_C_CHAR;
}

//--------------------------------------------------------------------Odbc_file::get_blob_sqltype2

SQLSMALLINT Odbc_file::get_blob_sqltype2( HSTMT hstmt )
{
    SQLCHAR     name [100+1];
    SQLSMALLINT data_type, scale, nullable;
    SQLUINTEGER column_size;
    RETCODE     retcode;

    if( _debug )  *log_ptr << "SQLDescribeCol(" << hstmt << ",1)" << std::flush;
    retcode = ODBC_LIB SQLDescribeCol( hstmt, 1, name, sizeof name, NULL, &data_type, &column_size, &scale, &nullable );
    if( _debug )  *log_ptr << "  fSqlType=" << data_type << endl;
    check_error( retcode, "SQLDescribeCol" );

    if( session()->_dbms == dbms_oracle  &&  session()->_sql_driver_ver == "8.0.5.0.0"  &&  data_type == sql_oracle_805_blob )  
        data_type = SQL_LONGVARBINARY;   // 2005-01-07 Für AOK

    return data_type;
}

//----------------------------------------------------------------------Odbc_file::get_blob_sqltype

SQLSMALLINT Odbc_file::get_blob_sqltype()
{
    LOGI( "get_blob_sqltype\n" );     

    string      stmt  = "select " + _blob + " from " + _table_name + " where 1=0";
    HSTMT       hstmt = NULL;
    SQLSMALLINT data_type;
    RETCODE     retcode;

    if( _debug )  *log_ptr << "SQLAllocStmt(" << session()->_hdbc << ')' << endl;
    retcode = ODBC_LIB SQLAllocStmt( session()->_hdbc, &hstmt );
    check_error( retcode, "SQLAllocStmt" );

    try
    {
        if( _debug )  *log_ptr << "SQLPrepare(" << hstmt << ',' << stmt << ')' << endl;
        retcode = ODBC_LIB SQLPrepare( hstmt, (Byte*)c_str( stmt ), length( stmt ) );
        check_error( retcode, "SQLPrepare" );

        data_type = get_blob_sqltype2( hstmt );
    }
    catch( const exception& )
    {
        if( _debug )  LOG( "SQLFreeStmt(" << _hstmt << ",SQL_DROP)\n" );
        retcode = ODBC_LIB SQLFreeStmt( hstmt, SQL_DROP );
        check_error_hstmt( retcode, hstmt, "SQLFreeStmt", _debug );
        throw;
    }

    if( _debug )  LOG( "SQLFreeStmt(" << _hstmt << ",SQL_DROP)\n" );
    retcode = ODBC_LIB SQLFreeStmt( hstmt, SQL_DROP );
    check_error_hstmt( retcode, hstmt, "SQLFreeStmt", _debug );

    return data_type;
}

//-----------------------------------------------------------------Odbc_file::bind_blob_parameter

void Odbc_file::bind_blob_parameter()
{
    RETCODE retcode;

    SDWORD precision = _long_data_length? _long_data_length : 0x40000000; // 1GB s. "Inside ODBC"
    //Sos_string blob_type = read_profile_string( "", "debug", "blob-type", "-1");
    //int bt = SQL_LONGVARCHAR;
    //bt = as_int( (const char*)blob_type );

    SQLSMALLINT value_type, parameter_type;

    parameter_type = get_blob_sqltype();

    value_type = ctype_from_sqltype( parameter_type );

  //if( ( _open_mode & binary ) && session()->_dbms != dbms_access )  value_type = SQL_C_BINARY,  parameter_type = SQL_LONGVARBINARY;
  //                                                            else  value_type = SQL_C_CHAR  ,  parameter_type = SQL_LONGVARCHAR;

    if( _debug )  *log_ptr << "SQLBindParameter(" << _hstmt << ",1,SQL_PARAM_INPUT,"
                           << name_of_ctype( value_type ) << ","
                           << name_of_sqltype( parameter_type ) << ","
                           << precision << ","
                           << _cbValue << ')' << endl;

    retcode = ODBC_LIB SQLBindParameter( _hstmt, 1, SQL_PARAM_INPUT, value_type, parameter_type, precision, 0, NULL, 0, &_cbValue );

    check_error( retcode, "SQLBindParameter" );
}

//-----------------------------------------------------------------Odbc_file::describe_columns

void Odbc_file::describe_columns()
{                                                                                                                
    RETCODE retcode;

    std::set<string> ignore = set_split( " *, *", lcase(_ignore_fields) );

    _type = Record_type::create();
    _result_bindings.first_index( 1 );
    _result_bindings.last_index( _column_count );

    for ( UWORD i=1; i <= _column_count; i++ )
    {
        SWORD             nullable;
        Dynamic_area      name       ( 100+1 );
        SDWORD            usigned;
        Sos_odbc_binding* b          = &_result_bindings[ i ];

        if( _debug )  *log_ptr << "SQLDescribeCol(" << _hstmt << ',' << i << ')' << std::flush;
        retcode = ODBC_LIB SQLDescribeCol( _hstmt, i, name.byte_ptr(), name.size(), NULL, &b->_fSqlType, &b->_precision, &b->_scale, &nullable );
        if( _debug )  *log_ptr << "  fSqlType=" << name_of_sqltype( b->_fSqlType ) << " precision=" << b->_precision << " scale=" << b->_scale << " nullable=" << nullable << endl;
        check_error( retcode, "SQLDescribeCol" );

        if( ignore.find( lcase(name.char_ptr()) ) == ignore.end() )
        {
            //_column_map[ _type->field_count() ] = i;

            if( !b->_fSqlType )  throw_xc( "SOS-1355", name );   // Tritt bei Jet/Access 97 auf, wenn der Feldname nicht stimmt. jz 12.7.97

            if( _debug )  *log_ptr << "SQLColAttributes(" << _hstmt << ',' << i << ",SQL_COLUMN_UNSIGNED)" << std::flush;
            retcode = ODBC_LIB SQLColAttributes( _hstmt, i, SQL_COLUMN_UNSIGNED, 0, 0, 0, &usigned );
            if( _debug )  *log_ptr << "  unsigned=" << usigned << endl;
            check_error( retcode, "SQLColAttributes" );

            b->_nullable   = nullable != SQL_NO_NULLS;
            b->_fParamType = SQL_PARAM_INPUT;

            if( b->_fSqlType == SQL_DECIMAL  &&  b->_scale == 0  &&  b->_precision == 1  &&  !b->_nullable )    // Oracles NUMBER(1) NOT NULL wird als bool gedeutet
            {
                LOG( name << " NUMBER(1) NOT NULL wird als Bool interpretiert (weil Oracle kein Bool kennt)\n" );
                b->_fCType = SQL_C_BIT;
            }
            else
                b->_fCType = sql_to_c_default( b->_fSqlType, session()->_odbc_version, usigned != 0, b->_scale, b->_precision );
    
            if( b->_fCType == SQL_C_CHAR  ||  b->_fCType == SQL_C_BINARY ) 
            {
                if( _debug )  *log_ptr << "SQLColAttributes(" << _hstmt << ',' << i << ",SQL_COLUMN_LENGTH)" << std::flush;
                retcode = ODBC_LIB SQLColAttributes( _hstmt, i, SQL_COLUMN_LENGTH, 0, 0, 0, &b->_cbValueMax );
                if( _debug )  *log_ptr << "  cbValueMax=" << b->_cbValueMax << endl;
                check_error( retcode, "SQLColAttributes" );

#               ifdef SYSTEM_UNIX   // unixODBC und Oracle-ODBC-Treiber von Easysoft liboraodbc.so
                    if( b->_fSqlType == SQL_NUMERIC  &&  b->_cbValueMax == 22 )  b->_cbValueMax = 64;   // Bei SQLFetch() will Oracle 64 Bytes. 29.7.01
#               endif

                if( b->_cbValueMax < _min_length )  b->_cbValueMax = _min_length;
            
                if( _max_length ) {
                    if( b->_cbValueMax > _max_length )  b->_cbValueMax = _max_length;
                } else {
                    if( b->_cbValueMax > 32*1024u )  throw_xc( "SOS-1354", name.char_ptr() );
                }          

                if( b->_fCType == SQL_C_CHAR )  b->_cbValueMax++;  // Platz für '\0'.
            } 

            b->prepare();
            b->_field->name( name.char_ptr() );

            b->_field->add_to( _type );
            if( b->_nullable  &&  !b->_field->type().nullable() )  b->_field->add_null_flag_to( _type );

            if ( _debug ) *log_ptr << "  describe_columns: field=" << name.char_ptr() << " scale=" << b->_scale 
                                   << " prec=" << b->_precision << " SqlType=" << name_of_sqltype( b->_fSqlType ) 
                                   << " => c_type=" << name_of_ctype(b->_fCType) 
                                   << ", " << *b->_field << endl;
        }
    }

    _cbValue_array.first_index( 1 );
    _cbValue_array.last_index( _column_count );
}

//-------------------------------------------------------------------Odbc_file::bind_parameters

void Odbc_file::bind_parameters( const Record_type* param_type, const Byte* param_base )
{
    RETCODE ret;
    
    _param_type = (Record_type*)param_type;
    _param_base = param_base;

    if( param_type ) 
    {
        int i;

        _odbc_param_type = Record_type::create();
        _odbc_param_type->allocate_fields( param_type->field_count() );

        _param_bindings.clear();
        _param_bindings.last_index( param_type->field_count() - 1 );

        for( i = 0; i < param_type->field_count(); i++ ) 
        {
            Field_descr*      f = param_type->field_descr_ptr( i );
            Sos_odbc_binding* b = &_param_bindings[ i ];

            if( f->type_ptr() ) {
                b->_fCType = odbc_c_default_type( *f->type_ptr()->info(), f->type_ptr()->field_size() );

                SDWORD cbValueMax = f->type_ptr()->field_size();
                if( b->_fCType == SQL_C_CHAR )  cbValueMax++;   // odbctype.cxx zieht wieder eins ab. jz 4.9.97
                b->_cbValueMax = cbValueMax;
                b->_fParamType = SQL_PARAM_INPUT;

                b->prepare( f );

                if( b->_fCType == SQL_C_CHAR )  b->_default_length = SQL_NTS;
            } else {
                b->_field = SOS_NEW( Field_descr );
            }
            
            if( length( b->_field->name() ) == 0 )  b->_field->name( "bind_par_" + as_string( 1+i ) );

            b->_field->add_to( _odbc_param_type );
        }

        _odbc_param_record.allocate_min( _odbc_param_type->field_size() );

        for( i = 0; i <= _param_bindings.last_index(); i++ ) 
        {
            Sos_odbc_binding*   b = &_param_bindings[ i ];
            Type_param          par;

            if( b->_field->type_ptr() )  b->_field->type_ptr()->get_param( &par );

            SWORD sql_type = odbc_sql_type( *b->_field->type_ptr()->info(), b->_field->type_ptr()->field_size() );
            if( sql_type == SQL_VARCHAR )  sql_type = SQL_CHAR;  //jz 14.10.97 für oracle (Visigenic 2.0): where adr_kurzname = ?

            if( _debug )  *log_ptr << "SQLBindParameter(" << _hstmt << ',' << (1+i) << ',' << b->_fParamType << ',' << name_of_ctype(b->_fCType) 
                                   << ',' << sql_type << ',' << par.precision_10() << ',' << par._scale << ",," << b->_field->type_ptr()->field_size() << ')' << endl;
            ret = ODBC_LIB SQLBindParameter( _hstmt, 
                                             1 + i, 
                                             b->_fParamType, 
                                             b->_fCType, 
                                             sql_type,
                                             par.precision_10(), 
                                             par._scale, 
                                             b->_field->ptr( _odbc_param_record.byte_ptr() ), 
                                             b->_field->type_ptr()->field_size(), 
                                             &b->_length );   // Wird vor SQLExecute versorgt
            try {
                check_error( ret, "SQLBindParameters" );
            } 
            catch( Xc& x ) {
                x.insert( i );
                x.insert( param_type->field_descr_ptr( i ) );
                throw;
            }

        }
    } 
    else 
    {
        ret = ODBC_LIB SQLFreeStmt( _hstmt, SQL_UNBIND );
        check_error( ret, "SQLCloseStmt SQL_UNBIND" );
/* jz 26.11.97
        for( int i = 0; i < _param_bindings.last_index(); i++ ) 
        {
            if( _debug )  *log_ptr << "SQLBindParameter(" << _hstmt << ',' << (1+i) << "SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,0,0,,0,)" << endl;

            ret = SQLBindParameter( _hstmt, 
                                             1 + i, 
                                             SQL_PARAM_INPUT, 
                                             SQL_C_CHAR, 
                                             SQL_CHAR,
                                             0, 
                                             0, 
                                             NULL, 
                                             0, 
                                             NULL );
            check_error( ret, "SQLBindParameters" );
        }
*/
        _param_bindings.clear();
        _odbc_param_record.free();
    }
}

// -----------------------------------------------------------------------------Odbc_file::open

void Odbc_file::open( const char*, Open_mode, const File_spec& )
{
    if( _stmt_prepared ) 
    {
        if( !_stmt_executed && !_buffered_blob )  execute_stmt();

        _any_file_ptr->_record_count = _row_count;

        if( _has_result_set ) 
        {
            if( _field_as_file ) {
                if( _debug )  *log_ptr << "SQLFetch(" << _hstmt << ')' << endl;
                RETCODE retcode = ODBC_LIB SQLFetch( _hstmt );
    
                switch( retcode ) {
                    case SQL_SUCCESS:       break;
                    case SQL_NO_DATA_FOUND: 
                        if( _debug )  LOG( "Odbc_file::open: has_result_set, field_as_file, no_data_found: column_count=" << _column_count << '\n' );
                        // ??? js 21.1.99: _eof = true; break; 
                        throw_not_exist_error( "SOS-1251" );
                    default:                check_error( retcode, "SQLFetch" );
                }
            } 
            else 
            {
                bind_columns();
            }
        } else {
            // ??? js 21.1.99: if ( _field_as_file ) throw_not_exist_error( "SOS-1251" );
        }
    }
}

//----------------------------------------------------------------------Odbc_file::execute_stmt

void Odbc_file::execute_stmt()
{
    RETCODE         retcode;
    Dynamic_area    hilfspuffer;

    _row_count = -1;


    // Mit SQLBindParameter gebundene Parameter:
    for( int i = 0; i <= _param_bindings.last_index(); i++ ) 
    {
        Sos_odbc_binding* b = &_param_bindings[ i ];
        Field_descr*      f = _param_type->field_descr_ptr( i );

        if( f->null( _param_base ) ) {
            b->_length = SQL_NULL_DATA;
        } else {
            copy_field( b->_field, _odbc_param_record.byte_ptr(), f, _param_base, &hilfspuffer );
            b->_length = b->_default_length;
            //b->_default_length = b->_field->type_ptr()->field_length( b->_field->ptr( _odbc_param_record.byte_ptr() ) );
            LOG( "Par " << i << " " << *b->_field << ": _default_length=" << b->_default_length << ", value=" );
            if( log_ptr )  b->_field->print( _odbc_param_record.byte_ptr(), log_ptr, std_text_format );
            LOG( "\n" );
        }
    }


    if( _debug )  *log_ptr << "SQLExecute(" << _hstmt << ")  _cbValue=" << _cbValue << endl;
    retcode = ODBC_LIB SQLExecute( _hstmt );


    if( _field_as_file  &&  retcode == SQL_NEED_DATA ) 
    {
        _sql_need_data = true;

/*
        SQLINTEGER row_count = -1;
        if( _debug )  *log_ptr << "SQLRowCount(" << _hstmt << ")  ";
        retcode = ODBC_LIB SQLRowCount( _hstmt, &row_count );

        _row_count = row_count;
        if( _debug )  if( retcode == SQL_SUCCESS )  *log_ptr << _row_count << endl;
                                            //else  log_retcode( retcode );  

        if( retcode != SQL_SUCCESS )  remove_errors();
        if( retcode == SQL_SUCCESS && _single && _row_count == 0 ) throw_not_found_error();
*/

        PTR ptr;
        if( _debug )  *log_ptr << "SQLParamData(" << _hstmt << ')' << endl;
        retcode = ODBC_LIB SQLParamData( _hstmt, &ptr );
        if( retcode != SQL_NEED_DATA ) {
            check_error( retcode, "SQLParamData" );
        }
    }
    else
    {
        if( retcode != SQL_NO_DATA_FOUND )  check_error( retcode, "SQLExecute" );  // SQL_NO_DATA_FOUND liefert SQLServer bei where 1=0

        SQLINTEGER row_count = -1;
        if( _debug )  *log_ptr << "SQLRowCount(" << _hstmt << ")  ";
        retcode = ODBC_LIB SQLRowCount( _hstmt, &row_count );

        _row_count = row_count; 
        _any_file_ptr->_record_count = _row_count;
        //check_error_hstmt( retcode, _hstmt, "SQLRowCount" );
        if( _debug )  if( retcode == SQL_SUCCESS )  *log_ptr << _row_count << endl;
                                              else  log_retcode( retcode );

        if( retcode != SQL_SUCCESS )  remove_errors();
        //if( retcode == SQL_SUCCESS && _assert_row_count != -1 && _row_count != -1 && _row_count != _assert_row_count )
        //    throw_xc( "SOS-1446", _row_count, _assert_row_count );
        //if( retcode == SQL_SUCCESS && _single && _row_count == 0 ) throw_not_found_error();

        _stmt_executed = true;
    }
}

//----------------------------------------------------------------------Odbc_file::bind_columns

void Odbc_file::bind_columns()
{
    RETCODE retcode;

    _record.allocate_min( _type->field_size() );

    for( int i = 1; i <= _result_bindings.last_index(); i++ )
    {
        Sos_odbc_binding* b = &_result_bindings[ i ];
        if( b->_field )
        {
            if( _debug )  *log_ptr << "SQLBindCol(" << _hstmt << ',' << i << ',' << name_of_ctype(b->_fCType) << ",," << b->_field->type().field_size() << ')' << endl;
            retcode = ODBC_LIB SQLBindCol( /*hstmt     */ _hstmt,
                                           /*icol      */ i,
                                           /*fCType    */ b->_fCType,
                                           /*rgbValue  */ b->_field->ptr( _record.byte_ptr() ),
                                           /*cbValueMax*/ b->_field->type().field_size(),
                                           /*pcbValue  */ &_cbValue_array[ i ]
                                          );

            const char func[] = "SQLBindCol/";
            string str = func; str += b->_field->name();
            check_error( retcode, c_str(str) );
        }
    }
}

// --------------------------------------------------------------------------Odbc_file::execute
/*
void Odbc_file::execute()
{
}
*/
// --------------------------------------------------------------------------- Odbc_file::close

void Odbc_file::close( Close_mode close_mode )
{
    RETCODE retcode;
    Bool reset = false;

    if ( _buffered_blob ) {
        _cbValue = SQL_LEN_DATA_AT_EXEC( _blob_filesize > 0 ? _blob_filesize : 1 ); // Jetzt wissen wir den Wert. Oder Dummy-Wert
        bind_blob_parameter();
        if( !_stmt_executed )  execute_stmt();
        if ( _blob_filesize > 0 ) put_blob_file();
        else _blob_data_count = 0; // Damit wir auf jeden Fall in den Reset-Fall unten kommen
    } 
    if( _sql_need_data ) 
    {
        PTR ptr;


        if( !_sqlputdata_called ) { // keine Daten geschrieben
            reset = _blob_data_count == 0 && !empty(_reset_statement);
            // !!! TODO: evtl. lieber SQLCancel aufrufen, statt Dummy-Wert zu schreiben und Reset-Statement auszuführen
            if( _debug )  *log_ptr << "SQLPutData(" << _hstmt << ",,0)" << endl;
            retcode = ODBC_LIB SQLPutData( _hstmt, (char*)( reset?"x":"" ), reset?1:0 );
            check_error( retcode, "SQLPutData" );
        }

        if( _debug )  *log_ptr << "SQLParamData(" << _hstmt << ')' << endl;
        retcode = ODBC_LIB SQLParamData( _hstmt, &ptr );  // Führt die Anweisung aus
        check_error( retcode, "SQLParamData" );

        if( _debug )  *log_ptr << "SQLRowCount(" << _hstmt << ")  ";
        SQLINTEGER row_count = -1;
        retcode = ODBC_LIB SQLRowCount( _hstmt, &row_count );

        _row_count = row_count;
        _any_file_ptr->_record_count = _row_count;

        if( _debug )  if( retcode == SQL_SUCCESS )  *log_ptr << _row_count << endl;
                                              else  log_retcode( retcode );

        //check_error_hstmt( retcode, _hstmt, "SQLRowCount" );
        if( retcode != SQL_SUCCESS )  remove_errors();

        if( _assert_row_count != -1 && retcode == SQL_SUCCESS && _row_count != -1 && _row_count != _assert_row_count )  throw_xc( "SOS-1446", _row_count, _assert_row_count );
        if ( retcode == SQL_SUCCESS && _single && _row_count == 0 ) throw_not_found_error();

    }
    if ( reset ) session()->_execute_direct( c_str(_reset_statement) ); // Blob auf NULL setzen

    if( _hstmt ) {
        _stmt_executed = false;  // jz 16.11.97
        //RETCODE retcode = SQLFreeStmt( _hstmt, SQL_DROP );
        //_hstmt = 0;
        if( _debug )  *log_ptr << "SQLFreeStmt(" << _hstmt << ",SQL_CLOSE)" << endl;
        retcode = ODBC_LIB SQLFreeStmt( _hstmt, SQL_CLOSE );
        check_error( retcode, "SQLFreeStmt" );
    }

    if( _commit  &&  close_mode == close_normal )  session()->commit();

    if( close_mode != close_cursor )  // jz 17.6.2002
    {
        if( _hstmt )
        {
            if( _debug )  LOG( "SQLFreeStmt(" << _hstmt << ",SQL_DROP)\n" );
            RETCODE retcode = ODBC_LIB SQLFreeStmt( _hstmt, SQL_DROP );
            check_error_hstmt( retcode, _hstmt, "SQLFreeStmt", _debug );
            _hstmt = 0;
        }

        session_disconnect();
    }

    //Nicht bei SQL_CLOSE, also wenn nur der Cursor geschlossen werden soll: session_disconnect();
}

// ---------------------------------------------------------------------- Odbc_file::put_record

void Odbc_file::put_record( const Const_area& record )
{
    if( _buffered_blob ) 
    { // Blob zwischenspeichern ...
        if( !_blob_file ) 
        { 
            _blob_file = sos_mkstemp();
        }

        int ret = write( _blob_file, record.ptr(), record.length() );
        if( ret < record.length() )  throw_errno( errno, "write" );

        _blob_filesize += record.length();
    } else if( _sql_need_data ) 
    {
        put_blob_record( record ); // direkt schreiben
    }
    else 
    {
        _session->execute_direct( record );
        _any_file_ptr->_record_count = _session->_row_count;
    }
}

// ----------------------------------------------------------------- Odbc_file::put_blob_record

void Odbc_file::put_blob_record( const Const_area& record )
{
    uint4 cbBytes          = record.length();
    const Byte* start      = record.byte_ptr();
    const uint4 byte_count = _odbc_default_max_length > 0? _odbc_default_max_length : 32768;


    if ( record.length() == 0 ) return; // Gibt sonst einen Oracle-Fehler: No data value pending.

    while ( cbBytes > 0 ) 
    {
        int len = min( byte_count, cbBytes );
        if( _debug )  *log_ptr << "SQLPutData(" << _hstmt << ",," << len << ")   gesamt=" << (_blob_data_count+len) << endl;
        
        RETCODE retcode = ODBC_LIB SQLPutData( _hstmt, (PTR)start, len );
        check_error( retcode, "SQLPutData" );

        cbBytes          -= len;
        _blob_data_count += len;
        start            += len;
    }



/*    
    while ( cbBytes > byte_count ) 
    {
        if( _debug )  *log_ptr << "SQLPutData(" << _hstmt << ",," << byte_count << ")   gesamt=" << (_blob_data_count+byte_count) << endl;
        RETCODE retcode = ODBC_LIB SQLPutData( _hstmt, (PTR)start, byte_count );
        check_error( retcode, "SQLPutData" );
        cbBytes          -= byte_count;
        _blob_data_count += byte_count;
        start            += byte_count;
    }

    if ( cbBytes > 0 ) { // Rest schreiben
        if( _debug )  *log_ptr << "SQLPutData(" << _hstmt << ",," << cbBytes << ")   gesamt=" << (_blob_data_count+cbBytes) << endl;
        RETCODE retcode = ODBC_LIB SQLPutData( _hstmt, (PTR)start, cbBytes );
        check_error( retcode, "SQLPutData" );
        _blob_data_count += cbBytes;
    }
*/
    _sqlputdata_called = true;

    //if ( _debug ) *log_ptr << "Odbc_file::put_record: SQLPutData written " << record.length() << " bytes" << ", total: " << _blob_data_count << endl;
}

// ---------------------------------------------------------------------- Odbc_file::put_blob_file

void Odbc_file::put_blob_file()
{
    Any_file blob_file;
    int ret;

    ret = lseek( _blob_file, 0, SEEK_SET );
    if( ret == -1 )  throw_errno( errno, "lseek" );

    while (1) 
    {
        char buffer [4096];
        ret = read( _blob_file, buffer, sizeof buffer );
        if( ret == -1 )  throw_errno( errno, "read" );
        if( ret == 0 )  break;
        put_blob_record( Const_area( buffer, ret ) );
    }

    ::close( _blob_file );
    _blob_file = 0;

    blob_file.close();
    if ( _debug ) *log_ptr << "Odbc_file::put_blob_file:: Blob written size=" << _blob_filesize << " bytes" << endl;
}

//----------------------------------------------------------------------- Odbc_file::get_record

void Odbc_file::get_record( Area& buffer )
{
    RETCODE retcode;

    if( _eof )  throw_eof_error();

    if( _field_as_file ) 
    {
        SDWORD len;

        if( _fixed_length == 0 )  throw_xc( "SOS-1334" );
        buffer.allocate_min( _fixed_length );    //???

      //if( _debug )  *log_ptr << "SQLGetData(" << _hstmt << ",1," << ( _open_mode & binary? "SQL_C_BINARY)" : "SQL_C_CHAR)" ) << endl;
        if( _debug )  *log_ptr << "SQLGetData(" << _hstmt << ",1," << name_of_ctype( _blob_ctype ) << ')' << endl;

        retcode = ODBC_LIB SQLGetData( _hstmt, 1, 
                                       //_open_mode & binary? SQL_C_BINARY : SQL_C_CHAR, 
                                       _blob_ctype,
                                       buffer.byte_ptr(), buffer.size(), &len );

        if( retcode == SQL_SUCCESS_WITH_INFO ) {
            Byte sql_state [ 5+1 ];
            sql_state[ 0 ] = '\0';
            call_SQLError( SQL_NULL_HENV, SQL_NULL_HDBC, _hstmt, sql_state, NULL, NULL, 0, NULL, _debug );
            if( memcmp( sql_state, "01004", 5 ) == 0 ) {   // Data truncated?
                len = buffer.size();
                //if( !(_open_mode & binary)  &&  len > 0  &&  buffer.byte_ptr()[ len-1 ] == '\0' )  len--;       // '\0' abschneiden  //jz 12.10.00  
                if( _blob_ctype != SQL_C_BINARY  &&  len > 0  &&  buffer.byte_ptr()[ len-1 ] == '\0' )  len--;       // '\0' abschneiden  //jz 12.10.00  
                
            } else {
                check_error( retcode, "SQLGetData" );
            }
        } else {
            _eof = true;
            check_error( retcode, "SQLGetData" );
        }

        if( len == SQL_NULL_DATA )  throw_eof_error(); //throw_null_error( "SOS-1335" );

        _blob_data_count += len;
        if ( _debug ) *log_ptr << "Odbc_file::get_record: SQLGetData read " << len << " bytes" << ", total: " << _blob_data_count << endl;

        //jz 8.10.00 if( /*Nicht bei Binärdaten! &&*/ len > 0  &&  buffer.byte_ptr()[ len-1 ] == '\0' )  len--;       // '\0' abschneiden

        buffer.length( len );
    }
    else 
    {
        if( !_type )  throw_xc( "SOS-1193", "odbc" );

        _record.allocate_min( _type->field_size() );
        memset( _record.ptr(), '\0', _type->field_size() );
    
        if( _debug )  *log_ptr << "SQLFetch(" << _hstmt << ')' << endl;
        retcode = ODBC_LIB SQLFetch( _hstmt );
    
        switch ( retcode ) {
            case SQL_SUCCESS:       break;
            case SQL_NO_DATA_FOUND: throw_eof_error( "D310" );
            default:                check_error( retcode, "SQLFetch" );
        }
    
        // Nullflags setzen:
        for ( int i=0; i < _type->field_count(); i++ ) {
            if( _cbValue_array[ i+1 ] == SQL_NULL_DATA ) {
                _type->field_descr_ptr( i )->set_null( _record.byte_ptr() );
            }
        }
    
        _record.length( _type->field_size() );
    
        buffer.assign( _record );
    }
}


} //namespace sos

#endif
