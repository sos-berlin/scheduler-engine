// $Id$
/*
    Hier sind implementiert

    Spooler::add_job_chain
    Spooler::job_chain
    Spooler::xml_from_job_chains

    Job_chain
    Order
    Order_queue
*/


#include "spooler.h"
#include "../zschimmer/z_sql.h"
#include "../zschimmer/xml.h"

using stdext::hash_set;
using stdext::hash_map;

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

const int    check_database_orders_period               = 15;
const int    check_database_orders_period_minimum       = 1;
const int    database_orders_read_ahead_count           = 1;
const int    max_insert_race_retry_count                = 5;                            // Race condition beim Einfügen eines Datensatzes

//--------------------------------------------------------------------------------------------const

const string now_database_distributed_next_time         = "2000-01-01 00:00:00";        // Auftrag ist verteilt und ist sofort ausführbar
const string never_database_distributed_next_time       = "3111-11-11 00:00:00";        // Auftrag ist verteilt, hat aber keine Startzeit (weil z.B. suspendiert)
const string blacklist_database_distributed_next_time   = "3111-11-11 00:01:00";        // Auftrag ist auf der schwarzen Liste
const string replacement_database_distributed_next_time = "3111-11-11 00:02:00";        // <order replacement="yes">
const string default_end_state_name                     = "<END_STATE>";
const string order_select_database_columns              = "`id`, `priority`, `state`, `state_text`, `initial_state`, `title`, `created_time`";

//----------------------------------------------------------------------------------Order_subsystem

struct Order_subsystem : Order_subsystem_interface
{

                                Order_subsystem             ( Spooler* );


    // Subsystem

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();


    // Order_subsystem_interface

    void                        load_job_chains_from_xml    ( const xml::Element_ptr& );
    void                        add_job_chain               ( Job_chain* );
    void                        remove_job_chain            ( Job_chain* );
    void                        check_exception             ();

    void                        request_order               ();
    ptr<Order>                  load_order_from_database    ( Transaction*, const string& job_chain_name, const Order::Id&, Load_order_flags );
    ptr<Order>              try_load_order_from_database    ( Transaction*, const string& job_chain_name, const Order::Id&, Load_order_flags );

    Job_chain*                  job_chain                   ( const string& name );
    Job_chain*                  job_chain_or_null           ( const string& name );
    xml::Element_ptr            job_chains_dom_element      ( const xml::Document_ptr&, const Show_what& );
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    int                         finished_orders_count       () const                                { return _finished_orders_count; }


    // Privat

    void                        close_job_chains            ();

    void                        load_orders_from_database   ();
    bool                        are_orders_distributed      ();
    bool                        is_job_in_any_job_chain     ( Job* );
    bool                        is_job_in_any_distributed_job_chain( Job* );
    string                      job_chain_db_where_condition( const string& job_chain_name );
    string                      order_db_where_condition    ( const string& job_chain_name, const string& order_id );
    void                        count_started_orders        ();
    void                        count_finished_orders       ();
    int                         job_chain_map_version       () const                                { return _job_chain_map_version; }


    Fill_zero                  _zero_;
    typedef map< string, ptr<Job_chain> >  Job_chain_map;
    Job_chain_map              _job_chain_map;
    int                        _job_chain_map_version;             // Zeitstempel der letzten Änderung (letzter Aufruf von Spooler::add_job_chain()), 
  //long32                     _next_free_order_id;

  private:
    ptr<Database_order_detector> _database_order_detector;
    int                        _started_orders_count;
    int                        _finished_orders_count;
};

//--------------------------------------------------------------------------Database_order_detector

struct Database_order_detector : Async_operation, Scheduler_object
{
                                Database_order_detector     ( Spooler* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    // Scheduler_operation
    Scheduler_object::obj_name;


    string                      make_union_select_order_sql ( const string& select_sql_begin, const string& select_sql_end );
    string                      make_where_expression_for_distributed_orders_at_job( Job* );
    bool                        is_job_requesting_order_then_calculate_next     ( Job* );
    int                         read_result_set             ( Read_transaction*, const string& select_sql );
    void                        set_alarm                   ();
    void                        request_order               ();

  private:
    Fill_zero                  _zero_;
    Time                       _now;                        // Zeitpunkt von async_continue_()
    time_t                     _now_utc;
    Time                       _now_database_distributed_next_time;
    Time                       _never_database_distributed_next_time;
    Time                       _blacklist_database_distributed_next_time;
};

//-------------------------------------------------Database_order_detector::Database_order_detector

Database_order_detector::Database_order_detector( Spooler* spooler ) 
:
    _zero_(this+1),
    Scheduler_object( spooler, this, Scheduler_object::type_database_order_detector )
{
    _now_database_distributed_next_time  .set_datetime( now_database_distributed_next_time   );
    _never_database_distributed_next_time.set_datetime( never_database_distributed_next_time );
    _blacklist_database_distributed_next_time.set_datetime( blacklist_database_distributed_next_time );
}

//---------------------------------------------------------Database_order_detector::async_finished_
    
bool Database_order_detector::async_finished_() const
{ 
    return false;
}

//-------------------------------------------------------Database_order_detector::async_state_text_

string Database_order_detector::async_state_text_() const
{
    S result;
    S jobs;
    time_t now = 0;

    result << obj_name();

    FOR_EACH_JOB( j )
    {
        Job* job = *j;
        if( job->order_queue() ) 
        {
            time_t t = job->order_queue()->next_distributed_order_check_time();
            if( t < time_max )
            {
                if( !jobs.empty() )  jobs << ", ";
                jobs << job->name();
                jobs << " ";
                if( t == 0 )  jobs << "0";
                else
                {
                    if( !now )  now = ::time(NULL);
                    int64 seconds = now - t;
                    if( seconds > 0 )  jobs << "+";
                    jobs << seconds;    // Normalerweise negativ (noch verbleibende Zeit)
                }
                //if( t )  jobs << string_gmt_from_time_t( t ) << " UTC";
                //   else  jobs << "now";
                jobs << "s";
            }
        }
    }

    if( !jobs.empty() )  result << "  requesting jobs: " << jobs;
        
    return result;
}

//---------------------------------------------------------Database_order_detector::async_continue_

bool Database_order_detector::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.order", __FUNCTION__ << "  " << async_state_text() << "\n" );
    _spooler->assert_are_orders_distributed( __FUNCTION__ );


    _now     = Time::now();
    _now_utc = ::time(NULL);

  //Time read_until             = _now + check_database_orders_period;
    int  announced_orders_count = 0;

    S select_sql_begin;
    select_sql_begin << "select ";
    if( database_orders_read_ahead_count < INT_MAX ) select_sql_begin << " %limit(" << database_orders_read_ahead_count << ") ";
    select_sql_begin << "`distributed_next_time`, `job_chain`, `state`"
                    "  from " << _spooler->_orders_tablename <<
                    "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                       " and `distributed_next_time` is not null"
                       " and `occupying_cluster_member_id` is null";

    string select_sql_end = "  order by `distributed_next_time`";

    
    bool database_can_limit_union_selects = db()->dbms_kind() == dbms_oracle  ||  
                                            db()->dbms_kind() == dbms_oracle_thin;
    // PostgresQL: Union kann nicht Selects mit einzelnen Limits verknüpfen, das Limit gilt fürs ganze Ergebnis,
    // und die einzelnen Selects können nicht geordnet werden (wodurch die Limits erst Sinn machen)

    for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
    {
        if( database_can_limit_union_selects )
        {
            string select_sql = make_union_select_order_sql( select_sql_begin, select_sql_end );
            if( select_sql != "" )
            {
                announced_orders_count +=
                read_result_set( &ta, select_sql );
            }
        }
        else
        {
            FOR_EACH_JOB( job_iterator )
            {
                Job* job = *job_iterator;
                
                if( is_job_requesting_order_then_calculate_next( job ) )
                {
                    string job_chain_expression = make_where_expression_for_distributed_orders_at_job( job );

                    if( job_chain_expression != "" )
                    {
                        announced_orders_count +=
                        read_result_set( &ta, S() << select_sql_begin + " and " + job_chain_expression << select_sql_end );
                    }
                }
            }
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), __FUNCTION__ ); }

    //if( announced_orders_count )  on_new_order();


    //_now_utc = ::time(NULL);

    //FOR_EACH_JOB( job_iterator )
    //{
    //    if( Order_queue* order_queue = (*job_iterator)->order_queue() )
    //    {
    //        order_queue->calculate_next_distributed_order_check_time( _now_utc );
    //    }
    //}

    set_alarm();

    return true;
}

//---------------------------------------------Database_order_detector::make_union_select_order_sql

string Database_order_detector::make_union_select_order_sql( const string& select_sql_begin, const string& select_sql_end )
{
    S    result;
    bool has_any_job = false;

    FOR_EACH_JOB( job_iterator )
    {
        Job* job = *job_iterator;

        if( is_job_requesting_order_then_calculate_next( job ) )
        {
            string job_chains_expression = make_where_expression_for_distributed_orders_at_job( job );

            if( job_chains_expression != "" )
            {
                if( has_any_job )  result << "  UNION  ";
                has_any_job = true;
                result << select_sql_begin << " and " << job_chains_expression << select_sql_end;
            }
        }
    }

    return result;
}

//-----------------------------Database_order_detector::is_job_requesting_order_then_calculate_next

bool Database_order_detector::is_job_requesting_order_then_calculate_next( Job* job )
{
    bool result = false;

    if( Order_queue* order_queue = job->order_queue() )
    {
        if( order_queue->is_distributed_order_requested( _now_utc ) )
        {
            order_queue->calculate_next_distributed_order_check_time( _now_utc );

            result = order_queue->next_announced_distributed_order_time() > _now;
        }
    }

    return result;
}

//---------------------Database_order_detector::make_where_expression_for_distributed_orders_at_job

string Database_order_detector::make_where_expression_for_distributed_orders_at_job( Job* job )
{
    S result;
                  
    result << job->order_queue()->db_where_expression_for_distributed_orders();

    if( !result.empty() )
    {
        Time t = job->order_queue()->next_announced_distributed_order_time();
        assert( t );

        result << " and `distributed_next_time` < {ts'" 
               << ( t < Time::never? t.as_string( Time::without_ms ) 
                                   : never_database_distributed_next_time ) 
               << "'}";
    }

    return result;
}

//---------------------------------------------------------Database_order_detector::read_result_set

int Database_order_detector::read_result_set( Read_transaction* ta, const string& select_sql )
{
    int      count      = 0;
    Any_file result_set = ta->open_result_set( select_sql, __FUNCTION__ );

    while( !result_set.eof() )
    {
        Record     record    = result_set.get_record();
        Job_chain* job_chain = order_subsystem()->job_chain( record.as_string( "job_chain" ) );
        Job*       job       = job_chain->job_from_state( record.as_string( "state" ) );
        Time       distributed_next_time;

        distributed_next_time.set_datetime( record.as_string( "distributed_next_time" ) );
        if( distributed_next_time == _now_database_distributed_next_time   )  distributed_next_time.set_null();
        if( distributed_next_time >= _never_database_distributed_next_time )  distributed_next_time.set_never();
        
        bool is_now = distributed_next_time <= _now;
        job->order_queue()->set_next_announced_distributed_order_time( distributed_next_time, is_now );
        
        //_names_of_requesting_jobs.erase( job->name() );
        count += is_now;
    }

    return count;
}

//---------------------------------------------------------------Database_order_detector::set_alarm

void Database_order_detector::set_alarm()
{
    time_t next_alarm = time_max;

    FOR_EACH_JOB( j )
    {
        Job* job = *j;

        if( Order_queue* order_queue = job->order_queue() )
        {
            time_t t = order_queue->next_distributed_order_check_time();
            if( next_alarm > t )  next_alarm = t;
        }
    }

    set_async_next_gmtime( next_alarm );
}

//-----------------------------------------------------------Database_order_detector::request_order

void Database_order_detector::request_order()

// Nur einmal für einen Job rufen, solange der Job keinen neuen Auftrag bekommen hast! Order_queue::request_order() kümmert sich darum.

{
    set_alarm();
}

//------------------------------------------------------------------------------new_order_subsystem

ptr<Order_subsystem_interface> new_order_subsystem( Scheduler* scheduler )
{
    ptr<Order_subsystem> order_subsystem = Z_NEW( Order_subsystem( scheduler ) );
    return +order_subsystem;
}

//---------------------------------------------Order_subsystem_interface::Order_subsystem_interface

Order_subsystem_interface::Order_subsystem_interface( Scheduler* scheduler ) 
: 
    Subsystem( scheduler, Scheduler_object::type_order_subsystem )
{
}

//-----------------------------------------------------------------Order_subsystem::Order_subsystem

Order_subsystem::Order_subsystem( Spooler* spooler )
:
    Order_subsystem_interface( spooler ),
    _zero_(this+1)
{
}

//---------------------------------------------------------------------------Order_subsystem::close

void Order_subsystem::close()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    close_job_chains();
}

//----------------------------------------------------------------------Order_subsystem::initialize

bool Order_subsystem::subsystem_initialize()
{
    init_file_order_sink( _spooler );

    _subsystem_state = subsys_initialized;
    return true;
}

//------------------------------------------------------------------Order_subsystem::subsystem_load

bool Order_subsystem::subsystem_load()
{
    load_orders_from_database();

    _subsystem_state = subsys_loaded;
    return true;
}

//-------------------------------------------------------Order_subsystem::load_orders_from_database

void Order_subsystem::load_orders_from_database()
{
    if( db()->opened()  &&  _spooler->has_exclusiveness() )
    {
        Read_transaction ta ( db() );

        FOR_EACH( Job_chain_map, _job_chain_map, it ) 
        {
            Job_chain* job_chain = it->second;
            if( job_chain->_load_orders_from_database )
            {
                job_chain->add_orders_from_database( &ta );  // Die Jobketten aus der XML-Konfiguration
            }
        }
    }
}

//--------------------------------------------------------------Order_subsystem::subsystem_activate

bool Order_subsystem::subsystem_activate()
{
    _subsystem_state = subsys_active;  // Jetzt schon aktiv für die auszuführenden Skript-Funktionen <run_time start_time_function="">


    Z_FOR_EACH( Job_chain_map, _job_chain_map, it ) 
    {
        Job_chain* job_chain = it->second;
        job_chain->activate();
    }

    if( are_orders_distributed() )
    {
        _database_order_detector = Z_NEW( Database_order_detector( _spooler ) );
        _database_order_detector->set_async_manager( _spooler->_connection_manager );
        _database_order_detector->async_wake();
    }

    return true;
}

//----------------------------------------------------------------Order_subsystem::close_job_chains

void Order_subsystem::close_job_chains()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    for( Job_chain_map::iterator j = _job_chain_map.begin(); j != _job_chain_map.end(); j++ )  //j = _job_chain_map.erase( j ) )
    {
        j->second->close(); 
    }
}

//----------------------------------------------------------Order_subsystem::job_chains_dom_element

xml::Element_ptr Order_subsystem::job_chains_dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr job_chains_element = document.createElement( "job_chains" );

    job_chains_element.setAttribute( "count", (int)_job_chain_map.size() );

    if( show.is_set( show_job_chains | show_job_chain_jobs | show_job_chain_orders ) )
    {
        FOR_EACH( Job_chain_map, _job_chain_map, it )
        {
            Job_chain* job_chain = it->second;
            if( job_chain->visible() )
            {
                job_chains_element.appendChild( job_chain->dom_element( document, show ) );
            }
        }
    }

    return job_chains_element;
}

