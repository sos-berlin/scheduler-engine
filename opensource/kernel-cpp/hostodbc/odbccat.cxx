//#define MODULE_NAME "odbccat"
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

namespace sos {


struct Abs_file_type;                            // Dateityp odbc_statistics
extern void call_for_linker( const void* );      // filetab.cxx
extern const Abs_file_type& odbc_data_types_file_type;  // odbctypf.cxx

//------------------------------------------------------------------------------------SQLTables
/*
 Stichwort SQLTables, Microsoft Developer Network CD April '96:

 1.	The szTableQualifier argument does not support search patterns.  [Chapter 22, SQLTables]
 2.	If szTableType is a percent sign (%) and szTableQualifier, szTableOwner, and szTableName are empty strings (""), SQLTables returns a result set of the table types supported by the data source.  [Chapter 22, SQLTables]
 3.	Table types in the szTableType argument can be unquoted or surrounded in single quotes.  [Chapter 22, SQLTables]
 4.	In addition to the types listed in ODBC 1.0, szTableType can be LOCAL TEMPORARY or GLOBAL TEMPORARY.  [Chapter 22, SQLTables]
 5.	SQLSTATE changes:
	S1008: Returned if the function was canceled from a different thread.
	S1010: Returned if the hstmt was in a Need Data state (see Appendix B).
	S1C00: Returned if the combination of the concurrency and the cursor type was not supported.

[Chapter 22, SQLTables; Appendix B, Statement Transitions]

*/
DEFINE_ODBC_CALL_8( Sos_odbc_stmt, SQLTables, HSTMT,
                    UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD )


RETCODE Sos_odbc_stmt::SQLTables(
    HSTMT     hstmt, 
	UCHAR FAR *szTableQualifier,
	SWORD	  cbTableQualifier,
	UCHAR FAR *szTableOwner,
	SWORD	  cbTableOwner,
	UCHAR FAR *szTableName,                                 
	SWORD	  cbTableName,
	UCHAR FAR *szTableType,
	SWORD	  cbTableType)
{
    Sos_string filename;
    Sos_string where_clause;
    Sos_string table_qualifier = odbc_as_string( szTableQualifier, cbTableQualifier );
    Sos_string table_owner     = odbc_as_string( szTableOwner    , cbTableOwner     );
    Sos_string table_name      = odbc_as_string( szTableName     , cbTableName      );
    Sos_string table_type      = odbc_as_string( szTableType     , cbTableType      );

    ODBC_LOG( "szTableQualifier=\"" << table_qualifier <<
              "\", szTableOwner=\"" << table_owner <<
              "\", szTableName=\""  << table_name <<
              "\", szTableType=\""  << table_type << "\"\n" );

    Bool qualifier_empty = szTableQualifier  &&  szTableQualifier[ 0 ] == '\0';
    Bool owner_empty     = szTableOwner      &&  szTableOwner    [ 0 ] == '\0';
    Bool name_empty      = szTableName       &&  szTableName     [ 0 ] == '\0';
    
    if( table_qualifier == "%"  &&  owner_empty  &&  name_empty ) {
        filename = "select distinct TABLE_QUALIFIER, null TABLE_OWNER, null TABLE_NAME, null TABLE_TYPE, null REMARKS"
                   " where TABLE_QUALIFIER like '%'";  // like '%' filtert NULL heraus
    }
    else
    if( table_owner == "%"  &&  qualifier_empty  &&  name_empty ) {
        filename = "select distinct null TABLE_QUALIFIER, TABLE_OWNER, TABLE_NAME, null TABLE_TYPE, null REMARKS"
                   " where TABLE_OWNER like '%'";
    }
    else 
    if( table_type == "%"  &&  qualifier_empty &&  owner_empty && name_empty ) {
        filename = "select distinct null TABLE_QUALIFIER, null TABLE_OWNER, null TABLE_NAME, TABLE_TYPE, null REMARKS"
                   " where TABLE_TYPE like '%'";
    }
    else {
        //filename = "hostAPI -in "
        //           "-fields=(TABLE_QUALIFIER,TABLE_OWNER,TABLE_NAME,TABLE_TYPE,REMARKS) "
        //           "sossql -catalog";
        // js 9.6.99: Reihenfolge der Felder ist signifikant!!! s. Doku von SQLTables!
        filename = "select TABLE_QUALIFIER,TABLE_OWNER,TABLE_NAME,TABLE_TYPE,REMARKS";
        
        if( szTableQualifier ) {
            if( length( where_clause ) )  where_clause += " and ";
            where_clause += "TABLE_QUALIFIER LIKE ";
            where_clause += quoted_string( table_qualifier, '\'', '\'' );
        }
    
        if( szTableOwner ) {
            if( length( where_clause ) )  where_clause += " and ";
            where_clause += "TABLE_OWNER LIKE ";
            where_clause += quoted_string( table_owner, '\'', '\'' );
        }
    
        if( szTableName ) {
            if( length( where_clause ) )  where_clause += " and ";
            where_clause += "TABLE_NAME LIKE ";
            where_clause += quoted_string( table_name, '\'', '\'' );
        }
    
        if( table_type != empty_string ) {
            if( length( where_clause ) )  where_clause += " and ";
            where_clause += "TABLE_TYPE in (";
            if( table_type[ 0 ] == '\'' ) {       // Apostrophe sind schon drin?
                where_clause += table_type;
            } else {   
                where_clause += '\'';
                const char* p = c_str( table_type );
                while( *p ) {
                    if( *p == ',' )  where_clause += "','";
                            else  where_clause += *p;
                    p++;
                }
                where_clause += '\'';
            }
            where_clause += ')';
        }
    }

    if( length( where_clause ) ) {
        filename += " where ";
        filename += where_clause;
    }

    filename += " order by TABLE_TYPE, TABLE_QUALIFIER, TABLE_OWNER, TABLE_NAME"
                " | sossql -catalog";

    //append_option( &filename, " -db=", _conn->_data_source_name );
    filename += _conn->_dbms_param;

    return SQLExecDirect( hstmt, (Byte*)c_str( filename ), length( filename ) );
}

//--------------------------------------------------------------------------SQLColumnPrivileges

extern "C"
RETCODE SQL_API SQLColumnPrivileges(
	HSTMT	,//hstmt,
	UCHAR*  ,//szTableQualifier,
	SWORD	,//cbTableQualifier,
	UCHAR*  ,//szTableOwner,
	SWORD	,//cbTableOwner,
	UCHAR*  ,//szTableName,
	SWORD	,//cbTableName,
	UCHAR*  ,//szColumnName,
	SWORD	)//cbColumnName)
{
    ODBC_LOG( "SQLColumnPrivileges()\n" );
	return SQL_ERROR;
}

//-------------------------------------------------------------------------------SQLGetTypeInfo

DEFINE_ODBC_CALL_1( Sos_odbc_stmt, SQLGetTypeInfo, HSTMT, SWORD )

RETCODE Sos_odbc_stmt::SQLGetTypeInfo( HSTMT hstmt, SWORD fSqlType )
{
    call_for_linker( &odbc_data_types_file_type );

    Sos_string filename = "select *";
    if( fSqlType )  filename += " where DATA_TYPE=" + as_string( fSqlType );
    filename += " order by DATA_TYPE, TYPE_NAME | -in sossql -types";

    return SQLExecDirect( hstmt, (Byte*)c_str( filename ), length( filename ) );
}

//-------------------------------------------------------------------------------SQLPrimaryKeys

extern "C"
RETCODE SQL_API SQLPrimaryKeys(
	HSTMT	  ,//hstmt,
	UCHAR FAR *,//szTableQualifier,
	SWORD	  ,//cbTableQualifier,
	UCHAR FAR *,//szTableOwner,
	SWORD	  ,//cbTableOwner,
	UCHAR FAR *,//szTableName,
	SWORD	   )//cbTableName)
{
    ODBC_LOG( "SQLPrimaryKeys()\n" );
	return SQL_ERROR;
}

//-------------------------------------------------------------------------------SQLForeignKeys

extern "C"
RETCODE SQL_API SQLForeignKeys(
	HSTMT	  ,//hstmt,
	UCHAR FAR *,//szPkTableQualifier,
	SWORD	  ,//cbPkTableQualifier,
	UCHAR FAR *szPkTableOwner,
	SWORD	  ,//cbPkTableOwner,
	UCHAR FAR *,//szPkTableName,
	SWORD	  ,//cbPkTableName,
	UCHAR FAR *,//szFkTableQualifier,
	SWORD	  ,//cbFkTableQualifier,
	UCHAR FAR *,//szFkTableOwner,
	SWORD	  ,//cbFkTableOwner,
	UCHAR FAR *,//szFkTableName,
	SWORD	  )//cbFkTableName)
{
    ODBC_LOG( "SQLForeignKeys()\n" );
	return SQL_ERROR;
}

//---------------------------------------------------------------------------SQLTablePrivileges

extern "C"
RETCODE SQL_API SQLTablePrivileges(
	HSTMT	  ,//hstmt,
	UCHAR FAR *,//szTableQualifier,
	SWORD	  ,//cbTableQualifier,
	UCHAR FAR *,//szTableOwner,
	SWORD	  ,//cbTableOwner,
	UCHAR FAR *,//szTableName,
	SWORD	  )//cbTableName)
{
    ODBC_LOG( "SQLTablePrivileges()\n" );
	return SQL_ERROR;
}

//--------------------------------------------------------------------------------SQLProcedures

extern "C"
RETCODE SQL_API SQLProcedures(
	HSTMT	  ,//hstmt,
	UCHAR FAR *,//szProcQualifier,
	SWORD	  ,//cbProcQualifier,
	UCHAR FAR *,//szProcOwner,
	SWORD	  ,//cbProcOwner,
	UCHAR FAR *,//szProcName,
	SWORD	  )//cbProcName)
{
    ODBC_LOG( "SQLProcedures()\n" );
	return SQL_ERROR;
}

//--------------------------------------------------------------------------SQLProcedureColumns

extern "C"
RETCODE SQL_API SQLProcedureColumns(
	HSTMT	  ,//hstmt,
	UCHAR FAR *,//szProcQualifier,
	SWORD	  ,//cbProcQualifier,
	UCHAR FAR *,//szProcOwner,
	SWORD	  ,//cbProcOwner,
	UCHAR FAR *,//szProcName,
	SWORD	  ,//cbProcName,
	UCHAR FAR *,//szColumnName,
	SWORD	  )//cbColumnName)
{
    ODBC_LOG( "SQLProcedureColumns()\n" );
	return SQL_ERROR;
}


} //namespace sos
