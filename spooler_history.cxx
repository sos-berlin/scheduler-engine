// $Id$

#include "spooler.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/z_sql.h"
#include "../kram/sleep.h"
#include "../kram/sos_java.h"



using namespace zschimmer;

namespace sos {
namespace spooler {

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
                                       "parameters";

const char history_column_names_db[] = "log";    // Spalten zusätzlich in der Datenbank

const int max_field_length = 1024;      // Das ist die Feldgröße von Any_file -type=(...) für tabulierte Datei.
const int blob_field_size  = 1900;      // Bis zu dieser Größe wird ein Blob im Datensatz geschrieben. ODBC erlaubt nur 2000 Zeichen lange Strings
const int db_error_retry_max = 0;       // Nach DB-Fehler max. so oft die Datenbank neu eröffnen und Operation wiederholen.
const int seconds_before_reopen = 60;   // Solange warten, bis Datenbank nach Fehler erneut geöffnet wird

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

//-------------------------------------------------------------------------Transaction::Transaction

Transaction::Transaction( Spooler_db* db, Transaction* outer_transaction )
: 
    _db(db), 
    _guard(&db->_lock),
    _outer_transaction(outer_transaction)
{
    if( !_outer_transaction )
    {
        _db->rollback();     // Falls irgendeine Transaktion offengeblieben ist
    }
}

//------------------------------------------------------------------------Transaction::~Transaction

Transaction::~Transaction()
{ 
    if( _db  &&  !_outer_transaction )  
    {
        try 
        { 
            rollback();
        } 
        catch( exception& ) {}
    }
}

//------------------------------------------------------------------------------Transaction::commit

void Transaction::commit()
{ 
    if( !_outer_transaction )
    {
        _db->commit();   
    }
     
    _db = NULL;
    _guard.leave(); 
}

//----------------------------------------------------------------------------Transaction::rollback

void Transaction::rollback()
{ 
    _db->rollback(); 
    _db = NULL; 
    _guard.leave(); 

    if( _outer_transaction )  throw_xc( "ROLLBACK-FEHLER", "Rollback in innerer Transaktion setzt die äußere Transaktion zurück" );
}

//--------------------------------------------------------------Transaction::try_reopen_after_error
/*
void Transaction::try_reopen_after_error( exception& x, bool wait_endless )
{
    _db->try_reopen_after_error();
}
*/
//---------------------------------------------------------------------------Spooler_db::Spooler_db

Spooler_db::Spooler_db( Spooler* spooler )
:
    Scheduler_object( spooler, spooler, Scheduler_object::type_database ),
    _zero_(this+1),
    _lock("Spooler_db"),
    _db_descr( z::sql::flag_uppercase_names | z::sql::flag_quote_names | z::sql::flag_dont_quote_table_names )
{
    _log = Z_NEW( Prefix_log( this, "Database" ) );
}

//---------------------------------------------------------------------------------Spooler_db::open

void Spooler_db::open( const string& db_name )
{
    try
    {
        open2( db_name );
    }
    catch( exception& x )
    {
        if( !_spooler->_wait_endless_for_db_open )  throw;
        
        try_reopen_after_error( x, true );
    }
}

//--------------------------------------------------------------------------------Spooler_db::open2

void Spooler_db::open2( const string& db_name )
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

