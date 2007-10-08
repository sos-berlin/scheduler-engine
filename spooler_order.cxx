// $Id$


#include "spooler.h"
#include "../zschimmer/z_sql.h"
#include "../zschimmer/xml.h"


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
const int    max_insert_race_retry_count                = 5;                            // Race condition beim Einfügen eines Datensatzes

//--------------------------------------------------------------------------------------------const

const string now_database_distributed_next_time         = "2000-01-01 00:00:00";        // Auftrag ist verteilt und ist sofort ausführbar
const string never_database_distributed_next_time       = "3111-11-11 00:00:00";        // Auftrag ist verteilt, hat aber keine Startzeit (weil z.B. suspendiert)
const string blacklist_database_distributed_next_time   = "3111-11-11 00:01:00";        // Auftrag ist auf der schwarzen Liste
const string replacement_database_distributed_next_time = "3111-11-11 00:02:00";        // <order replacement="yes">
const string default_end_state_name                     = "<END_STATE>";
const string order_select_database_columns              = "`id`, `priority`, `state`, `state_text`, `initial_state`, `title`, `created_time`";

//---------------------------------------------------------------------------------Job_chain_folder

struct Job_chain_folder : Job_chain_folder_interface
{
                                Job_chain_folder            ( Folder* );

    void                        add_job_chain               ( Job_chain* );
    void                        remove_job_chain            ( Job_chain* );
};

//-----------------------------------------------------------------------------------Order_id_space

struct Order_id_space : Object, Scheduler_object
{
                                Order_id_space              ( Order_subsystem* );

    void                        close                       ();
    void                        connect_job_chain           ( Job_chain* );
  //void                        disconnect_job_chain        ( Job_chain* );
    void                        check_for_unique_order_ids_of( Job_chain* ) const;
    Job_chain*                  job_chain_by_order_id_or_null( const string& order_id ) const;
    ptr<Order>                  order_or_null               ( const string& order_id ) const;
    bool                        has_order_id                ( const string& order_id ) const        { return job_chain_by_order_id_or_null( order_id ) != NULL; }
    void                        complete_and_add            ( Job_chain* causing_job_chain );
    int                         index                       () const                                { return _index; }
    string                      name                        () const;
    string                      path                        () const                                { return name(); }
    int                         size                        () const                                { return _job_chain_set.size(); }
    void                        on_order_id_space_added     ( Job_chain* causing_job_chain );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    string                      obj_name                    () const;

  private:
    friend struct               Order_id_spaces;

    void                        add_job_chain               ( Job_chain*, bool check_ids = true );
    void                        remove_job_chain            ( Job_chain* );


    Fill_zero                  _zero_;
    int                        _index;                      // this == Order_id_spaces::_array[ index ]

  public:
    String_set                 _job_chain_set;
};

//----------------------------------------------------------------------------------Order_id_spaces

struct Order_id_spaces : Order_id_spaces_interface
{
                                Order_id_spaces             ( Order_subsystem* );
                               ~Order_id_spaces             ()                                     {}

    void                        add_order_id_space          ( Order_id_space*, Job_chain* causing_job_chain, int index = 0 );
    void                        remove_order_id_space       ( Order_id_space*, Job_chain* causing_job_chain, Do_log = do_log );

    void                        recompute_order_id_spaces   ( const Job_chain_set& disconnected_job_chains, Job_chain* causing_job_chain );

    bool                        is_empty                    () const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

  private:
    Order_subsystem*               _order_subsystem;
    vector< ptr<Order_id_space> >  _array;                  // [0] unbenutzt, Lücken sind NULL
};

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

    void                        check_exception             ();

    ptr<Job_chain_folder_interface> new_job_chain_folder    ( Folder* );

    void                        request_order               ();
    ptr<Order>                  load_order_from_database    ( Transaction*, const Absolute_path& job_chain_path, const Order::Id&, Load_order_flags );
    ptr<Order>              try_load_order_from_database    ( Transaction*, const Absolute_path& job_chain_path, const Order::Id&, Load_order_flags );

    bool                        has_any_order               ();

    Job_chain*                  active_job_chain            ( const Absolute_path& path )           { return active_file_based( path ); }
    Job_chain*                  job_chain                   ( const Absolute_path& path )           { return file_based( path ); }
    Job_chain*                  job_chain_or_null           ( const Absolute_path& path )           { return file_based_or_null( path ); }
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    int                         finished_orders_count       () const                                { return _finished_orders_count; }
    Order_id_spaces_interface*  order_id_spaces_interface   ()                                      { return &_order_id_spaces; }
    Order_id_spaces*            order_id_spaces             ()                                      { return &_order_id_spaces; }



    // File_based_subsystem

    string                      object_type_name            () const                                { return "Job_chain"; }
    string                      filename_extension          () const                                { return ".job_chain.xml"; }
    string                      xml_element_name            () const                                { return "job_chain"; }
    string                      xml_elements_name           () const                                { return "job_chains"; }
    string                      normalized_name             ( const string& name ) const            { return lcase( name ); }
    ptr<Job_chain_folder_interface> new_job_chain_folder_interface();
    ptr<Job_chain>              new_file_based              ();
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr&, const Show_what& );



    // Privat

    bool                        orders_are_distributed      ();
    string                      job_chain_db_where_condition( const Absolute_path& job_chain_path );
    string                      order_db_where_condition    ( const Absolute_path& job_chain_path, const string& order_id );
    void                        count_started_orders        ();
    void                        count_finished_orders       ();


    Fill_zero                  _zero_;
    Order_id_spaces            _order_id_spaces;

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

//-----------------------------------------------------------------FOR_EACH_DISTRIBUTED_ORDER_QUEUE

#define FOR_EACH_DISTRIBUTED_ORDER_QUEUE( ORDER_QUEUE )                                             \
    FOR_EACH_JOB_CHAIN( job_chain )                                                                 \
        if( job_chain->is_distributed() )                                                           \
            Z_FOR_EACH( Job_chain::Node_list, job_chain->_node_list, it )                           \
                if( Order_queue_node* order_queue_node = Order_queue_node::try_cast( *it ) )        \
                    if( Order_queue* ORDER_QUEUE = order_queue_node->order_queue() )

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
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), Z_FUNCTION ); }

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

    result << " and `distributed_next_time` < {ts'" 
           << ( t < Time::never? t.as_string( Time::without_ms ) 
                               : never_database_distributed_next_time ) 
           << "'}";

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

ptr<Order_subsystem_interface> new_order_subsystem( Scheduler* scheduler )
{
    ptr<Order_subsystem> order_subsystem = Z_NEW( Order_subsystem( scheduler ) );
    return +order_subsystem;
}

//---------------------------------------------Order_subsystem_interface::Order_subsystem_interface

Order_subsystem_interface::Order_subsystem_interface( Scheduler* scheduler ) 
: 
    file_based_subsystem<Job_chain>( scheduler, this, Scheduler_object::type_order_subsystem )
{
}

//-----------------------------------------------------------------Order_subsystem::Order_subsystem

Order_subsystem::Order_subsystem( Spooler* spooler )
:
    Order_subsystem_interface( spooler ),
    _zero_(this+1),
    _order_id_spaces(this)
{
}

//---------------------------------------------------------------------------Order_subsystem::close

void Order_subsystem::close()
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

//------------------------------------------------------------Order_subsystem::subsystem_initialize

