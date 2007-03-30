// $Id: spooler_job.cxx 4910 2007-03-23 23:15:25Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"


namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------Lock_holder::Lock_holder

Lock_holder::Lock_holder( Scheduler_object* o ) 
: 
    Scheduler_object( o->_spooler, this, type_lock_holder ),
    _zero_(this+1), 
    _object(o), 
    _lock_mode(Scheduler_lock::lk_exclusive)
{
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
    if( _state == s_is_holding_lock )
    {
        release_lock();
    }

    if( _state == s_is_waiting_for_lock )
    {
        remove_from_waiting_queue();
    }

    _lock = NULL;
}

//----------------------------------------------------------------Lock_holder::clear_lock_reference

void Lock_holder::clear_lock_reference()
{
    _lock = NULL;
    set_state( s_none );
}

//-----------------------------------------------------------------------------Lock_holder::set_dom

void Lock_holder::set_dom( const xml::Element_ptr& lock_element )
{
    assert( lock_element.nodeName_is( "lock.lock" ) );

    _lock_name = lock_element.getAttribute( "lock" );
    
    _lock_mode = lock_element.bool_getAttribute( "exclusive", _lock_mode == Scheduler_lock::lk_exclusive )? 
                    Scheduler_lock::lk_exclusive 
                  : Scheduler_lock::lk_non_exclusive;
}

//--------------------------------------------------------------------------------Lock_holder::load

void Lock_holder::load()
{
    _spooler->lock_subsystem()->lock( _lock_name );
}

//---------------------------------------------------------------------------Lock_holder::set_state

void Lock_holder::set_state( State new_state )
{
    _state = new_state;
    _state_time = Time::now();
}

//------------------------------------------------------------------------Lock_holder::request_lock

bool Lock_holder::request_lock()
{
    return _lock->request_lock_for( this );
}

//------------------------------------------------------------------------Lock_holder::release_lock

void Lock_holder::release_lock()
{
    if( _state == s_is_holding_lock )
    {
        _lock->release_lock_for( this );
        set_state( s_none );
    }
}

//-----------------------------------------------------------Lock_holder::remove_from_waiting_queue

void Lock_holder::remove_from_waiting_queue()
{
    if( _state == s_is_waiting_for_lock )
    {
        _lock->remove_waiting_holder( this );
        set_state( s_none );
    }
}

//----------------------------------------------------------------------------Lock_holder::obj_name

string Lock_holder::obj_name() const
{
    S result;

    result << "Lock_holder(";
    if( _object )  result << _object->obj_name();
    result << ",";
    if( _lock )  result << _lock->obj_name();
    result << ")";

    return result;
}

//-------------------------------------------------------------------Scheduler_lock::Scheduler_lock

Scheduler_lock::Scheduler_lock( Lock_subsystem* lock_subsystem )
:
    Scheduler_object( lock_subsystem->_spooler, this, type_lock ),
    _zero_(this+1),
    _lock_subsystem(lock_subsystem)
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
    for( Holder_list::iterator it = _holder_list.begin(); it != _holder_list.end(); )
    {
        Lock_holder* holder = *it;
        log()->error( message_string( "SCHEDULER-854", obj_name(), holder->obj_name() ) );
        holder->clear_lock_reference();
        it = _holder_list.erase( it );
    }

    clear_waiting_queue();
}

//--------------------------------------------------------------Scheduler_lock::clear_waiting_queue

void Scheduler_lock::clear_waiting_queue()
{
    for( int lock_mode = lk_non_exclusive; lock_mode <= lk_exclusive; lock_mode++ )
    {
        Holder_list* list = &_waiting_queue[ lock_mode ];

        for( Holder_list::iterator it = list->begin(); it != list->end(); )
        {
            Lock_holder* holder = *it;
            holder->_lock = NULL;
            it = list->erase( it );
        }
    }

    _waiting_queue.clear();
}

//--------------------------------------------------------------------------Scheduler_lock::set_dom

void Scheduler_lock::set_dom( const xml::Element_ptr& lock_element )
{
    assert( lock_element.nodeName_is( "lock" ) );

    _name = lock_element.getAttribute( "name" );
}

//----------------------------------------------------------------------Scheduler_lock::dom_element

xml::Element_ptr Scheduler_lock::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock" );

    result.setAttribute( "name", _name );

    return result;
}

//-----------------------------------------------------------------Scheduler_lock::request_lock_for

bool Scheduler_lock::request_lock_for( Lock_holder* holder )
{
    if( holder->_state == Lock_holder::s_is_holding_lock )  z::throw_xc( __FUNCTION__ );

    if( holder->lock_mode() == lk_exclusive? _holder_list.empty() : 
                                             !is_exclusive()  &&  _waiting_queue[ lk_exclusive ].empty() )
    {
        if( holder->_state == Lock_holder::s_is_waiting_for_lock )
        {
            remove_waiting_holder( holder );
        }

        _holder_list.push_back( holder );

        holder->set_state( Lock_holder::s_is_holding_lock );
        _log->info( message_string( "SCHEDULER-855", holder->obj_name() ) );
    }

    if( holder->_state != Lock_holder::s_is_holding_lock ) 
    {
        _waiting_queue[ holder->lock_mode() ].push_back( holder );
    }

    return holder->_state == Lock_holder::s_is_holding_lock;
}

//-----------------------------------------------------------------Scheduler_lock::release_lock_for

void Scheduler_lock::release_lock_for( Lock_holder* holder )
{
    Z_FOR_EACH( Holder_list, _holder_list, it )
    {
        if( *it == holder )
        {
            _holder_list.erase( it );
            holder->set_state( Lock_holder::s_none );
            _log->info( message_string( "SCHEDULER-856", holder->obj_name() ) );
            break;
        }
    }

    if( holder->_state == Lock_holder::s_is_holding_lock )  z::throw_xc( __FUNCTION__ );


    // Was tun wir, wenn Lock_holder die Sperre nicht übernehmen kann?

    if( !_waiting_queue[ lk_exclusive ].empty() )  
    {
        ( *_waiting_queue[ lk_exclusive ].begin() ) -> on_lock_is_available( this );
    }
    else
    if( !_waiting_queue[ lk_non_exclusive ].empty() )  
    {
        ( *_waiting_queue[ lk_non_exclusive ].begin() ) -> on_lock_is_available( this );
    }
}

//------------------------------------------------------------Scheduler_lock::remove_waiting_holder

void Scheduler_lock::remove_waiting_holder( Lock_holder* holder )
{
    for( int lock_mode = lk_non_exclusive; lock_mode <= lk_exclusive; lock_mode++ )
    {
        Holder_list* list = &_waiting_queue[ lock_mode ];
        Z_FOR_EACH( Holder_list, *list, it )
        {
            if( *it == holder )  
            {
                list->erase( it );
                return;
            }
        }
    }
}

//---------------------------------------------------------------------Scheduler_lock::is_exclusive

bool Scheduler_lock::is_exclusive() const
{ 
    return _holder_list.size() == 1  &&  (*_holder_list.begin())->lock_mode() == lk_exclusive; 
}

//-------------------------------------------------------------------------Scheduler_lock::obj_name

string Scheduler_lock::obj_name() const
{
    S result;

    result << "Lock " << _name;

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
        ptr<Scheduler_lock> lock = this->lock( lock_element.getAttribute( "name" ) );
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