//----------------------------------------------------Order_subsystem::append_calendar_dom_elements

void Order_subsystem::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    FOR_EACH( Job_chain_map, _job_chain_map, it )
    {
        if( options->_count >= options->_limit )  break;

        Job_chain* job_chain = it->second;

        if( !are_orders_distributed()  ||  job_chain->is_distributed() )
            job_chain->append_calendar_dom_elements( element, options );
    }


    if( options->_count < options->_limit  &&  are_orders_distributed()  
     &&  _spooler->db()  &&  _spooler->db()->opened()  &&  !_spooler->db()->is_in_transaction() )
    {
        Read_transaction ta ( _spooler->db() );

        S select_sql;
        select_sql << "select %limit(" << ( options->_limit - options->_count ) << ") " 
                   << order_select_database_columns << ", `job_chain`"
                      "  from " << _spooler->_orders_tablename <<
                    "  where `spooler_id`=" << sql::quoted(_spooler->id_for_db());
        if(  options->_from              )  select_sql << " and `distributed_next_time` >= {ts'" << options->_from   << "'}";
        if( !options->_before.is_never() )  select_sql << " and `distributed_next_time` < {ts'"  << options->_before << "'}";
        else
        if( !options->_from              )  select_sql << " and `distributed_next_time` is not null ";
        
        //select_sql << "  order by `distributed_next_time`";

        Any_file result_set = ta.open_result_set( select_sql, __FUNCTION__ ); 

        while( options->_count < options->_limit  &&  !result_set.eof() )
        {
            Record record = result_set.get_record();

            try
            {
                string job_chain_name = record.as_string( "job_chain" );

                ptr<Order> order = new Order( _spooler, record, job_chain_name );
                order->load_order_xml_blob( &ta );
                
                if( order->run_time() )
                    order->run_time()->append_calendar_dom_elements( element, options );
            }
            catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << "  " << x.what() << "\n" ); }  // Auftrag kann inzwischen gelöscht worden sein
        }
    }
}

//-------------------------------------------------------------------Order_subsystem::add_job_chain

void Order_subsystem::add_job_chain( Job_chain* job_chain )
{
    try
    {
        job_chain->finish();   // Jobkette prüfen und in Ordnung bringen

        string lname = lcase( job_chain->name() );
        if( _job_chain_map.find( lname ) != _job_chain_map.end() )  z::throw_xc( "SCHEDULER-160", lname );

        _job_chain_map[lname] = job_chain;
        _job_chain_map_version++;

        job_chain->set_state( Job_chain::s_ready );

        /*
        THREAD_LOCK( _prioritized_order_job_array )
        {
            // In _prioritized_order_job_array stehen Jobs, die am Ende einer Jobkette sind, am Anfang, so dass sie vorrangig ausgeführt werden können.
            // Ein Aufträg in einer Jobkette soll so schnell wie möglich durchgeschleust werden, bevor andere Aufträge in die Jobkette gelangen.
            // Damit sind weniger Aufträge gleichzeitig in einer Jobkette.

            _prioritized_order_job_array.clear();
            FOR_EACH_JOB( it )  if( (*it)->order_controlled() )  _prioritized_order_job_array.push_back( *it );
            sort( _prioritized_order_job_array.begin(), _prioritized_order_job_array.end(), Job::higher_job_chain_priority );
        }
        */


        if( _spooler->has_exclusiveness()  &&  job_chain->_orders_recoverable  &&  !job_chain->is_distributed() )
        {
            if( !_spooler->db()  ||  !_spooler->db()->opened() )  // Beim Start des Schedulers
            {
                job_chain->_load_orders_from_database = true;
            }
            else
            {
                for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
                {
                    job_chain->add_orders_from_database( &ta );
                }
                catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), __FUNCTION__ ); }
            }
        }


        job_chain->_order_sources.start();
    }
    catch( exception&x )
    {
        _log->error( x.what() );
        throw;
    }
}

//----------------------------------------------------------------Order_subsystem::remove_job_chain

void Order_subsystem::remove_job_chain( Job_chain* job_chain )
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

#   ifdef Z_DEBUG
        //assert( job_chain->_order_map.empty() );
        assert( job_chain->_blacklist_map.empty() );
#   endif

    for( Job_chain_map::iterator j = _job_chain_map.begin(); j != _job_chain_map.end(); j++ )
    {
        if( j->second == job_chain )  
        { 
            _job_chain_map.erase( j );  
            _job_chain_map_version++;
            break; 
        }
    }
}

//--------------------------------------------------------Order_subsystem::load_job_chains_from_xml

void Order_subsystem::load_job_chains_from_xml( const xml::Element_ptr& element )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "job_chain" ) )
        {
            // Siehe auch Command_processor::execute_job_chain()
            ptr<Job_chain> job_chain = new Job_chain( _spooler );
            job_chain->set_dom( e );
            add_job_chain( job_chain );
        }
    }
}

//---------------------------------------------------------------Order_subsystem::job_chain_or_null

Job_chain* Order_subsystem::job_chain_or_null( const string& name )
{
    Job_chain* result = NULL;

    string lname = lcase( name );

    Job_chain_map::iterator it = _job_chain_map.find( lname );
    if( it == _job_chain_map.end() )  result = NULL;
                                else  result = it->second;

    return result;
}

//-----------------------------------------------------------------------Order_subsystem::job_chain

Job_chain* Order_subsystem::job_chain( const string& name )
{
    Job_chain* result = job_chain_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-161", name );

    return result;
}

//---------------------------------------------------------Order_subsystem::is_job_in_any_job_chain

bool Order_subsystem::is_job_in_any_job_chain( Job* job )
{
    Z_FOR_EACH( Job_chain_map, _job_chain_map, jc )
    {
        Job_chain* job_chain = jc->second;
        if( job_chain->contains_job( job ) )  return true;
    }

    return false;
}

//---------------------------------------------Order_subsystem::is_job_in_any_distributed_job_chain

bool Order_subsystem::is_job_in_any_distributed_job_chain( Job* job )
{
    if( are_orders_distributed() ) 
    {
        Z_LOG2( "joacim", __FUNCTION__ << "  " << job->obj_name() << "\n" );    // Diese Routine ist nicht so effizient

        Z_FOR_EACH( Job_chain_map, _job_chain_map, jc )
        {
            Job_chain* job_chain = jc->second;
            if( job_chain->_is_distributed  &&  job_chain->contains_job( job ) )  return true;
        }
    }

    return false;
}

//--------------------------------------------------------Order_subsystem::load_order_from_database

ptr<Order> Order_subsystem::load_order_from_database( Transaction* outer_transaction, const string& job_chain_name, const Order::Id& order_id, Load_order_flags flag )
{
    ptr<Order> result = try_load_order_from_database( outer_transaction, job_chain_name, order_id, flag );

    if( !result )  z::throw_xc( "SCHEDULER-162", order_id.as_string(), job_chain_name );

    return result;
}

//----------------------------------------------------Order_subsystem::try_load_order_from_database

ptr<Order> Order_subsystem::try_load_order_from_database( Transaction* outer_transaction, const string& job_chain_name, const Order::Id& order_id, Load_order_flags flag )
{
    ptr<Order> result;

    for( Retry_nested_transaction ta ( _spooler->_db, outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        S select_sql;
        select_sql <<  "select " << order_select_database_columns << ", `occupying_cluster_member_id`"
                       "  from " << _spooler->_orders_tablename;
        if( flag & lo_lock )  select_sql << " %update_lock";
        select_sql << "  where " << order_db_where_condition( job_chain_name, order_id.as_string() );

        if( flag & lo_blacklisted )  select_sql << " and `distributed_next_time`={ts'" << blacklist_database_distributed_next_time << "'}";
                               else  select_sql << " and `distributed_next_time` is not null";

        Any_file result_set = ta.open_result_set( select_sql, __FUNCTION__ );

        if( !result_set.eof() )
        {
            Record record = result_set.get_record();
            result = new Order( _spooler, record, job_chain_name );
            result->set_distributed();
            if( !record.null( "occupying_cluster_member_id" ) )  z::throw_xc( "SCHEDULER-379", result->obj_name(), record.as_string( "occupying_cluster_member_id" ) );

            try
            {
                result->load_blobs( &ta );
            }
            catch( exception& ) 
            { 
                result = NULL;  // Jemand hat wohl den Auftrag gelöscht
            }      
        }
    }
    catch( exception& x ) 
    { 
        if( result )  result->close( Order::cls_dont_remove_from_job_chain ),  result = NULL;
        ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), __FUNCTION__ ); 
    }

    return result;
}

//----------------------------------------------------------Order_subsystem::are_orders_distributed

bool Order_subsystem::are_orders_distributed()
{
    return _spooler->are_orders_distributed();
}

//-------------------------------------------------------------------Order_subsystem::request_order

void Order_subsystem::request_order()
{
    if( _database_order_detector )
    {
        _database_order_detector->request_order();
    }
}

//-----------------------------------------------------------------Order_subsystem::check_exception

void Order_subsystem::check_exception()
{
    if( _database_order_detector )  _database_order_detector->async_check_exception();
}

//--------------------------------------------------------Order_subsystem::order_db_where_condition

string Order_subsystem::order_db_where_condition( const string& job_chain_name, const string& order_id )
{
    S result;

    result << job_chain_db_where_condition( job_chain_name ) << " and `id`="  << sql::quoted( order_id );

    return result;
}

//----------------------------------------------------Order_subsystem::job_chain_db_where_condition

string Order_subsystem::job_chain_db_where_condition( const string& job_chain_name )
{
    S result;

    result << "`spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
         " and `job_chain`="  << sql::quoted( job_chain_name );

    return result;
}

//------------------------------------------------------------Order_subsystem::count_started_orders

void Order_subsystem::count_started_orders()
{
    _started_orders_count++;
    //_spooler->update_console_title();
}

//-----------------------------------------------------------Order_subsystem::count_finished_orders

void Order_subsystem::count_finished_orders()
{
    _finished_orders_count++;
    _spooler->update_console_title( 2 );
}

//-----------------------------------------------------------------------Order_source::Order_source

Order_source::Order_source( Job_chain* job_chain, Scheduler_object::Type_code t ) 
: 
    Scheduler_object( job_chain->_spooler, static_cast<Object*>(this), t ),
    _zero_(this+1),
    _job_chain( job_chain )
{
}

//--------------------------------------------------------------------------------Order_source::log
    
Prefix_log* Order_source::log()
{ 
    assert( _job_chain );
    return _job_chain->log(); 
}

//-----------------------------------------------------------------------------Order_source::finish

void Order_source::finish()
{
    if( !_job_chain )  z::throw_xc( __FUNCTION__ );

    if( _next_state.is_missing() )  _next_state = _job_chain->first_node()->_state;
    _next_order_queue = _job_chain->node_from_state( _next_state )->_job->order_queue();    // Ist nicht NULL
}

//-----------------------------------------------------------------------------Order_sources::close

void Order_sources::close()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->close();
    }
}

//----------------------------------------------------------------------------Order_sources::finish

void Order_sources::finish()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->finish();
    }
}

//-----------------------------------------------------------------------------Order_sources::start

void Order_sources::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->start();
    }
}

//---------------------------------------------------------------------Order_sources::request_order

//Order* Order_sources::request_order( const string& cause )
//{
//    Order* result = NULL;
//
//    Z_FOR_EACH( Order_source_list, _order_source_list, it )
//    {
//        Order_source* order_source = *it;
//        result = order_source->request_order( cause );
//        if( result )  break;
//    }
//
//    if( result )  assert( result->is_immediately_processable() );
//
//    return result;
//}

//-------------------------------------------------------------xml::Element_ptr Job_chain_node::xml

xml::Element_ptr Job_chain_node::dom_element( const xml::Document_ptr& document, const Show_what& show, Job_chain* job_chain )
{
    Read_transaction ta ( _job_chain->_spooler->_db );

    xml::Element_ptr element = document.createElement( "job_chain_node" );

        element.setAttribute( "state", debug_string_from_variant( _state ) );

        if( !is_file_order_sink() )
        {
            if( !_next_state.is_empty()  )  element.setAttribute( "next_state" , debug_string_from_variant( _next_state  ) );
            if( !_error_state.is_empty() )  element.setAttribute( "error_state", debug_string_from_variant( _error_state ) );
        }

        if( is_file_order_sink() )
        {
            xml::Element_ptr file_order_sink_element = document.createElement( "file_order_sink" );

            if( _file_order_sink_remove )  file_order_sink_element.setAttribute( "remove", "yes" );
            file_order_sink_element.setAttribute_optional( "move_to", _file_order_sink_move_to );

            element.appendChild( file_order_sink_element );
        }

        if( _job )
        {
            element.setAttribute( "orders", order_count( &ta, job_chain ) );
            element.setAttribute( "job"   , _job->name() );
            
            //if( _job->order_queue()->is_order_requested() )
            //    element.setAttribute( "is_order_requested", "yes" );

            if( show.is_set( show_job_chain_jobs ) )
            {
                dom_append_nl( element );
                element.appendChild( _job->dom_element( document, show, job_chain ) );
                dom_append_nl( element );
            }
            else
            if( show.is_set( show_job_chain_orders ) )
            {
                // Nur Aufträge im Job zeigen, sonst nichts vom Job (der wird bereits von <show_state> in <jobs> gezeigt)
                xml::Element_ptr job_element = document.createElement( "job" );
                job_element.setAttribute( "name", _job->name() );

                element.appendChild( job_element );
                job_element.appendChild( _job->order_queue()->dom_element( document, show | show_orders, job_chain ) );
            }
        }

    return element;
}

//----------------------------------------------------------------------Job_chain_node::order_count

int Job_chain_node::order_count( Read_transaction* ta, Job_chain* job_chain )
{
    return _job? _job->order_queue()->order_count( ta, job_chain ) : 0;
}

//-----------------------------------------------------------------------------Job_chain::Job_chain

Job_chain::Job_chain( Spooler* spooler )
:
    Com_job_chain( this ),
    Scheduler_object( spooler, static_cast<spooler_com::Ijob_chain*>( this ), type_job_chain ),
    _zero_(this+1),
    _orders_recoverable(true),
    _visible(true)
{
    set_name( "" );     // Ruft log()->set_prefix()
}

//----------------------------------------------------------------------------Job_chain::~Job_chain

Job_chain::~Job_chain()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << ": " << x.what() << '\n' ); }
}

//---------------------------------------------------------------------------------Job_chain::close

void Job_chain::close()
{
    Z_LOGI2( "scheduler", obj_name() << ".close()\n" );

    remove_all_pending_orders( true );

    _blacklist_map.clear();
    _order_sources.close();
    set_state( s_closed );
}

//----------------------------------------------------------------------------Job_chain::state_name

string Job_chain::state_name( State state )
{
    switch( state )
    {
        case s_under_construction:  return "under_construction";
        case s_ready:               return "ready";
        case s_removing:            return "removing";
        default:                    return S() << "State(" << state << ")";
    }
}

//-------------------------------------------------------------------------------Job_chain::set_dom