bool Order_subsystem::subsystem_initialize()
{
    init_file_order_sink( _spooler );

    _subsystem_state = subsys_initialized;
    return true;
}

//------------------------------------------------------------------Order_subsystem::subsystem_load

bool Order_subsystem::subsystem_load()
{
    _subsystem_state = subsys_loaded;
    file_based_subsystem<Job_chain>::subsystem_load();

    return true;
}

//--------------------------------------------------------------Order_subsystem::subsystem_activate

bool Order_subsystem::subsystem_activate()
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

//------------------------------------------------------------Order_subsystem::new_job_chain_folder

ptr<Job_chain_folder_interface> Order_subsystem::new_job_chain_folder( Folder* folder )
{
    ptr<Job_chain_folder> result = Z_NEW( Job_chain_folder( folder ) );
    return +result;
}

//------------------------------------------------------------------Order_subsystem::new_file_based

ptr<Job_chain> Order_subsystem::new_file_based()
{
    return new Job_chain( _spooler );
}

//-----------------------------------------------------Order_subsystem::new_file_baseds_dom_element

xml::Element_ptr Order_subsystem::new_file_baseds_dom_element( const xml::Document_ptr& doc, const Show_what& )
{ 
    xml::Element_ptr result = doc.createElement( "job_chains" );
    result.setAttribute( "count", (int64)_file_based_map.size() );
    return result;
}

//----------------------------------------------------Order_subsystem::append_calendar_dom_elements

void Order_subsystem::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    FOR_EACH_JOB_CHAIN( job_chain )
    {
        if( options->_count >= options->_limit )  break;

        if( !orders_are_distributed()  ||  job_chain->is_distributed() )
            job_chain->append_calendar_dom_elements( element, options );
    }


    if( options->_count < options->_limit  &&  orders_are_distributed()  
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

        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION ); 

        while( options->_count < options->_limit  &&  !result_set.eof() )
        {
            Record record = result_set.get_record();

            try
            {
                Absolute_path job_chain_path ( root_path, record.as_string( "job_chain" ) );

                ptr<Order> order = new Order( _spooler );
                order->load_record( job_chain_path, record );
                order->load_order_xml_blob( &ta );
                
                if( order->run_time() )
                    order->run_time()->append_calendar_dom_elements( element, options );
            }
            catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  " << x.what() << "\n" ); }  // Auftrag kann inzwischen gelöscht worden sein
        }
    }
}

//-------------------------------------------------------------------Order_subsystem::has_any_order

bool Order_subsystem::has_any_order()
{ 
    FOR_EACH_JOB_CHAIN( job_chain )
    {
        if( job_chain->has_order() )  return true;
    }

    return false;
}

//--------------------------------------------------------Order_subsystem::load_order_from_database

ptr<Order> Order_subsystem::load_order_from_database( Transaction* outer_transaction, const Absolute_path& job_chain_path, const Order::Id& order_id, Load_order_flags flag )
{
    ptr<Order> result = try_load_order_from_database( outer_transaction, job_chain_path, order_id, flag );

    if( !result )  z::throw_xc( "SCHEDULER-162", order_id.as_string(), job_chain_path );

    return result;
}

//----------------------------------------------------Order_subsystem::try_load_order_from_database

ptr<Order> Order_subsystem::try_load_order_from_database( Transaction* outer_transaction, const Absolute_path& job_chain_path, const Order::Id& order_id, Load_order_flags flag )
{
    ptr<Order> result;

    for( Retry_nested_transaction ta ( _spooler->_db, outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        S select_sql;
        select_sql <<  "select " << order_select_database_columns << ", `occupying_cluster_member_id`"
                       "  from " << _spooler->_orders_tablename;
        if( flag & lo_lock )  select_sql << " %update_lock";
        select_sql << "  where " << order_db_where_condition( job_chain_path, order_id.as_string() );

        if( flag & lo_blacklisted )  select_sql << " and `distributed_next_time`={ts'" << blacklist_database_distributed_next_time << "'}";
                               else  select_sql << " and `distributed_next_time` is not null";

        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION );

        if( !result_set.eof() )
        {
            Record record = result_set.get_record();
            result = new Order( _spooler );
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
        ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), Z_FUNCTION ); 
    }

    return result;
}

//----------------------------------------------------------Order_subsystem::orders_are_distributed

bool Order_subsystem::orders_are_distributed()
{
    return _spooler->orders_are_distributed();
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

string Order_subsystem::order_db_where_condition( const Absolute_path& job_chain_path, const string& order_id )
{
    S result;

    result << job_chain_db_where_condition( job_chain_path ) << " and `id`="  << sql::quoted( order_id );

    return result;
}

//----------------------------------------------------Order_subsystem::job_chain_db_where_condition

string Order_subsystem::job_chain_db_where_condition( const Absolute_path& job_chain_path )
{
    S result;

    result << "`spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
         " and `job_chain`="  << sql::quoted( job_chain_path.without_slash() );

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
    Node( job_chain, state, type )
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

//--------------------------------------------------------------------Order_queue_node::dom_element

xml::Element_ptr Order_queue_node::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr element = Base_class::dom_element( document, show_what );

    //{
    //    Read_transaction ta ( spooler()->_db );

    //    element.setAttribute( "orders", _order_queue->order_count( &ta ) );
    //}

    if( show_what.is_set( show_job_chain_orders ) )
    {
        element.appendChild( order_queue()->dom_element( document, show_what | show_orders ) );
    }

    return element;
}

//-------------------------------------------------------------------------------Job_node::Job_node

Job_node::Job_node( Job_chain* job_chain, const Order::State& state, const Absolute_path& job_path ) 
: 
    Order_queue_node( job_chain, state, n_job ),
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
    remove_dependant( job_subsystem(), _job_path );
    disconnect_job();

    Base_class::close();
}

//-----------------------------------------------------------------------------Job_node::initialize

bool Job_node::initialize()
{
    bool ok = Base_class::initialize();

    if( ok )
    {
        add_dependant( job_subsystem(), _job_path );
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
            //if( Order* order = order_queue()->first_order() )
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

//--------------------------------------------------------------------Job_node::on_dependant_loaded

bool Job_node::on_dependant_loaded( File_based* file_based )
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
                {
                    if( Order* order = order_queue()->first_order() )
                    {
                        if( Job* job = job_or_null() )  job->signal_earlier_order( order );
                    }

                    break;
                }

                default: ;
            }
        }
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
    return spooler()->job_subsystem()->normalized_name( _job_path );
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
        disconnected_job_chains.insert( _job_chain );
        disconnected_job_chains.insert( _nested_job_chain );

        _nested_job_chain->check_for_replacing_or_removing();
        _nested_job_chain = NULL;

        spooler()->order_subsystem()->order_id_spaces()->recompute_order_id_spaces( disconnected_job_chains, _job_chain );
    }

    Base_class::close();
}

//----------------------------------------------------------------Nested_job_chain_node::initialize
    
