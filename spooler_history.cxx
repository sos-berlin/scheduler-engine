// $Id: spooler_history.cxx,v 1.52 2003/07/29 11:20:48 jz Exp $

#include "spooler.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/z_sql.h"
#include "../kram/sleep.h"
#include "../kram/sos_java.h"


#ifdef Z_HPUX
#   define GZIP_AUTO ""   // gzip -auto liefert ZLIB_STREAM_ERROR mit gcc 3.1, jz 7.5.2003
#else
#   define GZIP_AUTO "gzip -auto | "
#endif


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
const int db_error_retry_max = 5;       // Nach DB-Fehler max. so oft die Datenbank neu eröffnen und Operation wiederholen.

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

static string uquoted( const string& value ) 
{ 
    return quoted_string( ucase( value ), '\"', '\"' ); 
}

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

//------------------------------------------------------------------------------------prepend_comma

static string prepend_comma( const string& s )  
{ 
    return ", " + s; 
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
        catch( const exception& ) {}
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

//---------------------------------------------------------------------------Spooler_db::Spooler_db

Spooler_db::Spooler_db( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler)
{
}

//---------------------------------------------------------------------------------Spooler_db::open

void Spooler_db::open( const string& db_name )
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
                    if( _spooler->_need_db )  throw_xc( "SPOOLER-142", _db_name );
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

                _spooler->_log.info( "Datenbank wird geöffnet: " + my_db_name );

                _db.open( "-in -out " + my_db_name );   // -create

                _db_name += " ";

                create_table_when_needed( _spooler->_variables_tablename, 
                                          "\"NAME\" char(100) not null,"
                                          "\"WERT\" char(250),"  
                                          "primary key ( \"name\" )" );


                vector<string> create_extra = vector_map( sql_quoted_name, vector_split( " *, *", _spooler->_job_history_columns ) );
                for( int i = 0; i < create_extra.size(); i++ )  create_extra[i] += " char(250),";

                create_table_when_needed( _spooler->_job_history_tablename, 
                                          "\"ID\"          integer not null,"
                                          "\"SPOOLER_ID\"  char(100),"
                                          "\"JOB_NAME\"    char(100) not null,"
                                          "\"START_TIME\"  datetime not null,"
                                          "\"END_TIME\"    datetime,"
                                          "\"CAUSE\"       char(50),"
                                          "\"STEPS\"       integer,"
                                          "\"ERROR\"       bit,"
                                          "\"ERROR_CODE\"  char(50),"
                                          "\"ERROR_TEXT\"  char(250),"
                                          "\"PARAMETERS\"  clob,"
                                          "\"LOG\"         blob," 
                                          + join( "", create_extra ) 
                                          + "primary key( \"ID\" )" );

                create_table_when_needed( _spooler->_orders_tablename, 
                                          "\"JOB_CHAIN\"   char(100) not null,"         // Primärschlüssel
                                          "\"ID\"          char(100) not null,"         // Primärschlüssel
                                          "\"SPOOLER_ID\"  char(100),"
                                          "\"PRIORITY\"    integer not null,"
                                          "\"STATE\"       char(100),"
                                          "\"STATE_TEXT\"  char(100),"
                                          "\"TITLE\"       char(200),"
                                          "\"CREATED_TIME\" datetime not null,"
                                          "\"MOD_TIME\"    datetime,"
                                          "\"ORDERING\"    integer,"                    // Um die Reihenfolge zu erhalten
                                          "\"PAYLOAD\"     blob,"
                                          "primary key( \"JOB_CHAIN\", \"ID\" )" );

                create_table_when_needed( _spooler->_order_history_tablename, 
                                          "\"HISTORY_ID\"  integer not null,"           // Primärschlüssel
                                          "\"JOB_CHAIN\"   char(100) not null,"         // Primärschlüssel
                                          "\"ORDER_ID\"    char(100) not null,"
                                          "\"SPOOLER_ID\"  char(100),"
                                          "\"TITLE\"       char(200) not null,"
                                          "\"STATE\"       varchar(100),"
                                          "\"STATE_TEXT\"  varchar(100) not null,"
                                          "\"START_TIME\"  datetime not null,"
                                          "\"END_TIME\"    datetime not null,"
                                          "\"LOG\"         blob," 
                                          "primary key( \"HISTORY_ID\" )" );

              //stmt = "UPDATE " + uquoted(_spooler->_variables_tablename) + " set \"WERT\" = \"WERT\"+1 where \"NAME\"='spooler_job_id'";
              //_job_id_update.prepare( _db_name + stmt );


              //stmt = "SELECT \"WERT\" from " + _spooler->_variables_tablename + " where \"NAME\"='spooler_job_id'";
              //_job_id_select.prepare( "-in " + _db_name + stmt );

              //_job_id_select.execute();
              //if( _job_id_select.eof() )  execute( "INSERT into " + uquoted(_spooler->_variables_tablename) + " (\"NAME\",\"WERT\") values ('spooler_job_id','0')" );
              //_job_id_select.close( close_cursor );
                commit();

                //stmt = "UPDATE " + _spooler->_job_history_tablename + " set \"end\"=?, steps=?, \"error\"=?, error_code=?, error_text=?  where \"id\"=?";
                //_history_update.prepare( _db_name + stmt );       //            1        2            3             4             5               6
                //_history_update_params.resize( 1+6 );
                //for( int i = 1; i <= 6; i++ )  _history_update.bind_parameter( i, &_history_update_params[i] );

                _email_sent_after_db_error = false;
            }
            catch( const exception& x )  
            { 
                close();

                if( _spooler->_need_db )  throw;
            
                _spooler->_log.warn( string("FEHLER BEIM ÖFFNEN DER HISTORIENDATENBANK: ") + x.what() ); 
            }
        }
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

        _db.close();
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
                                 " | " + _db_name + " -ignore=(log) -max-length=" + as_string(blob_field_size) );
        }
    }
}