void Job_chain::set_dom( const xml::Element_ptr& element )
{
    if( !element )  return;

    set_name(             element.     getAttribute( "name" ) );
    _visible            = element.bool_getAttribute( "visible"           , _visible            );
    _orders_recoverable = element.bool_getAttribute( "orders_recoverable", _orders_recoverable );

    if( order_subsystem()->are_orders_distributed() )
    _is_distributed     = element.bool_getAttribute( "distributed"       , _is_distributed     );

    if( _is_distributed  &&  !_orders_recoverable )  z::throw_xc( message_string( "SCHEDULER-380", obj_name() ) );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "file_order_source" ) )      // Wegen _is_on_blacklist und _is_virgin
        {
            ptr<Directory_file_order_source_interface> d = new_directory_file_order_source( this, e );
            _order_sources._order_source_list.push_back( +d );
        }
        else
        if( e.nodeName_is( "file_order_sink" ) )
        {
            string state = e.getAttribute( "state" );

            bool can_be_not_initialized = true;
            Job* job = _spooler->job_subsystem()->get_job( file_order_sink_job_name, can_be_not_initialized );
            job->set_visible( true );

            Job_chain_node* node = add_job( job, state, Variant(Variant::vt_missing), Variant(Variant::vt_missing) );

            node->_file_order_sink_move_to.set_directory( subst_env( e.getAttribute( "move_to" ) ) );
            node->_file_order_sink_remove  = e.bool_getAttribute( "remove" );
            node->_delay = e.int_getAttribute( "delay", node->_delay );
        }
        else
        if( e.nodeName_is( "job_chain_node" ) )
        {
            string job_name = e.getAttribute( "job" );
            string state    = e.getAttribute( "state" );

            bool can_be_not_initialized = true;
            Job* job = job_name == ""? NULL : _spooler->job_subsystem()->get_job( job_name, can_be_not_initialized );
            if( state == "" )  z::throw_xc( "SCHEDULER-231", "job_chain_node", "state" );

            Job_chain_node* node = add_job( job, state, e.getAttribute( "next_state" ), e.getAttribute( "error_state" ) );

            if( e.bool_getAttribute( "suspend", false ) )  node->_suspend = true;
            node->_delay = e.int_getAttribute( "delay", node->_delay );
        }
    }
}

//----------------------------------------------------------xml::Element_ptr Job_chain::dom_element

xml::Element_ptr Job_chain::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    Read_transaction ta ( _spooler->_db );

    Show_what modified_show = show;
    if( modified_show.is_set( show_job_chain_orders ) )  modified_show |= show_orders;


    xml::Element_ptr element = document.createElement( "job_chain" );

    //THREAD_LOCK( _lock )
    {
        element.setAttribute( "name"  , _name );
        element.setAttribute( "orders", order_count( &ta ) );
        element.setAttribute( "state" , state_name( state() ) );
        if( !_visible ) element.setAttribute( "visible", _visible );
        element.setAttribute( "orders_recoverable", _orders_recoverable );

        if( _state >= s_ready )
        {
            FOR_EACH( Order_sources::Order_source_list, _order_sources._order_source_list, it )
            {
                Order_source* order_source = *it;
                element.appendChild( order_source->dom_element( document, modified_show ) );
            }

            FOR_EACH( Chain, _chain, it )
            {
                Job_chain_node* node = *it;
                element.appendChild( node->dom_element( document, modified_show, this ) );
            }
        }


        if( show.is_set( show_order_history )  &&  _spooler->_db->opened() )
        {
            xml::Element_ptr order_history_element = document.createElement( "order_history" );

            try
            {
                Read_transaction ta ( _spooler->_db );

                Any_file sel = ta.open_result_set( 
                               "select %limit(20) \"ORDER_ID\" as \"ID\", \"HISTORY_ID\", \"JOB_CHAIN\", \"START_TIME\", \"END_TIME\", \"TITLE\", \"STATE\", \"STATE_TEXT\""
                               " from " + _spooler->_order_history_tablename +
                               " where \"JOB_CHAIN\"=" + sql::quoted( _name ) +
                                 " and \"SPOOLER_ID\"=" + sql::quoted( _spooler->id_for_db() ) +
                               " order by \"HISTORY_ID\" desc",
                               __FUNCTION__ );

                while( !sel.eof() )
                {
                    Record record = sel.get_record();

                    ptr<Order> order = new Order( _spooler );
                    order->set_id        ( record.as_string( "id"         ) );
                    order->set_state     ( record.as_string( "state"      ) );
                    order->set_state_text( record.as_string( "state_text" ) );
                    order->set_title     ( record.as_string( "title"      ) );
                    order->_start_time.set_datetime( record.as_string( "start_time" ) );
                    order->_end_time  .set_datetime( record.as_string( "end_time"   ) );

                    xml::Element_ptr order_element = order->dom_element( document, show );
                    order_element.setAttribute_optional( "job_chain" , record.as_string( "job_chain"  ) );
                    order_element.setAttribute         ( "history_id", record.as_string( "history_id" ) );

                    order_history_element.appendChild( order_element );
                }
            }
            catch( exception& x )
            {
                order_history_element.appendChild( create_error_element( document, x, 0 ) );
            }

            element.appendChild( order_history_element );
        }


        if( !_blacklist_map.empty() )
        {
            xml::Element_ptr blacklist_element = document.createElement( "blacklist" );
            blacklist_element.setAttribute( "count", (int)_blacklist_map.size() );

            if( show.is_set( show_blacklist ) )
            {
                Z_FOR_EACH( Blacklist_map, _blacklist_map, it )
                {
                    Order* order = it->second;
                    blacklist_element.appendChild( order->dom_element( document, modified_show ) );
                }
            }

            element.appendChild( blacklist_element );
        }
    }

    return element;
}

//----------------------------------------------------------Job_chain::append_calendar_dom_elements

void Job_chain::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    FOR_EACH( Order_map, _order_map, it )
    {
        if( options->_count >= options->_limit )  break;

        Order* order = it->second;
        order->append_calendar_dom_elements( element, options );
    }


    //if( _is_distributed  &&  options->_count < options->_limit )
    //{
    //    Ist bereit von Order_subsystem::append_calendar_dom_elements() gelesen worden
    //}
}

//---------------------------------------------------------------------------------normalized_state

Order::State normalized_state( const Order::State& state )
{
    if( state.vt == VT_BSTR  &&  ( state.bstrVal == NULL || SysStringLen( state.bstrVal ) == 0 ) )
    {
        return Variant( Variant::vt_missing );      // Für Java
    }
    else
    {
        return state;
    }
}

//-------------------------------------------------------------------------------Job_chain::add_job

Job_chain_node* Job_chain::add_job( Job* job, const Order::State& state, const Order::State& next_state, const Order::State& error_state )
{
    Order::check_state( state );
    if( !next_state.is_missing() )  Order::check_state( next_state );
    if( !error_state.is_missing() )  Order::check_state( error_state );

    if( job  &&  !job->order_queue() )  z::throw_xc( "SCHEDULER-147", job->name() );

    if( _state != s_under_construction )  z::throw_xc( "SCHEDULER-148" );

    ptr<Job_chain_node> node = new Job_chain_node;

    node->_job_chain = this;
    node->_job       = job;
    node->_state     = state;

    if( node->_state.is_missing() )  node->_state = job->name();      // Parameter state nicht angegeben? Default ist der Jobname

    node->_next_state  = normalized_state( next_state );
    node->_error_state = normalized_state( error_state );

    // Bis finish() bleibt nicht angegebener Zustand als VT_ERROR/is_missing() (fehlender Parameter) stehen.
    // finish() unterscheidet dann die nicht angegebenen Zustände von VT_ERROR und setzt Defaults oder VT_EMPTY (außer <file_order_sink>)

    //THREAD_LOCK( _lock )
    {
        if( Job_chain_node* n = node_from_state_or_null( node->_state ) )
        {
            if( !job  &&  next_state.is_missing()  &&  error_state.is_missing() )  return n;     // job_chain.add_end_state() darf mehrfach gerufen werden.
            z::throw_xc( "SCHEDULER-150", debug_string_from_variant(node->_state), name() );
        }

        _chain.push_back( node );

        if( job )  job->set_job_chain_priority( _chain.size() );   // Weiter hinten stehende Jobs werden vorrangig ausgeführt
    }

    return node;
}

//--------------------------------------------------------------------------------Job_chain::finish

void Job_chain::finish()
{
    //THREAD_LOCK( _lock )
    {
        if( _state != s_under_construction )  return;

        if( !_chain.empty() )
        {
            Job_chain_node* n = *_chain.rbegin();
            if( n->_job  &&  n->_next_state.is_missing() )  add_job( NULL, default_end_state_name );    // Endzustand fehlt? Dann hinzufügen
        }

        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            Chain::iterator next = it;  next++;

            if( n->is_file_order_sink() )
            {
                // _next_state und _error_state unverändert lassen
            }
            else
            {
                if( n->_next_state.is_missing()  &&  next != _chain.end()  &&  n->_job )  n->_next_state = (*next)->_state;

                if( !n->_next_state.is_missing() )  n->_next_node  = node_from_state( n->_next_state );
                                              else  n->_next_state = empty_variant;

                if( !n->_error_state.is_missing() )  n->_error_node  = node_from_state( n->_error_state );
                                               else  n->_error_state = empty_variant;
            }
        }

        _order_sources.finish();

        if( zschimmer::Log_ptr log = "" )
        {
            log << "Job_chain " << _name << " finished:\n";

            for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
            {
                Job_chain_node* n = *it;

                log << "    job=" << ( n->_job? n->_job->_name : "(end)" );
                log << " state=" << n->_state;
                log << " next="  << n->_next_state;
                log << " error=" << n->_error_state;
                log << '\n';
            }
        }
    }
}

//-------------------------------------------------------------------------Job_chain::node_from_job

Job_chain_node* Job_chain::node_from_job( Job* job )
{
    //THREAD_LOCK( _lock )
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            if( n->_job == job )  return n;
        }
    }

    z::throw_xc( "SCHEDULER-152", job->name(), name() );
    return NULL;
}

//-----------------------------------------------------------------------Job_chain::node_from_state

Job_chain_node* Job_chain::node_from_state( const Order::State& state )
{
    Job_chain_node* result = node_from_state_or_null( state );
    if( !result )  z::throw_xc( "SCHEDULER-149", name(), debug_string_from_variant(state) );
    return result;
}

//---------------------------------------------------------------Job_chain::node_from_state_or_null

