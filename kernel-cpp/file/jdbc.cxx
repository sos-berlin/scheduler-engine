// $Id: jdbc.cxx 14579 2011-06-08 09:34:04Z jz $
// §968

#include "precomp.h"
#include <stdio.h>
#include "../kram/sysdep.h"

#include <string.h>

#if defined SYSTEM_WIN
#   include <windows.h>
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#endif

#if defined SYSTEM_UNIX
#   include <stdio.h>               // fileno
#   include <unistd.h>              // read(), write(), close()
#   include <errno.h>
#   include <unistd.h>
#endif


#include "../kram/sosstrng.h"
#include "../kram/sos.h"

#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/stdfield.h"
#include "../kram/log.h"
#include "../kram/sosdate.h"
#include "../file/absfile.h"
#include "../file/sosdb.h"

//#include "../file/jdbctype.h"
#include "../kram/env.h"
#include "../kram/sos_java.h"
#include "../zschimmer/java.h"
#include "../zschimmer/file.h"


/*
JDBC ™ 3.0 Specification

8.3.1 Silent Truncation

The Statement.setMaxFieldSize method allows a maximum size (in bytes) to
be set. This limit applies only to the BINARY, VARBINARY, LONGVARBINARY, CHAR,
VARCHAR and LONGVARCHAR data types.

If a limit has been set using setMaxFieldSize and there is an attempt to read or
write data that exceeds the limit, any truncation that occurs as a result of exceeding
the set limit will not be reported.

*/

using namespace std;
using namespace zschimmer;
using namespace zschimmer::file;
using namespace zschimmer::javabridge;


namespace sos {

struct Jdbc_static;

//----------------------------------------------------------------------------------------Jdbc_type

enum Jdbc_type
{
    jdbc_type_array         = 2003,
    jdbc_type_bigint        = -5, 
    jdbc_type_binary        = -2, 
    jdbc_type_bit           = -7, 
    jdbc_type_blob          = 2004, 
    jdbc_type_boolean       = 16, 
    jdbc_type_char          = 1, 
    jdbc_type_clob          = 2005, 
    jdbc_type_datalink      = 70, 
    jdbc_type_date          = 91, 
    jdbc_type_decimal       = 3, 
    jdbc_type_distinct      = 2001, 
    jdbc_type_double        = 8, 
    jdbc_type_float         = 6, 
    jdbc_type_integer       = 4, 
    jdbc_type_java_object   = 2000, 
    jdbc_type_longvarbinary = -4, 
    jdbc_type_longvarchar   = -1, 
    jdbc_type_null          = 0, 
    jdbc_type_numeric       = 2, 
    jdbc_type_other         = 1111, 
    jdbc_type_real          = 7, 
    jdbc_type_ref           = 2006, 
    jdbc_type_smallint      = 5, 
    jdbc_type_struct        = 2002, 
    jdbc_type_time          = 92, 
    jdbc_type_timestamp     = 93, 
    jdbc_type_tinyint       = -6, 
    jdbc_type_varbinary     = -3, 
    jdbc_type_varchar       = 12 
};

//-----------------------------------------------------------------------------------------Jdbc_session

struct Jdbc_session : Sos_database_session
{
                                Jdbc_session            ( Sos_database_static* );
                               ~Jdbc_session            ();

    void                       _open                    ( Sos_database_file* );
    void                       _close                   ( Close_mode = close_normal );
    void                       _execute_direct          ( const Const_area& );
  //void                        execute_direct_without_error_check( const Const_area& );
    void                       _commit                  ();
    void                       _rollback                ();
    string                      name                    ()                                      { return "jdbc"; }
    string                      modify_oracle_thin_stmt ( const string& );
    void                       _open_postprocessing     ();
    ::stdext::hash_map<string,string> properties        ();

    Jdbc_static*                static_ptr              ()                                      { return (Jdbc_static*)_static; }

    void                       _obj_print               ( ostream* ) const;

    Fill_zero                  _zero_;
    Class                      _jdbc_driver_class;
    Global_jobject             _jdbc_driver;
    Global_jobject             _jdbc_connection;
    Global_jobject             _jdbc_statement;
    bool                       _auto_commit;
    string                     _driver_class_name;
    string                     _driver_name;
    bool                       _connection_get_meta_data_not_implemented;
    bool                       _lob_get_precision_not_implemented;
};

//--------------------------------------------------------------------------------------Jdbc_static

struct Jdbc_static : Sos_database_static
{
                                Jdbc_static              ();
                               ~Jdbc_static              ();

    Jdbc_static*                static_ptr               ()                                     { return this; }

    ptr<javabridge::Vm>        _java_vm;
    Class                      _class_class;
    Class                      _driver_manager_class;
};

//----------------------------------------------------------------------------------------Jdbc_file

struct Jdbc_file : Sos_database_file
{
                                Jdbc_file               ();
                               ~Jdbc_file               ();

    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode  );
    void                        bind_parameters         ( const Record_type*, const Byte* );

    virtual void                put_record              ( const Const_area&  );
            void                put_lob_file            ();
    virtual void                get_record              ( Area&              );
    virtual void                get_lob_record          ( Area&              );

    virtual void               _create_static           ();
    
    virtual Sos_ptr<Sos_database_session> 
                               _create_session          ();

  private:
    void                        prepare_stmt            ( const Sos_string& );
    void                        describe_columns        ();
    void                        bind_columns            ();
    void                        execute_stmt            ();

    Jdbc_static*                static_ptr              ()                                          { return (Jdbc_static*)+_static; }
    Jdbc_session*               session                 ()                                          { return (Jdbc_session*)+_session; }


    friend struct               Jdbc_session;

    Fill_zero                  _zero_;
    bool                       _field_as_file;
    int                        _fixed_length;
    int                        _max_length;
    string                     _driver_class_name;
    Global_jobject             _jdbc_statement;
    Global_jobject             _jdbc_result_set;
  //Global_jobject             _jdbc_output_stream;
    Global_jobject             _jdbc_input_stream;
    bool                       _has_result_set;
    vector<int>                _jdbc_columns;
    vector<Jdbc_type>          _jdbc_column_types;
    int                        _column_count;
    Sos_ptr<Record_type>       _type;
    zschimmer::file::File      _lob_file;
    bool                       _oracle_lob;
    int                        _oracle_lob_block_size;
    bool                       _is_clob;                    // CLOB, sonst BLOB
    bool                       _commit_at_close;
  //Sos_simple_array<Sos_odbc_binding> _result_bindings;
};

