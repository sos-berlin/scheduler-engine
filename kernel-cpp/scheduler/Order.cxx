// $Id: Order.cxx 14990 2011-08-21 17:06:05Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "Order_subsystem_impl.h"

#include "../javaproxy/com__sos__scheduler__engine__kernel__order__Order.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderState.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderStateChangedEvent.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderFinishedEvent.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderTouchedEvent.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderSuspendedEvent.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderResumedEvent.h"
typedef javaproxy::com::sos::scheduler::engine::kernel::order::OrderState OrderStateJ;
typedef javaproxy::com::sos::scheduler::engine::kernel::order::OrderStateChangedEvent OrderStateChangedEventJ;
typedef javaproxy::com::sos::scheduler::engine::kernel::order::OrderFinishedEvent OrderFinishedEventJ;
typedef javaproxy::com::sos::scheduler::engine::kernel::order::OrderTouchedEvent OrderTouchedEventJ;
typedef javaproxy::com::sos::scheduler::engine::kernel::order::OrderSuspendedEvent OrderSuspendedEventJ;
typedef javaproxy::com::sos::scheduler::engine::kernel::order::OrderResumedEvent OrderResumedEventJ;

namespace sos {
namespace scheduler {
namespace order {

using namespace job_chain;

//---------------------------------------------------------------------------------------------const

const int    max_insert_race_retry_count                = 5;                            // Race condition beim Einfügen eines Datensatzes

//-------------------------------------------------------------------------------Order_schedule_use

struct Order_schedule_use : Schedule_use
{
                                Order_schedule_use          ( Order* order )                        : Schedule_use(order), _order(order) {}

    void                        on_schedule_loaded          ()                                      { return _order->on_schedule_loaded(); }
    void                        on_schedule_modified        ()                                      { return _order->on_schedule_modified(); }
    bool                        on_schedule_to_be_removed   ()                                      { return _order->on_schedule_to_be_removed(); }
    string                      name_for_function           () const                                { return _order->string_id(); }

  private:
    Order*                     _order;
};

//--------------------------------------------------------------------db_text_distributed_next_time
// Für SqlServer: distributed_next_time kann zwischen Sommer- und Winterzeit liegen.
// Bei einem Vergleich konvertiert SqlServer in einen echten Zeitstempel und stößt auf einen Fehler,
// weil es die Stunde zwischen 2 und 3 nicht gibt.
// Das Problem umgehen wir, wenn wir nach Text (ISO-Format) konvertierem.

string db_text_distributed_next_time()
{
    return "%texttimestamp(SCHEDULER_ORDERS.`distributed_next_time`)";
}

//------------------------------------------------------------------------split_standing_order_name

void split_standing_order_name( const string& name, string* job_chain_name, string* order_id )
{
    // Könnte bei Bedarf nach File_based_subsystem verallgemeinert werden

    size_t pos = name.find( folder_name_separator );
    if( pos == string::npos )  pos = 0;

    *job_chain_name = name.substr( 0, pos );

    if( pos < name.length() )  pos++;
    *order_id = name.substr( pos );
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Standing_order_subsystem* subsystem )
:
    Com_order(this),
    file_based<Order,Standing_order_folder,Standing_order_subsystem>( subsystem, static_cast<IDispatch*>( this ), type_standing_order ),
    javabridge::has_proxy<Order>(subsystem->spooler()),
    _zero_(this+1)
{
    _com_log = new Com_log;
    _com_log->set_log( log() );

    _created       = Time::now();
    _is_virgin     = true;
    //_signaled_next_time = Time::never;

    _schedule_use = Z_NEW( Order_schedule_use( this ) );
    _schedule_use->set_scheduler_holidays_usage( schedule::without_scheduler_holidays );  // <config><holidays> nicht übernehmen, eMail von Andreas Liebert 2008-04-21
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
    if( _http_operation )  _http_operation->unlink_order();

#   ifdef Z_DEBUG
        assert( !_is_db_occupied );
        assert( !_task );
        assert( !_job_chain );
        assert( !_is_on_blacklist );
        assert( !_is_in_order_queue );
      //assert( !_is_replacement );
      //assert( !_replaced_by );
      //assert( !_order_queue );
#   endif

    set_replacement( false );

    if( _replaced_by )  
    {
        _replaced_by->set_replacement( false );
    }

    _schedule_use = NULL;
    if( _com_log  )  _com_log->set_log( NULL );
}

//-------------------------------------------------------------------------------Order::load_record

void Order::load_record( const Absolute_path& job_chain_path, const Record& record )
{
    _job_chain_path = job_chain_path;
    //_job_chain_path.set_absolute( "/", job_chain_path );

    set_id      ( record.as_string( "id"         ) );   _id_locked = true;
    _state      = record.as_string( "state"      );
    _state_text = record.as_string( "state_text" );
    _title      = record.as_string( "title"      );
    _priority   = record.as_int   ( "priority"   );
//    _suspended  = record.as_int   ( "suspended" ) != 0;   // JS-333

    string initial_state = record.as_string( "initial_state" );
    if( initial_state != "" )
    {
        _initial_state = initial_state;
        _initial_state_set = true;
    }

    _created.set_datetime( record.as_string( "created_time" ) );

    if( record.has_field( "distributed_next_time" ) )  _setback.set_datetime( record.as_string( "distributed_next_time" ) );

    _log->set_prefix( obj_name() );

    _order_xml_modified  = false;            
    _state_text_modified = false; 
    _title_modified      = false;
    _state_text_modified = false;
    _is_in_database      = true;
}

//--------------------------------------------------------------------------------Order::load_blobs

void Order::load_blobs( Read_transaction* ta )
{
    load_run_time_blob( ta );
    load_order_xml_blob( ta );      // Setzt _period, deshalb nach load_run_time_blob()
    load_payload_blob( ta );
}

//-----------------------------------------------------------------------Order::load_order_xml_blob

void Order::load_order_xml_blob( Read_transaction* ta )
{
    string order_xml = db_read_clob( ta, "order_xml" );
    if( order_xml != "" )  set_dom( xml::Document_ptr( order_xml ).documentElement() );
}

//------------------------------------------------------------------------Order::load_run_time_blob

void Order::load_run_time_blob( Read_transaction* ta )
{
    string run_time_xml = db_read_clob( ta, "run_time" );
    if( run_time_xml != "" )  set_schedule( (File_based*)NULL, xml::Document_ptr( run_time_xml ).documentElement() );
}

//-------------------------------------------------------------------------Order::load_payload_blob

void Order::load_payload_blob( Read_transaction* ta )
{
    string payload_string = db_read_clob( ta, "payload" );
    if( payload_string.find( "<" + Com_variable_set::xml_element_name() ) != string::npos )
    {
        ptr<Com_variable_set> v = new Com_variable_set;
        v->put_Xml( Bstr( payload_string ) );               // 2008-05-23  SCHLECHT: Löst keine Exception, sollte aber! 
        _payload = v;
    }
    else
    {
        if( payload_string.empty() )  _payload = (IDispatch*)NULL;
                                else  _payload = payload_string;
    }
}

//---------------------------------------------------------------------------------Order::string_id

string Order::string_id() 
{
    return string_id( _id );
}

//---------------------------------------------------------------------------------Order::string_id

string Order::string_id( const Id& id ) 
{
    try
    {
        return string_from_variant( id );
    }
    catch( exception& x ) 
    { 
        z::throw_xc( "SCHEDULER-249", x.what() ); 
    }
}

//----------------------------------------------------------------Order::is_immediately_processable

bool Order::is_immediately_processable( const Time& at )
{
    // select ... from scheduler_orders  where not suspended and replacement_for is null and setback is null 
    // scheduler_orders.processable := !_is_on_blacklist && !_suspende

    return _setback <= at  &&  is_processable();
}

//----------------------------------------------------------------------------Order::is_processable

bool Order::is_processable()
{
    if( _is_on_blacklist ) return false;  
    if( _suspended )       return false;
    if( _task )            return false;               // Schon in Verarbeitung
    if( _is_replacement )  return false;

    if( _job_chain && !_job_chain->is_ready_for_order_processing())  return false;
    if( Node* node = job_chain_node() )
        if (!node->is_ready_for_order_processing())  return false;

    return true;
}

//---------------------------------------------------------------------------Order::why_dom_element

xml::Element_ptr Order::why_dom_element(const xml::Document_ptr& doc, const Time& now) {
    xml::Element_ptr result = doc.createElement("order");
    result.setAttribute("id", string_id());
    if (now < _setback) append_obstacle_element(result, _setback_count == 0? "at" : "setback", _setback.xml_value());
    if (_is_on_blacklist) append_obstacle_element(result, "on_blacklist", as_bool_string(_is_on_blacklist));
    if (_suspended) append_obstacle_element(result, "suspended", as_bool_string(_suspended));
    if (_task)  append_obstacle_element(result, _task->dom_element(doc, Show_what()));
    if (_is_replacement)  append_obstacle_element(result, "is_replacement", as_bool_string(_is_replacement));
    //Schon oben aufrufen (sonst rekursiv): if (_job_chain) result.appendChild(_job_chain->why_dom_element(doc));
    //Schon oben aufrufen (sonst rekursiv): if (Node* node = job_chain_node()) result.appendChild(node->why_dom_element(doc, now));
    return result;
}

//----------------------------------------------------------Order::handle_changed_processable_state
// Nach Änderung von next_time(), also _setback und is_processable() zu rufen!

void Order::handle_changed_processable_state()
{
    // Eine schönere Implementierung wäre vielleicht, nur für den ersten Auftrag der Warteschlange 
    // Order_queue_node::handle_changed_processable_state() aufzurufen,
    // denn die weiter hinten stehenden Aufträge laufen nie vor dem ersten an.
    // Dann hätten wir weniger Aufruf an Job::signal_earlier_order().

    Time new_next_time = next_time();

    //if( new_next_time <= _signaled_next_time )
    //{
        if( Job_node* job_node = Job_node::try_cast( _job_chain_node ) )
        {
            if( Job* job = job_node->job_or_null() )  
            {
                job->signal_earlier_order( this );
                //_signaled_next_time = new_next_time;
            }
        }
    //}
}

//-----------------------------------------------------------------Order::assert_is_not_distributed

void Order::assert_is_not_distributed( const string& debug_text )
{
    if( _is_distributed )  z::throw_xc( "SCHEDULER-375", debug_text );
}

//---------------------------------------------------------------------------Order::set_distributed

void Order::set_distributed( bool distributed )
{
    assert( distributed );

    _is_distributed = distributed;
}

//----------------------------------------------------------------------------Order::assert_no_task

void Order::assert_no_task( const string& debug_text )
{
    if( _task )  z::throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name(), debug_text );
}

//-------------------------------------------------------------------------------Order::assert_task

void Order::assert_task( const string& debug_text )
{
    if( !_task )  assert(0), z::throw_xc( "ORDER-HAS-NO-TASK", obj_name(), debug_text );
}

//---------------------------------------------------------------------------Order::occupy_for_task

void Order::occupy_for_task( Task* task, const Time& now )
{
    assert( task );
    assert_no_task( Z_FUNCTION );   // Vorsichtshalber
    

    if( !_log->is_active() )  open_log();

    if( _delay_storing_until_processing  &&  _job_chain  &&  _job_chain->_orders_are_recoverable  &&  !_is_in_database  &&  db()->opened() )
    {
        db_insert();
        _delay_storing_until_processing = false;
    }


    if( _moved )  assert(0), z::throw_xc( "SCHEDULER-0", obj_name() + " _moved=true?" );

    _setback        = 0;
    _setback_called = false;
    _moved          = false;
    _task           = task;
    if( !_start_time )  _start_time = now;      


    bool was_virgin = _is_virgin;
    _is_virgin = false;
    if( was_virgin )
    {
        if ( _job_chain )   // JS-682
            _job_chain->check_max_orders();  // Keine Exception auslösen oder occupy_for_task() zurücknehmen (also _task=NULL setzen)
        if( _http_operation )  _http_operation->on_first_order_processing( task );
        order_subsystem()->count_started_orders();
        report_event( OrderTouchedEventJ::new_instance(java_sister()) );
    }
}

//------------------------------------------------------------Order::db_insert_order_history_record

void Order::db_insert_order_history_record( Transaction* ta )
{
    if( _spooler->_order_history_yes  &&  db()->opened() )
    {
        _history_id = db()->get_order_history_id( ta );

        sql::Insert_stmt insert ( ta->database_descriptor() );

        insert.set_table_name( db()->_order_history_tablename );
        
        insert[ "history_id" ] = _history_id;
        insert[ "job_chain"  ] = job_chain_path().without_slash();
        insert[ "order_id"   ] = id().as_string();
        insert[ "title"      ] = title();
        insert[ "state"      ] = state().as_string();
        insert[ "state_text" ] = state_text();
        insert[ "spooler_id" ] = _spooler->id_for_db();
        insert.set_datetime( "start_time", start_time().as_string(Time::without_ms) );
        //insert.set_datetime( "end_time"  , "0000-00-00 00:00:00" );

        ta->execute( insert, Z_FUNCTION );
    }
}

//------------------------------------------------------------Order::db_update_order_history_record

void Order::db_update_order_history_record( Transaction* outer_transaction )
{
    if( _spooler->_order_history_yes  &&  db()->opened() )
    {
        if( !start_time() )  return;
        assert( _history_id );

        for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
        {
            sql::Update_stmt update ( ta.database_descriptor() );

            update.set_table_name( db()->_order_history_tablename );
            update.and_where_condition( "history_id", _history_id );
            
            update[ "state"      ] = state().as_string();
            update[ "state_text" ] = state_text();
            update[ "title"      ] = title();
            update.set_datetime( "end_time"  , ( end_time()? end_time() : Time::now() ).as_string(Time::without_ms) );

            ta.execute( update, Z_FUNCTION );

            // Auftragsprotokoll

            if( _spooler->_order_history_with_log )
            {
                string log_text = log()->as_string_ignore_error();
                if( log_text != "" )
                {
                    try 
                    {
                        ta.set_transaction_written();
                        string blob_filename = db()->db_name() + " -table=" + db()->_order_history_tablename + " -blob=log where `history_id`=" + as_string( _history_id );
                        if( _spooler->_order_history_with_log == arc_gzip )  blob_filename = GZIP + blob_filename;
                        Any_file blob_file( "-out -binary " + blob_filename );
                        blob_file.put( log_text );
                        blob_file.close();
                    }
                    catch( exception& x ) 
                    { 
                        _log->warn( message_string( "SCHEDULER-267", db()->_order_history_tablename, x.what() ) );      // "FEHLER BEIM SCHREIBEN DES LOGS IN DIE TABELLE "
                    }
                }
            }

            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_order_history_tablename, x ), Z_FUNCTION ); }
    }

