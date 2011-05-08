//$Id$
//#define MODULE_NAME "odbcconn"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
# else
    const int TRUE = 1;
    const int FALSE = 0;
#endif


#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h> // ODBC_CONFIG_DSN in SQLDriverConnect
#include "precomp.h"

#include <string.h>     // strstr()
#include <ctype.h>
#include <limits.h>
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/licence.h"
#include "../kram/sosprof.h"
#include "../kram/sossql.h"
#include "sosodbc.h"
#include "sosodbc_version.h"

//#define INT_MAX 32767

using namespace std;
namespace sos {

//--------------------------------------------------------------------const supported_functions

//	This is the list of all the functions supported / not supported by this
//	driver. This information is returned by SQLGetFunctions

static const UWORD supported_functions[100] =
{
	FALSE,		// Not used
	TRUE,		// SQL_API_SQLALLOCCONNECT
	TRUE,		// SQL_API_SQLALLOCENV
	TRUE,		// SQL_API_SQLALLOCSTMT
	TRUE,		// SQL_API_SQLBINDCOL
	TRUE,		// SQL_API_SQLCANCEL
	TRUE,		// SQL_API_SQLCOLATTRIBUTES
	TRUE,		// SQL_API_SQLCONNECT
	TRUE,		// SQL_API_SQLDESCRIBECOL
	TRUE,		// SQL_API_SQLDISCONNECT
	TRUE,		// SQL_API_SQLERROR (10)
	TRUE,		// SQL_API_SQLEXECDIRECT
	TRUE,		// SQL_API_SQLEXECUTE
	TRUE,		// SQL_API_SQLFETCH
	TRUE,		// SQL_API_SQLFREECONNECT
	TRUE,		// SQL_API_SQLFREEENV
	TRUE,		// SQL_API_SQLFREESTMT
 	FALSE,		// SQL_API_SQLGETCURSORNAME
	TRUE,		// SQL_API_SQLNUMRESULTCOLS
	TRUE,		// SQL_API_SQLPREPARE
	TRUE,		// SQL_API_SQLROWCOUNT (20)
	FALSE,		// SQL_API_SQLSETCURSORNAME
 	FALSE,		// SQL_API_SQLSETPARAM
	FALSE,		// SQL_API_SQLTRANSACT


	FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // Unused (30)
	FALSE, FALSE, FALSE, FALSE, FALSE, // Unused
	FALSE, FALSE, FALSE, FALSE, // Unused

	TRUE,		// SQL_API_SQLCOLUMNS (40)
	TRUE,		// SQL_API_SQLDRIVERCONNECT
	FALSE,		// SQL_API_SQLGETCONNECTOPTION

	TRUE,		// SQL_API_SQLGETDATA
	TRUE,		// SQL_API_SQLGETFUNCTIONS
	TRUE,		// SQL_API_SQLGETINFO
	TRUE,		// SQL_API_SQLGETSTMTOPTION
	TRUE,		// SQL_API_SQLGETTYPEINFO
	TRUE,		// SQL_API_SQLPARAMDATA
	TRUE,		// SQL_API_SQLPUTDATA
	TRUE,		// SQL_API_SQLSETCONNECTOPTION (50)

	TRUE,		// SQL_API_SQLSETSTMTOPTION    51
	TRUE,		// SQL_API_SQLSPECIALCOLUMNS   52
	TRUE,		// SQL_API_SQLSTATISTICS       53
	TRUE,		// SQL_API_SQLTABLES           54

	FALSE,		// SQL_API_SQLBROWSECONNECT    55    /* Level 2 Functions        */
	FALSE,		// SQL_API_SQLCOLUMNPRIVILEGES 56
	FALSE,		// SQL_API_SQLDATASOURCES      57
	FALSE,		// SQL_API_SQLDESCRIBEPARAM    58
	FALSE,		// SQL_API_SQLEXTENDEDFETCH    59
	FALSE,		// SQL_API_SQLFOREIGNKEYS      60

	TRUE,		// SQL_API_SQLMORERESULTS      61
	FALSE,		// SQL_API_SQLNATIVESQL        62
	FALSE,		// SQL_API_SQLNUMPARAMS        63
	FALSE,		// SQL_API_SQLPARAMOPTIONS     64
	FALSE,		// SQL_API_SQLPRIMARYKEYS      65
	FALSE,		// SQL_API_SQLPROCEDURECOLUMNS 66
	FALSE,		// SQL_API_SQLPROCEDURES       67
	FALSE,		// SQL_API_SQLSETPOS           68
	FALSE,		// SQL_API_SQLSETSCROLLOPTIONS 69
	FALSE,		// SQL_API_SQLTABLEPRIVILEGES  70

	FALSE,		// SQL_API_SQLDRIVERS          71
	TRUE,		// SQL_API_SQLBINDPARAMETER	   72
	FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // Unused (80)
	FALSE, FALSE, FALSE, FALSE, FALSE, // Unused
	FALSE, FALSE, FALSE, FALSE, FALSE, // Unused (90)
	FALSE, FALSE, FALSE, FALSE, FALSE, // Unused
	FALSE, FALSE, FALSE, FALSE 		   // Unused (99)
};

//---------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------SQLGetInfo

#define SQL_INFO_STRING(   INFO_TYPE, VALUE )  { INFO_TYPE, #INFO_TYPE, (long)VALUE, k_string }
#define SQL_INFO_SWORD(    INFO_TYPE, VALUE )  { INFO_TYPE, #INFO_TYPE, (long)VALUE, k_sword  }
#define SQL_INFO_DWORD(    INFO_TYPE, VALUE )  { INFO_TYPE, #INFO_TYPE, (long)VALUE, k_dword  }

enum Sql_info_kind { k_string, k_dword, k_sword };

struct Sql_info
{
    UWORD                      _fInfoType;
    const char*                _info_type_name;
    long                       _value;
    int                        _kind;
}
sql_info_table[] =
{
    SQL_INFO_STRING( SQL_ACCESSIBLE_PROCEDURES, "N" ),  // "Y" if the user can execute all procedures returned by SQLProcedures
    SQL_INFO_STRING( SQL_ACCESSIBLE_TABLES    , "Y" ),  // "Y" if the user is guaranteed SELECT privileges to all tables returned by SQLTables
	SQL_INFO_SWORD ( SQL_ACTIVE_CONNECTIONS   ,   0 ),
    SQL_INFO_SWORD ( SQL_ACTIVE_STATEMENTS    ,   0 ),  // the maximum number of active hstmts
    SQL_INFO_DWORD ( SQL_ALTER_TABLE          ,   0 ),
    SQL_INFO_DWORD ( SQL_AGGREGATE_FUNCTIONS  , SQL_AF_AVG | SQL_AF_COUNT | SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM ),
    SQL_INFO_DWORD ( SQL_BOOKMARK_PERSISTENCE ,   0 ),
    SQL_INFO_STRING( SQL_COLUMN_ALIAS         , "Y" ),  // Ist das gemeint: select expr alias, ...
    SQL_INFO_SWORD ( SQL_CONCAT_NULL_BEHAVIOR , SQL_CB_NULL ),    // oder SQL_CB_NON_NULL ?
    SQL_INFO_DWORD ( SQL_CONVERT_BIGINT       , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_BINARY       , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_BIT          , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_CHAR         , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_DATE         , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_DECIMAL      , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_DOUBLE       , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_FLOAT        , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_INTEGER      , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_LONGVARBINARY, 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_LONGVARCHAR  , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_NUMERIC      , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_REAL         , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_SMALLINT     , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_TIME         , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_TIMESTAMP    , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_TINYINT      , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_VARBINARY    , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_VARCHAR      , 0x00000000  ),
    SQL_INFO_DWORD ( SQL_CONVERT_FUNCTIONS          , 0           ),        // SQL_FN_CVT_CONVERT
    SQL_INFO_SWORD ( SQL_CORRELATION_NAME           , SQL_CN_ANY        ),
    SQL_INFO_SWORD ( SQL_CURSOR_COMMIT_BEHAVIOR     , SQL_CB_PRESERVE ),  // Transaction Tracking null behaviour
    SQL_INFO_SWORD ( SQL_CURSOR_ROLLBACK_BEHAVIOR   , SQL_CB_PRESERVE ),  // Transaction Tracking null behaviour
    SQL_INFO_STRING( SQL_DATA_SOURCE_NAME           , ""                ),  // variabel
    SQL_INFO_STRING( SQL_DATA_SOURCE_READ_ONLY      , ""                ),  // variabel
    SQL_INFO_STRING( SQL_DBMS_NAME                  , "hostODBC"    ),       // "FS", "BS2000", "LEASY" ?
    SQL_INFO_STRING( SQL_DBMS_VER                   , "00.00.0000"  ),       // Fileserver-, BS2000- oder Leasy-Version
    SQL_INFO_DWORD ( SQL_DEFAULT_TXN_ISOLATION      , 0             ),       // Transaktionen werden nicht unterstützt
    SQL_INFO_STRING( SQL_DRIVER_NAME                , VER_NAME_STR  ),
    SQL_INFO_STRING( SQL_DRIVER_ODBC_VER            , "02.00"       ),
    SQL_INFO_STRING( SQL_DRIVER_VER                 , VER_PRODUCTVERSION_STR ),
    SQL_INFO_STRING( SQL_EXPRESSIONS_IN_ORDERBY     , "Y"           ),
    SQL_INFO_DWORD ( SQL_FETCH_DIRECTION            , SQL_FD_FETCH_NEXT     ),
    SQL_INFO_SWORD ( SQL_FILE_USAGE                 , SQL_FILE_NOT_SUPPORTED ), // Führt zum Datei-Öffnen-Dialog: SQL_FILE_TABLE        ),
    SQL_INFO_DWORD ( SQL_GETDATA_EXTENSIONS         , SQL_GD_ANY_COLUMN
                                                    | SQL_GD_ANY_ORDER
                                                    | SQL_GD_BOUND          ),
    SQL_INFO_SWORD ( SQL_GROUP_BY                   , SQL_GB_NO_RELATION    ),
    SQL_INFO_SWORD ( SQL_IDENTIFIER_CASE            , SQL_IC_MIXED          ),  // cobol-type: SQL_IC_UPPER
  //SQL_INFO_STRING( SQL_IDENTIFIER_QUOTE_CHAR      , "‘"                   ),  // MS Access 2.0: -7755 Spezifikationsfehler
    SQL_INFO_STRING( SQL_IDENTIFIER_QUOTE_CHAR      , sql_identifier_quote_string ),
    SQL_INFO_STRING( SQL_KEYWORDS                   , ""                    ),  // s.u. SQL_ODBC_KEYWORDS ",LOOP" ),
    SQL_INFO_STRING( SQL_LIKE_ESCAPE_CLAUSE         , "N"                   ),
    SQL_INFO_DWORD ( SQL_LOCK_TYPES                 , SQL_LOCK_NO_CHANGE    ),
    SQL_INFO_DWORD ( SQL_MAX_BINARY_LITERAL_LEN     , 0                     ),
    SQL_INFO_DWORD ( SQL_MAX_CHAR_LITERAL_LEN       , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_COLUMN_NAME_LEN        , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_COLUMNS_IN_GROUP_BY    , max_groupby_params    ),
    SQL_INFO_SWORD ( SQL_MAX_COLUMNS_IN_INDEX       , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_COLUMNS_IN_ORDER_BY    , max_orderby_params    ),
    SQL_INFO_SWORD ( SQL_MAX_COLUMNS_IN_SELECT      , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_COLUMNS_IN_TABLE       , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_CURSOR_NAME_LEN        , 0                     ),
    SQL_INFO_DWORD ( SQL_MAX_INDEX_SIZE             , 256                   ),  // BS2000
    SQL_INFO_SWORD ( SQL_MAX_OWNER_NAME_LEN         , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_PROCEDURE_NAME_LEN     , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_QUALIFIER_NAME_LEN     , 0                     ),
    SQL_INFO_DWORD ( SQL_MAX_ROW_SIZE               , 32767L                ),
    SQL_INFO_STRING( SQL_MAX_ROW_SIZE_INCLUDES_LONG , "Y"                   ),
    SQL_INFO_DWORD ( SQL_MAX_STATEMENT_LEN          , INT_MAX               ),
    SQL_INFO_SWORD ( SQL_MAX_TABLE_NAME_LEN         , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_TABLES_IN_SELECT       , 0                     ),
    SQL_INFO_SWORD ( SQL_MAX_USER_NAME_LEN          , 0                     ),
    SQL_INFO_STRING( SQL_MULT_RESULT_SETS           , "Y"                   ),
    SQL_INFO_STRING( SQL_MULTIPLE_ACTIVE_TXN        , "N"                   ),  // ?
    SQL_INFO_STRING( SQL_NEED_LONG_DATA_LEN         , "N"                   ),
    SQL_INFO_SWORD ( SQL_NON_NULLABLE_COLUMNS       , SQL_NNC_NON_NULL      ),  // ?
    SQL_INFO_SWORD ( SQL_NULL_COLLATION             , SQL_NC_LOW            ),  // ?
    SQL_INFO_DWORD ( SQL_NUMERIC_FUNCTIONS          , 0                     ),
    SQL_INFO_SWORD ( SQL_ODBC_API_CONFORMANCE       , SQL_OAC_LEVEL1        ),
    SQL_INFO_SWORD ( SQL_ODBC_SAG_CLI_CONFORMANCE   , SQL_OSCC_NOT_COMPLIANT ),
    SQL_INFO_SWORD ( SQL_ODBC_SQL_CONFORMANCE       , SQL_OSC_CORE          ),  // STIMMT NICHT!
    SQL_INFO_SWORD ( SQL_ODBC_SQL_OPT_IEF           , "N"                   ),  // integrity enhancement facility
    SQL_INFO_STRING( SQL_ORDER_BY_COLUMNS_IN_SELECT , "N"                   ),
    SQL_INFO_STRING( SQL_OUTER_JOINS                , "N"                   ),
    SQL_INFO_STRING( SQL_OWNER_TERM                 , "owner"               ),  // ?
    SQL_INFO_DWORD ( SQL_OWNER_USAGE                , 0L                    ),  // SQL_OU_DML_STATEMENTS
    SQL_INFO_DWORD ( SQL_POS_OPERATIONS             , 0L                    ),
    SQL_INFO_DWORD ( SQL_POSITIONED_STATEMENTS      , 0                     ),
    SQL_INFO_STRING( SQL_PROCEDURE_TERM             , "procedure"           ),
    SQL_INFO_STRING( SQL_PROCEDURES                 , "N"                   ),
    SQL_INFO_SWORD ( SQL_QUALIFIER_LOCATION         , SQL_QL_START          ),
    SQL_INFO_STRING( SQL_QUALIFIER_NAME_SEPARATOR   , "."                   ),  // ?
    SQL_INFO_STRING( SQL_QUALIFIER_TERM             , "qualifier"           ),
    SQL_INFO_DWORD ( SQL_QUALIFIER_USAGE            , 0                     ),  // SQL_QU_DML_STATEMENTS
    SQL_INFO_SWORD ( SQL_QUOTED_IDENTIFIER_CASE     , SQL_IC_MIXED          ),  // SQL_IC_UPPER für Cobol
    SQL_INFO_STRING( SQL_ROW_UPDATES                , "N"                   ),
    SQL_INFO_DWORD ( SQL_SCROLL_CONCURRENCY         , 0                     ),
    SQL_INFO_DWORD ( SQL_SCROLL_OPTIONS             , SQL_SO_FORWARD_ONLY   ),
    SQL_INFO_STRING( SQL_SEARCH_PATTERN_ESCAPE      , ""                    ),
    SQL_INFO_STRING( SQL_SERVER_NAME                , ""                    ), // BS2000-Hostname?
    SQL_INFO_STRING( SQL_SPECIAL_CHARACTERS         , ""                    ), // "$@#"
    SQL_INFO_DWORD ( SQL_STATIC_SENSITIVITY         , 0                     ), // scrollable cursor
    SQL_INFO_DWORD ( SQL_STRING_FUNCTIONS           , SQL_FN_STR_CONCAT
                                                    | SQL_FN_STR_SUBSTRING  ),
/*                                                    SQL_FN_STR_ASCII
                                                      SQL_FN_STR_CHAR
                                                      SQL_FN_STR_CONCAT
                                                      SQL_FN_STR_DIFFERENCE
                                                      SQL_FN_STR_INSERT
                                                      SQL_FN_STR_LCASE
                                                      SQL_FN_STR_LEFT
                                                      SQL_FN_STR_LENGTH
                                                      SQL_FN_STR_LOCATE
                                                      SQL_FN_STR_LOCATE_2
                                                      SQL_FN_STR_LTRIM
                                                      SQL_FN_STR_REPEAT
                                                      SQL_FN_STR_REPLACE
                                                      SQL_FN_STR_RIGHT
                                                      SQL_FN_STR_RTRIM
                                                      SQL_FN_STR_SOUNDEX
                                                      SQL_FN_STR_SPACE
                                                      SQL_FN_STR_SUBSTRING
                                                      SQL_FN_STR_UCASE
*/
    SQL_INFO_DWORD ( SQL_SUBQUERIES                 , SQL_SQ_COMPARISON | SQL_SQ_IN | SQL_SQ_QUANTIFIED ),
    SQL_INFO_DWORD ( SQL_SYSTEM_FUNCTIONS           , SQL_FN_SYS_IFNULL     ),
    SQL_INFO_STRING( SQL_TABLE_TERM                 , "table"               ),
    SQL_INFO_DWORD ( SQL_TIMEDATE_ADD_INTERVALS     , 0                     ),
    SQL_INFO_DWORD ( SQL_TIMEDATE_DIFF_INTERVALS    , 0                     ),
    SQL_INFO_DWORD ( SQL_TIMEDATE_FUNCTIONS         , 0                     ),
    SQL_INFO_SWORD ( SQL_TXN_CAPABLE                , SQL_TC_NONE           ),
    SQL_INFO_SWORD ( SQL_TXN_ISOLATION_OPTION       , SQL_TXN_READ_UNCOMMITTED ),
    SQL_INFO_DWORD ( SQL_UNION                      , 0                     ),
    SQL_INFO_STRING( SQL_USER_NAME                  , ""                    )       // variabel
//,{ -1, "ENDE", (long)0, k_dword }

};

//------------------------------------------------------------------------------SQLAllocConnect

extern "C"
RETCODE SQL_API SQLAllocConnect( HENV henv, HDBC FAR* phdbc )
{
    ODBC_LOG( "SQLAllocConnect()\n");

    try {
        if( !SOS_LICENCE( licence_hostodbc ) )    throw_xc( "SOS-1000", "hostODBC" );

        sos_static_ptr()->_licence->set_par( licence_sossql );  // SQL-Prozessor erlauben

        Sos_odbc_connection* conn = new Sos_odbc_connection;
        conn->_env = (Sos_odbc_env*)henv;
        conn->_open_mode = Any_file::inout;

        *phdbc = (HDBC*)conn;
    }
    catch( const Xc& x ) {
        SHOW_MSG( "Fehler bei der Benutzung von hostODBC:\n" << x );    // Access 7 zeigt den Fehlertext nicht an (besonders SOS-1000 Lizenz)
        return ((Sos_odbc_env*)henv)->sql_error( "S1000", x );
    }
	return SQL_SUCCESS;
}

//-------------------------------------------------------------------------------SQLFreeConnect

extern "C"
RETCODE SQL_API SQLFreeConnect( HDBC hdbc )
{
    ODBC_LOG( "SQLFreeConnect()\n" );
    Sos_odbc_connection* c = (Sos_odbc_connection*)hdbc;
    c->_sqlstate = "";

    //Sos_odbc_env* env = c->_env;

    //((Sos_odbc_connection*)hdbc)->_file.close();   VORSICHT Exceptions
    delete (Sos_odbc_connection*)hdbc;

    //jz 2.11.96 ?  delete env;     // SQLFreeEnv wird ja nicht aufgerufen...

    //LOG( "sos_static_ptr()->_ref_count=" << sos_static_ptr()->_ref_count << '\n' );
	return SQL_SUCCESS;
}

//------------------------------------------------------------------------------SQLGetFunctions

extern "C"
RETCODE SQL_API SQLGetFunctions(
	HDBC      hdbc,
	UWORD     fFunction,
	UWORD FAR *pfExists)
{
    Sos_odbc_connection* c = (Sos_odbc_connection*)hdbc;
    c->_sqlstate = "";

    if (fFunction == SQL_API_ALL_FUNCTIONS)
        memcpy( pfExists, supported_functions, sizeof supported_functions );
    else
        *pfExists = supported_functions[ fFunction ];

    return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------------SQLGetInfo

DEFINE_ODBC_CALL_4( Sos_odbc_connection, SQLGetInfo, HDBC,
                    UWORD, PTR, SWORD, SWORD* );

RETCODE Sos_odbc_connection::SQLGetInfo(
    HDBC      hdbc,
    UWORD     fInfoType,
    PTR       rgbInfoValue,
    SWORD     cbInfoValueMax,
    SWORD FAR *pcbInfoValue )                                                       
{
    Sos_odbc_connection* c = (Sos_odbc_connection*)hdbc;

    switch (fInfoType)
    {
        case SQL_DATABASE_NAME:
        {
            bool ok = return_odbc_string0( rgbInfoValue, cbInfoValueMax, pcbInfoValue, c->_data_source_name );
            if( ok )  ODBC_LOG( "SQL_DATABASE_NAME: \"" << (char*)rgbInfoValue << "\"\n" );
            return SQL_SUCCESS;
        }

        case SQL_DATA_SOURCE_NAME:
        {
            bool ok = return_odbc_string0( rgbInfoValue, cbInfoValueMax, pcbInfoValue, c->_data_source_name );
            if( ok )  ODBC_LOG( "SQL_DATA_SOURCE_NAME: \"" << (char*)rgbInfoValue << "\"\n" );
            return SQL_SUCCESS;
        }

        case SQL_DATA_SOURCE_READ_ONLY:
        {
            bool ok = return_odbc_string0( rgbInfoValue, cbInfoValueMax, pcbInfoValue,
                                           c->_open_mode & Any_file::out? "N" : "Y" );
            if( ok )  ODBC_LOG( "SQL_DATA_SOURCE_READ_ONLY: \"" << (char*)rgbInfoValue << "\"\n" );
            return SQL_SUCCESS;
        }

        case SQL_KEYWORDS:
        {
            Bool       comma = false;
            Sos_string keywords;
            Sos_string ks;
            Sos_string odbc_keywords = ",";

            odbc_keywords += SQL_ODBC_KEYWORDS;
            odbc_keywords += ",";

            for( int i = 0; i < sql_token_table_size; i++ ) 
            {
                const char* k = sql_token_table[ i ]._name;
                if( isalpha( k[0] ) ) {
                    ks = ",";
                    ks += k;
                    ks += ",";
                    if( !strstr( c_str( odbc_keywords ), c_str( ks ) ) ) {
                        if( comma )  keywords += ',';
                        comma = true;
                        keywords += k;
                    }
                }
            }

            return_odbc_string0( rgbInfoValue, cbInfoValueMax, pcbInfoValue, c_str( keywords ) );
            return SQL_SUCCESS;
        }

      //case SQL_USER_NAME:       ??

        default: break;
    }

    Sql_info* p = sql_info_table;

    while( p < sql_info_table + NO_OF( sql_info_table ) ) {
        if( fInfoType == p->_fInfoType )  goto FOUND;
        p++;
    }

    ODBC_LOG( fInfoType << '\n' );
    return sql_error( "S1C00" );

  FOUND:
    ODBC_LOG( p->_info_type_name );

    switch( p->_kind )
    {
        case k_string: return_odbc_string0( rgbInfoValue, cbInfoValueMax, pcbInfoValue,
                                            (const char*)p->_value );
                       ODBC_LOG( ": \"" << (char*)rgbInfoValue << "\"\n" );
                       break;

        case k_dword:  *(DWORD*)rgbInfoValue = p->_value;
                       if( pcbInfoValue )  *pcbInfoValue = sizeof (DWORD);
                       ODBC_LOG( ": " << p->_value << "\n" );
                       break;

        case k_sword:  *(SWORD*)rgbInfoValue = (SWORD)p->_value;
                       if( pcbInfoValue )  *pcbInfoValue = sizeof (SWORD);
                       ODBC_LOG( ": " << p->_value << "\n" );
                       break;
    }

    return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------------SQLConnect

DEFINE_ODBC_CALL_6( Sos_odbc_connection, SQLConnect, HDBC,
                    UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD );

RETCODE Sos_odbc_connection::SQLConnect
(
    HDBC,
    UCHAR FAR*      data_source_name,
    SWORD           data_source_name_length,
	UCHAR FAR*      user_name_ptr,
	SWORD	        user_name_length,
	UCHAR FAR*      password_ptr,
	SWORD	        password_length
)
{
    Sos_string filename;
    _sqlstate = "";

    static Bool profile_read;
    static Bool sqlconnect_allowed;

    if( !profile_read )  sqlconnect_allowed = read_profile_bool( "", "sosodbc", "SQLConnect_allowed", true );
    if( !sqlconnect_allowed )  throw_xc( "SOS-1376", "SQLConnect" );

    return connect( odbc_as_string( data_source_name, data_source_name_length ),
                    odbc_as_string( user_name_ptr, user_name_length ),
                    odbc_as_string( password_ptr, password_length ),
                    empty_string );
}

//--------------------------------------------------------------------------------------connect

RETCODE Sos_odbc_connection::connect( const Sos_string& data_source_name, const Sos_string& user_name, 
                                      const Sos_string& password, const Sos_string& catalog )
{
    Sos_string filename;

    _data_source_name = data_source_name;
    //_data_source_name darf leer sein, dann verwendet sossql den Abschnitt [alias]

    _dbms_param = "";
    append_option( &_dbms_param, " -db="     , _data_source_name );
    append_option( &_dbms_param, " -catalog=", catalog );
    append_option( &_dbms_param, " -user="   , user_name );

    filename  = "sossql ";
    filename += _dbms_param;
    append_option( &filename, " -password=", password );

    _file.open( filename, _open_mode );

	return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------SQLBrowseConnect

//	This function as its "normal" behavior is supposed to bring up a
//	dialog box if it isn't given enough information via "szConnStrIn".  If
//	it is given enough information, it's supposed to use "szConnStrIn" to
//	establish a database connection.  In either case, it returns a
//	string to the user that is the string that was eventually used to
//	establish the connection.

extern "C"
RETCODE SQL_API SQLBrowseConnect(
	HDBC	  hdbc,
	UCHAR FAR *szConnStrIn,
	SWORD	  cbConnStrIn,
	UCHAR FAR *szConnStrOut,
	SWORD	  cbConnStrOutMax,
	SWORD FAR *pcbConnStrOut)
{
    ODBC_LOG( "SQLBrowseConnect()\n");
    Sos_odbc_connection* c = (Sos_odbc_connection*)hdbc;
    c->_sqlstate = "";

	return SQL_ERROR;
}

//-----------------------------------------------------------------------------SQLDriverConnect
// aus odbcstup.cxx
#define ODBC_DRIVERCONNECT_DSN 4
extern BOOL SosConfigDSN( HWND hwnd, WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes, Sos_string* dsn_ptr, Sos_string* cat_ptr );


DEFINE_ODBC_CALL_7( Sos_odbc_connection, SQLDriverConnect, HDBC,
                    HWND, UCHAR*, SWORD, UCHAR*, SWORD, SWORD*, UWORD );


RETCODE Sos_odbc_connection::SQLDriverConnect(
    HDBC      hdbc,
	HWND	  hwnd,
	UCHAR FAR*szConnStrIn,
	SWORD 	  cbConnStrIn,
	UCHAR FAR*szConnStrOut,
	SWORD	  cbConnStrOutMax,
	SWORD FAR*pcbConnStrOut,
	UWORD	  fDriverCompletion)
{
    Sos_string dsn;
    Sos_string uid;
    Sos_string pwd;
    Sos_string catalog;
    Sos_string driver;
    Sos_string driver_name;
    Sos_string completed_connect_string;
    Sos_string connect_string;
    RETCODE    retcode;

    ODBC_LOG( "SQLDriverConnect()\n");

    Sos_odbc_connection* conn = (Sos_odbc_connection*)hdbc;

    connect_string  = odbc_as_string( szConnStrIn, cbConnStrIn );

    conn->_sqlstate = "00000";

    ODBC_LOG( "    connect_string=" << connect_string << '\n' );

    Bool       dsn_set = false;
    Bool       db_set  = false;
    Bool       uid_set = false;
    Bool       pwd_set = false;
    Bool       catalog_set = false;
    Bool       driver_set = false;
    const char* p     = c_str( connect_string );
    const char* p_end = p + length( connect_string );

    // "DSN=test;UID=Admin;PWD="
    while( *p ) {
        const char* n = (char*)memchr( p, ';', p_end - p );
        if( !n )  n = p_end;

        if( p + 4 <= p_end  &&  strnicmp( p, "DSN=", 4 ) == 0 ) {
            p += 4;
            if( !dsn_set )  dsn = as_string( p, n - p );
            dsn_set = true;
        }
        else
        if( p + 4 <= p_end  &&  strnicmp( p, "UID=", 4 ) == 0 ) {
            p += 4;
            if( !uid_set )  uid = as_string( p, n - p );
            uid_set = true;
        }
        else
        if( p + 4 <= p_end  &&  strnicmp( p, "PWD=", 4 ) == 0 ) {
            p += 4;
            if( !pwd_set )  pwd = as_string( p, n - p );
            pwd_set = true;
        }
        else
        if( p + 3 <= p_end  &&  strnicmp( p, "DB=", 3 ) == 0 ) {  // DB= wird im Gegensatz zu DSN= nicht vom Driver Manager verarbeitet.
            p += 3;
            if( !dsn_set )  dsn = as_string( p, n - p );
            db_set = true;
        }
        else
        if( p + 8 <= p_end  &&  strnicmp( p, "CATALOG=", 8 ) == 0 ) {
            p += 8;
            if( !catalog_set ) {
                catalog = as_string( p, n - p );
                if( p < n  &&  catalog[ 0 ] == '{' )  catalog = as_string( c_str( catalog ) + 1, length( catalog ) - 2 );
            }
            catalog_set = true;
        }
        else
        if( p + 7 <= p_end  &&  strnicmp( p, "DRIVER=", 7 ) == 0 ) {
            p += 7;
            if( !driver_set ) {
                driver = as_string( p, n - p );  // Mit '{' und '}'
                driver_name = driver;
                if( driver_name[ 0 ] == '{'  &&  length( driver_name ) >= 2  ) {
                    driver_name = as_string( c_str( driver_name ) + 1, length( driver_name ) - 2 );
                }
            }
            driver_set = true;
        }
        else
        {
            LOG( "Nicht erkannt: \"" << Const_area( p, n - p ) << "\"\n" );
            // Nicht erkanntes Schlüsselwort: SQL_SUCCESS_WITH_INFO 01S00
        }

        p = n;
        if( *p == ';' )  p++;
    }



    // Für uns reichen die Schlüsselwörter DSN oder DB
    if (fDriverCompletion == SQL_DRIVER_PROMPT || (dsn == "" && catalog == "")  ) {
    PROMPT:
        if( fDriverCompletion == SQL_DRIVER_NOPROMPT )  { conn->_sqlstate = "S1000"; return SQL_ERROR; }

        BOOL success;

#       if defined SYSTEM_WIN
            success = SosConfigDSN(hwnd,ODBC_DRIVERCONNECT_DSN,c_str(driver_name),"",&dsn,&catalog);
            //jz 9.12.97 SosConfigDSN liefert nicht Bool, sondern int. Ist das richtig? Ja.
#        else
            success = false;
#       endif

        if ( !success ) {
            // Abbruch
            return SQL_NO_DATA_FOUND;
        }

        if ( dsn == "" ) {
            dsn_set     = false;
            db_set      = false;
            catalog_set = true;
        } else {
            db_set      = !dsn_set;
            catalog_set = false;
        }
    }

    retcode = conn->connect( dsn, uid, pwd, catalog );

    if ( retcode == SQL_ERROR  &&  fDriverCompletion != SQL_DRIVER_NOPROMPT )
    {
        UCHAR        szSqlState     [ SQL_SQLSTATE_SIZE+1 ];
        Dynamic_area text           ( SQL_MAX_MESSAGE_LENGTH );
        SWORD        cbErrorMsg     = 0;
        SDWORD       NativeError    = 0;
        RETCODE      ret;

        memset( szSqlState, 0, sizeof szSqlState );

        ret = ::SQLError( NULL, hdbc, NULL, szSqlState, &NativeError,
                          text.byte_ptr(), text.size(), &cbErrorMsg );
        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) {
            text.length( cbErrorMsg );
            SHOW_MSG( "Fehler bei SQLConnect: state=" << szSqlState << ", text='" << text << "'" );
        } else {
            conn->_sqlstate = "S1000";
            return SQL_ERROR;
        }
        // Dialogbox nochmal ausführen ...
        goto PROMPT;
    }


    // completed_connect_string zusammenbauen
    if( driver_set ) {
        completed_connect_string += "DRIVER=";
        completed_connect_string += driver;
        completed_connect_string += ';';
    }
    if( db_set ) {
        completed_connect_string += "DB=";
        completed_connect_string += dsn;
        completed_connect_string += ';';
    }
    if( catalog_set ) {
        completed_connect_string += "CATALOG=";
        completed_connect_string += catalog;
        completed_connect_string += ';';
    }
    if( dsn_set ) {
        completed_connect_string += "DSN=";
        completed_connect_string += dsn;
        completed_connect_string += ';';
    }
    if( uid_set ) {
        completed_connect_string += "UID=";
        completed_connect_string += uid;
        completed_connect_string += ';';
    }
    if( pwd_set ) {
        completed_connect_string += "PWD=";
        completed_connect_string += pwd;
        completed_connect_string += ';';
    }

    LOG( "Neuer Connect-String: " << completed_connect_string << '\n' );
    return_odbc_string0( szConnStrOut, cbConnStrOutMax, pcbConnStrOut, completed_connect_string );
    return retcode;
}

//---------------------------------------------------------------------------FDriverConnectProc
#if defined SYSTEM_WIN

extern "C"
BOOL FAR PASCAL FDriverConnectProc(
	HWND	hdlg,
	WORD	wMsg,
	WPARAM  wParam,
	LPARAM  lParam)
{
    ODBC_LOG( "FDriverConnectProc()\n" );

    return FALSE;
/*jz
	switch (wMsg) {
	case WM_INITDIAODBC_LOG:
		Ctl3dRegister (s_hModule);
#ifdef WIN32
		Ctl3dSubclassDlg(hdlg, CTL3D_ALL);
#else
		Ctl3dSubclassDlgEx(hdlg, CTL3D_ALL);
#endif
		return TRUE;

#ifdef WIN32
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
#endif

	case WM_SYSCOLORCHANGE:
		return Ctl3dColorChange();

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:

		case IDCANCEL:
			Ctl3dUnregister (s_hModule);
			EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
			return TRUE;
		}
	}
	return FALSE;
*/
}

#endif
//--------------------------------------------------------------------------------SQLDisconnect

DEFINE_ODBC_CALL_0( Sos_odbc_connection, SQLDisconnect, HDBC );

RETCODE Sos_odbc_connection::SQLDisconnect( HDBC )
{
    ODBC_LOG( "SQLDisconnect()\n" );

    Sos_odbc_connection* c = (Sos_odbc_connection*)this;
    c->_sqlstate = "";
    c->_file.close();

	return SQL_SUCCESS;
}

//--------------------------------------------------------------------------SQLSetConnectOption

DEFINE_ODBC_CALL_2( Sos_odbc_connection, SQLSetConnectOption, HDBC,
                    UWORD, UDWORD );

RETCODE Sos_odbc_connection::SQLSetConnectOption( HDBC hdbc, UWORD fOption, UDWORD vParam )
{
    Sos_odbc_connection* conn = (Sos_odbc_connection*)hdbc;
    conn->_sqlstate = "";

	switch (fOption)
	{
    	case SQL_AUTOCOMMIT:        if( vParam == SQL_AUTOCOMMIT_ON )  return SQL_SUCCESS;
                                    break;
        case SQL_ACCESS_MODE:       if( vParam == SQL_MODE_READ_ONLY ) {
                                        conn->_open_mode = (Any_file::Open_mode)( conn->_open_mode & ~Any_file::out );
                                        return SQL_SUCCESS;
                                    }
                                    if( vParam == SQL_MODE_READ_WRITE ) {
                                        conn->_open_mode = (Any_file::Open_mode)( conn->_open_mode | Any_file::out );
                                        return SQL_SUCCESS;
                                    }
                                    break;
        case SQL_CURRENT_QUALIFIER: if( !vParam || *(char*)vParam == '\0' )  return SQL_SUCCESS;
                                    break;
        case SQL_LOGIN_TIMEOUT:     if( vParam != 0 )  goto CHANGED;
                                    return SQL_SUCCESS;
      //case SQL_ODBC_CURSORS:      *(DWORD*)pvParam = ;    Driver Manager
      //case SQL_OPT_TRACE                                  Driver Manager
      //case SQL_OPT_TRACEFILE                              Driver Manager
      //case SQL_PACKET_SIZE:       *(DWORD*)pvParam = ;    // network packet size
      //case SQL_QUIET_MODE:
        case SQL_TRANSLATE_DLL:     if( !vParam || *(char*)vParam == '\0' )  return SQL_SUCCESS;
                                    goto CHANGED;
        case SQL_TRANSLATE_OPTION:  break;
        case SQL_TXN_ISOLATION:     if( vParam == SQL_TXN_READ_UNCOMMITTED ) return SQL_SUCCESS;
                                    break;
        default: break;
    }

    return sql_error( "S1C00" );

  CHANGED:
    sql_error( "01S02" );
    return SQL_SUCCESS_WITH_INFO;
}

//--------------------------------------------------------------------------SQLGetConnectOption

DEFINE_ODBC_CALL_2( Sos_odbc_connection, SQLGetConnectOption, HDBC,
                    UWORD, PTR );

RETCODE Sos_odbc_connection::SQLGetConnectOption( HDBC hdbc, UWORD fOption, PTR pvParam )
{
    // wenn pvParam (char*) ist, max. SQL_MAX_OPTION_STRING_LENGTH Zeichen zuzügl. '\0' übertragen

    Sos_odbc_connection* conn = (Sos_odbc_connection*)hdbc;

	switch (fOption)
	{
        case SQL_ACCESS_MODE:   *(DWORD*)pvParam = conn->_open_mode & Any_file::out? SQL_MODE_READ_WRITE
                                                                                   : SQL_MODE_READ_ONLY;
                                return SQL_SUCCESS;
        case SQL_AUTOCOMMIT:    *(DWORD*)pvParam = SQL_AUTOCOMMIT_ON;   return SQL_SUCCESS;
        case SQL_CURRENT_QUALIFIER: *(char*)pvParam = '\0';             return SQL_SUCCESS;
        case SQL_LOGIN_TIMEOUT: *(DWORD*)pvParam = 0;
      //case SQL_ODBC_CURSORS:  *(DWORD*)pvParam = ;    Driver Manager
      //case SQL_OPT_TRACE                              Driver Manager
      //case SQL_OPT_TRACEFILE                          Driver Manager
      //case SQL_PACKET_SIZE:   *(DWORD*)pvParam = ;    // network packet size
      //case SQL_QUIET_MODE:    
        case SQL_TRANSLATE_DLL:  return SQL_NO_DATA_FOUND;
        case SQL_TRANSLATE_OPTION: return SQL_NO_DATA_FOUND;
        case SQL_TXN_ISOLATION:  *(DWORD*)pvParam = SQL_TXN_READ_UNCOMMITTED; return SQL_SUCCESS;
        default: ;
    }

    sql_error( "S1C00" );  // driver not capable?
    return SQL_ERROR;  // Oder SQL_NO_DATA_FOUND?
}

//----------------------------------------------------------------------------------SQLTransact

extern "C"
RETCODE SQL_API SQLTransact(
	HENV	henv,
	HDBC	hdbc,
	UWORD	fType)
{
    ODBC_LOG( "SQLTransact()\n");

    Sos_odbc_connection* conn = (Sos_odbc_connection*)hdbc;

    switch( fType ) {
        case SQL_COMMIT  : return SQL_SUCCESS;
        case SQL_ROLLBACK:
            conn->_sqlstate = "S1C00";      // "Driver not capable"
            return SQL_ERROR;
    }

    return SQL_ERROR;
}


} //namespace sos
