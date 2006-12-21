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
namespace spooler {

//-------------------------------------------------------------------------------------------------

const int    check_database_orders_period           = 15;
const int    check_database_orders_period_minimum   = 1;
const int    database_orders_read_ahead_count       = 1;
const string database_null_at_date                  = "2000-01-01 00:00:00";
const string default_end_state_name                 = "<END_STATE>";
const string order_select_database_columns          = "`id`, `priority`, `state`, `state_text`, `initial_state`, `title`, `created_time`";

//--------------------------------------------------------------------------Database_order_detector

struct Database_order_detector : Async_operation, Scheduler_object
{
                                Database_order_detector( Spooler* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    // Scheduler_operation
    Scheduler_object::obj_name;


    string                      make_union_select_order_sql ( const string& select_sql_begin, const string& select_sql_end );
    string                      make_where_expression_for_job( Job* );
    bool                        is_job_requesting_order     ( Job* );
    int                         read_result_set             ( Transaction*, const string& select_sql );
    void                        set_alarm                   ();
    void                        request_order_for_job       ( Job* );
    void                        withdraw_order_request_for_job( Job* );

  private:
    Fill_zero                  _zero_;
    Time                       _now;                        // Zeitpunkt von async_continue_()
    Time                       _database_null_at_time;
    int                        _next_check_period;
  //stdext::hash_set<string>   _names_of_requesting_jobs;
};

//-----------------------------------------------------------------Order_subsystem::Order_subsystem

Order_subsystem::Order_subsystem( Spooler* spooler )
:
    Scheduler_object( spooler, this, Scheduler_object::type_order_subsystem ),
    _zero_(this+1)
{
}

//----------------------------------------------------------------------------Order_subsystem::init

void Order_subsystem::init()
{
    init_file_order_sink();
}

//----------------------------------------------------------------------------Order_subsystem::init

void Order_subsystem::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    if( is_sharing_orders_in_database() )
    {
        _database_order_detector = Z_NEW( Database_order_detector( _spooler ) );
        _database_order_detector->set_async_manager( _spooler->_connection_manager );
    }
}

//---------------------------------------------------------------------------Order_subsystem::close

void Order_subsystem::close()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    _job_chain_map.clear();
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
    int DATENBANK_LESEN;

    xml::Element_ptr job_chains_element = document.createElement( "job_chains" );

    job_chains_element.setAttribute( "count", (int)_job_chain_map.size() );

    if( show & ( show_job_chains | show_job_chain_jobs | show_job_chain_orders ) )
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


        if( !order_subsystem()->is_sharing_orders_in_database()  &&  job_chain->_orders_recoverable )
        {
            if( !_spooler->db()  ||  !_spooler->db()->opened() )  // Beim Start des Schedulers
            {
                job_chain->_load_orders_from_database = true;
            }
            else
            {
                for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
                {
                    job_chain->add_order_from_database( &ta );
                }
                catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ) ); }
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

//-------------------------------------------------------Order_subsystem::load_orders_from_database

void Order_subsystem::load_orders_from_database()
{
    if( db()->opened()  &&  !is_sharing_orders_in_database() )
    {
        Transaction ta ( db() );

        FOR_EACH( Job_chain_map, _job_chain_map, it ) 
        {
            Job_chain* job_chain = it->second;
            if( job_chain->_load_orders_from_database )
                job_chain->add_order_from_database( &ta );  // Die Jobketten aus der XML-Konfiguration
        }

        ta.commit( __FUNCTION__ );
    }
}

//---------------------------------------------------Order_subsystem::is_sharing_orders_in_database

bool Order_subsystem::is_sharing_orders_in_database()
{
    return _spooler->is_distributed();
}

//--------------------------------------------Order_subsystem::assert_is_sharing_orders_in_database

void Order_subsystem::assert_is_sharing_orders_in_database( const string& debug_text )
{
    return _spooler->assert_is_distributed( debug_text );
}

//-------------------------------------------Order_subsystem::assert_not_sharing_orders_in_database

void Order_subsystem::assert_not_sharing_orders_in_database( const string& debug_text )
{
    if( is_sharing_orders_in_database() )  z::throw_xc( "SCHEDULER-375", debug_text );
}

//-----------------------------------------------------------Order_subsystem::request_order_for_job

void Order_subsystem::request_order_for_job( Job* job )
{
    if( _database_order_detector )
    {
        _database_order_detector->request_order_for_job( job );
    }
}

//-----------------------------------------------------------------Order_subsystem::check_exception

void Order_subsystem::check_exception()
{
    if( _database_order_detector )  _database_order_detector->async_check_exception();
}

//-------------------------------------------------Database_order_detector::Database_order_detector

Database_order_detector::Database_order_detector( Spooler* spooler ) 
:
    _zero_(this+1),
    Scheduler_object( spooler, this, Scheduler_object::type_database_order_detector )
{
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

    result << obj_name();

    FOR_EACH_JOB( j )
    {
        Job* job = *j;
        if( job->order_controlled()  &&  job->order_queue()->is_order_requested() )  
        {
            if( !jobs.empty() )  jobs << ", ";
            jobs << job->name();
        }
    }

    if( !jobs.empty() )  result << ", requesting jobs: " << jobs;
        
    return result;
}

//---------------------------------------------------------Database_order_detector::async_continue_

bool Database_order_detector::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  " << async_state_text() << "\n" );
    order_subsystem()->assert_is_sharing_orders_in_database( __FUNCTION__);


    _now = Time::now();

    Time read_until             = _now + check_database_orders_period;
    int  announced_orders_count = 0;

    string select_sql_begin = S() << "select %limit(" << database_orders_read_ahead_count << ") `at`, `job_chain`, `state`"
                    "  from " << _spooler->_orders_tablename <<
                    "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                       " and `processable` is not null"
                       " and `processing_scheduler_member_id` is null";

    string select_sql_end = "  order by `at`";

    
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
                
                if( is_job_requesting_order( job ) )
                {
                    string job_chain_expression = make_where_expression_for_job( job );

                    if( job_chain_expression != "" )
                    {
                        announced_orders_count +=
                        read_result_set( &ta, S() << select_sql_begin + " and " + job_chain_expression << select_sql_end );
                    }
                }
            }
        }

        //if( ta.is_transaction_used() )
        //{
        //    bool ok = _spooler->do_a_heart_beat( &ta );
        //    if( !ok )  log()->error( S() << __FUNCTION__ << "  do_a_heart_beat() return false" );
        //    int WAS_TUN_BEI_EINEM_INFARKT;
        //}
     
        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }

    if( announced_orders_count )  _next_check_period = check_database_orders_period_minimum;
    set_alarm();

    return true;
}

//--------------------------------------Database_order_detector::make_union_select_order_sql