bool Nested_job_chain_node::initialize()
{
    bool ok = Base_class::initialize();
    _job_chain->add_dependant( _spooler->order_subsystem(), _nested_job_chain_path );

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
    if( _job_chain )  _job_chain->log()->warn( message_string( "SCHEDULER-424", ref->obj_name(), obj_name() ) );
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
    file_based<Job_chain,Job_chain_folder_interface,Order_subsystem_interface>( scheduler->order_subsystem(), static_cast<spooler_com::Ijob_chain*>( this ), type_job_chain ),
    _zero_(this+1),
    _orders_are_recoverable(true),
    _visible(true)
{
}

//----------------------------------------------------------------------------Job_chain::~Job_chain

Job_chain::~Job_chain()
{
    Z_LOGI2( "joacim", obj_name() << "." << Z_FUNCTION << "\n" );

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

    disconnected_nested_job_chains_and_rebuild_order_id_space();


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

//-----------------------------Job_chain::disconnected_nested_job_chains_and_rebuild_order_id_space

void Job_chain::disconnected_nested_job_chains_and_rebuild_order_id_space()
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

    if( !disconnected_job_chains.empty() )
    {
        disconnected_job_chains.insert( this );
        order_subsystem()->order_id_spaces()->recompute_order_id_spaces( disconnected_job_chains, this );
    }
}    

//----------------------------------------------------------------------------Job_chain::state_name

string Job_chain::state_name( State state )
{
    switch( state )
    {
        case s_under_construction:  return "under_construction";
        case s_initialized:         return "initialized";
        case s_loaded:              return "loaded";
        case s_active:              return "running";   //"active";
        case s_stopped:             return "stopped";
        default:                    return S() << "State(" << state << ")";
    }
}

//-------------------------------------------------------------------------------Job_chain::set_dom

void Job_chain::set_dom( const xml::Element_ptr& element )
{
    assert_is_not_initialized();
    if( !element )  return;
    if( !element.nodeName_is( "job_chain" ) )  z::throw_xc( "SCHEDULER-409", "job_chain", element.nodeName() );

    set_name(                 element.     getAttribute( "name"              , name()                  )  );
    _visible                = element.bool_getAttribute( "visible"           , _visible                );
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
                job->set_visible( true );

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
                    node = add_job_node( job_path, state, next_state, error_state );
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
        if( _is_distributed )  z::throw_xc( "SCHEDULER-384", "job_chain.modify" );

        string new_state = element.getAttribute( "state" );
        
        if( new_state == "running" )
        {
            if( _state == s_stopped  ||  _state == s_active )  set_state( s_active );
            else  z::throw_xc( "SCHEDULER-405", new_state, state_name( state() ) );
        }
        else
        if( new_state == "stopped" )
        {
            if( _state == s_stopped  ||  _state == s_active )  set_state( s_stopped );
            else  z::throw_xc( "SCHEDULER-405", new_state, state_name( state() ) );
        }
        else
            z::throw_xc( "SCHEDULER-391", "state", new_state, "running, stopped" );

        return command_processor->_answer.createElement( "ok" );
    }
    else
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
}

//--------------------------------------------------------------Job_chain::is_visible_in_xml_folder

bool Job_chain::is_visible_in_xml_folder( const Show_what& show_what ) const
{
    return _visible  &&  show_what.is_set( show_job_chains | show_job_chain_jobs | show_job_chain_orders );
}

//----------------------------------------------------------xml::Element_ptr Job_chain::dom_element

xml::Element_ptr Job_chain::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    Read_transaction ta ( _spooler->_db );

    Show_what modified_show = show_what;
    if( modified_show.is_set( show_job_chain_orders ) )  modified_show |= show_orders;


    xml::Element_ptr result = document.createElement( "job_chain" );

    fill_file_based_dom_element( result, show_what );
    //if( has_base_file() )  result.appendChild_if( File_based::dom_element( document, show_what ) );
    //if( replacement()   )  result.append_new_element( "replacement" ).appendChild( replacement()->dom_element( document, show_what ) );

    //result.setAttribute( "name"  , name() );
    result.setAttribute( "orders", order_count( &ta ) );
    result.setAttribute( "state" , state_name( state() ) );
    if( !_visible ) result.setAttribute( "visible", _visible? "yes" : "no" );
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


    if( show_what._max_order_history  &&  _spooler->_db->opened() )
    {
        xml::Element_ptr order_history_element = document.createElement( "order_history" );

        try
        {
            Read_transaction ta ( _spooler->_db );

            Any_file sel = ta.open_result_set( S() <<
                           "select %limit(" << show_what._max_order_history << ")"
                           " `order_id` as `id`, `history_id`, `job_chain`, `start_time`, `end_time`, `title`, `state`, `state_text`"
                           " from " << _spooler->_order_history_tablename <<
                           " where `job_chain`=" << sql::quoted( path().without_slash() ) <<
                             " and `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                           " order by `history_id` desc",
                           Z_FUNCTION );

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

//--------------------------------------------------------------------------Job_chain::add_job_node

Node* Job_chain::add_job_node( const Path& job_path, const Order::State& state_, const Order::State& next_state, const Order::State& error_state )
{
    assert_is_not_initialized();
    //if( _state != s_under_construction )  z::throw_xc( "SCHEDULER-148" );
    if( job_path == "" )  z::throw_xc( Z_FUNCTION );

    Order::State state = state_;

    if( state.is_missing() )  state = job_path;//job->name();      // Parameter state nicht angegeben? Default ist der Jobname


    Absolute_path absolute_job_path ( folder_path(), Absolute_path( folder_path(), job_path ) );

    ptr<Node> node = new Job_node( this, state, absolute_job_path );
    node->set_next_state ( next_state );
    node->set_error_state( error_state );

    _node_list.push_back( node );

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
        stdext::hash_set<Node*> node_set;
        Node*                   n       = node;

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

//-----------------------------------------------Job_chain::add_nested_job_chains_to_order_id_space

void Job_chain::add_nested_job_chains_to_order_id_space( Order_id_space* order_id_space )
{
    Z_FOR_EACH( Node_list, _node_list, it )
    {
        if( Nested_job_chain_node* n = Nested_job_chain_node::try_cast( *it ) )
        {
            Job_chain* nested_job_chain = order_subsystem()->job_chain( n->nested_job_chain_path() );
            order_id_space->connect_job_chain( nested_job_chain );                  // Exception bei doppelter Auftragskennung
        }
    }
}

//------------------------------------------------------------Job_chain::complete_nested_job_chains

void Job_chain::complete_nested_job_chains()
{
    ptr<Order_id_space> order_id_space = Z_NEW( Order_id_space( order_subsystem() ) );
    add_nested_job_chains_to_order_id_space( order_id_space );


    Z_FOR_EACH( Node_list, _node_list, it )
    {
        Node* node = *it;

        if( Nested_job_chain_node* n = Nested_job_chain_node::try_cast( node ) )
        {
            n->_nested_job_chain = order_subsystem()->job_chain( n->nested_job_chain_path() );
        }
    }


    if( order_id_space->size() > 0 )    // Die verschachtelten Jobketten
    {
        order_id_space->connect_job_chain( this );
        order_id_space->complete_and_add( this );

        //S job_chains_string;

        //Z_FOR_EACH( Job_chain_set, order_id_space->_job_chain_set, it )
        //{
        //    Job_chain* job_chain = *it;
        //    if( job_chain != this )
        //    {
        //        if( !job_chains_string.empty() )  job_chains_string << ", ";
        //        job_chains_string << '\'' << job_chain->path() << '\'';
        //    }
        //}

        //log()->info( message_string( "SCHEDULER-872", order_id_space->obj_name(), job_chains_string ) );
    }
}

//-------------------------------------------------------------------------Job_chain::node_from_job

Node* Job_chain::node_from_job( Job* job )
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

Node* Job_chain::first_node()
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
    

    set_visible( true );

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

//-------------------------------------------------------------------------------Job_chain::on_load

bool Job_chain::on_load()  // Read_transaction* ta )
{
    bool result = false;

    if( !is_to_be_removed()  && _state < s_loaded )
    {
        complete_nested_job_chains();   // Exception bei doppelter Auftragskennung in den verschachtelten Jobketten


        if( _spooler->db()  &&  
            _spooler->db()->opened()  &&
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
                for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
                {
                    Any_file result_set = ta.open_result_set
                        ( 
                            S() << "select " << order_select_database_columns << ", `distributed_next_time`"
                            "  from " << _spooler->_orders_tablename <<
                            "  where " << db_where_condition() <<
                               " and `state` in ( " << join( ", ", state_sql_list ) << " )"
                            "  order by `ordering`",
                            Z_FUNCTION
                        );

                    int count = load_orders_from_result_set( &ta, &result_set );
                    log()->debug( message_string( "SCHEDULER-935", count ) );
                }
                catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), Z_FUNCTION ); }
            }


            Z_FOR_EACH( list<Order_queue_node*>, node_list, it )  (*it)->order_queue()->_is_loaded = true;
        }

        set_state( s_loaded );
        result = true;
    }

    return result;
}

