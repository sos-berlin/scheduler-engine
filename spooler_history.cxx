// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/z_sql.h"
#include "../kram/sleep.h"
#include "../kram/sos_java.h"
#include "../file/sosdb.h"

#ifdef Z_WINDOWS
#   include <process.h>     // getpid()
#endif

using namespace zschimmer;

namespace sos {
namespace scheduler {
namespace database {

//-------------------------------------------------------------------------------------------------

const char history_column_names[] =    "id"           ":numeric," 
                                       "spooler_id,"
                                       "job_name,"
                                       "start_time"   ":Datetime,"
                                       "end_time"     ":Datetime,"
                                       "cause,"
                                       "steps,"
                                       "error,"
                                       "error_code,"
                                       "error_text,"
                                       "parameters,"
                                       "cluster_member_id,"
                                       "exit_code,"
                                       "pid";

const char history_column_names_db[] = "log";    // Spalten zusätzlich in der Daten..bank

const int max_field_length = 1024;      // Das ist die Feldgröße von Any_file -type=(...) für tabulierte Datei.
const int blob_field_size  = 1900;      // Bis zu dieser Größe wird ein Blob im Datensatz geschrieben. ODBC erlaubt nur 2000 Zeichen lange Strings
const int db_error_retry_max = 0;       // Nach DB-Fehler max. so oft die Datenbank neu eröffnen und Operation wiederholen.
const int max_column_length = 249;      // Für MySQL 249 statt 250. jz 7.1.04

//-------------------------------------------------------------------------------------------------

const int Database::seconds_before_reopen   = Z_NDEBUG_DEBUG( 60, 20 );     // Solange warten, bis Datenbank nach Fehler erneut geöffnet wird. 
                                                                            // Zeiten wie Cluster::heart_beat_period
//const int Database::lock_timeout            = 30;   // Wartezeit für gesperrte Datenbanksätze, nicht von jeder Datenbank begrenzbar

//---------------------------------------------------------------------------------------------test
/*
template< class B >
string operator << ( const string& a, const B& b )
{
    ostrstream s;
    s << a << b;
    return s.str();
}

template< class B >
string operator << ( const char* a, const B& b )
{
    ostrstream s;
    s << a << b;
    return s.str();
}
*/
//------------------------------------------------------------------------------------------uquoted
/*
static string uquoted( const string& value ) 
{ 
    return quoted_string( ucase( value ), '\"', '\"' ); 
}
*/
//---------------------------------------------------------------------------------------sql_quoted

inline string sql_quoted( const string& value ) 
{ 
    return quoted_string( value, '\'', '\'' ); 
}

//----------------------------------------------------------------------------------sql_quoted_name

inline string sql_quoted_name( const string& value ) 
{ 
    return quoted_string( value, '"', '"' ); 
}

//--------------------------------------------------------------------------quote_and_prepend_comma

static string quote_and_prepend_comma( const string& s )  
{ 
    return ", `" + s + "`"; 
}

//---------------------------------------------------------------Read_transaction::Read_transaction

Read_transaction::Read_transaction( Database* db )
: 
    _zero_(this+1),
    _guard(&db->_lock),
    _spooler(db->_spooler),
    _log(db->_log)
{
    assert( db );
    //Das müssen wir später prüfen  if( !_db->opened() )  z::throw_xc( "SCHEDULER-361" );

    begin_transaction( db );
}

//--------------------------------------------------------------Read_transaction::~Read_transaction
    
Read_transaction::~Read_transaction()
{ 
}

//------------------------------------------------------------Read_transaction::assert_is_read_only

void Read_transaction::assert_is_commitable( const string& debug_text ) const
{
    if( is_read_only() )  z::throw_xc( "READ_ONLY_TRANSACTION", debug_text );
}

//--------------------------------------------------------------Read_transaction::begin_transaction
    
void Read_transaction::begin_transaction( Database* db )
{ 
    if( !_guard )  _guard.enter( &db->_lock, Z_FUNCTION );

    _db = db; 
}

//-------------------------------------------------------------Read_transaction::read_single_record

Record Read_transaction::read_single_record( const string& sql, const string& debug_text, bool need_commit_or_rollback )
{
    Any_file result_set = open_result_set( sql, debug_text, need_commit_or_rollback );
    
    Record result = result_set.get_record();
    if( !result_set.eof() )  z::throw_xc( Z_FUNCTION );

    return result;
}

//----------------------------------------------------------------Read_transaction::open_result_set

Any_file Read_transaction::open_result_set( const string& sql, const string& debug_text, bool need_commit_or_rollback )
{ 
    if( need_commit_or_rollback )  assert_is_commitable( debug_text );

    string native_sql = db()->_db.sos_database_session()->translate_sql( sql );

    S s;
    if( !need_commit_or_rollback )  s << "-read-transaction ";
    s << "%native " << native_sql;

    return open_file_2( "-in " + db()->db_name(), s, debug_text, need_commit_or_rollback, native_sql );
}

//----------------------------------------------------------------------Read_transaction::open_file

Any_file Read_transaction::open_file( const string& db_prefix, const string& sql, const string& debug_text, bool need_commit_or_rollback )
{
    if( need_commit_or_rollback )  assert_is_commitable( debug_text );

    return open_file_2( db_prefix, sql, debug_text, need_commit_or_rollback, sql );
}

//--------------------------------------------------------------------Read_transaction::open_file_2

Any_file Read_transaction::open_file_2( const string& db_prefix, const string& execution_sql, const string& debug_text, bool need_commit_or_rollback, const string& logging_sql )
{
    if( need_commit_or_rollback )  assert_is_commitable( debug_text );
    //if( need_commit_or_rollback ) set_transaction_written(); else set_transaction_read();

    string debug_extra;
    if( debug_text != "" )  debug_extra = "  (" + debug_text + ")";

    Any_file result;

    try
    {
        result.open( db_prefix + " " + execution_sql );

        if( _db->_log->is_enabled_log_level( _spooler->_db_log_level ) )     // Hostware protokolliert schon ins scheduler.log
        {
            S line;
            line << logging_sql << debug_extra;
            if( _db->_db.record_count() >= 0 )  line << "  ==> " << _db->_db.record_count() << " records";
            _db->_log->log( _spooler->_db_log_level, line );
        }
    }
    catch( exception& x )
    {
        if( _log_sql )  // Vielleicht für alle Anweisungen, aber dann haben wir einen Fehler im Protokoll, das ist nicht 100%ig kompatibel
        {
            _db->_log->warn( logging_sql + debug_extra );
            _db->_log->error( x.what() );
        }
        else
            _db->_log->debug( logging_sql + debug_extra ), _db->_log->debug( x.what() );

        throw;
    }

    return result;
}

//----------------------------------------------------------------------Read_transaction::read_clob

string Read_transaction::read_clob( const string& table_name, const string& column_name, const string& key_name, const string& key_value )
{
    return read_clob( table_name, column_name, "  where `" + key_name + "`=" + sql::quoted( key_value ) );
}

//----------------------------------------------------------------------Read_transaction::read_clob

string Read_transaction::read_clob_or_empty( const string& table_name, const string& column_name, const string& where )
{
    try
    {
        return this->file_as_string( " -read-transaction -table=" + table_name + " -clob=" + column_name + "  " + where );
    }
    catch( sos::Not_exist_error& x )
    {
        if( string(x.code()) == "SOS-1251" )  return "";    // "Gesuchter Satz nicht vorhanden, SELECT liefert leere Ergebnismenge"
        throw;
    }
}

//----------------------------------------------------------------------Read_transaction::read_clob

string Read_transaction::read_clob( const string& table_name, const string& column_name, const string& where )
{
    return this->file_as_string( " -read-transaction -table=" + table_name + " -clob=" + column_name + "  " + where );
}

//-----------------------------------------------------------------Read_transaction::file_as_string

string Read_transaction::file_as_string( const string& hostware_filename )
{
    if( _db->db_name() == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    //set_transaction_read();
    try
    {
        return ::sos::file_as_string( "-binary " + _db->db_name() + hostware_filename, "" );
    }
    catch( exception& x )
    {
        _log->log( _spooler->_db_log_level, S() << x.what() << ", when reading blob: " << hostware_filename );
        throw;
    }
}

//-------------------------------------------------------------------------Transaction::Transaction

Transaction::Transaction( Database* db )
: 
    Read_transaction( db ),
    _zero_(this+1)
{
    assert( db );
    //Das müssen wir später prüfen  if( !_db->opened() )  z::throw_xc( "SCHEDULER-361" );

    begin_transaction( db );
}

//-------------------------------------------------------------------------Transaction::Transaction

Transaction::Transaction( Database* db, Transaction* outer_transaction )
: 
    Read_transaction( db ),
    _zero_(this+1),
    _outer_transaction(outer_transaction)
{
    assert( db );
    //Das müssen wir später prüfen  if( !_db->opened() )  z::throw_xc( "SCHEDULER-361" );

    if( _outer_transaction )
    {
        _log_sql = _outer_transaction->_log_sql;
    }

    begin_transaction( db );
}

//------------------------------------------------------------------------Transaction::~Transaction

Transaction::~Transaction()
{ 
    if( _db  &&  !_outer_transaction )  
    {
        try 
        { 
            rollback( Z_FUNCTION );
        } 
        catch( exception& x ) { Z_LOG( "*** ERROR " << Z_FUNCTION << " " << x.what() << "\n" ); }
    }
}

//-------------------------------------------------------------------Transaction::begin_transaction

void Transaction::begin_transaction( Database* db )
{ 
    if( !_outer_transaction  &&  db->_transaction )
    {
        Z_DEBUG_ONLY( Z_WINDOWS_ONLY( assert( "NESTED TRANSACTION"==NULL ) ) );
        z::throw_xc( "NESTED_TRANSACTION", Z_FUNCTION );
    }

    Read_transaction::begin_transaction( db );

    db->_transaction = this;
}

//------------------------------------------------------------------------------Transaction::commit

void Transaction::commit( const string& debug_text )
{ 
    assert( _db );

    if( _outer_transaction ) 
    {
        Z_LOG2( "joacim", Z_FUNCTION << "  Commit delayed because of _outer_transaction.  " << debug_text << "\n" );
        //_outer_transaction->_transaction_written |= _transaction_written;
        //_outer_transaction->_transaction_read    |= _transaction_read;
    }
    else
    {
        if( db()->opened() )  
        {
            if( !_suppress_heart_beat_timeout_check  &&  !_spooler->assert_is_still_active( Z_FUNCTION, debug_text, this ) )
            {
                _spooler->abort_immediately_after_distribution_error( Z_FUNCTION );     // Wir wollen das verspätete Commit verhindern
                //_spooler->throw_distribution_error( "commit" );       // Das führt nach Datenbank-Reopen doch noch zu einem Commit (weil assert_is_still_active wieder true liefert?) 
                                                                        // Außerdem geht dann kein Commit mehr, auch nicht Order::db_release_occupation().
            }

            execute( "COMMIT", debug_text );
        }
    }

    _db->_transaction = _outer_transaction;
    _db = NULL;
    _guard.leave(); 


    if( !_suppress_heart_beat_timeout_check  &&  !_outer_transaction )  _spooler->assert_is_still_active( Z_FUNCTION, debug_text );
}

//-----------------------------------------------------------------Transaction::intermediate_commit

void Transaction::intermediate_commit( const string& debug_text )
{ 
    assert( _db );

    //if( _outer_transaction ) 
    //{
    //    _outer_transaction->_transaction_written |= _transaction_written;
    //    _outer_transaction->_transaction_read    |= _transaction_read;
    //}
    
    if( db()->opened() )  execute( "COMMIT", debug_text, ex_force );
}

//----------------------------------------------------------------------------Transaction::rollback

void Transaction::rollback( const string& debug_text, Execute_flags flags )
{ 
    if( _db )
    {
        if( _outer_transaction )  
        {
            //_outer_transaction->_transaction_written |= _transaction_written;
            //_outer_transaction->_transaction_read    |= _transaction_read;

            Z_DEBUG_ONLY( Z_WINDOWS_ONLY( DebugBreak() ) );
            Z_LOG( "Rollback in inner transaction." << debug_text );        // Keine Exception, weil wir schon in einer Exception sein können.
        }

        if( db()->opened() )  
        {
            try
            {
                execute( "ROLLBACK", debug_text, flags );
            }
            catch( exception& )
            {
                _db->_transaction = _outer_transaction;
                _db = NULL; 
                _guard.leave(); 
                throw;
            }
        }

        _db->_transaction = _outer_transaction;
        _db = NULL; 
        _guard.leave(); 

        //Wir sind vielleicht in einer Exception, also keine weitere Exception auslösen:  if( !db->_transaction )  _spooler->assert_is_still_active( Z_FUNCTION, debug_text );
    }
}

//--------------------------------------------Retry_nested_transaction::reopen_database_after_error

void Retry_nested_transaction::reopen_database_after_error( const exception& x, const string& function )
{ 
    if( _outer_transaction )  throw;    // Wenn's eine äußere Transaktion gibt, dann die Schleife dort wiederholen

    assert( _db );

    Database* db = _db;

    try
    {
        rollback( S() << Z_FUNCTION << ", " << function, ex_force );
    }
    catch( exception& x )  { Z_LOG2( "scheduler", Z_FUNCTION << "  " << x.what() << "\n" ); }
    assert( !db->is_in_transaction() );

    _database_retry.reopen_database_after_error( x, function );

    begin_transaction( db );
}

//-------------------------------------------------------------Retry_nested_transaction::operator++

void Retry_nested_transaction::operator++(int)
{ 
    _database_retry++; 
    _database_retry.enter_loop();

    //if( _database_retry.enter_loop() )
    //{
    //    rollback( Z_FUNCTION );  
    //    begin_transaction( _database_retry._db );
    //}
}

//--------------------------------------------------Retry_nested_transaction::intermediate_rollback

void Retry_transaction::intermediate_rollback( const string& debug_text )
{ 
    assert( _db );
    
    if( db()->opened() )  execute( "ROLLBACK", debug_text, ex_force );
}

//------------------------------------------------------Database_retry::reopen_database_after_error

void Database_retry::reopen_database_after_error( const exception& x, const string& function )
{ 
    assert( _db );
    assert( !_db->is_in_transaction() );

    _db->try_reopen_after_error( x, function ); 

    if( _db->opened() )  repeat_loop(); 
}

//-------------------------------------------------------------------------------Database::Database

Database::Database( Spooler* spooler )
:
    Scheduler_object( spooler, spooler, Scheduler_object::type_database ),
    _zero_(this+1),
    _lock("Database"),
    _db_descr( z::sql::flag_uppercase_names | z::sql::flag_quote_names | z::sql::flag_dont_quote_table_names )
{
}

//----------------------------------------------------------------------------Database::transaction
    
Transaction* Database::transaction()
{
    if( !_transaction )
    {
        Z_DEBUG_ONLY( Z_WINDOWS_ONLY( assert( ("Database::transaction()",0) ) ) );
        z::throw_xc( "NO-TRANSACTION" );
    }

    return _transaction;
}

//-------------------------------------------------------------------Database::subsystem_initialize

//bool Database::subsystem_initialize()
//{
//    set_subsystem_state( subsys_initialized );
//}
//
////-------------------------------------------------------------------------Database::subsystem_load
//
//bool Database::subsystem_load()
//{
//    set_subsystem_state( subsys_loaded );
//}
//
////------------------------------------------------------------------------Database::subsystem_
//
//bool Database::subsystem_activate()
//{
//    set_subsystem_state( subsys_active );
//}

//-----------------------------------------------------------------------------------Database::open

void Database::open( const string& db_name )
{
    try
    {
        open2( db_name );
    }
    catch( exception& x )
    {
        if( !_spooler->_wait_endless_for_db_open )  throw;
        
        try_reopen_after_error( x, Z_FUNCTION, true );
    }

    if( opened() ) 
    {
        create_tables_when_needed();
        check_database();
    }
}

//----------------------------------------------------------------------------------Database::open2

void Database::open2( const string& db_name )
{
    Z_MUTEX( _lock )
    {
        string my_db_name = db_name;

        _db_name = db_name;
    
        if( _db_name != "" )
        {
            if( _db_name.find(' ') == string::npos  &&  _db_name.find( ':', 1 ) == string::npos )
            {
                if( !is_absolute_filename( _db_name  )  &&  (_spooler->log_directory() + " ")[0] == '*' ) 
                {
                    if( _spooler->_need_db )  z::throw_xc( "SCHEDULER-142", _db_name );
                    return;
                }

                  _db_name = "odbc "         + make_absolute_filename( _spooler->log_directory(),   _db_name );
                my_db_name = "odbc -create " + make_absolute_filename( _spooler->log_directory(), my_db_name );
            }

            if( _db_name.substr(0,5) == "odbc " 
             || _db_name.substr(0,5) == "jdbc ")   _db_name = _db_name.substr(0,5) + " -id=spooler " +   _db_name.substr(5),
                                                 my_db_name = _db_name.substr(0,5) + " -id=spooler " + my_db_name.substr(5);

            _log->info( message_string( "SCHEDULER-907", my_db_name ) );     // Datenbank wird geöffnet

            try
            {
                _db.open( "-in -out " + my_db_name );

                switch( _db.dbms_kind() )
                {
                    case dbms_sybase: 
                        _db_descr._use_simple_iso_datetime_string = true;       // 'yyyy-mm-dd' statt {ts'yyyy-mm-dd'}
                        break;

                    default: ;
                }

                _log->info( message_string( "SCHEDULER-807", _db.dbms_name() ) );     // Datenbank ist geöffnet

                _db_name += " ";
                _email_sent_after_db_error = false;
            }
            catch( exception& x )  
            { 
                close();

                if( _spooler->_need_db )  throw;
            
                _log->warn( message_string( "SCHEDULER-309", x ) );         // "FEHLER BEIM ÖFFNEN DER HISTORIENDATENBANK: "
            }
        }
    }
}

//----------------------------------------------------------------Database::run_create_table_script

//void Database::run_create_table_script()
//{
//    string filename;
//
//    switch( _db.dbms_kind() )
//    {
//        case dbms_access:       filename = "create_tables.access.sql";                break;
//        case dbms_db2:          filename = "create_tables.db2.sql";                   break;
//        case dbms_firebird:     filename = "create_tables.firebird.sql";              break;
//        case dbms_mysql:        filename = "create_tables.mysql.sql";                 break;
//        case dbms_oracle:
//        case dbms_oracle_thin:  filename = "create_tables.oracle.sql";                break;
//        case dbms_sql_server:   filename = "create_tables.microsoft_sql_server.sql";  break;
//        case dbms_postgresql:   filename = "create_tables.postgresql.sqk";            break;
//        default: ;
//    }
//
//    if( const Embedded_file* embedded_file = embedded_files.get_embedded_file_or_null( "database/" + filename ) )
//    {
//        try
//        {
//            Transaction ta ( this );
//            //string cmd = sql::without_comments_regardless_quotes( embedded_file->_content );
//            //for( int i = 0; i < cmd.length(); i++ )  if( cmd[i] == '\r'  ||  cmd[i] == '\n' )  cmd[i] = ' ';
//            //ta.execute( cmd, Z_FUNCTION, Transaction::ex_native );
//            ta.execute( embedded_file->_content , Z_FUNCTION, Transaction::ex_native );
//            ta.commit( Z_FUNCTION );
//        }
//        catch( exception& x )
//        {
//            _log->warn( message_string( "SCHEDULER-883", embedded_file->_filename, x ) );
//        }
//    }
//    else
//        _log->info( message_string( "SCHEDULER-882", _db.dbms_name() ) );
//}

//-------------------------------------------------------------------------Database::check_database

void Database::check_database()
{
    try
    {
        string temporary_name = "SCHEDULER.TEST.CHECK_DATABASE." + Time::now().as_string( Time::with_ms ) + "." + as_string( rand() );
        string check_value;
        
        check_value.reserve( 300 );
        check_value = "\t\r\n";                                  
        for( int c = ' ' ; c <= 0x7E; c++ )  check_value += (char)c;     // Alle ASCII-Zeichen
        //2007-11-26 Andreas Püschel will das nicht:  for( int c = 0xA0; c <= 0xFF; c++ )  check_value += (char)c;     // Alle Latin1-Zeichen
        check_value += '\\';    // Letztes Zeichen, um einen Syntaxfehler hervorzurufen, wenn die Datenbank \ im String interpretiert

        {
            Transaction ta ( this );

            ta.set_variable( temporary_name, check_value );
            string read_value = ta.get_variable_text( temporary_name );
            if( read_value != check_value )  z::throw_xc( "SCHEDULER-452", check_value, read_value );

            ta.rollback( __FUNCTION__ );
        }

        {
            Transaction ta ( this );

            bool record_exists = false;
            ta.get_variable_text( temporary_name, &record_exists );
            if( record_exists )  z::throw_xc( "SCHEDULER-453" );
        }
    }
    catch( exception& x )
    {
        z::throw_xc( "SCHEDULER-451", x );
    }
}

//--------------------------------------------------------------Database::create_tables_when_needed

void Database::create_tables_when_needed()
{
    string chararacter_set = _db.dbms_kind() == dbms_mysql? " character set latin1" : "";
    string null            = _db.dbms_kind() == dbms_sybase? " null" : "";

    ////////////////////////////

    {
        Transaction ta ( this );

        bool created = create_table_when_needed( &ta, _spooler->_variables_tablename, 
                         S() << "`name`"     " varchar(100)"     " not null,"
                                "`wert`"     " integer"             << null << ","  
                                "`textwert`" " varchar(250)"        << null << ","  
                                "primary key ( `name` )" );
        if( created )
        {
            // Hier Index anlegen
        }
        else
        {
            add_column( &ta, _spooler->_variables_tablename, "textwert", " add `textwert` varchar(250)" );
        }

        ta.commit( Z_FUNCTION );
    }

    ////////////////////////////

    {
        Transaction ta ( this );

        bool created = create_table_when_needed( &ta, _spooler->_tasks_tablename, 
                         S() << "`task_id`"          " integer"      " not null,"          // Primärschlüssel
                                "`spooler_id`"       " varchar(100)" " not null,"
                                "`cluster_member_id`"" varchar(100)"    << null << ","
                                "`job_name`"         " varchar(255)" " not null,"
                                "`enqueue_time`"     " datetime"        << null << ","
                                "`start_at_time`"    " datetime"        << null << ","
                                "`parameters`"       " clob"            << null << ","
                                "`task_xml`"         " clob"            << null << ","
                                "primary key( `task_id` )" );
        if( created )
        {
            // Hier Index anlegen
        }
        else
        {
            bool added;
            added = add_column( &ta, _spooler->_tasks_tablename, "cluster_member_id", "add `cluster_member_id` varchar(100)" );
            
            if( added )  
                    add_column( &ta, _spooler->_tasks_tablename, "task_xml"         , "add `task_xml` clob" );

        }

        ta.commit( Z_FUNCTION );
    }

    ////////////////////////////

    {
        Transaction ta ( this );

        vector<string> create_extra = vector_map( sql_quoted_name, vector_split( " *, *", _spooler->_job_history_columns ) );
        for( int i = 0; i < create_extra.size(); i++ )  create_extra[i] += " varchar(250),";

        bool created = create_table_when_needed( &ta, _spooler->_job_history_tablename, 
                         S() << "`id`"                " integer"       " not null,"
                                "`spooler_id`"        " varchar(100)"  " not null,"
                                "`cluster_member_id`" " varchar(100)"     << null << ","
                                "`job_name`"          " varchar(255)"  " not null,"
                                "`start_time`"        " datetime"      " not null,"
                                "`end_time`"          " datetime"         << null << ","
                                "`cause`"             " varchar(50)"      << null << ","
                                "`steps`"             " integer"          << null << ","
                                "`exit_code`"         " integer"          << null << ","
                                "`error`"             " boolean"          << null << ","
                                "`error_code`"        " varchar(50)"      << null << ","
                                "`error_text`"        " varchar(250)"     << null << ","
                                "`parameters`"        " clob"             << null << ","
                                "`log`"               " blob"             << null << ","
                                "`pid`"               " integer"          << null << ","
                                + join( "", create_extra ) 
                                + "primary key( `id` )" );

        // Nicht vergessen: Konstante history_column_names um neue Spaltennamen erweitern!


        if( created )
        {
            ta.create_index( _spooler->_job_history_tablename, "SCHEDULER_HISTORY_START_TIME" , "SCHEDULER_HIST_1", "`start_time`"       , Z_FUNCTION );
            ta.create_index( _spooler->_job_history_tablename, "SCHEDULER_HISTORY_SPOOLER_ID" , "SCHEDULER_HIST_2", "`spooler_id`"       , Z_FUNCTION );
            ta.create_index( _spooler->_job_history_tablename, "SCHEDULER_HISTORY_JOB_NAME"   , "SCHEDULER_HIST_3", "`job_name`"         , Z_FUNCTION );
            ta.create_index( _spooler->_job_history_tablename, "SCHEDULER_H_CLUSTER_MEMBER"   , "SCHEDULER_HIST_4", "`cluster_member_id`", Z_FUNCTION );
        }
        else
        {
            bool added;
            added = add_column( &ta, _spooler->_job_history_tablename, "pid", "add `pid` integer" );

            if( !added )
            {
                added = add_column( &ta, _spooler->_job_history_tablename, "cluster_member_id", "add `cluster_member_id` varchar(100)" );

                //Jira JS-???  Setup der SOS legt exit_code nicht an.   if( added )
                add_column( &ta, _spooler->_job_history_tablename, "EXIT_CODE"        , "add `EXIT_CODE`     integer" );
            }

        }

        ta.commit( Z_FUNCTION );
    }

    ////////////////////////////

    {
        Transaction ta ( this );

        bool created = create_table_when_needed( &ta, _spooler->_orders_tablename, S() <<
                                "`job_chain`"                   " varchar(250)" << chararacter_set << " not null,"                                    // Primärschlüssel
                                "`id`"                          " varchar(" << const_order_id_length_max << ")" << chararacter_set << " not null,"    // Primärschlüssel
                                "`spooler_id`"                  " varchar(100)" << chararacter_set << " not null,"                                    // Primärschlüssel
                                "`distributed_next_time`"       " datetime"        << null << ","     // Auftrag ist verteilt ausführbar
                                "`occupying_cluster_member_id`" " varchar(100)"    << null << ","     // Index
                                "`priority`"                    " integer"      " not null,"
                                "`state`"                       " varchar(100)"    << null << ","
                                "`state_text`"                  " varchar(100)"    << null << ","
                                "`title`"                       " varchar(200)"    << null << ","
                                "`created_time`"                " datetime"     " not null,"
                                "`mod_time`"                    " datetime"        << null << ","
                                "`ordering`"                    " integer"      " not null,"     // Um die Reihenfolge zu erhalten, sollte geordneter Index sein
                                "`payload`"                     " clob"            << null << ","
                                "`initial_state`"               " varchar(100)"    << null << ","               
                                "`run_time`"                    " clob"            << null << ","
                                "`order_xml`"                   " clob"            << null << ","
                                "primary key( `spooler_id`, `job_chain`, `id` )" );
        if( created )
        {
            // Hier Index anlegen
        }
        else
        {
            bool added;
            added = add_column( &ta, _spooler->_orders_tablename, "distributed_next_time"      , "add `distributed_next_time`"       " datetime"     );
            
            if( added )
            added = add_column( &ta, _spooler->_orders_tablename, "occupying_cluster_member_id", "add `occupying_cluster_member_id`" " varchar(100)" );

            if( added )
                    add_column( &ta, _spooler->_orders_tablename, "initial_state"              , "add `initial_state`"             " varchar(100)" );

            if( added )
                    add_column( &ta, _spooler->_orders_tablename, "run_time"                   , "add `run_time`"                  " clob"         );

            if( added )
                    add_column( &ta, _spooler->_orders_tablename, "order_xml"                  , "add `order_xml`"                 " clob"         );
        }

        ta.commit( Z_FUNCTION );
    }
    
    ////////////////////////////

    {
        bool   created;
        string primary_key        = "`history_id`";
        string column_definitions = S() <<
            "`history_id`"  " integer"      " not null,"             // Primärschlüssel
            "`job_chain`"   " varchar(250)" " not null,"
            "`order_id`"    " varchar(" << const_order_id_length_max << ")" " not null,"
            "`spooler_id`"  " varchar(100)" " not null,"
            "`title`"       " varchar(200)"    << null << ","
            "`state`"       " varchar(100)"    << null << ","
            "`state_text`"  " varchar(100)"    << null << ","
            "`start_time`"  " datetime"     " not null,"
            "`end_time`"    " datetime"        << null << ","
            "`log`"         " blob"            << null;
        

        {
            Transaction ta ( this );

            created = create_table_when_needed( &ta, _spooler->_order_history_tablename, 
                                                S() << column_definitions << ", primary key( " << primary_key << " )" );

            ta.commit( Z_FUNCTION );
        }

        if( !created )
        {
            try
            {
                Transaction ta ( this );                                                    // JS-150, Spalte end_time soll null aufnehmen können

                sql::Insert_stmt insert ( ta.database_descriptor() );

                insert.set_table_name( _spooler->_order_history_tablename );
                
                insert[ "history_id" ] = -1;
                insert[ "job_chain"  ] = "TEST";
                insert[ "order_id"   ] = "TEST";
                insert[ "state"      ] = "TEST";
                insert[ "spooler_id" ] = _spooler->id_for_db();
                insert.set_datetime( "start_time", Time::now().as_string(Time::without_ms) );

                ta.execute( insert, Z_FUNCTION );
                ta.rollback( Z_FUNCTION );
            }
            catch( exception& )
            {
                //if( _db.dbms_kind() == dbms_access )
                //{
                //    Transaction ta ( this );
                //    ta.intermediate_commit( Z_FUNCTION );
                //    recreate_table( &ta, _spooler->_order_history_tablename, column_definitions, primary_key );
                //    ta.commit( Z_FUNCTION );
                //    created = true;
                //}
                //else
                //{
                    Transaction ta ( this );
                    alter_column_allow_null( &ta, _spooler->_order_history_tablename, "end_time", "datetime" );
                    ta.commit( Z_FUNCTION );
                //}
            }
        }

        if( created )
        {
            Transaction ta ( this );
            ta.create_index( _spooler->_order_history_tablename, "SCHEDULER_O_HISTORY_SPOOLER_ID", "SCHED_O_HIST_1", "`spooler_id`"            , Z_FUNCTION );
            ta.create_index( _spooler->_order_history_tablename, "SCHEDULER_O_HISTORY_JOB_CHAIN" , "SCHED_O_HIST_2", "`job_chain`, `order_id`" , Z_FUNCTION );
            ta.create_index( _spooler->_order_history_tablename, "SCHEDULER_O_HISTORY_START_TIME", "SCHED_O_HIST_3", "`start_time`"            , Z_FUNCTION );
            ta.commit( Z_FUNCTION );
        }
    }

    ////////////////////////////

    {
        Transaction ta ( this );

        bool created = create_table_when_needed( &ta, _spooler->_order_step_history_tablename, S() <<
                                "`history_id`"  " integer"      " not null,"             // Primärschlüssel
                                "`step`"        " integer"      " not null,"             // Primärschlüssel
                                "`task_id`"     " integer"      " not null,"
                                "`state`"       " varchar(100)" " not null,"
                                "`start_time`"  " datetime"     " not null,"
                                "`end_time`"    " datetime"        << null << ","
                                "`error`"       " boolean"         << null << ","
                                "`error_code`"  " varchar(50)"     << null << ","
                                "`error_text`"  " varchar(250)"    << null << ","
                                "primary key( `history_id`, `step` )" );

        if( created )
        {
        }
        else
        {
            bool         added = add_column( &ta, _spooler->_order_step_history_tablename, "error"     , S() << "add `error` boolean" << null );
            if( added )  added = add_column( &ta, _spooler->_order_step_history_tablename, "error_code", S() << "add `error_code` varchar(250)" << null );
            if( added )  added = add_column( &ta, _spooler->_order_step_history_tablename, "error_text", S() << "add `error_text` varchar(250)" << null );
        }

        ta.commit( Z_FUNCTION );
    }

    ////////////////////////////

    {
        Transaction ta ( this );
        handle_order_id_columns( &ta );

        ta.commit( Z_FUNCTION );
    }
}

//---------------------------------------------------------------Database::create_table_when_needed

bool Database::create_table_when_needed( Transaction* ta, const string& tablename, const string& fields )
{
    bool result = false;

    if( _db_name == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    try
    {
        //Transaction ta ( this );        // Select und Create table nicht in derselben Transaktion. Für Access und PostgresQL

        Any_file select = ta->open_result_set( "select count(*)  from " + tablename + "  where 1=0", Z_FUNCTION );
        select.get_record();
        select.close();
        // ok
    }
    catch( exception& x )
    {
        _log->warn( x.what() );
        _log->info( message_string( "SCHEDULER-909", tablename ) );

        try
        {
            ta->intermediate_commit( Z_FUNCTION ); 

            S create_table;
            create_table << "CREATE TABLE " << tablename << " (" << fields << ")";
            if( dbms_kind() == dbms_mysql )  create_table << " Type=InnoDB";

            ta->execute( create_table, Z_FUNCTION );

            //ta->intermediate_commit( Z_FUNCTION ); 

            result = true;
        }
        catch( exception& x )
        {
            z::throw_xc( "SCHEDULER-363", tablename, x );
        }
    }

    return result;
}

//-------------------------------------------------------------------------Database::recreate_table
// Wird nicht benutzt.

void Database::recreate_table( Transaction* ta, const string& tablename, const string& column_definitions, const string& primary_key )
{
    //log()->info( message_string( "SCHEDULER-704", tablename ) );

    string copy_tablename = tablename + "_copy";

    ta->execute( S() << "CREATE TABLE " << copy_tablename << " ( " << column_definitions << " )", Z_FUNCTION );
    ta->execute( S() << "INSERT into " << copy_tablename << " select *  from " << tablename     , Z_FUNCTION );
    ta->execute( S() << "DROP TABLE " << tablename                                              , Z_FUNCTION );

    ta->execute( S() << "CREATE TABLE " << tablename << " ( " << column_definitions << ", primary key( " << primary_key << " ) )", Z_FUNCTION );  // Besser: primary key nach insert einrichten
    ta->execute( S() << "INSERT into " << tablename << " select *  from " << copy_tablename                                      , Z_FUNCTION );
    ta->execute( S() << "DROP TABLE " << copy_tablename                                                                          , Z_FUNCTION );
}

//-----------------------------------------------------------------------------Database::add_column

bool Database::add_column( Transaction* ta, const string& table_name, const string& column_name, const string add_clause )
{
    bool result = false;

    if( _db_name == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    try
    {
        ta->open_file( "-in " + db_name() + " -max-length=1K -read-transaction", "select `" + column_name + "` from " + table_name + " where 1=0", Z_FUNCTION );
    }
    catch( exception& x )
    {
        _log->warn( x.what() );

        ta->intermediate_commit( Z_FUNCTION ); 
        
        string cmd = "ALTER TABLE " + table_name + " " + add_clause;
        _log->info( message_string( "SCHEDULER-908", table_name, column_name, cmd ) );
        
        ta->execute( cmd, Z_FUNCTION );
        ta->intermediate_commit( Z_FUNCTION ); 

        result = true;
    }

    return result;
}

//----------------------------------------------------------------Database::alter_column_allow_null

bool Database::alter_column_allow_null( Transaction* ta, const string& table_name, const string& column_name, const string& type )
{
    bool result = false;
    S    cmd;

    switch( _db.dbms_kind() )
    {
        case dbms_access:         // Die Anweisung ist wirkungslos (eMail von Ghassan Beydoun 2007-03-06 12:53)
        {
            // Access kann NULL nicht wegnehmen. Und kann auch Spalten nicht umbenennen. Deshalb kopieren wir zweimal:

            string column_copy_name = column_name + "_copy";
            rename_column( ta, table_name, column_name, column_copy_name, type );
            rename_column( ta, table_name, column_copy_name, column_name, type );
            result = true;
            break;
        }


        case dbms_sql_server:
            cmd << "ALTER TABLE " << table_name << " alter column `" << column_name << "` " << type << " null";
            break;

        case dbms_firebird:
            cmd << "ALTER TABLE " << table_name << " alter column `" << column_name << "` type " << type << " null";
            break;

        case dbms_db2:
        case dbms_postgresql:
            cmd << "ALTER TABLE " << table_name << " alter column `" << column_name << "` drop not null";
            break;

        case dbms_mysql:    // "datetime not null" wirkt wie "datetime null"
            cmd << "ALTER TABLE " << table_name << " modify column `" << column_name << "` " << type << " null";
            break;

        case dbms_sybase:
            cmd << "ALTER TABLE " << table_name << " modify `" << column_name << "` " << type << " null";
            break;

        case dbms_oracle:
        case dbms_oracle_thin:
        default:
            cmd << "ALTER TABLE " << table_name << " modify ( `" << column_name << "` " << type << " null )";
    }


    if( !cmd.empty() )
    {
        _log->info( cmd );
        ta->execute( cmd, Z_FUNCTION );

        result = true;
    }

    return result;
}

//--------------------------------------------------------------------------Database::rename_column

void Database::rename_column( Transaction* ta, const string& table_name, const string& column_name, const string& new_column_name, const string& type )
{
    switch( _db.dbms_kind() )
    {
        case dbms_access:         // Die Anweisung ist wirkungslos (eMail von Ghassan Beydoun 2007-03-06 12:53)
        {
            ta->execute( S() << "ALTER TABLE " << table_name << " add column `" << new_column_name << "` " << type << " null", Z_FUNCTION );
            ta->execute( S() << "UPDATE `" << table_name << "` set `" << new_column_name << "`=`" << column_name << "`"      , Z_FUNCTION );
            ta->execute( S() << "ALTER TABLE " << table_name << " drop column `" << column_name << "`"                       , Z_FUNCTION );
            break;
        }

        default:
            z::throw_xc( Z_FUNCTION );
    }
}

//---------------------------------------------------------------------Database::column_is_nullable

//bool Database::column_is_nullable( const string& table_name, const string& column_name )
//{
//    // true ist nicht zuverlässig
//
//    return Any_file( S() << "-in " << db_name() << "select `" << column_name <<"` from " << table_name << " where 1=0" )
//           .record_type()->field_descr(0).nullable();
//}

//----------------------------------------------------------------Database::handle_order_id_columns

void Database::handle_order_id_columns( Transaction* ta )
{
    int orders_column_width  = expand_varchar_column( ta, _spooler->_orders_tablename       , "id"      , const_order_id_length_max - 1, const_order_id_length_max );
    int history_column_width = expand_varchar_column( ta, _spooler->_order_history_tablename, "order_id", const_order_id_length_max - 1, const_order_id_length_max );

    _order_id_length_max = 0;

    if( orders_column_width > 0  )  _order_id_length_max = orders_column_width;
    if( history_column_width > 0 )  _order_id_length_max = history_column_width;

    if( _order_id_length_max == 0 )
    {
        _order_id_length_max = const_order_id_length_max;
        _log->warn( message_string( "SCHEDULER-350", _spooler->_orders_tablename, "id", _order_id_length_max ) );
    }

    Z_LOG2( "scheduler", "_order_id_length_max=" << _order_id_length_max << "\n" );
}

//------------------------------------------------------------------Database::expand_varchar_column

int Database::expand_varchar_column( Transaction* ta, const string& table_name, const string& column_name, int minimum_width, int new_width )
{
    int width = column_width( ta, table_name, column_name );
    
    if( width < minimum_width )
    {
        try
        {
            ta->intermediate_commit( Z_FUNCTION );

            S cmd;

            switch( _db.dbms_kind() )
            {
                case dbms_access:
                {
                    cmd << " alter column " << sql::uquoted_name( column_name ) 
                        << " varchar(" << new_width << ")";

                    _log->info( cmd );
                    ta->execute( cmd, Z_FUNCTION );
                    break;
                }


                case dbms_sql_server:
                case dbms_sybase:
                {
                    if( table_name == _spooler->_orders_tablename  &&  lcase(column_name) == "id" )
                    {
                        cmd << "-split- ";  // Für sosdb.cxx: Nicht an Semikolons auftrennen
                        
                        cmd << "DECLARE @pk_id varchar(255); ";
                        cmd << "BEGIN ";
                        cmd <<      "SELECT @pk_id = \"NAME\" from sysindexes where name like 'PK__" << table_name << "_%'; ";
                        cmd <<      "EXEC ('ALTER TABLE " << table_name << " DROP CONSTRAINT ' + @pk_id ); ";
                        cmd <<      "ALTER TABLE " << table_name << " ALTER COLUMN \"ID\" VARCHAR(" << new_width << ") NOT NULL; ";
                        cmd <<      "ALTER TABLE " << table_name << " ADD PRIMARY KEY (\"JOB_CHAIN\", \"ID\"); ";
                        cmd << "END;";

                        _log->info( cmd );
                        ta->execute( cmd, Z_FUNCTION );
                    }
                    else
                    {
                        cmd << "ALTER TABLE " << table_name;
                        cmd << " alter column " << sql::uquoted_name( column_name ) 
                            << " varchar(" << new_width << ")";

                        _log->info( cmd );
                        ta->execute( cmd, Z_FUNCTION );
                    }

                    break;
                }


                case dbms_firebird:
                    /*  Hallo Joacim,

                        damit wir es nicht vergessen, einbauen möchte ich es nicht. wir bleiben
                        dabei, dass wir für Firebird aktuell gar nichts tun (also auskommentiert
                        lassen).

                        Das Problem ist, dass Firebird 1.5 nur kleine Indizes verwenden kann, die
                        nicht mehr als 250 Byte lang sind. Abzgl. einiger Metadaten können wir
                        aktuell tatsächlich nur 2 Attribute zu je 100 Byte aufnehmen. 

                        Mit Firebird 2.x wird sich das verbessern, da dann Funktionsindizes
                        unterstützt werden, mit denen ggf. nur ein Teil des Attributs in den Index
                        aufgenommen wird. Sollte sich diese Version durchgesetzt haben, können wir
                        das wieder reaktivieren.

                        Für Firebird geht es so:

                        SET TERMINATOR ^;
                        RECREATE PROCEDURE tmp AS DECLARE VARIABLE pk_id varchar(255); 
                        BEGIN
                          SELECT "RDB$CONSTRAINT_NAME" FROM RDB$RELATION_CONSTRAINTS WHERE
                        "RDB$RELATION_NAME"='SCHEDULER_ORDERS' AND "RDB$CONSTRAINT_TYPE"='PRIMARY KEY' INTO :pk_id;
                          EXECUTE STATEMENT 'ALTER TABLE SCHEDULER_ORDERS DROP CONSTRAINT ' || :pk_id;
                          EXECUTE STATEMENT 'ALTER TABLE SCHEDULER_ORDERS ALTER "ID" TYPE VARCHAR(255)';
                          EXECUTE STATEMENT 'ALTER TABLE SCHEDULER_ORDERS ADD PRIMARY KEY ("JOB_CHAIN", "ID")';
                        END^
                        SET TERMINATOR ;^
                        COMMIT RETAIN;
                        EXECUTE PROCEDURE tmp;

                        Dieser gesamte Block muss in einem Statement abgesetzt werden.
                        Danach in einem neuen Statement

                        TABLE SCHEDULER_ORDER_HISTORY ALTER "ORDER_ID" TYPE VARCHAR(255);

                        Gruß
                        Andreas
                    */
                    return width;


                case dbms_postgresql:
                {
                    cmd << "ALTER TABLE " << table_name;
                    cmd << " alter column " << sql::uquoted_name( column_name ) 
                        << " type varchar(" << new_width << ")";
                    
                    _log->info( cmd );
                    ta->execute( cmd, Z_FUNCTION );
                    break;
                }


                case dbms_db2:
                {
                    cmd << "ALTER TABLE " << table_name;
                    cmd << " alter column " << sql::uquoted_name( column_name ) 
                        << " set data type varchar(" << new_width << ")";

                    _log->info( cmd );
                    ta->execute( cmd, Z_FUNCTION );
                    break;
                }


                case dbms_oracle:
                case dbms_oracle_thin:
                case dbms_mysql:
                default:
                {
                    cmd << "ALTER TABLE " << table_name;
                    cmd << " modify " << sql::uquoted_name( column_name ) 
                        << " varchar(" << new_width << ")";
                    
                    _log->info( cmd );
                    ta->execute( cmd, Z_FUNCTION );
                    break;
                }
            }

            ta->intermediate_commit( Z_FUNCTION );    // Damit Postgres nicht in column_width() hängen bleibt (Linux)
        }
        catch( exception& x )
        {
            _log->warn( x.what() );
            _log->warn( message_string( "SCHEDULER-349", table_name, column_name ) );
        }

        width = column_width( ta, table_name, column_name );
        if( width != new_width )  _log->warn( S() << "Retrieved column width is different: " << width );
    }

    return width;
}

//---------------------------------------------------------------------------Database::column_width

int Database::column_width( Transaction* ta, const string& table_name, const string& column_name )
{
    int result;

    if( _db_name == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    Any_file f = ta->open_result_set( S() << "select `" << column_name << "` from " << table_name << " where 1=0", Z_FUNCTION );
    int field_size= +f.spec()._field_type_ptr->field_descr_ptr( 0 )->type_ptr()->field_size();
    result = max( 0, field_size - 1 );   // Eins weniger fürs 0-Byte

    if( result == 0 )  _log->warn( message_string( "SCHEDULER-348", _spooler->_orders_tablename, ".id" ) );

    Z_LOG2( "scheduler", "width( " << table_name << "." << column_name << ") = " << result << "\n" );

    return result;
}

//----------------------------------------------------------------------------------Database::close

void Database::close()
{
    Z_MUTEX( _lock )
    {
      //_job_id_select.close();
      //_job_id_select.destroy();

      //_history_update.close();

        _history_table.close();
        _history_table.destroy();

        try
        {
            _db.close();  // odbc.cxx und jdbc.cxx unterdrücken selbst Fehler.
        }
        catch( exception& x ) { _log->warn( message_string( "SCHEDULER-310", x ) ); }

        _db.destroy();
    }
}

//---------------------------------------------------------------------Database::open_history_table

void Database::open_history_table( Read_transaction* ta )
{
    if( _db_name == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    THREAD_LOCK( _lock )
    {
        if( !_history_table.opened() )
        {
            ta->set_transaction_read();
            _history_table.open( "-in -out -key=id sql -table=" + _spooler->_job_history_tablename + 
                               //" -sql-fields=(id,spooler_id,job_name,start_time,cause,parameters) | " +       extra-Felder nicht vergessen!
                                 " | " + _db_name + " -ignore=(parameters,log) -max-length=" + as_string(blob_field_size) );
        }
    }
}

//-----------------------------------------------------------------Database::try_reopen_after_error

void Database::try_reopen_after_error( const exception& callers_exception, const string& function, bool wait_endless )
{
    bool    too_much_errors = false;
    string  warn_msg;


    // Wenn ein TCP-Kommando ausgeführt wird, müssen wir hier sofort raus, sonst wird das Kommando doppelt oder rekursiv aufgerufen werden async_continue_selected(), s.u.
    if( _spooler->_executing_command )  throw;

    if( In_recursion in_recursion = &_waiting )  throw_xc( callers_exception );   
    else
    {
        THREAD_LOCK( _error_lock )  _error = callers_exception.what();

        _spooler->log()->error( message_string( "SCHEDULER-303", callers_exception, function ) );

        if( _db.opened() )  _spooler->log()->info( message_string( "SCHEDULER-957" ) );   // "Datenbank wird geschlossen"
        try
        {
            close();
        }
        catch( exception& x ) { _log->warn( message_string( "SCHEDULER-310", x ) ); }       // Fehler beim Schließen der Datenbank


        while( !_db.opened()  &&  !too_much_errors )
        {
            if( !_spooler->_executing_command )
            {
                warn_msg = "After max_db_errors=" + as_string(_spooler->_max_db_errors) + " Problems with the database it is no longer used";

                if( !wait_endless || _error_count == 0 )  ++_error_count;
                
                if( !wait_endless  &&  _error_count >= _spooler->_max_db_errors )
                {
                    too_much_errors = true;
                    break;
                }


                if( !_email_sent_after_db_error )
                {
                    string body = "This is the " + as_string(_error_count) + ". problem with the database.";
                    if( !_spooler->_wait_endless_for_db_open )  body += "\n(" + warn_msg + ")";
                    body += "\ndb=" + remove_password( _spooler->_db_name ) + "\r\n\r\n" + callers_exception.what() + "\r\n\r\nThe Scheduler is trying to open the database again.";
                    //if( !_spooler->_need_db )  body += "\r\nWenn das nicht geht, schreibt der Scheduler die Historie in Textdateien.";

                    Scheduler_event scheduler_event ( evt_database_error, log_warn, this );
                    scheduler_event.set_error( callers_exception );
                    scheduler_event.set_count( _error_count );

                    Mail_defaults mail_defaults ( _spooler );
                    mail_defaults.set( "subject", S() << "ERROR ON DATABASE ACCESS: " << callers_exception.what() );
                    mail_defaults.set( "body"   , body );

                    scheduler_event.send_mail( mail_defaults );

                    _email_sent_after_db_error = true;
                }
            }

            //sos_sleep( 2 );    // Bremse, falls der Fehler nicht an einer unterbrochenen Verbindung liegt. Denn für jeden Fehler gibt es eine eMail!

            while(1)
            {
                try
                {
                    open2( _spooler->_db_name );
                    //open_history_table();
                    THREAD_LOCK( _error_lock )  _error = "";
 
                    _reopen_time = ::time(NULL);
                    break;
                }
                catch( exception& x )
                {
                    THREAD_LOCK( _error_lock )  _error = x.what();

                    if( _spooler->_executing_command )  throw;

                    _spooler->log()->warn( x.what() );

                    if( !_spooler->_need_db )  break;
                    
                    if( !_spooler->_wait_endless_for_db_open )  // need_db=strict?
                    {
                        too_much_errors = true;
                        warn_msg = message_string( "SCHEDULER-314", _spooler->string_need_db() );   // "Datenbank lässt sich nicht öffnen. Wegen need_db=strict wird der Scheduler sofort beendet.";
                        break;
                    }

                    _spooler->log()->warn( message_string( "SCHEDULER-958", seconds_before_reopen ) );   // "Eine Minute warten bevor Datenbank erneut geöffnet wird ..."
                    _spooler->_connection_manager->async_continue_selected( is_allowed_operation_while_waiting, seconds_before_reopen );
                }
            }
        }


        if( _db.opened() )
        {
            if( _email_sent_after_db_error )    // Zurzeit immer true
            {
                Scheduler_event scheduler_event ( evt_database_continue, log_info, this );

                Mail_defaults mail_defaults ( _spooler );

                mail_defaults.set( "subject", "Scheduler is connected again with the database" );
                mail_defaults.set( "body"   , "Scheduler continues processing.\n\ndb=" + remove_password( _spooler->_db_name ) );

                scheduler_event.send_mail( mail_defaults );
            }
        }
        else
        {
            if( too_much_errors )
            {
                _spooler->log()->warn( warn_msg );
                
                if( _spooler->_need_db ) 
                {
                    string msg = message_string( "SCHEDULER-265", callers_exception );     // "SCHEDULER WIRD BEENDET WEGEN FEHLERS BEIM ZUGRIFF AUF DATENBANK"
                    _log->error( msg );

                    Scheduler_event scheduler_event ( evt_database_error_abort, log_error, this );
                    scheduler_event.set_error( callers_exception );
                    scheduler_event.set_scheduler_terminates( true );

                    Mail_defaults mail_defaults ( _spooler );

                    mail_defaults.set( "subject", msg );
                    mail_defaults.set( "body"   , S() << "db=" << remove_password( _spooler->_db_name ) << "\r\n\r\n" << callers_exception.what() << "\r\n\r\n" << warn_msg );

                    scheduler_event.send_mail( mail_defaults );
                    

                    _spooler->abort_immediately();
                }
            }

            _spooler->log()->info( message_string( "SCHEDULER-959" ) );   // "Historie wird von Datenbank auf Dateien umgeschaltet" );

            Scheduler_event scheduler_event ( evt_database_error_switch_to_file, log_warn, this );
            scheduler_event.set_error( callers_exception );

            Mail_defaults mail_defaults( _spooler );
            mail_defaults.set( "subject", string("SCHEDULER CONTINUES WITHOUT DATABASE AFTER ERRORS: ") + callers_exception.what() );
            mail_defaults.set( "body"   , S() << "Because of need_db=no\n" "db=" << remove_password( _spooler->_db_name ) << "\r\n\r\n" << callers_exception.what() << "\r\n\r\n" << warn_msg );
            
            scheduler_event.send_mail( mail_defaults );

            open2( "" );     // Umschalten auf dateibasierte Historie
        }

        if( !_transaction ||  !_transaction->_suppress_heart_beat_timeout_check )  _spooler->assert_is_still_active( Z_FUNCTION );
    }
}

//----------------------------------------------------------------------------Database::lock_syntax

Database_lock_syntax Database::lock_syntax()
{
    switch( _db.dbms_kind() )
    {
        case dbms_sybase: 
        case dbms_sql_server: return db_lock_with_updlock;      // from table  with(updlock)

        case dbms_access    : return db_lock_none;              

                     default: return db_lock_for_update;        // for update
    }
}

//---------------------------------------------------------------------------------Database::get_id
// Wird von den Threads gerufen

int Database::get_id( const string& variable_name, Transaction* outer_transaction )
{
    int  id;

    try
    {
        while(1)
        {
            try
            {
                id = get_id_( variable_name, outer_transaction );
                break;
            }
            catch( exception& x )
            {
                if( outer_transaction )  throw;         // Fehlerschleife in der rufenden Routine, um Transaktion und damit _lock freizugeben!
                try_reopen_after_error( x, Z_FUNCTION );
            }
        }
    }
    catch( exception& x ) 
    { 
        _spooler->log()->error( message_string( "SCHEDULER-304", x, variable_name ) );   // "FEHLER BEIM LESEN DER NÄCHSTEN ID: "
        throw;
    }

    return id;
}

//--------------------------------------------------------------------------------Database::get_id_
// Wird von den Threads gerufen

int Database::get_id_( const string& variable_name, Transaction* outer_transaction )
{
    int id;

    //if( get_java_vm(false)->running() )  get_java_vm(false)->attach_thread( "" );


    if( _db.opened() )
    {
        Transaction ta ( this, outer_transaction );

        if( dbms_kind() == dbms_access )
        {
            ta.execute( S() << "UPDATE " << _spooler->_variables_tablename << "  set `wert`=`wert`+1  where `name`=" << sql::quoted( variable_name ), Z_FUNCTION );

            Any_file sel = ta.open_result_set( S() << "select `wert`  from " << _spooler->_variables_tablename << "  where `name`=" << sql::quoted( variable_name ),
                                               Z_FUNCTION );
            if( sel.eof() )
            {
                id = 1;
                ta.execute( S() << "INSERT into " << _spooler->_variables_tablename << " (`name`,`wert`) " 
                            "values (" << sql::quoted( variable_name ) << "," << id << ")",
                            Z_FUNCTION);
            }
            else
            {
                id = sel.get_record().as_int(0);
            }
        }
        else
        {
            for( int tries = 2; tries > 0; tries-- )
            {
                Any_file sel = ta.open_commitable_result_set( S() << "select `wert`  from " << _spooler->_variables_tablename << " %update_lock  where `name`=" << sql::quoted( variable_name ), 
                                                              Z_FUNCTION );
                if( !sel.eof() )
                {
                    id = sel.get_record().as_int(0) + 1;
                    ta.execute( S() << "UPDATE " << _spooler->_variables_tablename << "  set `wert`=`wert`+1  where `name`=" << sql::quoted( variable_name ), Z_FUNCTION );
                    tries = 0;
                }
                else
                {
                    try
                    {
                        id = 1;
                        ta.execute( S() << "INSERT into " << _spooler->_variables_tablename << " (`name`,`wert`) " 
                                    "values (" << sql::quoted( variable_name ) << "," << id << ")",
                                    Z_FUNCTION);
                    }
                    catch( exception& x )
                    {
                        if( tries <= 1 )  throw;
                        _log->warn( x.what() );     // Möglicherweise hat gerade ein anderer den Satz eingefügt
                    }
                }
            }
        }

        Z_LOG2( "scheduler", "Database::get_id(\"" + variable_name + "\") = " << id << '\n' );

        _id_counters[ variable_name ] = id + 1;

        ta.commit( Z_FUNCTION );
    }
    else
    if( _waiting )
    {
        z::throw_xc( "SCHEDULER-184" );
    }
    else
    {
        id = InterlockedIncrement( &_id_counters[ variable_name ] );
    }

    return id;
}

//-------------------------------------------------------------------Transaction::get_variable_text

string Transaction::get_variable_text( const string& name, bool* record_exists )
{
    Any_file select = open_result_set( S() << "select \"TEXTWERT\"  from " << _spooler->_variables_tablename << "  where \"NAME\"=" << sql::quoted( name ), Z_FUNCTION );

    if( select.eof() )
    {
        if( record_exists )  *record_exists = false;
        return "";
    }
    else
    {
        if( record_exists )  *record_exists = true;
        return select.get_record().as_string(0);
    }
}

//---------------------------------------------------------------------------Database::set_variable

void Transaction::set_variable( const string& name, const string& value )
{
    if( !try_update_variable( name, value ) )
    {
        insert_variable( name, value );
    }
}

//---------------------------------------------------------------------Transaction::insert_variable

void Transaction::insert_variable( const string& name, const string& value )
{
    assert( _db );
    sql::Insert_stmt insert ( database_descriptor(), _spooler->_variables_tablename );
    
    insert[ "name" ] = name;
    insert[ "textwert" ] = value;

    execute( insert, Z_FUNCTION );
}

//-------------------------------------------------------------------------Transaction::set_variable
/* Nicht getestet
void Transaction::update_variable( const string& name, const string& value )
{
    sql::Update_stmt update ( database_descriptor(), _spooler->_variables_tablename );
    
    update.and_where_condition( "name", name );
    update[ "wert"     ] = value;

    execute_single( update );
}
*/
//-----------------------------------------------------------------Transaction::try_update_variable

bool Transaction::try_update_variable( const string& name, const string& value )
{
    assert( _db );

    sql::Update_stmt update ( database_descriptor(), _spooler->_variables_tablename );
    
    update.and_where_condition( "name", name );
    update[ "textwert" ] = value;

    execute( update, Z_FUNCTION );

    return record_count() == 1;
}

//-----------------------------------------------------------------------------Transaction::execute

void Transaction::execute( const string& stmt, const string& debug_text, Execute_flags flags )
{ 
    bool is_commit   = stmt == "COMMIT";
    bool is_rollback = stmt == "ROLLBACK";
  //if( !is_commit_or_rollback )  set_transaction_written();

    if( flags == ex_force || !is_commit && !is_rollback || need_commit_or_rollback() )
    {
        string debug_extra;
        if( debug_text != "" )  debug_extra = "  (" + debug_text + ")";
        if( !_log->is_enabled_log_level( _spooler->_db_log_level ) )  Z_LOG2( "scheduler", Z_FUNCTION << "  " << stmt << debug_extra << "\n" );

        string native_sql = flags == ex_native? stmt 
                                              : db()->_db.sos_database_session()->translate_sql( stmt );

        try
        {
            _db->_db.put( "%native " + native_sql );    // -split-: Semikolons nicht auftrennen, sondern alles auf einmal übergeben
        }
        catch( zschimmer::Xc& x )
        {
            // Besser Fehler melden (mit Andreas klären)
            //_log->warn( stmt );
            //_log->error( x.what() );
            _log->log( _spooler->_db_log_level, native_sql + debug_extra );
            _log->log( _spooler->_db_log_level, x.what() );

            x.append_text( debug_text );
            throw;
        }
        catch( exception& x )
        {
            // Besser Fehler melden (mit Andreas klären)
            //_log->warn( stmt );
            //_log->error( x.what() );
            _log->log( _spooler->_db_log_level, native_sql + debug_extra );
            _log->log( _spooler->_db_log_level, x.what() );

            throw;
        }

        if( _log->is_enabled_log_level( _spooler->_db_log_level ) )      // Hostware protokolliert schon ins scheduler.log
        {
            S line;
            line << native_sql;
            line << debug_extra;
            if( record_count() >= 0 )  line << "  ==> " << record_count() << " records";
            _log->log( _spooler->_db_log_level, line );
        }
    }
}

//----------------------------------------------------------------------Transaction::execute_single

void Transaction::execute_single( const string& stmt, const string& debug_text )
{ 
    execute( stmt, debug_text );
    if( record_count() != 1 )  z::throw_xc( "SCHEDULER-355", record_count(), stmt );
}

//------------------------------------------------------------------Transaction::try_execute_single

bool Transaction::try_execute_single( const string& stmt, const string& debug_text )
{ 
    execute( stmt, debug_text );
    return record_count() == 1;
}

//------------------------------------------------------------------------Transaction::create_index

bool Transaction::create_index( const string& table_name, const string& index_name, const string& short_index_name, const string& column_list, 
                                const string& debug_text )
{
    bool result = false;

    S create_index_sql;
    create_index_sql << "CREATE INDEX " 
                     << ( _db->dbms_kind() == dbms_db2? short_index_name : index_name )
                     << " on " << table_name
                     << " (" << column_list << ")";

    try
    {
        execute( create_index_sql, debug_text );
    }
    catch( exception& x )
    {
        _log->warn( x.what() );
        _log->info( message_string( "SCHEDULER-880" ) );
    }

    return result;
}

//--------------------------------------------------------------------------Database::spooler_start

void Database::spooler_start()
{
    if( _db.opened() )
    {
        //try   Fehler beim Spooler-Start führen zum Abbruch
        {
            _id = get_task_id();     // Der Spooler-Satz hat auch eine Id

            if( _db.opened() )   // get_id() kann die DB schließen (nach Fehler)
            {
                Transaction ta ( this );
                {
                    sql::Insert_stmt insert ( ta.database_descriptor(), _spooler->_job_history_tablename );
        
                    insert[ "id"                ] = _id;
                    insert[ "spooler_id"        ] = _spooler->id_for_db();

                    if( _spooler->is_cluster() )
                    insert[ "cluster_member_id" ] = _spooler->cluster_member_id();

                    insert[ "job_name"          ] = "(Spooler)";
                    insert[ "start_time"        ].set_datetime( Time::now().as_string(Time::without_ms) );
                    insert[ "pid"               ] = getpid();

                    ta.execute( insert, Z_FUNCTION );

                    ta.commit( Z_FUNCTION );
                }
            }
        }
        //catch( exception& x )  
        //{ 
        //    _log->warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        //}
    }
}

//---------------------------------------------------------------------------Database::spooler_stop

void Database::spooler_stop()
{
    if( _db.opened() )
    {
        try
        {
            Transaction ta ( this );
            {
                sql::Update_stmt update ( database_descriptor() );
                update.set_table_name( _spooler->_job_history_tablename  );
                update[ "end_time" ].set_datetime( Time::now().as_string(Time::without_ms) );
                update.and_where_condition( "id", _id );
                ta.execute( update, Z_FUNCTION );

                ta.commit( Z_FUNCTION );
            }
        }
        catch( exception& x )  
        { 
            _log->warn( message_string( "SCHEDULER-306", _spooler->_job_history_tablename, x ) ); 
        }
    }
}

//----------------------------------------------------------------------------Database::update_clob

void Transaction::update_clob( const string& table_name, const string& column_name, const string& key_name, int key_value, const string& value )
{
    if( value == "" )
    {
        sql::Update_stmt update ( database_descriptor() );
        update.set_table_name( table_name );
        update[ column_name ].set_direct( "null" );
        update.and_where_condition( key_name, key_value );
        execute( update, Z_FUNCTION );
    }
    else
    {
        update_clob( table_name, column_name, value, "where `" + key_name + "`=" + as_string( key_value ) );
    }
}

//----------------------------------------------------------------------------Database::update_clob

void Transaction::update_clob( const string& table_name, const string& column_name, const string& key_name, const string& key_value, const string& value )
{
    if( db()->db_name() == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    if( value == "" )
    {
        sql::Update_stmt update ( database_descriptor() );
        update.set_table_name( table_name );
        update[ column_name ].set_direct( "null" );
        update.and_where_condition( key_name, key_value );
        execute( update, Z_FUNCTION );
    }
    else
    {
        update_clob( table_name, column_name, value, "where `" + key_name + "`=" + sql::quoted( key_value ) );
    }
}

//----------------------------------------------------------------------------Database::update_clob

void Transaction::update_clob( const string& table_name, const string& column_name, const string& value, const string& where )
{
    string hostware_filename = S() <<  "-no-blob-record-allowed -table=" << table_name << " -clob=" << column_name << "  " << where;
    // Falls wir mal auf direkten Aufruf von jdbc umstellen, kann -no-blob-record-allowed wegfallen. Dann können wir das genauer programmieren.

    Any_file clob = open_file( "-out -binary " + db()->db_name(), hostware_filename );

    try
    {
        clob.put( value );
        clob.close();
    }
    catch( exception& x )
    {
        _log->log( _spooler->_db_log_level, S() << x.what() << ", when writing clob: " << hostware_filename );
        throw;
    }
}

//------------------------------------------------------------------------------Database::read_task
// Die XML-Struktur ist wie Task::dom_element(), nicht wie Job_history::read_tail()

xml::Element_ptr Database::read_task( const xml::Document_ptr& doc, int task_id, const Show_what& show )
{
    if( !opened() )  z::throw_xc( "SCHEDULER-184" );

    xml::Element_ptr task_element = doc.createElement( "task" );

    try
    {
        Transaction ta ( this );
        {
            Any_file sel = ta.open_result_set(
                            "select \"SPOOLER_ID\", \"JOB_NAME\", \"START_TIME\", \"END_TIME\", \"CAUSE\", \"STEPS\", \"ERROR\", \"ERROR_CODE\", \"ERROR_TEXT\" " 
                            "  from " + _spooler->_job_history_tablename  +
                            "  where \"ID\"=" + as_string(task_id),
                            Z_FUNCTION);
            if( sel.eof() )  z::throw_xc( "SCHEDULER-207", task_id );

            Record record = sel.get_record();


            // s.a. Task::dom_element() zum Aufbau des XML-Elements <task>
            task_element.setAttribute( "id"              , task_id );
            //task_element.setAttribute( "state"           , state_name() );

            //if( _thread )
            //task_element.setAttribute( "thread"          , _thread->name() );

            //task_element.setAttribute( "name"            , _name );

            //if( _running_since )
            //task_element.setAttribute( "running_since"   , _running_since.as_string() );
            task_element.setAttribute( "start_time"      , record.as_string( "START_TIME" ) );      // Gibt es nicht in Task::dom_element()
            task_element.setAttribute( "end_time"        , record.as_string( "END_TIME" ) );        // Gibt es nicht in Task::dom_element()

            //if( _idle_since )
            //task_element.setAttribute( "idle_since"      , _idle_since.as_string() );

            task_element.setAttribute( "cause"           , record.as_string( "CAUSE" ) );

            //if( _state == s_running  &&  _last_process_start_time )
            //task_element.setAttribute( "in_process_since", _last_process_start_time.as_string() );

            task_element.setAttribute( "steps"           , record.as_string( "STEPS" ) );

            //task_element.setAttribute( "log_file"        , _log.filename() );

            if( !record.null( "ERROR" )  &&  record.as_int( "ERROR" ) )  
            {
                xml::Element_ptr error_element = doc.createElement( "ERROR" );
                error_element.setAttribute( "code", record.as_string( "ERROR_CODE" ) );
                error_element.setAttribute( "text", record.as_string( "ERROR_TEXT" ) );
                task_element.appendChild( error_element );
            }

            if( show.is_set( show_log ) )
            {
                try
                {
                    ta.set_transaction_read();

                    string log = file_as_string( "-binary " GZIP_AUTO + db_name() + " -table=" + _spooler->_job_history_tablename + " -blob=\"LOG\"" 
                                                " where \"ID\"=" + as_string(task_id), "" );
                    task_element.append_new_text_element( "log", log );
                }
                catch( exception& x ) { _log->warn( message_string( "SCHEDULER-268", task_id, x ) ); }  // "FEHLER BEIM LESEN DES LOGS FÜR TASK "
            }
        }
    }
    catch( const _com_error& x ) { throw_com_error( x, "Database::read_task" ); }

    return task_element;
}

//-------------------------------------------------------------------------Job_history::Job_history

Job_history::Job_history( Job* job )
: 
    _zero_(this+1) 
{ 
    _job = job; 
    _spooler = _job->_spooler; 
}

//-------------------------------------------------------------------------Job_history::Job_history

Job_history::~Job_history()
{
    try
    {
        close();
    }
    catch( exception& x ) { _job->_log->warn( message_string( "SCHEDULER-269", x ) ); }  // "FEHLER BEIM SCHLIESSEN DER JOB-HISTORIE: "
}

//--------------------------------------------------------------------------------Job_history::open

void Job_history::open( Transaction* outer_transaction )
{
    string section = _job->profile_section();

    _job_path   = _job->path();            // Damit read_tail() nicht mehr auf Job zugreift (das ist ein anderer Thread)

    _filename   = read_profile_string            ( _spooler->_factory_ini, section, "history_file" );
    _history_yes= read_profile_bool              ( _spooler->_factory_ini, section, "history"           , _spooler->_job_history_yes );
    _on_process = read_profile_history_on_process( _spooler->_factory_ini, section, "history_on_process", _spooler->_job_history_on_process );

    try
    {
        if( !_history_yes )  return;

        if( _spooler->_db->opened()  &&  _filename == "" )
        {
            Transaction ta ( +_spooler->_db, outer_transaction );
            {
                _with_log = read_profile_with_log( _spooler->_factory_ini, section, "history_with_log", _spooler->_job_history_with_log );

                set<string> my_columns = set_map( lcase, set_split( ", *", replace_regex( string(history_column_names) + "," + history_column_names_db, ":[^,]+", "" ) ) );

                _spooler->_db->open_history_table( &ta );

                if( const Record_type* type = _spooler->_db->_history_table.spec().field_type_ptr() ) 
                {
                    _extra_type = SOS_NEW( Record_type );

                    for( int i = 0; i < type->field_count(); i++ )
                    {
                        string name = type->field_descr_ptr(i)->name();
                        if( my_columns.find( lcase(name) ) == my_columns.end() )  
                        {
                            _extra_names.push_back( name );
                            type->field_descr_ptr(i)->add_to( _extra_type );
                        }
                    }
                }
            }
            ta.commit( Z_FUNCTION );

            _use_db = true;
        }
        else
        {
            string         extra_columns = read_profile_string ( _spooler->_factory_ini, section, "history_columns", _spooler->_job_history_columns );
            Archive_switch arc           = read_profile_archive( _spooler->_factory_ini, section, "history_archive", _spooler->_job_history_archive );

            _type_string = history_column_names;

            _extra_type = make_record_type( extra_columns );

            if( extra_columns != "" )  _type_string += "," + extra_columns;

            _extra_names = vector_split( ", *", replace_regex( extra_columns, ":[^,]+", "" ) );

            if( _filename == "" )
            {
                _filename = "history";
                if( !_spooler->id().empty() )  _filename += "." + _spooler->id();
                _filename += ".job." + _job_path.to_filename() + ".txt";
            }
            _filename = make_absolute_filename( _spooler->log_directory(), _filename );
            if( _filename[0] == '*' )  return;      // log_dir = *stderr

            if( arc )  archive( arc, _filename );  

            _file.open( _filename, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, 0600 );
            _file.print( replace_regex( _type_string, "(:[^,]+)?,", "\t" ) + SYSTEM_NL );

            _job->_log->debug( message_string( "SCHEDULER-910", _filename ) );
            _use_file = true;
        }

         //record.type()->field_descr_ptr("error_text")->type_ptr()->field_size()
    }
    catch( exception& x )  
    { 
        _job->_log->warn( message_string( "SCHEDULER-270", x ) );   // "FEHLER BEIM ÖFFNEN DER HISTORIE: "
        _error = true;
    }
}

//-------------------------------------------------------------------------------Job_history::close

void Job_history::close()
{
    try
    {
        _use_db = false;
        _use_file = false;
      //_task_id = 0;

        _file.close();
    }
    catch( exception& x )  
    { 
        _job->_log->warn( message_string( "SCHEDULER-269", x ) );   //"FEHLER BEIM SCHLIESSEN DER HISTORIE: "
    }
}

//-----------------------------------------------------------------------------Job_history::archive

void Job_history::archive( Archive_switch arc, const File_path& filename )
{
    if( filename.file_exists() )
    {
        string ext   = filename.extension();
        string rumpf = filename;
        if( ext != "" )  rumpf = filename.substr( 0, filename.length() - ext.length() - 1 );

        Sos_optional_date_time time = Time::now().as_time_t();
        string arc_filename = rumpf + "." + time.formatted( "yyyy-mm-dd-HHMMSS" );
        if( ext != "" )  arc_filename +=  "." + ext;

        if( arc == arc_gzip )
        {
            arc_filename += ".gz";
            copy_file( "file -b " + filename, "gzip | " + arc_filename );
        }
        else
        {
            rename_file( filename, arc_filename );
        }

        _job->_log->info( message_string( "SCHEDULER-913", arc_filename ) );    // "Bisherige Historie ist archiviert worden unter "
    }
}

//---------------------------------------------------------------------------Job_history::read_tail
// Anderer Thread.
// Hier nicht auf _job etc. zugreifen!

xml::Element_ptr Job_history::read_tail( const xml::Document_ptr& doc, int id, int next, const Show_what& show, bool use_task_schema )
{
    bool with_log = show.is_set( show_log );

    xml::Element_ptr history_element;

    if( !_error )  
    {
        history_element = doc.createElement( "history" );
        dom_append_nl( history_element );

        with_log &= _use_db;

        try
        {
            try
            {
                if( !_history_yes )  z::throw_xc( "SCHEDULER-141", _job_path );

                if( _use_db  &&  !_spooler->_db->opened() )  z::throw_xc( "SCHEDULER-184" );     // Wenn die DB verübergegehen (wegen Nichterreichbarkeit) geschlossen ist, s. get_task_id()

                Transaction ta ( +_spooler->_db );
                {
                    Any_file sel;

                    if( _use_file )
                    {
                        if( id != -1  ||  next >= 0 )  z::throw_xc( "SCHEDULER-139" );
                        sel.open( "-in -seq tab -field-names | tail -head=1 -reverse -" + as_string(-next) + " | " + _filename );
                    }
                    else
                    if( _use_db )
                    {
                        S prefix;
                        S clause;

                        prefix << "-in -seq head -" << max( 1, abs(next) ) << " | ";

                        clause << " where `job_name`="        << sql_quoted( _job_path.without_slash() );
                        clause << " and `spooler_id`="        << sql_quoted( _spooler->id_for_db() );
                        clause << " and `cluster_member_id` " << sql::null_string_equation( _spooler->cluster_member_id() );
                        
                        if( id != -1 )
                        {
                            clause << " and `id`";
                            clause << ( next < 0? "<" : 
                                        next > 0? ">" 
                                                : "=" );
                            clause << id;
                        }

                        clause << " order by `id` ";  
                        if( next < 0 )  clause << " desc";
                        
                        if( _spooler->_db->_db_name == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

                        sel = ta.open_file( prefix + _spooler->_db->_db_name, 
                                S() << "select " <<
                                ( next == 0? "" : "%limit(" + as_string(abs(next)) + ") " ) <<
                                " `id`, `spooler_id`, `job_name`, `start_time`, `end_time`, `cause`, `steps`, `error`, `error_code`, `error_text`, "
                                " `cluster_member_id`, `exit_code`, `pid`" <<
                                join( "", vector_map( quote_and_prepend_comma, _extra_names ) ) <<
                                " from " << _spooler->_job_history_tablename <<
                                clause );
                    }
                    else
                        z::throw_xc( "SCHEDULER-136" );

                    const Record_type* type = sel.spec().field_type_ptr();
                    Dynamic_area rec ( type->field_size() );
        
                    while( !sel.eof() )
                    {
                        string           param_xml;
                        string           error_code;
                        string           error_text;
                        xml::Element_ptr history_entry = doc.createElement( "history.entry" );

                        sel.get( &rec );
            
                        for( int i = 0; i < type->field_count(); i++ )
                        {
                            string value = type->as_string( i, rec.byte_ptr() );
                            if( value != "" )
                            {
                                string name = lcase( type->field_descr_ptr(i)->name() );

                                if( name == "parameters" ) 
                                {
                                    param_xml = value;
                                }
                                else
                                if( name == "id" )  
                                {
                                    history_entry.setAttribute( "task", value );
                                    if( !use_task_schema )
                                        history_entry.setAttribute( "id", value );      // id sollte nicht verwendet werden. jz 6.9.04
                                }
                                else
                                if( use_task_schema  &&  name == "spooler_id" )  {} // ignorieren
                                else
                                if( use_task_schema  &&  name == "error"      )  {} // ignorieren
                                else
                                if( use_task_schema  &&  name == "error_code" )  error_code = value;
                                else
                                if( use_task_schema  &&  name == "error_text" )  error_text = value;
                                else
                                {
                                    history_entry.setAttribute( name, value );
                                }
                            }
                        }

                        if( use_task_schema  &&  error_text != "" )
                        {
                            Xc x ( error_code.c_str() );
                            x.set_what( error_text );
                            history_entry.appendChild( create_error_element( doc, x ) );
                        }


                        int id = type->field_descr_ptr("id")->as_int( rec.byte_ptr() );


#ifndef SPOOLER_USE_LIBXML2     // libxml2 stürzt in Dump() ab:
                        if( _use_db ) 
                            param_xml = file_as_string( "-binary " + _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -clob=parameters where `id`=" + as_string(id), "" );

                        if( !param_xml.empty() )
                        {
                            try {
                                dom_append_nl( history_element );
                                xml::Document_ptr par_doc;
                                par_doc.create();
                                par_doc.load_xml( param_xml );
                                if( par_doc.documentElement() )  history_entry.appendChild( par_doc.documentElement() );
                            }
                            catch( exception&  x ) { _log->warn( string("History: ") + x.what() ); }
                            catch( const _com_error& x ) { _log->warn( string("History: ") + w_as_string(x.Description() )) ; }
                        }
#endif
                        if( with_log )
                        {
                            try
                            {
                                ta.set_transaction_read();

                                string log = file_as_string( "-binary " GZIP_AUTO + _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -blob=log where \"ID\"=" + as_string(id), "" );
                                if( !log.empty() )  history_entry.append_new_text_element( "log", log );
                            }
                            catch( exception&  x ) { _job->_log->warn( string("History: ") + x.what() ); }
                        }

                        history_element.appendChild( history_entry );
                        dom_append_nl( history_element );
                    }

                    sel.close();
                }
                ta.commit( Z_FUNCTION );
            }
            catch( const _com_error& x )  { throw_com_error( x, "Job_history::read_tail" ); }
        }
        catch( exception& x ) 
        { 
            if( !use_task_schema )  throw x;
            history_element.appendChild( create_error_element( doc, x, 0 ) );
        }
    }

    return history_element;
}

//-----------------------------------------------------------------------Task_history::Task_history

Task_history::Task_history( Job_history* job_history, Task* task )
: 
    _zero_(this+1) 
{ 
    _job_history = job_history; 
    _task        = task;
    _spooler     = _job_history->_spooler; 

    if( _job_history->_extra_type )  _extra_record.construct( _job_history->_extra_type );
}

//-----------------------------------------------------------------------Task_history::Task_history

Task_history::~Task_history()
{
    if( _job_history->_last_task == _task )  _job_history->_last_task = NULL;
}

//----------------------------------------------------------------------Task_history::append_tabbed

void Task_history::append_tabbed( string value )
{
    if( !_tabbed_record.empty() )  _tabbed_record += '\t';

    int i = _tabbed_record.length();

    _tabbed_record += value.substr( 0, max_field_length );

    if( strchr( value.c_str(), '\t' )
     || strchr( value.c_str(), '\n' ) )
    {
        for(; i < _tabbed_record.length(); i++ )  
        {
            if( _tabbed_record[i] == '\t' )  _tabbed_record[i] = ' ';
            if( _tabbed_record[i] == '\n' )  _tabbed_record[i] = ' ';
        }
    }
}

//------------------------------------------------------------------------------Task_history::write

void Task_history::write( bool start )
{
    string parameters;
    
    _job_history->_last_task = _task;

    if( start | _job_history->_use_file )  parameters = _task->has_parameters()? xml_as_string( _task->parameters_as_dom() )
                                                                               : "";

    string start_time = !start || _task->_running_since? _task->_running_since.as_string(Time::without_ms)
                                                       : Time::now().as_string(Time::without_ms);

    while(1)
    {
        try
        {
            if( _job_history->_use_db )
            {
                if( !_spooler->_db->opened() )       // Datenbank ist (wegen eines Fehlers) geschlossen worden?
                {
                    _job_history->close();
                    _job_history->open( (Transaction*)NULL );
                    
                    if( !start )  
                    {
                        _spooler->log()->info( message_string( "SCHEDULER-307" ) );   // "Historiensatz wird wegen vorausgegangen Datenbankfehlers nicht geschrieben"
                        return;
                    }
                }
            }

            if( _job_history->_use_db )
            {
                Transaction ta ( +_spooler->_db );
                {
                    if( start )
                    {
                        sql::Insert_stmt insert ( ta.database_descriptor() );
                        
                        insert.set_table_name( _spooler->_job_history_tablename );
                        
                        insert[ "id"                ] = _task->_id;
                        insert[ "spooler_id"        ] = _spooler->id_for_db();

                        if( _spooler->is_cluster() )
                        insert[ "cluster_member_id" ] = _spooler->cluster_member_id();

                        insert[ "job_name"          ] = _task->job()->path().without_slash();
                        insert[ "cause"             ] = start_cause_name( _task->_cause );
                        insert.set_datetime( "start_time", start_time );
                        
                        if( int pid = _task->pid() )
                        insert[ "pid"               ] = pid;

                        ta.execute( insert, Z_FUNCTION );

                        if( !parameters.empty() )
                        {
                            Any_file blob = ta.open_file( "-out " + _spooler->_db->db_name(), " -table=" + _spooler->_job_history_tablename + " -clob=parameters where \"ID\"=" + as_string( _task->_id ) );
                            blob.put( parameters );
                            blob.close();
                        }
                    }
                    else
                    {
                        sql::Update_stmt update ( _spooler->db()->database_descriptor(), _spooler->_job_history_tablename  );
                        
                        update.and_where_condition( "id", _task->_id );
                        update[ "start_time" ].set_datetime( start_time );
                        update[ "end_time"   ].set_datetime( Time::now().as_string(Time::without_ms) );
                        update[ "steps"      ] = _task->_step_count;
                        update[ "exit_code"  ] = _task->_exit_code;
                        update[ "error"      ] = _task->has_error();

                        if( !_task->_error.code().empty() )  update[ "error_code" ] = _task->_error.code();
                        if( !_task->_error.what().empty() )  update[ "error_text" ] = _task->_error.what().substr( 0, max_column_length );    // Für MySQL 249 statt 250. jz 7.1.04

                        if( _extra_record.type() )
                        {
                            for( int i = 0; i < _extra_record.type()->field_count(); i++ )
                            {
                                if( !_extra_record.null(i) )
                                {
                                    string s = _extra_record.as_string(i);
                                    if( !is_numeric( _extra_record.type()->field_descr_ptr(i)->type_ptr()->info()->_std_type ) )  s = sql_quoted(s);
                                    update [ _job_history->_extra_names[i] ] = s;
                                }
                            }
                        }

                        ta.execute( update, Z_FUNCTION );



                        // Task-Protokoll
                        string log_filename = _task->_log->filename();
                        if( _job_history->_with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
                        {
                            try {
                                ta.set_transaction_written();

                                string blob_filename = _spooler->_db->db_name() + " -table=" + _spooler->_job_history_tablename + " -blob='log' where \"ID\"=" + as_string( _task->_id );
                                if( _job_history->_with_log == arc_gzip )  blob_filename = GZIP + blob_filename;
                                copy_file( "file -b " + log_filename, "-binary " + blob_filename );
                            }
                            catch( exception& x ) { _task->_log->warn( string("History: ") + x.what() ); }
                        }
                    }
                }

                ta.commit( Z_FUNCTION );
            }

            break;
        }
        catch( exception& x )
        {
            _spooler->_db->try_reopen_after_error( x, Z_FUNCTION );
        }
    }

    if( _job_history->_use_file )
    {
        _tabbed_record = "";
        append_tabbed( _task->_id );
        append_tabbed( _spooler->id_for_db() );
        append_tabbed( _task->_job->name() );
        append_tabbed( start_time );
        append_tabbed( start? "" : Time::now().as_string(Time::without_ms) );
        append_tabbed( start_cause_name( _task->_cause ) );
        append_tabbed( _task->_step_count );
        append_tabbed( _task->has_error()? 1 : 0 );
        append_tabbed( _task->_error.code() );
        append_tabbed( _task->_error.what() );
        append_tabbed( parameters );

        if( !start  &&  _extra_record.type() )
        {
            for( int i = 0; i < _extra_record.type()->field_count(); i++ )
            {
                append_tabbed( _extra_record.as_string(i) );
            }
        }

        _job_history->_file.print( _tabbed_record + SYSTEM_NL );
        //zu langsam: _file.syncdata();
    }
}

//------------------------------------------------------------------------------Task_history::start

void Task_history::start()
{
    if( !_job_history->_history_yes )  return;

    if( _task_id == _task->id() )  return;        // start() bereits gerufen
    _task_id = _task->id();

    if( _job_history->_error )  return;

    _start_called = true;


    try
    {
        if( _job_history->_use_file )  _record_pos = _job_history->_file.tell();

        write( true );
    }
    catch( exception& x )  
    { 
        _task->_log->warn( message_string( "SCHEDULER-266", x ) );      // "FEHLER BEIM SCHREIBEN DER HISTORIE: "
        //_error = true;
    }
}

//--------------------------------------------------------------------------------Task_history::end

void Task_history::end()
{
    if( !_job_history->_history_yes )  return;

    if( !_start_called )  return;
    _start_called = false;

    if( _job_history->_error )  return;
    if( !_task )  return;     // Vorsichtshalber

    try
    {
        if( _job_history->_use_file && _job_history->_last_task == _task )  _job_history->_file.seek( _record_pos );

        //if( _job->has_error() )  
        //{
            write( false );
/*
        }
        else
        {
            Z_DEBUG_ONLY( if(_use_file|_use_db) _task->_log->debug9( "Historieneintrag wird wieder gelöscht, weil nicht genug Jobschritte ausgeführt worden sind\n" ); )

            //if( _use_file )  SetEndOfFile();

            if( _use_db )
            {
                Transaction ta ( &_spooler->_db );
                _spooler->_db->execute( "DELETE from " + _spooler->_job_history_tablename + " where \"id\"=" + as_string(_task->_id) );
                ta.commit( Z_FUNCTION );
            }
        }
*/
    }
    catch( exception& x )  
    { 
        _task->_log->warn( message_string( "SCHEDULER-266", x ) );      // "FEHLER BEIM SCHREIBEN DER HISTORIE: "
        //_error = true;
    }

    _task_id = 0;
}

//--------------------------------------------------------------------Task_history::set_extra_field
    
void Task_history::set_extra_field( const string& name, const Variant& value )
{
    if( !_job_history->_history_yes )  return;

    _extra_record.set_field( name, variant_as_string(value) );
}

//-------------------------------------------------------------------------------------------------

} //namespace database
} //namespace scheduler
} //namespace sos

