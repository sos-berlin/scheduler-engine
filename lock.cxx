// $Id: spooler_job.cxx 4910 2007-03-23 23:15:25Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"


namespace sos {
namespace scheduler {

//-------------------------------------------------------------------Scheduler_lock::Scheduler_lock

Scheduler_lock::Scheduler_lock( Lock_subsystem* lock_subsystem )
:
    Scheduler_object( lock_subsystem->_spooler, this, type_lock ),
    _zero_(this+1),
    _lock_subsystem(lock_subsystem),
    _max_non_exclusive(INT_MAX),
    _waiting_queues(2)
{
}

//------------------------------------------------------------------Scheduler_lock::~Scheduler_lock

Scheduler_lock::~Scheduler_lock()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << ": " << x.what() ); }
}

//----------------------------------------------------------------------------Scheduler_lock::close

void Scheduler_lock::close()
{
    for( Holder_set::iterator it = _holder_set.begin(); it != _holder_set.end(); )
    {
        Lock_holder* holder = *it;
        log()->error( message_string( "SCHEDULER-854", obj_name(), holder->obj_name() ) );
    }

    _waiting_queues.clear();
}

//--------------------------------------------------------------------------Scheduler_lock::set_dom

void Scheduler_lock::set_dom( const xml::Element_ptr& lock_element )
{
    assert( lock_element.nodeName_is( "lock" ) );

    _name              = lock_element.    getAttribute( "name" );
    _max_non_exclusive = lock_element.int_getAttribute( "max_non_exclusive", _max_non_exclusive );

    _log->set_prefix( obj_name() );
}

//----------------------------------------------------------------------Scheduler_lock::dom_element

xml::Element_ptr Scheduler_lock::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock" );

    result.setAttribute( "name", _name );
    if( _max_non_exclusive < INT_MAX )  result.setAttribute( "max_non_exclusive", _max_non_exclusive );

    return result;
}

//-----------------------------------------------------------------Scheduler_lock::request_lock_for

bool Scheduler_lock::is_available_for( const Lock_requestor* lock_requestor )
{
    bool result = false;

    if( _waiting_queues[ lock_requestor->lock_mode() ].empty()  ||                       // Warteschlange ist leer?
       *_waiting_queues[ lock_requestor->lock_mode() ].begin() == lock_requestor )       // Wir sind dran?
    {
        if( lock_requestor->lock_mode() == lk_exclusive )
        {
            if( _holder_set.empty() )
            {
                result = true;
            }
        }
        else 
        if( _waiting_queues[ lk_exclusive ].empty()  &&      // Keine exklusive Sperre verlangt?
            !is_exclusive()  &&                             // Nicht exklusive gesperrt?
            _holder_set.size() < _max_non_exclusive )
        {
            result = true;
        }
    }

    return result;
}

//-----------------------------------------------------------------Scheduler_lock::request_lock_for

bool Scheduler_lock::request_lock_for( Lock_holder* holder )
{
    bool result = false;

    //assert( holder->_is_locked _state == Lock_holder::s_is_holding_lock )  z::throw_xc( __FUNCTION__ );

    if( is_available_for( holder->lock_requestor() ) )
    {        
        _holder_set.insert( holder );
        result = true;
    }

    return result;
}

//-----------------------------------------------------------------Scheduler_lock::release_lock_for

void Scheduler_lock::release_lock_for( Lock_holder* holder )
{
    Holder_set::iterator it = _holder_set.find( holder );

    if( it != _holder_set.end() )  _holder_set.erase( it );
                             else  Z_DEBUG_ONLY( assert( !"Unbekannte Sperre" ) );

    if( !_waiting_queues[ lk_exclusive ].empty() )  
    {
        ( *_waiting_queues[ lk_exclusive ].begin() ) -> on_lock_is_available( this );
    }
    else
    if( !_waiting_queues[ lk_non_exclusive ].empty() )  
    {
        ( *_waiting_queues[ lk_non_exclusive ].begin() ) -> on_lock_is_available( this );
    }
}

//-----------------------------------------------------------Scheduler_lock::enqueue_lock_requestor

int Scheduler_lock::enqueue_lock_requestor( Lock_requestor* lock_requestor )
{
    _waiting_queues[ lock_requestor->lock_mode() ].push_back( lock_requestor );

    int result = _waiting_queues[ lk_exclusive ].size();
    if( lock_requestor->lock_mode() == lk_non_exclusive )  result += _waiting_queues[ lk_non_exclusive ].size();

    return result;
}