            try
            {
                string stmt;

                _log->info( message_string( "SCHEDULER-907", my_db_name ) );     // Datenbank wird geöffnet

                _db.open( "-in -out " + my_db_name );   // -create

                if( _db.opened() )
                {
                    _db_name += " ";

                    create_table_when_needed( _spooler->_variables_tablename, 
                                            "\"NAME\" varchar(100) not null,"
                                            "\"WERT\" integer,"  
                                            "\"TEXTWERT\" varchar(250),"  
                                            "primary key ( \"name\" )" );


                    vector<string> create_extra = vector_map( sql_quoted_name, vector_split( " *, *", _spooler->_job_history_columns ) );
                    for( int i = 0; i < create_extra.size(); i++ )  create_extra[i] += " varchar(250),";

                    create_table_when_needed( _spooler->_job_history_tablename, 
                                            "\"ID\"          integer not null,"
                                            "\"SPOOLER_ID\"  varchar(100),"
                                            "\"JOB_NAME\"    varchar(100) not null,"
                                            "\"START_TIME\"  datetime not null,"
                                            "\"END_TIME\"    datetime,"
                                            "\"CAUSE\"       varchar(50),"
                                            "\"STEPS\"       integer,"
                                            "\"EXIT_CODE\"   integer,"
                                            "\"ERROR\"       boolean,"
                                            "\"ERROR_CODE\"  varchar(50),"
                                            "\"ERROR_TEXT\"  varchar(250),"
                                            "\"PARAMETERS\"  clob,"
                                            "\"LOG\"         blob," 
                                            + join( "", create_extra ) 
                                            + "primary key( \"ID\" )" );

                    create_table_when_needed( _spooler->_orders_tablename, 
                                            "\"JOB_CHAIN\"   varchar(100) not null,"        // Primärschlüssel
                                            "\"ID\"          varchar(100) not null,"        // Primärschlüssel
                                            "\"SPOOLER_ID\"  varchar(100),"
                                            "\"PRIORITY\"    integer not null,"
                                            "\"STATE\"       varchar(100),"
                                            "\"STATE_TEXT\"  varchar(100),"
                                            "\"TITLE\"       varchar(200),"
                                            "\"CREATED_TIME\" datetime not null,"
                                            "\"MOD_TIME\"    datetime,"
                                            "\"ORDERING\"    integer not null,"             // Um die Reihenfolge zu erhalten
                                            "\"PAYLOAD\"     clob,"
                                            "\"INITIAL_STATE\" varchar(100),"               
                                            "\"RUN_TIME\"    clob,"
                                            "\"ORDER_XML\"   clob,"
                                            "primary key( \"JOB_CHAIN\", \"ID\" )" );


                    add_column( _spooler->_orders_tablename, "INITIAL_STATE" , "add \"INITIAL_STATE\" varchar(100)" );
                    add_column( _spooler->_orders_tablename, "RUN_TIME"      , "add \"RUN_TIME\"      clob" );
                    add_column( _spooler->_orders_tablename, "ORDER_XML"     , "add \"ORDER_XML\"     clob" );
                    add_column( _spooler->_job_history_tablename, "EXIT_CODE", "add \"EXIT_CODE\"     integer" );

                    create_table_when_needed( _spooler->_order_history_tablename, 
                                            "\"HISTORY_ID\"  integer not null,"             // Primärschlüssel
                                            "\"JOB_CHAIN\"   varchar(100) not null,"        // Primärschlüssel
                                            "\"ORDER_ID\"    varchar(100) not null,"
                                            "\"SPOOLER_ID\"  varchar(100),"
                                            "\"TITLE\"       varchar(200),"
                                            "\"STATE\"       varchar(100) not null,"
                                            "\"STATE_TEXT\"  varchar(100),"
                                            "\"START_TIME\"  datetime not null,"
                                            "\"END_TIME\"    datetime not null,"
                                            "\"LOG\"         blob," 
                                            "primary key( \"HISTORY_ID\" )" );

                    create_table_when_needed( _spooler->_tasks_tablename, 
                                            "\"TASK_ID\"        integer not null,"          // Primärschlüssel
                                            "\"SPOOLER_ID\"     varchar(100),"
                                            "\"JOB_NAME\"       varchar(100) not null,"
                                            "\"ENQUEUE_TIME\"   datetime,"
                                            "\"START_AT_TIME\"  datetime,"
                                            "\"PARAMETERS\"     clob,"
                                            "\"TASK_XML\"       clob,"
                                            "primary key( \"TASK_ID\" )" );

                    add_column( _spooler->_tasks_tablename, "TASK_XML", " add \"TASK_XML\" clob" );

                    commit();
                }

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

//---------------------------------------------------------------------------Spooler_db::add_column

void Spooler_db::add_column( const string& table_name, const string& column_name, const string add_clause )
{
    try
    {
        Transaction ta ( this );
        Any_file select ( "-in " + _db_name + " -max-length=1K  SELECT \"" + column_name + "\" from " + table_name + " where 1=0" );
    }
    catch( exception& x )
    {
        Transaction ta ( this );
        
        _log->warn( x.what() );
        string cmd = "ALTER TABLE " + table_name + " " + add_clause;
        _log->info( message_string( "SCHEDULER-908", table_name, column_name, cmd ) );
        
        _db.put( cmd );
        ta.commit();
    }
}

//--------------------------------------------------------------------------------Spooler_db::close

void Spooler_db::close()
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

//-------------------------------------------------------------------Spooler_db::open_history_table

void Spooler_db::open_history_table()
{
    THREAD_LOCK( _lock )
    {
        if( !_history_table.opened() )
        {
            _history_table.open( "-in -out -key=id sql -table=" + _spooler->_job_history_tablename + 
                               //" -sql-fields=(id,spooler_id,job_name,start_time,cause,parameters) | " +       extra-Felder nicht vergessen!
                                 " | " + _db_name + " -ignore=(parameters,log) -max-length=" + as_string(blob_field_size) );
        }
    }
}

//-------------------------------------------------------------Spooler_db::create_table_when_needed

void Spooler_db::create_table_when_needed( const string& tablename, const string& fields )
{
    try
    {
        Transaction ta ( this );        // Select und Create table nicht in derselben Transaktion. Für Access und PostgresQL

        Any_file select;
        select.open( "-in " + _db_name + " SELECT count(*) from " + tablename + " where 1=0" );
        select.get_record();
        select.close();
        // ok
    }
    catch( exception& x )
    {
        _log->warn( x.what() );
        _log->info( message_string( "SCHEDULER-909", tablename ) );

        Transaction ta ( this );
            _db.put( "CREATE TABLE " + tablename + " (" + fields + ") " );
        ta.commit(); 
    }
}

//---------------------------------------------------------------Spooler_db::try_reopen_after_error

void Spooler_db::try_reopen_after_error( exception& x, bool wait_endless )
{
    bool    too_much_errors = false;
    string  warn_msg;


    // Wenn ein TCP-Kommando ausgeführt wird, müssen wir hier sofort raus, sonst wird das Kommando doppelt oder rekursiv aufgerufen werden async_continue_selected(), s.u.
    if( _spooler->_executing_command )  throw;

    if( In_recursion in_recursion = &_waiting )  throw_xc( x );   
    else
    {
        THREAD_LOCK( _error_lock )  _error = x.what();

        _spooler->log()->error( message_string( "SCHEDULER-303", x ) );

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
                    body += "\ndb=" + _spooler->_db_name + "\r\n\r\n" + x.what() + "\r\n\r\nThe Scheduler is trying to open the database again.";
                    //if( !_spooler->_need_db )  body += "\r\nWenn das nicht geht, schreibt der Scheduler die Historie in Textdateien.";

                    Scheduler_event scheduler_event ( Scheduler_event::evt_database_error, log_warn, this );
                    scheduler_event.set_error( x );
                    scheduler_event.set_count( _error_count );

                    Mail_defaults mail_defaults ( _spooler );
                    mail_defaults.set( "subject", S() << "ERROR ON DATABASE ACCESS: " << x.what() );
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
                        warn_msg = message_string( "SCHEDULER-314" );   // "Datenbank lässt sich nicht öffnen. Wegen need_db=strict wird der Scheduler sofort beendet.";
                        break;
                    }

                    _spooler->log()->warn( message_string( "SCHEDULER-958", seconds_before_reopen ) );   // "Eine Minute warten bevor Datenbank erneut geöffnet wird ..."
                    _spooler->_connection_manager->async_continue_selected( is_communication_operation, seconds_before_reopen );
                }
            }
        }


