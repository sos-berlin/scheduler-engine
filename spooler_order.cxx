// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "Order_subsystem_impl.h"

using stdext::hash_set;
using stdext::hash_map;

namespace sos {
namespace scheduler {
namespace order {

using namespace job_chain;

//--------------------------------------------------------------------------------------------const

const int    check_database_orders_period               = 15;
const int    check_database_orders_period_minimum       = 1;
const int    database_orders_read_ahead_count           = 1;
const int    unlimited_max_orders                       = 99999;

//--------------------------------------------------------------------------------------------const

// Datenbank-Feld distributed_next_time
const string now_database_distributed_next_time         = "2000-01-01 00:00:00";        // Auftrag ist verteilt und ist sofort ausführbar
const string never_database_distributed_next_time       = "3111-11-11 00:00:00";        // Auftrag ist verteilt, hat aber keine Startzeit (weil z.B. suspendiert)
const string blacklist_database_distributed_next_time   = "3111-11-11 00:01:00";        // Auftrag ist auf der schwarzen Liste
const string replacement_database_distributed_next_time = "3111-11-11 00:02:00";        // <order replacement="yes">
// distributed_next_time is null => Auftrag ist nicht verteilt

//--------------------------------------------------------------------------------------------const

const string default_end_state_name                     = "<END_STATE>";
const string order_select_database_columns              = "`id`, `priority`, `state`, `state_text`, `initial_state`, `title`, `created_time`";

//---------------------------------------------------------------------------------Job_chain_folder

struct Job_chain_folder : Job_chain_folder_interface
{
                                Job_chain_folder            ( Folder* );

    void                        add_job_chain               ( Job_chain* );
    void                        remove_job_chain            ( Job_chain* );
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
    string                      make_where_expression_for_distributed_orders_at_order_queue( Order_queue* );
    bool                        is_order_queue_requesting_order_then_calculate_next( Order_queue* );
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

//-----------------------------------------------------------------FOR_EACH_DISTRIBUTED_ORDER_QUEUE

#define FOR_EACH_DISTRIBUTED_ORDER_QUEUE( ORDER_QUEUE )                                             \
    FOR_EACH_JOB_CHAIN( job_chain )                                                                 \
        if( job_chain->is_distributed() )                                                           \
            Z_FOR_EACH( Job_chain::Node_list, job_chain->_node_list, it )                           \
                if( Order_queue_node* order_queue_node = Order_queue_node::try_cast( *it ) )        \
                    if( Order_queue* ORDER_QUEUE = order_queue_node->order_queue() )

//-----------------------------------------------Standing_order_subsystem::Standing_order_subsystem
    
Standing_order_subsystem::Standing_order_subsystem( Scheduler* scheduler )
:
    file_based_subsystem<Order>( scheduler, this, type_standing_order_subsystem )
{
}

//------------------------------------------------------------------Standing_order_subsystem::close
    
void Standing_order_subsystem::close()
{
    set_subsystem_state( subsys_stopped );
    file_based_subsystem<Order>::close();
}

//---------------------------------------------------Standing_order_subsystem::subsystem_initialize

bool Standing_order_subsystem::subsystem_initialize()
{
    file_based_subsystem<Order>::subsystem_initialize();
    set_subsystem_state( subsys_initialized );
    return true;
}

//---------------------------------------------------------Standing_order_subsystem::subsystem_load

bool Standing_order_subsystem::subsystem_load()
{
    file_based_subsystem<Order>::subsystem_load();
    set_subsystem_state( subsys_loaded );
    return true;
}

//-----------------------------------------------------Standing_order_subsystem::subsystem_activate

bool Standing_order_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    file_based_subsystem<Order>::subsystem_activate();
    return true;
}

//---------------------------------------------------------Standing_order_subsystem::new_file_based

ptr<Order> Standing_order_subsystem::new_file_based()
{
    return new Order( this );
}

//--------------------------------------------------------Standing_order_subsystem::normalized_name

string Standing_order_subsystem::normalized_name( const string& name ) const
{
    string job_chain_name;
    string order_id;

    split_standing_order_name( name, &job_chain_name, &order_id );

    return spooler()->order_subsystem()->normalized_name( job_chain_name ) + 
           folder_name_separator +
           order_id;
}

//--------------------------------------------------------------Standing_order_subsystem::make_path

Path Standing_order_subsystem::make_path( const Path& job_chain_path, const string& order_id ) const
{
    return job_chain_path + folder_name_separator + order_id;
}

//--------------------------------------------------------------Standing_order_subsystem::make_path

Absolute_path Standing_order_subsystem::make_path( const Absolute_path& job_chain_path, const string& order_id ) const
{
    return Absolute_path( make_path( static_cast<const Path&>( job_chain_path ), order_id ) );
}

//------------------------------------------------Standing_order_subsystem::assert_xml_element_name

void Standing_order_subsystem::assert_xml_element_name( const xml::Element_ptr& e ) const
{ 
    if( !e.nodeName_is( "add_order" ) )  File_based_subsystem::assert_xml_element_name( e );
}

//-----------------------------------------------------Standing_order_folder::Standing_order_folder

Standing_order_folder::Standing_order_folder( Folder* folder )
:
    typed_folder<Order>( folder->spooler()->standing_order_subsystem(), folder, type_standing_order_folder )
{
}

//----------------------------------------------------Standing_order_folder::~Standing_order_folder
    
Standing_order_folder::~Standing_order_folder()
{
}

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
    S      result;
    S      text;
    time_t now = 0;

    result << obj_name();

    FOR_EACH_DISTRIBUTED_ORDER_QUEUE( order_queue )
    {
        time_t t = order_queue->next_distributed_order_check_time();
        
        if( t < time_max )
        {
            if( !text.empty() )  text << ", ";
            text << order_queue->job_chain()->path().without_slash() << ":" << order_queue->order_queue_node()->order_state() << " ";

            if( t == 0 )  text << "0";
            else
            {
                if( !now )  now = ::time(NULL);
                int64 seconds = now - t;
                if( seconds > 0 )  text << "+";
                text << seconds;    // Normalerweise negativ (noch verbleibende Zeit)
            }
            //if( t )  text << string_gmt_from_time_t( t ) << " UTC";
            //   else  text << "now";
            text << "s";
        }
    }

    if( !text.empty() )  result << "  requesting Node: " << text;
        
    return result;
}

//---------------------------------------------------------Database_order_detector::async_continue_

bool Database_order_detector::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.order", Z_FUNCTION << "  " << async_state_text() << "\n" );
    _spooler->assert_are_orders_distributed( Z_FUNCTION );


    _now     = Time::now();
    _now_utc = ::time(NULL);

    int  announced_orders_count = 0;

    S select_sql_begin;
    select_sql_begin << "select ";
    if( database_orders_read_ahead_count < INT_MAX )  select_sql_begin << " %limit(" << database_orders_read_ahead_count << ") ";
    select_sql_begin << db_text_distributed_next_time() << " as distributed_next_time, `job_chain`, `state`"
                      "  from " << db()->_orders_tablename <<
                      "  where `distributed_next_time` is not null"
                         " and `occupying_cluster_member_id` is null";

    string select_sql_end = "  order by `distributed_next_time`";

    
    bool database_can_limit_union_selects = db()->dbms_kind() == dbms_oracle  ||  
                                            db()->dbms_kind() == dbms_oracle_thin;
    // PostgresQL: Union kann nicht Selects mit einzelnen Limits verknüpfen, das Limit gilt fürs ganze Ergebnis,
    // und die einzelnen Selects können nicht geordnet werden (wodurch die Limits erst Sinn machen)

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
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
            FOR_EACH_DISTRIBUTED_ORDER_QUEUE( order_queue )
            {
                if( is_order_queue_requesting_order_then_calculate_next( order_queue ) )
                {
                    announced_orders_count +=
                    read_result_set( &ta, S() << select_sql_begin << " and " 
                                              << make_where_expression_for_distributed_orders_at_order_queue( order_queue ) 
                                              << select_sql_end );
                }
            }
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); }

    set_alarm();

    return true;
}

//---------------------------------------------Database_order_detector::make_union_select_order_sql

string Database_order_detector::make_union_select_order_sql( const string& select_sql_begin, const string& select_sql_end )
{
    S result;

    FOR_EACH_DISTRIBUTED_ORDER_QUEUE( order_queue )
    {
        if( is_order_queue_requesting_order_then_calculate_next( order_queue ) )
        {
            if( !result.empty() )  result << "  UNION  ";

            result << select_sql_begin << " and " 
                   << make_where_expression_for_distributed_orders_at_order_queue( order_queue ) 
                   << select_sql_end;
        }
    }

    return result;
}

//---------------------Database_order_detector::is_order_queue_requesting_order_then_calculate_next

bool Database_order_detector::is_order_queue_requesting_order_then_calculate_next( Order_queue* order_queue )
{
    bool result = false;

    order_queue->set_next_announced_distributed_order_time( Time::never, false );

    if( order_queue->is_distributed_order_requested( _now_utc ) )
    {
        order_queue->calculate_next_distributed_order_check_time( _now_utc );
        result = order_queue->next_announced_distributed_order_time() > _now;
    }

    return result;
}

//-------------Database_order_detector::make_where_expression_for_distributed_orders_at_order_queue

string Database_order_detector::make_where_expression_for_distributed_orders_at_order_queue( Order_queue* order_queue )
{
    S result;
                  
    result << order_queue->db_where_expression();

    Time t = order_queue->next_announced_distributed_order_time();
    assert( t );

    string before = t < Time::never? t.as_string( Time::without_ms ) 
                                   : never_database_distributed_next_time;
    result << " and " << db_text_distributed_next_time() << " < " << sql::quoted( before );

    return result;
}

//---------------------------------------------------------Database_order_detector::read_result_set

int Database_order_detector::read_result_set( Read_transaction* ta, const string& select_sql )
{
    int      count      = 0;
    Any_file result_set = ta->open_result_set( select_sql, Z_FUNCTION );

    while( !result_set.eof() )
    {
        Record     record                = result_set.get_record();
        Job_chain* job_chain             = order_subsystem()->job_chain( Absolute_path( root_path, record.as_string( "job_chain" ) ) );
        Time       distributed_next_time;

        Order_queue_node* node = Order_queue_node::cast( job_chain->node_from_state( record.as_string( "state" ) ) );

        distributed_next_time.set_datetime( record.as_string( "distributed_next_time" ) );
        if( distributed_next_time == _now_database_distributed_next_time   )  distributed_next_time.set_null();
        if( distributed_next_time >= _never_database_distributed_next_time )  distributed_next_time.set_never();
        
        bool is_now = distributed_next_time <= _now;
        node->order_queue()->set_next_announced_distributed_order_time( distributed_next_time, is_now );
        
        count += is_now;
    }

    return count;
}

//---------------------------------------------------------------Database_order_detector::set_alarm

void Database_order_detector::set_alarm()
{
    time_t next_alarm = time_max;

    FOR_EACH_DISTRIBUTED_ORDER_QUEUE( order_queue )
    {
        time_t t = order_queue->next_distributed_order_check_time();
        if( next_alarm > t )  next_alarm = t;
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

ptr<Order_subsystem> new_order_subsystem( Scheduler* scheduler )
{
    ptr<Order_subsystem_impl> order_subsystem = Z_NEW( Order_subsystem_impl( scheduler ) );
    return +order_subsystem;
}

//-----------------------------------------------------------------Order_subsystem::Order_subsystem

Order_subsystem::Order_subsystem( Scheduler* scheduler ) 
: 
    file_based_subsystem<Job_chain>( scheduler, this, Scheduler_object::type_order_subsystem )
{
}

//-------------------------------------------------------Order_subsystem_impl::Order_subsystem_impl

Order_subsystem_impl::Order_subsystem_impl( Spooler* spooler )
:
    Order_subsystem( spooler ),
    _zero_(this+1),
    _order_id_spaces(this)
{
}

//----------------------------------------------------------------------Order_subsystem_impl::close

void Order_subsystem_impl::close()
{
    Z_LOGI2( "scheduler", Z_FUNCTION << "\n" );

    set_subsystem_state( subsys_stopped );


    FOR_EACH( File_based_map, _file_based_map, it )
    {
        Job_chain* job_chain = static_cast<Job_chain*>( +it->second );
        job_chain->remove_all_pending_orders( true );
    }


    file_based_subsystem<Job_chain>::close();
}

//-------------------------------------------------------Order_subsystem_impl::subsystem_initialize

bool Order_subsystem_impl::subsystem_initialize()
{
    init_file_order_sink( _spooler );

    _subsystem_state = subsys_initialized;
    return true;
}

//-------------------------------------------------------------Order_subsystem_impl::subsystem_load

bool Order_subsystem_impl::subsystem_load()
{
    _subsystem_state = subsys_loaded;
    file_based_subsystem<Job_chain>::subsystem_load();

    return true;
}

//---------------------------------------------------------Order_subsystem_impl::subsystem_activate

bool Order_subsystem_impl::subsystem_activate()
{
    _subsystem_state = subsys_active;  // Jetzt schon aktiv für die auszuführenden Skript-Funktionen <run_time start_time_function="">

    file_based_subsystem<Job_chain>::subsystem_activate();

    if( orders_are_distributed() )
    {
        _database_order_detector = Z_NEW( Database_order_detector( _spooler ) );
        _database_order_detector->set_async_manager( _spooler->_connection_manager );
        _database_order_detector->async_wake();
    }

    return true;
}

//-------------------------------------------------------Order_subsystem_impl::new_job_chain_folder

ptr<Job_chain_folder_interface> Order_subsystem_impl::new_job_chain_folder( Folder* folder )
{
    ptr<Job_chain_folder> result = Z_NEW( Job_chain_folder( folder ) );
    return +result;
}

//-------------------------------------------------------------Order_subsystem_impl::new_file_based

ptr<Job_chain> Order_subsystem_impl::new_file_based()
{
    return new Job_chain( _spooler );
}

//------------------------------------------------Order_subsystem_impl::new_file_baseds_dom_element

xml::Element_ptr Order_subsystem_impl::new_file_baseds_dom_element( const xml::Document_ptr& doc, const Show_what& )
{ 
    xml::Element_ptr result = doc.createElement( "job_chains" );
    result.setAttribute( "count", (int64)_file_based_map.size() );
    return result;
}

//-----------------------------------------------Order_subsystem_impl::append_calendar_dom_elements

void Order_subsystem_impl::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    FOR_EACH_JOB_CHAIN( job_chain )
    {
        if( options->_count >= options->_limit )  break;

        if( !orders_are_distributed()  ||  job_chain->is_distributed() )
            job_chain->append_calendar_dom_elements( element, options );
    }


    if( options->_count < options->_limit  &&  orders_are_distributed()  
     &&  db()  &&  db()->opened()  &&  !db()->is_in_transaction() )
    {
        Read_transaction ta ( db() );

        S select_sql;
        select_sql << "select %limit(" << ( options->_limit - options->_count ) << ") " 
                   << order_select_database_columns << ", `job_chain`"
                      "  from " << db()->_orders_tablename <<
                    "  where `spooler_id`=" << sql::quoted(_spooler->id_for_db());
        if(  options->_from              )  select_sql << " and " << db_text_distributed_next_time() << " >= " << sql::quoted( options->_from  .as_string(Time::without_ms) );
        if( !options->_before.is_never() )  select_sql << " and " << db_text_distributed_next_time() << " < "  << sql::quoted( options->_before.as_string(Time::without_ms) );
        else
        if( !options->_from              )  select_sql << " and `distributed_next_time` is not null ";
        
        //select_sql << "  order by `distributed_next_time`";

        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION ); 

        while( options->_count < options->_limit  &&  !result_set.eof() )
        {
            Record record = result_set.get_record();

            try
            {
                Absolute_path job_chain_path ( root_path, record.as_string( "job_chain" ) );

                ptr<Order> order = _spooler->standing_order_subsystem()->new_order();
                order->load_record( job_chain_path, record );
                order->load_order_xml_blob( &ta );

                order->schedule_use()->append_calendar_dom_elements( element, options );
            }
            catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  " << x.what() << "\n" ); }  // Auftrag kann inzwischen gelöscht worden sein
        }
    }
}