//-----------------------------------------------------------Scheduler_lock::dequeue_lock_requestor

void Scheduler_lock::dequeue_lock_requestor( Lock_requestor* lock_requestor )
{
    Requestor_list& list = _waiting_queues[ lock_requestor->lock_mode() ];

    Z_FOR_EACH( Requestor_list, list, it )
    {
        if( *it == lock_requestor )  
        {
            list.erase( it );
            return;
        }
    }
}

//---------------------------------------------------------------------Scheduler_lock::is_exclusive

bool Scheduler_lock::is_exclusive() const
{ 
    return _holder_set.size() == 1  &&  (*_holder_set.begin())->lock_mode() == lk_exclusive; 
}

//-------------------------------------------------------------------------Scheduler_lock::obj_name

string Scheduler_lock::obj_name() const
{
    S result;

    result << "Lock " << _name;

    return result;
}

//-------------------------------------------------------------------Lock_requestor::Lock_requestor

Lock_requestor::Lock_requestor( Scheduler_object* o ) 
: 
    Scheduler_object( o->_spooler, this, type_lock_requestor ),
    _zero_(this+1), 
    _object(o),
    _lock_mode( Scheduler_lock::lk_exclusive )
{
    _log = o->_log;
}

//------------------------------------------------------------------Lock_requestor::~Lock_requestor
    
Lock_requestor::~Lock_requestor()
{ 
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << "  ERROR " << x.what() << "\n" ); }
}

//----------------------------------------------------------------------------Lock_requestor::close

void Lock_requestor::close()
{ 
    if( Scheduler_lock* lock = _spooler->lock_subsystem()->lock_or_null( _lock_name ) )
    {
        lock->dequeue_lock_requestor( this );
    }
}

//--------------------------------------------------------------------------Lock_requestor::set_dom

void Lock_requestor::set_dom( const xml::Element_ptr& lock_element )
{
    assert( lock_element.nodeName_is( "lock.lock" ) );

    _lock_name = lock_element.getAttribute( "lock" );
    
    _lock_mode = lock_element.bool_getAttribute( "exclusive", _lock_mode == Scheduler_lock::lk_exclusive )? 
                    Scheduler_lock::lk_exclusive 
                  : Scheduler_lock::lk_non_exclusive;
}

//-----------------------------------------------------------------------------Lock_requestor::load

void Lock_requestor::load()
{
    _spooler->lock_subsystem()->lock( _lock_name );
}

//-------------------------------------------------------------Lock_requestor::enqueue_lock_request

void Lock_requestor::enqueue_lock_request()
{ 
    if( _is_enqueued )  z::throw_xc( __FUNCTION__ );

    int place = lock()->enqueue_lock_requestor( this ); 
    _is_enqueued = true; 

    _log->info( message_string( "SCHEDULER-857", lock()->obj_name(), place ) );
}

//-------------------------------------------------------------Lock_requestor::dequeue_lock_request

void Lock_requestor::dequeue_lock_request()
{ 
    if( _is_enqueued )
    {
        lock()->dequeue_lock_requestor( this ); 
        _is_enqueued = false; 
    }
}

//-----------------------------------------------------------------------------Lock_requestor::lock

Scheduler_lock* Lock_requestor::lock() const
{ 
    return _spooler->lock_subsystem()->lock( _lock_name ); 
}

//----------------------------------------------------------------------Lock_requestor::dom_element

xml::Element_ptr Lock_requestor::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock.requestor" );

    result.setAttribute( "name", _lock_name );

    if( _is_enqueued )  result.setAttribute( "enqueued", "yes" );

    return result;
}

//-------------------------------------------------------------------------Lock_requestor::obj_name

string Lock_requestor::obj_name() const
{
    S result;
    result << "Lock_requestor";
    
    if( _lock_mode == Scheduler_lock::lk_exclusive )  result << " exclusive";
                                                else  result << " non-exclusive";

    if( Scheduler_lock* lock = _spooler->lock_subsystem()->lock_or_null( _lock_name ) )
    {
        result << " " << lock->obj_name();
    }

    return result;
}

//-------------------------------------------------------------------------Lock_holder::Lock_holder

Lock_holder::Lock_holder( Scheduler_object* o, const Lock_requestor* lock_requestor ) 
: 
    Scheduler_object( o->_spooler, this, type_lock_holder ),
    _zero_(this+1), 
    _lock_requestor(lock_requestor),
    _object(o)
{
    _log = o->_log;
}

