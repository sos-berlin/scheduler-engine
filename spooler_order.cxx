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

using stdext::hash_set;
using stdext::hash_map;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

const string default_end_state_name            = "<END_STATE>";

//-------------------------------------------------------------------------Spooler::init_job_chains

void Spooler::init_job_chains()
{
    init_file_order_sink();
}

//---------------------------------------------------------------------------Spooler::add_job_chain

void Spooler::add_job_chain( Job_chain* job_chain )
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

        job_chain->load_orders_from_database();
        job_chain->_order_sources.start();
    }
    catch( exception&x )
    {
        _log.error( x.what() );
        throw;
    }
}

//------------------------------------------------------------------------Spooler::job_chain_or_null

Job_chain* Spooler::job_chain_or_null( const string& name )
{
    Job_chain* result = NULL;

    //THREAD_LOCK( _job_chain_lock )
    {
        string lname = lcase( name );

        Job_chain_map::iterator it = _job_chain_map.find( lname );
        if( it == _job_chain_map.end() )  result = NULL;
                                    else  result = it->second;
    }

    return result;
}

//-------------------------------------------------------------------------------Spooler::job_chain

Job_chain* Spooler::job_chain( const string& name )
{
    Job_chain* result = job_chain_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-161", name );

    return result;
}

//------------------------------------------------------------------Spooler::job_chains_dom_element

xml::Element_ptr Spooler::job_chains_dom_element( const xml::Document_ptr& document, const Show_what& show )
{
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
    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->close();
    }
}

//----------------------------------------------------------------------------Order_sources::finish

void Order_sources::finish()
{
    Z_FOR_EACH( Order_source_list, _order_source_list, it )
    {
        Order_source* order_source = *it;
        order_source->finish();
    }
}

//-----------------------------------------------------------------------------Order_sources::start

void Order_sources::start()
{
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
    return _job? _job->order_queue()->order_count( job_chain ) : 0;
}

//-----------------------------------------------------------------------------Job_chain::Job_chain