    _history_id = 0;        // Beim nächsten Start neue history_id ziehen
}

//-------------------------------------------------------Order::db_insert_order_step_history_record

void Order::db_insert_order_step_history_record( Transaction* ta )
{
    if( _spooler->_order_history_yes  &&  db()->opened() )
    {
        assert( _history_id );
        assert( _task );


        try
        {
            Record record = ta->read_single_record( S() << "select max(`step`)  from " << db()->_order_step_history_tablename 
                                                        << "  where `history_id`=" << _history_id,
                                                   Z_FUNCTION );
            _step_number = record.null( 0 )? 1 : record.as_int( 0 ) + 1;
        }
        catch( sos::Null_error& )     // Die Hostware macht das Feld im Record nicht nullable
        {
            _step_number = 1;
        }

        sql::Insert_stmt insert ( ta->database_descriptor(), db()->_order_step_history_tablename );
        
        insert[ "history_id" ] = _history_id;
        insert[ "step"       ] = _step_number;
        insert[ "state"      ] = state().as_string();
        insert[ "task_id"    ] = _task->id();
        insert[ "start_time" ].set_datetime( Time::now().as_string( Time::without_ms ) );

        ta->execute( insert, Z_FUNCTION );
    }
}

//-------------------------------------------------------Order::db_update_order_step_history_record

void Order::db_update_order_step_history_record( Transaction* ta )
{
    if( _spooler->_order_history_yes  &&  db()->opened() )
    {
        assert( _history_id );
        assert( _step_number );


        sql::Update_stmt update ( db()->database_descriptor(), db()->_order_step_history_tablename );

        update.and_where_condition( "history_id", _history_id  );
        update.and_where_condition( "step"      , _step_number );
        
        update[ "end_time"   ].set_datetime( Time::now().as_string( Time::without_ms ) );
      //update[ "state_text" ] = state_text();
        update[ "error"      ] = _task_error != NULL;

        if( _task_error )
        {
            if( !_task_error.code().empty() )  update[ "error_code" ] = _task_error->code();
            if( !_task_error.what().empty() )  update[ "error_text" ] = string( _task_error->what() ).substr( 0, max_column_length );    // Für MySQL 249 statt 250. jz 7.1.04
        }

        ta->execute_single( update, Z_FUNCTION );
    }
}

//------------------------------------------------------------------Order::db_occupy_for_processing

bool Order::db_occupy_for_processing()
{
    assert( _is_distributed );
    assert( _job_chain == NULL );   // Der Auftrag kommt erst nach der Belegung in die Jobkette, deshalb ist _job_chain == NULL

    _spooler->assert_are_orders_distributed( Z_FUNCTION );

    sql::Update_stmt update = db_update_stmt();

    update[ "occupying_cluster_member_id" ] = _spooler->cluster_member_id();

    update.and_where_condition( "state"                      , state().as_string() );
    update.and_where_condition( "occupying_cluster_member_id", sql::null_value );
    update.add_where( S() << " and " << db_text_distributed_next_time() << "=" + sql::quoted( calculate_db_distributed_next_time() ) );
    //update.and_where_condition( "distributed_next_time"      , calculate_db_distributed_next_time() );

    bool update_ok = false;

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        update_ok = ta.try_execute_single( update, Z_FUNCTION );
        ta.commit( Z_FUNCTION );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); }

    
    if( update_ok )
    {
        _is_db_occupied = true, _occupied_state = _state;
        //if( order_subsystem()->_database_order_detector )  order_subsystem()->_database_order_detector->on_new_order();     // Verkürzt die Überwachungsperiode
    }
    else
    {
        _log->debug( message_string( "SCHEDULER-812" ) );
        db_show_occupation( log_debug9 );
    }

    return _is_db_occupied;
}

//------------------------------------------------------------------------Order::db_show_occupation

void Order::db_show_occupation( Log_level log_level )
{
    try
    {
        Read_transaction ta ( db() );

        Any_file result_set = ta.open_result_set
            ( 
                S() << "select `occupying_cluster_member_id`"
                       "  from " << db()->_orders_tablename
                       << db_where_clause().where_string(),
                Z_FUNCTION );

        if( result_set.eof() )
        {
            _log->log( log_level, message_string( "SCHEDULER-817" ) );
        }
        else
        {
            // ?Haben wir eine Satzsperre oder kann ein anderer Scheduler occupying_cluster_member_id schon wieder auf null gesetzt haben?
            Record record = result_set.get_record();
            _log->log( log_level, message_string( "SCHEDULER-813", record.as_string(0) ) );
        }
    }
    catch( exception& x ) 
    { 
        _log->error( S() << x.what() << ", in " << Z_FUNCTION );
    }
}

//---------------------------------------------------------------------------------Order::db_insert

void Order::db_insert()
{
    bool ok = db_try_insert( true );
    if( !ok )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_path, "in database" );
}

//-----------------------------------------------------------------------------Order::db_try_insert

bool Order::db_try_insert( bool throw_exists_exception )
{
    bool   insert_ok               = false;
    string payload_string          = string_payload();
    bool   record_exists_error     = false;
    string record_exists_insertion;

    if( db()->opened() )
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        if( !db()->opened() )  break;

        if( _is_replacement )
        {
            Any_file result_set = ta.open_result_set
                ( 
                    S() << "select `occupying_cluster_member_id`"
                           "  from " << db()->_orders_tablename << " %update_lock" 
                           << db_update_stmt().where_string(), 
                    Z_FUNCTION 
                );

            if( result_set.eof() )
            {
                set_replacement( false );
            }
            else
            {
                Record record = result_set.get_record();

                if( record.null( "occupying_cluster_member_id" ) )
                    set_replacement( false );
                else
                {
                    _replaced_order_occupator = record.as_string( "occupying_cluster_member_id" );
                    _log->info( message_string( "SCHEDULER-942", "Scheduler member " + record.as_string( "occupying_cluster_member_id" ), "replaced order" ) );
                }

                ta.execute_single( db_update_stmt().make_delete_stmt(), Z_FUNCTION );
            }
        }
        else
        if( !_is_distributed )
        {
            ta.execute( db_update_stmt().make_delete_stmt() + " and `distributed_next_time` is null", Z_FUNCTION );
        }
        else
        {
            // Satz darf nicht vorhanden sein.
        }


        int ordering = db_get_ordering( &ta );
        

        sql::Insert_stmt insert ( ta.database_descriptor(), db()->_orders_tablename );
        
        insert[ "ordering"      ] = ordering;
        insert[ "job_chain"     ] = _job_chain_path.without_slash();
        insert[ "id"            ] = id().as_string();
        insert[ "spooler_id"    ] = _spooler->id_for_db();
        insert[ "title"         ] = title()                     , _title_modified      = false;
        insert[ "state"         ] = state().as_string();
        insert[ "state_text"    ] = state_text()                , _state_text_modified = false;
        insert[ "priority"      ] = priority()                  , _priority_modified   = false;
        insert[ "initial_state" ] = initial_state().as_string();
//        insert[ "suspended"     ] = _suspended;   // JS-333

        db_fill_stmt( &insert );

        insert.set_datetime( "created_time", _created.as_string(Time::without_ms) );

        for( int insert_race_retry_count = 1; !insert_ok; insert_race_retry_count++ )
        {
            try
            {
                ta.execute( insert, Z_FUNCTION );
                insert_ok = true;
            }
            catch( exception& x )     // Datensatz ist bereits vorhanden?
            {
                ta.intermediate_rollback( Z_FUNCTION );      // Postgres verlangt nach Fehler ein Rollback

                if( insert_race_retry_count > max_insert_race_retry_count )  throw;

                Any_file result_set = ta.open_result_set
                    ( 
                        S() << "select " << db_text_distributed_next_time() << " as distributed_next_time" 
                               " from " << db()->_orders_tablename << 
                               db_where_clause().where_string(), 
                        Z_FUNCTION 
                    );

                if( !result_set.eof() )
                {
                    if( throw_exists_exception )  
                    {
                        Record record = result_set.get_record();
                        record_exists_error     = true;
                        record_exists_insertion = record.null( "distributed_next_time" )? "in database, not distributed" : "distributed";
                    }

                    break;
                }

                string msg = message_string( "SCHEDULER-851", insert_race_retry_count, x );
                ( _job_chain? _job_chain->log() : _spooler->log() ) -> info( S() << obj_name() << ":" << msg );
                _log->debug( msg );
            }
        }

        if( insert_ok )
        {
            if( payload_string != "" )  db_update_clob( &ta, "payload", payload_string );
            //_payload_modified = false;

            xml::Document_ptr order_document = dom( show_for_database_only );
            xml::Element_ptr  order_element  = order_document.documentElement();
            if( order_element.hasAttributes()  ||  order_element.firstChild() )
                db_update_clob( &ta, "order_xml", order_document.xml() );

            if( _schedule_use->is_defined() )
            {
                xml::Document_ptr doc = _schedule_use->dom_document( show_for_database_only );
                if( doc.documentElement().hasAttributes()  ||  doc.documentElement().hasChildNodes() )  db_update_clob( &ta, "run_time", doc.xml() );
            }

            ta.commit( Z_FUNCTION );

            _is_in_database = true;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-305", db()->_orders_tablename, x ), Z_FUNCTION ); }

    if( record_exists_error  &&  throw_exists_exception )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_path, record_exists_insertion );

    if( insert_ok )  tip_own_job_for_new_distributed_order_state();

    return insert_ok;
}

//---------------------------------------------------------------------Order::db_release_occupation

bool Order::db_release_occupation()
{
    bool update_ok = false;

    if( _is_db_occupied  &&  _spooler->cluster_member_id() != "" )
    {
        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
        {
            sql::Update_stmt update = db_update_stmt();

            update[ "occupying_cluster_member_id" ] = sql::null_value;
            update.and_where_condition( "occupying_cluster_member_id", _spooler->cluster_member_id() );
            update.and_where_condition( "state"                      , _occupied_state.as_string()   );

            update_ok = ta.try_execute_single( update, Z_FUNCTION );

            ta.commit( Z_FUNCTION );

            if( !update_ok ) 
            {
                _log->error( message_string( "SCHEDULER-816" ) );
                db_show_occupation( log_error );
            }

            _is_db_occupied = false; 
        }
        catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", db()->_orders_tablename, x ), Z_FUNCTION ); }
    }

    return update_ok;
}