//------------------------------------------------------------------------Lock_holder::~Lock_holder
    
Lock_holder::~Lock_holder()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << " ERROR " << x.what() << "\n" ); }
}

//-------------------------------------------------------------------------------Lock_holder::close

void Lock_holder::close()
{
    if( _is_holding )
    {
        release_lock();
    }
}

//------------------------------------------------------------------------Lock_holder::request_lock

bool Lock_holder::request_lock()
{
    bool result = lock()->request_lock_for( this );

    if( result )
    {
        _is_holding = true;
        _log->info( message_string( "SCHEDULER-855", lock()->obj_name(), _lock_requestor->lock_mode() == Scheduler_lock::lk_exclusive? "exclusively" : "non-exclusively" ) );
    }

    return result;
}

//------------------------------------------------------------------------Lock_holder::release_lock

void Lock_holder::release_lock()
{
    if( _is_holding )
    {
        lock()->release_lock_for( this );
        _log->info( message_string( "SCHEDULER-856", lock()->obj_name() ) );
        _is_holding = false;
    }
}

//-------------------------------------------------------------------------Lock_holder::dom_element

xml::Element_ptr Lock_holder::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock_holder" );

    if( _lock_requestor )
    {
        result.setAttribute( "name", _lock_requestor->lock_name() );
        result.setAttribute( "exclusive", _lock_requestor->lock_mode() == Scheduler_lock::lk_exclusive? "yes" : "no" );
    }

    return result;
}

//----------------------------------------------------------------------------Lock_holder::obj_name

string Lock_holder::obj_name() const
{
    S result;

    result << "Lock_holder(";
    if( _object )  result << _object->obj_name();
    result << ",";
    if( _lock_requestor )  result << _lock_requestor->obj_name();
    //if( Scheduler_lock* lock = lock_or_null() )  result << lock->obj_name();
    result << ")";

    return result;
}

//-------------------------------------------------------------------Lock_subsystem::Lock_subsystem

Lock_subsystem::Lock_subsystem( Scheduler* scheduler )
:
    Subsystem( scheduler, type_lock_subsystem )
{
    close();
}

//----------------------------------------------------------------------------Lock_subsystem::close
    
void Lock_subsystem::close()
{
    for( Lock_map::iterator it = _lock_map.begin(); it != _lock_map.end(); )
    {
        Lock_map::iterator erase_it = it;
        it++;

        erase_it->second->close();
        _lock_map.erase( erase_it );
    }
}

//-------------------------------------------------------------Lock_subsystem::subsystem_initialize

bool Lock_subsystem::subsystem_initialize()
{
    set_subsystem_state( subsys_initialized );
    return true;
}

//-------------------------------------------------------------------Lock_subsystem::subsystem_load

bool Lock_subsystem::subsystem_load()
{
    set_subsystem_state( subsys_loaded );
    return true;
}

//---------------------------------------------------------------Lock_subsystem::subsystem_activate

bool Lock_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    return true;
}

//--------------------------------------------------------------------------Lock_subsystem::set_dom

void Lock_subsystem::set_dom( const xml::Element_ptr& locks_element )
{
    assert( locks_element.nodeName_is( "locks" ) );

    DOM_FOR_EACH_ELEMENT( locks_element, lock_element )
    {
        ptr<Scheduler_lock> lock = this->lock_or_null( lock_element.getAttribute( "name" ) );
        if( !lock )  lock = Z_NEW( Scheduler_lock( this ) );
        lock->set_dom( lock_element );
        _lock_map[ lock->name() ] = lock;
    }
}

//----------------------------------------------------------------------Lock_subsystem::dom_element

xml::Element_ptr Lock_subsystem::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "locks" );

    Z_FOR_EACH( Lock_map, _lock_map, it )
    {
        Scheduler_lock* lock = it->second;
        result.appendChild( lock->dom_element( dom_document, show_what ) );
    }

    return result;
}

//-----------------------------------------------------------------------------Lock_subsystem::lock

Scheduler_lock* Lock_subsystem::lock( const string& name )
{
    Scheduler_lock* result = lock_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-401", name );
    return result;
}

//---------------------------------------------------------------------Lock_subsystem::lock_or_null

Scheduler_lock* Lock_subsystem::lock_or_null( const string& name )
{
    Lock_map::iterator it = _lock_map.find( name );
    return it == _lock_map.end()? NULL : it->second;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
