// $Id: spooler_order.cxx,v 1.1 2002/09/12 19:35:15 jz Exp $
/*
    Hier sind implementiert

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
    string lname = lcase( job_chain->name() );

    THREAD_LOCK( _job_chain_lock )
    {
        if( _job_chain_map.find( lname ) != _job_chain_map.end() )  throw_xc( "SPOOLER-160", lname );

        job_chain->finish();

        _job_chain_map[lname] = job_chain;
    }
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

//-----------------------------------------------------------------------------Job_chain::Job_chain

Job_chain::Job_chain( Spooler* spooler )
:
    Com_job_chain( this ),
    _zero_(this+1),
    _spooler(spooler)
  //_com_job_chain( new Com_job_chain(this) ),
{
}

//----------------------------------------------------------------------------Job_chain::~Job_chain

Job_chain::~Job_chain()
{
    //if( _com_job_chain )  _com_job_chain->close();
}

//---------------------------------------------------------------------------------Job_chain::close
/*
void Job_chain::close()
{
    THREAD_LOCK( _lock )
    {
        if( _com_job_chain )  _com_job_chain->close(), _com_job_chain = NULL;
    }
}
*/
//-------------------------------------------------------------------------------Job_chain::add_job

void Job_chain::add_job( Job* job, const Variant& state, const Variant& next_state, const Variant& error_state )
{
    if( job  &&  !job->order_queue() )  throw_xc( "SPOOLER-147", job->name() );

    if( _finished )  throw_xc( "SPOOLER-148" );

    ptr<Job_chain_node> node = Z_NEW( Job_chain_node );

    node->_job         = job;
    node->_state       = state;

    if( node->_state.vt == VT_EMPTY || node->_state.vt == VT_ERROR )  node->_state = combstr_from_string( job->name() );

    node->_next_state  = next_state;   if( node->_next_state.vt  == VT_ERROR )  node->_next_state.vt  = VT_EMPTY;
    node->_error_state = error_state;  if( node->_error_state.vt == VT_ERROR )  node->_error_state.vt = VT_EMPTY;

    if( _map.find( node->_state ) != _map.end() )  throw_xc( "SPOOLER-150", error_string_from_variant(node->_state), name() );

    _chain.push_back( node );
    _map[ node->_state ] = node;
}

//--------------------------------------------------------------------------------Job_chain::finish

void Job_chain::finish()
{
    if( _finished )  return;

    for( Chain::iterator it = _chain.begin(); it != _chain.end(); it++ )
    {
        Job_chain_node* n = *it;
        
        Chain::iterator next = it;  next++;

        if( n->_next_state.vt == VT_EMPTY  &&  next != _chain.end() )  n->_next_state = (*next)->_state;

        Map::iterator next_it  = _map.find( n->_next_state  );
        Map::iterator error_it = _map.find( n->_error_state );

        if( next_it != _map.end() )
            n->_next_node = next_it->second;
        else
        if( n->_next_state.vt  != VT_EMPTY )  throw_xc( "SPOOLER-149", error_string_from_variant(n->_next_state), name() );

        if( error_it != _map.end() )
            n->_error_node = error_it->second;
        else
        if( n->_error_state.vt != VT_EMPTY )  throw_xc( "SPOOLER-149", error_string_from_variant(n->_error_state), name() );

    }

    _finished = true;
}

//-------------------------------------------------------------------------Job_chain::node_from_job

Job_chain_node* Job_chain::node_from_job( Job* job )
{
    THREAD_LOCK( _lock )
    {
        for( Map::iterator it = _map.begin(); it != _map.end(); it++ )
        {
            Job_chain_node* n = it->second;
            if( n->_job == job )  return n;
        }

        throw_xc( "SPOOLER-152", job->name(), name() );
    }

    return NULL;
}

//-----------------------------------------------------------------------Job_chain::node_from_state