//--------------------------------------------------------------------------------Order::db_update2

bool Order::db_update2( Update_option update_option, bool delet, Transaction* outer_transaction )
{
    bool update_ok = false;

    // outer_transaction nur für db_handle_modified_order() oder vor sicherem Commit. Bei Rollback würde Order nicht mit Datenbanksatz übereinstimmen.

    if( update_option == update_and_release_occupation  &&  _spooler->_are_all_tasks_killed )
    {
        _log->warn( message_string( "SCHEDULER-830" ) );   // "Because all Scheduler tasks are killed, the order in database is not updated. Only the occupation is released"
        update_ok = db_release_occupation();
    }
    else
    if( _is_in_database &&  db()->opened() )
    {
        if( update_option == update_not_occupied  &&  _is_db_occupied )  z::throw_xc( Z_FUNCTION, "is_db_occupied" );


        string           state_string = state().as_string();    // Kann Exception auslösen;
        sql::Update_stmt update       = db_update_stmt();

        if( _is_db_occupied )
        {
            update.and_where_condition( "occupying_cluster_member_id", sql::Value( _spooler->cluster_member_id() ) );
            update.and_where_condition( "state"                      , _occupied_state.as_string() );
        }
        else
        {
            update.and_where_condition( "occupying_cluster_member_id", sql::null_value );
        }


        //if( _job_chain_path == "" )
        if( delet )
        {
            for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
            {
                if( !db()->opened() )  break;

                S delete_sql;
                delete_sql << update.make_delete_stmt();
                delete_sql << " and `distributed_next_time` is " << ( _is_distributed? "not null" : " null" );  // update_ok=false, wenn das nicht stimmt

                update_ok = ta.try_execute_single( delete_sql, Z_FUNCTION );
                if( !update_ok )  update_ok = db_handle_modified_order( &ta );  //int DISTRIBUTED_FEHLER_KOENNTE_GEZEIGT_WERDEN; // Zeigen, wenn distributed_next_time falsch ist.

                if( _history_id )
                {
                    if( update_option == update_and_release_occupation  &&  _step_number )
                        db_update_order_step_history_record( &ta );

                    db_update_order_history_record( &ta );
                }
        
                ta.commit( Z_FUNCTION );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", db()->_orders_tablename, x  ), Z_FUNCTION ); }

            if( !update_ok )  _log->warn( message_string( "SCHEDULER-385" ) );

            _is_in_database = false;  
            _is_db_occupied = false;
            _occupied_state = Variant();
        }
        else
        {
            string payload_string = string_payload();   // Kann Exception auslösen

            if( _is_db_occupied  &&  update_option == update_and_release_occupation )  update[ "occupying_cluster_member_id" ] = sql::null_value;

            update[ "state"         ] = state_string;
            update[ "initial_state" ] = initial_state().as_string();
//            update[ "suspended"     ] = _suspended;      // JS-333

            db_fill_stmt( &update );

            if( _priority_modified   )  update[ "priority"   ] = priority();
            if( _title_modified      )  update[ "title"      ] = title();
            if( _state_text_modified )  update[ "state_text" ] = state_text();

            for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
            {
                if( !db()->opened() )  break;

                if( update_option == update_and_release_occupation  &&  _history_id  &&  _step_number )
                    db_update_order_step_history_record( &ta );

                update_ok = ta.try_execute_single( update, Z_FUNCTION );

                if( !update_ok )
                {
                    update_ok = db_handle_modified_order( &ta );
                }
                else
                //if( !finished() )
                {
                    // _schedule_modified gilt nicht für den Datenbanksatz, sondern für den Auftragsneustart
                    // Vorschlag: xxx_modified auflösen zugunsten eines gecachten letzten Datenbanksatzes, mit dem verglichen wird.
                    if( _schedule_use->is_defined() ) 
                    {
                        xml::Document_ptr doc = _schedule_use->dom_document( show_for_database_only );
                        if( doc.documentElement().hasAttributes()  ||  doc.documentElement().hasChildNodes() )  db_update_clob( &ta, "run_time", doc.xml() );
                                                                                                          else  update[ "run_time" ].set_direct( "null" );
                    }
                    else
                        update[ "run_time" ].set_direct( "null" );

                    //if( _order_xml_modified )  // Das wird nicht überall gesetzt und sowieso ändert sich das Element fast immer
                    {
                        xml::Document_ptr order_document = dom( show_for_database_only );
                        xml::Element_ptr  order_element  = order_document.documentElement();
                        if( order_element.hasAttributes()  ||  order_element.firstChild() )  db_update_clob( &ta, "order_xml", order_document.xml() );
                                                                                       else  update[ "order_xml" ].set_direct( "null" );
                    }

                    //if( _payload_modified )
                    {
                        if( payload_string == "" )  update[ "payload" ].set_direct( "null" );
                                              else  db_update_clob( &ta, "payload", payload_string );
                        //_payload_modified = false;
                    }
                }

                ta.commit( Z_FUNCTION );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", db()->_orders_tablename, x ), Z_FUNCTION ); }
        }

        if( update_option == update_and_release_occupation )
        {
            if( update_ok )  _is_db_occupied = false, _occupied_state = Variant();
            else
            if( _is_db_occupied )
            {
                _log->error( message_string( "SCHEDULER-816" ) );
                db_show_occupation( log_error );
                _is_db_occupied = false, _occupied_state = Variant();  // 2008-05-28  Nicht mehr db_release_occupation() rufen
            }
        }

        _order_xml_modified  = false;            
        _state_text_modified = false; 
        _title_modified      = false;
        _state_text_modified = false;

        tip_own_job_for_new_distributed_order_state();
    }
    else
    if( db()->opened()  &&  _history_id )
    {
        for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
        {
            if( update_option == update_and_release_occupation  &&  _step_number )
                db_update_order_step_history_record( &ta );

            if( finished() )  
                db_update_order_history_record( &ta  );
    
            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", "order history", x ), Z_FUNCTION ); }
    }

    return update_ok;
}

//------------------------------------------------------------------Order::db_handle_modified_order

bool Order::db_handle_modified_order( Transaction* outer_transaction )
{
    // db_update() oder db_delete() konnte den Auftrag nicht ändern, weil die Where-Klausel nicht passte.
    // Wir sehen hier nach der Ursache.

    bool result = false;

    try
    {
        if( ptr<Order> modified_order = order_subsystem()->try_load_order_from_database( outer_transaction, _job_chain_path, _id, Order_subsystem_impl::lo_lock ) )
        {
            if( modified_order->_is_replacement )
            {
                // Der Auftrag in der Datenbank ersetzt unseren, gerade ausgeführten Auftrag.
                // Unser Auftrag bleibt gelöscht, wir speichern ihn nicht.
                // Der neue Auftrag in der Datenbank wird jetzt ausführbar gemacht:

                _log->info( message_string( "SCHEDULER-839" ) );

                modified_order->set_replacement( false );
                result = modified_order->db_update2( update_not_occupied, false, outer_transaction );
            }
            else
            {
                _log->info( message_string( "SCHEDULER-853" ) );
            }
        }
        else
        {
            _log->info( message_string( "SCHEDULER-852" ) );
        }
    }
    catch( exception& x ) 
    {
        _log->error( S() << x.what() << ", in " << Z_FUNCTION );
    }

    return result;
}

//------------------------------------------------------------------------------Order::db_fill_stmt

void Order::db_fill_stmt( sql::Write_stmt* stmt )
{
    stmt->set_datetime( "mod_time", Time::now().as_string(Time::without_ms) );

    string t = calculate_db_distributed_next_time();
    if( stmt->is_update()  ||  t != "" )  stmt->set_datetime( "distributed_next_time", t );
}

//---------------------------------------------------------------Order::close_log_and_write_history

void Order::close_log_and_write_history()
{
    _end_time = Time::now();
    _log->finish_log();
    
    if( _job_chain  &&  db()  &&  db()->opened() && _history_id ) 
    {
        for( Retry_nested_transaction ta ( db(), db()->transaction_or_null() ); ta.enter_loop(); ta++ ) try    // JS-461
        {
            if( _step_number )  db_update_order_step_history_record( &ta );
            db_update_order_history_record( &ta );    // Historie schreiben, aber Auftrag beibehalten
            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", "order history", x ), Z_FUNCTION ); }
    }

    //2007-09-25 HTTP-Protokoll weiterlaufen lassen   _log->close();
}

//--------------------------------------------------------Order::calculate_db_distributed_next_time

string Order::calculate_db_distributed_next_time()
{
    string result;

    if( _is_distributed )
    {
        if( _is_on_blacklist )  result = blacklist_database_distributed_next_time;
        else
        if( _is_replacement  )  result = replacement_database_distributed_next_time;
        else
        {
            Time next_time = this->next_time().rounded_to_next_second();

            result = next_time.is_null ()? now_database_distributed_next_time :
                     next_time.is_never()? never_database_distributed_next_time 
                                         : next_time.as_string( Time::without_ms );
        }
    }

    return result;
}

//------------------------------------------------------------------------------Order::db_read_clob

string Order::db_read_clob( Read_transaction* ta, const string& column_name )
{
    if( db()->db_name() == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    return ta->read_clob( db()->_orders_tablename, column_name, db_where_clause().where_string() );
}

//----------------------------------------------------------------------------Order::db_update_clob

void Order::db_update_clob( Transaction* ta, const string& column_name, const string& value )
{
    if( db()->db_name() == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    if( value == "" )
    {
        sql::Update_stmt update = db_update_stmt();
        update[ column_name ].set_direct( "null" );
        ta->execute( update, Z_FUNCTION );
    }
    else
    {
        ta->update_clob( db()->_orders_tablename, column_name, value, db_where_clause().where_string() );
    }
}

//----------------------------------------------------------------------------Order::db_update_stmt

sql::Update_stmt Order::db_update_stmt()
{
    sql::Update_stmt result ( db()->database_descriptor(), db()->_orders_tablename );
    db_fill_where_clause( &result );
    return result;
}

//---------------------------------------------------------------------------Order::db_where_clause

sql::Where_clause Order::db_where_clause()
{
    sql::Where_clause result ( db()->database_descriptor() );
    db_fill_where_clause( &result );
    return result;
}

//----------------------------------------------------------------------Order::db_fill_where_clause

void Order::db_fill_where_clause( sql::Where_clause* where )
{
    assert( _job_chain_path != "" );

    where->and_where_condition( "spooler_id", _spooler->id_for_db() );
    where->and_where_condition( "job_chain" , _job_chain_path.without_slash() );
    where->and_where_condition( "id"        , id().as_string()      );
}

//---------------------------------------------------------------------------Order::db_get_ordering

int Order::db_get_ordering( Transaction* ta )
{ 
    return db()->get_id( "spooler_order_ordering", ta ); 
}

//----------------------------------------------------------------------------------------Order::db

Database* Order::db()
{
    return _spooler->_db;
}

//----------------------------------------------------------------------------------Order::open_log

void Order::open_log()
{
    if( _job_chain_path != ""  &&  _spooler->_order_history_with_log  &&  !string_begins_with( _spooler->log_directory(), "*" ) )
    {
        string name = _id.as_string();
        
        for( int i = 0; i < name.length(); i++ )        // Ungültige Zeichen in '_' ändern. DAS KANN MEHRDEUTIG WERDEN!
        {
            char& c = name[i];
            switch( c )
            {
                case '/':
                Z_WINDOWS_ONLY( case '\\': )
                Z_WINDOWS_ONLY( case ':' : )
                {
                    c = '_';
                }
            }
        }

        _log->set_filename( _spooler->log_directory() + "/order." + _job_chain_path.to_filename() + "." + name + ".log" );      // Jobprotokoll

        //#if defined Z_DEBUG && defined Z_WINDOWS
        //    if( log_category_is_set( "zschimmer" ) )
        //    {
        //        _log->filename().try_unlink( _log );
        //        assert( ( "order log file", !_log->filename().exists() ) );
        //    }
        //#endif

        _log->set_remove_after_close( true );
        _log->open();
    }
}

//-------------------------------------------------------------------------------------Order::close

void Order::close()
{
    if( _http_operation )
    {
        _http_operation->unlink_order();
        _http_operation = NULL;
    }


    _task = NULL;
    set_replacement( false );
    if( _replaced_by )  _replaced_by->set_replacement( false ), _replaced_by = NULL;

    if( _is_db_occupied )
    {
        try
        {
            Z_LOGI2( "scheduler", Z_FUNCTION << "  db_release_occupation()\n" );
            db_release_occupation();
        }
        catch( exception& x ) { Z_LOG( Z_FUNCTION << "  ERROR " << x.what() << "\n" ); }
    }

    //if( close_flag == cls_remove_from_job_chain )  remove_from_job_chain();
    //else
    if( _job_chain )  _job_chain->remove_order( this );
    if( _schedule_use )  _schedule_use->close();
    check_for_replacing_or_removing();

    _log->close();

    remove_requisite( Requisite_path( order_subsystem(), _job_chain_path ) );
}

//----------------------------------------------------------------------------------Order::set_name
// Wird von folder.cxx aufgerufen

void Order::set_name( const string& name )
{
    assert( has_base_file() );

    file_based< Order, Standing_order_folder, Standing_order_subsystem >::set_name( name );

    split_standing_order_name( name, &_file_based_job_chain_name, &_file_based_id );

    if( _file_based_job_chain_name == ""  ||  _file_based_id == "" )  z::throw_xc( "SCHEDULER-435", base_file_info()._filename );

    set_id( _file_based_id );
    _file_based_job_chain_path = Absolute_path( folder_path(), _file_based_job_chain_name );
}

//-----------------------------------------------------------------------------Order::on_initialize

bool Order::on_initialize()
{
    bool result = true;

    if( has_base_file() )
    {
        set_id( _file_based_id ); 

        if( !job_chain_path().empty()  &&
            subsystem()->normalized_path( job_chain_path() ) != subsystem()->normalized_path( _file_based_job_chain_path ) )  z::throw_xc( "SCHEDULER-437", job_chain_path(), _file_based_job_chain_path );     // Order->set_dom() liest Attribut job_chain nicht!

        assert( _file_based_job_chain_path != "" );
        add_requisite( Requisite_path( order_subsystem(), _file_based_job_chain_path ) );

        if( Job_chain* job_chain = folder()->job_chain_folder()->job_chain_or_null( _file_based_job_chain_name ) )
        {
            result = job_chain->file_based_state() >= File_based::s_loaded;
        }
        else
            result = false;
    }

    return result;
}

//--------------------------------------------------------------------------Order::on_load

bool Order::on_load()
{
    return true;
}

//----------------------------------------------------------------------Order::on_activate

bool Order::on_activate()
{
    bool result = false;

    if( Job_chain* job_chain = folder()->job_chain_folder()->job_chain_or_null( _file_based_job_chain_name ) )
    {
        if( job_chain->is_distributed() )  z::throw_xc( "SCHEDULER-384", job_chain->obj_name(), base_file_info()._filename );

        place_or_replace_in_job_chain( job_chain );

        result = true;
    }

    return result;
}

//---------------------------------------------------------Order::order_is_removable_or_replaceable

bool Order::order_is_removable_or_replaceable()
{
    bool result;

    if( _task )
    {
        result = false;
    }
    else
    {
        result = job_chain_path() == ""  ||  
                 is_virgin()             || 
                 end_state_reached();

        if( !result )
        {
            Job_chain* job_chain = order_subsystem()->job_chain_or_null( job_chain_path() );
            result = !job_chain  ||  job_chain->is_to_be_removed();
        }
    }

    return result;
}

//------------------------------------------------------------------------Order::can_be_removed_now

bool Order::can_be_removed_now()
{
    return order_is_removable_or_replaceable();
}

//-----------------------------------------------------------------------------Order::on_remove_now

void Order::on_remove_now()
{
    remove_from_job_chain();

    //if( _order )
    //{
    //    ptr<Order> order = _order;
    //    
    //    _order->connect_with_standing_order( NULL );    // Verbindung zwischen Order und Standing_order lösen,
    //    _order = NULL;                                  // dass remove_job_chain() nicht check_for_replacing_or_removing() rufe!
    //                                        
    //    order->remove_from_job_chain();
    //}
}

//----------------------------------------------------------------------Order::on_requisite_removed

void Order::on_requisite_removed( File_based* )
{
    if( file_based_state() == s_active )  set_file_based_state( s_incomplete );     // Verallgemeinern nach File_based
}

//----------------------------------------------------------------------------Order::on_replace_now

Order* Order::on_replace_now() {
    replacement()->_suspended = _suspended;
    return static_cast<Order*>(My_file_based::on_replace_now());
}

//-----------------------------------------------------------------------------------Order::set_dom
// Wird von folder.cxx aufgerufen

void Order::set_dom( const xml::Element_ptr& element )
{
    assert_is_not_initialized();
    subsystem()->assert_xml_element_name( element );

    set_dom( element, (Variable_set_map*)NULL );

    //if( !job_chain_path().empty() )  z::throw_xc( "SCHEDULER-437", job_chain_path(), _file_based_job_chain_path );     // Order->set_dom() liest Attribut job_chain nicht!

}

//-----------------------------------------------------------------------------------Order::set_dom

void Order::set_dom( const xml::Element_ptr& element, Variable_set_map* variable_set_map )
{
    if( !element )  return;
    if( !element.nodeName_is( "order" )  && 
        !element.nodeName_is( "add_order" ) )  z::throw_xc( "SCHEDULER-409", "order", element.nodeName() );

    clear_source_xml();

    string priority         = element.getAttribute( "priority"  );
    string id               = element.getAttribute( "id"        );
    string title            = element.getAttribute( "title"     );
    string state_name       = element.getAttribute( "state"     );
    string web_service_name = element.getAttribute( "web_service" );
    string at_string        = element.getAttribute( "at" );
    string setback          = element.getAttribute( "setback" );
    _setback_count          = element.int_getAttribute( "setback_count", _setback_count );
    _history_id             = element.int_getAttribute( "history_id"   , _history_id );

    if( element.bool_getAttribute( "replacement" ) )  set_replacement( true );
    _replaced_order_occupator = element.getAttribute( "replaced_order_occupator" );

    if( priority         != "" )  set_priority( as_int(priority) );
    if( id               != "" )  set_id( id.c_str() );

    if( has_base_file() )   // Sicherstellen, das job_chain= und id=, falls vorhanden, mit Angaben im Dateinamen übereinstimmen
    {
        Absolute_path job_chain_path ( folder_path(), element.getAttribute( "job_chain" ) );  // Bei <add_order> vom Schema erlaubt
        if( !job_chain_path.empty()  &&  subsystem()->normalized_path( job_chain_path ) != subsystem()->normalized_path( _file_based_job_chain_path ) )  z::throw_xc( "SCHEDULER-437", job_chain_path, _file_based_job_chain_path );

        if( id != "" &&  id != _file_based_id )  z::throw_xc( "SCHEDULER-436" );     // Bei <add_order> vom Schema erlaubt    }
    }

    if( title            != "" )  set_title   ( title );
    if( state_name       != "" )  set_state   ( state_name.c_str() );
    if( element.hasAttribute( "end_state" ) ) set_end_state( element.getAttribute( "end_state" ) );
    if( web_service_name != "" )  set_web_service( _spooler->_web_services->web_service_by_name( web_service_name ), true );
    _is_virgin = !element.bool_getAttribute( "touched" );

    if( element.hasAttribute( "suspended" ) )
        set_suspended( element.bool_getAttribute( "suspended" ) );

    if( element.hasAttribute( "start_time" ) )  _start_time.set_datetime( element.getAttribute( "start_time" ) );
    if( element.hasAttribute( "end_time"   ) )  _end_time  .set_datetime( element.getAttribute( "end_time"   ) );

    if( setback != "" )  _setback.set_datetime( setback );


    DOM_FOR_EACH_ELEMENT( element, e )  
    {
        /*
        if( e.nodeName_is( "payload" ) )
        {
            xml::Node_ptr node = e.firstChild();
            while( node  &&  node.nodeType() == xml::COMMENT_NODE )  node = node.nextSibling();
            
            if( node )
            {
                if( node.nodeType() != xml::ELEMENT_NODE )  z::throw_xc( "SCHEDULER-239", node.nodeName() );
                Variant payload = ((xml::Element_ptr)node).xml();
                while( node  &&  node.nodeType() == xml::COMMENT_NODE )  node = node.nextSibling();
                if( node )  z::throw_xc( "SCHEDULER-239", node.nodeName() );
            }
        }
        else
        */
        if( e.nodeName_is( "params" ) )
        { 
            set_params( e, variable_set_map );
        }
        else
        if( e.nodeName_is( "payload" ) )
        {
            DOM_FOR_EACH_ELEMENT( e, ee )
            {
                if( ee.nodeName_is( "params"  ) )
                {
                    set_params( ee, variable_set_map );
                    break;
                }
            }
        }
        else
        if( e.nodeName_is( "xml_payload" ) )
        {
            DOM_FOR_EACH_ELEMENT( e, ee )
            {
                set_xml_payload( ee.xml() );
                break;
            }
        }
        else
        if( e.nodeName_is( "run_time" ) )       // Attribut at="..." setzt nächste Startzeit
        { 
            set_schedule( this, e );
        }
        else
        //if( e.nodeName_is( "period" ) )
        //{ 
        //    _period = Period();
        //    _period.set_dom( e, Period::with_date );
        //}
        //else
        if( e.nodeName_is( "log" ) )
        {
            assert( !_log->is_active() );
            _log->continue_with_text( e.text() );
        }
        else
        if( e.nodeName_is( "order.job_chain_stack" ) )
        {
            xml::Element_ptr e2 = e.select_element_strict( "order.job_chain_stack.entry" );
            _outer_job_chain_path = Absolute_path( root_path, e2.getAttribute( "job_chain", _outer_job_chain_path ) );
            _outer_job_chain_state = e2.getAttribute( "state" );

            // Nicht prüfen, weil Äußere Jobkette später definiert werden kann
            //order_subsystem()->job_chain( _outer_job_chain_path )->node_from_state( _outer_job_chain_state );   // Prüfen
        }
    }

    if( at_string != "" )  set_at( Time::time_with_now( at_string ) );
}

//-------------------------------------------------------------Order::set_identification_attributes

void Order::set_identification_attributes( const xml::Element_ptr& result )
{
    result.setAttribute( "job_chain", _job_chain_path != ""? _job_chain_path : _file_based_job_chain_path );
    result.setAttribute( "order"    , string_id()     );
}

//-------------------------------------------------------------------------------Order::dom_element

xml::Element_ptr Order::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what ) 
{
    return dom_element( dom_document, show_what, (const string*)NULL );
}

//-------------------------------------------------------------------------------Order::dom_element

xml::Element_ptr Order::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what, const string* log ) 
{
    assert( this ); if( !this )  z::throw_xc( Z_FUNCTION );


    xml::Element_ptr result = dom_document.createElement( "order" );

    if( _history_id )  result.setAttribute( "history_id", _history_id );

    fill_file_based_dom_element( result, show_what );

    if( !show_what.is_set( show_for_database_only ) )
    {
        if( !_id.is_empty() )
        {
            result.setAttribute( "order"     , debug_string_from_variant( _id ) );
            result.setAttribute( "id"        , debug_string_from_variant( _id ) );     // veraltet
        }
    }

    if( !show_what.is_set( show_for_database_only ) ) // &&  !show_what.is_set( show_id_only ) )
    {
        if( _setback )
            result.setAttribute( "next_start_time", _setback.as_string() );

        if( _schedule_use->is_defined() )   // Wie in Job::dom_element(), besser nach Schedule_use::dom_element()  <schedule.use covering_schedule="..."/>
            if( Schedule* covering_schedule = _schedule_use->schedule()->active_schedule_at( Time::now() ) )  
                if( covering_schedule->is_in_folder() )
                    result.setAttribute( "active_schedule", covering_schedule->path() );

        if( _title != "" )
            result.setAttribute( "title"     , _title );

        if( !_state.is_empty() )
            result.setAttribute( "state"     , debug_string_from_variant( _state ) );

        if( !_initial_state.is_empty() )
            result.setAttribute( "initial_state", debug_string_from_variant( _initial_state ) );

        if( Job_chain* job_chain = this->job_chain_for_api() )
            result.setAttribute( "job_chain" , job_chain->path().with_slash() );

        if( _replaced_by )
            result.setAttribute( "replaced"  , "yes" );
        else
        if( _removed_from_job_chain_path != "" )
            result.setAttribute( "removed"   , "yes" );

        if( Job* job = this->job() )  result.setAttribute( "job", job->path() );
        else
        if( Job_chain* job_chain = job_chain_for_api() )
        {
            if( Job_node* job_node = Job_node::try_cast( job_chain->node_from_state_or_null( _state ) ) )
                result.setAttribute( "job", job_node->job_path() );
        }

        if( _task )
        {
            result.setAttribute( "task"            , _task->id() );   // Kann nach set_state() noch die Vorgänger-Task sein (bis spooler_process endet)
            result.setAttribute( "in_process_since", _task->last_process_start_time().as_string() );
        }

        if( _state_text != "" )
            result.setAttribute( "state_text", _state_text );

        result.setAttribute( "priority"  , _priority );

        if( _created )
            result.setAttribute( "created"   , _created.as_string() );

        if( _log->is_active() )
            result.setAttribute( "log_file"  , _log->filename() );

        if( _is_in_database  &&  _job_chain_path != ""  &&  !_job_chain )
            result.setAttribute( "in_database_only", "yes" );

        if( show_what.is_set( show_payload )  &&  !_payload.is_null_or_empty_string()  &&  !_payload.is_missing() )
        {
            xml::Element_ptr payload_element = result.append_new_element( "payload" );
            xml::Node_ptr    payload_content;

            if( _payload.vt == VT_DISPATCH )
            {
                if( Com_variable_set* variable_set = dynamic_cast<Com_variable_set*>( V_DISPATCH( &_payload ) ) )
                {
                    payload_content = variable_set->dom_element( dom_document, "params", "param" );
                }
            }

            if( !payload_content )
            {
                string payload_string = string_payload();
                if( !payload_content )  payload_content = dom_document.createTextNode( payload_string );
            }

            payload_element.appendChild( payload_content );
        }
    }

    if( show_what.is_set( show_schedule )  &&  _schedule_use->is_defined() )  result.appendChild( _schedule_use->dom_element( dom_document, show_what ) );  // Vor _period setzen!

    //if( show_what.is_set( show_for_database_only ) )
    //{
    //    // Nach <run_time> setzen!
    //    if( _period.repeat() < Time::never ||
    //        _period.absolute_repeat() < Time::never )  result.appendChild( _period.dom_element( dom_document ) );     // Aktuelle Wiederholung merken, für <schedule>
    //}

    if( show_what.is_set( show_log )  ||  show_what.is_set( show_for_database_only ) )
    {
        Show_what log_show_what = show_what;
        if( show_what.is_set( show_for_database_only ) )  log_show_what |= show_log;

        if( log  &&  show_what.is_set( show_log ) ) result.append_new_text_element( "log", *log );     // Protokoll aus der Datenbank
        else
        if( _log )  result.appendChild( _log->dom_element( dom_document, log_show_what ) );
    }

    if( show_what.is_set( show_payload | show_for_database_only )  &&  _xml_payload != "" )
    {
        xml::Element_ptr xml_payload_element = result.append_new_element( "xml_payload" );

        try
        {
            xml::Document_ptr doc ( _xml_payload, scheduler_character_encoding );

            if( doc.documentElement() )
            {
                xml_payload_element.appendChild( doc.documentElement().cloneNode( true ) );
            }
        }
        catch( exception& x )   // Sollte nicht passieren
        {
            _log->error( "xml_payload: " + string(x.what()) );
            append_error_element( xml_payload_element, x );
        }
    }


    // Wenn die folgenden Werte sich ändern, _order_xml_modified = true setzen!

    if( _setback  )
    result.setAttribute( _setback_count == 0? "at" : "setback", _setback.as_string() );

    if( _setback_count > 0 )
    result.setAttribute( "setback_count", _setback_count );

    if( _web_service )
    result.setAttribute( "web_service", _web_service->name() );

    if( _http_operation  &&  _http_operation->web_service_operation_or_null() )
    {
        result.setAttribute( "web_service_operation", _http_operation->web_service_operation_or_null()->id() );
        result.setAttribute( "web_service_client"   , _http_operation->web_service_operation_or_null()->http_operation()->connection()->peer().as_string() );
    }

    if( _is_on_blacklist )  result.setAttribute( "on_blacklist", "yes" );
    if( _suspended       )  result.setAttribute( "suspended"   , "yes" );
    if( _is_replacement  )  result.setAttribute( "replacement" , "yes" ),
                            result.setAttribute_optional( "replaced_order_occupator", _replaced_order_occupator );
    if( !_is_virgin      )  result.setAttribute( "touched"     , "yes" );

    if( start_time()     )  result.setAttribute( "start_time", start_time().as_string() );
    if( end_time()       )  result.setAttribute( "end_time"  , end_time  ().as_string() );

    if( _outer_job_chain_path != "" )
    {
        xml::Element_ptr e  = result.append_new_element( "order.job_chain_stack" );
        xml::Element_ptr e2 = e.append_new_element( "order.job_chain_stack.entry" );
        e2.setAttribute( "job_chain", _outer_job_chain_path );
        e2.setAttribute( "state"    , _outer_job_chain_state.as_string() );
    }
    
    result.setAttribute_optional( "end_state", _end_state.as_string() );

    return result;
}

//--------------------------------------------------------------Order::append_calendar_dom_elements

void Order::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    xml::Node_ptr    node_before     = element.lastChild();

    xml::Element_ptr setback_element = append_calendar_dom_element_for_setback(element, options);
    _schedule_use->append_calendar_dom_elements( element, options );
    xml::Simple_node_ptr node = node_before ? node_before.nextSibling() : element.firstChild();
    set_attributes_and_remove_duplicates( element, node, setback_element );
}

//---------------------------------------------------Order::append_calendar_dom_element_for_setback

xml::Element_ptr Order::append_calendar_dom_element_for_setback(  const xml::Element_ptr& element, Show_calendar_options* options )
{
    xml::Element_ptr setback_element;
    if( is_processable()  &&  !_setback.is_never() && _setback_count > 0 )
    {
       if (_setback >= options->_from && _setback < options->_before) {
         setback_element = new_calendar_dom_element( element.ownerDocument(), _setback );
         element.appendChild( setback_element );
       }
    }
    return setback_element;
}

//-------------------------------------------------------Order::set_attributes_and_remove_duplicate

void Order::set_attributes_and_remove_duplicates(
      const xml::Element_ptr& element, 
      xml::Simple_node_ptr node, 
      xml::Element_ptr setback_element
)
{
    for( node; node; node = node.nextSibling() )
    {
        if( xml::Element_ptr e = xml::Element_ptr( node, xml::Element_ptr::no_xc ) )
        {
            if( setback_element  &&                                           // Duplikat?
                e != setback_element  && 
                e.nodeName_is( setback_element.nodeName() )  &&  
                e.getAttribute( "at" ) == setback_element.getAttribute( "at" ) )
            {
                element.removeChild( setback_element );
                setback_element = NULL;
            }

            if( _job_chain_path != "" )  e.setAttribute( "job_chain", _job_chain_path );
            e.setAttribute( "order", string_id() );
        }
    }
}

//--------------------------------------------------------Order::print_xml_child_elements_for_event

void Order::print_xml_child_elements_for_event( String_stream* s, Scheduler_event* )
{
    *s << "<order";
    
    if( _job_chain )
    *s << " job_chain=\"" << xml::encode_attribute_value( _job_chain_path )      << '"';
    *s << " id=\""        << xml::encode_attribute_value( string_id() )          << '"';

    if( _title != "" )
    *s << " title=\""     << xml::encode_attribute_value( _title )               << '"';

    *s << " state=\""     << xml::encode_attribute_value( _state.as_string() )   << '"';

    *s << "/>";
}

//---------------------------------------------------------------------------------------Order::dom

xml::Document_ptr Order::dom( const Show_what& show_what ) 
{
    xml::Document_ptr document;

    document.create();
    document.appendChild( dom_element( document, show_what ) );

    return document;
}

//-------------------------------------------------------------------------------Order::order_queue

Order_queue* Order::order_queue()
{
    Order_queue* result;

    if( !_job_chain_node )  
    {
        assert( !_is_in_order_queue );
        z::throw_xc( "SCHEDULER-163" );
    }

    if( Order_queue_node* node = Order_queue_node::try_cast( _job_chain_node ) )
    {
        result = node->order_queue();
    }
    else
        z::throw_xc( "SCHEDULER-163" );

    return result;
}

//---------------------------------------------------------------------------Order::set_job_by_name

void Order::set_job_by_name( const Absolute_path& job_path)
{
    set_job( _spooler->job_subsystem()->job( job_path ) );
}

//------------------------------------------------------------------------------------Order::set_id

void Order::set_id( const Order::Id& id )
{
    if( _id_locked  &&  id == _id )
    {
        // Dieselbe Kennung, das ist ok
    }
    else
    {
        if( _id_locked )  z::throw_xc( "SCHEDULER-159" );

        string id_string = string_id( id );    // Sicherstellen, das id in einen String wandelbar ist

        if( db()->opened()  &&  id_string.length() > db()->order_id_length_max() )  
            z::throw_xc( "SCHEDULER-345", id_string, db()->order_id_length_max(), db()->_orders_tablename + "." + "id" );

        if( id_string.length() > const_order_id_length_max )  z::throw_xc( "SCHEDULER-344", id_string, const_order_id_length_max );

        _id = id;

        _log->set_prefix( obj_name() );
        _log->set_title ( "Order " + _id.as_string() );
    }
}

//----------------------------------------------------------------------------Order::set_default_id

void Order::set_default_id()
{
    set_id( _spooler->_db->get_order_id() );
}

//-----------------------------------------------------------------------------Order::set_file_path

void Order::set_file_path( const File_path& path )
{
    string p = path.path();

    set_id( p );
    set_param( scheduler_file_path_variable_name, p );
}

//---------------------------------------------------------------------------------Order::file_path

File_path Order::file_path()
{
    File_path result;

    try
    {
        if( ptr<Com_variable_set> order_params = params_or_null() )
        {
            Variant path;
            order_params->get_Var( Bstr( scheduler_file_path_variable_name ), &path );
            result.set_path( string_from_variant( path ) );
        }
    }
    catch( exception& x )  { Z_LOG2( "scheduler", Z_FUNCTION << " " << x.what() << "\n" ); }

    return result;
}

//-----------------------------------------------------------------------------Order::is_file_order

bool Order::is_file_order()
{
    return file_path() != "";
}

//----------------------------------------------------------------------------Order::string_payload

string Order::string_payload() const
{
    try
    {
        if( Com_variable_set* v = params_or_null() )  
            if( v->is_empty() )  return "";

        return !_payload.is_null_or_empty_string()  &&  !_payload.is_missing()? _payload.as_string() 
                                                                              : "";
    }
    catch( exception& x )
    {
        z::throw_xc( "SCHEDULER-251", x.what() );
    }
}

//--------------------------------------------------------------------------------Order::set_params

void Order::set_params( const xml::Element_ptr& params_element, Variable_set_map* variable_set_map )
{
    ptr<Com_variable_set> params = new Com_variable_set;
    params->register_include_and_set_dom( _spooler, this, params_element, variable_set_map, "param" );    // Kann <include> registrieren
    //pars->set_dom( params_element, variable_set_map );  
    set_payload( Variant( static_cast<IDispatch*>( params ) ) );
}

//----------------------------------------------------------------------------Order::params_or_null

ptr<Com_variable_set> Order::params_or_null() const
{
    ptr<spooler_com::Ivariable_set> result;

    if( _payload.vt != VT_DISPATCH  &&  _payload.vt != VT_UNKNOWN )  return NULL;
    
    IUnknown* iunknown = V_UNKNOWN( &_payload );
    if( iunknown == NULL )  return NULL;

    HRESULT hr = result.Assign_qi( iunknown );
    if( FAILED(hr) )  return NULL;

    return dynamic_cast<Com_variable_set*>( +result );
}

//------------------------------------------------------------------------------------Order::params

ptr<Com_variable_set> Order::params() 
{
    if( _payload.is_null_or_empty_string() )   //is_empty() )
    {
        _payload = new Com_variable_set();
    }

    ptr<Com_variable_set> result = params_or_null();
    if( !result )  z::throw_xc( "SCHEDULER-338" );
    return result;
}

//---------------------------------------------------------------------------------Order::set_param

void Order::set_param( const string& name, const Variant& value )
{
    HRESULT hr;

    //if( _payload.is_empty() )  _payload = new Com_variable_set();

    Variant name_vt = variant_from_string( name );
    hr = params()->put_Value( &name_vt, const_cast<Variant*>( &value ) );
    if( FAILED(hr) )  throw_com( hr, Z_FUNCTION, name );
}

//-------------------------------------------------------------------------------------Order::param

Variant Order::param( const string& name )
{
    Variant result;

    if( ptr<Com_variable_set> params = params_or_null() )  
    {
        HRESULT hr = params->get_Var( Bstr( name ), &result );
        if( FAILED(hr) )  throw_com( hr, Z_FUNCTION, name );
    }

    return result;
}

//---------------------------------------------------------------------------Order::set_web_service

void Order::set_web_service( const string& name, bool force )
{ 
    if( !force  &&  _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );

    _order_xml_modified = true;

    set_web_service( name == ""? NULL 
                               : _spooler->_web_services->web_service_by_name( name ), force );
}

//---------------------------------------------------------------------------Order::set_web_service

void Order::set_web_service( Web_service* web_service, bool force )
{ 
    if( !force  &&  _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );

    _web_service = web_service; 
    _order_xml_modified = true;
}

//-----------------------------------------------------------------------------------Order::set_job

void Order::set_job( Job* job )
{
    if( _removed_from_job_chain_path != "" )
    {
        _log->warn( message_string( "SCHEDULER-298", job->path() ) );   //S() << "job=" << job->name() << " wird ignoriert, weil Auftrag bereits aus der Jobkette entfernt ist" );
    }
    else
    {
        if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );

        move_to_node( _job_chain->node_from_job( job ) );       // Fehler, wenn Job nicht in der Jobkette
    }
}

//---------------------------------------------------------------------------------------Order::job

Job* Order::job() const
{
    Job* result = NULL;

    if( Job_node* job_node = Job_node::try_cast( _job_chain_node ) )  result = job_node->job_or_null();

    return result;
}

//-------------------------------------------------------------------------------Order::set_payload

void Order::set_payload( const VARIANT& payload )
{
    _payload = payload;
}

//---------------------------------------------------------------------------Order::set_xml_payload

void Order::set_xml_payload( const string& xml_string )
{ 
    //Z_LOGI2( "scheduler.order", obj_name() << ".xml_payload=" << xml_string << "\n" );

    if( xml_string == "" )
    {
        _xml_payload = "";
    }
    else
    {
        xml::Document_ptr doc ( xml_string, scheduler_character_encoding );

        set_xml_payload( doc.documentElement() );
    }
}

//---------------------------------------------------------------------------Order::set_xml_payload

void Order::set_xml_payload( const xml::Element_ptr& element )
{ 
    _xml_payload = element? element.xml() : "";     // _xml_payload kann in order_xml <order> eingefügt werden, unabhängig von der Codierung (ist nur 7bit-Ascii)
    _order_xml_modified = true; 
}

//----------------------------------------------------------------------------------Order::finished

bool Order::finished()
{
    return !_suspended  &&  end_state_reached();
}

//-------------------------------------------------------------------------Order::end_state_reached

bool Order::end_state_reached()
{
    bool result = false;

    if( _end_state_reached )  result = true;
    else
    if( Job_chain* job_chain = this->job_chain() )
    {
        if( Node* node = job_chain->node_from_state_or_null( _state ) )
        {
            if( node->is_type( Node::n_end ) )  result = true;
        }
        else 
            result = true;
    }
    
    return result;
}

//-------------------------------------------------------------------------------Order::check_state

void Order::check_state( const State& state )
{
    try
    {
        string_from_variant( state );
    }
    catch( exception& x )
    {
        z::throw_xc( "SCHEDULER-250", x.what() );
    }
}

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state, const Time& start_time )
{
    set_state( state );
    set_setback( start_time );
}

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state )
{
    check_state( state );

    clear_setback();
    if( _is_on_blacklist )  remove_from_blacklist();
    set_state1( state );
}