Job_chain_node* Job_chain::node_from_state_or_null( const Order::State& state )
{
    //THREAD_LOCK( _lock )
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            if( n->_state == state )  return n;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------Job_chain::job_from_state

Job* Job_chain::job_from_state( const Order::State& state )
{
    Job_chain_node* node = node_from_state( state );
    if( !node->_job )  z::throw_xc( "SCHEDULER-374", obj_name(), state.as_string() );
    return node->_job;
}

//--------------------------------------------------------------------------Job_chain::contains_job

bool Job_chain::contains_job( Job* job )
{
    Z_FOR_EACH( Chain, _chain, n )
    {
        Job_chain_node* node = *n;
        if( node->_job == job )  return true;
    }

    return false;
}

//----------------------------------------------------------------------------Job_chain::first_node

Job_chain_node* Job_chain::first_node()
{
    if( _chain.empty() )  z::throw_xc( __FUNCTION__ );
    return *_chain.begin();
}

//-----------------------------------------------------------------------------Job_chain::add_order

void Job_chain::add_order( Order* order )
{
    assert( order->_is_distributed == order->_is_db_occupied );
    assert( !order->_is_distributed || _is_distributed );
    if( state() != s_ready )  z::throw_xc( "SCHEDULER-151" );
    if( !order->_is_distributed  &&  !_spooler->has_exclusiveness() )  z::throw_xc( "SCHEDULER-383", order->obj_name() );
    
    if( has_order_id( (Read_transaction*)NULL, order->id() ) )  z::throw_xc( "SCHEDULER-186", order->obj_name(), name() );


    set_visible( true );

    Job_chain_node* node = node_from_state( order->_state );
    if( ( !order->_suspended || !order->_is_on_blacklist )  &&  !node->_job )  z::throw_xc( "SCHEDULER-149", name(), debug_string_from_variant(order->_state) );
    if( node->_job )  assert( node->_job->order_queue() );

    order->_job_chain      = this;
    order->_job_chain_name = name();
    order->_removed_from_job_chain_name = "";
    order->_log->set_prefix( order->obj_name() );

    order->_job_chain_node = node;
    // <job_chain_node suspended="yes"> soll beim Laden aus der Datenbank nicht wirken: order->set_job_chain_node( node ); 

    register_order( order );

    if( order->_is_on_blacklist || !node->_job )  order->set_on_blacklist();
                                            else  node->_job->order_queue()->add_order( order );
}

//--------------------------------------------------------------------------Job_chain::remove_order

void Job_chain::remove_order( Order* order )
{
    assert( order->_job_chain_name == _name );
    assert( order->_job_chain == this );

    ptr<Order> hold_order = order;   // Halten

    if( order->_is_on_blacklist )
    {
        order->remove_from_blacklist();
    }

    if( order->_job_chain_node )
    {
        order->remove_from_job();
    }

    if( order->_is_db_occupied )  
    {
        //assert( order->_task );
        //order->db_release_occupation();
    }

    order->_job_chain      = NULL;
    order->_job_chain_name = "";
    order->_log->set_prefix( obj_name() );

    unregister_order( order );

    if( order->_task )
    {
        order->_removed_from_job_chain_name = _name;      // Für die Task merken, in welcher Jobkette wir waren
        order->_moved = true;
    }

    check_for_removing();
}

//-------------------------------------------------------------Job_chain::add_orders_from_database

void Job_chain::add_orders_from_database( Read_transaction* ta )
{
    assert( _orders_recoverable );
    _spooler->assert_has_exclusiveness( obj_name() + " " + __FUNCTION__ );
    assert( _spooler->db()  &&  _spooler->db()->opened() );

    _load_orders_from_database = false;


    int count = 0;

    Any_file result_set = ta->open_result_set
        ( 
            S() << "select " << order_select_database_columns << ", `distributed_next_time`"
            "  from " << _spooler->_orders_tablename <<
            "  where " << db_where_condition() <<
            "  order by `ordering`",
            __FUNCTION__
        );

    count += load_orders_from_result_set( ta, &result_set );

    log()->debug( message_string( "SCHEDULER-935", count ) );
}

//-----------------------------------------------------------Job_chain::load_orders_from_result_set

int Job_chain::load_orders_from_result_set( Read_transaction* ta, Any_file* result_set )
{
    int count = 0;

    while( !result_set->eof() )
    {
        Record record = result_set->get_record();

        try
        {
            add_order_from_database_record( ta, record );
            count++;
        }
        catch( exception& x )
        {
            log()->error( message_string( "SCHEDULER-295", record.as_string( "id" ), x ) ); 
        }
    }

    return count;
}

//--------------------------------------------------------Job_chain::add_order_from_database_record

Order* Job_chain::add_order_from_database_record( Read_transaction* ta, const Record& record )
{
    string order_id = record.as_string( "id" );

    ptr<Order> order = new Order( _spooler, record, name() );
    order->load_blobs( ta );

    if( record.as_string( "distributed_next_time" ) != "" )
    {
        z::throw_xc( "SCHEDULER-389", order->obj_name() );    // Wird von load_orders_from_result_set() ignoriert (sollte vielleicht nicht)
    }

    add_order( order );

    return order;
}

//------------------------------------------------------------------------------Job_chain::activate

void Job_chain::activate()
{
    // Wird nur von Order_subsystem::activate() für beim Start des Schedulers geladene Jobketten gerufen,
    // um nach Start des Scheduler-Skripts die <run_time next_start_function="..."> berechnen zu können.
    // Für später hinzugefügte Jobketten wird diese Routine nicht gerufen (sie würde auch nichts tun).
    Z_FOR_EACH( Order_map, _order_map, o )
    {
        Order* order = o->second;
        order->activate();
    }
}

//-------------------------------------------------------------Job_chain::remove_all_pending_orders

int Job_chain::remove_all_pending_orders( bool leave_in_database )
{
    bool        force        = false;
    int         result       = 0;
    Order_map   my_order_map = _order_map;

    Z_FOR_EACH( Order_map, my_order_map, o )
    {
        Order* order = o->second;

        if( !order->_task || force )
        {
            if( leave_in_database )  remove_order( order );
                               else  order->remove_from_job_chain();
            order = NULL;
            result++;
        }
        else
        {
            string task = "(deleted task)";
            if( _spooler->_task_subsystem  &&  _spooler->_task_subsystem->has_tasks() )  task = order->_task->obj_name();  
            Z_LOG2( "scheduler", __FUNCTION__ << ": " << order->obj_name() << " wird nicht entfernt, weil in Verarbeitung durch " << task << "\n" );
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Job_chain::order

ptr<Order> Job_chain::order( const Order::Id& id )
{
    ptr<Order> result = order_or_null( id );

    if( !result )  z::throw_xc( "SCHEDULER-162", debug_string_from_variant(id), _name );

    return result;
}

//-------------------------------------------------------------------------Job_chain::order_or_null

ptr<Order> Job_chain::order_or_null( const Order::Id& order_id )
{
    Order_map::iterator it = _order_map.find( Order::string_id( order_id ) );
    return it != _order_map.end()? it->second : NULL;
}

//-----------------------------------------------------------------------------Job_chain::has_order

bool Job_chain::has_order() const
{
    for( Chain::const_iterator it = _chain.begin(); it != _chain.end(); it++ )
    {
        Job* job = (*it)->_job;
        if( job  &&  job->order_queue()  &&  !job->order_queue()->empty( this ) )  return true;
    }

    return false;
}

//---------------------------------------------------------------------------Job_chain::order_count

int Job_chain::order_count( Read_transaction* ta )
{
    int       result = 0;
    set<Job*> jobs;             // Jobs können (theoretisch) doppelt vorkommen, sollen aber nicht doppelt gezählt werden.

    if( _is_distributed && ta )
    {
        result = ta->open_result_set( S() << "select count(*)  from " << _spooler->_orders_tablename <<"  where " << db_where_condition(),
                                      __FUNCTION__ )
                 .get_record().as_int( 0 );
    }
    else
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job* job = (*it)->_job;
            if( job  &&  !set_includes( jobs, job ) )  jobs.insert( job ),  result += job->order_queue()->order_count( (Read_transaction*)NULL, this );
        }
    }

    return result;
}

//--------------------------------------------------------------------------Job_chain::has_order_id

bool Job_chain::has_order_id( Read_transaction* ta, const Order::Id& order_id )
{
    Order_map::iterator it = _order_map.find( Order::string_id( order_id ) );
    bool result = it != _order_map.end();

    if( !result  &&  _is_distributed  &&  ta )
    {
        int count = ta->open_result_set
            ( 
                S() << "select count(*)  from " << _spooler->_orders_tablename << 
                       "  where " << order_subsystem()->order_db_where_condition( _name, order_id.as_string() ),
                __FUNCTION__
            )
            .get_record().as_int( 0 );

        result = count > 0;
    }

    return result;
}

//------------------------------------------------------------------------Job_chain::register_order

void Job_chain::register_order( Order* order )
{
    string id_string = order->string_id();
    Order_map::iterator it = _order_map.find( id_string );
    if( it != _order_map.end() )  z::throw_xc( "SCHEDULER-186", order->obj_name(), _name );
    _order_map[ id_string ] = order;
}

//----------------------------------------------------------------------Job_chain::unregister_order

void Job_chain::unregister_order( Order* order )
{
    assert( !order->_is_on_blacklist );

    Order_map::iterator it = _order_map.find( order->string_id() );
    if( it != _order_map.end() )  _order_map.erase( it );
                            else  Z_LOG2( "scheduler", __FUNCTION__ << " " << order->obj_name() << " ist nicht registriert.\n" );
}

//----------------------------------------------------------------Job_chain::add_order_to_blacklist

void Job_chain::add_order_to_blacklist( Order* order )
{
    //if( order->_suspended || !node_from_state_or_null( order->_state ) || !node_from_state_or_null( order->_state )->_job )  z::throw_xc( __FUNCTION__ );

    _blacklist_map[ order->string_id() ] = order;
}

//-----------------------------------------------------------Job_chain::remove_order_from_blacklist

void Job_chain::remove_order_from_blacklist( Order* order )
{
    _blacklist_map.erase( order->string_id() );
}

//-----------------------------------------------------------------------Job_chain::is_on_blacklist

bool Job_chain::is_on_blacklist( const string& order_id )
{
    Blacklist_map::iterator it = _blacklist_map.find( order_id );
    return it != _blacklist_map.end();
}

//-------------------------------------------------------------Job_chain::blacklisted_order_or_null

Order* Job_chain::blacklisted_order_or_null( const string& order_id )
{
    Blacklist_map::iterator it = _blacklist_map.find( order_id );
    return it != _blacklist_map.end()? it->second : NULL;
}

//-------------------------------------------------------------------Job_chain::db_blacklist_id_set

hash_set<string> Job_chain::db_blacklist_id_set()
{
    hash_set<string> result;

    Z_FOR_EACH( Blacklist_map, _blacklist_map, it )  result.insert( it->first );
    
    if( _is_distributed )
    {
        Read_transaction ta ( db() );
        S select_sql;
        select_sql << "select `id`  from " << _spooler->_orders_tablename <<
                      "  where " << db_where_condition() << 
                      "  and `distributed_next_time`={ts'" << blacklist_database_distributed_next_time << "'}";

        for( Any_file result_set = ta.open_result_set( select_sql, __FUNCTION__ ); !result_set.eof(); )
        {
            Record record = result_set.get_record();
            result.insert( record.as_string( 0 ) );
        }
    }

    return result;
}

//---------------------------------------------------------Job_chain::tip_for_new_distributed_order

bool Job_chain::tip_for_new_distributed_order( const Order::State& state, const Time& at )
{
    bool result = false;

    if( Job_chain_node* node = node_from_state_or_null( state ) )
    {
        if( Job* job = node->_job )
        {
            if( job->order_queue()->is_distributed_order_requested( time_max - 1 ) 
             && at < job->order_queue()->next_announced_distributed_order_time() )
            {
                job->order_queue()->set_next_announced_distributed_order_time( at, at.is_null() || at <= Time::now() );
                result = true;
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------Job_chain::remove

void Job_chain::remove()
{
    if( _state < s_ready )  z::throw_xc( "SCHEDULER-151" );

    remove_all_pending_orders( true );
    
    if( has_order() )
    {
        set_state( s_removing );
    }
    else
    {
        close();

        Z_FOR_EACH( Order_sources::Order_source_list, _order_sources._order_source_list, it )
        {
            Order_source* order_source = *it;
            order_source->close();
        }

        order_subsystem()->remove_job_chain( this );
    }
}

//--------------------------------------------------------------------Job_chain::check_for_removing

void Job_chain::check_for_removing()
{
    if( state() == s_removing &&  !has_order() )
    {
        log()->info( message_string( "SCHEDULER-936" ) );     // "Removing"
        remove();
    }
}

//--------------------------------------------------------------------Job_chain::db_where_condition

string Job_chain::db_where_condition() const
{ 
    return order_subsystem()->job_chain_db_where_condition( _name ); 
}

//-----------------------------------------------------------------------Job_chain::order_subsystem

Order_subsystem* Job_chain::order_subsystem() const
{
    return static_cast<Order_subsystem*>( _spooler->order_subsystem() );
}

//-------------------------------------------------------------------------Order_queue::Order_queue

Order_queue::Order_queue( Job* job, Prefix_log* log )
:
    _zero_(this+1),
    _spooler(job->_spooler),
    _job(job),
    _log(log),
    _next_announced_distributed_order_time( Time::never ),
    _next_distributed_order_check_time( time_max ),
    _next_distributed_order_check_delay( check_database_orders_period_minimum )
{
}

//------------------------------------------------------------------------Order_queue::~Order_queue

Order_queue::~Order_queue()
{
    try
    {
        close();
    }
    catch( exception& x )  { _log->warn( x.what() ); }
}

//-------------------------------------------------------------------------------Order_queue::close

void Order_queue::close()
{
    _job = NULL;    // Falls Job gelöscht wird

    for( Queue::iterator it = _queue.begin(); it != _queue.end();  )
    {
        Order* order = *it;
        _log->info( message_string( "SCHEDULER-937", order->obj_name() ) );
        order->_in_job_queue = false;
        it = _queue.erase( it );
    }

    //update_priorities();
    //_has_users_id = false;
}

//-------------------------------------------------------------------------Order_queue::dom_element

xml::Element_ptr Order_queue::dom_element( const xml::Document_ptr& document, const Show_what& show, Job_chain* which_job_chain )
{
    Read_transaction ta ( _spooler->_db );

    xml::Element_ptr element = document.createElement( "order_queue" );

    //THREAD_LOCK( _lock )
    {
        element.setAttribute( "length", order_count( &ta, which_job_chain ) );

        //if( Time next = next_time() )
        element.setAttribute( "next_start_time", next_time().as_string() );

        //if( is_distributed_order_requested() )
        //element.setAttribute( "order_requested", "yes" );

        if( show.is_set( show_orders ) )
        {
            dom_append_nl( element );

            int remaining = show._max_orders;

            FOR_EACH( Queue, _queue, it )
            {
                Order* order = *it;
                if( !which_job_chain  ||  order->_job_chain == which_job_chain )
                {
                    if( remaining-- <= 0 )  break;
                    element.appendChild( order->dom_element( document, show ) );
                    dom_append_nl( element );
                }
            }

            if( remaining > 0  &&  order_subsystem()->are_orders_distributed()  
             &&  _spooler->db()  &&  _spooler->db()->opened()  &&  !_spooler->db()->is_in_transaction() )
            {
                Read_transaction ta ( _spooler->db() );

                string w = db_where_expression_for_distributed_orders();
                if( w != "" )
                {
                    element.append_new_comment( "In database only:" );
                    dom_append_nl( element );

                    S select_sql;
                    select_sql << "select %limit(" << remaining << ") " << order_select_database_columns << ", `job_chain`, `occupying_cluster_member_id` " << 
                                  "  from " << _spooler->_orders_tablename <<
                                 "  where " << w << 
                                    " and `distributed_next_time` is not null "
                                    " and ( `occupying_cluster_member_id`<>" << sql::quoted( _spooler->cluster_member_id() ) << " or"
                                          " `occupying_cluster_member_id` is null )"
                                "  order by `distributed_next_time`, `priority`, `ordering`";

                    for( Any_file result_set = ta.open_result_set( select_sql, __FUNCTION__ ); 
                         remaining > 0 &&  !result_set.eof(); 
                         --remaining )
                    {
                        Record record = result_set.get_record();

                        try
                        {
                            string job_chain_name              = record.as_string( "job_chain" );
                            string occupying_cluster_member_id = record.as_string( "occupying_cluster_member_id" );

                            ptr<Order> order = new Order( _spooler, record, job_chain_name );

                            order->load_order_xml_blob( &ta );
                            if( show.is_set( show_payload  ) )  order->load_payload_blob( &ta );
                            if( show.is_set( show_run_time ) )  order->load_run_time_blob( &ta );


                            xml::Element_ptr order_element = order->dom_element( document, show );

                            order_element.setAttribute_optional( "occupied_by_cluster_member_id", occupying_cluster_member_id );
                            
                            if( _spooler->_cluster )
                            order_element.setAttribute_optional( "occupied_by_http_url", _spooler->_cluster->http_url_of_member_id( occupying_cluster_member_id ) );
                            
                            element.appendChild( order_element );
                            dom_append_nl( element );
                        }
                        catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << "  " << x.what() << "\n" ); }  // Auftrag kann inzwischen gelöscht worden sein
                    }
                }
            }
        }
    }

    return element;
}

//---------------------------------------------------------------Order_queue::register_order_source

void Order_queue::register_order_source( Order_source* order_source )
{
    Z_DEBUG_ONLY( Z_FOR_EACH( Order_source_list, _order_source_list, it )  assert( *it != order_source ); )

    _order_source_list.push_back( order_source );
}

//-------------------------------------------------------------Order_queue::unregister_order_source

void Order_queue::unregister_order_source( Order_source* order_source )
{
    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        if( *it == order_source )
        {
            it = _order_source_list.erase( it );
            return;
        }
    }
}

//-------------------------------------------------------------------------Order_queue::order_count

int Order_queue::order_count( Read_transaction* ta, const Job_chain* which_job_chain )
{
    int result = 0;

    if( ta  &&  ( which_job_chain? which_job_chain->_is_distributed : order_subsystem()->are_orders_distributed() ) )
    {
        string w = which_job_chain? db_where_expression_for_job_chain( which_job_chain )
                                  : db_where_expression_for_distributed_orders();
        if( w != "" )
        {
            int count = ta->open_result_set
                        (
                            S() << "select count(*)  from " << _spooler->_orders_tablename <<
                                   "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                                      " and `distributed_next_time` is not null"
                                    //" and ( `occupying_cluster_member_id`<>" << sql::quoted( _spooler->cluster_member_id() ) << " or"
                                    //      " `occupying_cluster_member_id` is null )"
                                      " and " << w,
                            __FUNCTION__
                        )
                        .get_record().as_int( 0 );

            result += count;
        }

        FOR_EACH( Queue, _queue, it )
        {
            Order* order = *it;
            if( !order->_is_in_database  &&  ( !which_job_chain || order->_job_chain == which_job_chain ) )  result++;
        }
    }
    else
    if( which_job_chain )
    {
        FOR_EACH( Queue, _queue, it )  if( (*it)->_job_chain == which_job_chain )  result++;
    }
    else
    {
        result += _queue.size();
    }

    return result;
}

//----------------------------------------------------------------------Order_queue::reinsert_order

void Order_queue::reinsert_order( Order* order )
{
    ptr<Order> hold_order = order;

    remove_order( order, dont_log );
    add_order( order, dont_log );         // Neu einsortieren
}

//---------------------------------------------------------------------------Order_queue::add_order

void Order_queue::add_order( Order* order, Do_log do_log )
{
#   ifdef Z_DEBUG
        Z_FOR_EACH( Queue, _queue, it )  assert( *it != order );
#   endif

    _job->set_visible( true );


    Time next_time = order->next_time();


    if( next_time )
    {
        if( !order->_suspended )
        {
            if( order->_setback < Time::never )  
            {
                if( order->_setback )  order->_log->log( do_log? log_info : log_debug3, message_string( "SCHEDULER-938", order->_setback ) );
            }
            else  
                order->_log->log( do_log? log_warn : log_debug3, message_string( "SCHEDULER-296" ) );       // "Die <run_time> des Auftrags hat keine nächste Startzeit" );
        }
    }
    else
        _log->debug( message_string( "SCHEDULER-990", order->obj_name() ) );


    Queue::iterator insert_before = _queue.begin();
    
    for(; insert_before != _queue.end(); insert_before++ )
    {
        Order* o = *insert_before;

        if( o->_suspended < order->_suspended )  continue;
        if( o->_suspended > order->_suspended )  break;
        
        if( o->_setback < order->_setback )  continue;
        if( o->_setback > order->_setback )  break;

        if( o->_priority < order->_priority )  continue;
        if( o->_priority > order->_priority )  break;
    }

    _queue.insert( insert_before, order );
    order->_in_job_queue = true;

    //update_priorities();

    order->handle_changed_processable_state();
}

//------------------------------------------------------------------------Order_queue::remove_order

void Order_queue::remove_order( Order* order, Do_log dolog )
{
    if( dolog == do_log )  _log->debug9( "remove_order " + order->obj_name() );

    Queue::iterator it;
    for( it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

    if( it == _queue.end() )  z::throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

    order->_in_job_queue = false;

    _queue.erase( it );
    order = NULL;  // order ist jetzt möglicherweise ungültig

    //update_priorities();
}

//-------------------------------------------------------------------Order_queue::update_priorities
/*
void Order_queue::update_priorities()
{
    if( !_queue.empty() )
    {
        _highest_priority = _queue.front()->priority();
        _lowest_priority  = _queue.back()->priority();
    }
    else
    {
        _highest_priority = 0;
        _lowest_priority  = 0;
    }
}
*/
//-----------------------------------------------------------------------Order_queue::request_order

bool Order_queue::request_order( const Time& now, const string& cause )
{
    // Diese Methode weniger oft aufgerufen werden.  Z_LOGI2( "joacim", _job->obj_name() << " " << __FUNCTION__ << "  " << cause << "\n" );

    bool result = false;


    if( !result )  result = _has_tip_for_new_order  ||  _next_announced_distributed_order_time <= now;

    if( !result )  result = has_order( now );

    if( !result )
    {
        // Dateiauftäge (File_order)

        Z_FOR_EACH( Order_source_list, _order_source_list, it ) 
        {
            Order_source* order_source = *it;
            result = order_source->request_order( cause );
            if( result )  break;
        }
    }


    // Jetzt prüfen wir die verteilten Aufträge.
    // Die können auch von anderen Schedulern verarbeitet werden, und sind deshalb nachrangig.

    if( !result )
    {
        if( _next_distributed_order_check_time == time_max  // Erster request_order()?
         && order_subsystem()->is_job_in_any_distributed_job_chain( _job ) )  // Das ist bei vielen Jobketten nicht effizent
        {
            _next_distributed_order_check_time  = 0;
            _next_distributed_order_check_delay = check_database_orders_period_minimum;

            order_subsystem()->request_order();     // Nur einmal rufen, bis ein neuer Auftrag für den Job eingetrifft
        }
    }

    return result;
}

//--------------------------------------------------------------Order_queue::withdraw_order_request

void Order_queue::withdraw_order_request()
{
    Z_FOR_EACH( Order_source_list, _order_source_list, it ) 
    {
        Order_source* order_source = *it;
        order_source->withdraw_order_request();
    }

    withdraw_distributed_order_request();

    //if( _spooler->_order_subsystem  &&  _spooler->_order_subsystem->_database_order_detector )
    //{
    //    _spooler->_order_subsystem->_database_order_detector->withdraw_order_request_for_job( _job );
    //}
}

//--------------------------------------------------Order_queue::withdraw_distributed_order_request

void Order_queue::withdraw_distributed_order_request()
{
    _next_distributed_order_check_time  = time_max;
    _next_distributed_order_check_delay = check_database_orders_period;
}

//-----------------------------------------Order_queue::calculate_next_distributed_order_check_time

void Order_queue::calculate_next_distributed_order_check_time( time_t now )
{
    _next_distributed_order_check_delay *= 2;
    if( _next_distributed_order_check_delay > check_database_orders_period )  _next_distributed_order_check_delay = check_database_orders_period;

    _next_distributed_order_check_time = now + _next_distributed_order_check_delay;
}

//-------------------------------------------Order_queue::set_next_announced_distributed_order_time

void Order_queue::set_next_announced_distributed_order_time( const Time& t, bool is_now )
{ 
    Z_LOG2( "scheduler.order", _job->obj_name() << "  " << __FUNCTION__ << "(" << t << ( is_now? ",is_now" : "" ) << ")  vorher: " << _next_announced_distributed_order_time << "\n" );

    _next_announced_distributed_order_time = t; 
    
    Z_DEBUG_ONLY( assert( is_now? t <= Time::now() : t > Time::now() ) );

    if( is_now )
    {
        //_is_distributed_order_requested = false;
        _job->signal( __FUNCTION__ );
    }
}

//-----------------------------------------------Order_queue::next_announced_distributed_order_time

Time Order_queue::next_announced_distributed_order_time()
{ 
    return _next_announced_distributed_order_time; 
}

//-------------------------------------------------------Order_queue::tip_for_new_distributed_order

void Order_queue::tip_for_new_distributed_order()
{
    Z_LOG2( "scheduler.order", _job->obj_name() << "  " << __FUNCTION__ << "\n" );

    if( !_has_tip_for_new_order )  _job->signal( __FUNCTION__ );
    _has_tip_for_new_order = true;
}

//-------------------------------------------------------------------------Order_queue::first_order

Order* Order_queue::first_order( const Time& now ) const
{
    // now kann 0 sein, dann werden nur Aufträge ohne Startzeit beachtet

    Order* result = NULL;

    //remove_outdated_orders();

    Z_FOR_EACH_CONST( Queue, _queue, o )
    {
        Order* order = *o;

        if( order->is_immediately_processable( now ) )
        {
            result = order;
            result->_setback = 0;
            break;
        }

        if( order->_setback > now )  break;
    }

    if( result )  assert( !result->_is_distributed );

    return result;
}

//--------------------------------------------------------------Order_queue::fetch_and_occupy_order

Order* Order_queue::fetch_and_occupy_order( const Time& now, const string& cause, Task* occupying_task )
{
    assert( occupying_task );


    // Zuerst Aufträge aus unserer Warteschlange im Speicher

    Order* order = first_order( now );
    if( order )  order->occupy_for_task( occupying_task, now );


    // Dann (alte) Aufträge aus der Datenbank
    if( !order  &&  _next_announced_distributed_order_time <= now  &&  is_in_any_distributed_job_chain() )   // Auftrag nur lesen, wenn vorher angekündigt
    {
        withdraw_distributed_order_request();

        order = load_and_occupy_next_distributed_order_from_database( occupying_task, now );
        // Möglicherweise NULL (wenn ein anderer Scheduler den Auftrag weggeschnappt hat)
        if( order )  assert( order->_is_distributed );
    }


    // Die Dateiaufträge (File_order_source)

    if( !order )
    {
        Z_FOR_EACH( Order_source_list, _order_source_list, it )
        {
            Order_source* order_source = *it;
            order = order_source->fetch_and_occupy_order( now, cause, occupying_task );
            if( order )  break;
        }
    }

    //if( order )
    //{
    //    _is_distributed_order_requested = false;            // Nächster request_order() führt zum async_wake() des Database_order_detector
    //}

    _next_announced_distributed_order_time = Time::never;
    _has_tip_for_new_order = false;

    if( order )  order->assert_task( __FUNCTION__ );

    return order;
}

//-----------------------------------------------------Order_queue::is_in_any_distributed_job_chain

bool Order_queue::is_in_any_distributed_job_chain()
{ 
    return order_subsystem()->is_job_in_any_distributed_job_chain( _job );
}

//--------------------------------Order_queue::load_and_occupy_next_distributed_order_from_database

Order* Order_queue::load_and_occupy_next_distributed_order_from_database( Task* occupying_task, const Time& now )
{
    Order* result    = NULL;
    S      select_sql;

    string w = db_where_expression_for_distributed_orders();

    if( w != "" )
    {
        select_sql << "select %limit(1)  `job_chain`, " << order_select_database_columns <<
                    "  from " << _spooler->_orders_tablename <<  //" %update_lock"  Oracle kann nicht "for update", limit(1) und "order by" kombinieren
                    "  where `spooler_id`=" << sql::quoted(_spooler->id_for_db()) <<
                       " and `distributed_next_time` < {ts'" << never_database_distributed_next_time << "'}"
                       " and `occupying_cluster_member_id` is null" << 
                       " and " << w <<
                    "  order by `distributed_next_time`, `priority`, `ordering`";

        //try
        //{
            Record record;
            bool   record_filled = false;


            for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
            {
                Any_file result_set = ta.open_result_set( select_sql, __FUNCTION__ );
                if( !result_set.eof() )
                {
                    record = result_set.get_record();
                    record_filled = true;
                }
            }
            catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), __FUNCTION__ ); }


            if( record_filled )
            {
                ptr<Order> order = new Order( _spooler, record, record.as_string( "job_chain" ) );
            
                order->_is_distributed = true;
                
                Job_chain* job_chain = order_subsystem()->job_chain( order->_job_chain_name );
                assert( job_chain->_is_distributed );

                bool ok = order->db_occupy_for_processing();
                if( ok )
                {
                    try
                    {
                        Read_transaction ta ( _spooler->db() );
                        order->load_blobs( &ta );
                    }
                    catch( exception& ) 
                    { 
                        ok = false;      // Jemand hat wohl den Datensatz gelöscht
                    }
            
                    if( ok )
                    {
                        order->occupy_for_task( occupying_task, now );
                        job_chain->add_order( order );

                        result = order,  order = NULL;
                    }
                }

                if( order )  order->close( Order::cls_dont_remove_from_job_chain ), order = NULL;
            }
        //}
        //catch( exception& x )
        //{
        //    Z_LOGI2( "scheduler", __FUNCTION__ << "  " << x.what() << "\n" );
        //    if( order )  order->close();
        //    throw;
        //}
    }

    return result;
}

//---------------------------------------------------------------------------Order_queue::next_time

Time Order_queue::next_time()
{
    Time result = Time::never;

    Z_FOR_EACH_CONST( Queue, _queue, o )
    {
        Order* order = *o;

        if( order->is_processable() )
        {
            assert( !order->_is_distributed );
            result = order->next_time();
            break;
        }
    }

    if( result > _next_announced_distributed_order_time )  result = _next_announced_distributed_order_time;

    return result;
}

//-----------------------------------------------------------------------Order_queue::order_or_null

//ptr<Order> Order_queue::order_or_null( const Order::Id& id )
//{
//    int NOT_DISTRIBUTED;
//
//    FOR_EACH( Queue, _queue, it )  if( (*it)->_id == id )  return *it;
//
//    return NULL;
//}

//---------------------------------------------------------------Order_queue::make_where_expression

string Order_queue::db_where_expression_for_distributed_orders()
{
    S   result;
    int is_in_any_job_chain = 0;

    Z_FOR_EACH( Order_subsystem::Job_chain_map, order_subsystem()->_job_chain_map, jc )
    {
        Job_chain* job_chain = jc->second;
        if( job_chain->_is_distributed )
        {
            string w = db_where_expression_for_job_chain( job_chain );
            if( w != "" )
            {
                if( is_in_any_job_chain )  result << " or ";
                is_in_any_job_chain++;

                result << w;
            }
        }
    }

    return is_in_any_job_chain <= 1? result
                                   : "( " + result + " )";
}

//-------------------------------------------------Order_queue::db_where_expression_for_job_chain

string Order_queue::db_where_expression_for_job_chain( const Job_chain* job_chain )
{
    S    result;
    bool is_in_job_chain = false;

    result << "(`job_chain`="  << sql::quoted( job_chain->name() ) << " and `state` in (";

    Z_FOR_EACH_CONST( Job_chain::Chain, job_chain->_chain, n )
    {
        Job_chain_node* node = *n;

        if( node->_job == _job )
        {
            if( is_in_job_chain )  result << ",";
            is_in_job_chain = true;

            result << sql::quoted( string_from_variant( node->_state ) );
        }
    }

    return is_in_job_chain? result + "))"
                          : "";

    //Z_FOR_EACH( Job_chain::Chain, job_chain->_chain, n )
    //{
    //    Job_chain_node* node = *n;

    //    if( node->_job == _job )
    //    {
    //        result << ( is_in_job_chain? " or " : "(" );
    //        is_in_any_job_chain = true;

    //        result << "`job_chain`="  << sql::quoted( job_chain->name() ) <<
    //                  " and `state`=" << sql::quoted( string_from_variant( node->_state ) );
    //    }
    //}

    //if( is_in_job_chain )  result << ")";

    //return result;
}

//---------------------------------------------------------------------Order_queue::order_subsystem

Order_subsystem* Order_queue::order_subsystem() const
{ 
    return static_cast<Order_subsystem*>( _spooler->order_subsystem() ); 
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler )
:
    Com_order(this),
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    _zero_(this+1)
{
    init();
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const VARIANT& payload )
:
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    Com_order(this),
    _zero_(this+1),
    _payload(payload)
{
    init();
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const Record& record, const string& job_chain_name )
:
    Com_order(this),
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    _zero_(this+1)
{
    init();

    _job_chain_name = job_chain_name;

    set_id      ( record.as_string( "id"         ) );   _id_locked = true;
    _state      = record.as_string( "state"      );
    _state_text = record.as_string( "state_text" );
    _title      = record.as_string( "title"      );
    _priority   = record.as_int   ( "priority"   );

    string initial_state = record.as_string( "initial_state" );
    if( initial_state != "" )
    {
        _initial_state = initial_state;
        _initial_state_set = true;
    }

    _created.set_datetime( record.as_string( "created_time" ) );

    _log->set_prefix( obj_name() );

    _order_xml_modified  = false;            
    _state_text_modified = false; 
    _title_modified      = false;
    _state_text_modified = false;
    _is_in_database      = true;
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
        assert( !_in_job_queue );
      //assert( !_is_replacement );
      //assert( !_replaced_by );
        assert( !_order_queue );
#   endif

    set_replacement( false );

    if( _replaced_by )  
    {
        _replaced_by->set_replacement( false );
    }

    if( _run_time )  _run_time->close();
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    //_recoverable = true;

    _log = Z_NEW( Prefix_log( this ) );
    _log->set_prefix( obj_name() );

    _created       = Time::now();
    _is_virgin     = true;
    _old_next_time = Time::never;

    set_run_time( NULL );
}

//--------------------------------------------------------------------------------Order::load_blobs

void Order::load_blobs( Read_transaction* ta )
{
    load_order_xml_blob( ta );
    load_run_time_blob( ta );
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
    if( run_time_xml != "" )  set_run_time( xml::Document_ptr( run_time_xml ).documentElement() );
}

//-------------------------------------------------------------------------Order::load_payload_blob

void Order::load_payload_blob( Read_transaction* ta )
{
    string payload_string = db_read_clob( ta, "payload" );
    if( payload_string.find( "<" + Com_variable_set::xml_element_name() ) != string::npos )
    {
        ptr<Com_variable_set> v = new Com_variable_set;
        v->put_Xml( Bstr( payload_string ) );
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

bool Order::is_immediately_processable( const Time& now )
{
    // select ... from scheduler_orders  where not suspended and replacement_for is null and setback is null 
    // scheduler_orders.processable := !_is_on_blacklist && !_suspende

    return _setback <= now  &&  is_processable();
}

//----------------------------------------------------------------------------Order::is_processable

bool Order::is_processable()
{
    if( _is_on_blacklist ) return false;  
    if( _suspended )       return false;
    if( _task )            return false;               // Schon in Verarbeitung
    if( _is_replacement )  return false;
    if( _job_chain  &&  _job_chain->state() != Job_chain::s_ready )  return false;   // Jobkette wird nicht gelöscht?

    return true;
}

//----------------------------------------------------------Order::handle_changed_processable_state
// Nach Änderung von next_time(), also _setback und is_processable() zu rufen!

void Order::handle_changed_processable_state()
{
    Time new_next_time = next_time();

    if( new_next_time <= _old_next_time )
    {
        if( Job* job = _job_chain_node? _job_chain_node->_job : NULL )
        {
            job->signal_earlier_order( this );
        }
    }

    _old_next_time = new_next_time;
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

    if( distributed )
    {
        if( _run_time->set() )  z::throw_xc( "SCHEDULER-397", "<run_time>" );
        //if( !job_chain()  ||  !job_chain()->_is_distributed )  z::throw_xc( __FUNCTION__, obj_name(), "no job_chain or job_chain is not distributed" );
    }

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
    if( !_task )  z::throw_xc( "ORDER-HAS-NO-TASK", obj_name(), debug_text );
}

//---------------------------------------------------------------------------Order::occupy_for_task

bool Order::occupy_for_task( Task* task, const Time& now )
{
    assert( task );
    assert_no_task( __FUNCTION__ );   // Vorsichtshalber
    

    if( !_log->opened() )  open_log();

    if( _delay_storing_until_processing  &&  _job_chain  &&  _job_chain->_orders_recoverable  &&  !_is_in_database  &&  db()->opened() )
    {
        db_insert();
        _delay_storing_until_processing = false;
    }


    if( _moved )  z::throw_xc( "SCHEDULER-0", obj_name() + " _moved=true?" );

    _setback        = 0;
    _setback_called = false;
    _moved          = false;
    _task           = task;
    if( !_start_time )  _start_time = now;      

    if( _is_virgin )
    {
        if( _http_operation )  _http_operation->on_first_order_processing( task );
        order_subsystem()->count_started_orders();
    }

    _is_virgin = false;
    //_is_virgin_in_this_run_time = false;

    return true;
}

//------------------------------------------------------------------Order::db_occupy_for_processing

bool Order::db_occupy_for_processing()
{
    assert( _is_distributed );
    assert( _job_chain == NULL );   // Der Auftrag kommt erst nach der Belegung in die Jobkette, deshalb ist _job_chain == NULL

    _spooler->assert_are_orders_distributed( __FUNCTION__ );

    sql::Update_stmt update = db_update_stmt();

    update[ "occupying_cluster_member_id" ] = _spooler->cluster_member_id();

    //update.and_where_condition( "spooler_id", _spooler->id_for_db() );
    //update.and_where_condition( "job_chain" , job_chain->name() );
    //update.and_where_condition( "id"        , id().as_string()    );
    update.and_where_condition( "state"                      , state().as_string() );
    update.and_where_condition( "occupying_cluster_member_id", sql::null_value );
    update.and_where_condition( "distributed_next_time"      , calculate_db_distributed_next_time() );

    bool update_ok = false;

    for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
    {
        update_ok = ta.try_execute_single( update, __FUNCTION__ );
        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), __FUNCTION__ ); }

    
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
        Read_transaction ta ( _spooler->db() );

        Any_file result_set = ta.open_result_set
            ( 
                S() << "select `occupying_cluster_member_id`"
                       "  from " << _spooler->_orders_tablename
                       << db_where_clause().where_string(),
                __FUNCTION__ );

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
        _log->error( S() << x.what() << ", in " << __FUNCTION__ );
    }
}

