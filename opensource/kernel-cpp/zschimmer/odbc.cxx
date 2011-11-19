// zschimmer.cxx                                    © 2000 Joacim Zschimmer
// $Id: odbc.cxx 11394 2005-04-03 08:30:29Z jz $

#include "zschimmer.h"
#include "regex_class.h"

#ifdef Z_BORLAND
#	pragma package(smart_init)
#endif

#include <stdio.h>

#ifdef Z_WINDOWS
#   include <windows.h>
#   define snprintf _snprintf
#endif


namespace zschimmer {
namespace odbc {

//-------------------------------------------------------------------------Odbc_handle::Odbc_handle

Odbc_handle::Odbc_handle( SMALLINT type, SQLHANDLE outer_handle )
: 
    _type(type),
    _handle(0)
{
    SQLRETURN ret = SQLAllocHandle( type, outer_handle, &_handle );  
    check_return(ret); 
}

//------------------------------------------------------------------------Odbc_handle::~Odbc_handle

Odbc_handle::~Odbc_handle()
{
    if( _handle )  
    {
        SQLFreeHandle( _type, _handle );
    }
}

//-------------------------------------------------------------------------------Odbc_handle::close

void Odbc_handle::close()
{
    if( _handle )  
    {
        SQLRETURN ret = SQLFreeHandle( _type, _handle );
        check( ret );
        _handle = 0;
    }
}

//-------------------------------------------------------------------------------Odbc_handle::check

void Odbc_handle::check( SQLRETURN ret )
{
    if( ret )  
    {
        throw_xc( "ODBC-??" );
    }
}

//------------------------------------------------------------------------------Conn::DriverConnect

void Conn::DriverConnect( SQLHWND wnd, const string& conn, string* conn_ret, SQLUSMALLINT DriverCompletion )
{
    SQLRETURN   ret;
    char        buffer [1024];
    SQLSMALLINT len = 0;

    ret = SQLDriverConnect( *this, wnd, conn.data(), conn.length(), conn_ret? buffer : NULL, conn_ret? sizeof buffer : 0, &len, DriverCompletion );
    check(ret);

    if( conn_ret )  *conn_ret = string( buffer, len );
}

//----------------------------------------------------------------------------Conn::connect_to_file

void Conn::connect_to_file( const string& filename )
{
    RETCODE     retcode;
    string      fileextns  = "FileExtns=*." + extension_of_path( filename );
    int         first_next = SQL_FETCH_FIRST;
    char        description[1000],  attributes[1000];
    SQLSMALLINT description_length, attributes_length;

    while(1)
    {
        retcode = SQLDrivers( *_env, first_next, 
                              description, sizeof description, &description_length,
                              attributes , sizeof attributes , &attributes_length   );
        if( retcode == SQL_NO_DATA )  throw_xc( "SOS-1445", filename );
        _env->check( retcode, "SQLDrivers" );

        const char* p = attributes.char_ptr();
        while( *p )
        {
            if( stricmp( p, fileextns.c_str() ) == 0 )  break;
            p += strlen( p ) + 1;
        }

        first_next = SQL_FETCH_NEXT;
    }

    DriverConnect( NULL, "DRIVER=" + description + ";DBQ=" + filename );
}

//------------------------------------------------------------------------------------Stmt::Prepare

void Stmt::Prepare( const string& stmt )
{
    SQLRETURN ret = SQLPrepare( *this, stmt.data(), stmt.length() );
    check(ret);
}

//------------------------------------------------------------------------------------Stmt::BindCol

void Stmt::BindCol( SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, 
                    SQLPOINTER TargetValuePtr, SQLINTEGER BufferLength, SQLLEN* StrLen_or_Ind )
{
    SQLRETURN ret = SQLBindCol( *this, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind );
    check(ret);
} 

//------------------------------------------------------------------------------------Stmt::Execute

void Stmt::Execute()
{
    SQLRETURN ret = SQLExecute( *this );
}
 
//-------------------------------------------------------------------------------------------------

} //namespace odbc
} //namespace zschimmer