//--------------------------------------------------------------------------------Order::set_state1

void Order::set_state1( const State& order_state )
{
    check_state( order_state );

    if( _job_chain )
    {
        ptr<Order>  hold_me        = this;
        State       previous_state = _state;
        Node*       node           = NULL;
        
        if( !order_state.is_empty() )
        {
            node = _job_chain->referenced_node_from_state( order_state );
            if( node != _job_chain->node_from_state( order_state ) )  _log->info( message_string( "SCHEDULER-859", node->order_state().as_string(), order_state ) );
        }

        move_to_node( node );

        if( previous_state != _state  &&  ( !_job_chain_node  ||  _job_chain_node->is_type( Node::n_end ) ) )
        {
            handle_end_state();
        }
    }
    else  
        set_state2( order_state );
}

//--------------------------------------------------------------------------------Order::set_state2

void Order::set_state2( const State& order_state, bool is_error_state )
{
    if( order_state != _state )
    {
        State previous_state = _state;
        _state = order_state;

        if( _job_chain )
        {
            string log_line = "set_state " + ( order_state.is_missing()? "(missing)" : order_state.as_string() );

            if( Job_node* job_node = Job_node::try_cast( _job_chain_node ) ) log_line += ", Job " + job_node->job_path();
            if( is_error_state )  log_line += ", error state";

            if( _setback )  log_line += ", at=" + _setback.as_string();

            if( _suspended )  log_line += ", suspended";

            _log->info( log_line );
        }

        if (is_in_job_chain())
        {
            report_event( OrderStateChangedEventJ::new_instance(java_sister(), OrderStateJ::new_instance(previous_state.as_string())) );

            Scheduler_event event ( evt_order_state_changed, log_info, this );
            _spooler->report_event( &event );
        }
    }

    if( !_initial_state_set )  _initial_state = order_state,  _initial_state_set = true;
}