//---------------------------------------------------------------------------------Order::db_insert

void Order::db_insert()
{
    bool ok = db_try_insert();
    if( !ok )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_name, "in database" );
}

//-------------------------f----------------------------------------------------Order::db_try_insert

bool Order::db_try_insert()
{
    bool   insert_ok     = false;
    string payload_string = string_payload();

    if( db()->opened() )
    for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
    {
        if( !db()->opened() )  break;

        if( _is_replacement )
        {
            Any_file result_set = ta.open_result_set
                ( 
                    S() << "select `occupying_cluster_member_id`"
                           "  from " << _spooler->_orders_tablename << " %update_lock" 
                           << db_update_stmt().where_string(), 
                    __FUNCTION__ 
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

                ta.execute_single( db_update_stmt().make_delete_stmt(), __FUNCTION__ );
            }
        }
        else
        if( !_is_distributed )
        {
            ta.execute( db_update_stmt().make_delete_stmt() + " and `distributed_next_time` is null", __FUNCTION__ );
        }
        else
        {
            // Satz darf nicht vorhanden sein.
        }


        int ordering = db_get_ordering( &ta );
        

        sql::Insert_stmt insert ( ta.database_descriptor(), _spooler->_orders_tablename );
        
        insert[ "ordering"      ] = ordering;
        insert[ "job_chain"     ] = _job_chain_name;
        insert[ "id"            ] = id().as_string();
        insert[ "spooler_id"    ] = _spooler->id_for_db();
        insert[ "title"         ] = title()                     , _title_modified      = false;
        insert[ "state"         ] = state().as_string();
        insert[ "state_text"    ] = state_text()                , _state_text_modified = false;
        insert[ "priority"      ] = priority()                  , _priority_modified   = false;
        insert[ "initial_state" ] = initial_state().as_string();

        db_fill_stmt( &insert );

        insert.set_datetime( "created_time", _created.as_string(Time::without_ms) );

        for( int insert_race_retry_count = 1; !insert_ok; insert_race_retry_count++ )
        {
            try
            {
                ta.execute( insert, __FUNCTION__ );
                insert_ok = true;
            }
            catch( exception& x )     // Datensatz ist bereits vorhanden?
            {
                if( insert_race_retry_count > max_insert_race_retry_count )  throw;

                ta.intermediate_rollback( __FUNCTION__ );      // Postgres verlangt nach Fehler ein Rollback

                Any_file result_set = ta.open_result_set
                    ( 
                        S() << "select " << order_select_database_columns << 
                               " from " << _spooler->_orders_tablename << 
                               db_where_clause().where_string(), 
                        __FUNCTION__ 
                    );

                if( !result_set.eof() )  break;

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

            if( run_time() )
            {
                xml::Document_ptr doc = run_time()->dom_document();
                if( doc.documentElement().hasAttributes()  ||  doc.documentElement().hasChildNodes() )  db_update_clob( &ta, "run_time", doc.xml() );
            }

            ta.commit( __FUNCTION__ );

            _is_in_database = true;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-305", _spooler->_orders_tablename, x ), __FUNCTION__ ); }


    if( insert_ok )  tip_own_job_for_new_distributed_order_state();

    return insert_ok;
}

//---------------------------------------------------------------------Order::db_release_occupation

bool Order::db_release_occupation()
{
    bool update_ok = false;

    if( _is_db_occupied  &&  _spooler->cluster_member_id() != "" )
    {
        for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
        {
            sql::Update_stmt update = db_update_stmt();

            update[ "occupying_cluster_member_id" ] = sql::null_value;
            update.and_where_condition( "occupying_cluster_member_id", _spooler->cluster_member_id() );
            update.and_where_condition( "state"                         , _occupied_state.as_string()     );

            update_ok = ta.try_execute_single( update, __FUNCTION__ );

            ta.commit( __FUNCTION__ );

            if( !update_ok ) 
            {
                _log->error( message_string( "SCHEDULER-816" ) );
                db_show_occupation( log_error );
            }

            _is_db_occupied = false; 
        }
        catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x ), __FUNCTION__ ); }
    }

    return update_ok;
}