//---------------------------------------------------------------------------Job_chain::on_activate

bool Job_chain::on_activate()
{
    bool result = false;

    if( !is_to_be_removed()  &&  _state < s_active )
    {
        set_state( s_active );

        Z_FOR_EACH( Node_list, _node_list, it )  (*it)->activate();     // Nur eimal beim Übergang von s_stopped zu s_active aufrufen!

        _order_sources.activate();

        //    // Wird nur von Order_subsystem::activate() für beim Start des Schedulers geladene Jobketten gerufen,
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

    ptr<Order> order = new Order( _spooler );
    order->load_record( path(), record );
    order->load_blobs( ta );

    if( record.as_string( "distributed_next_time" ) != "" )
    {
        z::throw_xc( "SCHEDULER-389", order->obj_name() );    // Wird von load_orders_from_result_set() ignoriert (sollte vielleicht nicht)
    }

    add_order( order );

    return order;
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

    if( !result )  z::throw_xc( "SCHEDULER-162", debug_string_from_variant(id), name() );

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

int Job_chain::order_count( Read_transaction* ta )
{
    int result = 0;

    if( _is_distributed && ta )
    {
        S select_sql;
        select_sql << "select count(*)  from " << _spooler->_orders_tablename 
                   << "  where " << db_where_condition()
                   <<    " and `distributed_next_time` " << ( _is_distributed? " is not null" 
                                                                             : " is null"     );

        result = ta->open_result_set( select_sql, Z_FUNCTION ).get_record().as_int( 0 );
    }
    else
    {
      //set<Job*> jobs;             // Jobs können (theoretisch) doppelt vorkommen, sollen aber nicht doppelt gezählt werden.

        for( Node_list::iterator it = _node_list.begin(); it != _node_list.end(); it++ )
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
                S() << "select count(*)  from " << _spooler->_orders_tablename << 
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
    if( it != _order_map.end() )  z::throw_xc( "SCHEDULER-186", order->obj_name(), name() );
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
        if( regex.match( path.name() ) )  result = true;
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
        select_sql << "select `id`  from " << _spooler->_orders_tablename <<
                      "  where " << db_where_condition() << 
                      "  and `distributed_next_time`={ts'" << blacklist_database_distributed_next_time << "'}";

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

//---------------------------------------------------------------------Job_chain::prepare_to_remove

//void Job_chain::prepare_to_remove()
//{
//    //if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );
//  //if( !is_in_folder() )  z::throw_xc( "SCHEDULER-151" );
//
//    //_remove = true;
//
//    //if( is_referenced() )  z::throw_xc( "SCHEDULER-425", obj_name(), is_referenced_by<Nested_job_chain_node,Job_chain>::string_referenced_by() );
//    //if( !can_be_removed_now() )  return false;
//
//    My_file_based::prepare_to_remove();
//}

//--------------------------------------------------------------------Job_chain::can_be_removed_now

bool Job_chain::can_be_removed_now()
{
    return is_to_be_removed()  && 
           !is_referenced()  &&
           !has_order();
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
                Order_queue::Queue queue = old_job_chain_node->order_queue()->_queue;
                Z_FOR_EACH( Order_queue::Queue, queue, it )
                {
                    ptr<Order> order = *it;
                    remove_order( order );
                    replacement()->add_order( order );
                    //order->remove_from_job_chain( jc_leave_in_job_chain_stack );
                    //order->place_in_job_chain( this, jc_leave_in_job_chain_stack );
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

//-----------------------------------------------------------------------Job_chain::order_subsystem

Order_subsystem* Job_chain::order_subsystem() const
{
    return static_cast<Order_subsystem*>( _spooler->order_subsystem() );
}

//-----------------------------------------------------------------Order_id_spaces::Order_id_spaces

Order_id_spaces::Order_id_spaces( Order_subsystem* order_subsystem )
:
    _order_subsystem(order_subsystem)
{
    _array.push_back( NULL );    // Index 0 benutzen wir nicht
}

//-------------------------------------------------------Order_id_spaces::recompute_order_id_spaces
    
void Order_id_spaces::recompute_order_id_spaces( const Job_chain_set& disconnected_job_chains, Job_chain* causing_job_chain )
{
    // Wird nach dem Löschen einer Jobkette aufrufen.
    // disconnected_job_chains enthält die vorher verschachtelten Jobketten.

    assert( disconnected_job_chains.size() >= 2 );


    Job_chain_set   job_chains                = disconnected_job_chains;
    Order_id_space* old_common_order_id_space = ( *disconnected_job_chains.begin() ) -> order_id_space();
    bool            first                     = true;

    while( !job_chains.empty() )
    {
        Job_chain* job_chain            = *job_chains.begin();
        String_set connected_job_chains = job_chain->connected_job_chains();

        if( first  &&  connected_job_chains == old_common_order_id_space->_job_chain_set )  break;    // Nur beim ersten Schleifendurchlauf, gilt für alle Jobketten
        first = false;

        if( connected_job_chains.empty() )
        {
            job_chain->set_order_id_space( NULL );
            job_chains.erase( job_chain );
        }
        else
        {
            assert( connected_job_chains.size() >= 1 );

            ptr<Order_id_space> order_id_space       = Z_NEW( Order_id_space( _order_subsystem ) );
            int                 order_id_space_index = 0;

            if( old_common_order_id_space )
            {
                order_id_space_index = old_common_order_id_space->index();
                remove_order_id_space( old_common_order_id_space, causing_job_chain, dont_log );  // Nicht protokollieren, denn gleich wird eine neue gleichnamige angelegt
                old_common_order_id_space = NULL;
            }


            Z_FOR_EACH_CONST( String_set, connected_job_chains, it2 )
            {
                Job_chain* jc = _order_subsystem->job_chain( Absolute_path( *it2 ) );
                jc->set_order_id_space( order_id_space );
                order_id_space->add_job_chain( jc, false );
                job_chains.erase( jc );
            }

            add_order_id_space( order_id_space, causing_job_chain, order_id_space_index );
            order_id_space_index = 0;
        }
    }

    if( job_chains.empty()  &&  old_common_order_id_space )
    {
        remove_order_id_space( old_common_order_id_space, causing_job_chain );
    }




    Z_FOR_EACH_CONST( Job_chain_set, disconnected_job_chains, it )
    {
        Job_chain* job_chain = *it;

        if( job_chain != causing_job_chain )
        {
            job_chain->check_for_replacing_or_removing(); 
        }
    }
}

//--------------------------------------------------------------Order_id_spaces::add_order_id_space

void Order_id_spaces::add_order_id_space( Order_id_space* order_id_space, Job_chain* causing_job_chain, int index )
{
    assert( order_id_space->size() >= 2 );
    assert( order_id_space->_index == 0 );

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
    assert( _array[ order_id_space->_index ] == order_id_space );

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
        for( int i = 0; i < _array.size(); i++ )
        {
            if( _array[ i ] )
                order_id_spaces_element.appendChild( _array[ i ]->dom_element( document, show_what ) );
        }
    }

    return order_id_spaces_element;
}

//-------------------------------------------------------------------Order_id_space::Order_id_space

Order_id_space::Order_id_space( Order_subsystem* order_subsystem )
: 
    Scheduler_object( order_subsystem->_spooler, this, type_job_chain_group ), 
    _zero_(this+1)
{
    _log->set_prefix( obj_name() );
}

//----------------------------------------------------------------------------Order_id_space::close
    
void Order_id_space::close()
{
    Z_FOR_EACH( String_set, _job_chain_set, it )
    {
        if( Job_chain* job_chain = spooler()->order_subsystem()->job_chain_or_null( Absolute_path( *it ) ) )
        {
            job_chain->set_order_id_space( NULL );
        }
    }
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

void Order_id_space::connect_job_chain( Job_chain* job_chain )
{
    add_job_chain( job_chain );

    if( Order_id_space* order_id_space = job_chain->order_id_space() )
    {
        Z_FOR_EACH( String_set, order_id_space->_job_chain_set, it )
        {
            Job_chain* jc = spooler()->order_subsystem()->job_chain( Absolute_path( *it ) );
            add_job_chain( jc );
            // jc->set_job_chain_node( this )   wird von Order_id_space::complete_and_add() ausgeführt.
            // Dieser Aufruf soll noch nicht die Jobketten ändern, weil eine Exception (doppelte Auftragskennung) auftreten kann.
        }
    }
}

//-----------------------------------------------------------------Order_id_space::complete_and_add

void Order_id_space::complete_and_add( Job_chain* causing_job_chain )
{
    // Wenn eine verschachtelte Jobkette hinzugefügt wird, 
    // werden die vorher getrennten Order_id_space zu einem zusammengefasst.
    // Der neue Order_id_space soll den Namen einer alten bekommen, um ein bisschen Kontinuität zu wahren.

    stdext::hash_set< ptr<Order_id_space> >  previous_order_id_spaces;

    Z_FOR_EACH( String_set, _job_chain_set, it )
    {
        Job_chain*      job_chain               = spooler()->order_subsystem()->job_chain( Absolute_path( *it ) );
        Order_id_space* previous_order_id_space = job_chain->order_id_space();

        if( previous_order_id_space != this )
        {
            if( previous_order_id_space )  
            {
                previous_order_id_space->remove_job_chain( job_chain );
                previous_order_id_spaces.insert( previous_order_id_space );
            }

            job_chain->set_order_id_space( this );
        }
    }
    

    int my_index = 0;

    Z_FOR_EACH( stdext::hash_set< ptr<Order_id_space> >, previous_order_id_spaces, it )
    {
        int previous_index = (*it)->index();
        order_subsystem()->order_id_spaces()->remove_order_id_space( *it, causing_job_chain, my_index? do_log : dont_log );
        if( !my_index )  my_index = previous_index;
    }

    order_subsystem()->order_id_spaces()->add_order_id_space( this, causing_job_chain, my_index );
}

//--------------------------------------------------------------------Order_id_space::add_job_chain

void Order_id_space::add_job_chain( Job_chain* job_chain, bool check_ids )
{
    assert( _job_chain_set.find( job_chain->normalized_path() ) == _job_chain_set.end() );  //if( _job_chain_set.find( job_chain->normalized_path() ) == _job_chain_set.end() )

    if( check_ids )
    {
        check_for_unique_order_ids_of( job_chain );
    }

    _job_chain_set.insert( job_chain->normalized_path() );
}

//----------------------------------------------------Order_id_space::check_for_unique_order_ids_of

void Order_id_space::check_for_unique_order_ids_of( Job_chain* job_chain ) const
{
    Z_FOR_EACH( Job_chain::Order_map, job_chain->_order_map, it )
    {
        if( Job_chain* other_job_chain = job_chain_by_order_id_or_null( it->first ) )
            z::throw_xc( "SCHEDULER-426", it->first, job_chain->obj_name(), other_job_chain->obj_name() );
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

        result = job_chain->order_or_null( order_id );
        if( result )  break;
    }

    return result;
}

//-----------------------------------------------------------------Order_id_space::remove_job_chain

void Order_id_space::remove_job_chain( Job_chain* job_chain )
{
    Z_DEBUG_ONLY( assert( job_chain->order_id_space() == this ) );

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

//------------------------------------------------------------------Job_chain::connected_job_chains

String_set Job_chain::connected_job_chains()
{
    String_set result;
    get_connected_job_chains( &result );
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
            if( node->_nested_job_chain  &&  result->find( node->_nested_job_chain->path() ) == result->end() )  
            {
                result->insert( node->_nested_job_chain->path() );
                node->_nested_job_chain->get_connected_job_chains( result );
            }
        }
    }


    // Alle übergeordneten Jobketten aufnehmen

    Z_FOR_EACH( Reference_register, _reference_register, it )
    {
        Node* node      = (*it)->referer();
        Job_chain*      job_chain = node->job_chain();

        if( result->find( job_chain->path() ) == result->end() )  
        {
            result->insert( job_chain->path() );
            job_chain->get_connected_job_chains( result );
        }
    }
}

//-------------------------------------------------------------------------Order_queue::Order_queue

Order_queue::Order_queue( Order_queue_node* order_queue_node )
:
    Scheduler_object( order_queue_node->spooler(), static_cast<spooler_com::Iorder_queue*>( this ), type_order_queue ),
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
    Read_transaction ta ( _spooler->_db );

    xml::Element_ptr element = document.createElement( "order_queue" );

    element.setAttribute( "length"         , order_count( &ta ) );
    element.setAttribute( "next_start_time", next_time().as_string() );

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
         &&  _spooler->db()  &&  _spooler->db()->opened()  &&  !_spooler->db()->is_in_transaction() )
        {
            Read_transaction ta ( _spooler->db() );

            string w = db_where_expression();

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

            for( Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION ); 
                 remaining > 0 &&  !result_set.eof(); 
                 --remaining )
            {
                Record record = result_set.get_record();

                try
                {
                    Absolute_path job_chain_path              ( root_path, record.as_string( "job_chain" ) );
                    string        occupying_cluster_member_id = record.as_string( "occupying_cluster_member_id" );

                    ptr<Order> order = new Order( _spooler );

                    order->load_record( job_chain_path, record );
                    order->load_order_xml_blob( &ta );
                    if( show_what.is_set( show_payload  ) )  order->load_payload_blob( &ta );
                    if( show_what.is_set( show_run_time ) )  order->load_run_time_blob( &ta );


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

//-------------------------------------------------------------------------Order_queue::order_count

int Order_queue::order_count( Read_transaction* ta )
{
    int result = 0;

    if( _job_chain )
    {
        if( ta  &&  _job_chain->is_distributed() )
        {
            result += ta->open_result_set
                      (
                          S() << "select count(*)  from " << _spooler->_orders_tablename <<
                                 "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                                    " and `distributed_next_time` is not null"
                                  //" and ( `occupying_cluster_member_id`<>" << sql::quoted( _spooler->cluster_member_id() ) << " or"
                                  //      " `occupying_cluster_member_id` is null )"
                                    " and " << db_where_expression(),
                          Z_FUNCTION
                      )
                      .get_record().as_int( 0 );

            FOR_EACH( Queue, _queue, it )
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
        if( Job* job = job_node->job_or_null() )  job->set_visible( true );


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
    order->_is_in_order_queue = true;

    //update_priorities();

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
    // Diese Methode weniger oft aufgerufen werden.  Z_LOGI2( "joacim", _job->obj_name() << " " << Z_FUNCTION << "  " << cause << "\n" );

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

    if( is_now )
    {
        //_is_distributed_order_requested = false;
        if( Job_node* job_node = Job_node::try_cast( _order_queue_node ) )
            if( Job* job = job_node->job_or_null() )  job->signal( Z_FUNCTION );
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
        order->assert_task( Z_FUNCTION );
        order->_is_success_state = true;
    }

    return order;
}

//--------------------------------Order_queue::load_and_occupy_next_distributed_order_from_database

Order* Order_queue::load_and_occupy_next_distributed_order_from_database( Task* occupying_task, const Time& now )
{
    Order* result    = NULL;
    S      select_sql;

    string w = db_where_expression();

    select_sql << "select %limit(1)  `job_chain`, `distributed_next_time`, " << order_select_database_columns <<
                "  from " << _spooler->_orders_tablename <<  //" %update_lock"  Oracle kann nicht "for update", limit(1) und "order by" kombinieren
                "  where `spooler_id`=" << sql::quoted(_spooler->id_for_db()) <<
                " and `distributed_next_time` <= {ts'" << now.as_string( Time::without_ms ) << "'}"
                   " and `occupying_cluster_member_id` is null" << 
                   " and " << w <<
                "  order by `distributed_next_time`, `priority`, `ordering`";

    Record record;
    bool   record_filled = false;


    for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION );
        if( !result_set.eof() )
        {
            record = result_set.get_record();
            record_filled = true;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), Z_FUNCTION ); }


    if( record_filled )
    {
        _next_announced_distributed_order_time = Time::never;

        ptr<Order> order;

        try
        {
            order = new Order( _spooler );

            order->load_record( Absolute_path( root_path, record.as_string( "job_chain" ) ), record );
            order->set_distributed();
            
            Job_chain* job_chain = order_subsystem()->job_chain( order->_job_chain_path );
            assert( job_chain->is_distributed() );

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

//string Order_queue::db_where_expression_for_distributed_orders()
//{
//    int is_in_any_job_chain = 0;
//
//    FOR_EACH_JOB_CHAIN( job_chain )
//    {
//        if( job_chain->is_distributed() )
//        {
//            string w = db_where_expression( job_chain );
//            if( w != "" )
//            {
//                if( is_in_any_job_chain )  result << " or ";
//                is_in_any_job_chain++;
//
//                result << w;
//            }
//        }
//    }
//
//    return is_in_any_job_chain <= 1? result
//                                   : "( " + result + " )";
//}

//-------------------------------------------------Order_queue::db_where_expression

string Order_queue::db_where_expression( )
{
    S  result;
    result << "`job_chain`="  << sql::quoted( _job_chain->path().without_slash() ) 
           << " and `state`=" << sql::quoted( _order_queue_node->order_state().as_string() );
    return result;

    //bool is_in_job_chain = false;
    //Z_FOR_EACH( Job_chain::Node_list, job_chain->_node_list, n )
    //{
    //    Node* node = *n;

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

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler )
:
    Com_order(this),
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    _zero_(this+1)
{
    //_log = Z_NEW( Prefix_log( this ) );
    //_log->set_prefix( obj_name() );

    _com_log = new Com_log;
    _com_log->set_log( log() );

    _created       = Time::now();
    _is_virgin     = true;
    //_signaled_next_time = Time::never;

    set_run_time( NULL );
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
        assert( !_standing_order );
      //assert( !_is_replacement );
      //assert( !_replaced_by );
      //assert( !_order_queue );
#   endif

    set_replacement( false );

    if( _replaced_by )  
    {
        _replaced_by->set_replacement( false );
    }

    if( _run_time )  _run_time->close();
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

    if( _job_chain )
    {
        if( _job_chain->is_to_be_removed()  || 
            _job_chain->replacement()  &&  _job_chain->replacement()->file_based_state() == File_based::s_initialized  ||
            _job_chain->state() != Job_chain::s_active )  return false;   // Jobkette wird nicht gelöscht oder ist gestopped (s_stopped)?

        if( Node* node = job_chain_node() )
            if( node->action() != Node::act_process )  return false;
    }

    return true;
}

//----------------------------------------------------------Order::handle_changed_processable_state
// Nach Änderung von next_time(), also _setback und is_processable() zu rufen!

void Order::handle_changed_processable_state()
{
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

    if( distributed )
    {
#     ifndef Z_DEBUG  //test
        //2007-07-27 if( _run_time->set() )  z::throw_xc( "SCHEDULER-397", "<run_time>" );
#     endif
        //if( !job_chain()  ||  !job_chain()->_is_distributed )  z::throw_xc( Z_FUNCTION, obj_name(), "no job_chain or job_chain is not distributed" );
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
    if( !_task )  assert(0), z::throw_xc( "ORDER-HAS-NO-TASK", obj_name(), debug_text );
}

//---------------------------------------------------------------------------Order::occupy_for_task

void Order::occupy_for_task( Task* task, const Time& now )
{
    assert( task );
    assert_no_task( Z_FUNCTION );   // Vorsichtshalber
    

    if( !_log->opened() )  open_log();

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

    if( _is_virgin )
    {
        if( _http_operation )  _http_operation->on_first_order_processing( task );
        order_subsystem()->count_started_orders();
    }

    _is_virgin = false;
    //_is_virgin_in_this_run_time = false;
}

//------------------------------------------------------------------Order::db_occupy_for_processing

bool Order::db_occupy_for_processing()
{
    assert( _is_distributed );
    assert( _job_chain == NULL );   // Der Auftrag kommt erst nach der Belegung in die Jobkette, deshalb ist _job_chain == NULL

    _spooler->assert_are_orders_distributed( Z_FUNCTION );

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
        update_ok = ta.try_execute_single( update, Z_FUNCTION );
        ta.commit( Z_FUNCTION );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), Z_FUNCTION ); }

    
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
    bool   insert_ok      = false;
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
        

        sql::Insert_stmt insert ( ta.database_descriptor(), _spooler->_orders_tablename );
        
        insert[ "ordering"      ] = ordering;
        insert[ "job_chain"     ] = _job_chain_path.without_slash();
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
                ta.execute( insert, Z_FUNCTION );
                insert_ok = true;
            }
            catch( exception& x )     // Datensatz ist bereits vorhanden?
            {
                ta.intermediate_rollback( Z_FUNCTION );      // Postgres verlangt nach Fehler ein Rollback

                if( insert_race_retry_count > max_insert_race_retry_count )  throw;

                Any_file result_set = ta.open_result_set
                    ( 
                        S() << "select `distributed_next_time`" 
                               " from " << _spooler->_orders_tablename << 
                               db_where_clause().where_string(), 
                        Z_FUNCTION 
                    );

                if( !result_set.eof() )
                {
                    Record record = result_set.get_record();
                    if( throw_exists_exception )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_path,
                                                               record.null( "distributed_next_time" )? "in database, not distributed" : "distributed" );
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

            if( run_time() )
            {
                xml::Document_ptr doc = run_time()->dom_document();
                if( doc.documentElement().hasAttributes()  ||  doc.documentElement().hasChildNodes() )  db_update_clob( &ta, "run_time", doc.xml() );
            }

            ta.commit( Z_FUNCTION );

            _is_in_database = true;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-305", _spooler->_orders_tablename, x ), Z_FUNCTION ); }


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
        catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x ), Z_FUNCTION ); }
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
            for( Retry_nested_transaction ta ( _spooler->_db, outer_transaction ); ta.enter_loop(); ta++ ) try
            {
                if( !_spooler->_db->opened() )  break;

                S delete_sql;
                delete_sql << update.make_delete_stmt();
                delete_sql << " and `distributed_next_time` is " << ( _is_distributed? "not null" : " null" );  // update_ok=false, wenn das nicht stimmt

                update_ok = ta.try_execute_single( delete_sql, Z_FUNCTION );
                if( !update_ok )  update_ok = db_handle_modified_order( &ta );  //int DISTRIBUTED_FEHLER_KOENNTE_GEZEIGT_WERDEN; // Zeigen, wenn distributed_next_time falsch ist.

                db()->write_order_history( this, &ta );

                ta.commit( Z_FUNCTION );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x  ), Z_FUNCTION ); }

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

                update_ok = ta.try_execute_single( update, Z_FUNCTION );

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

                ta.commit( Z_FUNCTION );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x ), Z_FUNCTION ); }
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
        _spooler->_db->write_order_history( this, outer_transaction );

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
        if( ptr<Order> modified_order = order_subsystem()->try_load_order_from_database( outer_transaction, _job_chain_path, _id, Order_subsystem::lo_lock ) )
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
    _log->close_file();
    
    if( _job_chain  &&  _spooler->_db  &&  _spooler->_db->opened() ) 
    {
        _spooler->_db->write_order_history( this );  // Historie schreiben, aber Auftrag beibehalten
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
    if( _spooler->_db->db_name() == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    return ta->read_clob( _spooler->_orders_tablename, column_name, db_where_clause().where_string() );
}

//----------------------------------------------------------------------------Order::db_update_clob

void Order::db_update_clob( Transaction* ta, const string& column_name, const string& value )
{
    if( _spooler->_db->db_name() == "" )  z::throw_xc( "SCHEDULER-361", Z_FUNCTION );

    if( value == "" )
    {
        sql::Update_stmt update = db_update_stmt();
        update[ column_name ].set_direct( "null" );
        ta->execute( update, Z_FUNCTION );
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

    if( _run_time )  _run_time->close(), _run_time = NULL;

    if( _standing_order )  _standing_order->check_for_replacing_or_removing();

    _log->close();
}

//-----------------------------------------------------------------------------------Order::set_dom

void Order::set_dom( const xml::Element_ptr& element, Variable_set_map* variable_set_map )
{
    if( !element )  return;
    if( !element.nodeName_is( "order" )  && 
        !element.nodeName_is( "add_order" ) )  z::throw_xc( "SCHEDULER-409", "order", element.nodeName() );


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
            set_run_time( e );
        }
        else
        if( e.nodeName_is( "period" ) )
        { 
            _period = Period();
            _period.set_dom( e, Period::with_date );
        }
        else
        if( e.nodeName_is( "log" ) )
        {
            assert( !_log->opened() );
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

//-------------------------------------------------------------------------------Order::dom_element

xml::Element_ptr Order::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what, const string* log ) const
{
    xml::Element_ptr result = dom_document.createElement( "order" );

    if( _standing_order )  _standing_order->fill_file_based_dom_element( result, show_what );

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

        if( Job* job = this->job() )  result.setAttribute( "job", job->name() );
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

        if( _log->opened() )
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

    if( show_what.is_set( show_run_time ) )  result.appendChild( _run_time->dom_element( dom_document ) );  // Vor _period setzen!

    if( show_what.is_set( show_for_database_only ) )
    {
        // Nach <run_time> setzen!
        if( _period.repeat() < Time::never ||
            _period.absolute_repeat() < Time::never )  result.appendChild( _period.dom_element( dom_document ) );     // Aktuelle Wiederholung merken, für <run_time>
    }

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
    
    return result;
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

                if( _job_chain_path != "" )  e.setAttribute( "job_chain", _job_chain_path );
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
    *s << " job_chain=\"" << xml::encode_attribute_value( _job_chain_path )      << '"';
    *s << " id=\""        << xml::encode_attribute_value( string_id() )          << '"';

    if( _title != "" )
    *s << " title=\""     << xml::encode_attribute_value( _title )               << '"';

    *s << " state=\""     << xml::encode_attribute_value( _state.as_string() )   << '"';

    *s << "/>";
}

//---------------------------------------------------------------------------------------Order::dom

xml::Document_ptr Order::dom( const Show_what& show_what ) const
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
    if( _id_locked )  z::throw_xc( "SCHEDULER-159" );

    string id_string = string_id( id );    // Sicherstellen, das id in einen String wandelbar ist

    if( db()->opened()  &&  id_string.length() > db()->order_id_length_max() )  
        z::throw_xc( "SCHEDULER-345", id_string, db()->order_id_length_max(), _spooler->_orders_tablename + "." + "id" );

    if( id_string.length() > const_order_id_length_max )  z::throw_xc( "SCHEDULER-344", id_string, const_order_id_length_max );

    _id = id;

    _log->set_prefix( obj_name() );
    _log->set_title ( "Order " + _id.as_string() );
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

        if( _id_locked )
        {
            Scheduler_event event ( evt_order_state_changed, log_info, this );
            _spooler->report_event( &event );
        }
    }

    if( !_initial_state_set )  _initial_state = order_state,  _initial_state_set = true;
}