Job_chain_node* Job_chain::node_from_state( const State& state )
{
    Map::iterator it = _map.find( state );
    if( it == _map.end() )  throw_xc( "SPOOLER-149", error_string_from_variant(state), name() );

    return it->second;
}

//-----------------------------------------------------------------------------Job_chain::add_order
/*
Order* Job_chain::add_order( VARIANT* order_or_payload, VARIANT* job_or_state )
{
    //HRESULT hr;

    CComPtr<spooler_com::Iorder> iorder;

/*
    if( order_or_payload->vt == VT_DISPATCH  ||  order_or_payload->vt == VT_UNKNOWN )
    {
        hr = V_UNKNOWN(order_or_payload)->QueryInterface( spooler_com::IID_Iorder, (void**)&iorder );
        if( FAILED(hr) )  iorder = NULL;
    }
* /

    if( !iorder )
    {
        ptr<Order> order = Z_NEW( Order( _spooler, *order_or_payload ) );
        iorder = order->com_order();
    }

    dynamic_cast<Com_order*>( &*iorder )->add_to_job_chain( this );  //, job_or_state );
    
    // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
}

*/
//-----------------------------------------------------------------------------Job_chain::add_order
/*
void Job_chain::add_order( Order* order )
{
    if( !_finished )  throw_xc( "SPOOLER-151" );

    if( !_chain.empty() )
    {
        Job_chain_node* node = *_chain.begin();

        node->_job->order_queue()->add_order( this );

        _job_chain      = job_chain;
        _job_chain_node = node;
    }
}
*/
//-----------------------------------------------------------------------------Job_chain::add_order
/*
ptr<Order> Job_chain::add_order( const Order::Payload& payload )
{
    ptr<Order> order = Z_NEW( Order( _spooler, payload ) );

    add_order( order );

    return order;
}
*/
//-------------------------------------------------------------------------Order_queue::Order_queue

Order_queue::Order_queue( Job* job )
: 
    _zero_(this+1),
    _spooler(job->_spooler), 
    _com_order_queue( new Com_order_queue(this) ),
    _job(job)
{
}

//------------------------------------------------------------------------Order_queue::~Order_queue

Order_queue::~Order_queue()
{
    if( _com_order_queue )  _com_order_queue->close();
}

//---------------------------------------------------------------------------Order_queue::add_order

void Order_queue::add_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    THREAD_LOCK( _lock )
    {
        Queue::iterator ins = _queue.begin();

        for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
        {
            if( (*it)->priority() > order->priority() )  ins = it;
            if( (*it)->id() == order->id() )  throw_xc( "SPOOLER-153", error_string_from_variant( order->id() ), _job->name() );
        }

        bool was_empty = _queue.empty();

        _queue.insert( ins, order );

        order->_in_job_queue = true;

        if( was_empty )  _job->signal( "Order" );
    }
}

//---------------------------------------------------------------------------Order_queue::add_order
/*
Order* Order_queue::add_order( const Variant& payload )
{
    ptr<Order> order = Z_NEW( Order(_spooler) );
    order->set_payload( payload );
    return add_order( order );
}
*/
//------------------------------------------------------------------------Order_queue::remove_order

void Order_queue::remove_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    THREAD_LOCK( _lock )
    {
        //Map::iterator it = _map.find( order->_id );
    
        for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

        if( it == _queue.end() )  throw_xc( "SPOOLER-156", order->obj_name(), _job->name() );

        _queue.erase( it );

        order->_in_job_queue = false;
    }
}

//---------------------------------------------------------------------------------Order_queue::pop

ptr<Order> Order_queue::pop()
{
    ptr<Order> result;

    THREAD_LOCK( _lock )
    {
        if( !_queue.empty() )
        {
            result = _queue.front();
            result->_in_job_queue = false;
            result->_moved = false;
            
            _queue.pop_front();
        }   
    }

    return result;
}

//-------------------------------------------------------------------------------------Order::Order

Order::Order( Spooler* spooler, const Payload& payload )
: 
    _zero_(this+1), 
    _spooler(spooler),
    _payload(payload),
  //_com_order( new Com_order(this) )
     Com_order(this)
{
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
    //if( _com_order )  _com_order->close();
}