//-----------------------------------------------------------------------------------Jdbc_file_type

struct Jdbc_file_type : Abs_file_type
{
    Jdbc_file_type() : Abs_file_type() {};

    virtual const char*         name                    () const                                    { return "jdbc"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Jdbc_file> f = SOS_NEW( Jdbc_file );
        return +f;
    }
};

//--------------------------------------------------------------------------------------------const

const Jdbc_file_type  _jdbc_file_type;
const Abs_file_type&  jdbc_file_type = jdbc_file_type;

//--------------------------------------------------------------------------------name_of_jdbc_type

static string name_of_jdbc_type( Jdbc_type type )
{
    switch( type )
    {
        case jdbc_type_array:           return "ARRAY";
        case jdbc_type_bigint:          return "BIGINT";
        case jdbc_type_binary:          return "BINARY";
        case jdbc_type_bit:             return "BIT";
        case jdbc_type_blob:            return "BLOB";
        case jdbc_type_boolean:         return "BOOLEAN";
        case jdbc_type_char:            return "CHAR";
        case jdbc_type_clob:            return "CLOB";
        case jdbc_type_datalink:        return "DATALINK";
        case jdbc_type_date:            return "DATE";
        case jdbc_type_decimal:         return "DECIMAL";
        case jdbc_type_distinct:        return "DISTINCT";
        case jdbc_type_double:          return "DOUBLE";
        case jdbc_type_float:           return "FLOAT";
        case jdbc_type_integer:         return "INTEGER";
        case jdbc_type_java_object:     return "JAVA_OBJECT";
        case jdbc_type_longvarbinary:   return "LONGVARBINARY";
        case jdbc_type_longvarchar:     return "LONGVARCHAR";
        case jdbc_type_null:            return "NULL";
        case jdbc_type_numeric:         return "NUMERIC";
        case jdbc_type_other:           return "OTHER";
        case jdbc_type_real:            return "REAL";
        case jdbc_type_ref:             return "REF";
        case jdbc_type_smallint:        return "SMALLINT";
        case jdbc_type_struct:          return "STRUCT";
        case jdbc_type_time:            return "TIME";
        case jdbc_type_timestamp:       return "TIMESTAMP";
        case jdbc_type_tinyint:         return "TINYINT";
        case jdbc_type_varbinary:       return "VARBINARY";
        case jdbc_type_varchar:         return "VARCHAR";
        default:                        return "jdbc_type-" + as_string( (int)type );
    }
}

//-------------------------------------------------------------------------Jdbc_static::Jdbc_static

Jdbc_static::Jdbc_static()
{
    _java_vm = get_java_vm();
    _java_vm->start();

    _class_class.load( "java/lang/Class" );
    _driver_manager_class.load( "java/sql/DriverManager" );
}

//------------------------------------------------------------------------Jdbc_static::~Jdbc_static

Jdbc_static::~Jdbc_static()
{
    _driver_manager_class = NULL;
    _class_class = NULL;
    _java_vm = NULL;
}

//-----------------------------------------------------------------------Jdbc_session::Jdbc_session

Jdbc_session::Jdbc_session( Sos_database_static* st )
:
    Sos_database_session( st ),
    _zero_(this+1)
    //_jdbc_connection  ( static_ptr()->_java_vm ),
    //_jdbc_statement   ( static_ptr()->_java_vm ),
    //_jdbc_driver_class( static_ptr()->_java_vm ),
    //_jdbc_driver      ( static_ptr()->_java_vm )
{
    _date_format            = std_date_format_odbc;         // {d'yyyy-mm-dd'}
    _date_time_format       = std_date_time_format_odbc;    // {dt'yyyy-mm-dd HH:MM:SS'}
    //_identifier_quote_begin = "[";
    //_identifier_quote_end   = "]";
    //_identifier_quote_begin = "\"";
    //_identifier_quote_end   = _identifier_quote_begin;

    _debug |= log_category_is_set( "jdbc" );
}

//----------------------------------------------------------------------Jdbc_session::~Jdbc_session

Jdbc_session::~Jdbc_session()
{
    try 
    {
        _close();
    }
    catch( const exception& ) {}

    if( _jdbc_statement )  
    {
        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.sql.Statement.close()\n" );
            _jdbc_statement.call_void_method( "close", "()V" );
        }
        catch( const exception& ) {}

        _jdbc_statement = NULL;
    }

    if( _jdbc_connection )  
    {
        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.sql.Connection.close()\n" );
            _jdbc_connection.call_void_method( "close", "()V" );
        }
        catch( const exception& ) {}

        _jdbc_connection = NULL;
    }
}

//------------------------------------------------------------------------------Jdbc_session::_open