//-------------------------------------------------------------------------------------Order::reset

void Order::reset()
{
    // Für http://www.sos-berlin.com/jira/browse/JS-305 

    assert_no_task( Z_FUNCTION );

    set_suspended( false );
    clear_setback();
    set_state( _initial_state );
    set_next_start_time();
}

//-----------------------------------------------------------------------------Order::set_end_state

void Order::set_end_state( const State& end_state )                  
{ 
    if( !end_state.is_null_or_empty_string() )
    {
        if( Job_chain* job_chain = this->job_chain() )  job_chain->referenced_node_from_state( end_state );       // Prüfen
    }

    _end_state = end_state;
    _order_xml_modified = true;
}

//------------------------------------------------------------------------Order::set_job_chain_node

void Order::set_job_chain_node( Node* node, bool is_error_state )
{
    _job_chain_node = node;

    if( node )
    {
        if( node->is_suspending_order() )  set_suspended();
        if( node->delay()  &&  !at() )  set_at_after_delay( Time::now() + node->delay() );
    }

    set_state2( node? node->order_state() : empty_variant, is_error_state );
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Node* node )
{
    if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );

    bool       is_same_node = node == _job_chain_node;
    ptr<Order> hold_me      = this;

    if( _task )  _moved = true;

    if( !is_same_node  &&  _job_chain_node  &&  _is_in_order_queue )
    {
        order_queue()->remove_order( this );
        _job_chain_node = NULL;
    }

    set_job_chain_node( node );

    if( !is_same_node  &&  !_is_distributed )
        if( Order_queue_node* order_queue_node = Order_queue_node::try_cast( node ) )
            order_queue_node->order_queue()->add_order( this );
}

