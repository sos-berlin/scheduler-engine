// $Id: spooler_order.cxx,v 1.27 2003/06/24 15:46:29 jz Exp $
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
    THREAD_LOCK( _job_chain_lock )
    {
        job_chain->finish();   // Jobkette prüfen und in Ordnung bringen

        string lname = lcase( job_chain->name() );
        if( _job_chain_map.find( lname ) != _job_chain_map.end() )  throw_xc( "SPOOLER-160", lname );

        _job_chain_map[lname] = job_chain;
    }

    job_chain->load_orders_from_database();

    _job_chain_time = Time::now();
}

//-------------------------------------------------------------------------------Spooler::job_chain

Job_chain* Spooler::job_chain( const string& name )
{
    Job_chain* result = NULL;

    THREAD_LOCK( _job_chain_lock )
    {
        string lname = lcase( name );
    
        Job_chain_map::iterator it = _job_chain_map.find( lname );
        if( it == _job_chain_map.end() )  throw_xc( "SPOOLER-161", lname );

        result = it->second;
    }

    return result;
}

//----------------------------------------------------xml::Element_ptr Spooler::xml_from_job_chains

xml::Element_ptr Spooler::xml_from_job_chains( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr job_chains_element = document.createElement( "job_chains" );

        dom_append_nl( job_chains_element );

        THREAD_LOCK( _job_chain_lock )
        {
            FOR_EACH( Job_chain_map, _job_chain_map, it )
            {
                Job_chain* job_chain = it->second;
                job_chains_element.appendChild( job_chain->dom( document, show ) );
                dom_append_nl( job_chains_element );            
            }
        }

    return job_chains_element;
}

//-------------------------------------------------------------xml::Element_ptr Job_chain_node::xml

xml::Element_ptr Job_chain_node::dom( const xml::Document_ptr& document, Show_what show, Job_chain* job_chain )
{
    xml::Element_ptr element = document.createElement( "job_chain_node" );

                                        element.setAttribute( "state"      , string_from_variant( _state       ) );
        if( !_next_state.is_empty()  )  element.setAttribute( "next_state" , string_from_variant( _next_state  ) );
        if( !_error_state.is_empty() )  element.setAttribute( "error_state", string_from_variant( _error_state ) );
   
        if( _job )
        {
            element.setAttribute( "job", _job->name() );

            //if( show & show_orders )  
            {
                dom_append_nl( element );
                element.appendChild( _job->dom( document, show, job_chain ) );
                dom_append_nl( element );
            }
        }

    return element;
}

//-----------------------------------------------------------------------------Job_chain::Job_chain

Job_chain::Job_chain( Spooler* spooler )
:
    Com_job_chain( this ),
    _zero_(this+1),
    _spooler(spooler)
{
}

//----------------------------------------------------------------------------Job_chain::~Job_chain

Job_chain::~Job_chain()
{
}

//------------------------------------------------------------------xml::Element_ptr Job_chain::dom

xml::Element_ptr Job_chain::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr element = document.createElement( "job_chain" );

        THREAD_LOCK( _lock )
        {
            element.setAttribute( "name", _name );
    
            if( _finished )
            {
                dom_append_nl( element );

                FOR_EACH( Chain, _chain, it )
                {
                    Job_chain_node* node = *it;
                    element.appendChild( node->dom( document, show, this ) );
                    dom_append_nl( element );
                }
            }
        }

        dom_append_nl( element );

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

// in spooler_history.cxx
void Job_chain::load_orders_from_database()
{
    int count = 0;

    {
        Transaction ta ( _spooler->_db );

        Any_file sel ( _spooler->_db->db_name() + " select \"ID\", \"PRIORITY\", \"STATE\", \"STATE_TEXT\", \"TITLE\", \"CREATED_TIME\""
                                                  " from " + sql::quoted_name( _spooler->_orders_tablename ) +
                                                  " where \"SPOOLER_ID\"=" + sql::quoted(_spooler->id_for_db()) + 
                                                  " and \"JOB_CHAIN\"=" + sql::quoted(_name) +
                                                  " order by \"ORDERING\"" );
        while( !sel.eof() )
        {
            ptr<Order> order = new Order( _spooler, sel.get_record() );
            order->add_to_job_chain( this, false );    // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
            count++;
        }
    }

    _spooler->log().debug( "Jobkette " + _name + ": " + as_string(count) + " Aufträge aus der Datenbank gelesen" );
}


