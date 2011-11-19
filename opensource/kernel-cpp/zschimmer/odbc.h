// zschimmer.h                                      ©2000 Joacim Zschimmer
// $Id: odbc.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __ZSCHIMMER_ODBC_H
#define __ZSCHIMMER_ODBC_H

#include <windows.h>
#include <sqlext.h>
#include <sql.h>
#include "zschimmer.h"

namespace zschimmer {
namespace odbc {

//-------------------------------------------------------------------------------------------------

void check_return( SQLRETURN );

//--------------------------------------------------------------------------------------Odbc_handle

struct Odbc_handle : Object
{
                                Odbc_handle             ( SQLSMALLINT type, SQLHANDLE outer_handle );
    virtual                    ~Odbc_handle             ();

    virtual                     close                   ();
    void                        check                   ( SQLRETURN );
    

    SQLSMALLINT                _type;
    SQLHANDLE                  _handle;
};

//----------------------------------------------------------------------------------------------Env

struct Env : Odbc_handle
{
                                Env                     ()                                          : Odbc_handle( SQL_HANDLE_ENV, SQL_NULL_HANDLE ) {}

                                operator SQLHENV        ()                                          { return (SQLHENV)_handle; }
};

//---------------------------------------------------------------------------------------------Conn

struct Conn : Odbc_handle
{
                                Conn                    ( Env* env )                                : Odbc_handle( SQL_HANDLE_DBC, *env ), _env(env) {}

    void                        DriverConnect           ( SQLHWND, const string& conn, string* conn_ret = NULL, SQLUSMALLINT DriverCompletion = SQL_DRIVER_NOPROMPT );
    void                        connect_to_file         ( const string& filename );

                                operator SQLHDBC        ()                                          { return (SQLHDBC)_handle; }

    ptr<Env>                   _env;
};

//---------------------------------------------------------------------------------------------Stmt

struct Stmt : Odbc_handle
{
                                Stmt                    ( Conn* conn )                              : Odbc_handle( SQL_HANDLE_DBC, *conn ), _conn(conn) {}
                               ~Stmt                    ();

                                operator SQLHDBC        ()                                          { return (SQLHDBC)_handle; }

    void                        Prepare                 ( const string& stmt );
    void                        BindCol                 ( SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, 
                                                          SQLPOINTER TargetValuePtr, SQLINTEGER BufferLength, SQLLEN* StrLen_or_Ind = NULL );
    void                        Execute                 ();
    bool                        Fetch                   ();                                         // false: EOF

    
    ptr<Conn>                  _conn;
};

//-------------------------------------------------------------------------------------------------

} //namespace odbc
} //namespace zschimmer


#endif