//------------------------------------------------------------------------------Order::set_priority

void Order::set_priority( Priority priority )
{
    if( _priority != priority )
    {
        _priority = priority;

        if( _is_in_order_queue  &&  !_task )   // Nicht gerade in Verarbeitung?
        {
            order_queue()->reinsert_order( this );
        }
    }

    _priority_modified = true;
}

//-----------------------------------------------------------------------------------Order::com_job

Com_job* Order::com_job()
{
    Com_job* result = NULL;

    Job* j = job();
    if( j )  result = j->com_job();

    return result;
}

//------------------------------------------------------------------------------------Order::remove

void Order::remove( File_based::Remove_flag remove_flag )
{
    ptr<Order> hold_me = this;

    if( is_in_folder() )  My_file_based::remove( remove_flag );
    // Löscht erst im Endzustand, deshalb noch remove_from_job_chain() rufen

    remove_from_job_chain(); 
}

//---------------------------------------------------------------------Order::remove_from_job_chain

void Order::remove_from_job_chain( Job_chain_stack_option job_chain_stack_option, Transaction* outer_transaction )
{
    ptr<Order> me = this;        // Halten

    if( !_end_time )  _end_time = Time::now();

    if( _job_chain_path != "" )
    {
        if( _job_chain || _is_in_database )     // Nur Protokollieren, wenn wirklich etwas entfernt wird, aus Jobkette im Speicher oder Datenbank (_is_distributed)
        {
            _log->info( _task? message_string( "SCHEDULER-941", _task->obj_name() ) 
                             : message_string( "SCHEDULER-940" ) );
        }

        db_delete( update_and_release_occupation, outer_transaction );     // Schreibt auch die Historie (auch bei orders_recoverable="no")

        // wird beim order remove aufgerufen und sollte evtl. ein eigenes event sein
        fire_event_if_finished();
    }

    assert( !_is_db_occupied );

    if( _job_chain )  _job_chain->remove_order( this );

    _setback_count = 0;
    _setback = Time(0);

    set_replacement( false );  


    _job_chain = NULL;
    _job_chain_path.clear();


    if( job_chain_stack_option == jc_remove_from_job_chain_stack )  
    {
        remove_from_job_chain_stack();
        if( !_task )  close();      // 2007-09-16
    }
}

//---------------------------------------------------------------Order::remove_from_job_chain_stack

void Order::remove_from_job_chain_stack()
{
    _outer_job_chain_path.clear();
    _outer_job_chain_state.clear();
}

//------------------------------------------------------------------------Order::place_in_job_chain

void Order::place_in_job_chain( Job_chain* job_chain,  Job_chain_stack_option job_chain_stack_option )
{
    bool exists_exception = true;
    bool ok = try_place_in_job_chain( job_chain, job_chain_stack_option, exists_exception );
    assert( ok );
    //if( !ok )  z::throw_xc( "SCHEDULER-186", obj_name(), job_chain->path() );
}

//----------------------------------------------------------------------Order::try_place_in_job_chain

bool Order::try_place_in_job_chain( Job_chain* job_chain, Job_chain_stack_option job_chain_stack_option, bool exists_exception )
{
    job_chain->assert_is_loaded();

    // Sollte mit assert_is_loaded() überflüssig geworden sein:
    if( job_chain->state() != Job_chain::s_loaded  &&  
        job_chain->state() != Job_chain::s_active  )  z::throw_xc( "SCHEDULER-151" );
      //job_chain->state() != Job_chain::s_stopped 

    if( _outer_job_chain_path == ""  &&  !_end_state.is_null_or_empty_string() )  job_chain->referenced_node_from_state( _end_state );       // Prüfen

    bool is_new = true;

    if( _id.vt == VT_EMPTY )  set_default_id();
    _id_locked = true;


    if( _is_replacement  &&  job_chain->is_distributed() )
    {
        // Bestehender Auftrag wird in der Datenbank ersetzt (_is_replacement steuert das)
    }
    else
    {
        Job_chain* other_job_chain = job_chain;

        if( job_chain->_order_id_space )
        {
            Order* other_order = job_chain->_order_id_space->order_or_null( string_id() );
            is_new = !other_order  ||  other_order == this && job_chain->subsystem()->normalized_path( _job_chain_path ) != job_chain->normalized_path();  // is_new, wenn Auftragskennung neu oder derselbe Auftrag nicht in job_chain ist.
        }
        else
        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
        {
            is_new = !job_chain->has_order_id( &ta, id() );
        }
        catch( exception& x ) { ta.reopen_database_after_error( x, Z_FUNCTION ); }

        if( !is_new  &&  exists_exception )  z::throw_xc( "SCHEDULER-186", obj_name(), other_job_chain->path().to_string() );
    }

    if( is_new )
    {
        ptr<Order> hold_me = this;   // Halten für remove_from_job_chain()
        
        if( _job_chain_path != "" )  remove_from_job_chain( job_chain_stack_option );
        assert( !_job_chain );
        
        if( _state.vt == VT_EMPTY )  
        {
            // Auftrag bekommt Zustand des ersten Jobs der Jobkette. 
            set_state2( job_chain->first_node()->order_state() );     
            //2007-07-30 Besser so?: set_job_chain_node( job_chain->first_node() );     // Node._action, ._is_suspending_order und ._delay berücksichtigen
        }

        Node* node = job_chain->node_from_state( _state );


        if( job_chain_stack_option == jc_remove_from_job_chain_stack )  remove_from_job_chain_stack();

        if( Nested_job_chain_node* n = Nested_job_chain_node::try_cast( node ) )
        {
            if( _outer_job_chain_path != "" )  z::throw_xc( "SCHEDULER-412" );      // Mehrfache Verschachtelung nicht möglich (sollte nicht passieren, wird schon vorher geprüft)

            _outer_job_chain_path = Absolute_path( root_path, job_chain->path() );
            _outer_job_chain_state = _state;
            job_chain = n->nested_job_chain();
            _state = job_chain->first_node()->order_state();    // S.a. handle_end_state_repeat_order(). Auftrag bekommt Zustand des ersten Jobs der Jobkette
        }


        if( !job_chain->node_from_state( _state )->is_type( Node::n_order_queue ) )  z::throw_xc( "SCHEDULER-438", _state );

        if( node->is_suspending_order() )  {
           _suspended = true;   
           fire_event_suspended();
        }

        if( !_is_distribution_inhibited  &&  job_chain->is_distributed() )  set_distributed();

        _job_chain_path = job_chain->path();
        _removed_from_job_chain_path.clear();

        activate_schedule();     // Errechnet die nächste Startzeit

        if( _delay_storing_until_processing ) 
        {
            if( _is_distributed )  assert(0), z::throw_xc( Z_FUNCTION, "_delay_storing_until_processing & _is_distributed not possible" );   // db_try_insert() muss Datenbanksatz prüfen können
        }
        else
        if( job_chain->_orders_are_recoverable  &&  !_is_in_database )
        {
            if( db()->opened() )  is_new = db_try_insert( exists_exception );       // false, falls aus irgendeinem Grund die Order-ID schon vorhanden ist
        }

        if( is_new  &&  !_is_distributed )
        {
            job_chain->add_order( this );
        }
    }

    assert( !exists_exception || is_new );
    return is_new;
}