//-------------------------------------------------------------------------------Job_chain::add_job

void Job_chain::add_job( Job* job, const Order::State& state, const Order::State& next_state, const Order::State& error_state )
{
    if( job  &&  !job->order_queue() )  throw_xc( "SPOOLER-147", job->name() );

    if( _finished )  throw_xc( "SPOOLER-148" );

    ptr<Job_chain_node> node = new Job_chain_node;

    node->_job   = job;
    node->_state = state;

    if( node->_state.is_error() )  node->_state = job->name();      // Parameter state nicht angegeben? Default ist der Jobname

    node->_next_state  = normalized_state( next_state );
    node->_error_state = normalized_state( error_state );

    // Bis finish() bleibt nicht angegebener Zustand als VT_ERROR/is_error (fehlender Parameter) stehen.
    // finish() unterscheidet dann die nicht angegebenen Zustände von VT_ERROR und setzt Defaults oder VT_EMPTY.

    THREAD_LOCK( _lock )
    {
        if( node_from_state_or_null( node->_state ) )  
        {
            if( !job  &&  next_state.is_error()  &&  error_state.is_error() )  return;     // job_chain.add_end_state() darf mehrfach gerufen werden.
            throw_xc( "SPOOLER-150", error_string_from_variant(node->_state), name() );
        }

        _chain.push_back( node );

        if( job )  job->set_job_chain_priority( _chain.size() );   // Weiter hinten stehende Jobs werden vorrangig ausgeführt
    }
}

//--------------------------------------------------------------------------------Job_chain::finish

void Job_chain::finish()
{
    THREAD_LOCK( _lock )
    {
        if( _finished )  return;

        if( !_chain.empty() )
        {
            Job_chain_node* n = *_chain.rbegin();
            if( n->_job  &&  n->_next_state.is_error() )  add_job( NULL, "<END_STATE>" );    // Endzustand fehlt? Dann hinzufügen
        }

        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            Chain::iterator next = it;  next++;

            if( n->_next_state.is_error()  &&  next != _chain.end() )  n->_next_state = (*next)->_state;

            if( !n->_next_state.is_error() )  n->_next_node  = node_from_state( n->_next_state );
                                        else  n->_next_state = empty_variant;

            if( !n->_error_state.is_error() )  n->_error_node  = node_from_state( n->_error_state );
                                         else  n->_error_state = empty_variant;
        }

        _finished = true;
    }
}

//-------------------------------------------------------------------------Job_chain::node_from_job

Job_chain_node* Job_chain::node_from_job( Job* job )
{
    THREAD_LOCK( _lock )
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            if( n->_job == job )  return n;
        }
    }

    throw_xc( "SPOOLER-152", job->name(), name() );
    return NULL;
}

//-----------------------------------------------------------------------Job_chain::node_from_state

Job_chain_node* Job_chain::node_from_state( const State& state )
{
    Job_chain_node* result = node_from_state_or_null( state );
    if( !result )  throw_xc( "SPOOLER-149", name(), error_string_from_variant(state) );
    return result;
}

//---------------------------------------------------------------Job_chain::node_from_state_or_null

Job_chain_node* Job_chain::node_from_state_or_null( const State& state )
{
    THREAD_LOCK( _lock )
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
    THREAD_LOCK( _lock )
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

        throw_xc( "SPOOLER-162", error_string_from_variant(id), _name );
    }

    return NULL;
}

//---------------------------------------------------------------------------Job_chain::order_count

int Job_chain::order_count()
{
    int       result = 0;
    set<Job*> jobs;             // Jobs können (theoretisch) doppelt vorkommen, sollen aber nicht doppelt gezählt werden.

    THREAD_LOCK( _lock )
    {
        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job* job = (*it)->_job;
            if( job  &&  !set_includes( jobs, job ) )  jobs.insert( job ),  result += job->order_queue()->length();
        }
    }

    return result;
}

//------------------------------------------------------------------------Job_chain::register_order