string Database_order_detector::make_union_select_order_sql( const string& select_sql_begin, const string& select_sql_end )
{
    S    result;
    bool has_any_job = false;

    FOR_EACH_JOB( job_iterator )
    {
        Job* job = *job_iterator;

        if( is_job_requesting_order( job ) )
        {
            string job_chains_expression = make_where_expression_for_job( job );

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

//-------------------------------------------------Database_order_detector::is_job_requesting_order

bool Database_order_detector::is_job_requesting_order( Job* job )
{
    bool result = false;

    if( job->order_controlled() )
    {
        Order_queue* order_queue = job->order_queue();
    
        result = order_queue->is_order_requested()  &&  order_queue->next_announced_order_time() > _now;
    }

    return result;
}

//-------------------------------------------Database_order_detector::make_where_expression_for_job

string Database_order_detector::make_where_expression_for_job( Job* job )
{
    S result;
                  
    result << job->order_queue()->make_where_expression();

    if( !result.empty() )
    {
        Time t = job->order_queue()->next_announced_order_time();
        if( t < Time::never )  result << " and `at`<{ts'" << t.as_string( Time::without_ms ) << "'}";
    }

    return result;
}

//---------------------------------------------------------Database_order_detector::read_result_set

int Database_order_detector::read_result_set( Transaction* ta, const string& select_sql )
{
    int      count      = 0;
    Any_file result_set = ta->open_result_set( select_sql, __FUNCTION__ );

    while( !result_set.eof() )
    {
        Record     record    = result_set.get_record();
        Job_chain* job_chain = order_subsystem()->job_chain( record.as_string( "job_chain" ) );
        Job*       job       = job_chain->job_from_state( record.as_string( "state" ) );
        Time       at;

        at.set_datetime( record.as_string( "at" ) );
        if( at == _database_null_at_time )  at.set_null();
        
        job->order_queue()->set_next_announced_order_time( at, at <= _now );
        
        //_names_of_requesting_jobs.erase( job->name() );
        count++;
    }

    return count;
}

//---------------------------------------------------------------Database_order_detector::set_alarm

void Database_order_detector::set_alarm()
{
    FOR_EACH_JOB( j )
    {
        Job* job = *j;
        if( job->order_controlled()  &&  job->order_queue()->is_order_requested() )  
        {
            if( !_next_check_period )  _next_check_period = check_database_orders_period_minimum;
            set_async_delay( _next_check_period );
            _next_check_period = min( 2*_next_check_period, check_database_orders_period );
            break;
        }
    }
}

//-----------------------------------------------------------Database_order_detector::request_order

void Database_order_detector::request_order_for_job( Job* job )

// Nur einmal für einen Job rufen, solange der Job keinen neuen Auftrag bekommen hast! Order_queue::request_order() kümmert sich darum.

{
  //assert( _names_of_requesting_jobs.find( job->name() ) == _names_of_requesting_jobs.end() );

    //_log->info( S() << "NEW REQUESTING JOB " << job->name() );

    //_names_of_requesting_jobs.insert( job->name() );
    //if( !_is_order_requested )
    {
        //_is_order_requested = true;
        async_wake();
    }
}

//------------------------------------------Database_order_detector::withdraw_order_request_for_job

void Database_order_detector::withdraw_order_request_for_job( Job* job )
{
    //_names_of_requesting_jobs.erase( job->name() );
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
    _next_job = _job_chain->node_from_state( _next_state )->_job;    // Ist nicht NULL
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
            element.setAttribute( "orders", order_count( job_chain ) );
            element.setAttribute( "job", _job->name() );
            
            if( _job->order_queue()->is_order_requested() )
                element.setAttribute( "is_order_requested", "yes" );

            if( show & show_job_chain_jobs )
            {
                dom_append_nl( element );
                element.appendChild( _job->dom_element( document, show, job_chain ) );
                dom_append_nl( element );
            }
            else
            if( show & show_job_chain_orders )
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

int Job_chain_node::order_count( Job_chain* job_chain )
{
    int DATENBANK_LESEN;

    return _job? _job->order_queue()->order_count( job_chain ) : 0;
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
    _visible            = element.bool_getAttribute( "visible"           , _visible );
    _orders_recoverable = element.bool_getAttribute( "orders_recoverable", _orders_recoverable );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "file_order_source" ) )
        {
            ptr<Directory_file_order_source> d = Z_NEW( Directory_file_order_source( this, e ) );
            _order_sources._order_source_list.push_back( +d );
        }
        else
        if( e.nodeName_is( "file_order_sink" ) )
        {
            string state = e.getAttribute( "state" );

            bool can_be_not_initialized = true;
            Job* job = _spooler->get_job( file_order_sink_job_name, can_be_not_initialized );
            job->set_visible( true );

            Job_chain_node* node = add_job( job, state, Variant(Variant::vt_missing), Variant(Variant::vt_missing) );

            node->_file_order_sink_move_to.set_directory( e.getAttribute( "move_to" ) );
            node->_file_order_sink_remove  = e.bool_getAttribute( "remove" );
        }
        else
        if( e.nodeName_is( "job_chain_node" ) )
        {
            string job_name = e.getAttribute( "job" );
            string state    = e.getAttribute( "state" );

            bool can_be_not_initialized = true;
            Job* job = job_name == ""? NULL : _spooler->get_job( job_name, can_be_not_initialized );
            if( state == "" )  z::throw_xc( "SCHEDULER-231", "job_chain_node", "state" );

            Job_chain_node* node = add_job( job, state, e.getAttribute( "next_state" ), e.getAttribute( "error_state" ) );

            if( e.bool_getAttribute( "suspend", false ) )  node->_suspend = true;
        }
    }
}

//----------------------------------------------------------xml::Element_ptr Job_chain::dom_element

xml::Element_ptr Job_chain::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    Show_what modified_show = show;
    if( modified_show & show_job_chain_orders )  modified_show |= show_orders;


    xml::Element_ptr element = document.createElement( "job_chain" );

    //THREAD_LOCK( _lock )
    {
        element.setAttribute( "name"  , _name );
        element.setAttribute( "orders", order_count() );
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


        if( show & show_order_history  &&  _spooler->_db->opened() )
        {
            xml::Element_ptr order_history_element = document.createElement( "order_history" );

            try
            {
                Transaction ta ( _spooler->_db, _spooler->_db->transaction() );

                Any_file sel = ta.open_result_set( 
                               "select %limit(20) \"ORDER_ID\" as \"ID\", \"JOB_CHAIN\", \"START_TIME\", \"TITLE\", \"STATE\", \"STATE_TEXT\""
                               " from " + _spooler->_order_history_tablename +
                               " where \"JOB_CHAIN\"=" + sql::quoted( _name ) +
                                 " and \"SPOOLER_ID\"=" + sql::quoted( _spooler->id_for_db() ) +
                               " order by \"HISTORY_ID\" desc" );

                while( !sel.eof() )
                {
                    Record record = sel.get_record();

                    ptr<Order> order = new Order( _spooler );
                    order->set_id        ( record.as_string( "id"         ) );
                    order->set_state     ( record.as_string( "state"      ) );
                    order->set_state_text( record.as_string( "state_text" ) );
                    order->set_title     ( record.as_string( "title"      ) );

                    xml::Element_ptr order_element = order->dom_element( document, show );
                    order_element.setAttribute_optional( "job_chain", record.as_string( "job_chain"  ) );

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

            if( show & show_blacklist )
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
    assert( order_subsystem()->is_sharing_orders_in_database() == order->_is_db_occupied );
    if( state() != s_ready )  z::throw_xc( "SCHEDULER-151" );
    if( has_order_id( order->id() ) )  z::throw_xc( "SCHEDULER-186", order->obj_name(), name() );

    set_visible( true );

    Job_chain_node* node = node_from_state( order->_state );
    if( !node->_job  || !node->_job->order_queue() )  z::throw_xc( "SCHEDULER-149", name(), debug_string_from_variant(order->_state) );

    order->_job_chain      = this;
    order->_job_chain_name = name();
    order->_removed_from_job_chain_name = "";
    order->_log->set_prefix( order->obj_name() );
    order->set_job_chain_node( node );

    register_order( order );
    node->_job->order_queue()->add_order( order );
}

//--------------------------------------------------------------------------Job_chain::remove_order

void Job_chain::remove_order( Order* order )
{
    assert( order->_job_chain_name == _name );
    assert( order->_job_chain == this );

    ptr<Order> hold_order = order;   // Halten

    if( order->_on_blacklist )
    {
        remove_from_blacklist( order );
    }

    if( order->_job_chain_node )
    {
        order->remove_from_job();
    }

    if( order->_is_db_occupied )  
    {
        order->db_release_occupation();
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

//-------------------------------------------------------------------------Order::bind_to_job_chain

//void Order::bind_to_job_chain( Job_chain* job_chain )
//{
//    _log->set_prefix( obj_name() );
//
//    Job_chain_node* node = job_chain->node_from_state( _state );
//    if( !node->_job  || !node->_job->order_queue() )  z::throw_xc( "SCHEDULER-149", job_chain->name(), debug_string_from_variant(_state) );
//    set_job_chain_node( node );
//}

//-------------------------------------------------------------Job_chain::add_order_from_database

void Job_chain::add_order_from_database( Transaction* ta )
{
    assert( _orders_recoverable );
    _spooler->assert_has_exclusiveness( __FUNCTION__ );
    assert( _spooler->db()  &&  _spooler->db()->opened() );

    _load_orders_from_database = false;


    int count = 0;

    Any_file result_set = ta->open_result_set
        ( 
            S() << "select " << order_select_database_columns <<
            "  from " << _spooler->_orders_tablename <<
            "  where \"SPOOLER_ID\"=" << sql::quoted(_spooler->id_for_db()) <<
              " and \"JOB_CHAIN\"="  << sql::quoted(_name) <<
            "  order by \"ORDERING\"",
            __FUNCTION__
        );

    count += load_orders_from_result_set( ta, &result_set );

    log()->debug( message_string( "SCHEDULER-935", count ) );
}

//-----------------------------------------------------------Job_chain::load_orders_from_result_set

int Job_chain::load_orders_from_result_set( Transaction* ta, Any_file* result_set )
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

Order* Job_chain::add_order_from_database_record( Transaction* ta, const Record& record )
{
    string order_id = record.as_string( "id" );

    ptr<Order> order = new Order( _spooler, record, name() );
    order->load_blobs( ta );

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
            Z_LOG2( "scheduler", __FUNCTION__ << ": " << order->obj_name() << " wird nicht entfernt, weil in Verarbeitung durch " << order->_task->obj_name() << "\n" );
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
    int DATENBANK_LESEN; //??

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

int Job_chain::order_count()
{
    int       result = 0;
    set<Job*> jobs;             // Jobs können (theoretisch) doppelt vorkommen, sollen aber nicht doppelt gezählt werden.

    if( order_subsystem()->is_sharing_orders_in_database() )
    {
        for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
        {
            result = ta.open_result_set
                        (
                            S() << "select count(*)  from " << _spooler->_orders_tablename <<
                                   "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                                   " and `job_chain`="  << sql::quoted( name() ) 
                        ).get_record().as_int( 0 );

            ta.commit( __FUNCTION__ );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
    }
    else
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job* job = (*it)->_job;
            if( job  &&  !set_includes( jobs, job ) )  jobs.insert( job ),  result += job->order_queue()->order_count( this );
        }
    }

    return result;
}

//--------------------------------------------------------------------------Job_chain::has_order_id

bool Job_chain::has_order_id( const Order::Id& order_id )
{
    int IN_DATENBANK_PRUEFEN;

    //if( _spooler->order_subsystem->is_share..() )
    //"select `id`  from  " << _spooler->_orders_tablename << "  where " << db_where()";
    //if( !record.eof() )  return true;
    //else

    Order_map::iterator it = _order_map.find( Order::string_id( order_id ) );
    return it != _order_map.end();
}

//------------------------------------------------------------------------Job_chain::register_order

void Job_chain::register_order( Order* order )
{
    //THREAD_LOCK( _lock )
    {
        string id_string = order->string_id();
        Order_map::iterator it = _order_map.find( id_string );
        if( it != _order_map.end() )  z::throw_xc( "SCHEDULER-186", order->obj_name(), _name );
        _order_map[ id_string ] = order;
    }
}

//----------------------------------------------------------------------Job_chain::unregister_order

void Job_chain::unregister_order( Order* order )
{
    remove_from_blacklist( order );

    //THREAD_LOCK( _lock )
    {
        Order_map::iterator it = _order_map.find( order->string_id() );
        if( it != _order_map.end() )  _order_map.erase( it );
                                else  Z_LOG2( "scheduler", __FUNCTION__ << " " << order->obj_name() << " ist nicht registriert.\n" );
    }
}

//----------------------------------------------------------------------Job_chain::add_to_blacklist

void Job_chain::add_to_blacklist( Order* order )
{
    order_subsystem()->assert_not_sharing_orders_in_database( __FUNCTION__ );

    _blacklist_map[ order->string_id() ] = order;
    //_blacklist.push_back( order );
    order->_on_blacklist = true;
}

//-----------------------------------------------------------------Job_chain::remove_from_blacklist

void Job_chain::remove_from_blacklist( Order* order )
{
    if( order->_on_blacklist )
    {
        _blacklist_map.erase( order->string_id() );
        order->_on_blacklist = false;
    }
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

//-------------------------------------------------------------------------Order_queue::Order_queue

Order_queue::Order_queue( Job* job, Prefix_log* log )
:
    _zero_(this+1),
    _spooler(job->_spooler),
    _job(job),
    _log(log),
    _next_announced_order_time( Time::never )
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

    for( Queue::iterator it = _queue.begin(); it != _queue.end(); it = _queue.erase( it ) )
    {
        Order* order = *it;
        _log->info( message_string( "SCHEDULER-937", order->obj_name() ) );
    }

    //update_priorities();
    //_has_users_id = false;
}

//-------------------------------------------------------------------------Order_queue::dom_element

xml::Element_ptr Order_queue::dom_element( const xml::Document_ptr& document, const Show_what& show, Job_chain* which_job_chain )
{
    xml::Element_ptr element = document.createElement( "order_queue" );

    //THREAD_LOCK( _lock )
    {
        int queue_length = order_count( which_job_chain );
        element.setAttribute( "length", queue_length );

        //if( Time next = next_time() )
        element.setAttribute( "next_start_time", next_time().as_string() );

        if( show & show_orders )
        {
            int limit = show._max_orders;

            FOR_EACH( Queue, _queue, it )
            {
                Order* order = *it;
                if( !which_job_chain  ||  order->_job_chain == which_job_chain )
                {
                    if( limit-- <= 0 )  break;
                    dom_append_nl( element );
                    element.appendChild( order->dom_element( document, show ) );
                }
            }

            dom_append_nl( element );
        }
    }

    return element;
}

//-------------------------------------------------------------------------Order_queue::order_count

int Order_queue::order_count( const Job_chain* which_job_chain )
{
    int result;

    if( order_subsystem()->is_sharing_orders_in_database() )
    {
        string w = which_job_chain? make_where_expression_for_job_chain( which_job_chain )
                                  : make_where_expression();
        if( w == "" )
        {
            result = 0;
        }
        else
        {
            for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
            {
                result = ta.open_result_set
                            (
                                S() << "select count(*)  from " << _spooler->_orders_tablename <<
                                       "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                                       " and " << w 
                            ).get_record().as_int( 0 );

                ta.commit( __FUNCTION__ );
            }
            catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
        }
    }
    else
    {
        if( which_job_chain )
        {
            int count = 0;

            FOR_EACH( Queue, _queue, it )  if( (*it)->_job_chain == which_job_chain )  count++;

            result = count;
        }
        else
        {
            result = _queue.size();
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

    _job->set_visible( true );


    Time next_time = order->next_time();


    if( next_time )
    {
        if( !order->_suspended )
        {
            if( order->_setback < Time::never )  order->_log->log( do_log? log_info : log_debug3, message_string( "SCHEDULER-938", order->_setback ) );
                                           else  order->_log->log( do_log? log_warn : log_debug3, message_string( "SCHEDULER-296" ) );       // "Die <run_time> des Auftrags hat keine nächste Startzeit" );
        }
    }
    else
        _log->debug( message_string( "SCHEDULER-990", order->obj_name() ) );


    Queue::iterator insert_before = _queue.begin();
    bool            wake_up       = !order->_task  &&  !has_order( Time::now() );
    
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

    _job->calculate_next_time_after_modified_order_queue();
    if( wake_up )  _job->signal( "Order" );
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

bool Order_queue::request_order( const Time& now )
{
    bool result = _next_announced_order_time <= now;

    if( !result )
    {
        if( first_order( now ) )
        {
            assert( !order_subsystem()->is_sharing_orders_in_database() );
            result = true;
        }
        else
        if( !_is_order_requested    // Das erste Mal?
         && order_subsystem()->is_sharing_orders_in_database()  
         && order_subsystem()->is_job_in_any_job_chain( _job ) )  // Das ist bei vielen Jobketten nicht effizent
        {
            order_subsystem()->request_order_for_job( _job );     // Nur einmal rufen, bis ein neuer Auftrag für den Job eingetrifft
            _is_order_requested = true;
        }
    }

    return result;
}

//--------------------------------------------------------------Order_queue::withdraw_order_request

void Order_queue::withdraw_order_request()
{
    _is_order_requested = false;

    if( _spooler->_order_subsystem  &&  _spooler->_order_subsystem->_database_order_detector )
    {
        _spooler->_order_subsystem->_database_order_detector->withdraw_order_request_for_job( _job );
    }
}

//-------------------------------------------------------Order_queue::set_next_announced_order_time

void Order_queue::set_next_announced_order_time( const Time& t, bool is_now )
{ 
    _next_announced_order_time = t; 

    Z_DEBUG_ONLY( assert( is_now? t <= Time::now() : t > Time::now() ) );

    if( is_now )
    {
        //_is_order_requested = false;
        _job->signal( __FUNCTION__ );
    }
}

//-----------------------------------------------------------Order_queue::next_announced_order_time

Time Order_queue::next_announced_order_time()
{ 
    return _next_announced_order_time; 
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

        if( order->next_time() > now )  break;
    }

    if( result )  assert( !order_subsystem()->is_sharing_orders_in_database() );

    return result;
}

//-------------------------------------------------------------------------Order_queue::fetch_order

Order* Order_queue::fetch_order( const Time& now )

// Kann zusätzlich zu Order_queue::first_order() die Warteschlange aufräumen

{
    Order* order = NULL;

    while( !order )
    {
        order = first_order( now );
        if( !order )  break;

        if( order->is_virgin()  &&  order->is_file_order()  &&  order->_job_chain
         && !file_exists( order->file_path() ) )
        {
            order->log()->info( message_string( "SCHEDULER-982" ) );  // Datei ist entfernt worden
            order->remove_from_job_chain();
            order = NULL;
        }
    }

    return order;
}

//--------------------------------------------------------------Order_queue::fetch_and_occupy_order

Order* Order_queue::fetch_and_occupy_order( const Time& now, const string& cause, Task* occupying_task )
{
    assert( occupying_task );

    Order* order = fetch_order( now );

    if( order )  
    {
        assert( !order_subsystem()->is_sharing_orders_in_database() );
        order->occupy_for_task( occupying_task, now );
    }
    else
    if( _next_announced_order_time <= now )   // Auftrag nur lesen, wenn vorher angekündigt
    {
        _is_order_requested = false;            // Nächster request_order() führt zum async_wake() des Database_order_detector
        _next_announced_order_time = Time::never;

        order = load_and_occupy_next_processable_order_from_database( occupying_task, now );
        // Möglicherweise NULL
    }

    //_is_order_requested = false;
    //if( order )  _is_order_requested = false;
    //       else  request_order( now );

    return order;
}

//--------------------------------Order_queue::load_and_occupy_next_processable_order_from_database

Order* Order_queue::load_and_occupy_next_processable_order_from_database( Task* occupying_task, const Time& now )
{
    Order* result   = NULL;
    S      select_sql;

    select_sql << "select %limit(1)  `job_chain`, " << order_select_database_columns <<
                "  from " << _spooler->_orders_tablename << " %update_lock" 
                "  where `spooler_id`=" << sql::quoted(_spooler->id_for_db()) <<
                   " and `processing_scheduler_member_id` is null" << 
                   " and " << make_where_expression() <<
                "  order by `at`, `priority`, `ordering`";

    for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( select_sql );
        if( !result_set.eof() )
        {
            Record     record    = result_set.get_record();
            ptr<Order> order     = new Order( _spooler, record, record.as_string( "job_chain" ) );
            Job_chain* job_chain = order_subsystem()->job_chain( order->_job_chain_name );

            try
            {
                bool ok = order->db_occupy_for_processing( &ta );
                if( ok )
                {
                    order->load_blobs( &ta );
                    order->occupy_for_task( occupying_task, now );

                    job_chain->add_order( order );

                    result = order;
                }
            }
            catch( exception& x )
            {
                ta.rollback( __FUNCTION__ );
                order->_is_db_occupied = false;

                Z_LOGI2( "scheduler", __FUNCTION__ << "  " << x.what() << "\n" );

                order->close();
                throw;
            }
        }

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }

    return result;
}

//---------------------------------------------------------------------------Order_queue::next_time

Time Order_queue::next_time()
{
    Order* order  = first_order( Time(0) );
    Time   result = order? order->next_time() : Time::never;

    if( result > _next_announced_order_time )  result = _next_announced_order_time;

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

string Order_queue::make_where_expression()
{
    S   result;
    int is_in_any_job_chain = 0;

    Z_FOR_EACH( Order_subsystem::Job_chain_map, order_subsystem()->_job_chain_map, jc )
    {
        Job_chain* job_chain = jc->second;

        string w = make_where_expression_for_job_chain( job_chain );
        if( w != "" )
        {
            if( is_in_any_job_chain )  result << " or ";
            is_in_any_job_chain++;

            result << w;
        }
    }

    return is_in_any_job_chain <= 1? result
                                   : "( " + result + " )";
}

//---------------------------------------------------------------Order_queue::make_where_expression

string Order_queue::make_where_expression_for_job_chain( const Job_chain* job_chain )
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
    return _spooler->order_subsystem(); 
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
        assert( !_on_blacklist );
        assert( !_in_job_queue );
        assert( !_replacement_for );
        assert( !_replaced_by );
        assert( !_order_queue );
#   endif
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    //_recoverable = true;

    _log = Z_NEW( Prefix_log( this ) );
    _log->set_prefix( obj_name() );
    _created = Time::now();
    _is_virgin = true;
    _is_virgin_in_this_run_time = true;

    set_run_time( NULL );
}

//--------------------------------------------------------------------------------Order::load_blobs

void Order::load_blobs( Transaction* ta )
{
    string payload_string = db_read_clob( ta, "payload"   );
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

    string run_time_xml = db_read_clob( ta, "run_time"  );
    if( run_time_xml != "" )  set_run_time( xml::Document_ptr( run_time_xml ).documentElement() );

    string order_xml = db_read_clob( ta, "order_xml" );
    if( order_xml != "" )  set_dom( xml::Document_ptr( order_xml ).documentElement() );
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
    // scheduler_orders.processable := !_on_blacklist && !_suspende

    return _setback <= now  &&  is_processable();
}

//----------------------------------------------------------------Order::is_immediately_processable

bool Order::is_processable()
{
    if( _on_blacklist )     return false;  
    if( _suspended )        return false;
    if( _task )             return false;               // Schon in Verarbeitung
    if( _replacement_for )  return false;
    if( _job_chain  &&  _job_chain->state() != Job_chain::s_ready )  return false;   // Jobkette wird nicht gelöscht?

    return true;
}

//----------------------------------------------------------------------------Order::assert_no_task

void Order::assert_no_task()
{
    if( _task )  z::throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
}

//---------------------------------------------------------------------------Order::occupy_for_task

bool Order::occupy_for_task( Task* task, const Time& now )
{
    assert( task );
    assert_no_task();   // Vorsichtshalber
    

    if( !_log->opened() )  open_log();

    if( _delay_storing_until_processing  &&  _job_chain  &&  _job_chain->_orders_recoverable  &&  !_is_in_database )
    {
        db_insert();
        _delay_storing_until_processing = false;
    }


    if( _moved )  z::throw_xc( "SCHEDULER-0", obj_name() + " _moved=true?" );

    _task       = task;
    if( !_start_time )  _start_time = now;      
    Z_DEBUG_ONLY( assert( !_setback ) );  // Schon in first_order() gelöscht
    _setback    = 0;

    if( _is_virgin  &&  _http_operation )  _http_operation->on_first_order_processing( task );
    _is_virgin = false;
    _is_virgin_in_this_run_time = false;

    return true;
}

//------------------------------------------------------------------Order::db_occupy_for_processing

bool Order::db_occupy_for_processing( Transaction* ta )
{
    assert( _job_chain == NULL );   // Der Auftrag kommt erst nach der Belegung in die Jobkette, deshalb ist _job_chain == NULL

    order_subsystem()->assert_is_sharing_orders_in_database( __FUNCTION__ );

    sql::Update_stmt update = db_update_stmt();

    update[ "processing_scheduler_member_id" ] = _spooler->scheduler_member_id();

    //update.and_where_condition( "spooler_id", _spooler->id_for_db() );
    //update.and_where_condition( "job_chain" , job_chain->name() );
    //update.and_where_condition( "id"        , id().as_string()    );
    update.and_where_condition( "state"     , state().as_string() );
    update.and_where_condition( "processing_scheduler_member_id", sql::null_value );
int COMMIT;
    bool update_ok = ta->try_execute_single( update, __FUNCTION__ );
    
    if( update_ok )  _is_db_occupied = true, _occupied_state = _state;
    else
    {
        update.remove_where_condition( "processing_scheduler_member_id" );
        db_show_occupation( ta, log_debug9 );
    }

    return _is_db_occupied;
}

//------------------------------------------------------------------------Order::db_show_occupation

void Order::db_show_occupation( Transaction* outer_transaction, Log_level log_level )
{
    for( Retry_transaction ta ( _spooler->_db, outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( S() << "select `processing_scheduler_member_id`"
            "  from " << _spooler->_orders_tablename
            << db_where_clause().where_string() );

        if( result_set.eof() )
        {
            _log->log( log_level, message_string( "SCHEDULER-812" ) );      int SCHEDULER_812;  // Die Fehlermeldung stimmt nicht immer
        }
        else
        {
            // ?Haben wir eine Satzsperre oder kann ein anderer Scheduler processing_scheduler_member_id schon wieder auf null gesetzt haben?
            Record record = result_set.get_record();
            _log->log( log_level, message_string( "SCHEDULER-813", record.as_string(0) ) );
        }

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x  ) ); }
}

//---------------------------------------------------------------------------------Order::db_insert

void Order::db_insert()
{
    string payload_string = payload().as_string();

    if( db()->opened() )
    for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
    {
        if( !db()->opened() )  break;

        int ordering = db_get_ordering( &ta );
        
        ta.execute( db_update_stmt().make_delete_stmt() );

        {
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

            insert.set_datetime( "at"          , _setback? _setback.as_string( Time::without_ms ) : database_null_at_date );
            insert.set_datetime( "created_time", _created.as_string(Time::without_ms) );
            insert.set_datetime( "mod_time"    , Time::now().as_string(Time::without_ms) );

            if( is_processable() )  insert[ "processable"   ] = true;

            ta.execute( insert, __FUNCTION__ );
        }

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
    catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-305", _spooler->_orders_tablename, x ) ); }

    tip_own_job_for_new_order_state();
}

//---------------------------------------------------------------------Order::db_release_occupation

void Order::db_release_occupation()
{
    if( _is_db_occupied  &&  _spooler->scheduler_member_id() != "" )
    {
        for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
        {
            sql::Update_stmt update = db_update_stmt();

            update[ "processing_scheduler_member_id" ] = sql::null_value;
            update.and_where_condition( "processing_scheduler_member_id", _spooler->scheduler_member_id() );
            update.and_where_condition( "state"                         , _occupied_state.as_string()     );

            bool update_ok = ta.try_execute_single( update, __FUNCTION__ );
            if( !update_ok ) 
            {
                _log->error( "Unable to release occupation in database" );
                db_show_occupation( &ta, log_error );
            }

            ta.commit( __FUNCTION__ );

            _is_db_occupied = false; 
        }
        catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x ) ); }
    }
}

//---------------------------------------------------------------------------------Order::db_update

void Order::db_update( Update_option update_option )
{
    if( _is_in_database &&  _spooler->_db->opened() )
    {
        if( update_option == update_not_occupated  &&  _is_db_occupied )  z::throw_xc( __FUNCTION__, "is_db_occupied" );


        string           state_string = state().as_string();    // Kann Exception auslösen;
        sql::Update_stmt update       = db_update_stmt();
        bool             update_ok    = false;


        update.and_where_condition( "processing_scheduler_member_id", _is_db_occupied? sql::Value( _spooler->scheduler_member_id() ) 
                                                                                     : sql::null_value                               );

        if( update_option == update_and_release_occupation )
        {
            update.and_where_condition( "state", _occupied_state.as_string() );
            update[ "processing_scheduler_member_id" ] = sql::null_value;
        }

        //if( _job_chain_name == "" )
        if( finished() )
        {
            for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
            {
                if( !_spooler->_db->opened() )  break;

                update_ok = ta.try_execute_single( update.make_delete_stmt(), __FUNCTION__ );

                db()->write_order_history( this, &ta );

                ta.commit( __FUNCTION__ );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x  ) ); }

            _is_in_database = false;  
            _is_db_occupied = false;
        }
        else
        {
            string payload_string = string_payload();   // Kann Exception auslösen

            update[ "state"         ] = state_string;
            update[ "initial_state" ] = initial_state().as_string();
            update[ "processable"   ] = is_processable()? sql::Value(true) : sql::null_value;
            update.set_datetime( "at"      , _setback? _setback.as_string( Time::without_ms ) : database_null_at_date );
            update.set_datetime( "mod_time", Time::now().as_string(Time::without_ms) );

            if( _priority_modified   )  update[ "priority"   ] = priority();
            if( _title_modified      )  update[ "title"      ] = title();
            if( _state_text_modified )  update[ "state_text" ] = state_text();

            for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
            {
                if( !_spooler->_db->opened() )  break;

                if( !finished() )
                {
                    // _run_time_modified gilt nicht für den Datenbanksatz, sondern für den Aufragsneustart
                    // Vorschlag: xxx_modified auflösen zugunsten eines gecachten letzten Datenbanksatzes, dessen Inhalt verglichen werden.
                    if( run_time() ) 
                    {
                        xml::Document_ptr doc = run_time()->dom_document();
                        if( doc.documentElement().hasAttributes()  ||  doc.documentElement().hasChildNodes() )  db_update_clob( &ta, "run_time", doc.xml() );
                                                                                                          else  update[ "run_time" ].set_direct( "null" );
                    }
                    else
                        update[ "run_time" ].set_direct( "null" );

                    if( _order_xml_modified )
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

                update_ok = ta.try_execute_single( update, __FUNCTION__ );

                ta.commit( __FUNCTION__ );
            }
            catch( exception& x ) { ta.reopen_database_after_error( z::Xc( "SCHEDULER-306", _spooler->_orders_tablename, x  ) ); }
        }

        if( update_option == update_and_release_occupation )
        {
            if( update_ok )  _is_db_occupied = false, _occupied_state = Variant();
            else
            if( _is_db_occupied )
            {
                //_log->error( "OCCUPATION LOST" );
                db_show_occupation( NULL, log_error );
            }
        }

        _order_xml_modified  = false;            
        _state_text_modified = false; 
        _title_modified      = false;
        _state_text_modified = false;

        tip_own_job_for_new_order_state();
    }
    else
    if( finished() )  
        _spooler->_db->write_order_history( this );
}

//------------------------------------------------------------------------------Order::db_read_clob

string Order::db_read_clob( Transaction* ta, const string& column_name )
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

Spooler_db* Order::db()
{
    return _spooler->_db;
}

//----------------------------------------------------------------------------------Order::open_log

void Order::open_log()
{
    if( _job_chain && _spooler->_order_history_with_log && !string_begins_with( _spooler->log_directory(), "*" ) )
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

void Order::close()
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
    if( _replaced_by )  _replaced_by->_replacement_for = NULL, _replaced_by = NULL;

    remove_from_job_chain();

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

    if( priority         != "" )  set_priority( as_int(priority) );
    if( id               != "" )  set_id      ( id.c_str() );
    if( title            != "" )  set_title   ( title );
    if( state_name       != "" )  set_state   ( state_name.c_str() );
    if( web_service_name != "" )  set_web_service( _spooler->_web_services.web_service_by_name( web_service_name ) );
    if( at_string        != "" )  set_at      ( Time::time_with_now( at_string ) );

    if( element.hasAttribute( "suspended" ) )
        set_suspended( element.bool_getAttribute( "suspended" ) );


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
            ptr<Com_variable_set> pars = new Com_variable_set;
            pars->set_dom( e, variable_set_map );  
            set_payload( Variant( static_cast<IDispatch*>( pars ) ) );
        }
        else
        if( e.nodeName_is( "payload" ) )
        {
            DOM_FOR_EACH_ELEMENT( e, ee )
            {
                if( ee.nodeName_is( "params"  ) )
                {
                    ptr<Com_variable_set> pars = new Com_variable_set;
                    pars->set_dom( ee, variable_set_map );  
                    set_payload( Variant( static_cast<IDispatch*>( pars ) ) );
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
    }
}

//-------------------------------------------------------------------------------Order::dom_element

xml::Element_ptr Order::dom_element( const xml::Document_ptr& document, const Show_what& show, const string* log ) const
{
    xml::Element_ptr element = document.createElement( "order" );

    if( show != show_for_database_only )
    {
        if( !_id.is_empty() )
        {
            element.setAttribute( "order"     , debug_string_from_variant( _id ) );
            element.setAttribute( "id"        , debug_string_from_variant( _id ) );     // veraltet
        }
    }

    if( show != show_for_database_only  &&  show != show_id_only )
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

        Job* job = this->job();
        if( job )
        element.setAttribute( "job"       , job->name() );

        if( _task )
        {
        element.setAttribute( "task"            , _task->id() );   // Kann nach set_state() noch die Vorgänger-Task sein (bis spooler_process endet)
      //element.setAttribute( "task"            , _task->obj_name() );   // Kann nach set_state() noch die Vorgänger-Task sein (bis spooler_process endet)
        element.setAttribute( "in_process_since", _task->last_process_start_time().as_string() );
        }

        if( _state_text != "" )
        element.setAttribute( "state_text", _state_text );

        element.setAttribute( "priority"  , _priority );

        if( _created )
        element.setAttribute( "created"   , _created.as_string() );

        if( _log->opened() )
        element.setAttribute( "log_file"  , _log->filename() );

        if( _setback  &&  _setback_count > 0 )
        element.setAttribute( "setback"   , _setback.as_string() );

        if( _replacement_for )
        element.setAttribute( "replacement", "yes" );

        if( show & show_payload  &&  !_payload.is_null_or_empty_string()  &&  !_payload.is_missing() )
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

                /*
                if( string_begins_with( payload_string, "<?xml" ) )
                {
                    try
                    {
                        xml::Document_ptr doc ( payload_string );
                        payload_content = document.clone( doc.documentElement() );
                    }
                    catch( exception& x )
                    {
                        Z_LOG2( "scheduler", obj_name() << ".payload enthält fehlerhaftes XML: " << x.what() );
                    }
                }
                */

                if( !payload_content )  payload_content = document.createTextNode( payload_string );
            }

            payload_element.appendChild( payload_content );
        }

        if( show & show_run_time )  element.appendChild( _run_time->dom_element( document ) );

        if( log  &&  show & show_log ) element.append_new_text_element( "log", *log );     // Protokoll aus der Datenbank
        else
        if( _log )  element.appendChild( _log->dom_element( document, show ) );
    }

    if( show & ( show_payload | show_for_database_only )  &&  _xml_payload != "" )
    {
        xml::Element_ptr xml_payload_element = element.append_new_element( "xml_payload" );

        try
        {
            xml::Document_ptr doc ( _xml_payload );

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

    if( _setback && _setback_count == 0 )
    element.setAttribute( "at"        , _setback.as_string() );

    if( _web_service )
    element.setAttribute( "web_service", _web_service->name() );

    if( _http_operation  &&  _http_operation->web_service_operation_or_null() )
    element.setAttribute( "web_service_operation", _http_operation->web_service_operation_or_null()->id() );

    if( _suspended )
    element.setAttribute( "suspended", "yes" );

    return element;
}

//--------------------------------------------------------Order::print_xml_child_elements_for_event

void Order::print_xml_child_elements_for_event( String_stream* s, Scheduler_event* )
{
    *s << "<order";
    
    if( _job_chain )
    *s << " job_chain=\"" << xml_encode_attribute_value( _job_chain_name )      << '"';
    *s << " id=\""        << xml_encode_attribute_value( string_id() )          << '"';

    if( _title != "" )
    *s << " title=\""     << xml_encode_attribute_value( _title )               << '"';

    *s << " state=\""     << xml_encode_attribute_value( _state.as_string() )   << '"';

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
    set_job( _spooler->get_job( jobname ) );
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
        _is_users_id = true;

        //_log->set_prefix( "Order " + _id.as_string() );
        _log->set_prefix( obj_name() );
        _log->set_title ( "Order " + _id.as_string() );
    }
}

//----------------------------------------------------------------------------Order::set_default_id

void Order::set_default_id()
{
    set_id( _spooler->_db->get_order_id() );
    _is_users_id = false;
}

//-----------------------------------------------------------------------------Order::set_file_path

void Order::set_file_path( const File_path& path )
{
    string p = path.path();

    set_id( p );
    set_param( scheduler_file_path_variable_name, p );
}

//---------------------------------------------------------------------------------Order::file_path

File_path Order::file_path() const
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

bool Order::is_file_order() const
{
    return file_path() != "";
}

//----------------------------------------------------------------------------Order::string_payload

string Order::string_payload() const
{
    try
    {
        return !_payload.is_null_or_empty_string()  &&  !_payload.is_missing()? _payload.as_string() 
                                                                              : "";
    }
    catch( exception& x )
    {
        z::throw_xc( "SCHEDULER-251", x.what() );
    }
}

//----------------------------------------------------------------------------Order::params_or_null

ptr<Com_variable_set> Order::params_or_null() const
{
    ptr<spooler_com::Ivariable_set> result;

    if( _payload.vt != VT_DISPATCH  &&  _payload.vt != VT_UNKNOWN )  return NULL;
    
    IUnknown* iunknown = V_UNKNOWN( &_payload );
    if( iunknown == NULL )  return NULL;

    HRESULT hr = iunknown->QueryInterface( spooler_com::IID_Ivariable_set, result.void_pp() );
    if( FAILED(hr) )  return NULL;

    return dynamic_cast<Com_variable_set*>( +result );
}

//------------------------------------------------------------------------------------Order::params

ptr<Com_variable_set> Order::params() const
{
    ptr<Com_variable_set> result = params_or_null();
    if( !result )  z::throw_xc( "SCHEDULER-338" );
    return result;
}

//---------------------------------------------------------------------------------Order::set_param

void Order::set_param( const string& name, const Variant& value )
{
    HRESULT hr;

    if( _payload.is_empty() )  _payload = new Com_variable_set();

    Variant name_vt = variant_from_string( name );
    hr = params()->put_Value( &name_vt, const_cast<Variant*>( &value ) );
    if( FAILED(hr) )  throw_com( hr, __FUNCTION__, name );
}

//-------------------------------------------------------------------------------------Order::param

Variant Order::param( const string& name ) const
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

void Order::set_web_service( const string& name )
{ 
    if( _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );

    _order_xml_modified = true;

    set_web_service( name == ""? NULL 
                               : _spooler->_web_services.web_service_by_name( name ) );
}

//---------------------------------------------------------------------------Order::set_web_service

void Order::set_web_service( Web_service* web_service )                
{ 
    if( _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );

    _web_service = web_service; 
    _order_xml_modified = true;
}

//-----------------------------------------------------------------------------------Order::set_job

void Order::set_job( Job* job )
{
    //THREAD_LOCK( _lock )
    {
        if( _removed_from_job_chain_name != "" )
        {
            _log->warn( message_string( "SCHEDULER-298", job->name() ) );   //S() << "job=" << job->name() << " wird ignoriert, weil Auftrag bereits aus der Jobkette entfernt ist" );
            return;
        }

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
        Z_LOG2( "scheduler.order", obj_name() << ".payload=" << debug_string_from_variant(payload) << "\n" );
        _payload = payload;
        //_payload_modified = true;
    }
}

//---------------------------------------------------------------------------Order::set_xml_payload

void Order::set_xml_payload( const string& xml_string )
{ 
    Z_LOGI2( "scheduler.order", obj_name() << ".xml_payload=" << xml_string << "\n" );
    
    xml::Document_ptr doc ( xml_string );       // Sicherstellen, dass xml_string valide ist

    _xml_payload = xml_string;  
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
    return _end_state_reached  ||  !_job_chain_node  ||  !_job_chain_node->_job;
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

    if( state != _state )
    {
        if( _job_chain )  move_to_node( _job_chain->node_from_state( state ) );
                    else  set_state2( state );
    }

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

    if( node  &&  node->_suspend )  set_suspended();

    set_state2( node? node->_state : empty_variant, is_error_state );
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Job_chain_node* node )
{
    order_subsystem()->assert_not_sharing_orders_in_database( __FUNCTION__ );

    if( _on_blacklist )  _job_chain->remove_from_blacklist( this );

    //THREAD_LOCK( _lock )
    {
        if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );

        if( _task )  _moved = true;
        //§1495  _task = NULL;

        if( _job_chain_node && _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this ), _job_chain_node = NULL;

        set_job_chain_node( node );
        clear_setback();

        if( node && node->_job )  node->_job->order_queue()->add_order( this );
    }
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
    order_subsystem()->assert_not_sharing_orders_in_database( __FUNCTION__ );

    //THREAD_LOCK( _lock )
    {
        ptr<Order_queue> order_queue = _spooler->get_job( job_name )->order_queue();
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
    order_subsystem()->assert_not_sharing_orders_in_database( __FUNCTION__ );

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
        if( _is_in_database )  db_update( update_and_release_occupation );

        _log->info( _task? message_string( "SCHEDULER-941", _task->obj_name() ) 
                         : message_string( "SCHEDULER-940" ) );

    }

    if( _job_chain )  _job_chain->remove_order( this );

    _setback_count = 0;
    _setback = Time(0);

    if( _replacement_for )  _replacement_for->_replaced_by = NULL,  _replacement_for = NULL;

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

bool Order::try_place_in_job_chain( Job_chain* job_chain)
{
    bool result = false;

    if( !job_chain->has_order_id( id() ) )
    {
        ptr<Order> hold_me = this;   // Halten für wegen remove_from_job_chain()
        
        if( _job_chain_name != "" )  remove_from_job_chain();
        
        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        if( _state.vt == VT_EMPTY )  set_state2( job_chain->first_node()->_state );     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

        Job* job = job_chain->job_from_state( _state );     // Fehler bei Endzustand. Wir speichern den Auftrag nur, wenn's einen Job zum Zustand gibt

        set_setback( _state == _initial_state  &&  !_setback  &&  _run_time->set()? next_start_time( true ) : _setback );

        if( order_subsystem()->is_sharing_orders_in_database() )
        {
            _job_chain_name = job_chain->name();
            _removed_from_job_chain_name = "";
        }
        else
        {
            job_chain->add_order( this );
        }

        if( !_delay_storing_until_processing  &&  job_chain->_orders_recoverable  &&  !_is_in_database )
        {
            db_insert();
        }

        result = true;
    }

    return result;
}

//-------------------------------------------------------------Order::place_or_replace_in_job_chain

void Order::place_or_replace_in_job_chain( Job_chain* job_chain )
{
    order_subsystem()->assert_not_sharing_orders_in_database( __FUNCTION__ );

    if( ptr<Order> other_order = job_chain->order_or_null( id() ) )   // Nicht aus der Datenbank gelesen
    {
        other_order->remove_from_job_chain();
        place_in_job_chain( job_chain );

        if( other_order->_task )
        {
            _replacement_for = other_order;
            _replacement_for->_replaced_by = this;

            _log->info( message_string( "SCHEDULER-942", other_order->_task->obj_name(), other_order->obj_name() ) );       // add_or_replace_order(): Auftrag wird verzögert bis <p1/> <p2/> ausgeführt hat
        }
    }
    else
    {
        place_in_job_chain( job_chain );
    }
}

//-----------------------------------------------------------Order::tip_own_job_for_new_order_state

bool Order::tip_own_job_for_new_order_state()
{
    bool result = false;

    if( is_processable() )
    {
        Job_chain_node* node = job_chain()->node_from_state( _state );

        if( Job* job = node->_job )
        {
            if( job->order_queue()->is_order_requested() 
             && at() < job->order_queue()->next_announced_order_time() )
            {
                job->order_queue()->set_next_announced_order_time( at(), at() <= Time::now() );
                result = true;
            }
        }
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
            _removed_from_job_chain_name != ""? order_subsystem()->job_chain_or_null( _removed_from_job_chain_name ) 
                                              : NULL; 
}

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( bool success )
{
    Job* last_job = _task? _task->job() : NULL;

    //THREAD_LOCK( _lock )
    {
        bool force_error_state = false;

        if( !_suspended  &&  _setback == Time::never  &&  _setback_count > _task->job()->max_order_setbacks() )
        {
            _log->info( message_string( "SCHEDULER-943", _setback_count ) );   // " mal zurückgestellt. Der Auftrag wechselt in den Fehlerzustand"
            success = false;
            force_error_state = true;
            _setback = Time(0);
            _setback_count = 0;
        }

        _task = NULL;



        if( !is_setback() && !_moved && !_end_state_reached  ||  force_error_state )
        {
            //_setback_count = 0;

            if( _job_chain_node )
            {
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
                    if( !order_subsystem()->is_sharing_orders_in_database() )
                    {
                        _job_chain_node->_job->order_queue()->add_order( this );
                    }
                }
                else
                {
                    // Endzustand erreicht

                    bool is_first_call = _run_time_modified;
                    _run_time_modified = false;
                    Time next_start = next_start_time( is_first_call );

                    if( next_start != Time::never )
                    {
                        _log->info( message_string( "SCHEDULER-944", _initial_state, next_start ) );        // "Kein weiterer Job in der Jobkette, der Auftrag wird mit state=<p1/> wiederholt um <p2/>"

                        _end_time = Time::now();
                        _log->close_file();
                        if( _job_chain  &&  _is_in_database )  _spooler->_db->write_order_history( this );  // Historie schreiben, aber Auftrag beibehalten
                        _log->close();

                        _start_time = 0;
                        _end_time = 0;
                        _is_virgin_in_this_run_time = true;

                        open_log();

                        try
                        {
                            set_state( _initial_state, next_start );
                        }
                        catch( exception& x )
                        {
                            _log->error( x.what() );
                        }
                    }
                    else
                        _log->info( message_string( "SCHEDULER-945" ) );     // "Kein weiterer Job in der Jobkette, der Auftrag ist erledigt"
                }
            }
            else
            if( _order_queue )
            {
                _order_queue->remove_order( this );
                _order_queue = NULL;
            }
        }

        postprocessing2( last_job );
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


    if( end_state_reached()  &&  _suspended )
    {
        add_to_blacklist();
    }

    if( finished() )
    {
        if( is_file_order()  &&  file_exists( file_path() ) )
        {
            _log->error( message_string( "SCHEDULER-340" ) );
            add_to_blacklist();
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


    if( finished() )
    {
        _end_time = Time::now();
    }

    if( _job_chain  &&  ( _is_in_database || finished() ) )
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

    if( _job_chain  &&  order_subsystem()->is_sharing_orders_in_database() )
    {
        _job_chain->remove_order( this );
    }


    if( finished()  &&  !_on_blacklist )  close();
}

//-----------------------------------------------------------------------------Order::set_suspended

void Order::set_suspended( bool suspended )
{
    if( _suspended != suspended )
    {
        _suspended = suspended;
        _order_xml_modified = true;

        if( _on_blacklist  &&  !_suspended )  remove_from_job_chain();
        else
        if( _in_job_queue )  order_queue()->reinsert_order( this );

        if( _suspended )  _log->info( message_string( "SCHEDULER-991" ) );
                    else  _log->info( message_string( "SCHEDULER-992", _setback ) );
    }
}

//--------------------------------------------------------------------------Order::add_to_blacklist

void Order::add_to_blacklist()
{ 
    order_subsystem()->assert_not_sharing_orders_in_database( __FUNCTION__ );

    assert( _job_chain );
    if( _in_job_queue )  remove_from_job();

    _job_chain->add_to_blacklist( this ); 
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
    assert_no_task();
    if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_name );

    set_setback( time );
}

//--------------------------------------------------------------------------------Order::next_time

Time Order::next_time()
{
    if( _suspended )  return Time::never;
    return _setback;
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

//--------------------------------------------------------------Order::before_modify_run_time_event

void Order::before_modify_run_time_event()
{
  //if( _task       )  z::throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
  //if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain_name );
}

//-------------------------------------------------------------------Order::run_time_modified_event

void Order::run_time_modified_event()
{
    if( _is_virgin_in_this_run_time )  set_setback( _run_time->set()? next_start_time( true ) : Time(0) );
                                else  _run_time_modified = true;
}

//------------------------------------------------------------------------------Order::set_run_time

void Order::set_run_time( const xml::Element_ptr& e )
{
    _run_time = Z_NEW( Run_time( _spooler, this ) );
    _run_time->set_modified_event_handler( this );

    if( e )  _run_time->set_dom( e );       // Ruft set_setback() über modify_event()
       else  run_time_modified_event();
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