//-----------------------------------------------------------------------------------Order::set_job
/*
void Order::set_job( spooler_com::Ijob* ijob )
{
    THREAD_LOCK( _lock )
    {
        Job* job = NULL;

        if( ijob )
        {
            job = dynamic_cast<Com_job*>( ijob )->_job;
        }

        set_job( job );
    }
}
*/

//---------------------------------------------------------------------------Order::set_job_by_name

void Order::set_job_by_name( const string& jobname )
{
    set_job( _spooler->get_job( jobname ) );
}

//------------------------------------------------------------------------------------Order::set_id

void Order::set_id( const Variant& id )
{ 
    THREAD_LOCK(_lock)
    {
        if( _job_chain )  throw_xc( "SPOOLER-159" );
      //if( _job )  throw_xc( "SPOOLER-159" );

        _id = id; 
    }
}

//-----------------------------------------------------------------------------------Order::set_job

void Order::set_job( Job* job )
{
    THREAD_LOCK( _lock )
    {
        if( _job_chain )
        {
            move_to_node( _job_chain->node_from_job( job ) );       // Fehler, wenn Job nicht in der Jobkette
        }
        else
            throw_xc( "SPOOLER-157", obj_name() );
    }
}

//---------------------------------------------------------------------------------------Order::job

Job* Order::job()
{
    Job* result = NULL;

    THREAD_LOCK( _lock )
    {
        if( _job_chain_node )  result = _job_chain_node->_job;
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

//-------------------------------------------------------------------------------------Order::state

Order::State Order::state()
{
    THREAD_LOCK_RETURN( _lock, State, _state );
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

//-------------------------------------------------------------------------Job_chain::com_job_chain
/*
CComPtr<Com_job_chain> Order::com_job_chain()
{ 
    THREAD_LOCK_RETURN( _lock, CComPtr<Com_job_chain>, _job_chain? _job_chain->com_job_chain() : NULL ); 
}
*/
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
        if( _job_chain )  remove_from_job_chain();
        else
        if( _id.vt == VT_EMPTY )  set_id( _spooler->get_free_order_id() );  //set_id( variant_from_string( "spooler_id." + as_string( _spooler->get_free_order_id() ) ) );

/*
        CComPtr<spooler_com::Ijob> job;

        if( job_or_state.vt == VT_DISPATCH  ||  job_or_state.vt == VT_UNKNOWN )
        {
            hr = V_UNKNOWN(job_or_state)->QueryInterface( spooler_com::IID_Ijob, (void**)&job );
            if( FAILED(hr) )  job = NULL;
        }

        if( job )
*/


        if( !job_chain->_chain.empty() )
        {
            Job_chain_node* node = _state.vt == VT_EMPTY? *job_chain->_chain.begin()
                                                        : job_chain->node_from_state( _state );

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
        if( !_job_chain )  throw_xc( "SPOOLER-157", error_string_from_variant(_id) );

        if( _job_chain_node && _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this );

        _state = node->_state;
        _job_chain_node = node;

        if( node->_job )  node->_job->order_queue()->add_order( this );

        _moved = true;
    }
}

//-------------------------------------------------------------------------------Order::move_to_job
/*
void Order::move_to_job( Job* job )
{
    THREAD_LOCK( _lock )
    {
        if( _job )  _job->order_queue()->remove_order( this );
        _job = NULL;

        if( job  )   job->order_queue()->add_order( this );
        _job = job;
    }
}
*/
//---------------------------------------------------------------------------Order::set_job_by_name
/*
void Order::set_job_by_name( const string& jobname )
{
    set_job( _spooler->get_job( jobname ) );
}
*/

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( bool success, Prefix_log* log )
{
    THREAD_LOCK( _lock )
    {
        if( !_moved )
        {
            if( _job_chain_node )
            {
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
        }

        _moved = false;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