void Job_chain::register_order( Order* order )
{
    THREAD_LOCK( _lock )
    {
        string id_string = string_from_variant( order->id() );
        Order_map::iterator it = _order_map.find( id_string );
        if( it != _order_map.end() )  throw_xc( "SPOOLER-186", id_string, _name );
        _order_map[ id_string ] = order;
    }
}

//----------------------------------------------------------------------Job_chain::unregister_order

void Job_chain::unregister_order( Order* order )
{
    THREAD_LOCK( _lock )
    {
        string id_string = string_from_variant( order->id() );
        Order_map::iterator it = _order_map.find( id_string );
        if( it != _order_map.end() )  _order_map.erase( it );
        _order_map[ id_string ] = order;
    }
}

//-------------------------------------------------------------------------Order_queue::Order_queue

Order_queue::Order_queue( Job* job, Prefix_log* log )
: 
    _zero_(this+1),
    _spooler(job->_spooler), 
    _job(job),
    _log(log)
{
}

//------------------------------------------------------------------------Order_queue::~Order_queue

Order_queue::~Order_queue()
{
}

//---------------------------------------------------------------------------------Order_queue::dom

xml::Element_ptr Order_queue::dom( const xml::Document_ptr& document, Show_what show, Job_chain* which_job_chain )
{
    xml::Element_ptr element = document.createElement( "order_queue" );

    THREAD_LOCK( _lock )
    {
        element.setAttribute( "length", length(which_job_chain) );

        if( show & show_orders )
        {
            FOR_EACH( Queue, _queue, it )
            {
                Order* order = *it;
                if( !which_job_chain  ||  order->job_chain() == which_job_chain )
                {
                    dom_append_nl( element );
                    element.appendChild( order->dom( document, show ) );
                }
            }

            dom_append_nl( element );
        }
    }

    return element;
}

//------------------------------------------------------------------------------Order_queue::length

int Order_queue::length( Job_chain* which_job_chain )
{ 
    if( which_job_chain )
    {
        int count = 0;
        THREAD_LOCK( _lock )  FOR_EACH( Queue, _queue, it )  if( (*it)->_job_chain == which_job_chain )  count++;
        return count;
    }
    else
    {
        return _queue.size(); 
    }
}

//---------------------------------------------------------------------------Order_queue::add_order

void Order_queue::add_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    THREAD_LOCK( _lock )
    {
        Queue::iterator ins       = _queue.end();
        bool            ins_set   = false;
        bool            was_empty = _queue.empty();
        bool            id_found  = false;


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

        order->_in_job_queue = true;

        if( was_empty )  _job->signal( "Order" );
    }
}

//------------------------------------------------------------------------Order_queue::remove_order

void Order_queue::remove_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    _log->debug( "remove_order " + order->obj_name() );

    THREAD_LOCK( _lock )
    {
        Queue::iterator it;
        for( it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

        if( it == _queue.end() )  throw_xc( "SPOOLER-156", order->obj_name(), _job->name() );

        _queue.erase( it );
      //_id_map.erase( order->_id );
        update_priorities();

        order->_in_job_queue = false;
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
}

//------------------------------------------------------------Order_queue::get_order_for_processing

ptr<Order> Order_queue::get_order_for_processing( Task* task )
{
    // Die Order_queue gehört genau einem Job. Der Job kann zur selben Zeit nur einen Schritt ausführen.
    // Deshalb kann nur der erste Auftrag in Verarbeitung sein.

    ptr<Order> order;

    THREAD_LOCK( _lock )
    {
        if( !_queue.empty() )
        {
            order = _queue.front();

            if( order->_task )  throw_xc( "Order_queue::get_order_for_processing" );   // Darf nicht passieren
            
            order->_task = task;
            order->_moved = false;

            if( !order->_start_time )  
            {
                order->_start_time = Time::now();
                order->open_log();
            }
        }   
    }

    return order;
}

//-----------------------------------------------------------------------Order_queue::order_or_null

ptr<Order> Order_queue::order_or_null( const Order::Id& id )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Queue, _queue, it )  if( (*it)->_id == id )  return *it;
    }

    return NULL;
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const VARIANT& payload )
: 
    Com_order(this),
    _zero_(this+1), 
    _spooler(spooler),
    _log(spooler),
    _payload(payload)
{
    init();
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const Record& record )
: 
    Com_order(this),
    _zero_(this+1), 
    _spooler(spooler),
    _log(spooler)
{
    //init();

    _id         = record.as_string( "id" );
    _state      = record.as_string( "state" );
    _state_text = record.as_string( "state_text" );
    _title      = record.as_string( "title" );
    _priority   = record.as_int   ( "priority" );
    _created.set_datetime( record.as_string( "created_time" ) );
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
    remove_from_job_chain();
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    _created = Time::now();
}