//--------------------------------------------------------------Order_subsystem_impl::has_any_order

bool Order_subsystem_impl::has_any_order()
{ 
    FOR_EACH_JOB_CHAIN( job_chain )
    {
        if( job_chain->has_order() )  return true;
    }

    return false;
}

//----------------------------------------------------------------Order_subsystem_impl::order_count

//int Order_subsystem_impl::order_count( Read_transaction* ta ) const // JS-507 Erste Version
//{
//    int result = 0;
//
//    if( ta )
//    {
//        list<string> job_chain_names;
//        FOR_EACH_JOB_CHAIN( job_chain )
//            if( job_chain->is_distributed() )
//                job_chain_names.push_back( job_chain->path().without_slash() );
//        
//        if( !job_chain_names.empty() ) {
//            S select_sql;
//            select_sql << "select count(*)  from " << db()->_orders_tablename 
//                       << "  where " << db_where_condition() 
//                       <<    " and `job_chain` in (" << join( ",", job_chain_names ) << ")" 
//                       <<    " and `distributed_next_time` is not null";
//
//            result += ta->open_result_set( select_sql, Z_FUNCTION ).get_record().as_int( 0 );
//        }
//    }
//
//
//    FOR_EACH_JOB_CHAIN( job_chain )
//        if( !job_chain->is_distributed() )
//            result += job_chain->order_count( (Read_transaction*)NULL );
//
//    return result;
//}

//---------------------------------------------------Order_subsystem_impl::load_order_from_database

ptr<Order> Order_subsystem_impl::load_order_from_database( Transaction* outer_transaction, const Absolute_path& job_chain_path, const Order::Id& order_id, Load_order_flags flag )
{
    ptr<Order> result = try_load_order_from_database( outer_transaction, job_chain_path, order_id, flag );

    if( !result )  z::throw_xc( "SCHEDULER-162", order_id.as_string(), job_chain_path );

    return result;
}

//-----------------------------------------------Order_subsystem_impl::try_load_order_from_database

ptr<Order> Order_subsystem_impl::try_load_order_from_database( Transaction* outer_transaction, const Absolute_path& job_chain_path, const Order::Id& order_id, Load_order_flags flag )
{
    assert( !( flag & lo_lock )  ||  outer_transaction );   // lo_lock => outer_transaction

    ptr<Order> result;

    for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        S select_sql;
        select_sql <<  "select " << order_select_database_columns << ", `occupying_cluster_member_id`"
                       "  from " << db()->_orders_tablename;
        if( flag & lo_lock )  select_sql << " %update_lock";
        select_sql << "  where " << order_db_where_condition( job_chain_path, order_id.as_string() );

        if( flag & lo_blacklisted )  select_sql << " and `distributed_next_time`=" << db()->database_descriptor()->timestamp_string( blacklist_database_distributed_next_time );
                               else  select_sql << " and `distributed_next_time` is not null";

        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION );

        if( !result_set.eof() )
        {
            Record record = result_set.get_record();
            result = _spooler->standing_order_subsystem()->new_order();
            result->load_record( job_chain_path, record );
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
        if( result )  result->close(),  result = NULL;
        ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); 
    }

    return result;
}

//-----------------------------------------------------Order_subsystem_impl::orders_are_distributed

bool Order_subsystem_impl::orders_are_distributed()
{
    return _spooler->orders_are_distributed();
}

//--------------------------------------------------------------Order_subsystem_impl::request_order

void Order_subsystem_impl::request_order()
{
    if( _database_order_detector )
    {
        _database_order_detector->request_order();
    }
}

//------------------------------------------------------------Order_subsystem_impl::check_exception

void Order_subsystem_impl::check_exception()
{
    if( _database_order_detector )  _database_order_detector->async_check_exception();
}

//---------------------------------------------------Order_subsystem_impl::order_db_where_condition

string Order_subsystem_impl::order_db_where_condition( const Absolute_path& job_chain_path, const string& order_id )
{
    S result;

    result << job_chain_db_where_condition( job_chain_path ) << " and `id`="  << sql::quoted( order_id );

    return result;
}

//---------------------------------------------------------Order_subsystem_impl::db_where_condition

string Order_subsystem_impl::db_where_condition() const
{
    S result;

    result << "`spooler_id`=" << sql::quoted( _spooler->id_for_db() );

    return result;
}

//-----------------------------------------------Order_subsystem_impl::job_chain_db_where_condition

string Order_subsystem_impl::job_chain_db_where_condition( const Absolute_path& job_chain_path )
{
    S result;

    result << db_where_condition() << " and `job_chain`="  << sql::quoted( job_chain_path.without_slash() );

    return result;
}

//----------------------------------Order_subsystem_impl::distributed_job_chains_db_where_condition

string Order_subsystem_impl::distributed_job_chains_db_where_condition() const  // JS-507
{
    list<string> job_chain_names;
    FOR_EACH_JOB_CHAIN( job_chain )
        if( job_chain->is_distributed() )
        {
            S s;
            s << "'" << job_chain->path().without_slash() << "'";
            job_chain_names.push_back( s );
        }

    if( job_chain_names.empty() ) 
        return "";
    else
        return S() << db_where_condition() 
                   << " and `job_chain` in (" << join( ",", job_chain_names ) << ")"; 
}

//----------------------------------------------------------------Order_subsystem_impl::order_count

int Order_subsystem_impl::order_count( Read_transaction* ta ) const // JS-507
{
    int result = 0;

    //if( ta )
    //{
    //    string w = distributed_job_chains_db_where_condition();
    //    if( w != "" ) {
    //        S select_sql;
    //        select_sql << "select count(*)  from " << db()->_orders_tablename 
    //                   << "  where " << w 
    //                   <<    " and `distributed_next_time` is not null";

    //        result += ta->open_result_set( select_sql, Z_FUNCTION ).get_record().as_int( 0 );
    //    }
    //}


    //FOR_EACH_JOB_CHAIN( job_chain )
    //    if( !job_chain->is_distributed() )
    //        result += job_chain->order_count( (Read_transaction*)NULL );

    FOR_EACH_JOB_CHAIN( job_chain )
        result += job_chain->order_count( ta );

    return result;
}


//-----------------------------------------------------Order_subsystem_impl::processing_order_count

int Order_subsystem_impl::processing_order_count( Read_transaction* ta ) const  // JS-507
{
    int result = 0;
    if( ta ) {
        string w = distributed_job_chains_db_where_condition();
        if( w != "" ) {
            S select_sql;
            select_sql << "select count(*)" << 
                          "  from " << db()->_orders_tablename <<
                          "  where " << w <<  " and `occupying_cluster_member_id` is not null";
            result += ta->open_result_set( select_sql, Z_FUNCTION ).get_record().as_int( 0 );
        }
    }

    //FOR_EACH_JOB_CHAIN( job_chain )
    //    if( !job_chain->is_distributed() )
    //        result += job_chain->processing_order_count( (Read_transaction*)NULL );

    return result;
}


//-------------------------------------------------------Order_subsystem_impl::count_started_orders

void Order_subsystem_impl::count_started_orders()
{
    _started_orders_count++;
    //_spooler->update_console_title();
}

//------------------------------------------------------Order_subsystem_impl::count_finished_orders

void Order_subsystem_impl::count_finished_orders()
{
    _finished_orders_count++;
    _spooler->update_console_title( 2 );
}


//-----------------------------------------------------------------Order_subsystem_impl::dom_element

xml::Element_ptr Order_subsystem_impl::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what ) const // JS-507
{
    // xml::Element_ptr result = Subsystem::dom_element( dom_document, show_what );
    xml::Element_ptr result = File_based_subsystem::dom_element( dom_document, show_what );
    
    xml::Element_ptr Order_subsystem_element = dom_document.createElement( "order_subsystem" ); 

    if( show_what.is_set( show_statistics ) ) {
        
        xml::Element_ptr statistics_element = Order_subsystem_element.append_new_element( "order_subsystem.statistics" );
        xml::Element_ptr order_statistics_element = statistics_element.append_new_element( "order.statistics" );
    
        Read_transaction ta ( db() );

        //xml::Element_ptr result = dom_document.createElement( "order.statistic" );
        //result.setAttribute( "order_state", "clustered" );
        //result.setAttribute( "count", processing_order_count( &ta ) );
        order_statistics_element.appendChild(state_statistic_element (dom_document, "order_state", "clustered", processing_order_count( &ta ) ) );
        order_statistics_element.appendChild(state_statistic_element (dom_document, "order_state", "any", order_count( &ta ) ) );
    }

    result.appendChild( Order_subsystem_element );
    return result;

}

//-----------------------------------------------------------Order_subsystem_impl::state_statistic_element

xml::Element_ptr Order_subsystem_impl::state_statistic_element (const xml::Document_ptr& dom_document,  const string& attribute_name, const string& attribute_value, int count) const
{
    xml::Element_ptr result = dom_document.createElement( "order.statistic" );
    result.setAttribute( attribute_name, attribute_value );
    result.setAttribute( "count", count );
    return result;
}


//----------------------------------------------------------------ob_chain_folder::Job_chain_folder

Job_chain_folder_interface::Job_chain_folder_interface( Folder* folder )
:
    typed_folder<Job_chain>( folder->spooler()->order_subsystem(), folder, type_job_chain_folder )
{
}

//----------------------------------------------------------------ob_chain_folder::Job_chain_folder

Job_chain_folder::Job_chain_folder( Folder* folder )
:
    Job_chain_folder_interface( folder )
{
}

//------------------------------------------------------------------Job_chain_folder::add_job_chain

void Job_chain_folder::add_job_chain( Job_chain* job_chain )
{
    add_file_based( job_chain );
}

//---------------------------------------------------------------Job_chain_folder::remove_job_chain

void Job_chain_folder::remove_job_chain( Job_chain* job_chain )
{
    Z_LOGI2( "scheduler", Z_FUNCTION << "\n" );

#   ifdef Z_DEBUG
        //assert( job_chain->_order_map.empty() );
        assert( job_chain->_blacklist_map.empty() );
#   endif

    remove_file_based( job_chain );
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

//-------------------------------------------------------------------------Order_source::initialize

void Order_source::initialize()
{
    if( !_job_chain )  assert(0), z::throw_xc( Z_FUNCTION );

    if( _next_state.is_missing() )  _next_state = _job_chain->first_node()->order_state();

    Order_queue_node* next_node = Order_queue_node::try_cast( _job_chain->node_from_state( _next_state ) );
    if( !next_node )  z::throw_xc( "SCHEDULER-342", _job_chain->obj_name() );
    
    _next_order_queue = next_node->order_queue();
}

//-----------------------------------------------------------------------------Order_sources::close

void Order_sources::close()
{
    Z_LOGI2( "scheduler", Z_FUNCTION << "\n" );

    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->close();
    }
}

//------------------------------------------------------------------------Order_sources::initialize

void Order_sources::initialize()
{
    Z_LOGI2( "scheduler", Z_FUNCTION << "\n" );

    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->initialize();
    }
}

//--------------------------------------------------------------------------Order_sources::activate

void Order_sources::activate()
{
    Z_LOGI2( "scheduler", Z_FUNCTION << "\n" );

    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->activate();
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

//-------------------------------------------------------------------------------------------------

namespace job_chain {

//---------------------------------------------------------------------------------------Node::Node

Node::Node( Job_chain* job_chain, const Order::State& order_state, Type type )         
: 
    Scheduler_object( job_chain->spooler(), static_cast<spooler_com::Ijob_chain_node*>( this ), type_job_chain_node ),
    javabridge::has_proxy<Node>(job_chain->spooler()),
    _zero_(this+1), 
    _job_chain(job_chain),
    _type(type),
    _order_state(order_state)
{
    Order::check_state( order_state );
    if( _job_chain->node_from_state_or_null( order_state ) )  z::throw_xc( "SCHEDULER-150", debug_string_from_variant( order_state ), _job_chain->path().to_string() );

    set_next_state ( Variant( Variant::vt_missing ) );
    set_error_state( Variant( Variant::vt_missing ) );

    _log = job_chain->log();

    _node_index = job_chain->_node_list.size();
}

//--------------------------------------------------------------------------------------Node::close
    
void Node::close()
{
    // Zirkel auflösen:
    _job_chain  = NULL;
    _next_node  = NULL;
    _error_node = NULL;

    set_state( s_closed );
}

//---------------------------------------------------------------------------------Node::initialize

bool Node::initialize()
{
    if( _state != s_initialized )
    {
        assert( _state == s_none );
        set_state( s_initialized );
    }

    return true;
}

//-------------------------------------------------------------------------Node::action_from_string

Node::Action Node::action_from_string( const string& str )
{
    Action result;

    if( str == "process"    )  result = act_process;
    else
    if( str == "stop"       )  result = act_stop;
    else
    if( str == "next_state" )  result = act_next_state;
    else
        z::throw_xc( "SCHEDULER-391", "action", str, "process, stop, next_state" );

    return result;
}

//-------------------------------------------------------------------------Node::string_from_action

string Node::string_from_action( Action action )
{
    string result;

    switch( action )
    {
        case act_process:       result = "process";     break;
        case act_stop:          result = "stop";        break;
        case act_next_state:    result = "next_state";  break;
        default:                result = S() << "Action(" << action << ")";
    }

    return result;
}

//--------------------------------------------------------------------------Node::state_from_string

Node::State Node::state_from_string( const string& str )
{
    State result;

    if( str == "initialized" )  result = s_initialized;
    else
    if( str == "active"      )  result = s_active;
    else
        z::throw_xc( "SCHEDULER-391", "state", str, "initialized, active" );

    return result;
}

//--------------------------------------------------------------------------Node::string_from_state

string Node::string_from_state( State state )
{
    string result;

    switch( state )
    {
        case s_none:        result = "none";        break;
        case s_initialized: result = "initialized"; break;
        case s_active:      result = "active";      break;
        case s_closed:      result = "closed";      break;
        default:            result = S() << "State(" << (int)state << ")";
    }

    return result;
}

//-----------------------------------------------------------------------------------Node::activate

void Node::activate()
{
    if( _state != s_active )
    {
        assert( _state == s_initialized );
        set_state( s_active );
    }
}

//-----------------------------------------------------------------------------Node::set_next_state

void Node::set_next_state( const Order::State& next_state )
{
    if( !next_state.is_missing() )  Order::check_state( next_state );
    _next_state = normalized_state( next_state );

    // Bis initialize() bleibt nicht angegebener Zustand als VT_ERROR/is_missing() (fehlender Parameter) stehen.
    // initialize() unterscheidet dann die nicht angegebenen Zustände von VT_ERROR und setzt Defaults oder VT_EMPTY (außer <file_order_sink>)
}

//----------------------------------------------------------------------------Node::set_error_state

void Node::set_error_state( const Order::State& error_state )
{
    if( !error_state.is_missing() )  Order::check_state( error_state );
    _error_state = normalized_state( error_state );

    // Bis initialize() bleibt nicht angegebener Zustand als VT_ERROR/is_missing() (fehlender Parameter) stehen.
    // initialize() unterscheidet dann die nicht angegebenen Zustände von VT_ERROR und setzt Defaults oder VT_EMPTY (außer <file_order_sink>)
}

//---------------------------------------------------------------------------------Node::set_action

void Node::set_action( const string& action_string )
{
    Action action = action_from_string( action_string );
    
    if( _job_chain->is_distributed()  &&  action != act_process )  z::throw_xc( "SCHEDULER-404", action_string );
    
    if( _action != action )
    {
        _action = action;

        database_record_store();
        
        if( _job_chain->state() >= Job_chain::s_active )
        {
            _job_chain->check_job_chain_node( this );
        }
    }
}

//--------------------------------------------------------------------------------Node::execute_xml

xml::Element_ptr Node::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "job_chain_node.modify" ) )
    {
        string action = element.getAttribute( "action" );
        if( action != "" )  set_action( action );

        return command_processor->_answer.createElement( "ok" );
    }
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
}

