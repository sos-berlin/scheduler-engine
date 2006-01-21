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


namespace sos {
namespace spooler {

//---------------------------------------------------------------------------Spooler::add_job_chain

void Spooler::add_job_chain( Job_chain* job_chain )
{
    //THREAD_LOCK( _job_chain_lock )
    {
        job_chain->finish();   // Jobkette prüfen und in Ordnung bringen

        string lname = lcase( job_chain->name() );
        if( _job_chain_map.find( lname ) != _job_chain_map.end() )  throw_xc( "SCHEDULER-160", lname );

        _job_chain_map[lname] = job_chain;

        job_chain->set_state( Job_chain::s_ready );
    }

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

    _job_chain_time = Time::now();
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
    if( !result )  throw_xc( "SCHEDULER-161", name );

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

//-------------------------------------------------------------xml::Element_ptr Job_chain_node::xml

xml::Element_ptr Job_chain_node::dom_element( const xml::Document_ptr& document, const Show_what& show, Job_chain* job_chain )
{
    xml::Element_ptr element = document.createElement( "job_chain_node" );

                                        element.setAttribute( "state"      , debug_string_from_variant( _state       ) );
        if( !_next_state.is_empty()  )  element.setAttribute( "next_state" , debug_string_from_variant( _next_state  ) );
        if( !_error_state.is_empty() )  element.setAttribute( "error_state", debug_string_from_variant( _error_state ) );
        if( _job )                      element.setAttribute( "orders"     , order_count( job_chain ) );

        if( _job )
        {
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
    _zero_(this+1),
    _spooler(spooler),
    _log(_spooler),
    _lock("Job_chain"),
    _store_orders_in_database(true),
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
    catch( exception& x ) { Z_LOG( __FUNCTION__ << ": " << x.what() << '\n' ); }
}

//---------------------------------------------------------------------------------Job_chain::close

void Job_chain::close()
{
    Z_LOGI2( "scheduler", *this << ".close()\n" );
    remove_all_pending_orders( true );
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
    set_name( element.getAttribute( "name" ) );
    _visible = element.bool_getAttribute( "visible", _visible );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "job_chain_node" ) )
        {
            string job_name    = e.getAttribute( "job" );
            string state       = e.getAttribute( "state" );

            bool can_be_not_initialized = true;
            Job* job = job_name == ""? NULL : _spooler->get_job( job_name, can_be_not_initialized  );
            if( state == "" )  throw_xc( "SCHEDULER-231", "job_chain_node", "state" );

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

        if( _state >= s_ready )
        {
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
    }

    return element;
}

//---------------------------------------------------------------------------------normalized_state

static Order::State normalized_state( const Order::State& state )
{
    if( state.vt == VT_BSTR  &&  ( state.bstrVal == NULL || SysStringLen( state.bstrVal ) == 0 ) )
    {
        return Variant( Variant::vt_error, DISP_E_PARAMNOTFOUND );      // Für Java
    }
    else
    {
        return state;
    }
}

//-------------------------------------------------------------Job_chain::load_orders_from_database

void Job_chain::load_orders_from_database()
{
    if( !_store_orders_in_database )  return;
    
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
                    " and \"JOB_CHAIN\"=" + sql::quoted(_name) +
                    " order by \"ORDERING\"" );

        while( !sel.eof() )
        {
            Record record = sel.get_record();
            ptr<Order> order = new Order( _spooler, record, 
                                          _spooler->_db->read_payload_clob( record.as_string( "id" ) ), 
                                          _spooler->_db->read_orders_runtime_clob( record.as_string( "id" ) ) );
            order->_is_in_database = true;
            order->add_to_job_chain( this );    // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
            count++;
        }
    }

    _log.debug( as_string(count) + " Aufträge aus der Datenbank gelesen" );
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
            Z_LOG( __FUNCTION__ << ": " << order->obj_name() << " wird nicht entfernt, weil in Verarbeitung durch " << order->_task->obj_name() << "\n" );
        }
    }

    return result;
}

//-------------------------------------------------------------------------------Job_chain::add_job