//----------------------------------------------------------------------------------Order::open_log

void Order::open_log()
{
    if( _job_chain && _spooler->_order_history_with_log )
    {
        _log.set_filename( _spooler->log_directory() + "/order." + _job_chain->name() + "." + _id.as_string() + ".log" );      // Jobprotokoll
        _log.open();
    }
}

//-------------------------------------------------------------------------------------Order::close

void Order::close()
{
    if( !_log.filename().empty() )
    {
        try
        {
            remove_file( _log.filename() );
        }
        catch( const exception& x )
        {
            _spooler->_log.warn( "FEHLER BEIM LÖSCHEN DER DATEI " + _log.filename() + ": " + x.what() );
        }
    }

    remove_from_job_chain();
}

//---------------------------------------------------------------------------------------Order::dom

xml::Element_ptr Order::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr element = document.createElement( "order" );

    THREAD_LOCK( _lock )
    {
        element.setAttribute( "id"        , string_from_variant( _id ) );
        element.setAttribute( "title"     , _title );
        element.setAttribute( "state"     , string_from_variant( _state ) );

        if( _job_chain )  
        element.setAttribute( "job_chain" , _job_chain->name() );

        Job* job = this->job();
        if( job )
        element.setAttribute( "job"       , job->name() );

        if( _task )
        element.setAttribute( "in_process_since", _task->last_process_start_time().as_string() );

        element.setAttribute( "state_text", _state_text );
        element.setAttribute( "priority"  , _priority );
        element.setAttribute( "created"   , _created.as_string() );
    }

    return element;
}

//-------------------------------------------------------------------------------Order::order_queue

Order_queue* Order::order_queue()
{
    Job* job = this->job();

    if( !job )  throw_xc( "SPOOLER-163" );

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
    THREAD_LOCK(_lock)
    {
        if( _id_locked )  throw_xc( "SPOOLER-159" );

        _id = id; 
        _is_users_id = true;
    }
}

//----------------------------------------------------------------------------Order::set_default_id

void Order::set_default_id()
{ 
    THREAD_LOCK( _lock )
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
    THREAD_LOCK( _lock )
    {
        if( !_job_chain )  throw_xc( "SPOOLER-157", obj_name() );
        
        move_to_node( _job_chain->node_from_job( job ) );       // Fehler, wenn Job nicht in der Jobkette
    }
}

//---------------------------------------------------------------------------------------Order::job

Job* Order::job()
{
    Job* result = NULL;

    THREAD_LOCK( _lock )
    {
        if( _job_chain_node )  result = _job_chain_node->_job;
        else
        if( _order_queue    )  result = _order_queue->job();
    }

    return result;
}

//----------------------------------------------------------------------------------Order::finished

bool Order::finished()
{ 
    return !_job_chain_node  ||  !_job_chain_node->_job; 
}

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state )
{
    THREAD_LOCK( _lock )
    {
        if( _job_chain )  move_to_node( _job_chain->node_from_state( state ) );
                    else  set_state2( state );
    }
}

//--------------------------------------------------------------------------------Order::set_state2

void Order::set_state2( const State& state )
{
    _state = state;
}

//------------------------------------------------------------------------------Order::set_priority

void Order::set_priority( Priority priority )
{ 
    THREAD_LOCK( _lock )
    {
        if( _priority != priority )
        {
            _priority = priority; 

            if( _in_job_queue  &&  !_task )   // Nicht gerade in Verarbeitung?
            {
                ptr<Order> hold_me = this;
                order_queue()->remove_order( this );
                order_queue()->add_order( this );
            }
        }

        _priority_modified = true;
    }
}

//-------------------------------------------------------------------------------Job_chain::com_job