        if( _db.opened() )
        {
            if( _email_sent_after_db_error )    // Zurzeit immer true
            {
                Scheduler_event scheduler_event ( Scheduler_event::evt_database_continue, log_info, this );

                Mail_defaults mail_defaults ( _spooler );

                mail_defaults.set( "subject", "Scheduler is connected again with the database" );
                mail_defaults.set( "body"   , "Scheduler continues processing.\n\ndb=" + _spooler->_db_name );

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
                    string msg = message_string( "SCHEDULER-265", x );     // "SCHEDULER WIRD BEENDET WEGEN FEHLERS BEIM ZUGRIFF AUF DATENBANK"
                    _log->error( msg );

                    Scheduler_event scheduler_event ( Scheduler_event::evt_database_error_abort, log_error, this );
                    scheduler_event.set_error( x );
                    scheduler_event.set_scheduler_terminates( true );

                    Mail_defaults mail_defaults ( _spooler );

                    mail_defaults.set( "subject", msg );
                    mail_defaults.set( "body"   , S() << "db=" << _spooler->_db_name << "\r\n\r\n" << x.what() << "\r\n\r\n" << warn_msg );

                    scheduler_event.send_mail( mail_defaults );
                    

                    _spooler->abort_immediately();
                }
            }

            _spooler->log()->info( message_string( "SCHEDULER-959" ) );   // "Historie wird von Datenbank auf Dateien umgeschaltet" );

            Scheduler_event scheduler_event ( Scheduler_event::evt_database_error_switch_to_file, log_warn, this );
            scheduler_event.set_error( x );

            Mail_defaults mail_defaults( _spooler );
            mail_defaults.set( "subject", string("SCHEDULER CONTINUES WITHOUT DATABASE AFTER ERRORS: ") + x.what() );
            mail_defaults.set( "body"   , S() << "Because of need_db=no\n" "db=" << _spooler->_db_name << "\r\n\r\n" << x.what() << "\r\n\r\n" << warn_msg );
            
            scheduler_event.send_mail( mail_defaults );

            open2( "" );     // Umschalten auf dateibasierte Historie
        }
    }
}

//-------------------------------------------------------------------------------Spooler_db::get_id
// Wird von den Threads gerufen

int Spooler_db::get_id( const string& variable_name, Transaction* outer_transaction )
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
                try_reopen_after_error( x );
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

//------------------------------------------------------------------------------Spooler_db::get_id_
// Wird von den Threads gerufen