//-------------------------------------------------------------Spooler_db::create_table_when_needed

void Spooler_db::create_table_when_needed( const string& tablename, const string& fields )
{
    Transaction ta = this;

    try
    {
        Any_file select;
        select.open( "-in " + _db_name + " SELECT count(*) from " + uquoted(tablename) );
        select.get_record();
        select.close();
        // ok
    }
    catch( const exception& x )
    {
        _spooler->_log.warn( x.what() );
        _spooler->_log.info( "Tabelle " + tablename + " wird eingerichtet" );
        _db.put( "CREATE TABLE " + uquoted(tablename) + " (" + fields + ") " );
    }

    ta.commit();        // Für select und für create table (jedenfalls bei Jet)
}

//---------------------------------------------------------------Spooler_db::try_reopen_after_error

void Spooler_db::try_reopen_after_error( const exception& x )
{
    _spooler->log().error( string("FEHLER BEIM ZUGRIFF AUF DATENBANK: ") + x.what() );
    _spooler->log().info( "Datenbank wird geschlossen" );

    if( !_email_sent_after_db_error )
    {
        string body = "db=" + _spooler->_db_name + "\r\n\r\n" + x.what() + "\r\n\r\nDer Spooler versucht, die Datenbank erneut zu oeffnen.";
        if( !_spooler->_need_db )  body += "\r\nWenn das nicht geht, schreibt der Spooler die Historie in Textdateien.";
        _spooler->send_error_email( string("FEHLER BEIM ZUGRIFF AUF DATENBANK: ") + x.what(), body );
        _email_sent_after_db_error = true;
    }

    Z_MUTEX( _lock )
    {
        //try
        //{
            close();
        //}
        //catch( const xception& x ) { _log->warn(" FEHLER BEIM SCHLIESSEN DER DATENBANK: " + x.what() ); }

        while(1)
        {
            try
            {
                open( _spooler->_db_name );
                open_history_table();
                break;
            }
            catch( const exception& x )
            {
                _spooler->log().warn( x.what() );

                if( !_spooler->_need_db )  break;

                sos_sleep( 30 );
            }
        }

        if( !_db.opened() )
        {
            _spooler->log().info( "Historie wird von Datenbank auf Dateien umgeschaltet" );
            open( "" );     // Umschalten auf dateibasierte Historie
        }
    }
}

//-------------------------------------------------------------------------------Spooler_db::get_id
// Wird von den Threads gerufen