//--------------------------------------------------------------------------------Order::db_update2

bool Order::db_update2( Update_option update_option, bool delet, Transaction* outer_transaction )
{
    bool update_ok = false;

    // outer_transaction nur für db_handle_modified_order()

    if( update_option == update_and_release_occupation  &&  _spooler->_are_all_tasks_killed )
    {
        _log->warn( message_string( "SCHEDULER-830" ) );   // "Because all Scheduler tasks are killed, the order in database is not updated. Only the occupation is released"
        update_ok = db_release_occupation();
    }
    else
    if( _is_in_database &&  _spooler->_db->opened() )
    {
        if( update_option == update_not_occupied  &&  _is_db_occupied )  z::throw_xc( __FUNCTION__, "is_db_occupied" );


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


        //if( _job_chain_name == "" )
        if( delet )
        {
            for( Retry_nested_transaction ta ( _spooler->_db, outer_transaction ); ta.enter_loop(); ta++ ) try
            {
                if( !_spooler->_db->opened() )  break;

                S delete_sql;
                delete_sql << update.make_delete_stmt();
                delete_sql << " and `distributed_next_time` is " << ( _is_distributed? "not null" : " null" );  // update_ok=false, wenn das nicht stimmt

                update_ok = ta.try_execute_single( delete_sql, __FUNCTION__ );
                if( !update_ok )  update_ok = db_handle_modified_order( &ta );  //int DISTRIBUTED_FEHLER_KOENNTE_GEZEIGT_WERDEN; // Zeigen, wenn distributed_next_time falsch ist.

                db()->write_order_history( this, &ta );

                ta.commit( __FUNCTION__ );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x  ), __FUNCTION__ ); }

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

            db_fill_stmt( &update );

            if( _priority_modified   )  update[ "priority"   ] = priority();
            if( _title_modified      )  update[ "title"      ] = title();
            if( _state_text_modified )  update[ "state_text" ] = state_text();

            for( Retry_nested_transaction ta ( _spooler->_db, outer_transaction ); ta.enter_loop(); ta++ ) try
            {
                if( !_spooler->_db->opened() )  break;

                update_ok = ta.try_execute_single( update, __FUNCTION__ );

                if( !update_ok )
                {
                    update_ok = db_handle_modified_order( &ta );
                }
                else
                //if( !finished() )
                {
                    // _run_time_modified gilt nicht für den Datenbanksatz, sondern für den Auftragsneustart
                    // Vorschlag: xxx_modified auflösen zugunsten eines gecachten letzten Datenbanksatzes, mit dem verglichen wird.
                    if( run_time() ) 
                    {
                        xml::Document_ptr doc = run_time()->dom_document();
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

                ta.commit( __FUNCTION__ );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x ), __FUNCTION__ ); }
        }

        if( update_option == update_and_release_occupation )
        {
            if( update_ok )  _is_db_occupied = false, _occupied_state = Variant();
            else
            if( _is_db_occupied )
            {
                _log->error( message_string( "SCHEDULER-816" ) );
                db_show_occupation( log_error );
            }
        }

        _order_xml_modified  = false;            
        _state_text_modified = false; 
        _title_modified      = false;
        _state_text_modified = false;

        tip_own_job_for_new_distributed_order_state();
    }
    else
    if( finished() )  
        _spooler->_db->write_order_history( this );

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
        if( ptr<Order> modified_order = order_subsystem()->try_load_order_from_database( outer_transaction, _job_chain_name, _id, Order_subsystem::lo_lock ) )
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
        _log->error( S() << x.what() << ", in " << __FUNCTION__ );
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

//--------------------------------------------------------Order::calculate_db_distributed_next_time

string Order::calculate_db_distributed_next_time()
{
    string result;

    if( _is_distributed )
    {
        if( _is_on_blacklist )    result = blacklist_database_distributed_next_time;
        else
        if( _is_replacement )  result = replacement_database_distributed_next_time;
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
    if( _spooler->_db->db_name() == "" )  z::throw_xc( "SCHEDULER-361", __FUNCTION__ );

    return ta->read_clob( _spooler->_orders_tablename, column_name, db_where_clause().where_string() );
}

//----------------------------------------------------------------------------Order::db_update_clob

void Order::db_update_clob( Transaction* ta, const string& column_name, const string& value )
{
    if( _spooler->_db->db_name() == "" )  z::throw_xc( "SCHEDULER-361", __FUNCTION__ );

    if( value == "" )
    {
        sql::Update_stmt update = db_update_stmt();
        update[ column_name ].set_direct( "null" );
        ta->execute( update, __FUNCTION__ );
    }
    else
    {
        ta->update_clob( _spooler->_orders_tablename, column_name, value, db_where_clause().where_string() );
    }
}

//----------------------------------------------------------------------------Order::db_update_stmt

sql::Update_stmt Order::db_update_stmt()
{
    sql::Update_stmt result ( _spooler->database_descriptor(), _spooler->_orders_tablename );
    db_fill_where_clause( &result );
    return result;
}

//---------------------------------------------------------------------------Order::db_where_clause

sql::Where_clause Order::db_where_clause()
{
    sql::Where_clause result ( _spooler->database_descriptor() );
    db_fill_where_clause( &result );
    return result;
}

//----------------------------------------------------------------------Order::db_fill_where_clause

void Order::db_fill_where_clause( sql::Where_clause* where )
{
    assert( _job_chain_name != "" );

    where->and_where_condition( "spooler_id", _spooler->id_for_db() );
    where->and_where_condition( "job_chain" , _job_chain_name       );
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
    if( _job_chain_name != ""  &&  _spooler->_order_history_with_log  &&  !string_begins_with( _spooler->log_directory(), "*" ) )
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

        _log->set_filename( _spooler->log_directory() + "/order." + _job_chain_name + "." + name + ".log" );      // Jobprotokoll
        _log->set_remove_after_close( true );
        _log->open();
    }
}

//-------------------------------------------------------------------------------------Order::close

void Order::close( Close_flag close_flag )
{
/*
    if( !_log->filename().empty() )
    {
        try
        {
            remove_file( _log->filename() );
        }
        catch( const exception& x )
        {
            _spooler->_log->warn( "FEHLER BEIM LÖSCHEN DER DATEI " + _log->filename() + ": " + x.what() );
        }
    }
*/
    if( _http_operation )
    {
        _http_operation->unlink_order();
        _http_operation = NULL;
    }


    _task = NULL;
    //_removed_from_job_chain = NULL;
    set_replacement( false );
    if( _replaced_by )  _replaced_by->set_replacement( false ), _replaced_by = NULL;

    if( _is_db_occupied )
    {
        Z_LOGI2( "scheduler", __FUNCTION__ << "  db_release_occupation()\n" );
        db_release_occupation();
    }

    if( close_flag == cls_remove_from_job_chain )  remove_from_job_chain();
    else
    if( _job_chain )  _job_chain->remove_order( this );

    if( _run_time )  _run_time->close(), _run_time = NULL;

    _log->close();
}

//-----------------------------------------------------------------------------------Order::set_dom