int Spooler_db::get_id_( const string& variable_name, Transaction* outer_transaction )
{
    int id;

    if( get_java_vm(false)->running() )  get_java_vm(false)->attach_thread( "" );


    Transaction ta ( this, outer_transaction );
    {
        if( _db.opened() )
        {
            //_job_id_update.execute();    // id++

            //_job_id_select.execute();
            //id = _job_id_select.get_record().as_int(0);
            //_job_id_select.close( close_cursor );

//static int c = 3;  if( --c <= 0 )  throw_xc( "FEHLER" );
            execute( "UPDATE " + _spooler->_variables_tablename + " set \"WERT\" = \"WERT\"+1 where \"NAME\"=" + sql::quoted( variable_name ) );

            Any_file sel;
            sel.open( "-in " + _db_name + "SELECT \"WERT\" from " + _spooler->_variables_tablename + " where \"NAME\"=" + sql::quoted( variable_name ) );
            if( sel.eof() )
            {
                //Any_file sel2 ( "-in " + _db_name + "SELECT max( \"ID\" )  from " + uquoted(_spooler->_job_history_tablename) );
                //Record record = sel2.get_record();
                //id = record.null( 0 )? 1 : record.as_int( 0 );  Fehler in Hostware: record.null(0) liefert immer true
                //sel2.close();

                id = 1;
                execute( "INSERT into " + _spooler->_variables_tablename + " (\"NAME\",\"WERT\") " 
                         "values (" + sql::quoted( variable_name ) + ",'" + as_string(id) + "')" );
            }
            else
            {
                id = sel.get_record().as_int(0);
            }

            LOG( "Spooler_db::get_id(\"" + variable_name + "\") = " << id << '\n' );

            _id_counters[ variable_name ] = id + 1;
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

        ta.commit();
    }

    return id;
}

//------------------------------------------------------------------------------Spooler_db::execute

void Spooler_db::execute( const string& stmt )
{ 
    THREAD_LOCK( _lock )
    {
        LOGI( "Spooler_db::execute  " << stmt << '\n' );
        _db.put( stmt ); 
    }
}

//-------------------------------------------------------------------------------Spooler_db::commit

void Spooler_db::commit()
{
    THREAD_LOCK( _lock )
    {
        if( _db.opened() )  execute( "COMMIT" );
    }
}

//-----------------------------------------------------------------------------Spooler_db::rollback

void Spooler_db::rollback()
{
    THREAD_LOCK( _lock )
    {
        if( _db.opened() )  execute( "ROLLBACK" );
    }
}   

//------------------------------------------------------------------------Spooler_db::spooler_start

void Spooler_db::spooler_start()
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
                    execute( "INSERT into " + _spooler->_job_history_tablename + " (\"ID\",\"SPOOLER_ID\",\"JOB_NAME\",\"START_TIME\") "
                             "values (" + as_string(_id) + "," + sql_quoted(_spooler->id_for_db()) + ",'(Spooler)',{ts'" + Time::now().as_string(Time::without_ms) + "'})" );
                    ta.commit();
                }
            }
        }
        //catch( exception& x )  
        //{ 
        //    _log->warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        //}
    }
}

//-------------------------------------------------------------------------Spooler_db::spooler_stop

void Spooler_db::spooler_stop()
{
    if( _db.opened() )
    {
        try
        {
            Transaction ta ( this );
            {
                execute( "UPDATE " + _spooler->_job_history_tablename + "  set \"END_TIME\"={ts'" + Time::now().as_string(Time::without_ms) + "'} "
                         "where \"ID\"=" + as_string(_id) );
                ta.commit();
            }
        }
        catch( exception& x )  
        { 
            _log->warn( message_string( "SCHEDULER-306", _spooler->_job_history_tablename, x ) ); 
        }
    }
}

//-------------------------------------------------------------------------Spooler_db::insert_order