Com_job* Order::com_job()
{ 
    Com_job* result = NULL;

    THREAD_LOCK( _lock )
    {
        Job* j = job();
        if( j )  result = j->com_job();
    }

    return result;
}

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_job( const string& job_name )
{
    THREAD_LOCK( _lock )
    {
        ptr<Order_queue> order_queue = _spooler->get_job( job_name )->order_queue();
        if( !order_queue )  throw_xc( "SPOOLER-147", job_name );
        add_to_order_queue( order_queue );
    }
}

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_order_queue( Order_queue* order_queue )
{
    if( !order_queue )  throw_xc( "SPOOLER-147", "?" );

    THREAD_LOCK( _lock )
    {
        _moved = true;

        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        if( _job_chain )  remove_from_job_chain();

        order_queue->add_order( this );
        _order_queue = order_queue;
    }
}

//---------------------------------------------------------------------Order::remove_from_job_chain

void Order::remove_from_job_chain()
{
    THREAD_LOCK( _lock )
    {
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
            _job_chain->unregister_order( this );
            _job_chain = NULL;
        }
    }
}

//--------------------------------------------------------------------------Order::add_to_job_chain

void Order::add_to_job_chain( Job_chain* job_chain, bool write_to_database )
{
    if( !job_chain->finished() )  throw_xc( "SPOOLER-151" );

    THREAD_LOCK( _lock )
    {
        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        if( _job_chain )  remove_from_job_chain();

        if( !job_chain->_chain.empty() )
        {
            job_chain->register_order( this );
            _job_chain = job_chain;

            if( _state.vt == VT_EMPTY )  set_state2( (*job_chain->_chain.begin())->_state );     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

            //Z_DEBUG_ONLY( LOG( "job_chain->node_from_state()\n" ); )
            Job_chain_node* node = job_chain->node_from_state( _state );

            if( !node->_job  || !node->_job->order_queue() )  throw_xc( "SPOOLER-149", job_chain->name(), error_string_from_variant(_state) );
            //Z_DEBUG_ONLY( LOG( "node->_job->order_queue()->add_order()\n" ); )
            node->_job->order_queue()->add_order( this );

            _job_chain_node = node;
        }

        if( write_to_database )  _spooler->_db->insert_order( this );
    }
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Job_chain_node* node )
{
    THREAD_LOCK( _lock )
    {
        if( !_job_chain )  throw_xc( "SPOOLER-157", obj_name() );

        _moved = true;
        _task = NULL;

        if( _job_chain_node && _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this ), _job_chain_node = NULL;

        set_state2( node? node->_state : Order::State() );
        _job_chain_node = node;

        if( node && node->_job )  node->_job->order_queue()->add_order( this );
    }
}

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( bool success, Prefix_log* log )
{
    THREAD_LOCK( _lock )
    {
        _task = NULL;

        if( !_moved )
        {
            if( _job_chain_node )
            {
                _job_chain_node->_job->order_queue()->remove_order( this );

                State new_state;

                if( success ) 
                {
                    if( log )  log->debug( "Auftrag " + obj_name() + ": Neuer Zustand ist " + error_string_from_variant(_job_chain_node->_next_state) );
                    new_state = _job_chain_node->_next_state;
                    _job_chain_node = _job_chain_node->_next_node;
                }
                else
                {
                    if( log )  log->debug( "Auftrag " + obj_name() + ": Neuer Fehler-Zustand ist " + error_string_from_variant(_job_chain_node->_error_state) );
                    new_state = _job_chain_node->_error_state;
                    _job_chain_node = _job_chain_node->_error_node;
                }

                set_state2( new_state );

                if( !finished() )  
                {
                    _job_chain_node->_job->order_queue()->add_order( this );
                }
                else 
                {
                    _end_time = Time::now();
                    if( log )  log->debug( "Auftrag " + obj_name() + ": Kein weiterer Job, Auftrag ist fertig" );
                }

            }
            else
            {
                _order_queue->remove_order( this );
                _order_queue = NULL;
            }
        }

        postprocessing2();

        _moved = false;
    }
}

//--------------------------------------------------------------------------Order::processing_error

void Order::processing_error()
{
    THREAD_LOCK( _lock )
    {
        _task = NULL;
        _moved = false;

        postprocessing2();
    }
}

//---------------------------------------------------------------------------Order::postprocessing2

void Order::postprocessing2()
{
    if( finished() )  _log.close();
    if( _job_chain )  _spooler->_db->update_order( this );
    if( finished() )  close();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