void Jdbc_session::_open( Sos_database_file* db_file )
{
    Jdbc_file*  file = (Jdbc_file*)db_file;

    if( file ) 
    {
        if( file->_auto_commit )  _auto_commit = true;
        _driver_class_name = file->_driver_class_name;
    }


    Env env;
    Local_frame local_frame( 20 );


    if( _driver_class_name != "" )
    {
#if 0  // Mit Class.forName()
        Class         class_loader_class ( static_ptr()->_java_vm, "java/lang/ClassLoader" );
        Local_jobject class_loader       ( static_ptr()->_java_vm );
        Local_jobject cls                ( static_ptr()->_java_vm );                     // Dann mit java.lang.Class.forName(), denn dieser Aufruf lädt nicht aus der .jar (Linux)
        class_loader = class_loader_class.call_static_object_method( "getSystemClassLoader", "()Ljava/lang/ClassLoader;" );

        Z_LOG2( "jdbc", "java Class.forName(\"" << _driver_class_name << "\")\n" );
        //cls = env->CallStaticObjectMethod( static_ptr()->_class_class,
        //                                 static_ptr()->_class_class.static_method_id( "forName", "(Ljava/lang/String;)Ljava/lang/Class;" ),
        //                                 (jstring)Local_jstring( _driver_class_name ) );
        cls = env->CallStaticObjectMethod( static_ptr()->_class_class,
                                         static_ptr()->_class_class.static_method_id( "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;" ),
                                         (jstring)Local_jstring( _driver_class_name ),
                                         (jboolean)true,
                                         (jobject)class_loader );

        if( !cls )  env.throw_java( "Class.forName", _driver_class_name );
        _jdbc_driver = env->CallObjectMethod( cls, static_ptr()->_class_class.method_id( "newInstance", "()Ljava/lang/Object;" ) );
#else
        _jdbc_driver_class.load( _driver_class_name );
        jobject jdbc_driver = env->CallObjectMethod( _jdbc_driver_class, static_ptr()->_class_class.method_id( "newInstance", "()Ljava/lang/Object;" ) );
#endif
        //_jdbc_driver = env->CallObjectMethod( c, env->GetMethodID( c, "newInstance", "()Ljava/lang/Object;" ) );
        if( !jdbc_driver || env->ExceptionCheck() )  env.throw_java( "newInstance", _driver_class_name );

        _jdbc_driver = jdbc_driver;

        
        // Verbindung über den geladenen Treiber öffnen

        Local_jobject properties = Class( "java/util/Properties" ).new_object( "()V" );

        properties.call_object_method( "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;",
                                       (jstring)Local_jstring( "user" ),    
                                       (jstring)Local_jstring( db_file->_user ) );

        properties.call_object_method( "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;",
                                       (jstring)Local_jstring( "password" ),    
                                       (jstring)Local_jstring( db_file->_password ) );

        _jdbc_connection = _jdbc_driver.call_object_method( "connect", "(Ljava/lang/String;Ljava/util/Properties;)Ljava/sql/Connection;", 
                                                            (jstring)Local_jstring( db_file->_db_name ),
                                                            (jobject)properties );
        if( !_jdbc_connection )  throw_xc( "SOS-1453", _driver_class_name, db_file->_db_name );
    }
    else
    {
        _jdbc_connection = env->CallStaticObjectMethod( static_ptr()->_driver_manager_class, 
                                                      static_ptr()->_driver_manager_class.static_method_id(  "getConnection", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/sql/Connection;" ),
                                                      (jstring)Local_jstring( db_file->_db_name ),
                                                      (jstring)Local_jstring( db_file->_user ),
                                                      (jstring)Local_jstring( db_file->_password ) );
        if( !_jdbc_connection || env->ExceptionCheck() )  env.throw_java( "java.sql.DriverManager.getConnection", db_file->_db_name );
    }


    Local_jobject meta_data ( _jdbc_connection.call_object_method( "getMetaData", "()Ljava/sql/DatabaseMetaData;" ) );
    if( meta_data )
    {
        _dbms_name   = meta_data.call_string_method( "getDatabaseProductName", "()Ljava/lang/String;" );
        _driver_name = meta_data.call_string_method( "getDriverName"         , "()Ljava/lang/String;" );

        Z_LOG( "DatabaseProductName=" << _dbms_name << ", DriverName=" << _driver_name << '\n' );

        if( _dbms_name == "ACCESS"               )  _dbms = dbms_access;
        else
        if( _dbms_name == "Oracle" )  
        {
            if( string_begins_with( _db_name, "jdbc:oracle:thin:" ) )  
                _dbms = dbms_oracle_thin;
            else
                _dbms = dbms_oracle;
        }
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
        if( _dbms_name == "Adaptive Server Enterprise" )  _dbms = dbms_sybase;      // Über jconn3
        else
        if( _dbms_name == "ASE"                        )  _dbms = dbms_sybase;      // Über jTDS
        else
        if( _dbms_name == "H2"                         )  _dbms = dbms_h2;
    }

    _jdbc_connection.call_void_method( "setAutoCommit", "(Z)V", (jboolean)_auto_commit );


    if( _dbms == dbms_oracle_thin )  _lob_get_precision_not_implemented = true;   // Oracles getPrecision() liefert java.lang.NumberFormatException: 4294967295 


/*
    SQLWarning w = c.getWarnings();
    while( w != null )
    {
        System.out.println( "Connect Error: " + w );
        w = w.getNextWarning();
    }

    c.clearWarnings();
*/

}

//-------------------------------------------------------------------------Jdbc_session::properties

Sos_database_session::Properties Jdbc_session::properties() {
    Properties result = Sos_database_session::properties();
    result["jdbc.driverClass"] = _driver_class_name;
    result["jdbc.driverName"] = _driver_name;
    return result;
}

//-----------------------------------------------------------------------------Jdbc_session::_close

void Jdbc_session::_close( Close_mode )
{
    if( _jdbc_statement )  
    {
        Local_frame local_frame ( 10 );

        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.sql.Statement.close()\n" );
            _jdbc_statement.call_void_method( "close", "()V" );
        } 
        catch( const exception& x ) { Z_LOG( "java.sql.Statement.close() ==> " << x << "\n" ); }      // Ignorieren (wie in odbc.cxx)

        _jdbc_statement = NULL;
    }

    if( _jdbc_connection )
    {
        Local_frame local_frame ( 10 );

        try 
        {
            _rollback();
        }
        catch( const exception& x ) { Z_LOG( "java.sql.Connection.rollback(): " << x.what() << '\n' ); }

        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.sql.Connection.close()\n" );
            _jdbc_connection.call_void_method( "close", "()V" );
        } 
        catch( const exception& x ) { Z_LOG( "java.sql.Connection.close() ==> " << x << "\n" ); }      // Ignorieren (wie in odbc.cxx)

        _jdbc_connection = NULL;
    }

    _jdbc_driver = NULL;
    _jdbc_driver_class = NULL;
}

//------------------------------------------------------------Jdbc_session::modify_oracle_thin_stmt

string Jdbc_session::modify_oracle_thin_stmt( const string& stmt )
{
    // {ts'yyyy-mm-dd hh:mm:ss'} in to_date('yyyy-mm-dd hh:mm:ss','YYYY-MM-DD HH24:MI:SS') umsetzen, 
    // weil der JDBC-Thin-Treiber die Uhrzeit nicht akzeptiert, denn er hält Oracle-Date für ein Jdbc-Date.

    if( stmt.find( "{ts'" ) == string::npos )  return stmt;


    string modified_stmt;

    modified_stmt.reserve( stmt.length() + 50 );

    const char* p = stmt.c_str();
    while( p[0] )
    {
        const char* p0 = p;

        while( p[0]  &&  p[0] != '{' )
        {
            while( p[0]  &&  p[0] != '{'  &&  p[0] != '\''  &&  p[0] != '"' )  p++;

            if( p[0] == '\''  ||  p[0] == '"' )  
            {
                char quote = p[0];
                p++;
                while( p[0]  &&  p[0] != quote )  p++;
                if( p[0] == quote )  p++;
            }
        }
                
        modified_stmt.append( p0, p-p0 );

        if( strncmp( p, "{ts'", 4 ) == 0 )
        {
            modified_stmt += "to_date('";
            p += 4;
            const char* p0 = p;
            while( p[0] && p[0] != '\'' )  p++;
            modified_stmt.append( p0, p-p0 );
            if( p[0] == '\'' )  p++;
            if( p[0] == '}' )  p++;
            modified_stmt.append( "','YYYY-MM-DD HH24:MI:SS')" );
        }
    }

    return modified_stmt;
}