void Spooler_db::insert_order( Order* order )
{
    try
    {
        while(1)
        {
            if( !_db.opened() )  return;

            try
            {
                Transaction ta ( this );
                {
                    delete_order( order, &ta );

                    sql::Insert_stmt insert ( &_db_descr );
                    
                    insert.set_table_name( _spooler->_orders_tablename );
                    
                    insert[ "ordering"      ] = get_order_ordering( &ta );
                    insert[ "job_chain"     ] = order->job_chain()->name();
                    insert[ "id"            ] = order->id().as_string();
                    insert[ "spooler_id"    ] = _spooler->id_for_db();
                    insert[ "title"         ] = order->title()                     , order->_title_modified      = false;
                    insert[ "state"         ] = order->state().as_string();
                    insert[ "state_text"    ] = order->state_text()                , order->_state_text_modified = false;
                    insert[ "priority"      ] = order->priority()                  , order->_priority_modified   = false;
                    insert.set_datetime( "created_time", order->_created.as_string(Time::without_ms) );
                    insert.set_datetime( "mod_time", Time::now().as_string(Time::without_ms) );

                    if( order->run_time() )
                    insert[ "run_time"      ] = order->run_time()->dom_document().xml();     //, order->_run_time_modified    = false;

                    insert[ "initial_state" ] = order->initial_state().as_string();

                    execute( insert );


                    string payload_string = order->payload().as_string();
                    if( payload_string != "" )  update_payload_clob( order->id().as_string(), payload_string );
                    //order->_payload_modified = false;

                    xml::Document_ptr order_document = order->dom( show_for_database_only );
                    xml::Element_ptr  order_element  = order_document.documentElement();
                    if( order_element.hasAttributes()  ||  order_element.firstChild() )
                        update_clob( _spooler->_orders_tablename, "order_xml", "id", order->id().as_string(), order_document.xml() );

                    ta.commit();
                }

                order->_is_in_database = true;

                break;
            }
            catch( exception& x )  
            { 
                try_reopen_after_error( x );
            }
        }
    }
    catch( exception& x ) 
    { 
        _spooler->log()->error( message_string( "SCHEDULER-305", _spooler->_orders_tablename, x ) );        // "FEHLER BEIM EINFÜGEN IN DIE TABELLE "
        throw;
    }
}

//------------------------------------------------------------------Spooler_db::update_payload_clob

void Spooler_db::update_payload_clob( const string& order_id, const string& payload_string )
{
    if( payload_string == "" )
    {
        sql::Update_stmt update ( &_db_descr );
        update.set_table_name( _spooler->_orders_tablename );
        update[ "payload" ].set_direct( "null" );
        update.and_where_condition( "id", order_id );
        execute( update );
    }
    else
    {
        Any_file clob ( "-out " + _db_name + " -table=" + _spooler->_orders_tablename + " -clob=payload  where `id`=" + sql::quoted( order_id ) );
        clob.put( payload_string );
        clob.close();
    }
}

//--------------------------------------------------------------------Spooler_db::read_payload_clob

string Spooler_db::read_payload_clob( const string& order_id )
{
    return file_as_string( _db_name + " -table=" + _spooler->_orders_tablename + " -clob=payload"
                           "  where `id`=" + sql::quoted( order_id ) );
}

//-------------------------------------------------------------------------------------------------

string Spooler_db::read_orders_runtime_clob( const string& order_id )
{
    return file_as_string( _db_name + " -table=" + _spooler->_orders_tablename + " -clob=run_time"
                           "  where `id`=" + sql::quoted( order_id ) );
}

//---------------------------------------------------------------Spooler_db::update_orders_xml_clob

void Spooler_db::update_clob( const string& table_name, const string& column_name, const string& key_name, int key_value, const string& value )
{
    if( value == "" )
    {
        sql::Update_stmt update ( &_db_descr );
        update.set_table_name( table_name );
        update[ column_name ].set_direct( "null" );
        update.and_where_condition( key_name, key_value );
        execute( update );
    }
    else
    {
        Any_file clob ( "-out " + _db_name + " -table=" + table_name + " -clob=" + column_name + " where `" + key_name + "`=" + as_string( key_value ) );
        clob.put( value );
        clob.close();
    }
}

//---------------------------------------------------------------Spooler_db::update_orders_xml_clob

void Spooler_db::update_clob( const string& table_name, const string& column_name, const string& key_name, const string& key_value, const string& value )
{
    if( value == "" )
    {
        sql::Update_stmt update ( &_db_descr );
        update.set_table_name( table_name );
        update[ column_name ].set_direct( "null" );
        update.and_where_condition( key_name, key_value );
        execute( update );
    }
    else
    {
        Any_file clob ( "-out " + _db_name + " -table=" + table_name + " -clob=" + column_name + " where `" + key_name + "`=" + sql::quoted( key_value ) );
        clob.put( value );
        clob.close();
    }
}

//-------------------------------------------------------------------------------------------------

string Spooler_db::read_clob( const string& table_name, const string& column_name, const string& key_name, const string& key_value )
{
    return file_as_string( _db_name + " -table=" + table_name + " -clob=" + column_name + "  where `" + key_name + "`=" + sql::quoted( key_value ) );
}

//-------------------------------------------------------------------------Spooler_db::delete_order

void Spooler_db::delete_order( Order* order, Transaction* )
{
    sql::Delete_stmt del ( &_db_descr );

    del.set_table_name( _spooler->_orders_tablename );

    del.and_where_condition( "job_chain" , order->job_chain()->name() );
    del.and_where_condition( "id"        , order->id().as_string() );
    del.and_where_condition( "spooler_id", _spooler->id_for_db() );

    execute( del );
}