//-------------------------------------------------------------Order::place_or_replace_in_job_chain

void Order::place_or_replace_in_job_chain( Job_chain* job_chain )
{
    assert( job_chain );
    Z_DEBUG_ONLY( order_subsystem()->order_id_spaces()->self_check() );


    if( job_chain->is_distributed() )
    {
        set_replacement( true );
        place_in_job_chain( job_chain );
    }
    else
    {
        if( ptr<Order> other_order = job_chain->_order_id_space? job_chain->_order_id_space->order_or_null( string_id() )
                                                               : job_chain->order_or_null( id() ) )  // Nicht aus der Datenbank gelesen
        {
            if( other_order != this ) {
                other_order->remove_from_job_chain();
                place_in_job_chain( job_chain );
            } else {
                activate_schedule();  // Wird sonst von place_in_job_chain ausgeführt // SOS-1219
            }

            if( other_order->_task )
            {
                set_replacement( other_order );
                _replaced_order_occupator = other_order->_task->obj_name();
                _log->info( message_string( "SCHEDULER-942", _replaced_order_occupator, other_order->obj_name() ) );       // add_or_replace_order(): Auftrag wird verzögert bis <p1/> <p2/> ausgeführt hat
            }
            //else
            //    other_order->close();       // 2007-09-16
        }
        else
        {
            place_in_job_chain( job_chain );
        }
    }
}

//-------------------------------------------------------------------------Order::activate_schedule

void Order::activate_schedule()
{
    bool ok = _schedule_use->try_load();
    if( ok )  
    {
        if( _setback )  set_setback( _setback );
                  else  set_next_start_time();
    }
}

//-----------------------------------------------------------------------Order::set_next_start_time

void Order::set_next_start_time()
{
    if( _state == _initial_state )  // &&  !_setback )
    {
        if( _schedule_use->is_defined() )
        {
            set_setback( next_start_time( true ) );     // Braucht für <schedule start_time_function=""> das Scheduler-Skript
        }
        else
            set_setback( Time::never );
    }
    else
    {
        set_setback( _setback );
    }
}

//-----------------------------------------------Order::tip_own_job_for_new_distributed_order_state

bool Order::tip_own_job_for_new_distributed_order_state()
{
    bool result = false;

    if( is_processable() )
    {
        result = job_chain()->tip_for_new_distributed_order( _state, at() );
    }

    return result;
}

//---------------------------------------------------------------------------------Order::job_chain

Job_chain* Order::job_chain() const
{ 
    return _job_chain           ? _job_chain : 
           _job_chain_path == ""? NULL 
                                : order_subsystem()->job_chain( _job_chain_path );
}

//---------------------------------------------------------------------------------Order::job_chain

Job_chain* Order::job_chain_for_api() const
{ 
    return  _job_chain                        ? _job_chain :
            _removed_from_job_chain_path != ""? order_subsystem()->job_chain_or_null( _removed_from_job_chain_path ) :
            _job_chain_path              != ""? order_subsystem()->job_chain_or_null( _job_chain_path ) 
                                              : NULL; 
}

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( Postprocessing_mode postprocessing_mode )
{
    _is_success_state = postprocessing_mode == post_success;

    Job*      last_job          = _task? _task->job() : NULL;
    Job_node* job_node          = Job_node::cast( _job_chain_node );
    bool      force_error_state = false;

    if( !_is_success_state  &&  job_node  &&  job_node->is_on_error_setback() )  setback();

    if( !_setback_called )  _setback_count = 0;

    if( !_suspended  &&  _setback == Time::never  &&  _setback_count > _task->job()->max_order_setbacks() )
    {
        _log->info( message_string( "SCHEDULER-943", _setback_count ) );   // " mal zurückgestellt. Der Auftrag wechselt in den Fehlerzustand"
        _is_success_state = false;
        force_error_state = true;
        _setback = Time(0);
        _setback_count = 0;
    }

    if( _task  &&  _moved  &&  job_node )
        if( Job* job = job_node->job_or_null() )  job->signal( "delayed set_state()" );

    _task = NULL;

    if( postprocessing_mode != post_keep_state  &&  !is_setback()  &&  !_moved  &&  !_end_state_reached  ||  
        force_error_state )
    {
        if( job_node )
        {
            assert( _job_chain );

            if( !_is_success_state  &&  job_node->is_on_error_suspend() )  
                set_suspended();
            else
            if( _is_success_state )
            {
                if( _outer_job_chain_path == ""  &&  _state == _end_state )
                {
                    log()->info( message_string( "SCHEDULER-704", _end_state ) );
                    set_end_state_reached();
                    handle_end_state();
                }
                else
                    set_state1( job_node->next_state() );
            }
            else
                set_state1( job_node->error_state() );
        }
    }
    else
    if( _end_state_reached )
    {
        handle_end_state();
    }
    else
    if( _is_distributed  &&  _is_in_order_queue )
    {
        order_queue()->remove_order( this );
    }
    else
    if( !_moved )
    {
        if( _job_chain_node  &&  _job_chain_node->action() == Node::act_next_state )
            clear_setback();

        set_state1( _state );       // Node._action, ._is_suspending_order und ._delay berücksichtigen
    }

    postprocessing2( last_job );
}

//--------------------------------------------------------------------------Order::fire_event_if_finished

void Order::fire_event_if_finished()
{
    // Endzustand erreicht. 
    if( finished() ) {
        report_event( OrderFinishedEventJ::new_instance(java_sister()) );
    }
}

//--------------------------------------------------------------------------Order::fire_event_suspended

void Order::fire_event_suspended()
{
    if( _suspended ) {
        report_event( OrderSuspendedEventJ::new_instance(java_sister()) );
    }
}

//--------------------------------------------------------------------------Order::fire_event_resumed

void Order::fire_event_resumed()
{
    // Endzustand erreicht. 
    if( !_suspended ) {
        report_event( OrderResumedEventJ::new_instance(java_sister()) );
    }
}

//--------------------------------------------------------------------------Order::handle_end_state

void Order::handle_end_state()
{
    // Endzustand erreicht. 
    // Möglicherweise nur der Endzustand in einer verschachtelten Jobkette. Dann beachten wir die übergeordnete Jobkette.
    bool is_real_end_state = false;


    if( _outer_job_chain_path == "" )
    {
        is_real_end_state = true;
    }
    else
    {
        is_real_end_state = handle_end_state_of_nested_job_chain();
    }

    if( !is_real_end_state )
    {
        _end_state_reached = false;
    }
    else
    {
        bool  is_first_call = false; //_schedule_modified;
        Time  next_start    = next_start_time( is_first_call );
        State s             = _outer_job_chain_path != ""? _outer_job_chain_state : _state;

        if( ( has_base_file()  ||  next_start != Time::never  ||  _schedule_use->is_incomplete() )  &&   // <schedule> verlangt Wiederholung?
           (s != _initial_state || _initial_state == _end_state) )   // JS-730  
        {
            _is_virgin = true;
            handle_end_state_repeat_order( next_start );
        }
        else
        {
            if( _job_chain )
            {
                if( is_file_order()  &&  file_path().file_exists() )
                {
                    _log->error( message_string( "SCHEDULER-340" ) );  // Auslösende Datei darf nach Auftragsende nicht mehr da sein.
                    set_on_blacklist();
                }
                
                if( _suspended )
                {
                    set_on_blacklist();
                }
            }

            _end_time = Time::now();

            if( !_is_on_blacklist )
            {
                _log->info( message_string( "SCHEDULER-945" ) );     // "Kein weiterer Job in der Jobkette, der Auftrag ist erledigt"
                remove_from_job_chain();
                close();
            }
        }

        on_carried_out();   // Kann Auftrag aus der Jobkette nehmen
    }
}

//------------------------------------------------------Order::handle_end_state_of_nested_job_chain

bool Order::handle_end_state_of_nested_job_chain()
{
    Z_DEBUG_ONLY( assert( !_is_distributed ) );

    bool end_state_reached = false;

    try
    {
        if( _outer_job_chain_state == _end_state )
        {
            log()->info( message_string( "SCHEDULER-704", _end_state ) );
            end_state_reached = true;
        }
        else
        {
            Job_chain* outer_job_chain            = order_subsystem()->job_chain( _outer_job_chain_path );
            Node*      outer_job_chain_node       = outer_job_chain->node_from_state( _outer_job_chain_state );
            State      next_outer_job_chain_state = _is_success_state? outer_job_chain_node->next_state() 
                                                                     : outer_job_chain_node->error_state();

            Nested_job_chain_node* next_outer_job_chain_node  = Nested_job_chain_node::try_cast( outer_job_chain->node_from_state_or_null( next_outer_job_chain_state ) );

            if( next_outer_job_chain_node  &&  next_outer_job_chain_node->nested_job_chain() )
            {
                Job_chain* previous_job_chain = _job_chain;
                Job_chain* next_job_chain     = next_outer_job_chain_node->nested_job_chain();

                _log->info( message_string( "SCHEDULER-862", next_job_chain->obj_name() ) );

                close_log_and_write_history();// Historie schreiben, aber Auftrag beibehalten
                _start_time = 0;
                _end_time = 0;
                open_log();

                _state.clear();     // Lässt place_in_job_chain() den ersten Zustand der Jobkette nehmen
                place_in_job_chain( next_job_chain, jc_leave_in_job_chain_stack );  // Entfernt Auftrag aus der bisherigen Jobkette
                _outer_job_chain_path = Absolute_path( root_path, outer_job_chain->path() );  // place_in_job_chain() hat's gelöscht

                _log->info( message_string( "SCHEDULER-863", previous_job_chain->obj_name() ) );
            }
            else
            {
                // Bei <schedule> Auftrag an den Anfang der ersten Jobkette setzen
                end_state_reached = true;
            }

            _outer_job_chain_state = next_outer_job_chain_state;
        }
    }
    catch( exception& x ) 
    { 
        _log->error( message_string( "SCHEDULER-415", x ) );  
        remove_from_job_chain();
        close();
        //end_state_reached = true;
    }

    return end_state_reached;
}

//-------------------------------------------------------------Order::handle_end_state_repeat_order