//--------------------------------------------------------------------Jdbc_session::_execute_direct

void Jdbc_session::_execute_direct( const Const_area& stmt_area )
{
    Local_frame local_frame ( 10 );

    if( !_jdbc_statement )  _jdbc_statement = _jdbc_connection.call_object_method( "createStatement", "()Ljava/sql/Statement;" );


    string stmt ( stmt_area.char_ptr(), stmt_area.length() );

    if( _dbms == dbms_oracle_thin )  stmt = modify_oracle_thin_stmt( stmt );

    Z_LOG2( "jdbc", "jdbc: executeUpdate  " << stmt << flush );

    _row_count = _jdbc_statement.call_int_method( "executeUpdate", "(Ljava/lang/String;)I", (jstring)Local_jstring( stmt ) );
    
    Z_LOG2( "jdbc", "  row_count=" << _row_count << "\n" );
}

//-------------------------------------------------Jdbc_session::execute_direct_without_error_check
/*
void Jdbc_session::execute_direct_without_error_check( const Const_area& )
{
}
*/
//----------------------------------------------------------------------------Jdbc_session::_commit

void Jdbc_session::_commit()
{
    _jdbc_connection.call_void_method( "commit", "()V" );
}

//--------------------------------------------------------------------------Jdbc_session::_rollback

void Jdbc_session::_rollback()
{
    if( _jdbc_connection ) 
    {
        _jdbc_connection.call_void_method( "rollback", "()V" );
    }
}

//-------------------------------------------------------------------------Jdbc_session::_obj_print

void Jdbc_session::_obj_print( ostream* s ) const
{
    Sos_database_session::_obj_print( s );
    //*s << " JDBC version " << ( _jdbc_version >> 8 ) << '.' << ( _jdbc_version & 0xFF );
}

// ----------------------------------------------------------------------------Jdbc_file::Jdbc_file

Jdbc_file::Jdbc_file()
:
    _zero_(this+1),
    //_jdbc_statement( NULL ),
    //_jdbc_result_set( NULL ),
    //_jdbc_input_stream( NULL ),
    _max_length( INT_MAX )
{
    //_result_bindings.obj_const_name( "Odbc_file::_result_bindings" );
}

//----------------------------------------------------------------------------Jdbc_file::~Jdbc_file

Jdbc_file::~Jdbc_file()
{
    if( _jdbc_statement )  
    {
        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.sql.Statement.close()\n" );
            _jdbc_statement.call_void_method( "close", "()V" );
        }
        catch( const exception& ) {}

        _jdbc_statement = NULL;
    }

    if( _jdbc_result_set )  
    {
        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.sql.ResultSet.close()\n" );
            _jdbc_result_set.call_void_method( "close", "()V" );
        }
        catch( const exception& ) {}

        _jdbc_result_set = NULL;
    }

    if( _jdbc_input_stream )  
    {
        try
        {
            if( _debug )  Z_LOG2( "jdbc", "java.io.InputStream.close()\n" );
            _jdbc_input_stream.call_void_method( "close", "()V" );
        }
        catch( const exception& ) {}

        _jdbc_input_stream = NULL;
    }

    _jdbc_column_types.clear();
    _type = NULL;

    session_disconnect();
}

//------------------------------------------------------------------------Jdbc_file::_create_static

void Jdbc_file::_create_static()
{
    Sos_ptr<Jdbc_static> o = SOS_NEW( Jdbc_static );
    _static = +o;
}

//-----------------------------------------------------------------------Jdbc_file::_create_session

Sos_ptr<Sos_database_session> Jdbc_file::_create_session()
{
    Sos_ptr<Jdbc_session> o = SOS_NEW( Jdbc_session( _static ) );
    return +o;
}

// ------------------------------------------------------------------------Jdbc_file::_prepare_open