//----------------------------------------------------------------------Node::database_record_store

void Node::database_record_store()
{
    if( _state == s_active  &&  db()->opened() )
    {
        if( _db_action != _action )
        {
            for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
            {
                sql::Update_stmt update ( &db()->_job_chain_nodes_table );
                
                update[ "spooler_id"        ] = _spooler->id_for_db();
                update[ "cluster_member_id" ] = _spooler->db_distributed_member_id();
                update[ "job_chain"         ] = _job_chain->path().without_slash();
                update[ "order_state"       ] = _order_state.as_string();
                update[ "action"            ] = _action == act_process? sql::Value() : action_name();

                ta.store( update, Z_FUNCTION );
                ta.commit( Z_FUNCTION );
            }
            catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_job_chain_nodes_table.name(), x ), Z_FUNCTION ); }

            _db_action = _action;
        }
    }
}

//---------------------------------------------------------------------Node::database_record_remove

//void Node::database_record_remove()
//{
//    Wird von Job_chain::database_record_remove() erledigt
//
//    if( db()->opened() )
//    {
//        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
//        {
//            sql::Delete_stmt delete_statement ( &db()->_job_chains_table );
//            
//            delete_statement.and_where_condition( "spooler_id"       , _spooler->id_for_db()         );
//            delete_statement.and_where_condition( "cluster_member_id", _spooler->db_distributed_member_id() );
//            delete_statement.and_where_condition( "job_chain"        , _job_chain->path().without_slash()        );
//            delete_statement.and_where_condition( "order_state"      , _order_state.as_string() );
//
//            ta.execute( delete_statement, Z_FUNCTION );
//            ta.commit( Z_FUNCTION );
//        }
//        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_job_chains_table.name(), x ), Z_FUNCTION ); }
//    }
//}

//-----------------------------------------------------------------------Node::database_record_load

//void Node::database_record_load( Read_transaction* ta )
//{
//    Wird von Job_chain::database_record_load() erledigt
//}

//-----------------------------------------------------------------------------------Node::obj_name

string Node::obj_name() const
{
    S result;

    result << Scheduler_object::obj_name();
    result << " ";
    if( _job_chain )  result << _job_chain->path().without_slash();
    result << ":";
    result << _state;

    return result;
}

//------------------------------------------------------------------------------------Node::set_dom

//void Node::set_dom( const xml::Element_ptr& element )
//{
//    _is_suspending_order = e.bool_getAttribute( "suspend", _is_suspending_order );
//    _delay               = e. int_getAttribute( "delay"  , _delay               );
//}

//--------------------------------------------------------------------------------Node::dom_element

xml::Element_ptr Node::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    Read_transaction ta ( spooler()->_db );

    xml::Element_ptr element;

    element = document.createElement( "job_chain_node" );

    element.setAttribute( "state", debug_string_from_variant( _order_state ) );

    if( !is_type( n_file_order_sink ) )
    {
        if( !_next_state.is_empty()  )  element.setAttribute( "next_state" , debug_string_from_variant( _next_state  ) );
        if( !_error_state.is_empty() )  element.setAttribute( "error_state", debug_string_from_variant( _error_state ) );
    }


    if( _action != act_process )  element.setAttribute( "action", string_action() );

    return element;
}

//---------------------------------------------------------------Order_queue_node::Order_queue_node

Order_queue_node::Order_queue_node( Job_chain* job_chain, const Order::State& state, Node::Type type ) 
: 
    Node( job_chain, state, type ),
    javabridge::has_proxy<Order_queue_node>(job_chain->spooler())
{
    _order_queue = new Order_queue( this );
}

//--------------------------------------------------------------------------Order_queue_node::close
    
void Order_queue_node::close()
{
    if( _order_queue )  _order_queue->close(), _order_queue = NULL;

    Base_class::close();
}

//------------------------------------------------------------------------Order_queue_node::replace

//void Order_queue_node::replace( Node* old )
//{
//    if( Order_queue_node* old_node = Order_queue_node::try_cast( old ) )
//    {
//        _order_queue = old_node->_order_queue;
//        old_node->_order_queue = NULL;
//        _order_queue->on_node_replaced( this );
//    }
//}

//---------------------------------------------------------------------Order_queue_node::set_action

void Order_queue_node::set_action( const string& action_string )
{
    Action action = action_from_string( action_string );
    
    if( _action != action )
    {
        Node::set_action( action_string );
        
        if( _job_chain->state() >= Job_chain::s_active )
        {
            switch( _action )
            {
                case act_process:
                {
                    break;
                }

                case act_next_state:
                {
                    Order::State next_state = _job_chain->referenced_node_from_state( _order_state )->order_state();

                    list<Order*> order_list;
                    
                    Z_FOR_EACH( Order_queue::Queue, _order_queue->_queue, o )
                        if( (*o)->state() == _order_state  &&  !(*o)->task() )  order_list.push_back( *o );

                    Z_FOR_EACH( list<Order*>, order_list, o )
                        (*o)->set_state1( next_state );

                    break;
                }                            

                default: ;
            }
        }
    }
}

//--------------------------------------------------Order_queue_node::is_ready_for_order_processing

bool Order_queue_node::is_ready_for_order_processing()
{
    return _action != act_stop  &&  
        (this == _job_chain->first_node() 
            ? _job_chain->is_ready_for_new_order_processing() 
            : _job_chain->is_ready_for_order_processing()
        );
}

//---------------------------------------------------------Order_queue_node::fetch_and_occupy_order

Order* Order_queue_node::fetch_and_occupy_order( const Time& now, const string& cause, Task* occupying_task )
{
    Order* result = NULL;

    if( is_ready_for_order_processing() )
    {
        result = order_queue()->fetch_and_occupy_order( now, cause, occupying_task );
    }

    return result;
}

//--------------------------------------------------------------------Order_queue_node::dom_element

xml::Element_ptr Order_queue_node::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr element = Base_class::dom_element( document, show_what );

    //{
    //    Read_transaction ta ( spooler()->_db );

    //    element.setAttribute( "orders", _order_queue->order_count( &ta ) );
    //}

    Show_what order_queue_show_what = show_what;
    if( show_what.is_set( show_job_chain_orders ) )  order_queue_show_what |= show_orders;
    element.appendChild( order_queue()->dom_element( document, show_what | show_orders ) );

    return element;
}

//-------------------------------------------------------------------------------Job_node::Job_node

Job_node::Job_node( Job_chain* job_chain, const Order::State& state, const Absolute_path& job_path ) 
: 
    Order_queue_node( job_chain, state, n_job ),
    _zero_(this+1),
    _job_path( job_path )
{
    if( job_path == "" )  assert(0), z::throw_xc( Z_FUNCTION, "no job path" );
}

//------------------------------------------------------------------------------Job_node::~Job_node
    
Job_node::~Job_node()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//----------------------------------------------------------------------------------Job_node::close
    
void Job_node::close()
{
    remove_requisite( Requisite_path( job_subsystem(), _job_path ) );
    disconnect_job();

    Base_class::close();
}

//-----------------------------------------------------------------------------Job_node::initialize

bool Job_node::initialize()
{
    bool ok = Base_class::initialize();

    if( ok )
    {
        add_requisite( Requisite_path( job_subsystem(), _job_path ) );
    }

    return ok;
}

//-------------------------------------------------------------------------------Job_node::activate
    
void Job_node::activate()
{
    if( Job* job = spooler()->job_subsystem()->job_or_null( _job_path ) )
    {
        if( job->file_based_state() >= File_based::s_loaded )
        {
            try
            {
                connect_job( job );
            }
            catch( exception& x )
            {
                log()->error( x.what() );
                // Exception für <show_state> aufheben? (Node ist fast sowas wie File_based)
            }
        }
    }

    Base_class::activate();
}

//----------------------------------------------------------------------------Job_node::connect_job

void Job_node::connect_job( Job* job )
{
    if( _state >= s_initialized )
    {
        assert( job == spooler()->job_subsystem()->job_or_null( _job_path ) );

        bool ok = job->connect_job_node( this );
        if( ok )
        {
            job->set_job_chain_priority( _node_index + 1 );   // Weiter hinten stehende Jobs werden vorrangig ausgeführt
            _job = job;

            //In connect_job_node() schon erledigt
            //if( Order* order = order_queue()->first_immediately_processable_order() )
            //{
            //    order->handle_changed_processable_state();
            //}
        }
    }
}

//-------------------------------------------------------------------------Job_node::disconnect_job

void Job_node::disconnect_job()
{
    if( _job ) 
    {
        _job->disconnect_job_node( this );
        _job = NULL;
    }
}

//--------------------------------------------------------------------Job_node::on_requisite_loaded

bool Job_node::on_requisite_loaded( File_based* file_based )
{
    assert( file_based->subsystem() == spooler()->job_subsystem() );
    assert( file_based->normalized_path() == normalized_job_path() );

    Job* job = dynamic_cast<Job*>( file_based );
    assert( job );

    connect_job( job );

    assert( _job );
    return true;
}

//-----------------------------------------------------------------------------Job_node::set_action

void Job_node::set_action( const string& action_string )
{
    Action action = action_from_string( action_string );
    
    if( _action != action )
    {
        Order_queue_node::set_action( action_string );

        if( _job_chain->state() >= Job_chain::s_active )
        {
            switch( _action )
            {
                case act_process:  
                    handle_changed_processable_state(); 
                    break;

                default: ;
            }
        }
    }
}

//-------------------------------------------------------Job_node::handle_changed_processable_state

void Order_queue_node::handle_changed_processable_state()
{
    if( Order* order = _order_queue->first_processable_order() )
    {
        order->handle_changed_processable_state();
    }
}

//----------------------------------------------------------------------------Job_node::dom_element

xml::Element_ptr Job_node::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr element = Base_class::dom_element( document, show_what );

    element.setAttribute( "job", _job_path );
    
    if( show_what.is_set( show_job_chain_jobs ) )
    {
        if( Job* job = spooler()->job_subsystem()->job_or_null( _job_path ) )
        {
            dom_append_nl( element );
            element.appendChild( job->dom_element( document, show_what, _job_chain ) );
            dom_append_nl( element );
        }
    }

    return element;
}

//--------------------------------------------------------------------Job_node::normalized_job_path

string Job_node::normalized_job_path() const
{
    return spooler()->job_subsystem()->normalized_path( _job_path );
}

//------------------------------------------------------------------------------------Job_node::job

Job* Job_node::job() const
{
    assert( _job == spooler()->job_subsystem()->job_or_null( _job_path ) );

    return _job? _job
               : spooler()->job_subsystem()->job( _job_path );      // Löst Exception aus
}

//----------------------------------------------------------------------------Job_node::job_or_null

Job* Job_node::job_or_null() const
{
    return _job;
}

//-----------------------------------------------------Nested_job_chain_node::Nested_job_chain_node

Nested_job_chain_node::Nested_job_chain_node( Job_chain* job_chain, const Order::State& state, const Absolute_path& job_chain_path ) 
: 
    Node( job_chain, state, n_job_chain ),
    _nested_job_chain_path( job_chain_path ),
    _nested_job_chain(this)
{
}

//----------------------------------------------------Nested_job_chain_node::~Nested_job_chain_node
    
Nested_job_chain_node::~Nested_job_chain_node()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << "  ERROR " << x.what() << "\n" ); }
}
//---------------------------------------------------------------------Nested_job_chain_node::close
    
void Nested_job_chain_node::close()
{
    Job_chain_set disconnected_job_chains;

    if( _job_chain  &&  _nested_job_chain )     // Soll schon von Job_chain::close() erledigt sein, damit weniger Meldungen erscheinen
    {
        disconnected_job_chains.insert( _nested_job_chain );

        _nested_job_chain->check_for_replacing_or_removing();
        _nested_job_chain = NULL;

        spooler()->order_subsystem()->order_id_spaces()->disconnect_order_id_spaces( _job_chain, disconnected_job_chains );
    }

    Base_class::close();
}

//----------------------------------------------------------------Nested_job_chain_node::initialize
    
bool Nested_job_chain_node::initialize()
{
    bool ok = Base_class::initialize();
    _job_chain->add_requisite( Requisite_path( _spooler->order_subsystem(), _nested_job_chain_path ) );

    if( ok )
    {
        if( Job_chain* nested_job_chain = order_subsystem()->job_chain_or_null( _nested_job_chain_path ) )
        {
            if( nested_job_chain == _job_chain )  z::throw_xc( "SCHEDULER-414", S() << _nested_job_chain_path << "->" << _job_chain->path() );
            if( nested_job_chain->is_distributed() )  z::throw_xc( "SCHEDULER-413" );

            Z_FOR_EACH( Job_chain::Node_list, nested_job_chain->_node_list, it )   // Nur einfache Verschachtelung ist erlaubt
            {
                if( Nested_job_chain_node::try_cast( *it ) )  z::throw_xc( "SCHEDULER-412", _job_chain->obj_name() );
                // Bei mehrfacher Verschachtelung die Order_id_spaces prüfen, insbesondere connected_job_chains() und disconnect_job_chains().
            }
        }
        else 
            ok = false;
    }

    return ok;
}

//-------------------------------------------------------------------Nested_job_chain_node::replace

//void Nested_job_chain_node::replace( Node* )
//{
//}

//--------------------------------------------Nested_job_chain_node::on_releasing_referenced_object

void Nested_job_chain_node::on_releasing_referenced_object( const reference< Nested_job_chain_node, Job_chain >& ref )
{
    if( _job_chain )
    {
        // Soll keine Warnung sein, s. eMail von Püschel 2007-11-13 15:33
        _job_chain->log()->info( message_string( "SCHEDULER-424", ref->obj_name(), obj_name() ) );  
    }
}

//---------------------------------------------------------------Nested_job_chain_node::dom_element
    
xml::Element_ptr Nested_job_chain_node::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr element = Base_class::dom_element( document, show_what );

    element.set_nodeName( "job_chain_node.job_chain" );
    element.setAttribute( "job_chain", _nested_job_chain_path );

    return element;
}

//-----------------------------------------------------------------------------Sink_node::Sink_node

Sink_node::Sink_node( Job_chain* job_chain, const Order::State& state, const Absolute_path& job_path, const string& move_to, bool remove ) 
: 
    Job_node( job_chain, state, job_path ) 
{
    _type = n_file_order_sink;

    _file_order_sink_move_to.set_directory( move_to );
    _file_order_sink_remove = remove;
}

