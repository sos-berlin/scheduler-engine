// $Id: spooler_order.cxx,v 1.17 2002/11/08 18:56:35 jz Exp $
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

    _job_chain_time = Time::now();
}

//-------------------------------------------------------------------------------Spooler::job_chain

Job_chain* Spooler::job_chain( const string& name )
{
    Job_chain* result;

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

xml::Element_ptr Spooler::xml_from_job_chains( xml::Document_ptr document, Show_what show )
{
    xml::Element_ptr job_chains_element = document->createElement( "job_chains" );

        dom_append_nl( job_chains_element );

        THREAD_LOCK( _job_chain_lock )
        {
            FOR_EACH( Job_chain_map, _job_chain_map, it )
            {
                Job_chain* job_chain = it->second;
                job_chains_element->appendChild( job_chain->xml( document, show ) );
                dom_append_nl( job_chains_element );            
            }
        }

    return job_chains_element;
}

//-------------------------------------------------------------xml::Element_ptr Job_chain_node::xml

xml::Element_ptr Job_chain_node::xml( xml::Document_ptr document, Show_what show )
{
    xml::Element_ptr element = document->createElement( "job_chain_node" );

                                           element->setAttribute( "state"      , _state       );
        if( _next_state.vt  != VT_ERROR )  element->setAttribute( "next_state" , _next_state  );
        if( _error_state.vt != VT_ERROR )  element->setAttribute( "error_state", _error_state );
   
        if( _job )
        {
            element->setAttribute( "job", _job->name().c_str() );

            //if( show & show_orders )  
            {
                dom_append_nl( element );
                element->appendChild( _job->xml( document, show ) );
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

//------------------------------------------------------------------xml::Element_ptr Job_chain::xml

xml::Element_ptr Job_chain::xml( xml::Document_ptr document, Show_what show )
{
    xml::Element_ptr element = document->createElement( "job_chain" );

        THREAD_LOCK( _lock )
        {
            element->setAttribute( "name", _name.c_str() );
    
            if( _finished )
            {
                dom_append_nl( element );

                FOR_EACH( Chain, _chain, it )
                {
                    Job_chain_node* node = *it;
                    element->appendChild( node->xml( document, show ) );
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
        VARIANT v;
        v.vt = VT_ERROR;
        return v;
    }
    else
    {
        return state;
    }
}

//-------------------------------------------------------------------------------Job_chain::add_job

void Job_chain::add_job( Job* job, const Order::State& state, const Order::State& next_state, const Order::State& error_state )
{
    if( job  &&  !job->order_queue() )  throw_xc( "SPOOLER-147", job->name() );

    if( _finished )  throw_xc( "SPOOLER-148" );

    ptr<Job_chain_node> node = new Job_chain_node;

    node->_job   = job;
    node->_state = state;

    if( node->_state.vt == VT_ERROR )  node->_state = job->name();

    node->_next_state  = normalized_state( next_state );
    node->_error_state = normalized_state( error_state );

    THREAD_LOCK( _lock )
    {
        if( node_from_state_or_null( node->_state ) )  
        {
            if( !job  &&  next_state.vt == VT_ERROR  &&  error_state.vt == VT_ERROR )  return;     // job_chain.add_end_state() darf mehrfach gerufen werden.
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
            VARIANT error; VariantInit( &error );  error.vt = VT_ERROR;
            if( n->_job  &&  n->_next_state.vt == VT_ERROR )  add_job( NULL, "<END_STATE>", error, error );    // Endzustand fehlt? Dann hinzufügen
        }

        for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
        {
            Job_chain_node* n = *it;
            Chain::iterator next = it;  next++;

            if( n->_next_state.vt == VT_ERROR  &&  next != _chain.end() )  n->_next_state = (*next)->_state;

            if( n->_next_state.vt  != VT_ERROR )  n->_next_node  = node_from_state( n->_next_state );
            if( n->_error_state.vt != VT_ERROR )  n->_error_node = node_from_state( n->_error_state );
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
    if( !result )  throw_xc( "SPOOLER-149", error_string_from_variant(state), name() );
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

//---------------------------------------------------------------------------------Order_queue::xml

xml::Element_ptr Order_queue::xml( xml::Document_ptr document, Show_what show )
{
    xml::Element_ptr element = document->createElement( "order_queue" );

    THREAD_LOCK( _lock )
    {
        element->setAttribute( "length", as_dom_string( length() ) );

        if( show & show_orders )
        {
            FOR_EACH( Queue, _queue, it )
            {
                dom_append_nl( element );
                element->appendChild( (*it)->xml( document, show ) );
            }

            dom_append_nl( element );
        }
    }

    return element;
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

    THREAD_LOCK( _lock )
    {
        for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

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

    ptr<Order> result;

    THREAD_LOCK( _lock )
    {
        if( !_queue.empty() )
        {
            result = _queue.front();

            if( result->_task )  throw_xc( "Order_queue::get_order_for_processing" );   // Darf nicht passieren
            
            result->_task = task;
            result->_moved = false;
        }   
    }

    return result;
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
    _payload(payload)
{
    init();
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    _created = Time::now();
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
}

//---------------------------------------------------------------------------------------Order::xml

xml::Element_ptr Order::xml( xml::Document_ptr document, Show_what show )
{
    xml::Element_ptr element = document->createElement( "order" );

    THREAD_LOCK( _lock )
    {
        element->setAttribute( "id"        , _id );
        element->setAttribute( "title"     , _title.c_str() );
        element->setAttribute( "state"     , _state );

        if( _job_chain )  
        element->setAttribute( "job_chain" , _job_chain->name().c_str() );

        Job* job = this->job();
        if( job )
        element->setAttribute( "job"       , job->name().c_str() );

        if( _task )
        element->setAttribute( "in_process_since", as_dom_string( _task->last_process_start_time().as_string() ) );

        element->setAttribute( "state_text", _state_text.c_str() );
        element->setAttribute( "priority"  , as_dom_string( _priority ) );
        element->setAttribute( "created"   , as_dom_string( _created.as_string() ) );
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
            set_id( _spooler->get_free_order_id() );  
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

//---------------------------------------------------------------------------------Order::set_state

void Order::set_state( const State& state )
{
    THREAD_LOCK( _lock )
    {
        if( _job_chain )  move_to_node( _job_chain->node_from_state( state ) );
                    else  _state = state;
    }
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
    }
}

//-------------------------------------------------------------------------------Job_chain::com_job

Com_job* Order::com_job()
{ 
    Com_job* result;

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
            _job_chain_node->_job->order_queue()->remove_order( this );

            _job_chain      = NULL;
            _job_chain_node = NULL;
        }
    }
}

//--------------------------------------------------------------------------Order::add_to_job_chain

void Order::add_to_job_chain( Job_chain* job_chain )
{
    if( !job_chain->finished() )  throw_xc( "SPOOLER-151" );

    THREAD_LOCK( _lock )
    {
        if( _id.vt == VT_EMPTY )  set_default_id(); 
        
        _id_locked = true;

        if( _job_chain )  remove_from_job_chain();

        if( !job_chain->_chain.empty() )
        {
            if( _state.vt == VT_EMPTY )  _state = (*job_chain->_chain.begin())->_state;     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

            Job_chain_node* node = job_chain->node_from_state( _state );

            node->_job->order_queue()->add_order( this );

            _job_chain      = job_chain;
            _job_chain_node = node;
        }
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

        if( _job_chain_node && _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this );

        _state = node? node->_state : Order::State();
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

                if( success ) 
                {
                    if( log )  log->debug( "Auftrag " + obj_name() + ": Neuer Zustand ist " + error_string_from_variant(_job_chain_node->_next_state) );
                    _state = _job_chain_node->_next_state;
                    _job_chain_node = _job_chain_node->_next_node;
                }
                else
                {
                    if( log )  log->debug( "Auftrag " + obj_name() + ": Neuer Fehler-Zustand ist " + error_string_from_variant(_job_chain_node->_error_state) );
                    _state = _job_chain_node->_error_state;
                    _job_chain_node = _job_chain_node->_error_node;
                }

                if( _job_chain_node  &&  _job_chain_node->_job )  _job_chain_node->_job->order_queue()->add_order( this );
                else 
                if( log )  log->debug( "Auftrag " + obj_name() + ": Kein weiterer Job, Auftrag ist fertig" );
            }
            else
            {
                _order_queue->remove_order( this );
                _order_queue = NULL;
            }
        }

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
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