//------------------------------------------------------------------------Order::set_job_chain_node

void Order::set_job_chain_node( Node* node, bool is_error_state )
{
    _job_chain_node = node;

    if( node )
    {
        if( node->is_suspending_order() )  set_suspended();
        if( node->delay()  &&  !at() )  set_at( Time::now() + node->delay() );
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

void Order::remove( File_based::Remove_flags remove_flag )
{
    ptr<Order> hold_me = this;

    if( _standing_order )  
    {
        _standing_order->remove( remove_flag );
        // Löscht erst im Endzustand, deshalb noch remove_from_job_chain() rufen
    }

    remove_from_job_chain(); 
}

//---------------------------------------------------------------------Order::remove_from_job_chain

void Order::remove_from_job_chain( Job_chain_stack_option job_chain_stack_option )
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

        db_delete( update_and_release_occupation );     // Schreibt auch die Historie (auch bei orders_recoverable="no")
    }

    if( _job_chain )  _job_chain->remove_order( this );

    _setback_count = 0;
    _setback = Time(0);

    set_replacement( false );  


    _job_chain = NULL;
    _job_chain_path.clear();


    if( job_chain_stack_option == jc_remove_from_job_chain_stack )  
    {
        remove_from_job_chain_stack();
        //if( _standing_order )  _standing_order->check_for_replacing_or_removing( Standing_order::act_now );  // Kann Auftrag aus der Jobkette nehmen

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
        job_chain->state() != Job_chain::s_active  &&
        job_chain->state() != Job_chain::s_stopped )  z::throw_xc( "SCHEDULER-151" );

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
        //job_chain->job_from_state( _state );     int JOB_KANN_FEHLEN;  // Fehler bei Endzustand. Wir speichern den Auftrag nur, wenn's einen Job zum Zustand gibt

        if( node->is_suspending_order() )  _suspended = true;

        if( !_is_distribution_inhibited  &&  job_chain->is_distributed() )  set_distributed();

        _job_chain_path = job_chain->path();
        _removed_from_job_chain_path.clear();

        //activate();     // Errechnet die nächste Startzeit
        set_next_start_time();

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
            other_order->remove_from_job_chain();
            place_in_job_chain( job_chain );

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

//----------------------------------------------------------------------------------Order::activate

//void Order::activate()
//{
//    //assert( order_subsystem()->subsystem_state() == subsys_active );
//
//    set_next_start_time();
//}

//-----------------------------------------------------------------------Order::set_next_start_time

void Order::set_next_start_time()
{
    if( _state == _initial_state  &&  !_setback  &&  _run_time->set() )
    {
        set_setback( next_start_time( true ) );     // Braucht für <run_time start_time_function=""> das Scheduler-Skript
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

void Order::postprocessing( bool success )
{
    _is_success_state = success;

    Job* last_job = _task? _task->job() : NULL;

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

    if( _task  &&  _moved )
        if( Job_node* job_node = Job_node::try_cast( _job_chain_node ) )
            if( Job* job = job_node->job_or_null() )  job->signal( "delayed set_state()" );

    _task = NULL;



    if( !is_setback()  &&  !_moved  &&  !_end_state_reached  ||  force_error_state )
    {
        if( _job_chain_node )
        {
            assert( _job_chain );

            set_state1( success? _job_chain_node->next_state()
                               : _job_chain_node->error_state() );
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
        bool is_first_call = _run_time_modified;
        _run_time_modified = false;
        Time next_start = next_start_time( is_first_call );

        if( next_start != Time::never  &&  _state != _initial_state )   // <run_time> verlangt Wiederholung?
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
                    _log->error( message_string( "SCHEDULER-340" ) );
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

//-------------------------------------------------------------------------------------------------

bool Order::handle_end_state_of_nested_job_chain()
{
    Z_DEBUG_ONLY( assert( !_is_distributed ) );

    bool end_state_reached = false;

    try
    {
        Job_chain*  outer_job_chain            = order_subsystem()->job_chain( _outer_job_chain_path );
        Node*       outer_job_chain_node       = outer_job_chain->node_from_state( _outer_job_chain_state );
        State       next_outer_job_chain_state = _is_success_state? outer_job_chain_node->next_state() 
                                                                  : outer_job_chain_node->error_state();

        Nested_job_chain_node* next_outer_job_chain_node = Nested_job_chain_node::try_cast( outer_job_chain->node_from_state_or_null( next_outer_job_chain_state ) );
        

        if( next_outer_job_chain_node  &&  next_outer_job_chain_node->nested_job_chain() )
        {
            Job_chain* next_job_chain = next_outer_job_chain_node->nested_job_chain();

            _log->info( message_string( "SCHEDULER-862", next_job_chain->obj_name() ) );

            close_log_and_write_history();// Historie schreiben, aber Auftrag beibehalten
            _start_time = 0;
            _end_time = 0;
            open_log();

            _state.clear();     // Lässt place_in_job_chain() den ersten Zustand der Jobkette nehmen
            place_in_job_chain( next_job_chain, jc_leave_in_job_chain_stack );  // Entfernt Auftrag aus der bisherigen Jobkette
            _outer_job_chain_path = Absolute_path( root_path, outer_job_chain->path() );  // place_in_job_chain() hat's gelöscht

            _log->info( message_string( "SCHEDULER-863", _job_chain->obj_name() ) );
        }
        else
        {
            // Bei <run_time> Auftrag an den Anfang der ersten Jobkette setzen
            end_state_reached = true;
        }

        _outer_job_chain_state = next_outer_job_chain_state;
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
    // Auftrag wird wegen <run_time> wiederholt

    Absolute_path first_nested_job_chain_path;

    if( _outer_job_chain_path != "" )
    {
        if( Nested_job_chain_node* n = Nested_job_chain_node::try_cast( order_subsystem()->active_job_chain( _outer_job_chain_path )->first_node() ) )
            first_nested_job_chain_path = n->nested_job_chain_path();

        remove_from_job_chain( jc_leave_in_job_chain_stack );  
    }

    _log->info( message_string( "SCHEDULER-944", _initial_state, next_start ) );        // "Kein weiterer Job in der Jobkette, der Auftrag wird mit state=<p1/> wiederholt um <p2/>"
    
    close_log_and_write_history();  // Historie schreiben, aber Auftrag beibehalten
    _start_time = 0;
    _end_time = 0;

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

    if( _standing_order )  _standing_order->check_for_replacing_or_removing( Standing_order::act_now );     // Kann Auftrag aus der Jobkette nehmen
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

    if( _moved  &&  job  &&  !order_queue()->has_order() )
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
                _web_service->forward_order( *this, last_job );
            }
        }
        catch( exception x )  { _log->error( x.what() ); }
    }


    if( _job_chain  &&  _is_in_database )
    {
        try
        {
            db_update( update_and_release_occupation );
        }
        catch( exception& x )
        {
            _log->error( message_string( "SCHEDULER-313", x ) );
        }
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

    if( _run_time  &&  _run_time->set() )
    {
        Time now = Time::now();

        if( first_call )
        {
            _period = _run_time->next_period( now, time::wss_next_period_or_single_start );

            if( !_period.absolute_repeat().is_never() )
            {
                result = _period.next_repeated( now );

                if( result.is_never() )
                {
                    _period = _run_time->next_period( _period.end(), time::wss_next_period_or_single_start );
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
            result = _period.next_repeated( now );

            if( result >= _period.end() )       // Periode abgelaufen?
            {
                Period next_period = _run_time->next_period( _period.end(), time::wss_next_begin );
                //Z_DEBUG_ONLY( fprintf(stderr,"%s %s\n", Z_FUNCTION, next_period.obj_name().c_str() ) );
                
                if( _period.repeat().is_never()
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

//-----------------------------------------------------------------Order::on_before_modify_run_time

void Order::on_before_modify_run_time()
{
# ifndef Z_DEBUG  //test
    //2007-07-27 if( _is_distributed )  z::throw_xc( "SCHEDULER-397", "<run_time>" );
# endif

  //if( _task       )  z::throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
  //if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_path );
}

//-------------------------------------------------------------------Order::run_time_modified_event

void Order::run_time_modified_event()
{
    _setback = 0;           // Änderung von <run_time> überschreibt Order.at
    set_next_start_time();

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

Order_subsystem* Order::order_subsystem() const
{
    return static_cast<Order_subsystem*>( _spooler->order_subsystem() );
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