void Jdbc_file::prepare_open( const char* filename, Open_mode open_mode, const File_spec& )
{
    Sos_string select_statement;
    Sos_string lob;
    Sos_string table_name;
  //bool       default_resultset_set = false;
    bool       zero_update_allowed = false;

    if( ( open_mode & (in|out) ) == 0 )  throw_xc( "D131" );

    _open_mode = open_mode;
    _assert_row_count = -1;
    _fixed_length = 30*1024;    // bei 16bit besser weniger als 32768.
    
    get_static( "jdbc" );

    //set_vm( static_ptr()->_java_vm );
    
    //_jdbc_statement   .set_vm( static_ptr()->_java_vm );
    //_jdbc_result_set  .set_vm( static_ptr()->_java_vm );
    //_jdbc_input_stream.set_vm( static_ptr()->_java_vm );

    Env env;
    Local_frame java_frame ( 10 );

    for ( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( database_option( opt ) )            { /* do nothing */ }
        else
        if( opt.with_value( "max-length" ) )    _max_length = opt.as_uintK();
        else
        if( opt.with_value( "class" ) )         _driver_class_name = opt.value();
        else
        if( opt.with_value( "blob" ) )          lob = opt.value();
        else
        if( opt.with_value( "clob" ) )          lob = opt.value(), _is_clob = true;
        else
        if( opt.flag      ( "no-blob-record-allowed" ) )  zero_update_allowed = opt.set();
        else
        if( opt.with_value( "table" ) )         table_name = opt.value();
        else
        if( opt.with_value( "fixed-length" ) )  _fixed_length = opt.as_uintK();
        else
        if( opt.flag      ( "commit" ) )        _commit_at_close = true;
        else
        if( opt.param(1) )                      _db_name = opt.value(); 
        else
        if( opt.param() )                       { select_statement = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    //s.u. if( _field_as_file  &&  _fixed_length == 0 )  throw_xc( "SOS-1334" );

    // Connection aufbauen, wenn nicht schon passiert
    session_connect( "jdbc" );


    if( !empty( lob ) ) 
    {
        _field_as_file = true;  
        if( _assert_row_count < 0  &&  !zero_update_allowed )  _assert_row_count = 1;

        if( empty( select_statement ) )  throw_xc( "SOS-1397" );

        if( open_mode & out ) 
        {
            if( _session->_dbms == dbms_oracle_thin )
            {
                _oracle_lob = true;

                try
                {
                    _session->execute_direct_single( "UPDATE " + ucase(table_name) + " "
                                                     "SET \"" + ucase(lob) + "\"=" + ( _is_clob? "empty_clob() " : "empty_blob() " ) + select_statement );
                }
                catch( const exception& )
                {
                    _is_clob = !_is_clob;
                    _session->execute_direct_single( "UPDATE " + ucase(table_name) + " "
                                                     "SET \"" + ucase(lob) + "\"=" + ( _is_clob? "empty_clob() " : "empty_blob() " ) + select_statement );
                }

                select_statement = "SELECT \"" + ucase(lob) + "\"  FROM " + ucase(table_name) + "  " + select_statement + "  FOR UPDATE";
            }
            else
            {
                select_statement = "UPDATE " + ucase(table_name) + " SET \"" + ucase(lob) + "\"=? " + select_statement;
            }

            _lob_file.open_temporary();
            _lob_file.unlink_later( true );
        } 
        else 
        {
            select_statement = "SELECT \"" + ucase(lob) + "\" FROM " + ucase(table_name) + " " + select_statement;
        }
    }


    if( !select_statement.empty() )
    {
        Dynamic_area stmt;
        _session->convert_stmt( Const_area( c_str( select_statement ), length( select_statement ) ), &stmt );
        select_statement = string( stmt.char_ptr(), length( stmt ) );

        Z_LOG2( "jdbc", "jdbc: prepareStatement  " << select_statement << "\n" );
        _jdbc_statement = session()->_jdbc_connection.call_object_method("prepareStatement", "(Ljava/lang/String;)Ljava/sql/PreparedStatement;",
                                                                         (jstring)Local_jstring( select_statement ) );

        if( _field_as_file )
        {
            //if( _open_mode & out )  bind_lob_parameter();
            //else
            if( _open_mode & in )   _has_result_set = true;
        }
        else
        {
            if( _open_mode & in )
            {
                describe_columns();
                _any_file_ptr->_spec._field_type_ptr = +_type;
            }
        }
    }
}

//--------------------------------------------------------------------------Jdbc_file::prepare_stmt

void Jdbc_file::prepare_stmt( const Sos_string& )
{
}

//---------------------------------------------------------------------Jdbc_file::describe_columns

void Jdbc_file::describe_columns()
{
    Local_frame   java_frame ( 10 );
    Local_jobject meta_data;

/* MySQL und neuerdings (Dezember 2003) auch Oracle liefern MetaData erst nach execute().

    if( !session()->_connection_get_meta_data_not_implemented )
    {
        try
        {
            meta_data = _jdbc_statement.call_object_method( "getMetaData", "()Ljava/sql/ResultSetMetaData;" );
            if( !meta_data )  return;  // Keine Ergebnismenge

            _column_count = meta_data.call_int_method( "getColumnCount", "()I" );
        }
        catch( const Java_exception& x )
        {
            if( string_begins_with( x.message(), "ORA-01003" ) 
             || x._exception_name == "com.mysql.jdbc.NotImplemented" )
            {
                LOG( "DER JDBC-TREIBER LIEFERT DIE METADATEN ERST NACH executeQuery()\n" );
                session()->_connection_get_meta_data_not_implemented = true;
                meta_data = NULL;  // Falls getColumnCount() Fehler geliefert hat, ist meta_data ungültig! (so bei Oracle thin driver ojdbc14.jar)
            }
            else
                throw;           
        }
    }
*/

    if( !meta_data )
    {
        // Manche JDBC-Treiber (MySQL, Oracle Thin) liefert die Metadaten erst nach executeQuery().

        if( !_jdbc_result_set )  _jdbc_result_set = _jdbc_statement.call_object_method( "executeQuery", "()Ljava/sql/ResultSet;" );
        meta_data = _jdbc_result_set.call_object_method( "getMetaData", "()Ljava/sql/ResultSetMetaData;" );
        if( !meta_data )  return;   // Keine Ergebnismenge

        _column_count = meta_data.call_int_method( "getColumnCount", "()I" );
    }

    if( _debug )  Z_LOG2( "jdbc", "column_count=" << _column_count << "\n" );
    if( _column_count == 0 )  return;   // Keine Ergebnismenge

    _has_result_set = true;
    _jdbc_columns.clear();
    _jdbc_columns.reserve( _column_count );
    _jdbc_column_types.clear();
    _jdbc_column_types.reserve( _column_count );
    _type = Record_type::create();

    std::set<string> ignore = set_split( " *, *", lcase(_ignore_fields) );

    for( int i = 1; i <= _column_count; i++ )
    {
      //Sos_odbc_binding* b = &_result_bindings[ i ];

        string      name            =            meta_data.call_string_method( "getColumnName"       , "(I)Ljava/lang/String;" , i );
        Jdbc_type   type            = (Jdbc_type)meta_data.call_int_method   ( "getColumnType"       , "(I)I"                  , i );
        int         display_size    =            meta_data.call_int_method   ( "getColumnDisplaySize", "(I)I"                  , i );
      //string      label           =            meta_data.call_string_method( "getColumnLabel"      , "(I)Ljava/lang/String;" , i );
        int         scale           =            meta_data.call_int_method   ( "getScale"            , "(I)I"                  , i );
      //bool        is_currency     =            meta_data.call_bool_method  ( "isCurrency"          , "(I)Z"                  , i );
        bool        is_nullable     =            meta_data.call_int_method   ( "isNullable"          , "(I)I"                  , i ) == 1;  //columnNoNulls
        bool        is_signed       =            meta_data.call_bool_method  ( "isSigned"            , "(I)Z"                  , i );
        int         precision       = -999;
        
        if( !session()->_lob_get_precision_not_implemented
         || type != jdbc_type_clob && type != jdbc_type_blob )
        {
            try
            {
                precision = meta_data.call_int_method( "getPrecision", "(I)I", i );
            }
            catch( const Java_exception& x )
            {
                // java.lang.NumberFormatException: 4294967295 von Oracles Thin-Driver
                Z_LOG2( "jdbc", "jdbc: Für CLOB und BLOB getPrecision(" << i << ") wird durch getColumnDisplaySize() ersetzt wegen des Fehlers " << x.what() << "\n" );
                session()->_lob_get_precision_not_implemented = true;
            }
        }

        if( precision == -999 )  
        {
            if( session()->_dbms == dbms_oracle_thin )  display_size = _max_length;     // Oracle würfelt die display_size von Clobs. Bei z.B. sag.sos kommt 86
            precision = display_size;
        }

        if( display_size <= 0  &&  precision <= 0 )  
            display_size = 1024;    // Wenigstens PostgresQL liefert bei String-Funktion keine vernünftige Länge.  2009-02-27

        if( precision == 0  &&  display_size > 0 )  precision = display_size;


        if( ignore.find( lcase(name) ) == ignore.end() )
        {
            Sos_ptr<Field_descr> field = SOS_NEW( Field_descr );
            
            field->name( name );

            switch( type )
            {
              //case jdbc_type_array:
                case jdbc_type_bigint:          field->set_type( is_signed? (Field_type*)&int64_type : &uint64_type );      break;
              //case jdbc_type_binary:
                case jdbc_type_bit:             field->set_type( &bool_type );                                              break;
              //case jdbc_type_blob:
                case jdbc_type_boolean:         field->set_type( &bool_type );                                              break;
              //case jdbc_type_char:            field->set_type( SOS_NEW( Text_type( display_size ) ) );                    break;
                case jdbc_type_char:            field->set_type( SOS_NEW( Text_type( precision ) ) );                       break;
              //case jdbc_type_clob:
              //case jdbc_type_datalink:
                case jdbc_type_date:            if( session()->_dbms == dbms_oracle_thin )
                                                    field->set_type( &sos_optional_date_time_type );                                 
                                                 else
                                                    field->set_type( &sos_optional_date_type );                                 
                                                break;
              //case jdbc_type_decimal:
              //case jdbc_type_distinct:
                case jdbc_type_double:          field->set_type( &double_type );                                            break;
                case jdbc_type_float:           field->set_type( &double_type );                                            break;
                case jdbc_type_integer:         field->set_type( is_signed? (Field_type*)&int32_type : &uint32_type );      break;
              //case jdbc_type_java_object:
              //case jdbc_type_longvarbinary:
              //case jdbc_type_longvarchar:
              //case jdbc_type_null:
              //case jdbc_type_numeric:
              //case jdbc_type_other:
              //case jdbc_type_real:
              //case jdbc_type_ref:
                case jdbc_type_smallint:        field->set_type( is_signed? (Field_type*)&int16_type : &uint16_type );      break;
              //case jdbc_type_struct:
              //case jdbc_type_time:            field->set_type( &sos_optional_date_time_type );                            break;
                case jdbc_type_timestamp:       field->set_type( &sos_optional_date_time_type );                            break;
                case jdbc_type_tinyint:         field->set_type( is_signed? (Field_type*)&int1_type : &uint1_type );        break;
              //case jdbc_type_varbinary:
              //case jdbc_type_varchar:         field->
                default:                        field->set_type( SOS_NEW( String0_type( min( display_size, _max_length ) ) ) );  break;
            }

            field->add_to( _type );
            if( is_nullable  &&  !field->type().nullable() )  field->add_null_flag_to( _type );

            _jdbc_columns.push_back( i );
            _jdbc_column_types.push_back( type );

            if( _debug )  *log_ptr << "  describe_columns:"
                                      " field="         << name 
                                   << " type="          << name_of_jdbc_type(type)
                                   << " display_size="  << display_size 
                                   << " precision= "    << precision 
                                   << " scale="         << scale 
                                   << ", " << *field << endl;
        }
    }
}

// ---------------------------------------------------------------------------------Jdbc_file::open

void Jdbc_file::open( const char*, Open_mode, const File_spec& )
{
    Local_frame java_frame ( 10 );

    _row_count = 0;

    if( _jdbc_statement ) 
    {
        execute_stmt();

        //_any_file_ptr->_record_count = _row_count;


        if( _field_as_file )
        {
            if( _open_mode & in )
            {
                bool ok = _jdbc_result_set.call_bool_method( "next", "()Z" );
                if( !ok )  throw_not_exist_error( "SOS-1251" );

                _jdbc_input_stream = _jdbc_result_set.call_object_method( _is_clob? "getAsciiStream" : "getBinaryStream", "(I)Ljava/io/InputStream;", 1 );
            } 

            if( ( _open_mode & out )  &&  _oracle_lob )
            {
                //lob = result_set.getBLOB( 1 );      oder getCLOB( 1 );
                //_log_stream = lob.getBinaryOutputStream( 1 );
                //_oracle_lob_block_size = lob.getChunkSize();
            }
        }
    }
}

//--------------------------------------------------------------------------Jdbc_file::execute_stmt

void Jdbc_file::execute_stmt()
{
    Local_frame local_frame ( 10 );

    if( _jdbc_statement  &&  !( _field_as_file && _open_mode & out ) )
    {
        if( _has_result_set )
        {
            if( !_jdbc_result_set )
            {
                _jdbc_result_set = _jdbc_statement.call_object_method( "executeQuery", "()Ljava/sql/ResultSet;" );
            }
        }
        else
        {
            _row_count = _jdbc_statement.call_int_method( "executeUpdate", "()I" );
        
            if(_assert_row_count != -1  &&  _row_count != _assert_row_count )  throw_xc( "SOS-1446", _row_count, _assert_row_count );
        }
    }
}

//---------------------------------------------------------------------------------Jdbc_file::close

void Jdbc_file::close( Close_mode close_mode )
{
    Local_frame local_frame ( 10 );

    if( _field_as_file  &&  ( _open_mode & out ) )
    {
        /*
        java.io.File file = new java.io.File("/tmp/data");
        int fileLength = file.length();
        java.io.InputStream fin = new java.io.FileInputStream(file);
        java.sql.PreparedStatement pstmt = con.prepareStatement("UPDATE Table5 SET stuff = ? WHERE index = 4");
        pstmt.setBinaryStream (1, fin, fileLength);
        pstmt.executeUpdate();
        */


        Env env;

        _lob_file.flush();

        int64 llength = _lob_file.length();
        if( llength > INT_MAX )  z::throw_xc( __FUNCTION__, "_lob_file.length() > INT_MAX" );
        int length = (int)llength;

        if( _oracle_lob )
        {
            _lob_file.close();

            _row_count = 0;
            _jdbc_result_set = _jdbc_statement.call_object_method( "executeQuery", "()Ljava/sql/ResultSet;" );
            
            bool ok = _jdbc_result_set.call_bool_method( "next", "()Z" );
            if( ok )
            {
                _row_count++;

                zschimmer::file::File file ( _lob_file.path(), "rb" );
                Local_jobject lob;
                Local_jobject output_stream;

                if( _is_clob )
                {
                    lob = _jdbc_result_set.call_object_method( "getCLOB", "(I)Loracle/sql/CLOB;", 1 );
                    if( lob == NULL )  throw_xc( "getCLOB()=NULL" );

                    output_stream = lob.call_object_method( "getCharacterOutputStream", "()Ljava/io/Writer;" );
                    if( output_stream == NULL )  throw_xc( "getCLOB=NULL" );

                    int block_size = lob.call_int_method( "getChunkSize", "()I" );      // getBufferSize()?
                    if( block_size <= 0 )  throw_xc( "jdbc", "getChunkSize() <= 0" );

                    local_jobject<jcharArray> jchar_array = env->NewCharArray( block_size );
                    Dynamic_area buffer ( block_size );

                    while( 1 )
                    {
                        int len = file.read( buffer.ptr(), block_size );
                        if( len == 0 )  break;

                        jchar* char_array = env->GetCharArrayElements( jchar_array, NULL );
                        for( int i = 0; i < len; i++ )  char_array[i] = (uint)buffer.char_ptr()[ i ];
                        env->ReleaseCharArrayElements( jchar_array, char_array, 0 );

                        if( _debug )  Z_LOG2( "jdbc", "jdbc write(" << len << " bytes)\n" );
                        output_stream.call_void_method( "write", "([CII)V", (jobject)jchar_array, (jint)0, (jint)len );
                    }
                }
                else
                {
                    lob = _jdbc_result_set.call_object_method( "getBLOB", "(I)Loracle/sql/BLOB;", 1 );
                    if( lob == NULL )  throw_xc( "getBLOB()=NULL" );

                    output_stream = lob.call_object_method( "getBinaryOutputStream", "()Ljava/io/OutputStream;" );
                    if( output_stream == NULL )  throw_xc( "getBLOB=NULL" );

                    int block_size = lob.call_int_method( "getChunkSize", "()I" );      // getBufferSize()?
                    if( block_size <= 0 )  throw_xc( "jdbc", "getChunkSize() <= 0" );

                    local_jobject<jbyteArray> jbyte_array = env->NewByteArray( block_size );

                    while( 1 )
                    {
                        jbyte* byte_array = env->GetByteArrayElements( jbyte_array, NULL );
                        int len = file.read( byte_array, block_size );
                        env->ReleaseByteArrayElements( jbyte_array, byte_array, 0 );
                        if( len == 0 )  break;

                        if( _debug )  Z_LOG2( "jdbc", "jdbc write(" << len << " bytes)\n" );
                        output_stream.call_void_method( "write", "([BII)V", (jobject)jbyte_array, (jint)0, (jint)len );
                    }
                }

                file.close();

                output_stream.call_void_method( "close", "()V" );

                //ok = _jdbc_result_set.call_bool_method( "next", "()Z" );
                //if( ok )  _row_count++;

                _jdbc_result_set.call_void_method( "close", "()V" );
            }
           
            if( _assert_row_count != -1  &&  _row_count != _assert_row_count )  throw_xc( "SOS-1446", _row_count, _assert_row_count );
        }
        else
        {
            Local_jobject file_input_stream;

            if( length > 0 )
            {
                _lob_file.close();

                file_input_stream = Class( "java/io/FileInputStream" )
                                    .new_object( "(Ljava/lang/String;)V", (jstring)Local_jstring( _lob_file.path() ) );
                file_input_stream.call_close_at_end();

                _jdbc_statement.call_void_method( _is_clob? "setAsciiStream" : "setBinaryStream", "(ILjava/io/InputStream;I)V", (jint)1, 
                                                  (jobject)file_input_stream , (jint)length );
            }
            else
                _lob_file.close();

            _row_count = _jdbc_statement.call_int_method( "executeUpdate", "()I" );
            if( _assert_row_count != -1  &&  _row_count != _assert_row_count )  throw_xc( "SOS-1446", _row_count, _assert_row_count );
        }
    }


    // Für JdbcOdbc erst das Statement nach dem ResultSet schließen, sonst liefert ResultSet.close() einen Fehler. 

    if( _jdbc_result_set )  
    {
        if( _debug )  Z_LOG2( "jdbc", "java.sql.ResultSet.close()\n" );
        _jdbc_result_set.call_void_method( "close", "()V" );
        _jdbc_result_set = NULL;
    }

    if( _jdbc_statement )  
    {
        if( _debug )  Z_LOG2( "jdbc", "java.sql.Statement.close()\n" );
        _jdbc_statement.call_void_method( "close", "()V" );
        _jdbc_statement = NULL;
    }

    _jdbc_input_stream = NULL;

    _jdbc_column_types.clear();
    _type = NULL;

    if( _commit_at_close  &&  close_mode == close_normal )  session()->commit();

    session_disconnect();
}

//-----------------------------------------------------------------------Odbc_file::bind_parameters

void Jdbc_file::bind_parameters( const Record_type* param_type, const Byte* param_base )
{
    Local_frame local_frame ( 10 );

    _jdbc_statement.call_void_method( "clearParameters", "()V" );

    if( param_type ) 
    {
        for( int i = 0; i < param_type->field_count(); i++ )
        {
            Field_descr* f = param_type->field_descr_ptr( i );
            if( f ) 
            {
                Field_type*  t = f->type_ptr();
                if( t )
                {
                    switch( t->info()->_std_type )
                    {
                        case std_type_integer:
                        case std_type_bool:
                        {
                            _jdbc_statement.call_void_method( "setLong", "(IJ)V",
                                                              1 + i,
                                                              (jlong)f->as_big_int( param_base ) );
                            break;
                        }
                        
                        case std_type_float:
                        {
                            _jdbc_statement.call_void_method( "setDouble", "(ID)V",
                                                              1 + i,
                                                              (jlong)f->as_double( param_base ) );
                            break;
                        }

                        case std_type_char:
                        case std_type_varchar:
                        case std_type_decimal:
                        case std_type_date:
                        case std_type_date_time:
                        case std_type_time:
                        default:
                        {
                            _jdbc_statement.call_void_method( "setString", "(ILjava/lang/String;)V",
                                                              1 + i,
                                                              (jstring)Local_jstring( f->as_string( param_base ) ) );
                            break;
                        }
                    }                        
                }
            }
        }
    }
}

//----------------------------------------------------------------------------Jdbc_file::put_record

void Jdbc_file::put_record( const Const_area& record )
{
    Local_frame local_frame ( 10 );

    if( _field_as_file )
    {
        //if( _oracle_lob )
        //{
        //}
        //else
        {
            _lob_file.write( record.ptr(), record.length() );
        }
    }
    else
    {
        _session->execute_direct( record );
        _any_file_ptr->_record_count = _session->_row_count;
    }
}

//----------------------------------------------------------------------------Jdbc_file::get_record

void Jdbc_file::get_record( Area& buffer )
{
    if( _field_as_file )
    {
        get_lob_record( buffer );
        return;
    }


    Local_frame local_frame ( 2 * _type->field_count() );


    bool ok = _jdbc_result_set.call_bool_method( "next", "()Z" );
    if( !ok )  throw_eof_error();

    buffer.allocate_min( _type->field_size() );
    buffer.length( _type->field_size() );
    _type->construct( buffer.byte_ptr() );
    //memset( buffer.ptr(), 0, _type->field_size() );


    for( int i = 0; i < _type->field_count(); i++ )
    {
        Jdbc_type column_type = _jdbc_column_types[ i ];
        switch( column_type )
        {
/*
          //case jdbc_type_array:
            case jdbc_type_bigint:          _jdbc_result_set->call_bigint_method( "getInt", "(I)I", i+1 );  break;
          //case jdbc_type_binary:
            case jdbc_type_bit:             field->set_type( &bool_type );                              break;
          //case jdbc_type_blob:
            case jdbc_type_boolean:         field->set_type( &bool_type );                              break;

            case jdbc_type_char:            field->set_type( SOS_NEW( Text_type( precision ) ) );       break;
          //case jdbc_type_clob:
          //case jdbc_type_datalink:
            case jdbc_type_date:            field->set_type( &sos_optional_date_time_type );            break;
          //case jdbc_type_decimal:
          //case jdbc_type_distinct:
            case jdbc_type_double:          field->set_type( &double_type );                            break;
            case jdbc_type_float:           field->set_type( &double_type );                            break;
            case jdbc_type_integer:         field->set_type( is_signed? (Field_type*)&int32_type : &uint32_type );   break;
          //case jdbc_type_java_object:
          //case jdbc_type_longvarbinary:
          //case jdbc_type_longvarchar:
          //case jdbc_type_null:
          //case jdbc_type_numeric:
          //case jdbc_type_other:
          //case jdbc_type_real:
          //case jdbc_type_ref:
            case jdbc_type_smallint:        field->set_type( is_signed? (Field_type*)&int16_type : &uint16_type );   break;
          //case jdbc_type_struct:
            case jdbc_type_time:            field->set_type( &sos_optional_date_time_type );            break;
            case jdbc_type_timestamp:       field->set_type( &sos_optional_date_time_type );            break;
            case jdbc_type_tinyint:         field->set_type( is_signed? (Field_type*)&int1_type : &uint1_type );     break;
          //case jdbc_type_varbinary:
          //case jdbc_type_varchar:         field->
*/
            case jdbc_type_blob:
            case jdbc_type_clob:
            {
                try
                {
                    Env            env;
                    Field_descr*   f      = _type->field_descr_ptr(i);
                    Local_jobject  stream;
                    
                    stream = _jdbc_result_set.call_object_method( column_type == jdbc_type_clob? "getAsciiStream" : "getBinaryStream", "(I)Ljava/io/InputStream;", _jdbc_columns[ i ] );
                    if( stream )
                    {
                        int    size = f->type_ptr()->field_size() + 100;    // Ein paar mehr, um Überlauf zu bemerken
                        string value;

                        while( value.length() < size )
                        {
                            local_jobject<jbyteArray> jbyte_array = env->NewByteArray( size - value.length() );

                            int length = stream.call_int_method( "read", "([B)I", (jbyteArray)jbyte_array );
                            if( length == -1 )  break;

                            jbyte* byte_array = env->GetByteArrayElements( jbyte_array, NULL );
                            value.append( (const char*)byte_array, length );
                            env->ReleaseByteArrayElements( jbyte_array, byte_array, JNI_ABORT );
                        }

                        f->set_string( &buffer, value );

                        if( _debug )  Z_LOG2( "jdbc", "java.io.InputStream.close()\n" );
                        stream.call_void_method( "close", "()V" );
                    }
                }
                catch( Xc& x ) { x.insert( string("Beim Lesen des BLOB/CLOB ") + _type->field_descr_ptr(i)->name() );  throw x; }

                break;
            }

            default: 
            {
                string value = _jdbc_result_set.call_string_method( "getString", "(I)Ljava/lang/String;", _jdbc_columns[ i ] );
              //if( _debug )  LOG( "jdbc " << _type->field_descr(i).name() << " := \"" << value << "\"\n" );
                Field_descr* f = _type->field_descr_ptr( i );
                bool was_null = false;

                if( f->has_null_flag()  ||  f->type().nullable() )
                    was_null = _jdbc_result_set.call_bool_method( "wasNull", "()Z" );

                if( was_null )  f->set_null( buffer.byte_ptr() );
                          else  f->set_string( &buffer, value );
            }
        }
    }
}

//------------------------------------------------------------------------Jdbc_file::get_lob_record

void Jdbc_file::get_lob_record( Area& buffer )
{
    Local_frame local_frame ( 10 );

    int size = buffer.size();
    if( buffer.resizable()  &&  _fixed_length > buffer.size() )  size = _fixed_length;
    if( size == 0 )  throw_xc( "SOS-1334" );

    Env env;

    local_jobject<jbyteArray> jbyte_array ( env->NewByteArray( size ) );

    if( !_jdbc_input_stream )  throw_eof_error();

    int length = _jdbc_input_stream.call_int_method( "read", "([B)I", (jbyteArray)jbyte_array );
    if( length == -1 )  throw_eof_error();

    jbyte* byte_array = env->GetByteArrayElements( jbyte_array, NULL );
    buffer.assign( byte_array, length );
    env->ReleaseByteArrayElements( jbyte_array, byte_array, JNI_ABORT );
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