//---------------------------------------------------------------------------Sink_node::dom_element
    
xml::Element_ptr Sink_node::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr element = Base_class::dom_element( document, show_what );

    xml::Element_ptr file_order_sink_element = document.createElement( "file_order_sink" );

    if( _file_order_sink_remove )  file_order_sink_element.setAttribute( "remove", "yes" );
    file_order_sink_element.setAttribute_optional( "move_to", _file_order_sink_move_to );

    element.appendChild( file_order_sink_element );

    return element;
}

//-------------------------------------------------------------------------------------------------

} //namespace job_chain

//-----------------------------------------------------------------------------Job_chain::Job_chain

Job_chain::Job_chain( Scheduler* scheduler )
:
    Com_job_chain( this ),
    file_based<Job_chain,Job_chain_folder_interface,Order_subsystem>( scheduler->order_subsystem(), static_cast<spooler_com::Ijob_chain*>( this ), type_job_chain ),
    javabridge::has_proxy<Job_chain>(scheduler),
    _zero_(this+1),
    _orders_are_recoverable(true),
    _visible(visible_yes)
{
}

//----------------------------------------------------------------------------Job_chain::~Job_chain

Job_chain::~Job_chain()
{
    Z_LOGI2( "zschimmer", obj_name() << "." << Z_FUNCTION << "\n" );

    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << ": " << x.what() << '\n' ); }
}

//---------------------------------------------------------------------------------Job_chain::close

void Job_chain::close()
{
    Z_LOGI2( "scheduler", obj_name() << ".close()\n" );

    
    remove_all_pending_orders( true );

    _blacklist_map.clear();
    _order_sources.close();
    set_state( s_closed );



    // VERSCHACHTELTE JOBKETTEN TRENNEN UND JOBKETTENGRUPPEN NEU BERECHNEN

    disconnect_nested_job_chains_and_rebuild_order_id_space();



    // JOBKETTENKNOTEN SCHLIEßEN

    Z_FOR_EACH( Node_list, _node_list, it )
    {
        Node* node = *it;
        node->close();
    }



    // AUFTRAGSQUELLEN SCHLIEßEN

    Z_FOR_EACH( Order_sources::Order_source_list, _order_sources._order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->close();
    }

    File_based::close();
}

//-------------------------------Job_chain::disconnect_nested_job_chains_and_rebuild_order_id_space

void Job_chain::disconnect_nested_job_chains_and_rebuild_order_id_space()
{
    Job_chain_set disconnected_job_chains;

    Z_FOR_EACH( Node_list, _node_list, it )
    {
        if( Nested_job_chain_node* node = Nested_job_chain_node::try_cast( *it ) )
        {
            if( node->_nested_job_chain )  
            {
                disconnected_job_chains.insert( node->_nested_job_chain );
                node->_nested_job_chain = NULL;      // reference<> auflösen
            }
        }
    }

    FOR_EACH_JOB_CHAIN( outer_job_chain )
        Z_FOR_EACH( Job_chain::Node_list, outer_job_chain->_node_list, it )                           
            if( Nested_job_chain_node* nested_job_chain_node = Nested_job_chain_node::try_cast( *it ) )        
            {
                if( nested_job_chain_node->nested_job_chain() == this )
                {
                    disconnected_job_chains.insert( outer_job_chain );
                    nested_job_chain_node->_nested_job_chain = NULL;
                }
            }


    if( !disconnected_job_chains.empty() )
    {
        order_subsystem()->order_id_spaces()->disconnect_order_id_spaces( this, disconnected_job_chains );
    }
}    

//--------------------------------------------------------------------Job_chain::set_order_id_space

void Job_chain::set_order_id_space( Order_id_space* order_id_space )
{
    assert( !order_id_space  ||  !_order_id_space );
    _order_id_space = order_id_space;
}

//------------------------------------------------------------------Job_chain::connected_job_chains

String_set Job_chain::connected_job_chains()
{
    String_set result;
    get_connected_job_chains( &result );
    assert( result.size() != 1 );           // 0 oder >= 2 (dann inklusive this)
    return result;
}

//--------------------------------------------------------------Job_chain::get_connected_job_chains

void Job_chain::get_connected_job_chains( String_set* result )
{
    // Eine sicherere Implementierung würde auf die Rekursion verzichten.
    // Mit sehr stark verschachtelten Jobketten (>1000?) wäre die Rekursion zu heftig.


    //result->insert( this );


    // Alle untergeordneten Jobketten aufnehmen

    Z_FOR_EACH( Node_list, _node_list, it )  
    {
        if( Nested_job_chain_node* node = Nested_job_chain_node::try_cast( *it ) )
        {
            if( node->_nested_job_chain  &&  result->find( node->_nested_job_chain->normalized_path() ) == result->end() )  
            {
                result->insert( node->_nested_job_chain->normalized_path() );
                node->_nested_job_chain->get_connected_job_chains( result );
            }
        }
    }


    // Alle übergeordneten Jobketten aufnehmen

    Z_FOR_EACH( Reference_register, _reference_register, it )
    {
        Node* node      = (*it)->referer();
        Job_chain*      job_chain = node->job_chain();

        if( result->find( job_chain->normalized_path() ) == result->end() )  
        {
            result->insert( job_chain->normalized_path() );
            job_chain->get_connected_job_chains( result );
        }
    }
}

//-------------------------------------------------------------------------------Job_chain::set_dom

void Job_chain::set_dom( const xml::Element_ptr& element )
{
    assert_is_not_initialized();
    if( !element )  return;
    if( !element.nodeName_is( "job_chain" ) )  z::throw_xc( "SCHEDULER-409", "job_chain", element.nodeName() );

    clear_source_xml();
    set_name( element.getAttribute( "name", name() ) );
    _title = element.getAttribute( "title", _title );

    /**
     * \change 2.1.2 - JS-538: neues Attribut max_orders
     */
    if( element.hasAttribute( "max_orders" ) )
        _max_orders = element.int_getAttribute("max_orders", unlimited_max_orders);
    else
        _max_orders = unlimited_max_orders;
    Z_LOG2( "scheduler", "at most " << _max_orders << " orders can run simultaneously." << "\n" );


    if( element.hasAttribute( "visible" ) )
        _visible = element.getAttribute( "visible" ) == "never"? visible_never :
                   element.bool_getAttribute( "visible" )      ? visible_yes 
                                                               : visible_no;

    _orders_are_recoverable = element.bool_getAttribute( "orders_recoverable", _orders_are_recoverable );

    if( order_subsystem()->orders_are_distributed() )
    _is_distributed         = element.bool_getAttribute( "distributed"       , _is_distributed     );

    if( _is_distributed  &&  !_orders_are_recoverable )  z::throw_xc( message_string( "SCHEDULER-380", obj_name() ) );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "file_order_source" ) )      // Wegen _is_on_blacklist und _is_virgin
        {
            ptr<Directory_file_order_source_interface> d = new_directory_file_order_source( this, e );
            _order_sources._order_source_list.push_back( +d );
        }
        else
        {
            Node*  node        = NULL;
            string state       = e.getAttribute( "state"       );
            string next_state  = e.getAttribute( "next_state"  );
            string error_state = e.getAttribute( "error_state" );

            if( e.nodeName_is( "file_order_sink" ) )
            {
                Job* job = _spooler->job_subsystem()->job( file_order_sink_job_path );
                job->set_visible();

                ptr<Node> sink_node = new Sink_node( this, state, job->path(), 
                                                     subst_env( e.getAttribute( "move_to" ) ), 
                                                     e.bool_getAttribute( "remove" ) );

                _node_list.push_back( sink_node );

                node = sink_node;
            }
            else
            if( e.nodeName_is( "job_chain_node" ) )
            {
                string job_path = e.getAttribute( "job" );

                if( state == "" )  z::throw_xc( "SCHEDULER-231", "job_chain_node", "state" );

                if( job_path != "" )
                {
                    node = add_job_node( job_path, state, next_state, error_state, e );
                }
                else
                {
                    add_end_node( state );
                }
            }
            else
            if( e.nodeName_is( "job_chain_node.job_chain" ) )
            {
                if( _is_distributed )  z::throw_xc( "SCHEDULER-413" );
                if( state == "" )  z::throw_xc( "SCHEDULER-231", "job_chain_node.job_chain", "empty state" );

                Absolute_path nested_job_chain_path ( folder_path(), e.getAttribute( "job_chain" ) );
                ptr<Nested_job_chain_node> nested_job_chain_node = new Nested_job_chain_node( this, state, nested_job_chain_path );
                nested_job_chain_node->set_next_state ( next_state  );
                nested_job_chain_node->set_error_state( error_state );

                _node_list.push_back( +nested_job_chain_node );

                node = nested_job_chain_node;
            }
            else
            if( e.nodeName_is( "job_chain_node.end" ) )
            {
                if( state == "" )  z::throw_xc( "SCHEDULER-231", "job_chain_node.end ", "state" );

                node = add_end_node( state );
            }

            if( node )
            {
                node->set_suspending_order( e.bool_getAttribute( "suspend", node->is_suspending_order() ) );
                node->set_delay           ( e. int_getAttribute( "delay"  , node->delay()               ) );
                //node->set_action( e.getAttribute( "action", node->string_action() ) );      // Hiernach _is_distributed nicht mehr setzen!
            }
        }
    }
}

//---------------------------------------------------------------------------Job_chain::execute_xml

xml::Element_ptr Job_chain::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "job_chain.modify" ) )
    {
        if( _is_distributed )  z::throw_xc( "SCHEDULER-384", obj_name(), "job_chain.modify" );

        string new_state = element.getAttribute( "state" );
        
        if( new_state == "running" )
        {
            if( _state == s_active )  set_stopped( false );
            else  z::throw_xc( "SCHEDULER-405", new_state, state_name() );
        }
        else
        if( new_state == "stopped" )
        {
            if( _state == s_active )  set_stopped( true );
            else  z::throw_xc( "SCHEDULER-405", new_state, state_name() );
        }
        else
            z::throw_xc( "SCHEDULER-391", "state", new_state, "running, stopped" );

        return command_processor->_answer.createElement( "ok" );
    }
    else
    {
        //File_based::execute_xml( command_processor, element, show_what );  <job_chain.remove>, das ist nicht vorgesehen
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
    }
}

//--------------------------------------------------------------Job_chain::is_visible_in_xml_folder

bool Job_chain::is_visible_in_xml_folder( const Show_what& show_what ) const
{
    return is_visible()  &&  show_what.is_set( show_job_chains | show_job_chain_jobs | show_job_chain_orders );
}

//----------------------------------------------------------xml::Element_ptr Job_chain::dom_element

xml::Element_ptr Job_chain::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    Read_transaction ta ( db() );

    Show_what modified_show = show_what;
    if( modified_show.is_set( show_job_chain_orders ) )  modified_show |= show_orders;


    xml::Element_ptr result = document.createElement( "job_chain" );

    fill_file_based_dom_element( result, show_what );
    result.setAttribute_optional( "title", _title );
    result.setAttribute( "orders", order_count( &ta ) );
    result.setAttribute( "running_orders", number_of_started_orders() );
    result.setAttribute( "state" , state_name() );
    if( !is_visible() ) result.setAttribute( "visible", _visible == visible_never? "never" : "no" );
    result.setAttribute( "orders_recoverable", _orders_are_recoverable? "yes" : "no" );
    if( _order_id_space )  result.setAttribute( "order_id_space", _order_id_space->path() );

    //if( _state >= s_active )
    {
        FOR_EACH( Order_sources::Order_source_list, _order_sources._order_source_list, it )
        {
            Order_source* order_source = *it;
            result.appendChild( order_source->dom_element( document, modified_show ) );
        }

        FOR_EACH( Node_list, _node_list, it )
        {
            Node* node = *it;
            result.appendChild( node->dom_element( document, modified_show ) );
        }
    }


    if( show_what._max_order_history  &&  db()->opened() )
    {
        xml::Element_ptr order_history_element = document.createElement( "order_history" );

        try
        {
            Read_transaction ta ( db() );

            Any_file sel = ta.open_result_set( S() <<
                           "select %limit(" << show_what._max_order_history << ")"
                           " `order_id`, `history_id`, `job_chain`, `start_time`, `end_time`, `title`, `state`, `state_text`"
                           " from " << db()->_order_history_tablename <<
                           " where `job_chain`=" << sql::quoted( path().without_slash() ) <<
                             " and `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                           " order by `history_id` desc",
                           Z_FUNCTION );

            while( !sel.eof() )
            {
                Record record = sel.get_record();

                ptr<Order> order = _spooler->standing_order_subsystem()->new_order();
                order->set_id        ( record.as_string( "order_id"   ) );
                order->set_state     ( record.as_string( "state"      ) );
                order->set_state_text( record.as_string( "state_text" ) );
                order->set_title     ( record.as_string( "title"      ) );
                order->_start_time.set_datetime( record.as_string( "start_time" ) );
                order->_end_time  .set_datetime( record.as_string( "end_time"   ) );

                xml::Element_ptr order_element = order->dom_element( document, show_what );
                order_element.setAttribute_optional( "job_chain" , record.as_string( "job_chain"  ) );
                order_element.setAttribute         ( "history_id", record.as_string( "history_id" ) );

                order_history_element.appendChild( order_element );
            }
        }
        catch( exception& x )
        {
            order_history_element.appendChild( create_error_element( document, x, 0 ) );
        }

        result.appendChild( order_history_element );
    }


    if( !_blacklist_map.empty() )
    {
        xml::Element_ptr blacklist_element = document.createElement( "blacklist" );
        blacklist_element.setAttribute( "count", (int)_blacklist_map.size() );

        if( show_what.is_set( show_blacklist ) )
        {
            Z_FOR_EACH( Blacklist_map, _blacklist_map, it )
            {
                Order* order = it->second;
                blacklist_element.appendChild( order->dom_element( document, modified_show ) );
            }
        }

        result.appendChild( blacklist_element );
    }

    return result;
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
    //    Ist bereit von Order_subsystem_impl::append_calendar_dom_elements() gelesen worden
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

//--------------------------------------------------------------------------Job_chain::add_job_node

Node* Job_chain::add_job_node( const Path& job_path, const Order::State& state_, const Order::State& next_state, const Order::State& error_state,
                               const xml::Element_ptr& element )
{
    assert_is_not_initialized();
    //if( _state != s_under_construction )  z::throw_xc( "SCHEDULER-148" );
    if( job_path == "" )  z::throw_xc( Z_FUNCTION );

    Order::State state = state_;

    if( state.is_missing() )  state = job_path;//job->name();      // Parameter state nicht angegeben? Default ist der Jobname


    Absolute_path absolute_job_path ( folder_path(), Absolute_path( folder_path(), job_path ) );

    ptr<Job_node> node = new Job_node( this, state, absolute_job_path );
    node->set_next_state ( next_state );
    node->set_error_state( error_state );

    if( element )
    {
        string on_error = element.getAttribute( "on_error" );
        if( on_error == "suspend" )  node->_on_error_suspend = true;
        else
        if( on_error == "setback" )  node->_on_error_setback = true;
    }

    _node_list.push_back( +node );

    return node;
}

//--------------------------------------------------------------------------Job_chain::add_end_node