void Order::set_dom( const xml::Element_ptr& element, Variable_set_map* variable_set_map )
{
    if( !element )  return;

    string priority         = element.getAttribute( "priority"  );
    string id               = element.getAttribute( "id"        );
    string title            = element.getAttribute( "title"     );
    string state_name       = element.getAttribute( "state"     );
    string web_service_name = element.getAttribute( "web_service" );
    string at_string        = element.getAttribute( "at" );
    string setback          = element.getAttribute( "setback" );
    _setback_count          = element.int_getAttribute( "setback_count", _setback_count );

    if( element.bool_getAttribute( "replacement" ) )  set_replacement( true );
    _replaced_order_occupator = element.getAttribute( "replaced_order_occupator" );

    if( priority         != "" )  set_priority( as_int(priority) );
    if( id               != "" )  set_id      ( id.c_str() );
    if( title            != "" )  set_title   ( title );
    if( state_name       != "" )  set_state   ( state_name.c_str() );
    if( web_service_name != "" )  set_web_service( _spooler->_web_services->web_service_by_name( web_service_name ), true );
    if( at_string        != "" )  set_at      ( Time::time_with_now( at_string ) );
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
            set_params( e );
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
        if( e.nodeName_is( "run_time" ) )
        { 
            set_run_time( e );
        }
        else
        if( e.nodeName_is( "log" ) )
        {
            assert( !_log->opened() );
            _log->continue_with_text( e.text() );
        }
    }
}

//-------------------------------------------------------------------------------Order::dom_element

xml::Element_ptr Order::dom_element( const xml::Document_ptr& document, const Show_what& show, const string* log ) const
{
    xml::Element_ptr element = document.createElement( "order" );

    if( !show.is_set( show_for_database_only ) )
    {
        if( !_id.is_empty() )
        {
            element.setAttribute( "order"     , debug_string_from_variant( _id ) );
            element.setAttribute( "id"        , debug_string_from_variant( _id ) );     // veraltet
        }
    }

    if( !show.is_set( show_for_database_only ) ) // &&  !show.is_set( show_id_only ) )
    {
        if( _setback )
        element.setAttribute( "next_start_time", _setback.as_string() );

        if( _title != "" )
        element.setAttribute( "title"     , _title );

        if( !_state.is_empty() )
        element.setAttribute( "state"     , debug_string_from_variant( _state ) );

        if( !_initial_state.is_empty() )
        element.setAttribute( "initial_state", debug_string_from_variant( _initial_state ) );

        if( Job_chain* job_chain = this->job_chain_for_api() )
        element.setAttribute( "job_chain" , job_chain->name() );

        if( _replaced_by )
        element.setAttribute( "replaced"  , "yes" );
        else
        if( _removed_from_job_chain_name != "" )
        element.setAttribute( "removed"   , "yes" );

        if( Job* job = this->job() )  element.setAttribute( "job", job->name() );
        else
        if( Job_chain* job_chain = job_chain_for_api() )
        {
            if( Job_chain_node* node = job_chain->node_from_state_or_null( _state ) )
                if( node->_job )  element.setAttribute( "job", node->_job->name() );
        }

        if( _task )
        {
            element.setAttribute( "task"            , _task->id() );   // Kann nach set_state() noch die Vorgänger-Task sein (bis spooler_process endet)
            element.setAttribute( "in_process_since", _task->last_process_start_time().as_string() );
        }

        if( _state_text != "" )
        element.setAttribute( "state_text", _state_text );

        element.setAttribute( "priority"  , _priority );

        if( _created )
        element.setAttribute( "created"   , _created.as_string() );

        if( _log->opened() )
        element.setAttribute( "log_file"  , _log->filename() );

        if( _is_in_database  &&  _job_chain_name != ""  &&  !_job_chain )
        element.setAttribute( "in_database_only", "yes" );

        if( show.is_set( show_payload )  &&  !_payload.is_null_or_empty_string()  &&  !_payload.is_missing() )
        {
            xml::Element_ptr payload_element = element.append_new_element( "payload" );
            xml::Node_ptr    payload_content;

            if( _payload.vt == VT_DISPATCH )
            {
                if( Com_variable_set* variable_set = dynamic_cast<Com_variable_set*>( V_DISPATCH( &_payload ) ) )
                {
                    payload_content = variable_set->dom_element( document, "params", "param" );
                }
            }

            if( !payload_content )
            {
                string payload_string = string_payload();
                if( !payload_content )  payload_content = document.createTextNode( payload_string );
            }

            payload_element.appendChild( payload_content );
        }

        if( show.is_set( show_run_time ) )  element.appendChild( _run_time->dom_element( document ) );
    }

    if( show.is_set( show_log )  ||  show.is_set( show_for_database_only ) )
    {
        Show_what log_show_what = show;
        if( show.is_set( show_for_database_only ) )  log_show_what |= show_log;

        if( log  &&  show.is_set( show_log ) ) element.append_new_text_element( "log", *log );     // Protokoll aus der Datenbank
        else
        if( _log )  element.appendChild( _log->dom_element( document, log_show_what ) );
    }

    if( show.is_set( show_payload | show_for_database_only )  &&  _xml_payload != "" )
    {
        xml::Element_ptr xml_payload_element = element.append_new_element( "xml_payload" );

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
    element.setAttribute( _setback_count == 0? "at" : "setback", _setback.as_string() );

    if( _setback_count > 0 )
    element.setAttribute( "setback_count", _setback_count );

    if( _web_service )
    element.setAttribute( "web_service", _web_service->name() );

    if( _http_operation  &&  _http_operation->web_service_operation_or_null() )
    element.setAttribute( "web_service_operation", _http_operation->web_service_operation_or_null()->id() );

    if( _is_on_blacklist )  element.setAttribute( "on_blacklist", "yes" );
    if( _suspended       )  element.setAttribute( "suspended"   , "yes" );
    if( _is_replacement  )  element.setAttribute( "replacement" , "yes" ),
                            element.setAttribute_optional( "replaced_order_occupator", _replaced_order_occupator );
    if( !_is_virgin      )  element.setAttribute( "touched"     , "yes" );

    if( start_time()     )  element.setAttribute( "start_time", start_time().as_string() );
    if( end_time()       )  element.setAttribute( "end_time"  , end_time  ().as_string() );
    
    return element;
}

//--------------------------------------------------------------Order::append_calendar_dom_elements

void Order::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    xml::Node_ptr    node_before     = element.lastChild();
    xml::Element_ptr setback_element;

    if( is_processable()  &&  !_setback.is_never() )
    {
        setback_element = new_calendar_dom_element( element.ownerDocument(), _setback );
        element.appendChild( setback_element );
    }

    if( _run_time )
    {
        _run_time->append_calendar_dom_elements( element, options );

        for( xml::Simple_node_ptr node = node_before? node_before.nextSibling() : element.firstChild();
             node;
             node = node.nextSibling() )
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

                if( _job_chain_name != "" )  e.setAttribute( "job_chain", _job_chain_name );
                e.setAttribute( "order", string_id() );
            }
        }
    }
}

//--------------------------------------------------------Order::print_xml_child_elements_for_event

void Order::print_xml_child_elements_for_event( String_stream* s, Scheduler_event* )
{
    *s << "<order";
    
    if( _job_chain )
    *s << " job_chain=\"" << xml::encode_attribute_value( _job_chain_name )      << '"';
    *s << " id=\""        << xml::encode_attribute_value( string_id() )          << '"';

    if( _title != "" )
    *s << " title=\""     << xml::encode_attribute_value( _title )               << '"';

    *s << " state=\""     << xml::encode_attribute_value( _state.as_string() )   << '"';

    *s << "/>";
}

//---------------------------------------------------------------------------------------Order::dom

xml::Document_ptr Order::dom( const Show_what& show ) const
{
    xml::Document_ptr document;

    document.create();
    document.appendChild( dom_element( document, show ) );

    return document;
}

//-------------------------------------------------------------------------------Order::order_queue

Order_queue* Order::order_queue()
{
    Job* job = this->job();

    if( !job )  z::throw_xc( "SCHEDULER-163" );

    return job->order_queue();
}

//---------------------------------------------------------------------------Order::set_job_by_name

void Order::set_job_by_name( const string& jobname )
{
    set_job( _spooler->job_subsystem()->get_job( jobname ) );
}

//------------------------------------------------------------------------------------Order::set_id

