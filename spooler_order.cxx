// $Id: spooler_order.cxx,v 1.55 2003/12/25 07:20:49 jz Exp $
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
        if( _job_chain_map.find( lname ) != _job_chain_map.end() )  throw_xc( "SCHEDULER-160", lname );

        _job_chain_map[lname] = job_chain;

        job_chain->set_finished( true );       // _finished erst, wenn Jobkette in der _job_chain_map eingetragen ist.
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

    if( _db->opened() )  job_chain->load_orders_from_database();


    _job_chain_time = Time::now();
}

//------------------------------------------------------------------------Spooler::job_chain_or_null

Job_chain* Spooler::job_chain_or_null( const string& name )
{
    Job_chain* result = NULL;

    THREAD_LOCK( _job_chain_lock )
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

                                        element.setAttribute( "state"      , debug_string_from_variant( _state       ) );
        if( !_next_state.is_empty()  )  element.setAttribute( "next_state" , debug_string_from_variant( _next_state  ) );
        if( !_error_state.is_empty() )  element.setAttribute( "error_state", debug_string_from_variant( _error_state ) );
   
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
    _spooler(spooler),
    _log(_spooler)
{
    set_name( "" );     // Ruft _log.set_prefix()
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

void Job_chain::load_orders_from_database()
{
    int count = 0;

    {
        Transaction ta ( _spooler->_db );

        Any_file sel ( "-in " + _spooler->_db->db_name() + "-max-length=32K "
                       " select \"ID\", \"PRIORITY\", \"STATE\", \"STATE_TEXT\", \"TITLE\", \"CREATED_TIME\", \"PAYLOAD\""
                       " from " + sql::quoted_name( _spooler->_orders_tablename ) +
                       " where \"SPOOLER_ID\"=" + sql::quoted(_spooler->id_for_db()) + 
                       " and \"JOB_CHAIN\"=" + sql::quoted(_name) +
                       " order by \"ORDERING\"" );

        while( !sel.eof() )
        {
            ptr<Order> order = new Order( _spooler, sel.get_record() );
            order->_is_in_database = true;
            order->add_to_job_chain( this );    // Einstieg nur über Order, damit Semaphoren stets in derselben Reihenfolge gesperrt werden.
            count++;
        }
    }

    _log.debug( as_string(count) + " Aufträge aus der Datenbank gelesen" );
}


//-------------------------------------------------------------------------------Job_chain::add_job

void Job_chain::add_job( Job* job, const Order::State& state, const Order::State& next_state, const Order::State& error_state )
{
    if( job  &&  !job->order_queue() )  throw_xc( "SCHEDULER-147", job->name() );

    if( _finished )  throw_xc( "SCHEDULER-148" );

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
            throw_xc( "SCHEDULER-150", debug_string_from_variant(node->_state), name() );
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

    throw_xc( "SCHEDULER-152", job->name(), name() );
    return NULL;
}

//-----------------------------------------------------------------------Job_chain::node_from_state

Job_chain_node* Job_chain::node_from_state( const State& state )
{
    Job_chain_node* result = node_from_state_or_null( state );
    if( !result )  throw_xc( "SCHEDULER-149", name(), debug_string_from_variant(state) );
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
    ptr<Order> result = order_or_null( id );
    
    if( !result )  throw_xc( "SCHEDULER-162", debug_string_from_variant(id), _name );
    
    return result;
}

//-------------------------------------------------------------------------Job_chain::order_or_null

ptr<Order> Job_chain::order_or_null( const Order::Id& id )
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
        if( it != _order_map.end() )  throw_xc( "SCHEDULER-186", id_string, _name );
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
            Queue* queues[] = { &_queue, &_setback_queue };
            for( Queue** q = queues; q < queues + NO_OF(queues); q++ )
            {
                FOR_EACH( Queue, **q, it )
                {
                    Order* order = *it;
                    if( !which_job_chain  ||  order->job_chain() == which_job_chain )
                    {
                        dom_append_nl( element );
                        element.appendChild( order->dom( document, show ) );
                    }
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
        
        THREAD_LOCK( _lock ) 
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

void Order_queue::add_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    THREAD_LOCK( _lock )
    {
        if( order->_setback )
        {
            _log->debug( "add_order (setback queue) " + order->obj_name() );

            // Auftrag nach Rückstellungszeitpunkt (und Priorität) geordnet in die _setback_queue einfügen:

            Queue::iterator it;
            for( it = _setback_queue.begin(); it != _setback_queue.end(); it++ )  
            {
                Order* o = *it;
                if( o->_setback > order->_setback )  break;
                if( o->_setback == order->_setback  &&  o->_priority > order->_priority )  break;
            }

            _setback_queue.insert( it, order );
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
            bool            was_empty = _queue.empty();
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

            order->_in_job_queue = true;

            if( was_empty )  _job->signal( "Order" );
        }
    }
}

//------------------------------------------------------------------------Order_queue::remove_order

void Order_queue::remove_order( Order* order )
{
    // Wird von Order mit geperrtem order->_lock gerufen.

    THREAD_LOCK( _lock )
    {
        if( order->_setback )
        {
            _log->debug9( "remove_order (setback) " + order->obj_name() );

            Queue::iterator it;
            for( it = _setback_queue.begin(); it != _setback_queue.end(); it++ )  if( *it == order )  break;

            if( it == _setback_queue.end() )  throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

            _setback_queue.erase( it );

            order->_setback = 0;
        }
        else
        {
            _log->debug9( "remove_order " + order->obj_name() );

            Queue::iterator it;
            for( it = _queue.begin(); it != _queue.end(); it++ )  if( *it == order )  break;

            if( it == _queue.end() )  throw_xc( "SCHEDULER-156", order->obj_name(), _job->name() );

            _queue.erase( it );
        //_id_map.erase( order->_id );
            update_priorities();

            order->_in_job_queue = false;
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
}

//-------------------------------------------------------------------------Order_queue::first_order

Order* Order_queue::first_order( const Time& now )
{
    // SEITENEFFEKT: Aufträge aus der _setback_queue, deren Rückstellungszeitpunkt erreicht ist, werden in die _queue verschoben.

    //Order* order = NULL;

    THREAD_LOCK( _lock )
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


        //if( !_queue.empty() )  order = *_queue.begin();
        FOR_EACH( Queue, _queue, o )  if( !(*o)->_task )  return *o;
    }

    //return order;
    return NULL;
}

//------------------------------------------------------------Order_queue::get_order_for_processing

ptr<Order> Order_queue::get_order_for_processing( const Time& now )
{
    // Die Order_queue gehört genau einem Job. Der Job kann zur selben Zeit nur einen Schritt ausführen.
    // Deshalb kann nur der erste Auftrag in Verarbeitung sein.

    ptr<Order> order;

    THREAD_LOCK( _lock )
    {
        order = first_order( now );

        if( order )
        {
            order->_start_time = now;
            order->_setback = 0;
            order->_moved = false;
        }   
    }

    return order;
}

//---------------------------------------------------------------------------Order_queue::next_time

Time Order_queue::next_time()
{
    THREAD_LOCK( _lock )
    {
        if( !_queue.empty() )  return latter_day;

        if( !_setback_queue.empty() )  return (*_setback_queue.begin())->_setback;
    }

    return latter_day;
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
    init();

    _id         = record.as_string( "id"         );
    _state      = record.as_string( "state"      );
    _state_text = record.as_string( "state_text" );
    _title      = record.as_string( "title"      );
    _priority   = record.as_int   ( "priority"   );

    string payload_string = record.as_string( "payload" );

    //LOG( "db payload=" << payload_string << "\n" );
    if( payload_string.find( "<" + Com_variable_set::xml_element_name() ) != string::npos )
    {
        //LOG( "... payload ist ein Variable_set!\n" );
        ptr<Com_variable_set> v = new Com_variable_set;
        v->put_xml( Bstr( payload_string ) );
        _payload = v;
    }
    else
    {
        if( payload_string.empty() )  _payload = (IDispatch*)NULL;
                                else  _payload = payload_string;
    }

    _created.set_datetime( record.as_string( "created_time" ) );
}

//------------------------------------------------------------------------------------Order::~Order

Order::~Order()
{
}

//--------------------------------------------------------------------------------------Order::init

void Order::init()
{
    _log.set_prefix( "Order" );
    _created = Time::now();
}

//-------------------------------------------------------------------------------Order::attach_task

void Order::attach_task( Task* task )
{
    _task = task;
    if( !_log.opened() )  open_log();
}

//----------------------------------------------------------------------------------Order::open_log

void Order::open_log()
{
    if( _job_chain && _spooler->_order_history_with_log && !string_begins_with( _spooler->log_directory(), "*" ) )
    {
        _log.set_filename( _spooler->log_directory() + "/order." + _job_chain->name() + "." + _id.as_string() + ".log" );      // Jobprotokoll
        _log.set_remove_after_close( true );
        _log.open();
    }
}

//-------------------------------------------------------------------------------------Order::close

void Order::close()
{
/*
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
*/

    remove_from_job_chain();
}

//---------------------------------------------------------------------------------------Order::dom

xml::Element_ptr Order::dom( const xml::Document_ptr& document, Show_what show, const string* log )
{
    xml::Element_ptr element = document.createElement( "order" );

    THREAD_LOCK( _lock )
    {
        element.setAttribute( "id"        , debug_string_from_variant( _id ) );
        element.setAttribute( "title"     , _title );
        element.setAttribute( "state"     , debug_string_from_variant( _state ) );

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

        if( _log.opened() )
        element.setAttribute( "log_file"  , _log.filename() );

        if( _setback )
        element.setAttribute( "setback"   , _setback.as_string() );


        if( show & show_log )
        {
            try
            {
                dom_append_text_element( element, "log", log? *log : _log.as_string() );
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
    THREAD_LOCK(_lock)
    {
        if( _id_locked )  throw_xc( "SCHEDULER-159" );

        _id = id; 
        _is_users_id = true;

        _log.set_prefix( "Order " + _id.as_string() );
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
        if( !_job_chain )  throw_xc( "SCHEDULER-157", obj_name() );
        
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

//-------------------------------------------------------------------------------Order::set_payload

void Order::set_payload( const VARIANT& payload )
{ 
    THREAD_LOCK( _lock )  
    { 
        LOG2( "scheduler.order", "Order " << obj_name() << ".payload=" << debug_string_from_variant(payload) << "\n" );
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
    THREAD_LOCK( _lock )
    {
        //if( _setback )  throw_xc( "SCHEDULER-188" );
        _setback = 0;
        _setback_count = 0;

        if( _job_chain )  move_to_node( _job_chain->node_from_state( state ) );
                    else  set_state2( state );
    }
}

//--------------------------------------------------------------------------------Order::set_state2

void Order::set_state2( const State& state, bool is_error_state )
{
    string log_line = "set_state " + state.as_string();

    if( _job_chain_node && _job_chain_node->_job )  log_line += ", " + _job_chain_node->_job->obj_name();
    if( is_error_state                           )  log_line += ", Fehlerzustand";

    if( _job_chain )  _log.info( log_line );

    THREAD_LOCK( _lock )  _state = state;
}

//------------------------------------------------------------------------------Order::set_priority

void Order::set_priority( Priority priority )
{ 
    THREAD_LOCK( _lock )
    {
        if( _priority != priority )
        {
            _priority = priority; 

            if( !_setback  &&  _in_job_queue  &&  !_task)   // Nicht gerade in Verarbeitung?
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
        if( !order_queue )  throw_xc( "SCHEDULER-147", job_name );
        add_to_order_queue( order_queue );
    }
}

//------------------------------------------------------------------------Order::add_to_order_queue

void Order::add_to_order_queue( Order_queue* order_queue )
{
    if( !order_queue )  throw_xc( "SCHEDULER-147", "?" );

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

void Order::add_to_job_chain( Job_chain* job_chain )
{
    if( !job_chain->finished() )  throw_xc( "SCHEDULER-151" );

    THREAD_LOCK( _lock )
    {
        if( _id.vt == VT_EMPTY )  set_default_id();
        _id_locked = true;

        if( _job_chain )  remove_from_job_chain();

        if( !job_chain->_chain.empty() )
        {
            job_chain->register_order( this );

            if( _state.vt == VT_EMPTY )  set_state2( (*job_chain->_chain.begin())->_state );     // Auftrag bekommt Zustand des ersten Jobs der Jobkette

            //Z_DEBUG_ONLY( LOG( "job_chain->node_from_state()\n" ); )
            Job_chain_node* node = job_chain->node_from_state( _state );

            if( !node->_job  || !node->_job->order_queue() )  throw_xc( "SCHEDULER-149", job_chain->name(), debug_string_from_variant(_state) );
            //Z_DEBUG_ONLY( LOG( "node->_job->order_queue()->add_order()\n" ); )
            node->_job->order_queue()->add_order( this );

            _job_chain = job_chain;
            _job_chain_node = node;
        }

        if( !_is_in_database )  _spooler->_db->insert_order( this ),  _is_in_database = true;
    }
}

//------------------------------------------------------------------------------Order::move_to_node

void Order::move_to_node( Job_chain_node* node )
{
    THREAD_LOCK( _lock )
    {
        if( !_job_chain )  throw_xc( "SCHEDULER-157", obj_name() );

        _moved = true;
        _task = NULL;

        if( _job_chain_node && _in_job_queue )  _job_chain_node->_job->order_queue()->remove_order( this ), _job_chain_node = NULL;

        _job_chain_node = node;

        set_state2( node? node->_state : Order::State() );

        if( node && node->_job )  node->_job->order_queue()->add_order( this );
    }
}

//----------------------------------------------------------------------------Order::postprocessing

void Order::postprocessing( bool success )
{
    THREAD_LOCK( _lock )
    {
        bool force_error_state = false;
        
        _task = NULL;

        if( _setback )
        {
            if( _setback == latter_day )
            {
                _log.debug( as_string(_setback_count) + " mal zurückgestellt. Der Auftrag wechselt in den Fehlerzustand" );
                success = false;
                force_error_state = true;
            }
        }

        if( !_setback && !_moved  ||  force_error_state )
        {
            _setback_count = 0;

            if( _job_chain_node )
            {
                _job_chain_node->_job->order_queue()->remove_order( this );

                State new_state;

                if( success ) 
                {
                    //_log.debug( "Neuer Zustand ist " + error_string_from_variant(_job_chain_node->_next_state) );
                    new_state = _job_chain_node->_next_state;
                    _job_chain_node = _job_chain_node->_next_node;
                }
                else
                {
                    //_log.debug( "Neuer Fehler-Zustand ist " + error_string_from_variant(_job_chain_node->_error_state) );
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
                    //_end_time = Time::now();
                    _log.debug( "Kein weiterer Job in der Jobkette, der Auftrag ist erledigt" );
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
    if( finished() ) 
    {
        _end_time = Time::now();
        _log.close();
    }

    if( _job_chain  &&  _is_in_database )  _spooler->_db->update_order( this );
    
    if( finished() )  close();
}

//----------------------------------------------------------------------------------Order::setback_

void Order::setback_()
{
    THREAD_LOCK( _lock )
    {
        if( !_task      )  throw_xc( "SCHEDULER-187" );
        if( _moved      )  throw_xc( "SCHEDULER-188", obj_name() );
        if( !_job_chain )  throw_xc( "SCHEDULER-157", obj_name() );

        order_queue()->remove_order( this );

        _setback_count++;

        int maximum = _task->job()->max_order_setbacks();
        if( _setback_count <= maximum )
        {
            _setback = Time::now() + _task->job()->get_delay_order_after_setback( _setback_count );
            _log.info( "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, bis " + _setback.as_string() );
        }
        else
        {
            _setback = latter_day;  // Das heißt: Der Auftrag kommt in den Fehlerzustand
            _log.warn( "setback(): Auftrag zum " + as_string(_setback_count) + ". Mal zurückgestellt, "
                       "das ist über dem Maximum " + as_string(maximum) + " des Jobs" );
        }

        order_queue()->add_order( this );

        // Weitere Verarbeitung in postprocessing()
    }
}

//----------------------------------------------------------------------------------Order::obj_name

string Order::obj_name()
{ 
    string result;

    THREAD_LOCK( _lock )
    {
        if( _job_chain )  result = _job_chain->name() + " ";

        result += debug_string_from_variant(_id) + rtrim( "  " + _title );

        if( _setback )
            if( _setback == latter_day )  result += ", setback (Maximum erreicht)";
                                    else  result += ", setback=" + _setback.as_string();
      //else
      //if( _priority )  result += ", pri=" + as_string( _priority );
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