//-------------------------------------------------------------------------Spooler_db::finish_order

void Spooler_db::finish_order( Order* order, Transaction* outer_transaction )
{
    while(1)
    {
        try
        {
            if( !_db.opened() )  return;


            Transaction ta ( this, outer_transaction );
            {
                if( order->_is_in_database )
                {
                    delete_order( order, &ta );
                    order->_is_in_database = false;
                }

                write_order_history( order, &ta );
            }

            ta.commit();
            break;
        }
        catch( exception& x )  
        { 
            if( outer_transaction )  throw;
            try_reopen_after_error( x );
        }
    }
}

//------------------------------------------------------------------Spooler_db::write_order_history

void Spooler_db::write_order_history( Order* order, Transaction* outer_transaction )
{
    if( !order->start_time() )  return;


    while(1)
    {
        if( !_db.opened() )  return;

        try
        {
            Transaction ta ( this, outer_transaction );

            int              history_id = get_order_history_id( &ta );
            sql::Insert_stmt insert     ( &_db_descr );

            insert.set_table_name( _spooler->_order_history_tablename );
            
            insert[ "history_id" ] = history_id;
            insert[ "job_chain"  ] = order->job_chain()->name();
            insert[ "order_id"   ] = order->id().as_string();
            insert[ "title"      ] = order->title();
            insert[ "state"      ] = order->state().as_string();
            insert[ "state_text" ] = order->state_text();
            insert[ "spooler_id" ] = _spooler->id_for_db();
            insert.set_datetime( "start_time", order->start_time().as_string(Time::without_ms) );

            if( order->end_time() )
            insert.set_datetime( "end_time"  , order->end_time().as_string(Time::without_ms) );

            execute( insert );


            // Auftragsprotokoll
            string log_filename = order->log()->filename();

            if( _spooler->_order_history_with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
            {
                try 
                {
                    string blob_filename = db_name() + " -table=" + _spooler->_order_history_tablename + " -blob=log where \"HISTORY_ID\"=" + as_string( history_id );
                    if( _spooler->_order_history_with_log == arc_gzip )  blob_filename = GZIP + blob_filename;
                    copy_file( "file -b " + log_filename, blob_filename );
                }
                catch( exception& x ) 
                { 
                    _log->warn( message_string( "SCHEDULER-267", _spooler->_order_history_tablename, x.what() ) );      // "FEHLER BEIM SCHREIBEN DES LOGS IN DIE TABELLE "
                }
            }

            ta.commit();
            break;
        }
        catch( exception& x )  
        { 
            if( outer_transaction )  throw;
            try_reopen_after_error( x );
        }
    }
}

//-------------------------------------------------------------------------Spooler_db::update_order

void Spooler_db::update_order( Order* order )
{
    if( order->finished() )  
    {
        finish_order( order );
    }
    else
    {
        string payload_string = order->string_payload();
        string state_string   = order->state().as_string();

        try
        {
            while(1)
            {
                if( !_db.opened() )  return;

                try
                {
                    Transaction ta ( this );
                    {
                        sql::Update_stmt update ( &_db_descr );

                        update.set_table_name( _spooler->_orders_tablename );

                        update[ "state" ] = state_string;
                        
                        if( order->_priority_modified   )  update[ "priority"   ] = order->priority()           ,  order->_state_text_modified = false;
                        if( order->_title_modified      )  update[ "title"      ] = order->title()              ,  order->_title_modified      = false;
                        if( order->_state_text_modified )  update[ "state_text" ] = order->state_text()         ,  order->_state_text_modified = false;

                        if( order->run_time() )  update[ "run_time" ] = order->run_time()->dom_document().xml();
                                           else  update[ "run_time" ].set_direct( "null" );

                        update[ "initial_state" ] = order->initial_state().as_string();

                        update.set_datetime( "mod_time", Time::now().as_string(Time::without_ms) );

                        //if( order->_payload_modified )
                        {
                            if( payload_string == "" )  update[ "payload" ].set_direct( "null" );
                                                  else  update_payload_clob( order->id().as_string(), payload_string );
                            //order->_payload_modified = false;
                        }
    

                        update.and_where_condition( "job_chain", order->job_chain()->name() );
                        update.and_where_condition( "id"       , order->id().as_string()    );

                        execute( update );

                        ta.commit();
                    }

                    break;
                }
                catch( exception& x )  
                { 
                    try_reopen_after_error( x );
                }
            }
        }
        catch( exception& x ) 
        { 
            _spooler->log()->error( message_string( "SCHEDULER-306", _spooler->_orders_tablename, x ) );      // "FEHLER BEIM UPDATE DER TABELLE "
            throw;
        }
    }
}

//----------------------------------------------------------------------------Spooler_db::read_task
// Die XML-Struktur ist wie Task::dom_element(), nicht wie Job_history::read_tail()

xml::Element_ptr Spooler_db::read_task( const xml::Document_ptr& doc, int task_id, const Show_what& show )
{
    if( !opened() )  z::throw_xc( "SCHEDULER-184" );

    xml::Element_ptr task_element = doc.createElement( "task" );

    try
    {
        Transaction ta ( this );
        {
            Any_file sel ( "-in " + _spooler->_db->db_name() + 
                            "select \"SPOOLER_ID\", \"JOB_NAME\", \"START_TIME\", \"END_TIME\", \"CAUSE\", \"STEPS\", \"ERROR\", \"ERROR_CODE\", \"ERROR_TEXT\" " +
                            "  from " + _spooler->_job_history_tablename  +
                            "  where \"ID\"=" + as_string(task_id) );
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

            if( record.as_int( "ERROR" ) )  
            {
                xml::Element_ptr error_element = doc.createElement( "ERROR" );
                error_element.setAttribute( "code", record.as_string( "ERROR_CODE" ) );
                error_element.setAttribute( "text", record.as_string( "ERROR_TEXT" ) );
                task_element.appendChild( error_element );
            }

            if( show & show_log )
            {
                try
                {
                    string log = file_as_string( GZIP_AUTO + _spooler->_db->db_name() + " -table=" + _spooler->_job_history_tablename + " -blob=\"LOG\"" 
                                                " where \"ID\"=" + as_string(task_id) );
                    dom_append_text_element( task_element, "log", log );
                }
                catch( exception& x ) { _log->warn( message_string( "SCHEDULER-268", task_id, x ) ); }  // "FEHLER BEIM LESEN DES LOGS FÜR TASK "
            }
        }
    }
    catch( const _com_error& x ) { throw_com_error( x, "Spooler_db::read_task" ); }

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

void Job_history::open()
{
    string section = _job->profile_section();

    _job_name = _job->name();            // Damit read_tail() nicht mehr auf Job zugreift (das ist ein anderer Thread)

    try
    {
        _filename   = read_profile_string            ( _spooler->_factory_ini, section, "history_file" );
        _history_yes= read_profile_bool              ( _spooler->_factory_ini, section, "history"           , _spooler->_job_history_yes );
        _on_process = read_profile_history_on_process( _spooler->_factory_ini, section, "history_on_process", _spooler->_job_history_on_process );

        if( !_history_yes )  return;

        if( _spooler->_db->opened()  &&  _filename == "" )
        {
            Transaction ta ( +_spooler->_db );
            {
                _with_log = read_profile_with_log( _spooler->_factory_ini, section, "history_with_log", _spooler->_job_history_with_log );

                set<string> my_columns = set_map( lcase, set_split( ", *", replace_regex( string(history_column_names) + "," + history_column_names_db, ":[^,]+", "" ) ) );

                _spooler->_db->open_history_table();

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
            ta.commit();

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
                _filename += ".job." + _job_name + ".txt";
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

void Job_history::archive( Archive_switch arc, const string& filename )
{
    if( file_exists( filename ) )
    {
        string ext   = extension_of_path( filename );
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
    bool with_log = ( show & show_log ) != 0;

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
                if( !_history_yes )  z::throw_xc( "SCHEDULER-141", _job_name );

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
                    //string prefix = ( next < 0? "-in -seq head -" : "-in -seq tail -reverse -" ) + as_string(max(1,abs(next))) + " | ";
                        string prefix = "-in -seq head -" + as_string(max(1,abs(next))) + " | ";
                        string clause = " where \"JOB_NAME\"=" + sql_quoted(_job_name);

                        clause += " and \"SPOOLER_ID\"=" + sql_quoted( _spooler->id_for_db() );
                        
                        if( id != -1 )
                        {
                            clause += " and \"ID\"";
                            clause += next < 0? "<" : 
                                    next > 0? ">" 
                                            : "=";
                            clause += as_string(id);
                        }

                        clause += " order by \"ID\" ";  if( next < 0 )  clause += " desc";
                        
                        sel.open( prefix + _spooler->_db->_db_name + 
                                " select " + 
                                ( next == 0? "" : "%limit(" + as_string(abs(next)) + ") " ) +
                                " \"ID\", \"SPOOLER_ID\", \"JOB_NAME\", \"START_TIME\", \"END_TIME\", \"CAUSE\", \"STEPS\", \"ERROR\", \"ERROR_CODE\", \"ERROR_TEXT\" " +
                                join( "", vector_map( quote_and_prepend_comma, _extra_names ) ) +
                                " from " + _spooler->_job_history_tablename + 
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
                            param_xml = file_as_string( _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -clob=parameters where `id`=" + as_string(id), "" );

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
                                string log = file_as_string( GZIP_AUTO + _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -blob=log where \"ID\"=" + as_string(id), "" );
                                if( !log.empty() ) dom_append_text_element( history_entry, "log", log );
                            }
                            catch( exception&  x ) { _job->_log->warn( string("History: ") + x.what() ); }
                        }

                        history_element.appendChild( history_entry );
                        dom_append_nl( history_element );
                    }

                    sel.close();
                }
                ta.commit();
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
                    _job_history->open();
                    
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
                        sql::Insert_stmt insert ( &_spooler->_db->_db_descr );
                        
                        insert.set_table_name( _spooler->_job_history_tablename );
                        
                        insert[ "id"         ] = _task->_id;
                        insert[ "spooler_id" ] = _spooler->id_for_db();
                        insert[ "job_name"   ] = _task->job()->name();
                        insert[ "cause"      ] = start_cause_name( _task->_cause );
                        insert.set_datetime( "start_time", start_time );

                        _spooler->_db->execute( insert );
                        /*
                        Record record = _spooler->_db->_history_table.create_record();

                        record.set_field( "id"             , _task->_id );
                        record.set_field( "spooler_id"     , _spooler->id_for_db() );
                        record.set_field( "job_name"       , _task->job()->name() );
                        record.set_field( "start_time"     , start_time );
                        record.set_field( "cause"          , start_cause_name( _task->_cause ) );

                        //if( !parameters.empty()  &&  parameters.length() < blob_field_size )  record.set_field( "parameters", parameters ), parameters = "";

                        _spooler->_db->_history_table.insert( record );
                        */

                        if( !parameters.empty() )
                        {
                            Any_file blob;
                            blob.open( "-out " + _spooler->_db->db_name() + " -table=" + _spooler->_job_history_tablename + " -clob=parameters where \"ID\"=" + as_string( _task->_id ) );
                            blob.put( parameters );
                            blob.close();
                        }
                    }
                    else
                    {
        /*
                        _spooler->_db->_history_update_params[1] = Time::now().as_string(Time::without_ms);
                        _spooler->_db->_history_update_params[2] = _task->_step_count;
                        _spooler->_db->_history_update_params[3] = _job->has_error()? 1 : 0;
                        _spooler->_db->_history_update_params[4] = _job->_error.code();
                        _spooler->_db->_history_update_params[5] = _job->_error.what().substr( 0, 250 );
                        _spooler->_db->_history_update_params[6] = _task->_id;
                        _spooler->_db->_history_update.execute();
        */
                        string stmt = "UPDATE " + _spooler->_job_history_tablename + "  set ";
                        stmt +=   "\"START_TIME\"={ts'" + start_time + "'}";
                        stmt += ", \"END_TIME\"={ts'" + Time::now().as_string(Time::without_ms) + "'}";
                        stmt += ", \"STEPS\"=" + as_string( _task->_step_count );
                        stmt += ", \"EXIT_CODE\"=" + as_string( _task->_exit_code );
                        stmt += ", \"ERROR\"=" + as_string( _task->has_error() );
                        if( !_task->_error.code().empty() ) stmt += ", \"ERROR_CODE\"=" + sql_quoted( _task->_error.code() );
                        if( !_task->_error.what().empty() ) stmt += ", \"ERROR_TEXT\"=" + sql_quoted( _task->_error.what().substr( 0, 249 ) );    // Für MySQL 249 statt 250. jz 7.1.04

                        if( _extra_record.type() )
                        {
                            for( int i = 0; i < _extra_record.type()->field_count(); i++ )
                            {
                                if( !_extra_record.null(i) )
                                {
                                    string s = _extra_record.as_string(i);
                                    if( !is_numeric( _extra_record.type()->field_descr_ptr(i)->type_ptr()->info()->_std_type ) )  s = sql_quoted(s);
                                    stmt += ", " + sql_quoted_name( _job_history->_extra_names[i] ) + "=" + s;
                                }
                            }
                        }

                        stmt += " where `id`=" + as_string( _task->_id );
                        _spooler->_db->execute( stmt );


                        // Task-Protokoll
                        string log_filename = _task->_log->filename();
                        if( _job_history->_with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
                        {
                            try {
                                string blob_filename = _spooler->_db->db_name() + " -table=" + _spooler->_job_history_tablename + " -blob='log' where \"ID\"=" + as_string( _task->_id );
                                if( _job_history->_with_log == arc_gzip )  blob_filename = GZIP + blob_filename;
                                copy_file( "file -b " + log_filename, blob_filename );
                            }
                            catch( exception& x ) { _task->_log->warn( string("History: ") + x.what() ); }
                        }
                    }
                }

                ta.commit();
            }

            break;
        }
        catch( exception& x )
        {
            _spooler->_db->try_reopen_after_error( x );
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
                ta.commit();
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

} //namespace spooler
} //namespace sos