void Job_chain::add_job( Job* job, const Order::State& state, const Order::State& next_state, const Order::State& error_state )
{
    if( job  &&  !job->order_queue() )  throw_xc( "SCHEDULER-147", job->name() );

    if( _state != s_under_construction )  throw_xc( "SCHEDULER-148" );

    ptr<Job_chain_node> node = new Job_chain_node;

    node->_job   = job;
    node->_state = state;

    if( node->_state.is_error() )  node->_state = job->name();      // Parameter state nicht angegeben? Default ist der Jobname

    node->_next_state  = normalized_state( next_state );
    node->_error_state = normalized_state( error_state );

    // Bis finish() bleibt nicht angegebener Zustand als VT_ERROR/is_error (fehlender Parameter) stehen.
    // finish() unterscheidet dann die nicht angegebenen Zustände von VT_ERROR und setzt Defaults oder VT_EMPTY.

    //THREAD_LOCK( _lock )
    {
        if( node_from_state_or_null( node->_state ) )
        {
            if( !job  &&  next_state.is_error()  &&  error_state.is_error() )  return;     // job_chain.add_end_state() darf mehrfach gerufen werden.
            throw_xc( "SCHEDULER-150", debug_string_from_variant(node->_state), name() );
        }

        _chain.push_back( node );

        if( job )  job->set_job_chain_priority( _chain.size() );   // Weiter hinten stehende Jobs werden vorrangig ausgeführt
    }
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
            if( n->_job  &&  n->_next_state.is_error() )  add_job( NULL, "<END_STATE>" );    // Endzustand fehlt? Dann hinzufügen
        }

        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            Chain::iterator next = it;  next++;

            if( n->_next_state.is_error()  &&  next != _chain.end()  &&  n->_job )  n->_next_state = (*next)->_state;

            if( !n->_next_state.is_error() )  n->_next_node  = node_from_state( n->_next_state );
                                        else  n->_next_state = empty_variant;

            if( !n->_error_state.is_error() )  n->_error_node  = node_from_state( n->_error_state );
                                         else  n->_error_state = empty_variant;
        }


        if( zschimmer::Log_ptr log = "" )
        {
            log << "Job_chain " << _name << " finished\n:";

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

    throw_xc( "SCHEDULER-152", job->name(), name() );
    return NULL;
}

//-----------------------------------------------------------------------Job_chain::node_from_state

Job_chain_node* Job_chain::node_from_state( const Order::State& state )
{
    Job_chain_node* result = node_from_state_or_null( state );
    if( !result )  throw_xc( "SCHEDULER-149", name(), debug_string_from_variant(state) );
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

    if( !result )  throw_xc( "SCHEDULER-162", debug_string_from_variant(id), _name );

    return result;
}

//-------------------------------------------------------------------------Job_chain::order_or_null

ptr<Order> Job_chain::order_or_null( const Order::Id& id )
{
    //THREAD_LOCK( _lock )
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job* job = (*it)->_job;
            if( job )
            {
                ptr<Order> result = job->order_queue()->order_or_null( id );
                if( result )  return result;
            }
        }
    }

    return NULL;
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
    string id_string = string_from_variant( order_id );
    Order_map::iterator it = _order_map.find( id_string );
    return it != _order_map.end();
}

//------------------------------------------------------------------------Job_chain::register_order

void Job_chain::register_order( Order* order )
{
    //THREAD_LOCK( _lock )
    {
        string id_string = string_from_variant( order->id() );
        Order_map::iterator it = _order_map.find( id_string );
        if( it != _order_map.end() )  throw_xc( "SCHEDULER-186", id_string, _name );
        _order_map[ id_string ] = order;
    }
}

//----------------------------------------------------------------------Job_chain::unregister_order

void Job_chain::unregister_order( Order* order )
{
    //THREAD_LOCK( _lock )
    {
        string id_string = string_from_variant( order->id() );
        Order_map::iterator it = _order_map.find( id_string );
        if( it != _order_map.end() )  _order_map.erase( it );
                                else  Z_LOG( __FUNCTION__ << " " << order->obj_name() << " ist nicht registriert!?\n" );
    }
}

//--------------------------------------------------------------------------------Job_chain::remove

void Job_chain::remove()
{
    if( _state < s_ready )  throw_xc( "SCHEDULER-151" );

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
            if( j->second == this )  { _spooler->_job_chain_map.erase( j );  break; }
        }
    }
}

//--------------------------------------------------------------------Job_chain::check_for_removing