Node* Job_chain::add_end_node( const Order::State& state )
{
    assert_is_not_initialized();
    //if( _state != s_under_construction )  z::throw_xc( "SCHEDULER-148" );

    Order::check_state( state );

    ptr<Node> node = node_from_state_or_null( state );
    
    if( !node )
    {
        node = new End_node( this, state );
        node->set_next_state ( normalized_state( "" ) );
        node->set_error_state( normalized_state( "" ) );
    }

    _node_list.push_back( node );

    return node;
}


//----------------------------------------------------------------------------Job_chain::java_nodes

javaproxy::java::util::ArrayList Job_chain::java_nodes() 
{
    javaproxy::java::util::ArrayList result = javaproxy::java::util::ArrayList::new_instance(_node_list.size());
    Z_FOR_EACH (Node_list, _node_list, it)
        result.add((*it)->java_sister());
    return result;
}

//-------------------------------------------------------------------------Job_chain::on_initialize

bool Job_chain::on_initialize()
{
    bool ok = true;

    if( _state != s_under_construction )   ok = false;

    if( ok )
    {
        fill_holes();

        Z_FOR_EACH( Node_list, _node_list, it ) 
        {
            ok = (*it)->initialize();
            if( !ok )  break;
        }

        if( ok )
        {
            Z_FOR_EACH( Node_list, _node_list, it )  check_job_chain_node( *it );
            _order_sources.initialize();

            set_state( s_initialized );
        }
    }

    return ok;
}

//----------------------------------------------------------------------------Job_chain::fill_holes

void Job_chain::fill_holes()
{
    if( !_node_list.empty() )
    {
        Node* node = *_node_list.rbegin();
        if( !node->is_type( Node::n_end )  &&  node->next_state().is_missing() )  add_end_node( default_end_state_name );    // Endzustand fehlt? Dann hinzufügen
    }

    Z_FOR_EACH( Node_list, _node_list, it )
    {
        Node* node = *it;

        if( node->is_type( Node::n_file_order_sink ) )
        {
            // _next_state und _error_state unverändert lassen
        }
        else
        {
            Node_list::iterator next = it;  next++;

            if( node->next_state().is_missing()  &&  
                next != _node_list.end()  &&  
                ( node->is_type( Node::n_order_queue ) || node->is_type( Node::n_job_chain ) ) )
            {
                node->_next_state = (*next)->order_state();
            }

            if( node->_next_state.is_null_or_empty_string() )  node->_next_state = empty_variant;
                                                         else  node->_next_node  = node_from_state( node->next_state() );

            if( node->_error_state.is_null_or_empty_string() )  node->_error_state = empty_variant;
                                                          else  node->_error_node  = node_from_state( node->error_state() );
        }
    }
}

//------------------------------------------------------------------Job_chain::check_job_chain_node

void Job_chain::check_job_chain_node( Node* node )
{
    try
    {
        hash_set<Node*> node_set;
        Node*           n       = node;

        while( n->action() == Node::act_next_state )
        {
            node_set.insert( n );

            n = node_from_state( n->next_state() );
            if( node_set.find( n ) != node_set.end() )  z::throw_xc( "SCHEDULER-403", node->order_state() );
        }
    }
    catch( exception& x )
    {
        z::throw_xc( "SCHEDULER-406", node->order_state(), x );
    }
}

//-------------------------------------------------------------------------------Job_chain::on_load

bool Job_chain::on_load() 
{
    bool result = false;

    if( !is_to_be_removed()  && _state < s_loaded )
    {
        complete_nested_job_chains();   // Exception bei doppelter Auftragskennung in den verschachtelten Jobketten


        if( db()  &&  
            db()->opened()  &&
            orders_are_recoverable()  &&  
            !is_distributed() )
        {
            list<Order_queue_node*> node_list;
            list<string>            state_sql_list;

            Z_FOR_EACH( Node_list, _node_list, it )
            {
                if( Order_queue_node* node = Order_queue_node::try_cast( *it ) )
                {
                    if( !node->order_queue()->_is_loaded )  
                    {   
                        node_list.push_back( node );
                        state_sql_list.push_back( sql::quoted( node->order_state().as_string() ) );
                    }
                }
            }


            if( !state_sql_list.empty() )
            {
                for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
                {
                    database_record_load( &ta );

                    Any_file result_set = ta.open_result_set
                        ( 
                            S() << "select " << order_select_database_columns << ", " <<
                                                db_text_distributed_next_time() << " as distributed_next_time"
                            "  from " << db()->_orders_tablename <<
                            "  where " << db_where_condition() <<
                               " and `state` in ( " << join( ", ", state_sql_list ) << " )"
                            "  order by `ordering`",
                            Z_FUNCTION
                        );

                    int count = load_orders_from_result_set( &ta, &result_set );
                    log()->debug( message_string( "SCHEDULER-935", count ) );
                }
                catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); }
            }


            Z_FOR_EACH( list<Order_queue_node*>, node_list, it )  (*it)->order_queue()->_is_loaded = true;
        }

        set_state( s_loaded );
        result = true;
    }

    return result;
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

    ptr<Order> order = _spooler->standing_order_subsystem()->order_or_null( path(), order_id );
    
    if( order )  
    {
        Z_LOG2( "scheduler", "Auftrag aus Datenbank ändert " << order->path() << "\n" );
        assert( !order->_job_chain );
        assert( !order->_task );
    }
    else
    {
        order = _spooler->standing_order_subsystem()->new_order();
    }

    order->load_record( path(), record );
    order->load_blobs( ta );

    if( record.as_string( "distributed_next_time" ) != "" )
    {
        z::throw_xc( "SCHEDULER-389", order->obj_name() );    // Wird von load_orders_from_result_set() ignoriert (sollte vielleicht nicht)
    }

    add_order( order );

    return order;
}

//------------------------------------------------------------Job_chain::complete_nested_job_chains

void Job_chain::complete_nested_job_chains()
{
    Z_FOR_EACH( Node_list, _node_list, n )
    {
        if( Nested_job_chain_node* node = Nested_job_chain_node::try_cast( *n ) )
        {
            node->_nested_job_chain = order_subsystem()->job_chain( node->nested_job_chain_path() );
        }
    }


    FOR_EACH_JOB_CHAIN( outer_job_chain )
    {
        Z_FOR_EACH( Job_chain::Node_list, outer_job_chain->_node_list, it )                           
        {
            if( Nested_job_chain_node* nested_job_chain_node = Nested_job_chain_node::try_cast( *it ) )        
            {
                if( subsystem()->normalized_path( nested_job_chain_node->_nested_job_chain_path ) == normalized_path() )
                {
                    nested_job_chain_node->_nested_job_chain = this;
                }
            }
        }
    }

    
    String_set original_job_chain_set;
    if( order_id_space() )  original_job_chain_set = order_id_space()->_job_chain_set;

    subsystem()->order_id_spaces()->rebuild_order_id_space( this, this, original_job_chain_set );
    subsystem()->order_id_spaces()->self_check();
}

//---------------------------------------------------------------------------Job_chain::on_activate

bool Job_chain::on_activate()
{
    bool result = false;

    if( !is_to_be_removed()  &&  _state < s_active )
    {
        Z_DEBUG_ONLY( order_subsystem()->order_id_spaces()->self_check() );

        set_state( s_active );

        Z_FOR_EACH( Node_list, _node_list, it )  (*it)->activate();     // Nur eimal beim Übergang von s_stopped zu s_active aufrufen!

        _order_sources.activate();

        //    // Wird nur von Order_subsystem_impl::activate() für beim Start des Schedulers geladene Jobketten gerufen,
        //    // um nach Start des Scheduler-Skripts die <run_time next_start_function="..."> berechnen zu können.
        //    // Für später hinzugefügte Jobketten wird diese Routine nicht gerufen (sie würde auch nichts tun).
        //    Z_FOR_EACH( Order_map, _order_map, o )
        //    {
        //        Order* order = o->second;
        //        order->activate();
        //    }

        result = true;
    }

    return result;
}

//-------------------------------------------------------------------------Job_chain::node_from_job

Job_node* Job_chain::node_from_job( Job* job )
{
    string job_path = job->path();

    for( Node_list::iterator it = _node_list.begin(); it != _node_list.end(); it++ )
    {
        if( Job_node* node = Job_node::try_cast( *it ) )
        {
            if( node->job_or_null() == job )  return node;
        }
    }

    z::throw_xc( "SCHEDULER-152", job->path().to_string(), path().to_string() );
    return NULL;
}

//------------------------------------------------------------Job_chain::referenced_node_from_state

Node* Job_chain::referenced_node_from_state( const Order::State& state )
{
    Node* node   = node_from_state( state );
    Node* result = node;
    int   n      = 1000;

    while( result->action() == Node::act_next_state )
    {
        result = node_from_state( result->next_state() );
        if( --n <= 0 )  z::throw_xc( "SCHEDULER-403", node->order_state() );
    }

    return result;
}

//-----------------------------------------------------------------------Job_chain::node_from_state

Node* Job_chain::node_from_state( const Order::State& state )
{
    Node* result = node_from_state_or_null( state );
    if( !result )  z::throw_xc( "SCHEDULER-149", path().to_string(), debug_string_from_variant(state) );
    return result;
}

//---------------------------------------------------------------Job_chain::node_from_state_or_null