Job_chain::Job_chain( Spooler* spooler )
:
    Com_job_chain( this ),
    Scheduler_object( spooler, static_cast<spooler_com::Ijob_chain*>( this ), type_job_chain ),
    _zero_(this+1),
    _lock("Job_chain"),
    _log(_spooler),
    _orders_recoverable(true),
    _visible(true)
{
    set_name( "" );     // Ruft _log.set_prefix()
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
    Z_LOGI2( "scheduler", *this << ".close()\n" );
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

            add_job( job, state, e.getAttribute( "next_state" ), e.getAttribute( "error_state" ) );
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
                Any_file sel ( "-in " + _spooler->_db->db_name() + // "-max-length=32K  "
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

//-------------------------------------------------------------Job_chain::load_orders_from_database

void Job_chain::load_orders_from_database()
{
    if( !_orders_recoverable )  return;
    
    if( !_spooler->_db  ||  !_spooler->_db->opened() )  
    {
        _load_orders_from_database = true;
        return;
    }

    _load_orders_from_database = false;


    int count = 0;

    {
        Transaction ta ( _spooler->_db );

        Any_file sel ( "-in " + _spooler->_db->db_name() + 
                    " select \"ID\", \"PRIORITY\", \"STATE\", \"STATE_TEXT\", \"TITLE\", \"CREATED_TIME\", \"INITIAL_STATE\""
                    " from " + _spooler->_orders_tablename +
                    " where \"SPOOLER_ID\"=" + sql::quoted(_spooler->id_for_db()) +
                      " and \"JOB_CHAIN\"="  + sql::quoted(_name) +
                    " order by \"ORDERING\"" );

        while( !sel.eof() )
        {
            Record record = sel.get_record();
            string order_id = record.as_string( "id" );

            try
            {
                ptr<Order> order = new Order( _spooler, record, 
                                              _spooler->_db->read_orders_clob( _name, order_id, "payload"   ), 
                                              _spooler->_db->read_orders_clob( _name, order_id, "run_time"  ),
                                              _spooler->_db->read_orders_clob( _name, order_id, "order_xml" ) );
                order->_is_in_database = true;
                order->add_to_job_chain( this );    // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
                count++;
            }
            catch( exception& x )
            {
                _log.error( message_string( "SCHEDULER-295", order_id, x ) ); 
            }
        }
    }

    _log.debug( message_string( "SCHEDULER-935", count ) );
}

//----------------------------------------------------------------------------Job_chain::first_node

Job_chain_node* Job_chain::first_node()
{
    if( _chain.empty() )  z::throw_xc( __FUNCTION__ );
    return *_chain.begin();
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
            order->remove_from_job_chain( leave_in_database );
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

//-------------------------------------------------------------------------------Job_chain::add_job

Job_chain_node* Job_chain::add_job( Job* job, const Order::State& state, const Order::State& next_state, const Order::State& error_state )
{
    Order::check_state( state );
    if( !next_state.is_missing() )  Order::check_state( next_state );
    if( !error_state.is_missing() )  Order::check_state( error_state );

    if( job  &&  !job->order_queue() )  z::throw_xc( "SCHEDULER-147", job->name() );

    if( _state != s_under_construction )  z::throw_xc( "SCHEDULER-148" );

    ptr<Job_chain_node> node = new Job_chain_node;

    node->_job   = job;
    node->_state = state;

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

    /*
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job* job = (*it)->_job;
            if( job )
            {
                ptr<Order> result = job->order_queue()->order_or_null( id );
                if( result )  return result;
            }
        }
    */
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

    //THREAD_LOCK( _lock )
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
                                else  Z_LOG2( "scheduler", __FUNCTION__ << " " << order->obj_name() << " ist nicht registriert!?\n" );
    }
}

//----------------------------------------------------------------------Job_chain::add_to_blacklist

void Job_chain::add_to_blacklist( Order* order )
{
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

        for( Spooler::Job_chain_map::iterator j = _spooler->_job_chain_map.begin(); j != _spooler->_job_chain_map.end(); j++ )
        {
            if( j->second == this )  
            { 
                _spooler->_job_chain_map.erase( j );  
                _spooler->_job_chain_map_version++;
                break; 
            }
        }

        Z_FOR_EACH( Order_sources::Order_source_list, _order_sources._order_source_list, it )
        {
            Order_source* order_source = *it;
            order_source->close();
        }
    }
}

//--------------------------------------------------------------------Job_chain::check_for_removing

void Job_chain::check_for_removing()
{
    if( state() == s_removing &&  !has_order() )
    {
        _log.info( message_string( "SCHEDULER-936" ) );     // "Removing"
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
    _lock("Order_queue")
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


    Queue* queues[] = { &_queue, &_setback_queue };
    for( Queue** q = queues; q < queues + NO_OF(queues); q++ )
    {
        for( Queue::iterator it = (*q)->begin(); it != (*q)->end(); it = (*q)->erase( it ) )
        {
            Order* order = *it;
            _log->info( message_string( "SCHEDULER-937", order->obj_name() ) );
        }
    }

    update_priorities();
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

            Queue* queues[] = { &_queue, &_setback_queue };
            for( Queue** q = queues; q < queues + NO_OF(queues); q++ )
            {
                if( limit <= 0 ) break;

                FOR_EACH( Queue, **q, it )
                {
                    Order* order = *it;
                    if( !which_job_chain  ||  order->job_chain() == which_job_chain )
                    {
                        dom_append_nl( element );
                        element.appendChild( order->dom_element( document, show ) );
                        if( --limit == 0 )  break;
                    }
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
    if( which_job_chain )
    {
        int count = 0;

        //THREAD_LOCK( _lock )
        {
            FOR_EACH( Queue, _queue        , it )  if( (*it)->_job_chain == which_job_chain )  count++;
            FOR_EACH( Queue, _setback_queue, it )  if( (*it)->_job_chain == which_job_chain )  count++;
        }

        return count;
    }
    else
    {
        return _queue.size() + _setback_queue.size();
    }
}

//---------------------------------------------------------------------------Order_queue::add_order

void Order_queue::add_order( Order* order, Do_log do_log )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

#   ifdef Z_DEBUG
        Z_FOR_EACH( Queue, _queue        , it )  assert( *it != order );
        Z_FOR_EACH( Queue, _setback_queue, it )  assert( *it != order );
#   endif

    _job->set_visible( true );

    //THREAD_LOCK( _lock )
    {
        if( order->_setback )
        {
            if( order->_setback < latter_day )  order->_log->log( do_log? log_info : log_debug3, message_string( "SCHEDULER-938", order->_setback ) );
                                          else  order->_log->log( do_log? log_warn : log_debug3, message_string( "SCHEDULER-296" ) );       // "Die <run_time> des Auftrags hat keine nächste Startzeit" );

          //_log->debug( "add_order (setback queue) " + order->obj_name() );

            // Auftrag nach Rückstellungszeitpunkt (und Priorität) geordnet in die _setback_queue einfügen:

            Queue::iterator it;
            for( it = _setback_queue.begin(); it != _setback_queue.end(); it++ )
            {
                Order* o = *it;
                if( o->_setback > order->_setback )  break;
                if( o->_setback == order->_setback  &&  o->_priority > order->_priority )  break;
            }

            _setback_queue.insert( it, order );

            _job->calculate_next_time_after_modified_order_queue();
        }
        else
        {
            _log->debug( "add_order " + order->obj_name() );
/*
            Id_map::iterator id_it = _id_map.find( order->_id );
            if( id_it != _id_map.end() )
            {
                _log->debug( "Auftrag mit gleicher Id wird ersetzt: " + order->obj_name() );
                _queue.erase( id_it->second );
                _id_map.erase( id_it );
            }
*/
            Queue::iterator ins       = _queue.end();
            bool            ins_set   = false;
            bool            wake_up   = !order->_task  &&  !has_order( Time::now() );  //_queue.empty();
          //bool            id_found  = false;

            //_has_users_id |= order->_is_users_id;

            if( /*_has_users_id  ||*/  order->priority() > _lowest_priority  &&  order->priority() <= _highest_priority )     // Optimierung
            {
                for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
                {
                    Order* o = *it;
                    if( !ins_set  &&  order->priority() > o->priority() )
                    {
                        ins = it;
                        ins_set = true;
                        //if( id_found )
                            break;
                    }

                    //if( !id_found  &&  o->id_is_equal( order->_id ) )
                    //{
                    //    _log->debug( message_string( "SCHEDULER-939", order->obj_name() ) );      // "Auftrag mit gleicher Id wird ersetzt: " 
                    //    if( ins == it )  { ins = _queue.erase( it ); break; }
                    //               else  it = _queue.erase( it );    Nachfolgendes it++ ist falsch
                    //    id_found = true;
                    //}
                }
            }

            if( ins_set )                                 _queue.insert( ins, order );
            else
            if( order->priority() > _highest_priority )   _queue.push_front( order );
                                                    else  _queue.push_back( order );

            update_priorities();

            if( wake_up )  _job->signal( "Order" );
        }

        order->_in_job_queue = true;
    }
}

//------------------------------------------------------------------------Order_queue::remove_order

void Order_queue::remove_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    //THREAD_LOCK( _lock )
    {
        if( order->_setback )
        {
            _log->debug9( "remove_order (setback) " + order->obj_name() );

            Queue::iterator it;
            for( it = _setback_queue.begin(); it != _setback_queue.end(); it++ )  if( *it == order )  break;

            if( it == _setback_queue.end() )  z::throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

            order->_setback = 0;
            order->_in_job_queue = false;

            _setback_queue.erase( it );
            order = NULL;  // order ist jetzt möglicherweise ungültig
        }
        else
        {
            _log->debug9( "remove_order " + order->obj_name() );

            Queue::iterator it;
            for( it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

            if( it == _queue.end() )  z::throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

            order->_in_job_queue = false;

            _queue.erase( it );
            order = NULL;  // order ist jetzt möglicherweise ungültig
            //_id_map.erase( order->_id );
            update_priorities();
        }
    }
}

//-------------------------------------------------------------------Order_queue::update_priorities

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

//-------------------------------------------------------------------------Order_queue::first_order

Order* Order_queue::first_order( const Time& now )
{
    // now kann 0 sein, dann werden nur Aufträge ohne Startzeit beachtet

    // SEITENEFFEKT: Aufträge aus der _setback_queue, deren Rückstellungszeitpunkt erreicht ist, werden in die _queue verschoben.

    //THREAD_LOCK( _lock )
    {
        // Zurückgestellte Aufträge, deren Wartezeit abgelaufen ist, hervorholen


        if( now > 0 )
        {
            while( !_setback_queue.empty() )
            {
                ptr<Order> o = *_setback_queue.begin();
                if( o->_setback > now )  break;

                remove_order( o );
                o->_setback = 0;
                add_order( o );
            }
        }

        FOR_EACH( Queue, _queue, o )
        {
            Order* order = *o;
            if( !order->is_immediately_processable( now ) )  continue;
            return order;
        }
    }

    return NULL;
}

//------------------------------------------------------------Order_queue::get_order_for_processing

ptr<Order> Order_queue::get_order_for_processing( const Time& now )
{
    // Die Order_queue gehört genau einem Job. Der Job kann zur selben Zeit nur einen Schritt ausführen.
    // Deshalb kann nur der erste Auftrag in Verarbeitung sein.

    ptr<Order> order;

    while(1)
    {
        order = first_order( now );
        if( !order )  break;

        if( !order->is_virgin()     )  break;
        if( !order->is_file_order() )  break;
        if( !order->job_chain()     )  break;
        if( file_exists( order->file_path() ) )  break;
        
        order->log()->info( message_string( "SCHEDULER-982" ) );  // Datei ist entfernt worden
        order->remove_from_job_chain();
    }

    if( order )
    {
        order->_start_time = now;
        order->_setback = 0;
        if( order->_moved )  z::throw_xc( "SCHEDULER-0", order->obj_name() + " _moved=true?" );
    }

    return order;
}

//---------------------------------------------------------------------------Order_queue::next_time

Time Order_queue::next_time()
{
    //THREAD_LOCK( _lock )
    {
        Order* o = first_order( 0 );
        if( o )  return o->_setback;

        //if( !_queue.empty() )  return 0;    //2004-02-25: latter_day;
        if( !_setback_queue.empty() )  return (*_setback_queue.begin())->_setback;
    }

    return latter_day;
}

//-----------------------------------------------------------------------Order_queue::order_or_null

ptr<Order> Order_queue::order_or_null( const Order::Id& id )
{
    //THREAD_LOCK( _lock )
    {
        FOR_EACH( Queue, _queue        , it )  if( (*it)->_id == id )  return *it;
        FOR_EACH( Queue, _setback_queue, it )  if( (*it)->_id == id )  return *it;
    }

    return NULL;
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler )
:
    Com_order(this),
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    _zero_(this+1),
    _lock("Order")
{
    init();
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const VARIANT& payload )
:
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    Com_order(this),
    _zero_(this+1),
    _lock("Order"),
    _payload(payload)
{
    init();
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const Record& record, const string& payload_string, const string& run_time_xml, const string& xml )
:
    Com_order(this),
    Scheduler_object( spooler, static_cast<IDispatch*>( this ), type_order ),
    _zero_(this+1),
    _lock("Order")
{

    init();

    set_id      ( record.as_string( "id"         ) );
    _state      = record.as_string( "state"      );
    _state_text = record.as_string( "state_text" );
    _title      = record.as_string( "title"      );
    _priority   = record.as_int   ( "priority"   );

    if( payload_string.find( "<" + Com_variable_set::xml_element_name() ) != string::npos )
    {
        //Z_LOG2( "scheduler", "... payload ist ein Variable_set!\n" );
        ptr<Com_variable_set> v = new Com_variable_set;
        v->put_Xml( Bstr( payload_string ) );
        _payload = v;
    }
    else
    {
        if( payload_string.empty() )  _payload = (IDispatch*)NULL;
                                else  _payload = payload_string;
    }

    _initial_state = record.as_string( "initial_state" );

    _created.set_datetime( record.as_string( "created_time" ) );

    _log->set_prefix( obj_name() );

    //string run_time_xml = record.as_string( "run_time" );
    if( run_time_xml != "" )  set_run_time( xml::Document_ptr( run_time_xml ).documentElement() );

    if( xml != "" )  set_dom( xml::Document_ptr( xml ).documentElement() );
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
    if( _http_operation )  _http_operation->unlink_order();
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    //_recoverable = true;

    _log = Z_NEW( Prefix_log( this ) );
    _log->set_prefix( obj_name() );
    _created = Time::now();
    _is_virgin = true;

    set_run_time( NULL );
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
    if( _on_blacklist )     return false;  
    if( _setback > now )    return false;
    if( _task )             return false;               // Schon in Verarbeitung
    if( _replacement_for )  return false;
    if( _job_chain  &&  _job_chain->state() != Job_chain::s_ready )  return false;   // Jobkette wird nicht gelöscht?

    return true;
}

//-------------------------------------------------------------------------------Order::attach_task

void Order::attach_task( Task* task )
{
    assert( task );
    assert_no_task();   // Vorsichtshalber


    if( !_log->opened() )  open_log();

    if( _delay_storing_until_processing  &&  _job_chain  &&  _job_chain->_orders_recoverable  &&  !_is_in_database )
    {
        _spooler->_db->insert_order( this );
        _delay_storing_until_processing = false;
    }

    _task = task;

    if( _is_virgin  &&  _http_operation )  _http_operation->on_first_order_processing( task );

    _is_virgin = false;
}

//----------------------------------------------------------------------------Order::assert_no_task

void Order::assert_no_task()
{
    if( _task )  z::throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
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

        _log->set_filename( _spooler->log_directory() + "/order." + _job_chain->name() + "." + name + ".log" );      // Jobprotokoll
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
    _removed_from_job_chain = NULL;
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
    string setback_string   = element.getAttribute( "at" );
    //if( setback_string == "" )  
    //    setback_string      = element.getAttribute( "setback" );        // So kommt's aus der Datenbank, siehe dom_element()


    if( priority         != "" )  set_priority( as_int(priority) );
    if( id               != "" )  set_id      ( id.c_str() );
    if( title            != "" )  set_title   ( title );
    if( state_name       != "" )  set_state   ( state_name.c_str() );
    if( web_service_name != "" )  set_web_service( _spooler->_web_services.web_service_by_name( web_service_name ) );
    if( setback_string   != "" )  setback( Time::time_with_now( setback_string ) );


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

        if( Job_chain* job_chain = this->job_chain() )
        element.setAttribute( "job_chain" , job_chain->name() );

        if( _replaced_by )
        element.setAttribute( "replaced"  , "yes" );
        else
        if( _removed_from_job_chain )
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

        element.appendChild( _log->dom_element( document, show ) );
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

    if( _setback && _setback_count == 0 )
    element.setAttribute( "at"        , _setback.as_string() );

    if( _web_service )
    element.setAttribute( "web_service", _web_service->name() );

    if( _http_operation  &&  _http_operation->web_service_operation_or_null() )
    element.setAttribute( "web_service_operation", _http_operation->web_service_operation_or_null()->id() );

    return element;
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

        string_id();    // Sicherstellen, das id in einen String wandelbar ist

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
        if( ptr<spooler_com::Ivariable_set> order_params = params_or_null() )
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

ptr<spooler_com::Ivariable_set> Order::params_or_null() const
{
    ptr<spooler_com::Ivariable_set> result;

    if( _payload.vt != VT_DISPATCH  &&  _payload.vt != VT_UNKNOWN )  return NULL;
    
    IUnknown* iunknown = V_UNKNOWN( &_payload );

    HRESULT hr = iunknown->QueryInterface( spooler_com::IID_Ivariable_set, result.void_pp() );
    if( FAILED(hr) )  return NULL;

    return result;
}

//------------------------------------------------------------------------------------Order::params

ptr<spooler_com::Ivariable_set> Order::params() const
{
    ptr<spooler_com::Ivariable_set> result = params_or_null();
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

    if( ptr<spooler_com::Ivariable_set> params = params_or_null() )  
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
}

//-----------------------------------------------------------------------------------Order::set_job

void Order::set_job( Job* job )
{
    //THREAD_LOCK( _lock )
    {
        if( _removed_from_job_chain )
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
    //if( _on_blacklist )  return false;
    return _finished  ||  !_job_chain_node  ||  !_job_chain_node->_job;
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

    _setback_count = 0;
}

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state, const Time& start_time )
{
    set_state( state );
    setback( start_time );
}

//--------------------------------------------------------------------------------Order::set_state2

void Order::set_state2( const State& state, bool is_error_state )
{
    if( _job_chain )
    {
        string log_line = "set_state " + state.as_string();

        if( _job_chain_node && _job_chain_node->_job )  log_line += ", " + _job_chain_node->_job->obj_name();
        if( is_error_state                           )  log_line += ", error state";

        if( _setback )  log_line += ", at=" + _setback.as_string();

        _log->info( log_line );
    }

    _state = state;

    if( !_initial_state_set )  _initial_state = state,  _initial_state_set = true;
}

//------------------------------------------------------------------------------Order::set_priority

void Order::set_priority( Priority priority )
{
    //THREAD_LOCK( _lock )
    {
        if( _priority != priority )
        {
            _priority = priority;

            if( !_setback  &&  _in_job_queue  &&  !_task )   // Nicht gerade in Verarbeitung?
            {
                ptr<Order> hold_me = this;
                order_queue()->remove_order( this );
                order_queue()->add_order( this );
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

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_job( const string& job_name )
{
    //THREAD_LOCK( _lock )
    {
        ptr<Order_queue> order_queue = _spooler->get_job( job_name )->order_queue();
        if( !order_queue )  z::throw_xc( "SCHEDULER-147", job_name );
        add_to_order_queue( order_queue );
    }
}

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_order_queue( Order_queue* order_queue )
{
    if( !order_queue )  z::throw_xc( "SCHEDULER-147", "?" );

    ptr<Order> me = this;   // Halten

    //THREAD_LOCK( _lock )
    {
        if( _task )  _moved = true;

        if( _job_chain )  remove_from_job_chain();
        _removed_from_job_chain = NULL;

        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        order_queue->add_order( this );
        _order_queue = order_queue;
    }
}

//---------------------------------------------------------------------Order::remove_from_job_chain

void Order::remove_from_job_chain( bool leave_in_database )
{
    ptr<Order> me        = this;        // Halten
    Job_chain* job_chain = _job_chain;

    if( !_end_time )  _end_time = Time::now();

    if( _is_in_database  &&  !leave_in_database )  _spooler->_db->finish_order( this );



    if( _job_chain_node )
    {
        if( _in_job_queue )
        {
            Order_queue* order_queue = _job_chain_node->_job->order_queue();        // Kann bei Programmende NULL sein
            if( order_queue )  order_queue->remove_order( this );
        }

        _job_chain_node = NULL;
    }

    if( _job_chain )
    {
        _log->info( _task? message_string( "SCHEDULER-941", _task->obj_name() ) 
                         : message_string( "SCHEDULER-940" ) );

        if( _task )  _removed_from_job_chain = _job_chain;      // Für die Task merken, in welcher Jobkette wir waren

        _job_chain = NULL;
        _log->set_prefix( obj_name() );

        job_chain->unregister_order( this );
        //_job_chain->_log.info( S() << obj_name() << " ist entfernt" );
    }

    _setback_count = 0;

    if( _replacement_for )  _replacement_for->_replaced_by = NULL,  _replacement_for = NULL;

    if( _task )  _moved = true;

    _id_locked = false;

    if( job_chain )
    {
        job_chain->check_for_removing();
    }
}

//--------------------------------------------------------------------------Order::add_to_job_chain

void Order::add_to_job_chain( Job_chain* job_chain )
{
    bool ok = try_add_to_job_chain( job_chain );
    if( !ok )  z::throw_xc( "SCHEDULER-186", obj_name(), job_chain->name() );
}

//--------------------------------------------------------------------------Order::add_to_job_chain

bool Order::try_add_to_job_chain( Job_chain* job_chain )
{
  //if( _remove_from_job_chain )  z::throw_xc( "SCHEDULER-228", obj_name() );
    if( job_chain->state() != Job_chain::s_ready )  z::throw_xc( "SCHEDULER-151" );
    if( job_chain->has_order_id( id() ) )  return false;

    job_chain->set_visible( true );

    ptr<Order> me = this;   // Halten

    //THREAD_LOCK( _lock )
    {
        if( _job_chain )  remove_from_job_chain();

        if( !job_chain->_chain.empty() )
        {
            if( _state.vt == VT_EMPTY )  set_state2( job_chain->first_node()->_state );     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

            //Z_DEBUG_ONLY( Z_LOG2( "scheduler", "job_chain->node_from_state()\n" ); )
            Job_chain_node* node = job_chain->node_from_state( _state );

            if( !node->_job  || !node->_job->order_queue() )  z::throw_xc( "SCHEDULER-149", job_chain->name(), debug_string_from_variant(_state) );

            if( _id.vt == VT_EMPTY )  set_default_id();
            _id_locked = true;

            job_chain->register_order( this );

            _removed_from_job_chain = NULL;
            _job_chain = job_chain;
            _job_chain_node = node;

            _log->set_prefix( obj_name() );

            node->_job->order_queue()->add_order( this );


            if( !_delay_storing_until_processing  &&  job_chain->_orders_recoverable  &&  !_is_in_database )
            {
                _spooler->_db->insert_order( this );
            }
        }


        setback( _state == _initial_state  &&  !_setback  &&  _run_time->set()? next_start_time( true ) : _setback );
    }

    return true;
}

//------------------------------------------------------------Order::add_to_or_replace_in_job_chain

void Order::add_to_or_replace_in_job_chain( Job_chain* job_chain )
{
    if( ptr<Order> other_order = job_chain->order_or_null( id() ) )
    {
        other_order->remove_from_job_chain();
        add_to_job_chain( job_chain );


        //if( other_order->_run_time == _run_time  &&  other_order->_setback_count == 0 )
        {
            // Für Andreas Liebert: repeat-Intervall beibehalten
        }

        if( other_order->_task )
        {
            _replacement_for = other_order;
            _replacement_for->_replaced_by = this;
            //_log = other_order->_log;
            //other_order->_dont_close_log = true;

            _log->info( message_string( "SCHEDULER-942", other_order->_task->obj_name(), other_order->obj_name() ) );       // add_or_replace_order(): Auftrag wird verzögert bis <p1/> <p2/> ausgeführt hat
        }
    }
    else
    {
        add_to_job_chain( job_chain );
    }
}

//--------------------------------------------------------------------------Order::add_to_blacklist

void Order::add_to_blacklist()
{ 
    assert( _job_chain );
    _job_chain->add_to_blacklist( this ); 
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Job_chain_node* node )
{
    if( _on_blacklist )  _job_chain->remove_from_blacklist( this );

    //THREAD_LOCK( _lock )
    {
        if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );

        if( _task )  _moved = true;
        //§1495  _task = NULL;

        if( _job_chain_node && _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this ), _job_chain_node = NULL;

        _job_chain_node = node;

        set_state2( node? node->_state : Order::State() );

        if( node && node->_job )  node->_job->order_queue()->add_order( this );
    }
}

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( bool success )
{
    Job* last_job = _task? _task->job() : NULL;

    //THREAD_LOCK( _lock )
    {
        bool force_error_state = false;

        if( _setback == latter_day  &&  _setback_count > _task->job()->max_order_setbacks() )
        {
            _log->info( message_string( "SCHEDULER-943", _setback_count ) );   // " mal zurückgestellt. Der Auftrag wechselt in den Fehlerzustand"
            success = false;
            force_error_state = true;
        }

        _task = NULL;



        if( !_setback && !_moved && !_finished  ||  force_error_state )
        {
            _setback_count = 0;

            if( _job_chain_node )
            {
                if( _job_chain_node->_job )
                {
                    if( !_job_chain_node->_job->order_queue() )  _log->warn( "Job " + _job_chain_node->_job->obj_name() + " without order queue (§1495)" );  // Problem §1495
                    else  _job_chain_node->_job->order_queue()->remove_order( this );
                }

                State new_state;

                if( success )
                {
                    //_log->debug( "Neuer Zustand ist " + error_string_from_variant(_job_chain_node->_next_state) );
                    new_state = _job_chain_node->_next_state;
                    _job_chain_node = _job_chain_node->_next_node;
                }
                else
                {
                    //_log->debug( "Neuer Fehler-Zustand ist " + error_string_from_variant(_job_chain_node->_error_state) );
                    new_state = _job_chain_node->_error_state;
                    _job_chain_node = _job_chain_node->_error_node;
                }

                set_state2( new_state, !success );

                if( _job_chain_node  &&  _job_chain_node->_job )
                {
                    _job_chain_node->_job->order_queue()->add_order( this );
                }
                else
                {
                    Time next_start = next_start_time();
                    if( next_start != latter_day )
                    {
                        _log->info( message_string( "SCHEDULER-944", _initial_state, next_start ) );        // "Kein weiterer Job in der Jobkette, der Auftrag wird mit state=<p1/> wiederholt um <p2/>"

                        _end_time = Time::now();
                        _log->close();
                        if( _job_chain  &&  _is_in_database )  _spooler->_db->write_order_history( this );

                        _start_time = 0;
                        _end_time = 0;
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


    //if( _job_chain_node  &&  _job_chain_node->is_file_order_sink() )  process_file_order_sink();

    if( finished() )
    {
        if( is_file_order()  &&  file_exists( file_path() ) )
        {
            _log->error( message_string( "SCHEDULER-340" ) );
            if( _job_chain )  add_to_blacklist();
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
            _spooler->_db->update_order( this );
        }
        catch( exception& x )
        {
            _log->error( message_string( "SCHEDULER-313", x ) );
        }
    }


    if( finished()  &&  !_on_blacklist )  close();
}

//----------------------------------------------------------------------------------Order::setback_

void Order::setback_()
{
    //THREAD_LOCK( _lock )
    {
        if( !_task      )  z::throw_xc( "SCHEDULER-187" );
        if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
        if( !_job_chain )  z::throw_xc( "SCHEDULER-157", obj_name() );
        if( !order_queue() )  z::throw_xc( "SCHEDULER-163", obj_name() );

        order_queue()->remove_order( this );

        _setback_count++;

        int maximum = _task->job()->max_order_setbacks();
        if( _setback_count <= maximum )
        {
            _setback = Time::now() + _task->job()->get_delay_order_after_setback( _setback_count );
            _log->info( message_string( "SCHEDULER-946", _setback_count, _setback ) );   // "setback(): Auftrag zum $1. Mal zurückgestellt, bis $2"
        }
        else
        {
            _setback = latter_day;  // Das heißt: Der Auftrag kommt in den Fehlerzustand
            _log->warn( message_string( "SCHEDULER-947", _setback_count, maximum ) );   // "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, ""das ist über dem Maximum " + as_string(maximum) + " des Jobs" );
        }

        order_queue()->add_order( this, Order_queue::dont_log );

        // Weitere Verarbeitung in postprocessing()
    }
}

//-----------------------------------------------------------------------------------Order::setback

void Order::setback( const Time& start_time_ )
{
    Time start_time = start_time_ > Time::now()? start_time_ : Time(0);

    if( _setback != start_time )
    {
        if( _in_job_queue )
        {
            ptr<Order> hold_me = this;
            order_queue()->remove_order( this );
            _setback = start_time;
            order_queue()->add_order( this );
        }
        else
            _setback = start_time;
    }


    _setback_count = 0;
}

//------------------------------------------------------------------------------------Order::set_at

void Order::set_at( const Time& time )
{
    assert_no_task();
    if( _moved      )  z::throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain->name() );

    /*
    xml::Document_ptr run_time_dom;
    run_time_dom.create();

    xml::Element_ptr run_time_element = run_time_dom    .create_root_element( "run_time" );
    xml::Element_ptr date_element     = run_time_element.append_new_element( "date" );
    xml::Element_ptr day_element      = date_element    .append_new_element( "day" );

    date_element.setAttribute( "date" , time.as_string().substr( 0, 10 ) );
    day_element .setAttribute( "begin", time.as_string().substr( 11 ) );

    set_run_time( run_time_element );
    */
    setback( time );
}

//---------------------------------------------------------------------------Order::next_start_time

Time Order::next_start_time( bool first_call )
{
    Time result = latter_day;

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
                
                if( _period.repeat() == latter_day
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
  //if( _job_chain  )  z::throw_xc( "SCHEDULER-186", obj_name(), _job_chain->name() );
}

//-------------------------------------------------------------------Order::run_time_modified_event

void Order::run_time_modified_event()
{
    if( _state == _initial_state  &&  !_task )  setback( _run_time->set()? next_start_time( true ) : Time(0) );
}

//------------------------------------------------------------------------------Order::set_run_time

void Order::set_run_time( const xml::Element_ptr& e )
{
    _run_time = Z_NEW( Run_time( _spooler, Run_time::application_order ) );
    _run_time->set_modified_event_handler( this );

    if( e )  _run_time->set_dom( e );       // Ruft setback() über modify_event()
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
        if( Job_chain* job_chain = this->job_chain() )  result += job_chain->name() + ":";

        result += debug_string_from_variant(_id);
        if( _title != "" )  result += " " + quoted_string( _title );

        /*
        if( _setback )
            if( _setback == latter_day )  result += ", without start time!";
                                    else  result += ", at=" + _setback.as_string();
        */
      //else
      //if( _priority )  result += ", pri=" + as_string( _priority );
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