void Job_chain::check_for_removing()
{
    if( state() == s_removing &&  !has_order() )
    {
        _log.info( "Removing" );
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
            _log->info( S() << "Removing " << order->obj_name() );
        }
    }

    update_priorities();
    _has_users_id = false;
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

    //THREAD_LOCK( _lock )
    {
        if( order->_setback )
        {
            if( order->_setback < latter_day )  order->_log->log( do_log? log_info : log_debug3, S() << "Auftrag wird gestartet um " << order->_setback );
                                          else  order->_log->log( do_log? log_warn : log_debug3, "Die <run_time> des Auftrags hat keine nächste Startzeit" );

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

            _job->calculate_next_time();
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
            bool            id_found  = false;

            _has_users_id |= order->_is_users_id;

            if( _has_users_id  ||  order->priority() > _lowest_priority  &&  order->priority() <= _highest_priority )     // Optimierung
            {
                for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
                {
                    Order* o = *it;
                    if( !ins_set  &&  order->priority() > o->priority() )
                    {
                        ins = it;
                        ins_set = true;
                        if( id_found )  break;
                    }

                    if( !id_found  &&  o->id_is_equal( order->_id ) )
                    {
                        _log->debug( "Auftrag mit gleicher Id wird ersetzt: " + order->obj_name() );
                        if( ins == it )  { ins = _queue.erase( it ); break; }
                                   else  it = _queue.erase( it );
                        id_found = true;
                    }
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

            if( it == _setback_queue.end() )  throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

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

            if( it == _queue.end() )  throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

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
    // SEITENEFFEKT: Aufträge aus der _setback_queue, deren Rückstellungszeitpunkt erreicht ist, werden in die _queue verschoben.

    //THREAD_LOCK( _lock )
    {
        // Zurückgestellte Aufträge, deren Wartezeit abgelaufen ist, hervorholen

        while( !_setback_queue.empty() )
        {
            ptr<Order> o = *_setback_queue.begin();
            if( o->_setback > now )  break;

            remove_order( o );
            o->_setback = 0;
            add_order( o );
        }


        FOR_EACH( Queue, _queue, o )
        {
            Order* order = *o;
            
            if( order->_task )  continue;               // Schon in Verarbeitung
            if( order->_replacement_for )  continue;
            if( order->_job_chain  &&  order->_job_chain->state() != Job_chain::s_ready )  continue;   // Jobkette wird nicht gelöscht?

            return order;
            //if( !(*o)->_task  &&  !(*o)->_replacement_for )  return *o;
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

    //THREAD_LOCK( _lock )
    {
        order = first_order( now );

        if( order )
        {
            order->_start_time = now;
            order->_setback = 0;
            if( order->_moved )  throw_xc( "SCHEDULER-0", order->obj_name() + " _moved=true?" );
            //order->_moved = false;
        }
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

Order::Order( Spooler* spooler, const Record& record, const string& payload_string, const string& run_time_xml )
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

    //string payload_string = record.as_string( "payload" );

    //LOG( "db payload=" << payload_string << "\n" );
    if( payload_string.find( "<" + Com_variable_set::xml_element_name() ) != string::npos )
    {
        //LOG( "... payload ist ein Variable_set!\n" );
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
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    _log = Z_NEW( Prefix_log( this ) );
    _log->set_prefix( obj_name() );
    _created = Time::now();

    set_run_time( NULL );
}

//-------------------------------------------------------------------------------Order::attach_task

void Order::attach_task( Task* task )
{
    assert_no_task();   // Vorsichtshalber

    _task = task;
    if( !_log->opened() )  open_log();
}

//----------------------------------------------------------------------------Order::assert_no_task

void Order::assert_no_task()
{
    if( _task )  throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
}

//----------------------------------------------------------------------------------Order::open_log

void Order::open_log()
{
    if( _job_chain && _spooler->_order_history_with_log && !string_begins_with( _spooler->log_directory(), "*" ) )
    {
        _log->set_filename( _spooler->log_directory() + "/order." + _job_chain->name() + "." + _id.as_string() + ".log" );      // Jobprotokoll
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

    _task = NULL;
    _removed_from_job_chain = NULL;
    if( _replaced_by )  _replaced_by->_replacement_for = NULL, _replaced_by = NULL;

    remove_from_job_chain();
}

//-------------------------------------------------------------------------------Order::dom_element

xml::Element_ptr Order::dom_element( const xml::Document_ptr& document, const Show_what& show, const string* log )
{
    xml::Element_ptr element = document.createElement( "order" );

    //THREAD_LOCK( _lock )
    {
        element.setAttribute( "order"     , debug_string_from_variant( _id ) );
        element.setAttribute( "id"        , debug_string_from_variant( _id ) );     // veraltet

        if( _setback )
        element.setAttribute( "next_start_time", _setback.as_string() );

        if( _title != "" )
        element.setAttribute( "title"     , _title );

        element.setAttribute( "state"     , debug_string_from_variant( _state ) );

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

        if( _setback )
        element.setAttribute( "setback"   , _setback.as_string() );

        if( _replacement_for )
        element.setAttribute( "replacement", "yes" );


        if( show & show_run_time )  element.appendChild( _run_time->dom_element( document ) );

        if( show & show_log )
        {
            try
            {
                dom_append_text_element( element, "log", log? *log : _log->as_string() );
            }
            catch( const exception& x ) { _spooler->_log.warn( string("<show_order what=\"log\">: ") + x.what() ); }
        }
    }

    return element;
}

//-------------------------------------------------------------------------------Order::order_queue

Order_queue* Order::order_queue()
{
    Job* job = this->job();

    if( !job )  throw_xc( "SCHEDULER-163" );

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
        if( _id_locked )  throw_xc( "SCHEDULER-159" );

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
    //THREAD_LOCK( _lock )
    {
        if( _id.vt == VT_EMPTY )
        {
            set_id( _spooler->_db->get_order_id() );
            _is_users_id = false;
        }
    }
}

//-----------------------------------------------------------------------------------Order::set_job

void Order::set_job( Job* job )
{
    //THREAD_LOCK( _lock )
    {
        if( _removed_from_job_chain )
        {
            _log->warn( S() << "job=" << job->name() << " wird ignoriert, weil Auftrag bereits aus der Jobkette entfernt ist" );
            return;
        }

        if( !_job_chain )  throw_xc( "SCHEDULER-157", obj_name() );

        move_to_node( _job_chain->node_from_job( job ) );       // Fehler, wenn Job nicht in der Jobkette
    }
}

//---------------------------------------------------------------------------------------Order::job

Job* Order::job()
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
        Z_LOG2( "scheduler.order", "Order " << obj_name() << ".payload=" << debug_string_from_variant(payload) << "\n" );
        _payload = payload;
        _payload_modified = true;
    }
}

//----------------------------------------------------------------------------------Order::finished

bool Order::finished()
{
    return !_job_chain_node  ||  !_job_chain_node->_job;
}

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state )
{
    //if( _removed_from_job_chain )
    //{
    //    _log->warn( S() << "state=" << debug_string_from_variant( state ) << " wird ignoriert, weil Auftrag bereits aus der Jobkette entfernt ist" );
    //    return;
    //}


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
        if( is_error_state                           )  log_line += ", Fehlerzustand";

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
        if( !order_queue )  throw_xc( "SCHEDULER-147", job_name );
        add_to_order_queue( order_queue );
    }
}

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_order_queue( Order_queue* order_queue )
{
    if( !order_queue )  throw_xc( "SCHEDULER-147", "?" );

    ptr<Order> me = this;   // Halten

    //THREAD_LOCK( _lock )
    {
        if( _task )  _moved = true;

        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        if( _job_chain )  remove_from_job_chain();
        _removed_from_job_chain = NULL;

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
        if( _task )  _removed_from_job_chain = _job_chain;      // Für die Task merken, in welcher Jobkette wir waren

        _job_chain = NULL;
        _log->set_prefix( obj_name() );

        job_chain->unregister_order( this );
        //_job_chain->_log.info( S() << obj_name() << " ist entfernt" );
    }

    _setback_count = 0;

    if( _replacement_for )  _replacement_for->_replaced_by = NULL,  _replacement_for = NULL;

    if( _task )  _moved = true;


    if( job_chain )
    {
        S log_line;
        log_line << "Auftrag ist aus Jobkette " << job_chain->name() << " entfernt";
        if( _task )  log_line << ", wird aber weiter von " << _task->obj_name() << " ausgeführt";
        _log->info( log_line );

        job_chain->check_for_removing();
    }
}

//--------------------------------------------------------------------------Order::add_to_job_chain

void Order::add_to_job_chain( Job_chain* job_chain )
{
    bool ok = try_add_to_job_chain( job_chain );
    if( !ok )  throw_xc( "SCHEDULER-186", obj_name(), job_chain->name() );
}

//--------------------------------------------------------------------------Order::add_to_job_chain

bool Order::try_add_to_job_chain( Job_chain* job_chain )
{
    if( job_chain->has_order_id( id() ) )  return false;

  //if( _remove_from_job_chain )  throw_xc( "SCHEDULER-228", obj_name() );
    if( job_chain->state() != Job_chain::s_ready )  throw_xc( "SCHEDULER-151" );

    ptr<Order> me = this;   // Halten

    //THREAD_LOCK( _lock )
    {
        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        if( _job_chain )  remove_from_job_chain();

        if( !job_chain->_chain.empty() )
        {
            if( _state.vt == VT_EMPTY )  set_state2( (*job_chain->_chain.begin())->_state );     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

            //Z_DEBUG_ONLY( LOG( "job_chain->node_from_state()\n" ); )
            Job_chain_node* node = job_chain->node_from_state( _state );

            if( !node->_job  || !node->_job->order_queue() )  throw_xc( "SCHEDULER-149", job_chain->name(), debug_string_from_variant(_state) );

            job_chain->register_order( this );

            _removed_from_job_chain = NULL;
            _job_chain = job_chain;
            _job_chain_node = node;

            _log->set_prefix( obj_name() );

            node->_job->order_queue()->add_order( this );
        }


        if( !_is_in_database  &&  job_chain->_store_orders_in_database )
        {
            _spooler->_db->insert_order( this );
        }

        setback( _state == _initial_state  &&  _run_time->set()? next_start_time( true ) : Time(0) );
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
            // FÜr Andreas Liebert: repeat-Intervall beibehalten
        }

        if( other_order->_task )
        {
            _replacement_for = other_order;
            _replacement_for->_replaced_by = this;
            //_log = other_order->_log;
            //other_order->_dont_close_log = true;

            _log->info( S() << "add_or_replace_order(): Auftrag wird verzögert bis " << other_order->_task->obj_name() << "  " << other_order->obj_name() << " ausgeführt hat" );
        }
    }
    else
    {
        add_to_job_chain( job_chain );
    }
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Job_chain_node* node )
{
    //THREAD_LOCK( _lock )
    {
        if( !_job_chain )  throw_xc( "SCHEDULER-157", obj_name() );

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
    //THREAD_LOCK( _lock )
    {
        bool force_error_state = false;

      //if( _setback == latter_day )
        if( _setback == latter_day  &&  _setback_count > _task->job()->max_order_setbacks() )
        {
            _log->info( as_string(_setback_count) + " mal zurückgestellt. Der Auftrag wechselt in den Fehlerzustand" );
            success = false;
            force_error_state = true;
        }

        _task = NULL;



        if( !_setback && !_moved  ||  force_error_state )
        {
            _setback_count = 0;

            if( _job_chain_node )
            {
                if( _job_chain_node->_job )
                {
                    if( !_job_chain_node->_job->order_queue() )  _log->warn( "Job " + _job_chain_node->_job->obj_name() + " ohne Auftragswarteschlange (§1495)" );  // Problem §1495
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

                if( !finished() )
                {
                    _job_chain_node->_job->order_queue()->add_order( this );
                }
                else
                {
                    Time next_start = next_start_time();
                    if( next_start != latter_day )
                    {
                        _log->info( S() << "Kein weiterer Job in der Jobkette, der Auftrag wird mit state=" << _initial_state << " wiederholt um " << next_start );

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
                            _log->error( S() << "Fehler beim Setzen des Initialzustands nach Wiederholung wegen <run_time>:\n" << x );
                        }
                    }
                    else
                        _log->info( "Kein weiterer Job in der Jobkette, der Auftrag ist erledigt" );
                }
            }
            else
            {
                _order_queue->remove_order( this );
                _order_queue = NULL;
            }
        }

        postprocessing2();
    }
}

//--------------------------------------------------------------------------Order::processing_error

void Order::processing_error()
{
    //THREAD_LOCK( _lock )
    {
        _task = NULL;

        postprocessing2();
    }
}

//---------------------------------------------------------------------------Order::postprocessing2

void Order::postprocessing2()
{
    Job* job = this->job();

    if( _moved  &&  job  &&  !order_queue()->has_order( Time::now() ) )
    {
        job->signal( "Order (delayed set_state)" );
    }

    _moved = false;



    if( finished()  &&  _web_service_name != "" )
    {
        try
        {
            _spooler->_web_services.web_service_by_name( _web_service_name )->forward_order( this );
            assert( !finished() );
        }
        catch( exception& x )
        {
            _log->error( string("Web-service forward: ") + x.what() );
        }
    }

    if( finished() )
    {
        _end_time = Time::now();
        //if( !_dont_close_log )
        _log->close();
    }

    if( _job_chain  &&  ( _is_in_database || finished() ) )  _spooler->_db->update_order( this );

    if( finished() )  close();
}

//----------------------------------------------------------------------------------Order::setback_

void Order::setback_()
{
    //THREAD_LOCK( _lock )
    {
        if( !_task      )  throw_xc( "SCHEDULER-187" );
        if( _moved      )  throw_xc( "SCHEDULER-188", obj_name() );
        if( !_job_chain )  throw_xc( "SCHEDULER-157", obj_name() );
        if( !order_queue() )  throw_xc( "SCHEDULER-163", obj_name() );

        order_queue()->remove_order( this );

        _setback_count++;

        int maximum = _task->job()->max_order_setbacks();
        if( _setback_count <= maximum )
        {
            _setback = Time::now() + _task->job()->get_delay_order_after_setback( _setback_count );
            _log->info( "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, bis " + _setback.as_string() );
        }
        else
        {
            _setback = latter_day;  // Das heißt: Der Auftrag kommt in den Fehlerzustand
            _log->warn( "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, "
                        "das ist über dem Maximum " + as_string(maximum) + " des Jobs" );
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
    if( _moved      )  throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  throw_xc( "SCHEDULER-186", obj_name(), _job_chain->name() );


    xml::Document_ptr run_time_dom;
    run_time_dom.create();

    xml::Element_ptr run_time_element = run_time_dom    .create_root_element( "run_time" );
    xml::Element_ptr date_element     = run_time_element.append_new_element( "date" );
    xml::Element_ptr day_element      = date_element    .append_new_element( "day" );

    date_element.setAttribute( "date" , time.as_string().substr( 0, 10 ) );
    day_element .setAttribute( "begin", time.as_string().substr( 11 ) );

    set_run_time( run_time_element );

    //setback( time );
}

//---------------------------------------------------------------------------Order::next_start_time

Time Order::next_start_time( bool first_call )
{
    Time result = latter_day;

    if( _run_time->set() )
    {
        Time now = Time::now();

        if( first_call  ||  now >= _period.end() )       // Periode abgelaufen?
        {
            _period = _run_time->next_period( now, time::wss_next_period_or_single_start );
            result = _period.begin();
        }
        else
        {
            result = now + _period.repeat();

            if( _period.is_single_start()  ||  result >= _period.end() )       // Periode am Ende?
            {
                Period next_period = _run_time->next_period( result == latter_day? _period.is_single_start()? now : _period.end() : result, time::wss_next_begin );

                if( _period.end()    != next_period.begin()
                 || _period.repeat() != next_period.repeat() )
                {
                    result = next_period.begin();  // Perioden sind nicht nahtlos: Wiederholungsintervall neu berechnen
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

//-----------------------------------------------------------------------Order::before_modify_event

void Order::before_modify_event()
{
  //if( _task       )  throw_xc( "SCHEDULER-217", obj_name(), _task->obj_name() );
  //if( _moved      )  throw_xc( "SCHEDULER-188", obj_name() );
  //if( _job_chain  )  throw_xc( "SCHEDULER-186", obj_name(), _job_chain->name() );
}

//----------------------------------------------------------------------------Order::modified_event

void Order::modified_event()
{
    if( _state == _initial_state  &&  !_task )  setback( _run_time->set()? next_start_time( true ) : Time(0) );
}

//------------------------------------------------------------------------------Order::set_run_time

void Order::set_run_time( const xml::Element_ptr& e )
{
    _run_time = Z_NEW( Run_time( _spooler, Run_time::application_order ) );
    _run_time->set_modified_event_handler( this );

    if( e )  _run_time->set_dom( e );       // Ruft setback() über modify_event()
       else  modified_event();
}

//----------------------------------------------------------------------------------Order::obj_name

string Order::obj_name()
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
