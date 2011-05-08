//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/optimize.h"

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
# else
    const int TRUE = 1;
    const int FALSE = 0;
#endif


#include <sql.h>
#include <sqlext.h>
#include "precomp.h"

#include <ctype.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/stdfield.h"
#include "../kram/sosdate.h"
#include "sosodbc.h"

namespace sos {

//---------------------------------------------------------------------------------SQLAllocStmt

DEFINE_ODBC_CALL_1( Sos_odbc_connection, SQLAllocStmt, HDBC,
                    HSTMT* )

RETCODE Sos_odbc_connection::SQLAllocStmt( HDBC, HSTMT* phstmt )
{
    Sos_odbc_stmt* stmt = new Sos_odbc_stmt;
    *phstmt = (HSTMT)stmt;

    stmt->_conn = this;
    stmt->_param_bindings.first_index( 1 );

	return SQL_SUCCESS;
}

//-----------------------------------------------------------------Sos_odbc_stmt::Sos_odbc_stmt

Sos_odbc_stmt::Sos_odbc_stmt()
: 
    _zero_(this+1) 
{ 
    _param_bindings.obj_const_name( "Sos_odbc_stmt::_param_bindings" ); 
    _col_bindings  .obj_const_name( "Sos_odbc_stmt::_col_bindings" ); 
    _cached_bindings.obj_const_name( "Sos_odbc_stmt::_cached_bindings" ); 
}

//----------------------------------------------------------------------------------SQLFreeStmt

DEFINE_ODBC_CALL_1( Sos_odbc_stmt, sos_SQLFreeStmt, HSTMT,
                    UWORD )

extern "C"
RETCODE SQL_API SQLFreeStmt( HSTMT hstmt, UWORD fOption )
{
    RETCODE rc = sos_SQLFreeStmt( hstmt, fOption );

    if( rc == SQL_SUCCESS  ||  rc == SQL_SUCCESS_WITH_INFO ) {  // Bei Fehler SOSODBC-10 (Rekursion) kein delete!!
        if( fOption == SQL_DROP ) {
            try {
                delete (Sos_odbc_stmt*)hstmt;
            }
            catch(...) {
                return SQL_ERROR;   // Muss der hstmt jetzt noch gültig sein?
            }
        }
    }
    return rc;
}


RETCODE Sos_odbc_stmt::sos_SQLFreeStmt( HSTMT, UWORD fOption )
{
	switch (fOption)
	{
    	case SQL_CLOSE:
            _file.close( close_cursor );      // prepare_open() bleibt gültig (? 31.3.96)
            _eof = false;
    		break;

    	case SQL_UNBIND:
            _col_bindings.last_index( _col_bindings.first_index() - 1 );
            _col_bindings.size( 0 );
            _cached_bindings.last_index( _cached_bindings.first_index() - 1 );
            _cached_bindings.size( 0 );     // löschen
    		break;

    	case SQL_RESET_PARAMS:
            _param_index = 0;
            if( _param_record_type )  {
                try { _file.bind_parameters( 0, 0 ); }
                catch(...) {}  // Not_implemented_error
                _param_record_type = NULL;
            }
            _param_bindings.last_index( _param_bindings.first_index() - 1 );
            _param_bindings.size( 0 );
    		break;

        case SQL_DROP:      // s. SQLFreeStmt und sos_SQLFreeStmt
            try {
                try { _file.bind_parameters( 0, 0 ); }
                catch(...) {}  // Not_implemented_error
                _param_record_type = NULL;
            }
            catch(...) {}
/*
            try {
                _file.close();
            }
            catch(...) {}
*/
    	    break;

        default:
            return SQL_ERROR;
   	}

    return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------SQLSetStmtOption

DEFINE_ODBC_CALL_2( Sos_odbc_stmt, SQLSetStmtOption, HSTMT,
                    UWORD, UDWORD )

RETCODE Sos_odbc_stmt::SQLSetStmtOption( HSTMT hstmt, UWORD fOption, UDWORD vParam )
{
    RETCODE rc;

	switch (fOption)
	{
        case SQL_ASYNC_ENABLE:      goto FIXED_ERROR;
        case SQL_BIND_TYPE:         goto FIXED_ERROR;
        case SQL_CONCURRENCY:       goto FIXED_ERROR;
        case SQL_CURSOR_TYPE:       goto FIXED_CHANGED;
        case SQL_KEYSET_SIZE:       goto FIXED_CHANGED;
        case SQL_MAX_LENGTH:        goto FIXED_CHANGED;
        case SQL_MAX_ROWS:          goto FIXED_CHANGED;
        case SQL_NOSCAN:            goto FIXED_ERROR;
        case SQL_QUERY_TIMEOUT:     goto FIXED_CHANGED;
        case SQL_RETRIEVE_DATA:     goto FIXED_ERROR;
        case SQL_ROWSET_SIZE:       goto FIXED_CHANGED;
        case SQL_SIMULATE_CURSOR:   goto FIXED_CHANGED;
        case SQL_USE_BOOKMARKS:     goto FIXED_ERROR;
    	                   default: ;
	}

    return sql_error( "S1C00" );
    
  FIXED_ERROR:
    {
        UDWORD vParamFixed;

        rc = SQLGetStmtOption( hstmt, fOption, &vParamFixed );
        if( rc != SQL_SUCCESS )  return rc;
        if( vParam != vParamFixed )  return sql_error( "S1C00" );
        return SQL_SUCCESS;
    }

  FIXED_CHANGED:
    {
        UDWORD vParamFixed;

        rc = SQLGetStmtOption( hstmt, fOption, &vParamFixed );
        if( rc != SQL_SUCCESS )  return rc;
        if( vParam != vParamFixed )  {
            sql_error( "01S02" );
            return SQL_SUCCESS_WITH_INFO;
        }

        return SQL_SUCCESS;
    }
}

//-----------------------------------------------------------------------------SQLGetStmtOption

DEFINE_ODBC_CALL_2( Sos_odbc_stmt, SQLGetStmtOption, HSTMT,
                    UWORD, PTR )

RETCODE Sos_odbc_stmt::SQLGetStmtOption( HSTMT, UWORD fOption, PTR pvParam )
{
	switch (fOption)
	{
        case SQL_ASYNC_ENABLE:      *(int4*)pvParam = SQL_ASYNC_ENABLE_OFF;    return SQL_SUCCESS;
        case SQL_BIND_TYPE:         *(int4*)pvParam = SQL_BIND_BY_COLUMN;      return SQL_SUCCESS;
        case SQL_CONCURRENCY:       *(int4*)pvParam = SQL_CONCUR_READ_ONLY;    return SQL_SUCCESS;
        case SQL_CURSOR_TYPE:       *(int4*)pvParam = SQL_CURSOR_FORWARD_ONLY; return SQL_SUCCESS;
        case SQL_KEYSET_SIZE:       *(int4*)pvParam = 0;                       return SQL_SUCCESS;
        case SQL_MAX_LENGTH:        *(int4*)pvParam = 0;                       return SQL_SUCCESS;
        case SQL_MAX_ROWS:          *(int4*)pvParam = 0;                       return SQL_SUCCESS;
        case SQL_NOSCAN:            *(int4*)pvParam = SQL_NOSCAN_ON;           return SQL_SUCCESS;
        case SQL_QUERY_TIMEOUT:     *(int4*)pvParam = 0;                       return SQL_SUCCESS;
        case SQL_RETRIEVE_DATA:     *(int4*)pvParam = SQL_RD_ON;               return SQL_SUCCESS;
        case SQL_ROWSET_SIZE :      *(int4*)pvParam = 1;                       return SQL_SUCCESS;
        case SQL_SIMULATE_CURSOR:   *(int4*)pvParam = SQL_SC_NON_UNIQUE;       return SQL_SUCCESS;
        case SQL_USE_BOOKMARKS	:   *(int4*)pvParam = SQL_UB_OFF;              return SQL_SUCCESS;
        case SQL_GET_BOOKMARK:                                                 break;
        case SQL_ROW_NUMBER:        *(int4*)pvParam = _row_no;                 return SQL_SUCCESS;
    	                   default: ;
    }

    return sql_error( "S1C00" );
}

//-----------------------------------------------------------------------------------SQLPrepare

DEFINE_ODBC_CALL_2( Sos_odbc_stmt, SQLPrepare, HSTMT,
                    UCHAR*, SDWORD )

RETCODE Sos_odbc_stmt::SQLPrepare( HSTMT, UCHAR* sql_string, SDWORD sql_string_length )
{
    if( sql_string_length == SQL_NTS )  sql_string_length = strlen( (const char*)sql_string );

    _sql_string = as_string( (const char*)sql_string, sql_string_length );
    ODBC_LOG( "    SQL-String=" << _sql_string << '\n' );

    _file.close();

    Sos_string filename;
    Sos_limited_text<30> st;

    const char* p1    = c_str( _sql_string );
    const char* p2;
    const char* p_end = p1 + length( _sql_string );

    while( isspace( *p1 ) == ' ' )  p1++;

    p2 = p1;
    if( p_end > p1 + st.size() )  p_end = p1 + st.size();
    while( p2 < p_end  &&  !isspace( *p2 ) )  p2++;

    st.assign( p1, p2 - p1 );
    st.upper_case();

    if( /*st == "DIRECT" ||*/ st == "HOSTAPI" ) {
        filename = p2;
        _with_result_set = true;
    } else {
        filename = "sossql ";
        filename += _conn->_dbms_param;
        filename += ' ';
        filename += _sql_string;
        _with_result_set = st == "SELECT";
    }

    try {
      //_file.prepare_open( filename, File_base::Open_mode( File_base::in | File_base::seq ) );
        _file.prepare_open( filename, File_base::inout );
    }
    catch( const Not_exist_error& x )
    {
        return sql_error( "S0002", x );     // "Base table not found"
    }
    catch( const Not_found_error& x )
    {
        if( strcmp( x.code(), "SOS-1179" ) == 0 ) {  // Ein Feld $1 gibt es nicht im Datensatz $2
            return sql_error( "S0022", x );     // "Column not found"
        }
        throw;
    }

    _record_type = (Record_type*)_file.spec().field_type_ptr();

    if( _record_type ) {
        _field_count = _record_type->field_count();
    } else {
        _field_count = 0;
    }

/* jz 20.10.96
    _cached_bindings.size( 0 );     // löschen
    _cached_bindings.first_index( 1 );

    _col_bindings.size( 0 );        // löschen
    _col_bindings.first_index( 1 );
    _col_bindings.last_index( _field_count );
*/
    if( _cached_bindings.last_index() < _field_count )  _cached_bindings.last_index( _field_count );
    if( _col_bindings   .last_index() < _field_count )  _col_bindings   .last_index( _field_count );
     
	return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------SQLDescribeParam

extern "C"
RETCODE SQL_API SQLDescribeParam(
	HSTMT	   hstmt,
	UWORD	   ipar,
	SWORD  FAR *pfSqlType,
	UDWORD FAR *pcbColDef,
	SWORD  FAR *pibScale,
	SWORD  FAR *pfNullable)
{
    ODBC_LOG( "SQLDescribeParam()\n" );
	return SQL_ERROR;
}

//-----------------------------------------------------------------Sos_odbc_stmt::clear_binding
/*
void Sos_odbc_stmt::clear_binding( Sos_simple_array<Sos_odbc_binding>* bindings )
{
    for( int i = bindings->first_index(); i <= bindings->last_index(); i++ ) {
        (*bindings)[ i ].clear();
    }
}
*/
//-----------------------------------------------------------------------------SQLBindParameter

DEFINE_ODBC_CALL_9( Sos_odbc_stmt, SQLBindParameter, HSTMT,
                    UWORD, SWORD, SWORD, SWORD, UDWORD, SWORD, PTR, SDWORD, SDWORD* )


RETCODE Sos_odbc_stmt::SQLBindParameter(
    HSTMT, 
	UWORD	   ipar,
	SWORD	   fParamType,      // nur SQL_PARAM_INPUT
	SWORD	   fCType,
	SWORD	   fSqlType,        // ?
	UDWORD	   cbColDef,        // ? precision
	SWORD	   ibScale,         // ?
	PTR 	   rgbValue,        // SQL_LEN_DATA_AT_EXEC(length) ?
	SDWORD	   cbValueMax,      // Kann -1 (=undefinert) sein
	SDWORD    *pcbValue )       // ist nur bei SQLExecute/SQLExecDirect gültig!
{
/* Wenn von einer ODBC 1.0 Applikation gerufen, ist folgender Verhalt nicht geklärt:

The cbColDef argument specifies the precision of the column or expression corresponding 
to the parameter marker, unless all of the following are true:

-   An ODBC 2.0 application calls SQLBindParameter in an ODBC 1.0 driver or an ODBC 1.0 application 
    calls SQLSetParam in an ODBC 2.0 driver. (Note that the Driver Manager converts these calls.)
-   The fSqlType argument is SQL_LONGVARBINARY or SQL_LONGVARCHAR.
-   The data for the parameter will be sent with SQLPutData.

In this case, the cbColDef argument contains the total number of bytes that will be sent for the 
parameter. For more information, see Passing Parameter Values and SQL_DATA_AT_EXEC in pcbValue Argument.
*/
    // Wenn eine ODBC-1.0-Applikation SQLSetParam aufruft, kommen hier folgende Parameter an:
    // fParamType = SQL_PARAM_INPUT_OUTPUT
    // cbValueMax == SQL_SETPARAM_VALUE_MAX  (= -1)

    _param_index = 1;
    _file.bind_parameters( 0, 0 );      // Referenzen auf _param_bindings[]._field freigeben
    _param_record_type = 0;

  //if( ipar > _param_bindings.last_index() )  return sql_error( "S1093" );  // "Invalid parameter number"
    if( fParamType == SQL_PARAM_OUTPUT )       return sql_error( "S1000" );

    if( _param_bindings.last_index() < (int)ipar )  _param_bindings.last_index( ipar );

    Sos_odbc_binding* b = &_param_bindings[ ipar ];

    b->_fParamType = fParamType;

    if( !rgbValue ) {
        b->_field    = 0;
        b->_pcbValue = 0;
    } else {
        b->_fParamType = fParamType;
        if( fCType == SQL_C_DEFAULT ) {
            fCType = sql_to_c_default( fSqlType, SOS_ODBC_VER, false, 0, 0 ); //?odbc_c_default_type( fSqlType );
            //if( fCType == SQL_C_CHAR )  b->_cbValueMax = cbColDef;
        }
        // cbValueMax kann SQL_SETPARAM_VALUE_MAX sein, wird in SQLExecute korrigiert.
        b->_fCType     = fCType;
        if( ibScale
         && fCType != SQL_C_CHAR
         && fCType != SQL_C_FLOAT && fCType != SQL_C_DOUBLE )  return sql_error( "S1C00", "SOSODBC-11" );
     //?b->_scale      = ibScale;
        b->_pcbValue   = pcbValue;
        b->_cbValueMax = 1;         // Bei SQL_C_CHAR und SQL_C_BINARY wird _field->type_ptr()->_field_size 
        b->prepare();               // in prepare_param_record() geändert!
        b->_cbValueMax = cbValueMax; // ODBC 1.0: SQL_SETPARAM_VALUE_MAX;
        b->_field->_offset = (long)rgbValue;
      //b->_field.name( "SQLBindParameter_" + as_string( icol ) );
    }

    return SQL_SUCCESS;
}

//------------------------------------------------------------------------------SQLParamOptions

extern "C" RETCODE SQL_API SQLParamOptions(
	HSTMT	   hstmt,
	UDWORD	   crow,
	UDWORD FAR *pirow)
{
    ODBC_LOG( "SQLParamOptions()\n" );
	return SQL_ERROR;
}

//---------------------------------------------------------------------------------SQLNumParams

extern "C" RETCODE SQL_API SQLNumParams(
	HSTMT	   hstmt,
	SWORD  FAR *pcpar)
{
    ODBC_LOG( "SQLNumParams()\n" );
	return SQL_ERROR;
}

//----------------------------------------------------------Sos_odbc_stmt::prepare_param_record

RETCODE Sos_odbc_stmt::prepare_param_record()
{
    // SQLBindParamter darf nicht mehr gerufen werden!!

    RETCODE retcode = SQL_SUCCESS;

    if( !_param_record_type ) {           // Neues SQLBindParameter?
        _param_record_type = Record_type::create();
        _param_record_type->allocate_fields( _param_bindings.count() );

        for( int i = 1; i <= _param_bindings.last_index(); i++ )
        {
            Sos_odbc_binding* b = &_param_bindings[ i ];
            if( b->_field ) {
                b->_field->_null_flag_offset = (long)&b->_null_flag;
                _param_record_type->add_field( b->_field );
            } else {
                sql_error( "07001" );  // "Wrong number of parameters"
                retcode = SQL_SUCCESS_WITH_INFO;
                _param_record_type->add_field( NULL );  // Vorsicht!
            }
        }
    }


    while( _param_index <= _param_bindings.last_index() )
    {
        Sos_odbc_binding* b = &_param_bindings[ _param_index ];
        if( b->_field )
        {
            //LOG( "b->_fParamType==" << b->_fParamType << ", *b->_pcbValue == " << *b->_pcbValue << "\n" );
            if( b->_fParamType == SQL_PARAM_OUTPUT )  b->_null_flag = true;

            SDWORD cbValue = b->_pcbValue? *b->_pcbValue : SQL_NTS;

            if( cbValue == SQL_DATA_AT_EXEC )  retcode = SQL_NEED_DATA;
            else
            if( cbValue <= SQL_LEN_DATA_AT_EXEC_OFFSET ) {
                b->_buffer.allocate_min( SQL_LEN_DATA_AT_EXEC_OFFSET - cbValue );
                retcode = SQL_NEED_DATA;
            }
            else
            if( cbValue == SQL_NULL_DATA ) {
                b->_null_flag = true;
            }
            else
            {
                b->_null_flag = false;

                if( b->_fCType == SQL_C_CHAR || b->_fCType == SQL_C_BINARY ) {
                    if( cbValue == SQL_NTS )  cbValue = strlen( (const char*)b->rgbValue() );
                    else
                    if( cbValue < 0 )  throw_xc( "SQLBindParameter-cbValue", cbValue );

                    b->_field->type_ptr()->_field_size = cbValue;
                } else {
                    if( b->_pcbValue  &&  cbValue < 0 )  throw_xc( "SQLBindParameter-cbValue", cbValue );
                }
            }
        }

        if( log_ptr ) {
            *log_ptr << "prepare_param_record: " << _param_index << ". Parameter ";
            if( b->_field ) {
                *log_ptr << *b->_field << " = ";
                SDWORD cbValue = b->_pcbValue? *b->_pcbValue : 0;
                if( cbValue == SQL_DATA_AT_EXEC )  *log_ptr << "SQL_DATA_AT_EXEC";
                else
                if( cbValue <= SQL_LEN_DATA_AT_EXEC_OFFSET ) *log_ptr << "SQL_LEN_DATA_AT_EXEC(" << ( SQL_LEN_DATA_AT_EXEC_OFFSET - cbValue ) << '\n';
                else
                if( b->_null_flag )  *log_ptr << "NULL";
                else {
                    *log_ptr << '"';
                    b->_field->print( 0, log_ptr, std_text_format );
                    *log_ptr << '"';
                }
                *log_ptr << '\n';
            } else {
                *log_ptr << "NOT BOUND (NULL)\n";
            }
        }

        if( retcode == SQL_NEED_DATA )  break;

        _param_index++;
    }

    return retcode;
}

//---------------------------------------------------------------------------------SQLParamData

DEFINE_ODBC_CALL_1( Sos_odbc_stmt, SQLParamData, HSTMT,
                    PTR* )


RETCODE Sos_odbc_stmt::SQLParamData( HSTMT hstmt, PTR* prgbValue )
{
    RETCODE retcode = prepare_param_record();
    if( retcode == SQL_NEED_DATA )  {
        *prgbValue = _param_bindings[ _param_index ].rgbValue();
        return SQL_NEED_DATA;
    }

    if( retcode != SQL_SUCCESS  &&  retcode != SQL_SUCCESS_WITH_INFO )  return retcode;

    return SQLExecute( hstmt );
}

//-----------------------------------------------------------------------------------SQLPutData

DEFINE_ODBC_CALL_2( Sos_odbc_stmt, SQLPutData, HSTMT,
                    PTR, SDWORD )


RETCODE Sos_odbc_stmt::SQLPutData( HSTMT, PTR rgbValue, SDWORD cbValue )
{
    Sos_odbc_binding* b = &_param_bindings[ _param_index ];

    if( cbValue == SQL_NULL_DATA ) {
        b->_buffer_length = SQL_NULL_DATA;
    } else {
        if( b->_default_length != SQL_NO_TOTAL )  cbValue = b->_default_length;
        else
        if( cbValue == SQL_NTS                 )  cbValue = strlen( (const char*)rgbValue );

        if( cbValue < 0                        )  throw_xc( "SQLPutData-cbValue" );

        b->_buffer.append( rgbValue, cbValue );

        b->_field->offset( (long)b->_buffer.ptr() );
        b->_buffer_length = b->_buffer.length();
    }

    b->_pcbValue = &b->_buffer_length;

    return SQL_SUCCESS;
}

//-------------------------------------------------------Sos_odbc_stmt::emulate_bind_parameters
/*
void Sos_odbc_stmt::emulate_bind_parameters()
{
    Dynamic_area        string ( 32767 );
    Dynamic_area        value  ( 256 );

    const char*         p0       = c_str( _sql_string );
    const char*         p        = p;
    int                 param_no = _param_bindings.first_index();

    _sql_string_2 = "";

    while( param_no <= _param_bindings.last_index() ) 
    {
        Sos_odbc_binding*   b = &_param_bindings[ param_no ];

        while( *p  &&  *p != '?' ) {
            if( *p == '\''  ||  *p == '"' ) {
                char quote = *p;
                while( *p  &&  *p != quote )  p++;
            }
            else p++;
        }

        if( !*p )  break;

        string.append( p0, p - p0 );
        p++;

        string.resize_min( string.length() + 2+2*256 );    // Kein Parameter länger als 256 
 
        Area rest = string.rest();

        if( b->_field->null( 0 ) ) {
            string.append( "NULL" );
        } 
        else 
        if( b->_fSqlType == SQL_CHAR  
         || b->_fSqlType == SQL_VARCHAR 
         || b->_fSqlType == SQL_LONGVARCHAR 
         || b->_fSqlType == SQL_BINARY
         || b->_fSqlType == SQL_VARBINARY
         || b->_fSqlType == SQL_LONGVARBINARY
         || b->_fSqlType == SQL_DATE                     // DBMS muss Format yyyy-mm-dd benutzen!
         || b->_fSqlType == SQL_DATETIME
         || b->_fSqlType == SQL_TIME ) 
        {
            b->_field->write_text( 0, &value );
            write_string( value, &rest, '\'', '\'' );
        } 
        else 
        {
            b->_field->write_text( 0, &rest );
        }

        string.length( string.length() + rest.length() );
        param_no++;
    }

    string.append( p0, p - p0 );

    // Kein _file.prepare()!
    // anschließend _file.open()
}
*/
//-----------------------------------------------------------------------------------SQLExecute

DEFINE_ODBC_CALL_0( Sos_odbc_stmt, SQLExecute, HSTMT )

RETCODE Sos_odbc_stmt::SQLExecute( HSTMT )
{
    // Jetzt ist *pcbValue von SqlBindParameter gültig: (falls pcbValue != NULL)
    RETCODE retcode = SQL_SUCCESS;

    if( _param_bindings.count() > 0 )     // SQLBindParameter?
    {
        _param_index = 1;
        retcode = prepare_param_record();
        if( retcode != SQL_SUCCESS  &&  retcode != SQL_SUCCESS_WITH_INFO )  return retcode;

        _file.bind_parameters( _param_record_type, 0 );
    }

    if( _with_result_set ) {
        _file.open();               // _file.close() erforderlich
    } else {
        _file.execute();
    }

    return retcode;
}

//--------------------------------------------------------------------------------SQLExecDirect

DEFINE_ODBC_CALL_2( Sos_odbc_stmt, SQLExecDirect, HSTMT, UCHAR*, SDWORD )

RETCODE Sos_odbc_stmt::SQLExecDirect(
    HSTMT     hstmt, 
	UCHAR*    szSqlStr,
	SDWORD    cbSqlStr )
{
    RETCODE        rc;

    rc = SQLPrepare( hstmt, szSqlStr, cbSqlStr );
    if( rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO )  return rc;

    rc = SQLExecute( hstmt );
    return rc;
}

//---------------------------------------------------------------------------------SQLNativeSql

extern "C"
RETCODE SQL_API SQLNativeSql(
    HDBC        hdbc,                    // sqlext.h
  //LPDBC      lpdbc,                    // vorher
	UCHAR FAR *szSqlStrIn,
	SDWORD     cbSqlStrIn,
	UCHAR FAR *szSqlStr,
 	SDWORD     cbSqlStrMax,
	SDWORD FAR *pcbSqlStr)
{
    ODBC_LOG( "SQLNativeSql()\n" );
	return SQL_ERROR;
}

//--------------------------------------------------------------------------SQLSetScrollOptions

extern "C" RETCODE
SQL_API SQLSetScrollOptions(
	HSTMT	   hstmt,
	UWORD	   fConcurrency,
	SDWORD	crowKeyset,
	UWORD	   crowRowset)
{
    ODBC_LOG( "SQLSetScrollOptions()\n" );
	return SQL_ERROR;
}

//-----------------------------------------------------------------------------SQLSetCursorName
// Nur für UPDATE .. WHERE CURRENT OF cursorname

extern "C"
RETCODE SQL_API SQLSetCursorName(
	HSTMT	  hstmt,
	UCHAR FAR *szCursor,
	SWORD	  cbCursor)
{
    ODBC_LOG( "SQLSetCursorName()\n" );
	return SQL_ERROR;
}

//-----------------------------------------------------------------------------SQLGetCursorName
// Nur für UPDATE .. WHERE CURRENT OF cursorname

extern "C"
RETCODE SQL_API SQLGetCursorName(
	HSTMT	  hstmt,
	UCHAR FAR *szCursor,
	SWORD	  cbCursorMax,
	SWORD FAR *pcbCursor)
{
    ODBC_LOG( "SQLGetCursorName()\n" );
	return SQL_ERROR;
}

//------------------------------------------------------------------------------------SQLCancel

extern "C"
RETCODE SQL_API SQLCancel( HSTMT hstmt )	// Statement to cancel.
{
    ODBC_LOG( "SQLCancel()\n");
	/* TBD - This function should free up any resources bound for run
	 * time parameters - I think.
	 */
	return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------SQLNumResultCols

DEFINE_ODBC_CALL_1( Sos_odbc_stmt, SQLNumResultCols, HSTMT,
                    SWORD* )


RETCODE Sos_odbc_stmt::SQLNumResultCols( HSTMT, SWORD* pccol )
{
    *pccol = _field_count;
    return SQL_SUCCESS;
}

//-------------------------------------------------------------------------------SQLDescribeCol

DEFINE_ODBC_CALL_8( Sos_odbc_stmt, SQLDescribeCol, HSTMT,
                    UWORD, UCHAR*, SWORD, SWORD*,
                    /*UNALIGNED*/SWORD*, /*UNALIGNED*/ UDWORD*, /*UNALIGNED*/ SWORD*,
                    SWORD* )


RETCODE Sos_odbc_stmt::SQLDescribeCol(
    HSTMT, 
	UWORD	   icol,
	UCHAR  FAR *szColName,
	SWORD	   cbColNameMax,
	SWORD  FAR *pcbColName,
	/* UNALIGNED */ SWORD  FAR *pfSqlType,
	/* UNALIGNED */ UDWORD FAR *pcbColDef,
	/* UNALIGNED */ SWORD  FAR *pibScale,
	SWORD  FAR *pfNullable)
{
    if( !_record_type )  throw_xc( "SOS-1193" );
    if( icol > _field_count )  return SQL_ERROR;

    Field_descr* f          = _record_type->field_descr_ptr( icol - 1 );
    Field_type*  t          = f->type_ptr();

    if( t->obj_is_type( tc_Record_type )        // Record_type?
     && ((Record_type*)t)->_group_type )        // Hat ein Gruppenfeld (geht über alle Felder)?
    {
        t = ((Record_type*)t)->_group_type;         // Das nehmen wir!
    }

    Type_param   type_param;
    t->get_param( &type_param );

    return_odbc_string0( szColName, cbColNameMax, pcbColName, f->name() );

    if( pfSqlType  )  *pfSqlType  = odbc_sql_type( type_param );
	if( pcbColDef  )  *pcbColDef  = type_param.precision_10();
	if( pibScale   )  *pibScale   = type_param._scale;
	if( pfNullable )  *pfNullable = f->nullable()?  SQL_NULLABLE : SQL_NO_NULLS;

    if( log_ptr ) {
        *log_ptr << *f << ": ";
        if( szColName  )  *log_ptr << " ColName="  << szColName;
        if( pfSqlType  )  *log_ptr << " SqlType="  << *pfSqlType;
        if( pcbColDef  )  *log_ptr << " ColDef="   << *pcbColDef;
        if( pibScale   )  *log_ptr << " Scale="    << *pibScale;
        if( pfNullable )  *log_ptr << " Nullable=" << *pfNullable;
        *log_ptr << endl;
    }

    return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------SQLColAttributes

DEFINE_ODBC_CALL_6( Sos_odbc_stmt, SQLColAttributes, HSTMT,
                    UWORD, UWORD, PTR, SWORD, SWORD*, SDWORD* )


RETCODE Sos_odbc_stmt::SQLColAttributes(
    HSTMT, 
	UWORD	   icol,
	UWORD	   fDescType,
	PTR 	   rgbDesc,
	SWORD	   cbDescMax,
	SWORD  FAR *pcbDesc,
	SDWORD FAR *pfDesc)
{
	if (fDescType == SQL_COLUMN_COUNT) {
        LOG( "SQL_COLUMN_COUNT: " << _field_count << '\n' );
        *pfDesc = _field_count;
        if( pcbDesc )  *pcbDesc = sizeof(int);
        return SQL_SUCCESS;
    }

    if( !_record_type )  throw_xc( "SOS-1193" );
    if( icol > _field_count )  return SQL_ERROR;

    Field_descr* f          = _record_type->field_descr_ptr( icol - 1 );
    Field_type*  t          = f->type_ptr();
    Type_param   type_param;

    if( t->obj_is_type( tc_Record_type )        // Record_type?
     && ((Record_type*)t)->_group_type )        // Hat ein Gruppenfeld (geht über alle Felder)?
    {
        t = ((Record_type*)t)->_group_type;         // Das nehmen wir!
    }

    t->get_param( &type_param );

    LOG( *f << ": " );

    switch (fDescType)
    {
        case SQL_COLUMN_AUTO_INCREMENT:
            *pfDesc = FALSE; // Not supported
            sos_log( "SQL_COLUMN_AUTO_INCREMENT: FALSE\n" );
            break;

        case SQL_COLUMN_CASE_SENSITIVE:
            *pfDesc =    type_param._std_type == std_type_char
                      || type_param._std_type == std_type_varchar;
            LOG( "SQL_COLUMN_CASE_SENSITIVE: " << *pfDesc << '\n' );
            break;

    	case SQL_COLUMN_DISPLAY_SIZE:
            *pfDesc = type_param._display_size;
            LOG( "SQL_COLUMN_DISPLAY_SIZE: " << *pfDesc << '\n' );
    		break;

        case SQL_COLUMN_LENGTH:
            *pfDesc = odbc_c_default_type_length( type_param );
            LOG( "SQL_COLUMN_LENGTH: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_MONEY:
            *pfDesc = FALSE; // Not supported
            sos_log( "SQL_COLUMN_MONEY: FALSE\n" );
            break;

        case SQL_COLUMN_LABEL:
        case SQL_COLUMN_NAME:
            return_odbc_string0( (uchar*)rgbDesc, cbDescMax, pcbDesc, f->name() );
            LOG( "SQL_COLUMN_LABEL/NAME: " << (const char*)rgbDesc << '\n' );
            break;

        case SQL_COLUMN_TABLE_NAME:
            return_odbc_string0( (uchar*)rgbDesc, cbDescMax, pcbDesc, "" );
            LOG( "SQL_COLUMN_TABLE_NAME: " << (const char*)rgbDesc << '\n' );
            break;

        case SQL_COLUMN_NULLABLE:
            *pfDesc = f->nullable()? SQL_NO_NULLS : SQL_NULLABLE;
            LOG( "SQL_COLUMN_NULLABLE: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_OWNER_NAME:
            return_odbc_string0( (uchar*)rgbDesc, cbDescMax, pcbDesc, "" );
            LOG( "SQL_COLUMN_OWNER_NAME: " << (const char*)rgbDesc << '\n' );
            break;

        case SQL_COLUMN_PRECISION:
            *pfDesc = type_param.precision_10();
            LOG( "SQL_COLUMN_DISPLAY_SIZE: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_QUALIFIER_NAME:
            return_odbc_string0( (uchar*)rgbDesc, cbDescMax, pcbDesc, "" );
            LOG( "SQL_COLUMN_QUALIFIER_NAME: " << (const char*)rgbDesc << '\n' );
            break;

        case SQL_COLUMN_SCALE:
            *(int*)pfDesc = type_param._scale;
            if( pcbDesc )  *pcbDesc = sizeof(int);
            LOG( "SQL_COLUMN_SCALE: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_SEARCHABLE:
            *pfDesc = SQL_SEARCHABLE;
            LOG( "SQL_COLUMN_SEARCHABLE: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_TYPE:
            *pfDesc = odbc_sql_type( type_param );
            LOG( "SQL_COLUMN_TYPE: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_TYPE_NAME:
            return_odbc_string0( (char*)rgbDesc, cbDescMax, pcbDesc, type_param._info_ptr->_name );
            LOG( "SQL_COLUMN_TYPE_NAME: " << (const char*)rgbDesc << '\n' );
            break;

        case SQL_COLUMN_UNSIGNED:
            *pfDesc = type_param._unsigned;
            LOG( "SQL_COLUMN_UNSIGNED: " << *pfDesc << '\n' );
            break;

        case SQL_COLUMN_UPDATABLE:
            *pfDesc = SQL_ATTR_WRITE;
            LOG( "SQL_COLUMN_UPDATABLE: " << *pfDesc << '\n' );
            break;

        default:
            return SQL_ERROR;
	}

	return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------------SQLBindCol

DEFINE_ODBC_CALL_5( Sos_odbc_stmt, SQLBindCol, HSTMT,
                    UWORD, SWORD, PTR, SDWORD, SDWORD* )

RETCODE Sos_odbc_stmt::SQLBindCol(
    HSTMT, 
	UWORD	   icol,
	SWORD	   fCType,
	PTR 	   rgbValue,
	SDWORD	   cbValueMax,
	SDWORD FAR *pcbValue)
{
    if( _eof )  return SQL_SUCCESS;     // Für leere Menge ohne Satzbeschreibung

    if( !_record_type )  throw_xc( "SOS-1193" );

    Sos_odbc_binding* b = &_col_bindings[ icol ];

    if( !rgbValue ) {
        b->_field    = 0;
        b->_pcbValue = 0;
    } else {
        Bool changed = b->_field == NULL;

        if( b->_fCType != fCType ) {
            b->_fCType = fCType;
            changed = true;
        }

        if( b->_cbValueMax != cbValueMax ) {
            b->_cbValueMax = cbValueMax;
            changed = true;
        }

        b->_pcbValue   = pcbValue;
        b->_fParamType = SQL_PARAM_OUTPUT;

        if( changed ) {
            b->prepare( _record_type->field_descr_ptr( icol - 1 ));
        }

        b->_field->_offset = (long)rgbValue;

        if( length( b->_field->name() ) == 0 ) {
            b->_field->name( "SQLBindCol_" + as_string( icol ) );
        }

        if( pcbValue )  *pcbValue = b->_default_length;//odbc_c_default_type_length( fCType );
    }

    return SQL_SUCCESS;
}

//------------------------------------------------------------Sos_odbc_stmt::copy_value_to_odbc

void Sos_odbc_stmt::copy_value_to_odbc( const Sos_odbc_binding* b,
                                        const Field_descr* source_field )
{
    if( b->rgbValue() )
    {
        if( source_field->null( _record.byte_ptr() ) ) {
            if( b->_pcbValue )  *b->_pcbValue = SQL_NULL_DATA;
            if( b->rgbValue() ) {
                // L-Bank, 13.2.97: Bei NULL Wert auf binär 0 setzen (besonders bei SQL_C_CHAR). Für wdodbc.wll
                if( b->_fCType == SQL_C_CHAR  &&  b->_cbValueMax >= 1 ) {
                    *(char*)b->rgbValue() = '\0';   // Bei SQL_C_CHAR: Leerstring
                } else {
                    SWORD len = odbc_c_default_type_length( b->_fCType );
                    if( len > SQL_NO_TOTAL )  memset( b->rgbValue(), 0, len );
                }
            }
        } else {
            //copy_value( b->_field, 0, source_field, _record.byte_ptr(), &_hilfspuffer );
            //b->_field->assign( 0, source_field, _record.byte_ptr(), &_hilfspuffer );

            // Vornullen müssen hier unterdrückt werden (also nicht Text_format.raw()).
            // Für Access 2.0 sind C_CHAR "00" und C_SHORT 0 verschieden!!!

            try {
                b->_field->type_ptr()->read_other_field(
                    b->_field->ptr( 0 ),
                    source_field->type_ptr(), source_field->const_ptr( _record.byte_ptr() ),
                    &_hilfspuffer, std_text_format );
            }
            catch( Xc& x )
            {
                x.insert( source_field );
                throw;
            }

            if( b->_pcbValue )  {
                *b->_pcbValue = b->_default_length != SQL_NO_TOTAL?
                                b->_default_length
                              : b->_field->type().field_length( (const Byte*)b->rgbValue() );
            }
/*?
            if( ( b->_fCType == SQL_C_CHAR
                  ||  b->_fCType == SQL_C_DEFAULT
                      &&  odbc_c_default_type( *b->_field->type_ptr()->info() ) == SQL_C_CHAR )
             &&  b->_cbValueMax >= 2
             && ((char*)b->rgbValue())[0] == '\0' )
            {
                // Für Access 1.1, 2.0, kommt sonst bei UPDATE in der WHERE-Klausel durcheinander (?)
                // Ein Leerstring gilt in ACCESS manchmal(?) als NULL-Wert.
                // MS-ACCESS 2.0 zählt bei SQLBindParameter die Fragezeichen von SQLPrepare falsch.
                ((char*)b->rgbValue())[0] = ' ';
                ((char*)b->rgbValue())[1] = '\0';
                if( b->_pcbValue )  *b->_pcbValue = 1;
            }
*/
        }

        if( log_ptr ) {
            *log_ptr << "    " << *source_field << " -> " << *b->_field <<  " = ";
            if( *b->_pcbValue == SQL_NULL_DATA )  *log_ptr << "NULL\n";
            else {
                *log_ptr << '"';
                b->_field->print( 0, log_ptr, std_text_format );
                *log_ptr << "\"\n";
            }
        }
    } else {
        if( b->_pcbValue )  *b->_pcbValue = SQL_NO_TOTAL;
    }
}

//-------------------------------------------------------------------------------------SQLFetch

DEFINE_ODBC_CALL_0( Sos_odbc_stmt, SQLFetch, HSTMT )

RETCODE Sos_odbc_stmt::SQLFetch( HSTMT )
{
    if( _eof )  {
        //ODBC_LOG( "_eof=true\n" );
        return SQL_NO_DATA_FOUND;     // SQLSpecialColumns
    }

    try {
        _file.get( &_record );
    }
    catch( const Eof_error& ) {
        _eof = true;
        return SQL_NO_DATA_FOUND;
    }
    _row_no++;

    for( int i = 1; i <= _col_bindings.last_index(); i++ )
    {
        Sos_odbc_binding* b = &_col_bindings[ i ];
        if( b->_field )  copy_value_to_odbc( b, _record_type->field_descr_ptr( i - 1 ) );
    }

    return SQL_SUCCESS;
}

//-----------------------------------------------------------------------------------SQLGetData

DEFINE_ODBC_CALL_5( Sos_odbc_stmt, SQLGetData, HSTMT,
                    UWORD, SWORD, PTR, SDWORD, SDWORD* )

RETCODE Sos_odbc_stmt::SQLGetData(
    HSTMT, 
	UWORD	   icol,
	SWORD	   fCType,
	PTR 	   rgbValue,
	SDWORD	   cbValueMax,
	SDWORD FAR*pcbValue)
{
    if( !_record_type )  throw_xc( "SOS-1193" );  // Sollte nicht passieren
    if( icol > _field_count )  { sql_error( "S1002" ); return SQL_NO_DATA_FOUND; }  // "invalid column number"

    Field_descr*      f = _record_type->field_descr_ptr( icol - 1 );

    if( !f             )  throw_xc( "Sos_odbc_stmt::SQLGetData", "no Field_descr" );  //? jz 21.5.97
    if( !f->type_ptr() )  throw_xc( "Sos_odbc_stmt::SQLGetData", "no Field_type"  );  //? jz 21.5.97

    Sos_odbc_binding* b = &_cached_bindings[ icol ];

    /// CACHE:
    if( b->_fCType != fCType  ||  b->_cbValueMax != cbValueMax  ||  !b->_field ) {
      //b->_valid      = true;
        b->_fCType     = fCType;
        b->_cbValueMax = cbValueMax;
        b->_fParamType = SQL_PARAM_OUTPUT;
        b->prepare( f );
      //b->_field->name( "SQLGetData_" + as_string( icol ) );
        b->_field->name( f->name() );
    }

    b->_pcbValue      = pcbValue;
    b->_field->_offset = (long)rgbValue;

    copy_value_to_odbc( b, f );

    return SQL_SUCCESS;
}

//-------------------------------------------------------------------------------SQLMoreResults
// Extension Level 2

extern "C" RETCODE SQL_API SQLMoreResults( HSTMT hstmt )
{
    ODBC_LOG( "SQLMoreResults()\n" );
    return SQL_ERROR;
}

//----------------------------------------------------------------------------------SQLRowCount
//	Return 1 in most cases this is the right answer.
//	may need to alter the Mini-SQL server to return this
//	information.
DEFINE_ODBC_CALL_1( Sos_odbc_stmt, SQLRowCount, HSTMT,
                    SDWORD* )

RETCODE Sos_odbc_stmt::SQLRowCount( HSTMT, SDWORD FAR*pcrow )
{
  //*pcrow = _eof? 0 : 1;
    *pcrow = _file.record_count();
    LOG( "SQLRowCount *pcrow=" << *pcrow << "\n" );

	return SQL_SUCCESS;
}

//------------------------------------------------------------------------------------SQLSetPos

extern "C" RETCODE SQL_API SQLSetPos(
	HSTMT	hstmt,
	UWORD	irow,
	UWORD	fOption,
	UWORD	fLock)
{
    ODBC_LOG( "SQLSetPos()\n" );
    return SQL_ERROR;
}

//-----------------------------------------------------------------------------SQLExtendedFetch

extern "C" RETCODE SQL_API SQLExtendedFetch(
	HSTMT	   hstmt,
	UWORD	   fFetchType,
	SDWORD	   irow,
	UDWORD FAR *pcrow,
	UWORD  FAR *rgfRowStatus)
{
    ODBC_LOG( "SQLExtendedFetch()\n" );
	return SQL_ERROR;
}


} //namespace sos