Node* Job_chain::node_from_state_or_null( const Order::State& order_state )
{
    if( !order_state.is_missing() )
    {
        for( Node_list::iterator it = _node_list.begin(); it != _node_list.end(); it++ )
        {
            Node* n = *it;
            if( n->order_state() == order_state )  return n;
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------Job_chain::first_node

Node* Job_chain::first_node() const
{
    if( _node_list.empty() )  assert(0), z::throw_xc( Z_FUNCTION );
    return *_node_list.begin();
}

//-----------------------------------------------------------------------------Job_chain::add_order

void Job_chain::add_order( Order* order )
{
    assert( order->_is_distributed == order->_is_db_occupied );
    assert( !order->_is_distributed || _is_distributed );


    if( _order_id_space )
    {
        if( Job_chain* in_job_chain = _order_id_space->job_chain_by_order_id_or_null( order->string_id() ) )
            z::throw_xc( "SCHEDULER-186", order->obj_name(), in_job_chain->path().to_string() );
    }
    else
    {
        if( has_order_id( (Read_transaction*)NULL, order->id() ) )  z::throw_xc( "SCHEDULER-186", order->obj_name(), path().to_string() );
    }
    

    set_visible();

    Node* node = referenced_node_from_state( order->_state );
    if( node != node_from_state( order->_state ) )  
    {
        _log->info( message_string( "SCHEDULER-859", node->order_state().as_string(), order->_state ) );
        order->set_state2( node->order_state() );
    }

    if( ( !order->_suspended || !order->_is_on_blacklist )  &&  !node->is_type( Node::n_job ) )  z::throw_xc( "SCHEDULER-149", path().to_string(), debug_string_from_variant(order->_state) );
    //if( node->_job )  assert( node->_job->order_queue() );

    order->_job_chain      = this;
    order->_job_chain_path = path();
    order->_removed_from_job_chain_path.clear();
    order->_log->set_prefix( order->obj_name() );

    //order->_job_chain_node = node;

    // <job_chain_node suspended="yes"> soll beim Laden aus der Datenbank nicht wirken: order->set_job_chain_node( node ); 

    register_order( order );

    if( order->_is_on_blacklist ) 
    {
        order->set_on_blacklist();
    }
    else
    if( Job_node* job_node = Job_node::try_cast( node ) )
    {
        order->_job_chain_node = job_node;
        job_node->order_queue()->add_order( order );
    }
    else    
        order->set_on_blacklist();
}

//--------------------------------------------------------------------------Job_chain::remove_order

void Job_chain::remove_order( Order* order )
{
    assert( subsystem()->normalized_path( order->_job_chain_path ) == normalized_path() );
    assert( order->_job_chain == this );
    assert( !order->_is_db_occupied );


    ptr<Order> hold_order = order;   // Halten

    if( order->_is_on_blacklist )
    {
        order->remove_from_blacklist();
    }

    if( order->_job_chain_node )
    {
        if( order->_is_in_order_queue )
            order->order_queue()->remove_order( order );

        order->_job_chain_node = NULL;
    }

    if( order->_is_db_occupied )  
    {
        //assert( order->_task );
        //order->db_release_occupation();
    }

    order->_job_chain      = NULL;
    order->_job_chain_path.clear();
    order->_log->set_prefix( order->obj_name() );

    unregister_order( order );

    if( order->_task )
    {
        order->_removed_from_job_chain_path = path();      // Für die Task merken, in welcher Jobkette wir waren
        order->_moved = true;
    }

    check_for_replacing_or_removing();
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
            Z_LOG2( "scheduler", Z_FUNCTION << ": " << order->obj_name() << " wird nicht entfernt, weil in Verarbeitung durch " << task << "\n" );
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Job_chain::order

ptr<Order> Job_chain::order( const Order::Id& id )
{
    ptr<Order> result = order_or_null( id );

    if( !result )  z::throw_xc( "SCHEDULER-162", debug_string_from_variant(id), path() );

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
    for( Node_list::const_iterator it = _node_list.begin(); it != _node_list.end(); it++ )
    {
        if( Order_queue_node* node = Order_queue_node::try_cast( *it ) )
            if( node->order_queue()  &&  !node->order_queue()->empty() )  return true;
    }

    return false;
}

//---------------------------------------------------------------------Job_chain::has_order_in_task

bool Job_chain::has_order_in_task() const
{
    Z_FOR_EACH_CONST( Node_list, _node_list, it )
    {
        if( Order_queue_node* order_queue_node = Order_queue_node::try_cast( *it ) )
        {
            Z_FOR_EACH_CONST( Order_queue::Queue, order_queue_node->order_queue()->_queue, it2 )
            {
                Order* order = *it2;
                if( order->_task )  return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------Job_chain::order_count

int Job_chain::order_count( Read_transaction* ta ) const
{
    int result = 0;

    if( _is_distributed && ta )
    {
        S select_sql;
        select_sql << "select count(*)  from " << db()->_orders_tablename 
                   << "  where " << db_where_condition()
                   <<    " and `distributed_next_time` " << ( _is_distributed? " is not null" 
                                                                             : " is null"     );

        result = ta->open_result_set( select_sql, Z_FUNCTION ).get_record().as_int( 0 );
    }
    else
    {
      //set<Job*> jobs;             // Jobs können (theoretisch) doppelt vorkommen, sollen aber nicht doppelt gezählt werden.

        for( Node_list::const_iterator it = _node_list.begin(); it != _node_list.end(); it++ )
        {
            if( Order_queue_node* node = Order_queue_node::try_cast( *it ) )
            {
                result += node->order_queue()->order_count( (Read_transaction*)NULL );
            }
            //Job* job = (*it)->_job;
            //if( job  &&  !set_includes( jobs, job ) )  jobs.insert( job ),  result += job->order_queue()->order_count( (Read_transaction*)NULL, this );
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
                S() << "select count(*)  from " << db()->_orders_tablename << 
                       "  where " << order_subsystem()->order_db_where_condition( path(), order_id.as_string() ),
                Z_FUNCTION
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
    if( it != _order_map.end() )  z::throw_xc( "SCHEDULER-186", order->obj_name(), path() );
    _order_map[ id_string ] = order;
}

//----------------------------------------------------------------------Job_chain::unregister_order

void Job_chain::unregister_order( Order* order )
{
    assert( !order->_is_on_blacklist );

    Order_map::iterator it = _order_map.find( order->string_id() );
    if( it != _order_map.end() )  _order_map.erase( it );
                            else  Z_LOG2( "scheduler", Z_FUNCTION << " " << order->obj_name() << " ist nicht registriert.\n" );
}

//----------------------------------------------------------------Job_chain::add_order_to_blacklist

void Job_chain::add_order_to_blacklist( Order* order )
{
    //if( order->_suspended || !node_from_state_or_null( order->_state ) || !node_from_state_or_null( order->_state )->_job )  z::throw_xc( Z_FUNCTION );

    _blacklist_map[ order->string_id() ] = order;
}

//-----------------------------------------------------------Job_chain::remove_order_from_blacklist

void Job_chain::remove_order_from_blacklist( Order* order )
{
    _blacklist_map.erase( order->string_id() );
}

//-----------------------------------------------------------------Job_chain::order_is_on_blacklist

//bool Job_chain::order_is_on_blacklist( const string& order_id )
//{
//    Blacklist_map::iterator it = _blacklist_map.find( order_id );
//    return it != _blacklist_map.end();
//}

//-------------------------------------------------------------Job_chain::blacklisted_order_or_null

Order* Job_chain::blacklisted_order_or_null( const string& order_id )
{
    Blacklist_map::iterator it = _blacklist_map.find( order_id );
    return it != _blacklist_map.end()? it->second : NULL;
}

//--------------------------------------------------------------------------------file_path_matches

bool file_path_matches( const File_path& path, const File_path& directory, const Regex& regex )
{
    bool result = false;

    if( path.directory() == File_path( directory, "" ).directory() )
    {
        if( !regex.is_compiled()  ||                        // Kein Regulärer Ausdruck angegeben
            regex.match( path.name() ) )  result = true;    // oder der angegebene passt
    }

    return result;
}

//-------------------------------------------------------Job_chain::db_get_blacklisted_order_id_set

hash_set<string> Job_chain::db_get_blacklisted_order_id_set( const File_path& directory, const Regex& regex )
{
    hash_set<string> result;

    Z_FOR_EACH( Blacklist_map, _blacklist_map, it )  
        if( file_path_matches( it->first, directory, regex ) )  result.insert( it->first );
    
    if( _is_distributed )
    {
        Read_transaction ta ( db() );
        S select_sql;
        select_sql << "select `id`  from " << db()->_orders_tablename <<
                      "  where " << db_where_condition() << 
                      "  and `distributed_next_time`=" << db()->database_descriptor()->timestamp_string( blacklist_database_distributed_next_time );

        for( Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION ); !result_set.eof(); )
        {
            Record record = result_set.get_record();
            File_path path = record.as_string( 0 );
            if( file_path_matches( path, directory, regex ) )  result.insert( path );
        }
    }

    return result;
}

//---------------------------------------------------------Job_chain::tip_for_new_distributed_order

bool Job_chain::tip_for_new_distributed_order( const Order::State& state, const Time& at )
{
    bool result = false;

    if( Order_queue_node* node = Order_queue_node::try_cast( node_from_state_or_null( state ) ) )
    {
        if( node->order_queue()->is_distributed_order_requested( time_max - 1 ) 
         && at < node->order_queue()->next_announced_distributed_order_time() )
        {
            node->order_queue()->set_next_announced_distributed_order_time( at, at.is_null() || at <= Time::now() );
            result = true;
        }

        //if( Job* job = node->_job )
        //{
        //    if( job->order_queue()->is_distributed_order_requested( time_max - 1 ) 
        //     && at < job->order_queue()->next_announced_distributed_order_time() )
        //    {
        //        job->order_queue()->set_next_announced_distributed_order_time( at, at.is_null() || at <= Time::now() );
        //        result = true;
        //    }
        //}
    }

    return result;
}

//------------------------------------------------------------------Job_chain::on_prepare_to_remove

void Job_chain::on_prepare_to_remove()
{
    My_file_based::on_prepare_to_remove();
}

//-------------------------------------------------------------------------Job_chain::on_remove_now

void Job_chain::on_remove_now()
{
    if( remove_flag() != rm_temporary )  database_record_remove();
}

//--------------------------------------------------------------------Job_chain::can_be_removed_now

bool Job_chain::can_be_removed_now()
{
    return is_to_be_removed()  && 
           !is_referenced()  &&
           !has_order_in_task();
}

//--------------------------------------------------------------------------Job_chain::remove_error

zschimmer::Xc Job_chain::remove_error()
{
    if( is_referenced() )
    {
        return zschimmer::Xc( "SCHEDULER-894", obj_name(), is_referenced_by<Nested_job_chain_node,Job_chain>::string_referenced_by() );
    }
    else
        return File_based::remove_error();
}

//--------------------------------------------------------------------Job_chain::prepare_to_replace

void Job_chain::prepare_to_replace()
{
    assert( replacement() );
}

//-------------------------------------------------------------------Job_chain::can_be_replaced_now

bool Job_chain::can_be_replaced_now()
{
    return replacement() &&
           replacement()->file_based_state() == File_based::s_initialized &&
           !has_order_in_task();
}

//------------------------------------------------------------------------Job_chain::on_replace_now

Job_chain* Job_chain::on_replace_now()
{               
    ptr<Job_chain> holder = this;

    if( !replacement()         )  assert(0), z::throw_xc( Z_FUNCTION, obj_name() );
    if( !can_be_replaced_now() )  assert(0), z::throw_xc( Z_FUNCTION, obj_name(), "!can_be_removed_now" );


    //replacement()->_order_id_space = _order_id_space;
    //_order_id_space = NULL;


    Z_FOR_EACH( Node_list, replacement()->_node_list, it )
    {
        if( Order_queue_node* new_job_chain_node = Order_queue_node::try_cast( *it ) )
        {
            if( Order_queue_node* old_job_chain_node = Order_queue_node::try_cast( node_from_state_or_null( new_job_chain_node->order_state() ) ) )
            {
                new_job_chain_node->order_queue()->_is_loaded = true;

                Order_queue::Queue queue = old_job_chain_node->order_queue()->_queue;
                Z_FOR_EACH( Order_queue::Queue, queue, it )
                {
                    ptr<Order> order = *it;
                    remove_order( order );
                    replacement()->add_order( order );
                }
            }
        }
    }

    close();

    return job_chain_folder()->replace_file_based( this );
}

//--------------------------------------------------------------------Job_chain::db_where_condition

string Job_chain::db_where_condition() const
{ 
    return order_subsystem()->job_chain_db_where_condition( path() ); 
}

//-----------------------------------------------------------------Job_chain::database_record_store

void Job_chain::database_record_store()
{
    if( file_based_state() >= File_based::s_loaded  &&      // Vorher ist database_record_load() nicht aufgerufen worden
        db()->opened() )
    {
        if( _db_stopped != _is_stopped )
        {
            for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
            {
                sql::Update_stmt update ( &db()->_job_chains_table );
                
                update[ "spooler_id"        ] = _spooler->id_for_db();
                update[ "cluster_member_id" ] = _spooler->db_distributed_member_id();
                update[ "path"              ] = path().without_slash();
                update[ "stopped"           ] = _is_stopped;

                ta.store( update, Z_FUNCTION );
                ta.commit( Z_FUNCTION );
            }
            catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_job_chains_table.name(), x ), Z_FUNCTION ); }

            _db_stopped = _is_stopped;
        }
    }
}

//----------------------------------------------------------------Job_chain::database_record_remove

void Job_chain::database_record_remove()
{
    if( db()->opened() )
    {
        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
        {
            {
                sql::Delete_stmt delete_statement ( &db()->_job_chains_table );
                
                delete_statement.and_where_condition( "spooler_id"       , _spooler->id_for_db() );
                delete_statement.and_where_condition( "cluster_member_id", _spooler->db_distributed_member_id() );
                delete_statement.and_where_condition( "path"              , path().without_slash() );

                ta.execute( delete_statement, Z_FUNCTION );
            }

            {
                sql::Delete_stmt delete_statement ( &db()->_job_chain_nodes_table );
                
                delete_statement.and_where_condition( "spooler_id"       , _spooler->id_for_db() );
                delete_statement.and_where_condition( "cluster_member_id", _spooler->db_distributed_member_id() );
                delete_statement.and_where_condition( "job_chain"        , path().without_slash() );

                ta.execute( delete_statement, Z_FUNCTION );
            }

            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_job_chains_table.name(), x ), Z_FUNCTION ); }
    }
}

//------------------------------------------------------------------Job_chain::database_record_load

void Job_chain::database_record_load( Read_transaction* ta )
{
    assert( file_based_state() == File_based::s_initialized );

    {
        Any_file result_set = ta->open_result_set
        ( 
            S() << "select `stopped`"
                << "  from " << db()->_job_chains_table.sql_name()
                << "  where `spooler_id`="        << sql::quoted( _spooler->id_for_db() )
                <<    " and `cluster_member_id`=" << sql::quoted( _spooler->db_distributed_member_id() )
                <<    " and `path`="              << sql::quoted( path().without_slash() ), 
            Z_FUNCTION 
        );
        
        if( !result_set.eof() )  
        {
            Record record  = result_set.get_record();
            _db_stopped = record.as_int( "stopped" ) != 0;
            set_stopped( _db_stopped );
        }
    }

    {
        Any_file result_set = ta->open_result_set
        ( 
            S() << "select `order_state`, `action`"
                << "  from " << db()->_job_chain_nodes_table.sql_name()
                << "  where `spooler_id`="        << sql::quoted( _spooler->id_for_db() )
                <<    " and `cluster_member_id`=" << sql::quoted( _spooler->db_distributed_member_id() )
                <<    " and `job_chain`="         << sql::quoted( path().without_slash() ), 
            Z_FUNCTION 
        );
        
        while( !result_set.eof() )  
        {
            Record record  = result_set.get_record();

            if( Node* node = node_from_state_or_null( record.as_string( "order_state" ) ) )
            {
                if( !record.null( "action" ) )  node->set_action( record.as_string( "action" ) );
                node->_db_action = node->_action;
            }
        }
    }
}

//-----------------------------------------------------------------------Job_chain::order_subsystem

Order_subsystem_impl* Job_chain::order_subsystem() const
{
    return static_cast<Order_subsystem_impl*>( _spooler->order_subsystem() );
}

//---------------------------------------------------------------------------Job_chain::set_stopped

void Job_chain::set_stopped( bool is_stopped )
{
    _is_stopped = is_stopped;

    database_record_store();

    notify_nodes();     // Fällige Aufträge fortsetzen
}

//---------------------------------------------------------Job_chain::is_ready_for_order_processing

bool Job_chain::is_ready_for_order_processing() const
{
    return
       state() == s_active  && 
       !_is_stopped;
}
//---------------------------------------------------------Job_chain::is_ready_for_new_order_processing

bool Job_chain::is_ready_for_new_order_processing() const
{
    return
       is_ready_for_order_processing() &&
       !is_max_orders_reached();
}

//-----------------------------------------------------------------Job_chain::is_max_orders_reached

bool Job_chain::is_max_orders_reached() const
{
    int count = number_of_started_orders();
    Z_LOGI2( "scheduler.order", Z_FUNCTION << "  " << count << " orders are currently running, " << _max_orders << " allowed at most." << "\n" );
    return count >= _max_orders;
}

//-----------------------------------------------------------------Job_chain::number_of_started_orders

int Job_chain::number_of_started_orders() const
{
    assert_is_not_distributed( Z_FUNCTION );

    int count = 0;
    Z_FOR_EACH_CONST( Node_list, _node_list, it )
    {
        if( Order_queue_node* node = Order_queue_node::try_cast( *it ) )
        {
            if (node == first_node())
                count += node->order_queue()->running_order_count();
            else
                count += node->order_queue()->order_count( NULL );          // nicht für verteilte Aufträge
        }
    }
    return count;
}

//-----------------------------------------------------------------Job_chain::assert_is_not_distributed

void Job_chain::assert_is_not_distributed( const string& debug_text ) const
{
    if( _is_distributed )  z::throw_xc( "SCHEDULER-376", debug_text );  // TODO eigener Fehlercode für jobchain
}

//-----------------------------------------------------------------------------Job_chain::set_state

void Job_chain::set_state( const State& state )                  
{ 
    if( _state != state )
    {
        _state = state; 

        notify_nodes();     // Fällige Aufträge fortsetzen
    }
}

//--------------------------------------------------------------------------Job_chain::notify_nodes

void Job_chain::notify_nodes()
{
    if( _state == s_active  &&  !_is_stopped ) 
    {
        Z_FOR_EACH( Node_list, _node_list, n )  (*n)->handle_changed_processable_state();
    }
}

//----------------------------------------------------------------------------Job_chain::state_name

string Job_chain::state_name()     // Brauchen wir eigentlich nicht mehr, ist durch file_based_state() abgedeckt
{
    switch( _state )
    {
        case s_under_construction:  return "under_construction";
        case s_initialized:         return "initialized";
        case s_loaded:              return "loaded";
        case s_active:              return _is_stopped? "stopped" : "running" ;   //"active";
        default:                    return S() << "State(" << _state << ")";
    }
}

//-----------------------------------------------------------------Order_id_spaces::Order_id_spaces

Order_id_spaces::Order_id_spaces( Order_subsystem_impl* order_subsystem )
:
    _zero_(this+1),
    _order_subsystem(order_subsystem)
{
    _array.push_back( NULL );    // Index 0 benutzen wir nicht
}

//-------------------------------------------------------------------------Order_id_spaces::spooler
    
Spooler* Order_id_spaces::spooler()                                      
{ 
    return _order_subsystem->spooler(); 
}

//----------------------------------------------------------Order_id_spaces::rebuild_order_id_space

void Order_id_spaces::rebuild_order_id_space( Job_chain* job_chain, Job_chain* causing_job_chain, const String_set& )
{
    String_set connected_job_chains = job_chain->connected_job_chains();

    if( connected_job_chains.empty() )
    {
        remove_job_chain_from_order_id_space( job_chain );
    }
    else
    if( job_chain->order_id_space()  &&  job_chain->order_id_space()->_job_chain_set == connected_job_chains )  
    {
        // Bestehenden Order_id_space belassen
    }
    else
    {
        ptr<Order_id_space> order_id_space       = Z_NEW( Order_id_space( _order_subsystem ) );
        int                 order_id_space_index = 0;

        Z_FOR_EACH_CONST( String_set, connected_job_chains, it )
        {
            Job_chain* connected_job_chain = _order_subsystem->job_chain( Absolute_path( *it ) );

            bool check_duplicate_order_ids = true; //Nur richtig, wenn die Jobketten aus original_job_chain_set zuerst übernommen werden. Also zwei Schleifen!  !set_includes( original_job_chain_set, connected_job_chain->normalized_path() );
            order_id_space->add_job_chain( connected_job_chain, check_duplicate_order_ids );   // check_duplicate_order_ids: Exception bei doppelter Auftragskennung
        }


        // Auftragskennungen sind eindeutig, also können die Jobketten jetzt dem Order_id_space zugeordnet werden

        Z_FOR_EACH_CONST( String_set, connected_job_chains, it )
        {
            Job_chain* connected_job_chain = _order_subsystem->job_chain( Absolute_path( *it ) );
        
            int freed = remove_job_chain_from_order_id_space( connected_job_chain );
            if( freed  &&  order_id_space_index > freed )  order_id_space_index = freed;
        }

        add_order_id_space( order_id_space, causing_job_chain, order_id_space_index );
    }
}

//------------------------------------------------------Order_id_spaces::disconnect_order_id_spaces

void Order_id_spaces::disconnect_order_id_spaces( Job_chain* causing_job_chain, const Job_chain_set& disconnected_job_chains )
{
    String_set original_job_chain_set;
    if( causing_job_chain->order_id_space() )  original_job_chain_set = causing_job_chain->order_id_space()->_job_chain_set;

    Job_chain_set job_chains = disconnected_job_chains;
    
    job_chains.insert( causing_job_chain );
    disconnect_order_id_space( causing_job_chain, causing_job_chain, original_job_chain_set, &job_chains );    // Die zuerst, dann muss vielleicht nur die Jobkette aus dem vorhandenen Order_id_space genommen werden

    while( !job_chains.empty() )
    {
        Job_chain* job_chain = *job_chains.begin();
        disconnect_order_id_space( job_chain, causing_job_chain, original_job_chain_set, &job_chains );
    }


    self_check();

    Z_FOR_EACH_CONST( Job_chain_set, disconnected_job_chains, jc )  (*jc)->check_for_replacing_or_removing(); 
}

//-------------------------------------------------------Order_id_spaces::disconnect_order_id_space

void Order_id_spaces::disconnect_order_id_space( Job_chain* job_chain, Job_chain* causing_job_chain, 
                                                 const String_set& original_job_chain_set, Job_chain_set* job_chains )
{
    rebuild_order_id_space( job_chain, causing_job_chain, original_job_chain_set );

    if( job_chain->order_id_space() )
    {
        const String_set& connected_job_chains = job_chain->order_id_space()->_job_chain_set;
    
        Z_FOR_EACH_CONST( String_set, connected_job_chains, jc )
        {
            Job_chain* connected_job_chain = _order_subsystem->job_chain( Absolute_path( *jc ) );
            job_chains->erase( connected_job_chain );
        }
    }

    job_chains->erase( job_chain );
}

//--------------------------------------------Order_id_spaces::remove_job_chain_from_order_id_space

int Order_id_spaces::remove_job_chain_from_order_id_space( Job_chain* job_chain )
{
    int result = 0;

    if( Order_id_space* order_id_space = job_chain->order_id_space() )  
    {
        order_id_space->remove_job_chain( job_chain );

        if( order_id_space->number_of_job_chains() == 1 )
        {
            if( Job_chain* remaining_job_chain = _order_subsystem->job_chain_or_null( Absolute_path( *order_id_space->_job_chain_set.begin() ) ) )
            {
                order_id_space->remove_job_chain( remaining_job_chain );
            }
        }

        if( order_id_space->number_of_job_chains() == 0 )  
        {
            result = order_id_space->index();
            remove_order_id_space( order_id_space, job_chain );
        }
    }

    return result;
}

//--------------------------------------------------------------Order_id_spaces::add_order_id_space

void Order_id_spaces::add_order_id_space( Order_id_space* order_id_space, Job_chain* causing_job_chain, int index )
{
    assert( order_id_space->size() >= 2 );
    assert( order_id_space->_index == 0 );

    //#ifdef Z_DEBUG
    //    Z_FOR_EACH( String_set, order_id_space->_job_chain_set, jc )
    //    {
    //        assert( _order_subsystem->job_chain( Absolute_path( *jc ) )->order_id_space() == order_id_space );
    //    }
    //#endif

    Z_FOR_EACH( String_set, order_id_space->_job_chain_set, jc )
    {
        Job_chain* job_chain = _order_subsystem->job_chain( Absolute_path( *jc ) );
        job_chain->set_order_id_space( order_id_space );
    }


    if( index > 0 )
    {
        assert( index < _array.size()  &&  _array[ index ] == NULL );
    }
    else
    {
        index = 1;
        for(; index < _array.size()  &&  _array[ index ]; index++ );
    }

    order_id_space->_index = index;
    if( index == _array.size() )  _array.push_back( order_id_space );
                            else  _array[ index ] = order_id_space;


    order_id_space->on_order_id_space_added( causing_job_chain );
}

//-----------------------------------------------------------Order_id_spaces::remove_order_id_space

void Order_id_spaces::remove_order_id_space( Order_id_space* order_id_space, Job_chain* causing_job_chain, Do_log do_log )
{
    assert( order_id_space->is_empty() );
    assert( _array[ order_id_space->_index ] == order_id_space );

    #ifdef Z_DEBUG
        FOR_EACH_JOB_CHAIN( job_chain )
        {
            Z_FOR_EACH( Job_chain::Node_list, job_chain->_node_list, it )                           
            {
                if( Nested_job_chain_node* nested_job_chain_node = Nested_job_chain_node::try_cast( *it ) )        
                {
                    if( Job_chain* nested_job_chain = nested_job_chain_node->nested_job_chain() )
                    {
                        assert( nested_job_chain->order_id_space() != order_id_space );
                    }
                }
            }
        }
    #endif


    order_id_space->close();

    if( do_log )  
    {
        if( causing_job_chain->state() == Job_chain::s_closed )
        {
            order_id_space->log()->info( message_string( "SCHEDULER-875", causing_job_chain->obj_name() ) );
        }
        else
        {
            order_id_space->log()->info( message_string( "SCHEDULER-874" ) );  //causing_job_chain->order_id_space()? causing_job_chain->order_id_space()->obj_name() : "-" 
        }
    }                                                                                                                

    int index = order_id_space->_index;
    order_id_space->_index = 0;
    _array[ index ] = NULL;
}

//----------------------------------------------------------------------Order_id_spaces::self_check

void Order_id_spaces::self_check()
{
    #ifdef Z_DEBUG
        hash_set<Job_chain*> combined_job_chains;


        // Prüfen, ob job_chain->order_id_space() zusammenhängender Jobketten identisch sind
        // Prüfen, ob nested_job_chain() korrekt gesetzt ist

        FOR_EACH_JOB_CHAIN( job_chain )
        {
            Z_FOR_EACH( Job_chain::Node_list, job_chain->_node_list, it )                           
            {
                if( Nested_job_chain_node* nested_job_chain_node = Nested_job_chain_node::try_cast( *it ) )        
                {
                    combined_job_chains.insert( job_chain );

                    if( Job_chain* nested_job_chain = nested_job_chain_node->nested_job_chain() )
                    {
                        combined_job_chains.insert( nested_job_chain );

                        assert( job_chain->order_id_space() );
                        assert( job_chain->order_id_space() == nested_job_chain->order_id_space() );
                        assert( job_chain->order_id_space() == _array[ job_chain->order_id_space()->index() ] );
                    }
                }
            }
        }

        FOR_EACH_JOB_CHAIN( job_chain )
        {
            if( !set_includes( combined_job_chains, job_chain ) )  assert( !job_chain->order_id_space() );
            if( job_chain->order_id_space() )  job_chain->order_id_space()->check_for_unique_order_ids_of( job_chain );
        }


        // Prüfen, ob alle Jobketten in den Order_id_space denselben job_chain->order_id_space() haben

        combined_job_chains.clear();

        for( int i = 0; i < _array.size(); i++ )  if( Order_id_space* order_id_space = _array[ i ] )
        {
            assert( i == order_id_space->index() );

            Z_FOR_EACH( String_set, order_id_space->_job_chain_set, jc )
            {
                if( Job_chain* job_chain = _order_subsystem->job_chain_or_null( Absolute_path( *jc ) ) )
                {
                    combined_job_chains.insert( job_chain );
                    assert( job_chain->order_id_space() == order_id_space );
                    order_id_space->check_for_unique_order_ids_of( job_chain );
                }
            }
        }


        // Prüfen, ob Jobketten außerhalb jedes Order_id_space kein job_chain->order_id_space() haben

        FOR_EACH_JOB_CHAIN( job_chain )
        {
            if( !set_includes( combined_job_chains, job_chain ) )  assert( !job_chain->order_id_space() );
        }

    #endif
}

//------------------------------------------------------------------------Order_id_spaces::is_empty

bool Order_id_spaces::is_empty() const
{ 
    for( int i = 1; i < _array.size(); i++ )  if( _array[ i ] )  return false;
    return true;
}

//---------------------------------------------------------------------Order_id_spaces::dom_element

xml::Element_ptr Order_id_spaces::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr order_id_spaces_element = document.createElement( "order_id_spaces" );

    if( show_what.is_set( show_job_chains | show_job_chain_jobs | show_job_chain_orders ) )
    {
        for( int i = 0; i < _array.size(); i++ )  if( Order_id_space* order_id_space = _array[ i ] )
        {
            order_id_spaces_element.appendChild( order_id_space->dom_element( document, show_what ) );
        }
    }

    return order_id_spaces_element;
}

//-------------------------------------------------------------------Order_id_space::Order_id_space

Order_id_space::Order_id_space( Order_subsystem_impl* order_subsystem )
: 
    Scheduler_object( order_subsystem->_spooler, this, type_job_chain_group ), 
    _zero_(this+1)
{
    _log->set_prefix( obj_name() );
}

//----------------------------------------------------------------------------Order_id_space::close
    
void Order_id_space::close()
{
    assert( is_empty() );   // Sollte immer vorher geleert worden sein.


    Z_FOR_EACH( String_set, _job_chain_set, it )
    {
        if( Job_chain* job_chain = spooler()->order_subsystem()->job_chain_or_null( Absolute_path( *it ) ) )
        {
            job_chain->set_order_id_space( NULL );
        }
    }

    _job_chain_set.clear();
}

//----------------------------------------------------------Order_id_space::on_order_id_space_added
    
void Order_id_space::on_order_id_space_added( Job_chain* causing_job_chain )
{
    _log->set_prefix( obj_name() );


    S job_chains_string;

    Z_FOR_EACH( String_set, _job_chain_set, it )
    {
        Job_chain* job_chain = spooler()->order_subsystem()->job_chain( Absolute_path( *it ) );
        if( job_chain != causing_job_chain )
        {
            if( !job_chains_string.empty() )  job_chains_string << ", ";
            job_chains_string << '\'' << job_chain->path() << '\'';
        }
    }

    log()->info( message_string( causing_job_chain->state() == Job_chain::s_closed? "SCHEDULER-873" 
                                                                                  : "SCHEDULER-872", 
                                 causing_job_chain->obj_name(), job_chains_string ) );
}

//----------------------------------------------------------------Order_id_space::connect_job_chain

//void Order_id_space::connect_job_chain( Job_chain* job_chain )
//{
//    add_job_chain( job_chain );
//
//    if( Order_id_space* order_id_space = job_chain->order_id_space() )
//    {
//        Z_FOR_EACH( String_set, order_id_space->_job_chain_set, it )
//        {
//            Job_chain* jc = spooler()->order_subsystem()->job_chain( Absolute_path( *it ) );
//            
//            add_job_chain( jc );
//
//            // jc->set_job_chain_node( this )   wird von Order_id_space::complete_and_add() ausgeführt.
//            // Dieser Aufruf soll noch nicht die Jobketten ändern, weil eine Exception (doppelte Auftragskennung) auftreten kann.
//        }
//    }
//}

//--------------------------------------------------------------------Order_id_space::add_job_chain

void Order_id_space::add_job_chain( Job_chain* job_chain, bool check_duplicate_order_ids )
{
    if( job_chain_is_included( job_chain ) )
    {
        assert( job_chain->order_id_space() == this );
    }
    else
    {
        log()->debug9( message_string( "SCHEDULER-708", job_chain->path(), check_duplicate_order_ids? "checking duplicate order IDs" : "" ) );

        if( check_duplicate_order_ids )
        {
            check_for_unique_order_ids_of( job_chain );
        }

        _job_chain_set.insert( job_chain->normalized_path() );
    }
}

//-----------------------------------------------------------------Order_id_space::complete_and_add

//void Order_id_space::complete_and_add( Job_chain* causing_job_chain )
//{
//    // Wenn eine verschachtelte Jobkette hinzugefügt wird, 
//    // werden die vorher getrennten Order_id_space zu einem zusammengefasst.
//    // Der neue Order_id_space soll den Namen einer alten bekommen, um ein bisschen Kontinuität zu wahren.
//
//    stdext::hash_set< ptr<Order_id_space> >  previous_order_id_spaces;
//
//    Z_FOR_EACH( String_set, _job_chain_set, it )
//    {
//        Job_chain*      job_chain               = spooler()->order_subsystem()->job_chain( Absolute_path( *it ) );
//        Order_id_space* previous_order_id_space = job_chain->order_id_space();
//
//        if( previous_order_id_space != this )
//        {
//            if( previous_order_id_space )  
//            {
//                previous_order_id_space->remove_job_chain( job_chain );
//                previous_order_id_spaces.insert( previous_order_id_space );
//            }
//
//            job_chain->set_order_id_space( this );
//        }
//    }
//    
//
//    int my_index = 0;
//
//    Z_FOR_EACH( stdext::hash_set< ptr<Order_id_space> >, previous_order_id_spaces, it )
//    {
//        int previous_index = (*it)->index();
//        order_subsystem()->order_id_spaces()->remove_order_id_space( *it, causing_job_chain, my_index? do_log : dont_log );
//        if( !my_index )  my_index = previous_index;
//    }
//
//    order_subsystem()->order_id_spaces()->add_order_id_space( this, causing_job_chain, my_index );
//}

//----------------------------------------------------Order_id_space::check_for_unique_order_ids_of

void Order_id_space::check_for_unique_order_ids_of( Job_chain* job_chain ) const
{
    Z_FOR_EACH( Job_chain::Order_map, job_chain->_order_map, it )
    {
        if( Job_chain* other_job_chain = job_chain_by_order_id_or_null( it->first ) )
        {
            if( other_job_chain != job_chain )  z::throw_xc( "SCHEDULER-426", it->first, job_chain->obj_name(), other_job_chain->obj_name() );
        }
    }
}

//----------------------------------------------------Order_id_space::job_chain_by_order_id_or_null

Job_chain* Order_id_space::job_chain_by_order_id_or_null( const string& order_id ) const
{
    Job_chain* result = NULL;

    if( Order* order = order_or_null( order_id ) )
    {
        result = order->job_chain();
    }

    return result;
}

//--------------------------------------------------------------------Order_id_space::order_or_null

ptr<Order> Order_id_space::order_or_null( const string& order_id ) const
{
    Order* result = NULL;

    Z_FOR_EACH_CONST( String_set, _job_chain_set, it )
    {
        Job_chain* job_chain = spooler()->order_subsystem()->job_chain( Absolute_path( *it ) );

        if( ptr<Order> order = job_chain->order_or_null( order_id ) )
        {
            assert( !result );
            result = order;

            #ifndef Z_DEBUG
                break;
            #endif
        }
    }

    return result;
}

//-----------------------------------------------------------------Order_id_space::remove_job_chain

void Order_id_space::remove_job_chain( Job_chain* job_chain )
{
    assert( job_chain->order_id_space() == this );

    job_chain->set_order_id_space( NULL );
    _job_chain_set.erase( job_chain->normalized_path() );
}

//----------------------------------------------------------------------Order_id_space::dom_element

xml::Element_ptr Order_id_space::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr result = document.createElement( "order_id_space" );

    result.setAttribute( "name", name() );

    Z_FOR_EACH( String_set, _job_chain_set, it )
    {
        Absolute_path job_chain_path = Absolute_path( *it );

        xml::Element_ptr job_chain_element = result.append_new_element( "job_chain" );

        if( Job_chain* job_chain = order_subsystem()->job_chain_or_null( job_chain_path ) )  job_chain_path = job_chain->path();    // Originale Großschreibung übernehmen
        job_chain_element.setAttribute( "job_chain", job_chain_path );
    }

    return result;
}

//-------------------------------------------------------------------------Order_id_space::obj_name

string Order_id_space::obj_name() const
{
    S result;

    result << "Order_id_space " << path();

    return result;
}

//-----------------------------------------------------------------------------Order_id_space::name

string Order_id_space::name() const
{
    S result;

    switch( _index )
    {
        case 0:     result << "(new)";          break;
        case 1:     result << "strawberries";   break;
        case 2:     result << "kiwis";          break;
        case 3:     result << "oranges";        break;
        case 4:     result << "raspberries";    break;
        case 5:     result << "pears";          break;
        case 6:     result << "apricots";       break;
        case 7:     result << "peaches";        break;
        default:    result << _index;
    }

    return result;
}

//-------------------------------------------------------------------------Order_queue::Order_queue

Order_queue::Order_queue( Order_queue_node* order_queue_node )
:
    Scheduler_object( order_queue_node->spooler(), static_cast<spooler_com::Iorder_queue*>( this ), type_order_queue ),
    javabridge::has_proxy<Order_queue>(order_queue_node->job_chain()->spooler()),
    _zero_(this+1),
    _order_queue_node(order_queue_node),
    _job_chain(order_queue_node->job_chain()),
    _next_announced_distributed_order_time( Time::never ),
    _next_distributed_order_check_time( time_max ),
    _next_distributed_order_check_delay( check_database_orders_period_minimum )
{
    _log->set_prefix( obj_name() );
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
    //_job = NULL;    // Falls Job gelöscht wird

    for( Queue::iterator it = _queue.begin(); it != _queue.end();  )
    {
        Order* order = *it;
        _log->info( message_string( "SCHEDULER-937", order->obj_name() ) );

        order->close();
        order->_is_in_order_queue = false;
        it = _queue.erase( it );
    }

    _job_chain        = NULL;
    _order_queue_node = NULL;

    //update_priorities();
    //_has_users_id = false;
}

//----------------------------------------------------------------------------Order_queue::obj_name

string Order_queue::obj_name() const
{
    S result;

    result << Scheduler_object::obj_name();
    if( _job_chain        )  result << " " << _job_chain->path().without_slash();
    if( _order_queue_node )  result << ":" << _order_queue_node->order_state();

    return result;
}

//-------------------------------------------------------------------------Order_queue::dom_element

xml::Element_ptr Order_queue::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    Read_transaction ta ( db() );

    xml::Element_ptr element = document.createElement( "order_queue" );

    element.setAttribute( "length"         , order_count( &ta ) );
    element.setAttribute( "next_start_time", next_time().as_string() );
    
    if( !_next_announced_distributed_order_time.is_never() )
    element.setAttribute( "next_announced_distributed_order_time", _next_announced_distributed_order_time.xml_value() );

    if( show_what.is_set( show_orders ) )
    {
        dom_append_nl( element );

        int remaining = show_what._max_orders;

        FOR_EACH( Queue, _queue, it )
        {
            Order* order = *it;
            if( remaining-- <= 0 )  break;
            element.appendChild( order->dom_element( document, show_what ) );
            dom_append_nl( element );
        }

        if( remaining > 0  &&  _job_chain->is_distributed()
         &&  db()  &&  db()->opened()  &&  !db()->is_in_transaction() )
        {
            Read_transaction ta ( db() );

            string w = db_where_expression();

            element.append_new_comment( "In database only:" );
            dom_append_nl( element );

            S select_sql;
            select_sql << "select %limit(" << remaining << ") " << order_select_database_columns << ", `job_chain`, `occupying_cluster_member_id` " << 
                          "  from " << db()->_orders_tablename <<
                         "  where " << w << 
                            " and `distributed_next_time` is not null "
                            " and ( `occupying_cluster_member_id`<>" << sql::quoted( _spooler->cluster_member_id() ) << " or"
                                  " `occupying_cluster_member_id` is null )"
                        "  order by `distributed_next_time`, `priority`, `ordering`";

            for( Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION ); 
                 remaining > 0 &&  !result_set.eof(); 
                 --remaining )
            {
                Record record = result_set.get_record();

                try
                {
                    Absolute_path job_chain_path              ( root_path, record.as_string( "job_chain" ) );
                    string        occupying_cluster_member_id = record.as_string( "occupying_cluster_member_id" );

                    ptr<Order> order = _spooler->standing_order_subsystem()->new_order();

                    order->load_record( job_chain_path, record );
                    order->load_order_xml_blob( &ta );
                    if( show_what.is_set( show_payload  ) )  order->load_payload_blob( &ta );
                    if( show_what.is_set( show_schedule ) )  order->load_run_time_blob( &ta );


                    xml::Element_ptr order_element = order->dom_element( document, show_what );

                    order_element.setAttribute_optional( "occupied_by_cluster_member_id", occupying_cluster_member_id );
                    
                    if( _spooler->_cluster )
                    order_element.setAttribute_optional( "occupied_by_http_url", _spooler->_cluster->http_url_of_member_id( occupying_cluster_member_id ) );
                    
                    element.appendChild( order_element );
                    dom_append_nl( element );
                }
                catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  " << x.what() << "\n" ); }  // Auftrag kann inzwischen gelöscht worden sein
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

//-------------------------------------------------------------------------Order_queue::running_order_count

int Order_queue::running_order_count()
{
    int result = 0;
    FOR_EACH( Queue, _queue, it )
    {
        Order* order = *it;
        if( order->is_running() ) result++;
    }
    return result;
}

//-------------------------------------------------------------------------Order_queue::order_count

int Order_queue::order_count( Read_transaction* ta ) const
{
    int result = 0;

    if( _job_chain )
    {
        if( ta  &&  _job_chain->is_distributed() )
        {
            result += ta->open_result_set
                      (
                          S() << "select count(*)  from " << db()->_orders_tablename <<
                                 "  where `distributed_next_time` is not null"
                                    " and " << db_where_expression(),
                          Z_FUNCTION
                      )
                      .get_record().as_int( 0 );

            FOR_EACH_CONST( Queue, _queue, it )
            {
                Order* order = *it;
                if( !order->_is_in_database )  result++;
            }
        }
        else
        {
            result += _queue.size();
        }
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

    if( Job_node* job_node = Job_node::try_cast( _order_queue_node ) )
        if( Job* job = job_node->job_or_null() )  job->set_visible();


    Time next_time = order->next_time();


    if( next_time )
    {
        if( !order->_suspended )
        {
            if( order->_setback < Time::never )  
            {
                if( order->_setback )  order->_log->log( do_log? log_info : log_debug3, message_string( "SCHEDULER-938", order->_setback ) );
            }
            //else  JS-474
            //    order->_log->log( do_log? log_warn : log_debug3, message_string( "SCHEDULER-296" ) );       // "Die <run_time> des Auftrags hat keine nächste Startzeit" ); JS-474
        }
    }
    else
        _log->debug( message_string( "SCHEDULER-990", order->obj_name() ) );


    Queue::iterator insert_before = _queue.begin();
    
    for(; insert_before != _queue.end(); insert_before++ )
    {
        Order* o = *insert_before;

        if( o->_suspended < order->_suspended )  continue;  // False nach vorne!
        if( o->_suspended > order->_suspended )  break;
        
        if( o->_setback < order->_setback )  continue;      // Frühere Zeit nach vorne!
        if( o->_setback > order->_setback )  break;

        if( o->_priority > order->_priority )  continue;    // Höhere Priorität nach vorne!
        if( o->_priority < order->_priority )  break;
    }

    _queue.insert( insert_before, order );
    order->_is_in_order_queue = true;

    //update_priorities();

    #ifdef Z_DEBUG
        // join( lamda(a,b) assert( a->_priority >= b->_priority ), _queue );
        if( !_queue.empty() )
            for( Queue::iterator o = _queue.begin();; )
            {
                Queue::iterator next_o = o;  next_o++;  if( next_o == _queue.end() )  break;
                assert( (*o)->_priority >= (*next_o)->_priority );
                o = next_o;
            }
    #endif

    order->handle_changed_processable_state();
}

//------------------------------------------------------------------------Order_queue::remove_order

void Order_queue::remove_order( Order* order, Do_log dolog )
{
    assert( order->_is_in_order_queue );

    if( dolog == do_log )  _log->debug9( "remove_order " + order->obj_name() );

    Queue::iterator it;
    for( it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

    if( it == _queue.end() )  z::throw_xc( "SCHEDULER-156", order->obj_name(), obj_name() );

    order->_is_in_order_queue = false;

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
    // Diese Methode weniger oft aufgerufen werden.  Z_LOGI2( "zschimmer", _job->obj_name() << " " << Z_FUNCTION << "  " << cause << "\n" );

    bool result = false;


    if( !result )  result = _has_tip_for_new_order  ||  _next_announced_distributed_order_time <= now;

    if( !result )  result = has_immediately_processable_order( now );

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
         && _job_chain  &&  _job_chain->is_distributed() )
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

    _next_distributed_order_check_time = min( now + _next_distributed_order_check_delay, _next_announced_distributed_order_time.as_utc_time_t() );
}

//-------------------------------------------Order_queue::set_next_announced_distributed_order_time

void Order_queue::set_next_announced_distributed_order_time( const Time& t, bool is_now )
{ 
    if( _next_announced_distributed_order_time != t  &&  !is_now )
    {
        Z_LOG2( "scheduler.order", obj_name() << "  " << Z_FUNCTION << "(" << t << ( is_now? ",is_now" : "" ) << ")  vorher: " << _next_announced_distributed_order_time << "\n" );
    }

    _next_announced_distributed_order_time = t; 
    
    //Z_DEBUG_ONLY( assert( is_now? t <= Time::now() : t > Time::now() ) );

    //if( is_now )
    {
        //_is_distributed_order_requested = false;
        if( Job_node* job_node = Job_node::try_cast( _order_queue_node ) )
            if( Job* job = job_node->job_or_null() )  job->signal_earlier_order( t, obj_name(), Z_FUNCTION );
    }
}

//-----------------------------------------------Order_queue::next_announced_distributed_order_time

Time Order_queue::next_announced_distributed_order_time()
{ 
    return _next_announced_distributed_order_time; 
}

////---------------------------------------------Order_queue::is_requesting_order_then_calculate_next
//
//bool Order_queue::is_requesting_order_then_calculate_next()
//{
//    bool result = false;
//
//    set_next_announced_distributed_order_time( Time::never, false );
//
//    if( is_distributed_order_requested( _now_utc ) )
//    {
//        calculate_next_distributed_order_check_time( _now_utc );
//        result = next_announced_distributed_order_time() > _now;
//    }
//
//    return result;
//}
//
//-------------------------------------------------------Order_queue::tip_for_new_distributed_order

void Order_queue::tip_for_new_distributed_order()
{
    Z_LOG2( "scheduler.order", obj_name() << "  " << Z_FUNCTION << "\n" );

    if( !_has_tip_for_new_order )
    {
        _has_tip_for_new_order = true;
        if( Job_node* job_node = Job_node::try_cast( _order_queue_node ) )
            if( Job* job = job_node->job_or_null() )  job->signal( Z_FUNCTION );
    }
}

//-------------------------------------------------------------Order_queue::first_processable_order

Order* Order_queue::first_processable_order() const
{
    // at kann 0 sein, dann werden nur Aufträge ohne Startzeit beachtet

    Order* result = NULL;

    Z_FOR_EACH_CONST( Queue, _queue, o )
    {
        Order* order = *o;

        if( order->is_processable() )
        {
            result = order;
            break;
        }
    }

    if( result )  assert( !result->_is_distributed );

    return result;
}

//-------------------------------------------------Order_queue::first_immediately_processable_order

Order* Order_queue::first_immediately_processable_order( const Time& now ) const
{
    // now kann 0 sein, dann werden nur Aufträge ohne Startzeit beachtet

    Order* result = NULL;

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

    Order* order = first_immediately_processable_order( now );
    if( order )  order->occupy_for_task( occupying_task, now );


    // Dann (alte) Aufträge aus der Datenbank
    //if( !order  &&  _next_announced_distributed_order_time <= now  &&  is_in_any_distributed_job_chain() )   // Auftrag nur lesen, wenn vorher angekündigt
    if( !order  &&  _next_announced_distributed_order_time <= now )   // Auftrag nur lesen, wenn vorher angekündigt
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

    //_next_announced_distributed_order_time = Time::never;
    _has_tip_for_new_order = false;

    if( order ) 
    {
        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
        {
            if( !order->_history_id )  order->db_insert_order_history_record( &ta );
            order->db_insert_order_step_history_record( &ta );
            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_order_history_tablename + " or " + db()->_order_step_history_tablename, x ), Z_FUNCTION ); }

        order->assert_task( Z_FUNCTION );
        order->_is_success_state = true;
        order->_end_state_reached = false;
        order->_task_error = NULL;
    }

    return order;
}

//--------------------------------Order_queue::load_and_occupy_next_distributed_order_from_database

Order* Order_queue::load_and_occupy_next_distributed_order_from_database( Task* occupying_task, const Time& now )
{
    Order* result    = NULL;
    S      select_sql;

    string w = db_where_expression();

    select_sql << "select %limit(1)  `job_chain`, " << db_text_distributed_next_time() << " as distributed_next_time, " << order_select_database_columns <<
                "  from " << db()->_orders_tablename <<  //" %update_lock"  Oracle kann nicht "for update", limit(1) und "order by" kombinieren
                "  where `distributed_next_time` <= " << db()->database_descriptor()->timestamp_string( now.as_string( Time::without_ms ) ) <<
                   " and `occupying_cluster_member_id` is null" << 
                   " and " << w <<
                "  order by `distributed_next_time`, `priority`, `ordering`";

    Record record;
    bool   record_filled = false;


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION );
        if( !result_set.eof() )
        {
            record = result_set.get_record();
            record_filled = true;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); }


    if( record_filled )
    {
        _next_announced_distributed_order_time = Time::never;

        ptr<Order> order;

        try
        {
            order = _spooler->standing_order_subsystem()->new_order();

            order->load_record( Absolute_path( root_path, record.as_string( "job_chain" ) ), record );
            order->set_distributed();
            
            Job_chain* job_chain = order_subsystem()->job_chain( order->_job_chain_path );
            assert( job_chain->is_distributed() );

            bool ok = order->db_occupy_for_processing();
            if( ok )
            {
                try
                {
                    Read_transaction ta ( db() );
                    order->load_blobs( &ta );
                }
                catch( exception& x ) 
                { 
                    Z_LOG2( "scheduler", Z_FUNCTION << " " << x.what() );
                    ok = false;      // Jemand hat wohl den Datensatz gelöscht.  
                                     // Wenn nicht, dann gibt's eine Schleife! Auftrag ungültig machen?
                }
        
                if( ok )
                {
                    order->occupy_for_task( occupying_task, now );
                    job_chain->add_order( order );

                    result = order,  order = NULL;
                }
            }
        }
        catch( exception& )
        {
            if( order )  order->close(), order = NULL;
            throw;
        }

        if( order )  order->close(), order = NULL;
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

//-------------------------------------------------Order_queue::db_where_expression

string Order_queue::db_where_expression() const
{
    S  result;
    result <<      "`spooler_id`=" << sql::quoted( _spooler->id_for_db() ) 
           << " and `job_chain`="  << sql::quoted( _job_chain->path().without_slash() ) 
           << " and `state`="      << sql::quoted( _order_queue_node->order_state().as_string() );
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace spoooler
} //namespace sos