void Order::set_id( const Order::Id& id )
{
    //THREAD_LOCK(_lock)
    {
        if( _id_locked )  z::throw_xc( "SCHEDULER-159" );

        string id_string = string_id( id );    // Sicherstellen, das id in einen String wandelbar ist

        if( db()->opened()  &&  id_string.length() > db()->order_id_length_max() )  
            z::throw_xc( "SCHEDULER-345", id_string, db()->order_id_length_max(), _spooler->_orders_tablename + "." + "id" );

        if( id_string.length() > const_order_id_length_max )  z::throw_xc( "SCHEDULER-344", id_string, const_order_id_length_max );


        _id = id;

        //_log->set_prefix( "Order " + _id.as_string() );
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
    catch( exception& x )  { Z_LOG2( "scheduler", __FUNCTION__ << " " << x.what() << "\n" ); }

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
    ptr<Com_variable_set> pars = new Com_variable_set;
    pars->set_dom( params_element, variable_set_map );  
    set_payload( Variant( static_cast<IDispatch*>( pars ) ) );
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
    if( FAILED(hr) )  throw_com( hr, __FUNCTION__, name );
}

//-------------------------------------------------------------------------------------Order::param

Variant Order::param( const string& name )
{
    Variant result;

    if( ptr<Com_variable_set> params = params_or_null() )  
    {
        HRESULT hr = params->get_Var( Bstr( name ), &result );
        if( FAILED(hr) )  throw_com( hr, __FUNCTION__, name );
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
    if( _removed_from_job_chain_name != "" )
    {
        _log->warn( message_string( "SCHEDULER-298", job->name() ) );   //S() << "job=" << job->name() << " wird ignoriert, weil Auftrag bereits aus der Jobkette entfernt ist" );
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

    //THREAD_LOCK( _lock )
    {
        if( _job_chain_node )  result = _job_chain_node->_job;
        else
        if( _order_queue    )  result = _order_queue->job();
    }

    return result;
}

//-------------------------------------------------------------------------------Order::set_payload

void Order::set_payload( const VARIANT& payload )
{
    //THREAD_LOCK( _lock )
    {
        //Z_LOG2( "scheduler.order", obj_name() << ".payload=" << debug_string_from_variant(payload) << "\n" );
        _payload = payload;
        //_payload_modified = true;
    }
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
    if( job_chain() )
    {
        if( Job_chain_node* node = job_chain()->node_from_state_or_null( _state ) )
        {
            if( node->is_end_state() )  result = true;
        }
        else 
            result = true;
    }
    
    return result;

    //return _end_state_reached  ||  !_job_chain_node  ||  !_job_chain_node->_job;
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

void Order::set_state( const State& state )
{
    //if( _removed_from_job_chain )
    //{
    //    _log->warn( S() << "state=" << debug_string_from_variant( state ) << " wird ignoriert, weil Auftrag bereits aus der Jobkette entfernt ist" );
    //    return;
    //}
    check_state( state );

    bool is_same_state = state == _state;

    if( _job_chain )
    {
        move_to_node( _job_chain->node_from_state( state ) );

        if( !is_same_state  &&  ( !_job_chain_node  ||  !_job_chain_node->_job ) )
        {
            handle_end_state();
        }
    }
    else  
        set_state2( state );

    clear_setback();
}

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state, const Time& start_time )
{
    set_state( state );
    set_setback( start_time );
}

//--------------------------------------------------------------------------------Order::set_state2

void Order::set_state2( const State& state, bool is_error_state )
{
    if( state != _state )
    {
        _state = state;

        if( _job_chain )
        {
            string log_line = "set_state " + ( state.is_missing()? "(missing)" : state.as_string() );

            if( _job_chain_node && _job_chain_node->_job )  log_line += ", " + _job_chain_node->_job->obj_name();
            if( is_error_state                           )  log_line += ", error state";

            if( _setback )  log_line += ", at=" + _setback.as_string();

            if( _suspended )  log_line += ", suspended";

            _log->info( log_line );
        }

        if( _id_locked )
        {
            Scheduler_event event ( evt_order_state_changed, log_info, this );
            _spooler->report_event( &event );
        }
    }

    if( !_initial_state_set )  _initial_state = state,  _initial_state_set = true;
}

//------------------------------------------------------------------------Order::set_job_chain_node

void Order::set_job_chain_node( Job_chain_node* node, bool is_error_state )
{
    _job_chain_node = node;

    if( node )
    {
        if( node->_suspend )  set_suspended();
        if( node->_delay  &&  !at() )  set_at( Time::now() + node->_delay );
    }

    set_state2( node? node->_state : empty_variant, is_error_state );
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Job_chain_node* node )
{
    if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );

    bool is_same_node = node == _job_chain_node;

    if( _is_on_blacklist )  remove_from_blacklist();
    if( _task )  _moved = true;

    if( !is_same_node  &&  _job_chain_node  &&  _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this ), _job_chain_node = NULL;

    clear_setback();
    set_job_chain_node( node );

    if( !is_same_node  &&  !_is_distributed && node && node->_job )  node->_job->order_queue()->add_order( this );
}

//------------------------------------------------------------------------------Order::set_priority

void Order::set_priority( Priority priority )
{
    //THREAD_LOCK( _lock )
    {
        if( _priority != priority )
        {
            _priority = priority;

            //2006-11-23  if( !_setback  &&  _in_job_queue  &&  !_task )   // Nicht gerade in Verarbeitung?
            if( _in_job_queue  &&  !_task )   // Nicht gerade in Verarbeitung?
            {
                order_queue()->reinsert_order( this );
            }
        }

        _priority_modified = true;
    }
}

//-----------------------------------------------------------------------------------Order::com_job

Com_job* Order::com_job()
{
    Com_job* result = NULL;

    //THREAD_LOCK( _lock )
    {
        Job* j = job();
        if( j )  result = j->com_job();
    }

    return result;
}

//--------------------------------------------------------------------------------Order::add_to_job

void Order::add_to_job( const string& job_name )
{
    assert_is_not_distributed( __FUNCTION__ );

    //THREAD_LOCK( _lock )
    {
        ptr<Order_queue> order_queue = _spooler->job_subsystem()->get_job( job_name )->order_queue();
        if( !order_queue )  z::throw_xc( "SCHEDULER-147", job_name );
        add_to_order_queue( order_queue );
    }
}

//---------------------------------------------------------------------------Order::remove_from_job

void Order::remove_from_job()
{
    if( _in_job_queue )
    {
        if( Order_queue* order_queue = _job_chain_node->_job->order_queue() )        // Kann bei Programmende NULL sein
            order_queue->remove_order( this );

        if( _order_queue )  _order_queue->remove_order( this );     // Auftrag ist nicht in einer Jobkette
    }

    _job_chain_node = NULL;
}

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_order_queue( Order_queue* order_queue )
{
    assert_is_not_distributed( __FUNCTION__ );

    if( !order_queue )  z::throw_xc( "SCHEDULER-147", "?" );

    ptr<Order> me = this;   // Halten

    //THREAD_LOCK( _lock )
    {
        if( _task )  _moved = true;

        if( _job_chain_name != "" )  remove_from_job_chain();
        _removed_from_job_chain_name = "";

        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        order_queue->add_order( this );
        _order_queue = order_queue;
    }
}

//---------------------------------------------------------------------Order::remove_from_job_chain

void Order::remove_from_job_chain()
{
    ptr<Order> me = this;        // Halten

    if( !_end_time )  _end_time = Time::now();

    if( _job_chain_name != "" )
    {
        if( _job_chain || _is_in_database )     // Nur Protokollieren, wenn wirklich etwas entfernt wird, aus Jobkette im Speicher oder Datenbank (_is_distributed)
        {
            _log->info( _task? message_string( "SCHEDULER-941", _task->obj_name() ) 
                             : message_string( "SCHEDULER-940" ) );
        }

        if( _is_in_database )  db_delete( update_and_release_occupation );
    }

    if( _job_chain )  _job_chain->remove_order( this );

    _setback_count = 0;
    _setback = Time(0);

    set_replacement( false );

    //_id_locked = false;
    

    _job_chain = NULL;
    _job_chain_name = "";
}

//--------------------------------------------------------------------------Order::place_in_job_chain

void Order::place_in_job_chain( Job_chain* job_chain )
{
    bool ok = try_place_in_job_chain( job_chain );
    if( !ok )  z::throw_xc( "SCHEDULER-186", obj_name(), job_chain->name() );
}

//----------------------------------------------------------------------Order::try_place_in_job_chain

bool Order::try_place_in_job_chain( Job_chain* job_chain )
{
    bool is_new = true;


    if( _id.vt == VT_EMPTY )  set_default_id();
    _id_locked = true;


    if( _is_replacement  &&  job_chain->_is_distributed )
    {
        // Bestehender Auftrag wird in der Datenbank ersetzt (_is_replacement steuert das)
    }
    else
    {
        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
        {
            is_new = !job_chain->has_order_id( &ta, id() );
        }
        catch( exception& x ) { ta.reopen_database_after_error( x, __FUNCTION__ ); }
    }

    if( is_new )
    {
        ptr<Order> hold_me = this;   // Halten für remove_from_job_chain()
        
        if( _job_chain_name != "" )  remove_from_job_chain();
        assert( !_job_chain );
        
        if( _state.vt == VT_EMPTY )  set_state2( job_chain->first_node()->_state );     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

        job_chain->job_from_state( _state );     // Fehler bei Endzustand. Wir speichern den Auftrag nur, wenn's einen Job zum Zustand gibt
        Job_chain_node* node = job_chain->node_from_state( _state );

        if( node->_suspend )  _suspended = true;

        if( !_is_distribution_inhibited  &&  job_chain->_is_distributed )  set_distributed();

        _job_chain_name = job_chain->name();
        _removed_from_job_chain_name = "";

        //set_setback( _state == _initial_state  &&  !_setback  &&  _run_time->set()? next_start_time( true ) : _setback );
        activate();

        if( _delay_storing_until_processing ) 
        {
            if( _is_distributed )  z::throw_xc( __FUNCTION__, "_delay_storing_until_processing & _is_distributed not possible" );   // db_try_insert() muss Datenbanksatz prüfen können
        }
        else
        if( job_chain->_orders_recoverable  &&  !_is_in_database )
        {
            if( db()->opened() )  is_new = db_try_insert();       // false, falls aus irgendeinem Grund die Order-ID schon vorhanden ist
        }

        if( is_new  &&  !_is_distributed )
        {
            job_chain->add_order( this );
        }
    }

    return is_new;
}

//-------------------------------------------------------------Order::place_or_replace_in_job_chain

void Order::place_or_replace_in_job_chain( Job_chain* job_chain )
{
    assert( job_chain );

    if( job_chain->_is_distributed )
    {
        set_replacement( true );
        place_in_job_chain( job_chain );
    }
    else
    {
        if( ptr<Order> other_order = job_chain->order_or_null( id() ) )  // Nicht aus der Datenbank gelesen
        {
            other_order->remove_from_job_chain();
            place_in_job_chain( job_chain );

            if( other_order->_task )
            {
                set_replacement( other_order );
                _replaced_order_occupator = other_order->_task->obj_name();
                _log->info( message_string( "SCHEDULER-942", _replaced_order_occupator, other_order->obj_name() ) );       // add_or_replace_order(): Auftrag wird verzögert bis <p1/> <p2/> ausgeführt hat
            }
        }
        else
        {
            place_in_job_chain( job_chain );
        }
    }
}

//----------------------------------------------------------------------------------Order::activate

void Order::activate()
{
    if( order_subsystem()->subsystem_state() == subsys_active )  
    {
        _order_state = s_active;
    }

    set_next_start_time();
}

//-----------------------------------------------------------------------Order::set_next_start_time

void Order::set_next_start_time()
{
    if( _state == _initial_state  &&  !_setback  &&  _run_time->set() )
    {
        if( _order_state == s_active )  
        {
            set_setback( next_start_time( true ) );     // Braucht für <run_time start_time_function=""> das Scheduler-Skript
        }
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
           _job_chain_name == ""? NULL 
                                : order_subsystem()->job_chain( _job_chain_name );
}

//---------------------------------------------------------------------------------Order::job_chain

Job_chain* Order::job_chain_for_api() const
{ 
    return  _job_chain                        ? _job_chain :
            _removed_from_job_chain_name != ""? order_subsystem()->job_chain_or_null( _removed_from_job_chain_name ) :
            _job_chain_name              != ""? order_subsystem()->job_chain_or_null( _job_chain_name ) 
                                              : NULL; 
}

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( bool success )
{
    Job* last_job = _task? _task->job() : NULL;

    //THREAD_LOCK( _lock )
    {
        bool force_error_state = false;

        if( !_setback_called )  _setback_count = 0;

        if( !_suspended  &&  _setback == Time::never  &&  _setback_count > _task->job()->max_order_setbacks() )
        {
            _log->info( message_string( "SCHEDULER-943", _setback_count ) );   // " mal zurückgestellt. Der Auftrag wechselt in den Fehlerzustand"
            success = false;
            force_error_state = true;
            _setback = Time(0);
            _setback_count = 0;
        }

        if( _task  &&  _moved  &&  _job_chain_node  &&  _job_chain_node->_job )  _job_chain_node->_job->signal( "delayed set_state()" );
        _task = NULL;



        if( !is_setback() && !_moved && !_end_state_reached  ||  force_error_state )
        {
            if( _job_chain_node )
            {
                assert( _job_chain );

                if( _job_chain_node->_job )
                {
                    if( !_job_chain_node->_job->order_queue() )  _log->warn( "Job " + _job_chain_node->_job->obj_name() + " without order queue (§1495)" );  // Problem §1495
                    else  _job_chain_node->_job->order_queue()->remove_order( this );
                }

                Job_chain_node* new_node = success? _job_chain_node->_next_node
                                                  : _job_chain_node->_error_node;

                set_job_chain_node( new_node, !success );

                if( _job_chain_node  &&  _job_chain_node->_job )
                {
                    if( !_is_distributed )
                    {
                        _job_chain_node->_job->order_queue()->add_order( this );
                    }
                }
                else
                {
                    handle_end_state();
                }
            }
            else
            if( _order_queue )
            {
                _order_queue->remove_order( this );
                _order_queue = NULL;
            }
        }
        else
        if( _is_distributed  &&  _job_chain_node  &&  _job_chain_node->_job )
        {
            _job_chain_node->_job->order_queue()->remove_order( this );
        }

        postprocessing2( last_job );
    }
}

//--------------------------------------------------------------------------Order::handle_end_state

void Order::handle_end_state()
{
    // Endzustand erreicht

    bool is_first_call = _run_time_modified;
    _run_time_modified = false;
    Time next_start = next_start_time( is_first_call );

    if( next_start != Time::never  &&  _state != _initial_state )
    {
        _log->info( message_string( "SCHEDULER-944", _initial_state, next_start ) );        // "Kein weiterer Job in der Jobkette, der Auftrag wird mit state=<p1/> wiederholt um <p2/>"

        try
        {
            set_state( _initial_state, next_start );
        }
        catch( exception& x ) { _log->error( x.what() ); }

        _end_time = Time::now();
        _log->close_file();
        if( _job_chain  &&  _is_in_database )  _spooler->_db->write_order_history( this );  // Historie schreiben, aber Auftrag beibehalten
        _log->close();

        _start_time = 0;
        _end_time = 0;
        //_is_virgin_in_this_run_time = true;

        open_log();
    }
    else
    {
        _log->info( message_string( "SCHEDULER-945" ) );     // "Kein weiterer Job in der Jobkette, der Auftrag ist erledigt"
    }
}

//--------------------------------------------------------------------------Order::processing_error

void Order::processing_error()
{
    //THREAD_LOCK( _lock )
    {
        Job* last_job = _task? _task->job() : NULL;

        _task = NULL;

        if( _http_operation )      
        {
            _job_chain_node = NULL;         // Nicht auf Neustart des Jobs warten, sondern Auftrag beenden, damit die Web-Service-Operation abgeschlossen werden kann
        }

        postprocessing2( last_job );
    } 
}

//---------------------------------------------------------------------------Order::postprocessing2

void Order::postprocessing2( Job* last_job )
{
    Job* job = this->job();

    if( _moved  &&  job  &&  !order_queue()->has_order( Time::now() ) )
    {
        job->signal( "Order (delayed set_state)" );
    }

    _moved = false;


    if( finished() )
    {
        if( is_file_order()  &&  file_exists( file_path() ) )
        {
            _log->error( message_string( "SCHEDULER-340" ) );
            if( _job_chain )  set_on_blacklist();
        }

        try
        {
            if( _web_service  &&  !_http_operation )
            {
                _web_service->forward_order( *this, last_job );
            }
        }
        catch( exception x )  { _log->error( x.what() ); }
    }


    if( _suspended  &&  _job_chain  &&  ( !_job_chain_node  ||  !_job_chain_node->_job ) )
    //if( _suspended  &&  end_state_reached() )
    {
        set_on_blacklist();
    }


    if( finished() )
    {
        _end_time = Time::now();
        order_subsystem()->count_finished_orders();
    }

    if( _job_chain  &&  ( _is_in_database || finished() ) )
    {
        try
        {
            if( !_is_on_blacklist  &&  finished() )  db_delete( update_and_release_occupation );
                                               else  db_update( update_and_release_occupation );
        }
        catch( exception& x )
        {
            _log->error( message_string( "SCHEDULER-313", x ) );
        }
    }

    if( _is_distributed  &&  _job_chain )
    {
        _job_chain->remove_order( this );
        close( Order::cls_dont_remove_from_job_chain );
    }


    if( finished() )
    {
        if( !_is_on_blacklist )  close( Order::cls_remove_from_job_chain ); 
    }
}

//-----------------------------------------------------------------------------Order::set_suspended

void Order::set_suspended( bool suspended )
{
    //if( suspended  &&  !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name(), __FUNCTION__ );

    if( _suspended != suspended )
    {
        _suspended = suspended;
        _order_xml_modified = true;

        if( _job_chain )
        {
            if( _is_on_blacklist  &&  !suspended )  remove_from_job_chain();
            else
            if( _in_job_queue )  order_queue()->reinsert_order( this );

            if( _suspended )  _log->info( message_string( "SCHEDULER-991" ) );
                        else  _log->info( message_string( "SCHEDULER-992", _setback ) );
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

void Order::setback()
{
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
    }
    else
    {
        _setback = Time::never;  // Das heißt: Der Auftrag kommt in den Fehlerzustand
        _log->warn( message_string( "SCHEDULER-947", _setback_count, maximum ) );   // "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, ""das ist über dem Maximum " + as_string(maximum) + " des Jobs" );
    }

    order_queue()->reinsert_order( this );

    // Weitere Verarbeitung in postprocessing()
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
        if( _in_job_queue )  order_queue()->reinsert_order( this );

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
    assert_no_task( __FUNCTION__ );
    if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_name );

    set_setback( time );
}

//--------------------------------------------------------------------------------Order::next_time

Time Order::next_time()
{
    return is_processable()? _setback 
                           : Time::never;
}

//---------------------------------------------------------------------------Order::next_start_time

Time Order::next_start_time( bool first_call )
{
    Time result = Time::never;

    if( _run_time->set() )
    {
        Time now = Time::now();

        if( first_call )
        {
            //_period = _run_time->next_period( now, time::wss_next_single_start );
            _period = _run_time->next_period( now, time::wss_next_period_or_single_start );
            result = _period.begin();
        }
        else
        {
            result = now + _period.repeat();

            if( result >= _period.end() )       // Periode abgelaufen?
            {
                Period next_period = _run_time->next_period( _period.end(), time::wss_next_begin );
                //Z_DEBUG_ONLY( fprintf(stderr,"%s %s\n", __FUNCTION__, next_period.obj_name().c_str() ) );
                
                if( _period.repeat() == Time::never
                 || _period.end()    != next_period.begin()
                 || _period.repeat() != next_period.repeat() )
                {
                    result = next_period.begin();  // Perioden sind nicht nahtlos: Wiederholungsintervall neu berechnen
                }

                if( next_period.end() < now )   // Nächste Periode ist auch abgelaufen?
                {
                    next_period = _run_time->next_period( now );
                    result = next_period.begin();
                }

                _period = next_period;
            }


            // Aber gibt es ein single_start vorher?

            Period next_single_start_period = _run_time->next_period( now, time::wss_next_single_start );
            if( result > next_single_start_period.begin() )
            {
                _period = next_single_start_period;
                result  = next_single_start_period.begin();
            }
        }

        if( result < now )  result = 0;
    }

    return result;
}

//-----------------------------------------------------------------Order::on_before_modify_run_time

void Order::on_before_modify_run_time()
{
    if( _is_distributed )  z::throw_xc( "SCHEDULER-397", "<run_time>" );
  //if( _task       )  z::throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
  //if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_name );
}

//-------------------------------------------------------------------Order::run_time_modified_event

void Order::run_time_modified_event()
{
    if( _order_state == s_active )
    {
        _setback = 0;           // Änderung von <run_time> überschreibt Order.at
        set_next_start_time();
    }

    //if( _state == _initial_state )  set_setback( _run_time->set()? next_start_time( true ) : Time(0) );
    //                         else  _run_time_modified = true;
}

//------------------------------------------------------------------------------Order::set_run_time

void Order::set_run_time( const xml::Element_ptr& e )
{
    _run_time = Z_NEW( Run_time( this ) );
    _run_time->set_modified_event_handler( this );

    if( e )  _run_time->set_dom( e );       // Ruft set_setback() über modify_event()
       else  run_time_modified_event();
}

//---------------------------------------------------------------------------Order::set_replacement

void Order::set_replacement( Order* replaced_order )
{
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
    if( !_job_chain )  z::throw_xc( __FUNCTION__, "no _job_chain" );        // Wenn _is_distributed

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

Order_subsystem* Order::order_subsystem() const
{
    return static_cast<Order_subsystem*>( _spooler->order_subsystem() );
}

//----------------------------------------------------------------------------------Order::obj_name

string Order::obj_name() const
{
    string result = "Order ";

    //THREAD_LOCK( _lock )
    {
        if( Job_chain* job_chain = this->job_chain_for_api() )  result += job_chain->name() + ":";

        result += debug_string_from_variant(_id);
        if( _title != "" )  result += " " + quoted_string( _title );
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