void Order::handle_end_state_repeat_order( const Time& next_start )
{
    // Auftrag wird wegen <schedule> wiederholt

    Absolute_path first_nested_job_chain_path;

    if( _outer_job_chain_path != "" )
    {
        if( Nested_job_chain_node* n = Nested_job_chain_node::try_cast( order_subsystem()->active_job_chain( _outer_job_chain_path )->node_from_state( _initial_state ) ) )
            first_nested_job_chain_path = n->nested_job_chain_path();

        remove_from_job_chain( jc_leave_in_job_chain_stack );  
    }

    _log->info( message_string( "SCHEDULER-944", _initial_state, next_start ) );        // "Kein weiterer Job in der Jobkette, der Auftrag wird mit state=<p1/> wiederholt um <p2/>"
    
    close_log_and_write_history();  // Historie schreiben, aber Auftrag beibehalten
    _start_time = 0;
    _end_time   = 0;

    try
    {
        if( first_nested_job_chain_path != "" )
        {
            _outer_job_chain_state = _initial_state;
            Job_chain* nested_job_chain = order_subsystem()->job_chain( first_nested_job_chain_path );
            Order::State first_nested_state = nested_job_chain->first_node()->order_state();  // Auftrag bekommt Zustand des ersten Jobs der Jobkette
            set_state( first_nested_state, next_start );
            place_in_job_chain( nested_job_chain, jc_leave_in_job_chain_stack );
        }
        else
            set_state( _initial_state, next_start );
    }
    catch( exception& x ) { _log->error( x.what() ); }


    open_log();     // Erst jetzt, weil _job_chain_path gesetzt sein muss
}

//----------------------------------------------------------------------------Order::on_carried_out

void Order::on_carried_out()
{
    order_subsystem()->count_finished_orders();

    check_for_replacing_or_removing( act_now );     // Kann Auftrag aus der Jobkette nehmen
}

//--------------------------------------------------------------------------Order::processing_error

void Order::processing_error()
{
    Job* last_job = _task? _task->job() : NULL;

    _task = NULL;

    if( _http_operation )      
    {
        _job_chain_node = NULL;         // Nicht auf Neustart des Jobs warten, sondern Auftrag beenden, damit die Web-Service-Operation abgeschlossen werden kann
    }

    postprocessing2( last_job );
}

//---------------------------------------------------------------------------Order::postprocessing2

void Order::postprocessing2( Job* last_job )
{
    Job* job = this->job();

    if( _moved  &&  job  &&  !order_queue()->has_immediately_processable_order() )
    {
        job->signal( "Order (delayed set_state)" );
    }

    _moved = false;


    if( finished() )
    {
        try
        {
            if( _web_service  &&  !_http_operation )
            {
                _web_service->forward_order( this, last_job );
            }
        }
        catch( exception& x )  { _log->error( x.what() ); }
    }
    
    fire_event_if_finished();


    if( _job_chain )  // 2008-03-07  &&  _is_in_database )
    {
        try
        {
            db_update( update_and_release_occupation );
        }
        catch( exception& x ) { _log->error( message_string( "SCHEDULER-313", x ) ); }
    }

    if( _is_distributed  &&  _job_chain )
    {
        _job_chain->remove_order( this );
        close();
    }

    if( _removed_from_job_chain_path != "" )
    {
        close();
    }

    if( _job_chain )  _job_chain->check_for_replacing_or_removing();
}

//-----------------------------------------------------------------------------Order::set_suspended

void Order::set_suspended( bool suspended )
{
    if( _suspended != suspended )
    {
        _suspended = suspended;
        _order_xml_modified = true;

        if( _job_chain )
        {
            if( _is_on_blacklist  &&  !suspended )  remove_from_job_chain();
            else
            if( _is_in_order_queue )  order_queue()->reinsert_order( this );

            if( _suspended )  {
               _log->info( message_string( "SCHEDULER-991" ) );
               fire_event_suspended();
            } else {
               _log->info( message_string( "SCHEDULER-992", _setback ) );
               fire_event_resumed();
            }
        }

        handle_changed_processable_state();
    }
}

//---------------------------------------------------------------------------------Order::start_now

void Order::start_now()
{
    set_at( 0 );
}

//-----------------------------------------------------------------------------------Order::setback

bool Order::setback()
{
    bool result;

    if( !_task      )  z::throw_xc( "SCHEDULER-187" );
    if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
    if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );
    if( !order_queue() )  z::throw_xc( "SCHEDULER-163", obj_name() );

    _setback_count++;
    _setback_called = true;

    int maximum = _task->job()->max_order_setbacks();
    if( _setback_count <= maximum )
    {
        Time delay = _task->job()->get_delay_order_after_setback( _setback_count );
        _setback = delay? Time::now() + delay : Time(0);
        _log->info( message_string( "SCHEDULER-946", _setback_count, _setback ) );   // "setback(): Auftrag zum $1. Mal zurückgestellt, bis $2"
        result = true;
    }
    else
    {
        _setback = Time::never;  // Das heißt: Der Auftrag kommt in den Fehlerzustand
        _log->info( message_string( "SCHEDULER-947", _setback_count, maximum ) );   // "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, ""das ist über dem Maximum " + as_string(maximum) + " des Jobs" );
        result = false;
    }

    order_queue()->reinsert_order( this );

    // Weitere Verarbeitung in postprocessing()

    return result;
}

//-------------------------------------------------------------------------------Order::set_setback

void Order::set_setback( const Time& start_time_, bool keep_setback_count )
{
    Time start_time = start_time_ == 0  ||  start_time_ > Time::now()? start_time_ 
                                                                     : Time(0);

    if( _setback != start_time )
    {
        _setback = start_time;
        _order_xml_modified = true;
        if( _is_in_order_queue )  order_queue()->reinsert_order( this );

        handle_changed_processable_state();
    }


    if( !keep_setback_count )  _setback_count = 0;
}

//-----------------------------------------------------------------------------Order::clear_setback

void Order::clear_setback( bool keep_setback_count )
{
    if( _setback_count > 0 )
    {
        set_setback( 0, keep_setback_count );
    }
}

//------------------------------------------------------------------------------------Order::set_at

void Order::set_at( const Time& time )
{
    assert_no_task( Z_FUNCTION );
    if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );

    set_setback( time );
}

//-------------------------------------------------------------------------Order::set_at_after_delay

void Order::set_at_after_delay( const Time& time )
{
    assert_no_task( Z_FUNCTION );
    if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );

    set_setback( time, true );
}

//--------------------------------------------------------------------------------Order::next_time

Time Order::next_time()
{
    return is_processable()? _setback 
                           : Time::never;
}

//---------------------------------------------------------------------------Order::next_start_time
// first_call: false, wenn Order endet oder nicht gestartet wurde
// first_call: true, bevor Order gestartet wird oder sich die Konfigurationsdatei der Order ändert

Time Order::next_start_time( bool first_call )
{
    Time result = Time::never;

    if( _schedule_use->is_defined() )
    {
        Time now = Time::now();

        _schedule_use->log_changed_active_schedule( now );

        if( first_call )
        {
            _period = _schedule_use->next_period( now, schedule::wss_next_any_start );

            if( !_period.absolute_repeat().is_never() )
            {
                result = _period.next_repeated( now );

                if( result.is_never() )
                {
                    _period = _schedule_use->next_period( _period.end(), schedule::wss_next_any_start );
                    result = _period.begin();
                }
            }
            else
            {
                result = _period.begin();
            }
        }
        else
        {
            result = _period.next_repeated_allow_after_end( now );

            if( result >= _period.end() )       // Periode abgelaufen?
            {
                Period next_period = _schedule_use->next_period( _period.end(), schedule::wss_next_any_start );
                
                if (result.is_never()) 
                        result = next_period.begin();  // SOS1219 next_period.begin() kann never sein!??
                else
                if (result >= next_period.end()) { // Nächste Periode ist auch abgelaufen?
                    next_period = _schedule_use->next_period(next_period.end(), schedule::wss_next_any_start);
                    result = next_period.begin();
                }
                else
                if( !next_period.is_seamless_repeat_of(_period))
                    result = next_period.begin();  // Perioden sind nicht nahtlos: Wiederholungsintervall neu berechnen
                else {
                    // Perioden gehen nahtlos ineinander über und in result berechneter repeat-Abstand bleibt erhalten.
                }

                _period = next_period;
            }


            // Aber gibt es ein single_start vorher?

            Period next_single_start_period = _schedule_use->next_period( now, schedule::wss_next_single_start );
            if( next_single_start_period._single_start  &&  result > next_single_start_period.begin() )
            {
                _period = next_single_start_period;
                result  = next_single_start_period.begin();
            }
        }

        if( result < now )  result = 0;
    }

    return result;
}

//------------------------------------------------------------------------Order::on_schedule_loaded

void Order::on_schedule_loaded()
{
    handle_changed_schedule();
}

//----------------------------------------------------------------------Order::on_schedule_modified

void Order::on_schedule_modified()
{
    handle_changed_schedule();

    //if( _state == _initial_state )  set_setback( _schedule->set()? next_start_time( true ) : Time(0) );
    //                         else  _schedule_modified = true;
}

//-----------------------------------------------------------------Order::on_schedule_to_be_removed

bool Order::on_schedule_to_be_removed()
{
    _schedule_use->disconnect();
    handle_changed_schedule();
    return true;
}

//-------------------------------------------------------------------Order::handle_changed_schedule

void Order::handle_changed_schedule()
{
    if (subsystem()->subsystem_state() == subsys_active) {  // JS-576
        _period = _schedule_use->is_defined()? _schedule_use->next_period( Time::now(), schedule::wss_next_any_start )
                                             : Period();
    
        if( is_virgin() )
        {
            //_setback = 0;           // Änderung von <run_time> überschreibt Order.at
            set_next_start_time();      // Änderung von <run_time> überschreibt Order.at
        }
    }
}

//------------------------------------------------------------------------------Order::set_schedule

void Order::set_schedule( File_based* source_file_based, const xml::Element_ptr& e )
{
    _schedule_use->set_dom( source_file_based, e );       // Ruft add_depandent() auf.
    // Bereits aufgerufen von _schedule_use->set_dom(): on_schedule_modified();
}

//------------------------------------------------------------------------------Order::schedule_use

Schedule_use* Order::schedule_use()
{
    return +_schedule_use;
}

//---------------------------------------------------------------------------Order::set_replacement

void Order::set_replacement( Order* replaced_order )
{
    // Bei verschachtelten Jobketten (in einer Order_id_space verbunden) können die 
    // zwei Aufträge in verschiedenen Jobketten sein.

    assert( !replaced_order->_replaced_by );
    assert( !replaced_order->_is_replacement );

    set_replacement( replaced_order != NULL );
    _replacement_for = replaced_order;
    _replacement_for->_replaced_by = this;
}

//---------------------------------------------------------------------------Order::set_replacement

void Order::set_replacement( bool b )
{
    bool was_replacement = _is_replacement;

    _replaced_order_occupator = "";

    if( _replacement_for )
    {
        _replacement_for->_replaced_by = NULL;
        _replacement_for = NULL;
    }

    if( b != _is_replacement )
    {
        _is_replacement = b;
        _order_xml_modified = true;

        if( !_is_replacement )  _replacement_for = NULL;
    }

    if( was_replacement != _is_replacement )  handle_changed_processable_state();
}

//--------------------------------------------------------------------------Order::set_on_blacklist

void Order::set_on_blacklist()
{
    if( !_job_chain )  assert(0), z::throw_xc( Z_FUNCTION, "no _job_chain" );        // Wenn _is_distributed

    _job_chain->add_order_to_blacklist( this );

    _is_on_blacklist    = true;
    _order_xml_modified = true;

    handle_changed_processable_state();
}

//---------------------------------------------------------------------Order::remove_from_blacklist

void Order::remove_from_blacklist()
{
    if( _is_on_blacklist )
    {
        assert( _job_chain );
        _job_chain->remove_order_from_blacklist( this );
        _is_on_blacklist = false;
    
        handle_changed_processable_state();
    }
}

//-------------------------------------------------------------------------------Order::web_service

Web_service* Order::web_service() const
{
    Web_service* result = web_service_or_null();
    if( !result )  z::throw_xc( "SCHEDULER-240" );
    return result;
}

//---------------------------------------------------------------------Order::web_service_operation

Web_service_operation* Order::web_service_operation() const
{
    Web_service_operation* result = web_service_operation_or_null();
    if( !result )  z::throw_xc( "SCHEDULER-246" );
    return result;
}

//---------------------------------------------------------------------------Order::order_subsystem

Order_subsystem_impl* Order::order_subsystem() const
{
    return static_cast<Order_subsystem_impl*>( _spooler->order_subsystem() );
}

//----------------------------------------------------------------------------------Order::obj_name

string Order::obj_name() const
{
    S result;
    
    result << "Order ";
    if( Job_chain* job_chain = this->job_chain_for_api() )  result << job_chain->path().without_slash() << ":";
    result << debug_string_from_variant(_id);
    if( _title != "" )  result << " " << quoted_string( _title );

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace spoooler
} //namespace sos