int Spooler_db::get_id( const string& variable_name, Transaction* outer_transaction )
{
    int  retry_count = db_error_retry_max;
    int  id;

    while(1)
    {
        try
        {
            id = get_id_( variable_name, outer_transaction );
            break;
        }
        catch( const exception& x )
        {
            if( --retry_count <= 0 )  throw;
            try_reopen_after_error( x );
        }
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

            execute( "UPDATE " + uquoted(_spooler->_variables_tablename) + " set \"WERT\" = \"WERT\"+1 where \"NAME\"=" + sql::quoted( variable_name ) );

            Any_file sel;
            sel.open( "-in " + _db_name + "SELECT \"WERT\" from " + uquoted(_spooler->_variables_tablename) + " where \"NAME\"=" + sql::quoted( variable_name ) );
            if( sel.eof() )
            {
                id = 1;
                execute( "INSERT into " + uquoted(_spooler->_variables_tablename) + " (\"NAME\",\"WERT\") " 
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
        LOG( "Spooler_db::execute  " << stmt << '\n' );
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
        try
        {
            _id = get_task_id();     // Der Spooler-Satz hat auch eine Id
     
            Transaction ta = this;
            {
                execute( "INSERT into " + uquoted(_spooler->_job_history_tablename) + " (\"ID\",\"SPOOLER_ID\",\"JOB_NAME\",\"START_TIME\") "
                         "values (" + as_string(_id) + "," + sql_quoted(_spooler->id_for_db()) + ",'(Spooler)',{ts'" + Time::now().as_string(Time::without_ms) + "'})" );
                ta.commit();
            }
        }
        catch( const exception& x )  
        { 
            _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        }
    }
}

//-------------------------------------------------------------------------Spooler_db::spooler_stop

void Spooler_db::spooler_stop()
{
    if( _db.opened() )
    {
        try
        {
            Transaction ta = this;
            {
                execute( "UPDATE " + uquoted(_spooler->_job_history_tablename) + " set end_time={ts'" + Time::now().as_string(Time::without_ms) + "'} "
                         "where id=" + as_string(_id) );
                ta.commit();
            }
        }
        catch( const exception& x )  
        { 
            _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        }
    }
}

//-------------------------------------------------------------------------Spooler_db::insert_order

void Spooler_db::insert_order( Order* order )
{
    int  retry_count = db_error_retry_max;

    while(1)
    {
        if( !_db.opened() )  return;

        try
        {
            Transaction ta = this;
            {
                delete_order( order, &ta );

                {
                    sql::Insert_stmt insert;
                    
                    insert.set_table_name( _spooler->_orders_tablename );
                   
                    insert[ "job_chain"  ] = order->job_chain()->name();
                    insert[ "id"         ] = order->id().as_string();
                    insert[ "spooler_id" ] = _spooler->id_for_db();
                    insert[ "title"      ] = order->title()                     , order->_title_modified      = false;
                    insert[ "state"      ] = order->state().as_string();
                    insert[ "state_text" ] = order->state_text()                , order->_state_text_modified = false;
                    insert[ "priority"   ] = order->priority()                  , order->_priority_modified   = false;
                    insert[ "payload"    ] = order->payload().as_string()       , order->_payload_modified    = false;
                    insert[ "ordering"   ] = get_order_ordering( &ta );
                    insert.set_datetime( "created_time", order->_created.as_string(Time::without_ms) );
                    insert.set_datetime( "mod_time", Time::now().as_string(Time::without_ms) );

                    execute( insert );
                }

                ta.commit();
            }

            break;
        }
        catch( const exception& x )  
        { 
            if( --retry_count <= 0 )  throw;
            try_reopen_after_error( x );
        }
    }
}

//-------------------------------------------------------------------------Spooler_db::delete_order

void Spooler_db::delete_order( Order* order, Transaction* transaction )
{
    sql::Delete_stmt del;

    del.set_table_name( _spooler->_orders_tablename );

    del.add_where_cond( "job_chain", order->job_chain()->name() );
    del.add_where_cond( "id"       , order->id().as_string() );

    execute( del );
}

//-------------------------------------------------------------------------Spooler_db::update_order

void Spooler_db::update_order( Order* order )
{
    int  retry_count = db_error_retry_max;

    while(1)
    {
        if( !_db.opened() )  return;

        try
        {
            Transaction ta = this;
            {
                if( order->finished() )
                {
                    delete_order( order, &ta );
                    write_order_history( order, &ta );
                }
                else
                {
                    sql::Update_stmt update;

                    update.set_table_name( _spooler->_orders_tablename );

                    update[ "state" ] = order->state().as_string();
                    
                    if( order->_priority_modified   )  update[ "priority"   ] = order->priority()           ,  order->_state_text_modified = false;
                    if( order->_title_modified      )  update[ "title"      ] = order->title()              ,  order->_title_modified      = false;
                    if( order->_state_text_modified )  update[ "state_text" ] = order->state_text()         ,  order->_state_text_modified = false;
                    if( order->_payload_modified    )  update[ "payload"    ] = order->payload().as_string(),  order->_payload_modified    = false;

                    update.set_datetime( "mod_time", Time::now().as_string(Time::without_ms) );

                    update.add_where_cond( "job_chain", order->job_chain()->name() );
                    update.add_where_cond( "id"       , order->id().as_string()    );

                    execute( update );
                }

                ta.commit();
            }

            break;
        }
        catch( const exception& x )  
        { 
            if( --retry_count <= 0 )  throw;
            try_reopen_after_error( x );
        }
    }
}

//------------------------------------------------------------------Spooler_db::write_order_history

void Spooler_db::write_order_history( Order* order, Transaction* outer_transaction )
{
    try
    {
        Transaction ta ( this, outer_transaction );

        int history_id = get_order_history_id( &ta );

        {
            sql::Insert_stmt insert;
            
            insert.set_table_name( _spooler->_order_history_tablename );
            
            insert[ "history_id" ] = history_id;
            insert[ "job_chain"  ] = order->job_chain()->name();
            insert[ "order_id"   ] = order->id().as_string();
            insert[ "title"      ] = order->title();
            insert[ "state"      ] = order->state().as_string();
            insert[ "state_text" ] = order->state_text();
            insert[ "spooler_id" ] = _spooler->id_for_db();
            insert.set_datetime( "start_time", order->start_time().as_string(Time::without_ms) );
            insert.set_datetime( "end_time"  , order->end_time().as_string(Time::without_ms) );

            execute( insert );


            // Auftragsprotokoll
            string log_filename = order->log().filename();

            if( _spooler->_order_history_with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
            {
                try 
                {
                    string blob_filename = db_name() + " -table=" + _spooler->_order_history_tablename + " -blob=log where \"HISTORY_ID\"=" + as_string( history_id );
                    if( _spooler->_order_history_with_log == arc_gzip )  blob_filename = "gzip | " + blob_filename;
                    copy_file( "file -b " + log_filename, blob_filename );
                }
                catch( const exception& x ) 
                { 
                    _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DES LOGS IN DIE ORDER-HISTORIE: ") + x.what() ); 
                }
            }
        }

        ta.commit();
    }
    catch( const exception& x )  
    { 
        _spooler->log().warn( string("FEHLER BEIM SCHREIBEN DER ORDER-HISTORIE: ") + x.what() ); 
    }
}

//---------------------------------------------------------------------Job_chain::read_order_history
#if 0
xml::Element_ptr Job_chain::read_history( const xml::Document_ptr& doc, int id, int next, Show_what show )
{
    bool with_log = ( show & show_log ) != 0;

    xml::Element_ptr history_element;

    with_log &= _use_db;

    try
    {
        if( !_spooler->_db->opened() )  throw_xc( "SPOOLER-184" );     // Wenn die DB verübergegehen (wegen Nichterreichbarkeit) geschlossen ist, s. get_task_id()

        Transaction ta = +_spooler->_db;
        {
            Any_file sel;
/*
            if( _use_file )
            {
                if( id != -1  ||  next >= 0 )  throw_xc( "SPOOLER-139" );
                sel.open( "-in -seq tab -field-names | tail -head=1 -reverse -" + as_string(-next) + " | " + _filename );
            }
            else
            if( _use_db )
*/
            {
                string prefix = ( next < 0? "-in -seq head -" : "-in -seq tail -reverse -" ) + as_string(max(1,abs(next))) + " | ";
                string clause = " where \"JOB_CHAIN\"=" + sql_quoted(_job_name);
                
                if( id != -1 )
                {
                    clause += " and \"ID\"";
                    clause += next<0? "<" : next>0? ">" : "=";
                    clause += as_string(id);
                }

                clause += " order by \"ID\" ";
                if( next < 0 )  clause += " desc";
                
                sel.open( prefix + _spooler->_db->_db_name + 
                            "select \"ID\", \"SPOOLER_ID\", \"JOB_NAME\", \"START_TIME\", \"END_TIME\", \"CAUSE\", \"STEPS\", \"ERROR\", \"ERROR_CODE\", \"ERROR_TEXT\" " +
                            join( "", vector_map( prepend_comma, _extra_names ) ) +
                            " from " + uquoted(_spooler->_job_history_tablename) + 
                            clause );
            }
            else
                throw_xc( "SPOOLER-136" );

            history_element = doc.createElement( "history" );
            dom_append_nl( history_element );

            const Record_type* type = sel.spec().field_type_ptr();
            Dynamic_area rec ( type->field_size() );

            while( !sel.eof() )
            {
                string           param_xml;
                xml::Element_ptr history_entry = doc.createElement( "history.entry" );

                sel.get( &rec );
    
                for( int i = 0; i < type->field_count(); i++ )
                {
                    string value = type->as_string( i, rec.byte_ptr() );
                    if( value != "" )
                    {
                        string name = type->field_descr_ptr(i)->name();
                        if( name == "parameters" )  param_xml = value;
                                                else  history_entry.setAttribute( lcase(name), value );
                    }
                }

                int id = type->field_descr_ptr("id")->as_int( rec.byte_ptr() );


                if( with_log )
                {
                    try
                    {
                        string log = file_as_string( GZIP_AUTO + _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -blob=log where \"ID\"=" + as_string(id), "" );
                        if( !log.empty() ) dom_append_text_element( history_entry, "log", log );
                    }
                    catch( const exception&  x ) { _spooler->_log.warn( string("Historie: ") + x.what() ); }
                }

                history_element.appendChild( history_entry );
                dom_append_nl( history_element );
            }

            sel.close();
        }
        ta.commit();
    }
    catch( const _com_error& x ) { throw_com_error( x, "Job_history::read_tail" ); }

    return history_element;
}

#endif
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
    catch( const exception& x ) { _job->_log.warn( string("FEHLER BEIM SCHLIESSEN DER JOB-HISTORIE: ") + x.what() ); }
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
            Transaction ta = +_spooler->_db;
            {
                _with_log = read_profile_with_log( _spooler->_factory_ini, section, "history_with_log", _spooler->_job_history_with_log );

                set<string> my_columns = set_map( lcase, set_split( ", *", replace_regex( string(history_column_names) + "," + history_column_names_db, ":[^,]+", "" ) ) );

                _spooler->_db->open_history_table();

                const Record_type* type = _spooler->_db->_history_table.spec().field_type_ptr();
                if( type ) 
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

            //_extra_record.construct( make_record_type( extra_columns ) );
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

            _job->_log.debug( "Neue Historiendatei eröffnet: " +  _filename );
            _use_file = true;
        }

         //record.type()->field_descr_ptr("error_text")->type_ptr()->field_size()
    }
    catch( const exception& x )  
    { 
        _job->_log.warn( string("FEHLER BEIM ÖFFNEN DER HISTORIE: ") + x.what() ); 
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
    catch( const exception& x )  
    { 
        _job->_log.warn( string("FEHLER BEIM SCHLIESSEN DER HISTORIE: ") + x.what() ); 
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

        _job->_log.info( "Bisherige Historie ist archiviert worden unter " + arc_filename );
    }
}

//---------------------------------------------------------------------------Job_history::read_tail
// Anderer Thread.
// Hier nicht auf _job etc. zugreifen!

xml::Element_ptr Job_history::read_tail( const xml::Document_ptr& doc, int id, int next, Show_what show )
{
    if( !_history_yes )  throw_xc( "SPOOLER-141", _job_name );

    bool with_log = ( show & show_log ) != 0;

    xml::Element_ptr history_element;

    if( !_error )  
    {
        with_log &= _use_db;

        try
        {
            if( _use_db  &&  !_spooler->_db->opened() )  throw_xc( "SPOOLER-184" );     // Wenn die DB verübergegehen (wegen Nichterreichbarkeit) geschlossen ist, s. get_task_id()

            Transaction ta = +_spooler->_db;
            {
                Any_file sel;

                if( _use_file )
                {
                    if( id != -1  ||  next >= 0 )  throw_xc( "SPOOLER-139" );
                    sel.open( "-in -seq tab -field-names | tail -head=1 -reverse -" + as_string(-next) + " | " + _filename );
                }
                else
                if( _use_db )
                {
                    string prefix = ( next < 0? "-in -seq head -" : "-in -seq tail -reverse -" ) + as_string(max(1,abs(next))) + " | ";
                    string clause = " where \"JOB_NAME\"=" + sql_quoted(_job_name);
                    
                    if( id != -1 )
                    {
                        clause += " and \"ID\"";
                        clause += next<0? "<" : next>0? ">" : "=";
                        clause += as_string(id);
                    }

                    clause += " order by \"ID\" ";
                    if( next < 0 )  clause += " desc";
                    
                    sel.open( prefix + _spooler->_db->_db_name + 
                              "select \"ID\", \"SPOOLER_ID\", \"JOB_NAME\", \"START_TIME\", \"END_TIME\", \"CAUSE\", \"STEPS\", \"ERROR\", \"ERROR_CODE\", \"ERROR_TEXT\" " +
                              join( "", vector_map( prepend_comma, _extra_names ) ) +
                              " from " + uquoted(_spooler->_job_history_tablename) + 
                              clause );
                }
                else
                    throw_xc( "SPOOLER-136" );

                history_element = doc.createElement( "history" );
                dom_append_nl( history_element );

                const Record_type* type = sel.spec().field_type_ptr();
                Dynamic_area rec ( type->field_size() );
    
                while( !sel.eof() )
                {
                    string           param_xml;
                    xml::Element_ptr history_entry = doc.createElement( "history.entry" );

                    sel.get( &rec );
        
                    for( int i = 0; i < type->field_count(); i++ )
                    {
                        string value = type->as_string( i, rec.byte_ptr() );
                        if( value != "" )
                        {
                            string name = type->field_descr_ptr(i)->name();
                            if( name == "parameters" )  param_xml = value;
                                                  else  history_entry.setAttribute( lcase(name), value );
                        }
                    }

                    int id = type->field_descr_ptr("id")->as_int( rec.byte_ptr() );


#ifndef SPOOLER_USE_LIBXML2     // libxml2 stürzt in Dump() ab:
                    if( _use_db ) 
                        param_xml = file_as_string( _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -blob=parameters where id=" + as_string(id), "" );

                    if( !param_xml.empty() )
                    {
                        try {
                            dom_append_nl( history_element );
                            xml::Document_ptr par_doc;
                            par_doc.create();
                            par_doc.load_xml( param_xml );
                            if( par_doc.documentElement() )  history_entry.appendChild( par_doc.documentElement() );
                        }
                        catch( const exception&  x ) { _spooler->_log.warn( string("Historie: ") + x.what() ); }
                        catch( const _com_error& x ) { _spooler->_log.warn( string("Historie: ") + w_as_string(x.Description() )) ; }
                    }
#endif
                    if( with_log )
                    {
                        try
                        {
                            string log = file_as_string( GZIP_AUTO + _spooler->_db->_db_name + "-table=" + _spooler->_job_history_tablename + " -blob=log where \"ID\"=" + as_string(id), "" );
                            if( !log.empty() ) dom_append_text_element( history_entry, "log", log );
                        }
                        catch( const exception&  x ) { _spooler->_log.warn( string("Historie: ") + x.what() ); }
                    }

                    history_element.appendChild( history_entry );
                    dom_append_nl( history_element );
                }

                sel.close();
            }
            ta.commit();
        }
        catch( const _com_error& x ) { throw_com_error( x, "Job_history::read_tail" ); }
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

    if( _job_history->_use_db )
    {
        if( !_spooler->_db->opened() )       // Datenbank ist (wegen eines Fehlers) geschlossen worden?
        {
            _job_history->close();
            _job_history->open();
            
            if( !start )  
            {
                _spooler->log().info( "Historiensatz wird wegen vorausgegangen Datenbankfehlers nicht geschrieben" );
                return;
            }
        }
    }

    if( _job_history->_use_db )
    {
        Transaction ta = +_spooler->_db;
        {
            if( start )
            {
                Record record = _spooler->_db->_history_table.create_record();

                record.set_field( "id"             , _task->_id );
                record.set_field( "spooler_id"     , _spooler->id_for_db() );
                record.set_field( "job_name"       , _task->job()->name() );
                record.set_field( "start_time"     , start_time );
                record.set_field( "cause"          , start_cause_name( _task->_cause ) );

                if( !parameters.empty()  &&  parameters.length() < blob_field_size )  record.set_field( "parameters", parameters ), parameters = "";

                _spooler->_db->_history_table.insert( record );

                if( !parameters.empty() )
                {
                    Any_file blob;
                    blob.open( "-out " + _spooler->_db->db_name() + " -table=" + _spooler->_job_history_tablename + " -blob='parameters' where \"ID\"=" + as_string( _task->_id ) );
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
                string stmt = "UPDATE " + uquoted(_spooler->_job_history_tablename) + " set ";
                stmt +=   "\"START_TIME\"={ts'" + start_time + "'}";
                stmt += ", \"END_TIME\"={ts'" + Time::now().as_string(Time::without_ms) + "'}";
                stmt += ", \"STEPS\"=" + as_string( _task->_step_count );
                stmt += ", \"ERROR\"=" + as_string( _task->has_error() );
                if( !_task->_error.code().empty() ) stmt += ", \"ERROR_CODE\"=" + sql_quoted( _task->_error.code() );
                if( !_task->_error.what().empty() ) stmt += ", \"ERROR_TEXT\"=" + sql_quoted( _task->_error.what().substr( 0, 250 ) );

                for( int i = 0; i < _extra_record.type()->field_count(); i++ )
                {
                    if( !_extra_record.null(i) )
                    {
                        string s = _extra_record.as_string(i);
                        if( !is_numeric( _extra_record.type()->field_descr_ptr(i)->type_ptr()->info()->_std_type ) )  s = sql_quoted(s);
                        stmt += ", " + sql_quoted_name( _job_history->_extra_names[i] ) + "=" + s;
                    }
                }


                stmt += " where id=" + as_string( _task->_id );
                _spooler->_db->execute( stmt );


                // Task-Protokoll
                string log_filename = _task->_log.filename();
                if( _job_history->_with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
                {
                    try {
                        string blob_filename = _spooler->_db->db_name() + " -table=" + _spooler->_job_history_tablename + " -blob='log' where \"ID\"=" + as_string( _task->_id );
                        if( _job_history->_with_log == arc_gzip )  blob_filename = "gzip | " + blob_filename;
                        copy_file( "file -b " + log_filename, blob_filename );
                    }
                    catch( const exception& x ) { _task->_log.warn( string("Historie: ") + x.what() ); }
                }
            }
        }
        ta.commit();
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

        if( !start )
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

    _extra_record.construct( _job_history->_extra_type );


    try
    {
        if( _job_history->_use_file )  _record_pos = _job_history->_file.tell();

        write( true );
    }
    catch( const exception& x )  
    { 
        _task->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() );
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
            Z_DEBUG_ONLY( if(_use_file|_use_db) _task->_log.debug9( "Historieneintrag wird wieder gelöscht, weil nicht genug Jobschritte ausgeführt worden sind\n" ); )

            //if( _use_file )  SetEndOfFile();

            if( _use_db )
            {
                Transaction ta = &_spooler->_db;
                _spooler->_db->execute( "DELETE from " + _spooler->_job_history_tablename + " where \"id\"=" + as_string(_task->_id) );
                ta.commit();
            }
        }
*/
    }
    catch( const exception& x )  
    { 
        _task->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
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

