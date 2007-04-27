// $Id: spooler_job.cxx 4910 2007-03-23 23:15:25Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"


namespace sos {
namespace scheduler {
namespace lock {

//-------------------------------------------------------------------------------------------------

Lock_subsystem::Class_descriptor    Lock_subsystem::class_descriptor ( &typelib, "Spooler.Locks", Lock_subsystem::_methods );
Lock          ::Class_descriptor    Lock          ::class_descriptor ( &typelib, "Spooler.Lock" , Lock          ::_methods );

//-------------------------------------------------------------------------Lock_subsystem::_methods

const Com_method Lock_subsystem::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Lock_subsystem,  1, Java_class_name     , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Lock_subsystem,  2, Lock                , VT_DISPATCH, 0, VT_BSTR ),
    COM_PROPERTY_GET( Lock_subsystem,  3, Lock_or_null        , VT_DISPATCH, 0, VT_BSTR ),
    COM_METHOD      ( Lock_subsystem,  4, Create_lock         , VT_DISPATCH, 0 ),
    COM_METHOD      ( Lock_subsystem,  5, Add_lock            , VT_EMPTY   , 0, VT_DISPATCH ),
#endif
    {}
};

//-----------------------------------------------------------------------------------Lock::_methods

const Com_method Lock::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Lock,  1, Java_class_name     , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Lock,  2, Name                ,              0, VT_BSTR ),
    COM_PROPERTY_GET( Lock,  2, Name                , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Lock,  3, Max_non_exclusive   ,              0, VT_INT ),
    COM_PROPERTY_GET( Lock,  3, Max_non_exclusive   , VT_INT     , 0 ),
    COM_METHOD      ( Lock,  4, Remove              , VT_EMPTY   , 0 ),
#endif
    {}
};

//---------------------------------------------------------------------------------------Lock::Lock

Lock::Lock( Lock_subsystem* lock_subsystem, const string& name )
:
    Idispatch_implementation( &class_descriptor ),
    Scheduler_object( lock_subsystem->_spooler, static_cast<spooler_com::Ilock*>( this ), type_lock ),
    _zero_(this+1),
    _lock_subsystem(lock_subsystem),
    _max_non_exclusive(INT_MAX),
    _waiting_queues(2)                  // [lk_exclusive] und [lk_non_ecklusive]
{
    if( name != "" )  set_name( name );
}

//--------------------------------------------------------------------------------------Lock::~Lock

Lock::~Lock()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << ": " << x.what() ); }
}

//--------------------------------------------------------------------------------------Lock::close

void Lock::close()
{
    for( Holder_set::iterator it = _holder_set.begin(); it != _holder_set.end(); )
    {
        Holder* holder = *it;
        log()->error( message_string( "SCHEDULER-854", obj_name(), holder->obj_name() ) );
    }

    _waiting_queues.clear();
}

//-----------------------------------------------------------------------------Lock::prepare_remove

void Lock::prepare_remove()
{
    if( !is_free()        )  z::throw_xc( "SCHEDULER-410", obj_name(), string_from_holders() );
    if( !_use_set.empty() )  z::throw_xc( "SCHEDULER-411", obj_name(), string_from_uses() );

    typedef stdext::hash_set< Requestor* >  Requestor_set;
    Requestor_set requestor_set;

    for( int lk = lk_exclusive; lk <= lk_non_exclusive; lk++ )
    {
        Z_FOR_EACH( Use_list, _waiting_queues[ lk ], it )
        {
            Use* lock_use = *it;
            requestor_set.insert( lock_use->requestor() );
        }
    }

    Z_FOR_EACH( Requestor_set, requestor_set, it )
    {
        Requestor* requestor = *it;
        requestor->dequeue_lock_requests();
    }

    close();

    log()->info( message_string( "SCHEDULER-861" ) );
}

//-----------------------------------------------------------------------------------Lock::set_name

void Lock::set_name( const string& name )
{
    _spooler->check_name( name );

    if( name != _name )
    {
        if( _name != "" )  z::throw_xc( "SCHEDULER-243", "Lock.name" );
      //if( is_added() )  z::throw_xc( "SCHEDULER-243", "Lock.name" );

        _name = name;

        _log->set_prefix( obj_name() );
    }
}

//----------------------------------------------------------------------Lock::set_max_non_exclusive

void Lock::set_max_non_exclusive( int max_non_exclusive )
{
    if( max_non_exclusive < count_non_exclusive_holders() )  z::throw_xc( "SCHEDULER-402", max_non_exclusive, string_from_holders() );

    _max_non_exclusive = max_non_exclusive;
}

//-----------------------------------------------------------------------------------Lock::is_added

bool Lock::is_added() const
{
    bool result = false;

    if( Lock* lock = _spooler->lock_subsystem()->lock_or_null( path() ) )
    {
        if( lock == this )  result = true;
    }

    return result;
}

//--------------------------------------------------------------------------------Lock::is_free_for

bool Lock::is_free_for( Lock_mode lock_mode ) const
{ 
    return _holder_set.empty()  ||   //deadlock? &&  ( _waiting_queues[ lk_exclusive ].empty() || lock_mode == lk_exclusive )  ||  
           lock_mode == lk_non_exclusive  &&  _lock_mode == lk_non_exclusive  &&  count_non_exclusive_holders() < _max_non_exclusive;
}

//--------------------------------------------------------------------------------Lock::its_my_turn

bool Lock::its_my_turn( const Use* lock_use )
{
    bool result = false;

    if( is_free_for( lock_use->lock_mode() ) )
    {
        Use_list&          queue = _waiting_queues[ lock_use->lock_mode() ];
        Use_list::iterator first = queue.begin();

        if( first == queue.end()  ||    // Warteschlange ist leer?
           *first == lock_use )         // Wir sind dran?
        {
            result = true;
        }
    }

    return result;
}

//---------------------------------------------------------------------------Lock::require_lock_for

void Lock::require_lock_for( Holder* holder, Use* lock_use )
{
    assert( is_free_for( lock_use->lock_mode() ) );
    assert( holder->requestor() == lock_use->requestor() );

    _holder_set.insert( holder );
    _lock_mode = lock_use->lock_mode();
}

//---------------------------------------------------------------------------Lock::release_lock_for

void Lock::release_lock_for( Holder* holder )
{
    Holder_set::iterator it = _holder_set.find( holder );

    if( it != _holder_set.end() )  _holder_set.erase( it );
                             else  Z_DEBUG_ONLY( assert( !"Unbekannter Holder" ) );


    Requestor* next_requestor = NULL; 

    if( !_waiting_queues[ lk_exclusive ].empty() )  
    {
        next_requestor = ( *_waiting_queues[ lk_exclusive ].begin() ) -> requestor();
    }
    else
    if( !_waiting_queues[ lk_non_exclusive ].empty() )  
    {
        next_requestor = ( *_waiting_queues[ lk_non_exclusive ].begin() ) -> requestor();
    }

    if( next_requestor  &&  next_requestor->locks_are_available() )     // Der Requestor kann auch die anderen Sperren belegen?
    {
        next_requestor->on_locks_are_available();
    }
}

//---------------------------------------------------------------------------Lock::enqueue_lock_use

int Lock::enqueue_lock_use( Use* lock_use )
{
#   ifdef Z_DEBUG
        for( int lk = lk_exclusive; lk <= lk_non_exclusive; lk++ )
        {
            Z_FOR_EACH( Use_list, _waiting_queues[lk], it )
            {
                assert( *it != lock_use );
            }
        }
#   endif


    _waiting_queues[ lock_use->lock_mode() ].push_back( lock_use );

    int result = _waiting_queues[ lk_exclusive ].size();
    if( lock_use->lock_mode() == lk_non_exclusive )  result += _waiting_queues[ lk_non_exclusive ].size();

    return result;
}

//---------------------------------------------------------------------------Lock::dequeue_lock_use

void Lock::dequeue_lock_use( Use* lock_use )
{
    Use_list& list = _waiting_queues[ lock_use->lock_mode() ];

    Z_FOR_EACH( Use_list, list, it )
    {
        if( *it == lock_use )  
        {
            list.erase( it );
            break;
        }
    }
}

//------------------------------------------------------------------------------------Lock::set_dom

void Lock::set_dom( const xml::Element_ptr& lock_element )
{
    Z_DEBUG_ONLY( assert( lock_element.nodeName_is( "lock" ) ) );

    set_max_non_exclusive( lock_element.int_getAttribute( "max_non_exclusive", _max_non_exclusive ) );
}

//--------------------------------------------------------------------------------Lock::dom_element

xml::Element_ptr Lock::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock" );

    result.setAttribute( "name", _name );
    if( _max_non_exclusive < INT_MAX )  result.setAttribute( "max_non_exclusive", _max_non_exclusive );

    xml::Element_ptr holders_element = result.append_new_element( "lock.holders" );
    Z_FOR_EACH( Holder_set, _holder_set, it )
    {
        Holder* holder = *it;

        xml::Element_ptr holder_element = holders_element.append_new_element( "lock.holder" );
        holder->object()->write_element_attributes( holder_element );
    }

    for( int lk = lk_exclusive; lk <= lk_non_exclusive; lk++ )
    {
        xml::Element_ptr queue_element = result.append_new_element( "lock.queue" );
        queue_element.setAttribute( "exclusive", lk == lk_exclusive? "yes" : "no" );

        Use_list& queue = _waiting_queues[ lk ];
        Z_FOR_EACH( Use_list, queue, it )
        {
            Use* lock_use = *it;
            xml::Element_ptr entry_element = queue_element.append_new_element( "lock.queue.entry" );

            Scheduler_object* object = lock_use->requestor()->object();
            object->write_element_attributes( entry_element );
        }
    }

    return result;
}

//------------------------------------------------------------------------Lock::string_from_holders

string Lock::string_from_holders() const
{
    S result;

    if( _holder_set.empty() )  // ||  lock_mode == lk_non_exclusive  &&  _lock_mode == lk_non_exclusive )
    {
        result << "free";
    }
    else
    {
        result << ( _lock_mode == lk_exclusive? "exclusive holder " : "non-exclusive holder " );

        Z_FOR_EACH_CONST( Holder_set, _holder_set, it )
        {
            Holder* holder = *it;

            if( it != _holder_set.begin() )  result << ", "; 
            result << holder->object()->obj_name();
        }
    }

    return result;
}

//---------------------------------------------------------------------------Lock::string_from_uses

string Lock::string_from_uses() const
{
    S result;

    Z_FOR_EACH_CONST( Use_set, _use_set, it )
    {
        Use* lock_use = *it;

        if( !result.empty() )  result << ", ";
        result << lock_use->requestor()->object()->obj_name();
    }

    return result;
}

//-----------------------------------------------------------------------------------Lock::obj_name

string Lock::obj_name() const
{
    S result;

    result << "Lock " << path();

    return result;
}

//-----------------------------------------------------------------------------------Lock::put_Name

STDMETHODIMP Lock::put_Name( BSTR name_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_name( string_from_bstr( name_bstr ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//----------------------------------------------------------------------Lock::put_Max_non_exclusive

STDMETHODIMP Lock::put_Max_non_exclusive( int max_non_exclusive )
{
    HRESULT hr = S_OK;

    try
    {
        set_max_non_exclusive( max_non_exclusive );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------------------------Lock::remove

void Lock::remove()
{
    _spooler->lock_subsystem()->remove_lock( this );
}

//-------------------------------------------------------------------------------------Lock::Remove

STDMETHODIMP Lock::Remove()
{
    HRESULT hr = S_OK;

    try
    {
        remove();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------------------Lock::execute_xml

void Lock::execute_xml( const xml::Element_ptr& element, const Show_what& )
{
    if( element.nodeName_is( "lock.remove" ) ) 
    {
        remove();
    }
    else
        z::throw_xc( "SCHEDULER-409", "lock.remove", element.nodeName() );
}

//-----------------------------------------------------------------------------Requestor::Requestor

Requestor::Requestor( Scheduler_object* o ) 
: 
    Scheduler_object( o->_spooler, this, type_lock_requestor ),
    _zero_(this+1),
    _object(o)
{
    _log = o->_log;
}

//----------------------------------------------------------------------------Requestor::~Requestor
    
Requestor::~Requestor()
{ 
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << "  ERROR " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------------------Requestor::close

void Requestor::close()
{ 
    Z_FOR_EACH( Use_list, _use_list, it )
    {
        Use* lock_use = *it;
        
        if( Lock* lock = lock_use->lock_or_null() )
        {
            lock->dequeue_lock_use( lock_use );
        }
    }

    _use_list.clear();
}

//-------------------------------------------------------------------------------Requestor::set_dom

void Requestor::set_dom( const xml::Element_ptr& lock_use_element )
{
    string path = lock_use_element.getAttribute( "lock" );

    ptr<Use> lock_use;

    Z_FOR_EACH( Use_list, _use_list, it )
    {
        if( (*it)->lock_path() == path )
        {
            lock_use = *it;
            break;
        }
    }

    if( !lock_use )  
    {
        lock_use = Z_NEW( Use( this ) );
        _use_list.push_back( lock_use );
    }

    lock_use->set_dom( lock_use_element );
}

//----------------------------------------------------------------------------------Requestor::load

void Requestor::load()
{
    Z_FOR_EACH( Use_list, _use_list, it )
    {
        Use* lock_use = *it;
        lock_use->load();
    }
}

//-------------------------------------------------------------------Requestor::locks_are_available

bool Requestor::locks_are_available() const
{
    bool all_locks_are_free = true;
    bool its_my_turn        = false;

    Z_FOR_EACH_CONST( Use_list, _use_list, it )
    {
        Use*  lock_use = *it;
        Lock* lock     = lock_use->lock();

        if( !lock->is_free_for( lock_use->lock_mode() ) )
        {
            all_locks_are_free = false;
            break;
        }

        if( !its_my_turn )  its_my_turn = lock->its_my_turn( lock_use );
    }

    return all_locks_are_free  &&  its_my_turn;   
}

//-----------------------------------------------------------------Requestor::enqueue_lock_requests

void Requestor::enqueue_lock_requests()
{ 
    if( _is_enqueued )  z::throw_xc( __FUNCTION__ );
    _is_enqueued = true; 

    //S lock_names;

    Z_FOR_EACH( Use_list, _use_list, it )
    {
        Use*  lock_use = *it;
        Lock* lock     = lock_use->lock();
        int   place    = lock->enqueue_lock_use( lock_use );    // Bei _use_list.size() > 1 kann die Sperre frei sein. Wir tragen uns trotzdem in die Warteschlange ein
        
        //if( !lock_names.empty() )  lock_names << ", ";
        //lock_names << lock_use->lock_path() << " (";
        //lock_names << ( lock_use->lock_mode() == Lock::lk_exclusive? "exclusive" : "non-exclusive" );
        //lock_names << ", at place " << place << ")";
        _log->info( message_string( "SCHEDULER-860", lock->obj_name(), lock_use->lock_mode() == Lock::lk_exclusive? "exclusive" : "non-exclusive", 
                                                     place, lock->string_from_holders() ) );
    }
    
    //_log->info( message_string( "SCHEDULER-860", lock_names ) );
}

//-----------------------------------------------------------------Requestor::dequeue_lock_requests

void Requestor::dequeue_lock_requests( Log_level log_level )
{ 
    if( _is_enqueued )
    {
        S lock_names;

        Z_FOR_EACH( Use_list, _use_list, it )
        {
            Use* lock_use = *it;
            
            if( !lock_names.empty() )  lock_names << ", ";
            lock_names << lock_use->lock_path();

            if( Lock* lock = lock_use->lock_or_null() )
            {
                lock->dequeue_lock_use( lock_use ); 
            }
            else
                lock_names << " (missing)";
        }

        _log->log( log_level, message_string( "SCHEDULER-858", lock_names ) );

        _is_enqueued = false; 
    }
}

//---------------------------------------------------------------------------Requestor::dom_element

xml::Element_ptr Requestor::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "lock.requestor" );

    if( _is_enqueued )  result.setAttribute( "waiting", "yes" );

    Z_FOR_EACH( Use_list, _use_list, it )
    {
        Use* lock_use = *it;
        result.appendChild( lock_use->dom_element( dom_document, show_what ) );
    }

    return result;
}

//------------------------------------------------------------------------------Requestor::obj_name

string Requestor::obj_name() const
{
    S result;
    result << "Lock.Requestor(";
    
    Z_FOR_EACH_CONST( Use_list, _use_list, it )
    {
        if( it != _use_list.begin() )  result << ", ";
        Use* lock_use = *it;
        result << lock_use->obj_name();
    }

    result << ")";

    return result;
}

//-----------------------------------------------------------------------------------------Use::Use

Use::Use( Requestor* requestor ) 
: 
    Scheduler_object( requestor->_spooler, this, type_lock_requestor ),
    _zero_(this+1), 
    _requestor(requestor),
    _lock_mode( Lock::lk_exclusive )
{
    _log = requestor->_log;
}

//----------------------------------------------------------------------------------------Use::~Use
    
Use::~Use()
{ 
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << "  ERROR " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------------------------Use::close

void Use::close()
{ 
    if( Lock* lock = lock_or_null() )
    {
        lock->dequeue_lock_use( this );
        lock->unregister_lock_use( this );
    }
}

//-------------------------------------------------------------------------------------Use::set_dom

void Use::set_dom( const xml::Element_ptr& lock_use_element )
{
    Z_DEBUG_ONLY( assert( lock_use_element.nodeName_is( "lock.use" ) ) );

    string lock_path = lock_use_element.getAttribute( "lock" );
    if( lock_path == "" )  z::throw_xc( "SCHEDULER-407", "lock" );

    Lock::Lock_mode lock_mode = lock_use_element.bool_getAttribute( "exclusive", _lock_mode == Lock::lk_exclusive )? 
                                    Lock::lk_exclusive 
                                  : Lock::lk_non_exclusive;

    if( _lock_path == "" )  // Neu?
    {
        _lock_path = lock_path;
        _lock_mode = lock_mode;
    }
    else
    {
        if( _lock_path != lock_path )  z::throw_xc( __FUNCTION__ );
        if( _lock_mode != lock_mode )  z::throw_xc( "SCHEDULER-408", "lock.use", "exclusive" );
    }
}

//----------------------------------------------------------------------------------------Use::load

void Use::load()
{
    Lock* lock = this->lock();     // Liefert die Sperre, stellt also sicher, dass sie bekannt ist.
    lock->register_lock_use( this );
}

//----------------------------------------------------------------------------------------Use::lock

Lock* Use::lock() const
{ 
    return _spooler->lock_subsystem()->lock( _lock_path ); 
}

//--------------------------------------------------------------------------------Use::lock_or_null

Lock* Use::lock_or_null() const
{ 
    return _spooler->lock_subsystem()->lock_or_null( _lock_path ); 
}

//---------------------------------------------------------------------------------Use::dom_element

xml::Element_ptr Use::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock.use" );

    result.setAttribute( "lock"     , _lock_path );
    result.setAttribute( "exclusive", _lock_mode == Lock::lk_exclusive? "yes" : "no" );

    return result;
}

//------------------------------------------------------------------------------------Use::obj_name

string Use::obj_name() const
{
    S result;
    result << "Lock.Use";
    
    if( _lock_mode == Lock::lk_exclusive )  result << " exclusive";
                                      else  result << " non-exclusive";

    if( Lock* lock = lock_or_null() )
    {
        result << " " << lock->obj_name();
    }

    return result;
}

//-----------------------------------------------------------------------------------Holder::Holder

Holder::Holder( Scheduler_object* o, const Requestor* requestor ) 
: 
    Scheduler_object( o->_spooler, this, type_lock_holder ),
    _zero_(this+1), 
    _requestor(requestor),
    _object(o)
{
    _log = o->_log;
}

//----------------------------------------------------------------------------------Holder::~Holder
    
Holder::~Holder()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << " ERROR " << x.what() << "\n" ); }
}

//------------------------------------------------------------------------------------Holder::close

void Holder::close()
{
    if( _is_holding )
    {
        release_locks();
    }
}

//-------------------------------------------------------------------------------Holder::hold_locks

void Holder::hold_locks()
{
    assert( !_is_holding );
    assert( _requestor->locks_are_available() );


    Z_FOR_EACH_CONST( Requestor::Use_list, _requestor->_use_list, it )
    {
        Use*  lock_use = *it;
        Lock* lock     = lock_use->lock();

        lock->require_lock_for( this, lock_use );

        log()->info( message_string( "SCHEDULER-855", lock->obj_name(), lock_use->lock_mode() == Lock::lk_exclusive? "exclusively" : "non-exclusively" ) );
    }

    _is_holding = true;
}

//----------------------------------------------------------------------------Holder::release_locks

void Holder::release_locks()
{
    if( _is_holding )
    {
        Z_FOR_EACH_CONST( Requestor::Use_list, _requestor->_use_list, it ) 
        {
            Use*  lock_use = *it;
            
            try
            {
                Lock* lock = lock_use->lock();

                lock->release_lock_for( this );

                if( int remaining = lock->count_non_exclusive_holders() )
                {
                    log()->info( message_string( "SCHEDULER-857", lock->obj_name(), remaining ) );
                }
                else
                {
                    log()->info( message_string( "SCHEDULER-856", lock->obj_name() ) );
                }
            }
            catch( exception& x )
            {
                log()->error( S() << x.what() << ", in " << __FUNCTION__ );
            }
        }

        _is_holding = false;
    }
}

//-------------------------------------------------------------------------Holder::dom_element

//xml::Element_ptr Holder::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
//{
//    xml::Element_ptr result = dom_document.createElement( "holder" );
//
//    if( _requestor )
//    {
//        result.setAttribute( "lock", _requestor->lock_name() );
//        result.setAttribute( "exclusive", _requestor->lock_mode() == Lock::lk_exclusive? "yes" : "no" );
//    }
//
//    return result;
//}

//---------------------------------------------------------------------------------Holder::obj_name

string Holder::obj_name() const
{
    S result;

    result << "Holder(";
    if( _requestor )  result << _requestor->obj_name();
    result << ")";

    return result;
}

//-------------------------------------------------------------------Lock_subsystem::Lock_subsystem

Lock_subsystem::Lock_subsystem( Scheduler* scheduler )
:
    Idispatch_implementation( &class_descriptor ),
    Subsystem( scheduler, static_cast<spooler_com::Ilocks*>( this ), type_lock_subsystem )
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

//-----------------------------------------------------------------------------Lock_subsystem::lock

Lock* Lock_subsystem::lock( const string& name )
{
    Lock* result = lock_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-401", name );
    return result;
}

//---------------------------------------------------------------------Lock_subsystem::lock_or_null

Lock* Lock_subsystem::lock_or_null( const string& name )
{
    Lock_map::iterator it = _lock_map.find( name );
    return it == _lock_map.end()? NULL : it->second;
}

//----------------------------------------------------------------------Lock_subsystem::remove_lock

void Lock_subsystem::remove_lock( Lock* lock )
{
    string             path = lock->path();
    Lock_map::iterator it   = _lock_map.find( path );

    if( it->second != lock )  z::throw_xc( "SCHEDULER-401", path, "Duplicate lock already removed" );

    lock->prepare_remove();
    _lock_map.erase( it );
}

//--------------------------------------------------------------------------Lock_subsystem::set_dom

void Lock_subsystem::set_dom( const xml::Element_ptr& locks_element )
{
    assert( locks_element.nodeName_is( "locks" ) );

    DOM_FOR_EACH_ELEMENT( locks_element, lock_element )
    {
        execute_xml_lock( lock_element );
    }
}

//-----------------------------------------------------------------Lock_subsystem::execute_xml_lock

void Lock_subsystem::execute_xml_lock( const xml::Element_ptr& lock_element )
{
    if( !lock_element.nodeName_is( "lock" ) )  z::throw_xc( "SCHEDULER-409", "lock", lock_element.nodeName() );

    string lock_name = lock_element.getAttribute( "name" );
    
    ptr<Lock> lock = lock_or_null( lock_name );
    if( !lock )  lock = Z_NEW( Lock( this, lock_name ) );

    lock->set_dom( lock_element );
    _lock_map[ lock->name() ] = lock;
}

//----------------------------------------------------------------------Lock_subsystem::execute_xml

xml::Element_ptr Lock_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "lock" ) )  execute_xml_lock( element );
    else
    if( string_begins_with( element.nodeName(), "lock." ) ) 
    {
        lock( element.getAttribute( "lock" ) )->execute_xml( element, show_what );
    }
    else
        z::throw_xc( "SCHEDULER-113", element.nodeName() );

    return command_processor->_answer.createElement( "ok" );
}

//----------------------------------------------------------------------Lock_subsystem::dom_element

xml::Element_ptr Lock_subsystem::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "locks" );

    Z_FOR_EACH( Lock_map, _lock_map, it )
    {
        Lock* lock = it->second;
        result.appendChild( lock->dom_element( dom_document, show_what ) );
    }

    return result;
}

//-------------------------------------------------------------------------Lock_subsystem::add_lock

void Lock_subsystem::add_lock( Lock* lock )
{
    if( !lock )  z::throw_xc( __FUNCTION__ );

    if( lock->is_added() )  z::throw_xc( "SCHEDULER-416", lock->obj_name() );

    _lock_map[ lock->name() ] = lock;
}

//-------------------------------------------------------------------------Lock_subsystem::get_Lock

STDMETHODIMP Lock_subsystem::get_Lock( BSTR path_bstr, spooler_com::Ilock** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = lock( string_from_bstr( path_bstr ) );
        if( result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------Lock_subsystem::get_Lock_or_null

STDMETHODIMP Lock_subsystem::get_Lock_or_null( BSTR path_bstr, spooler_com::Ilock** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = lock_or_null( string_from_bstr( path_bstr ) );
        if( result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//----------------------------------------------------------------------Lock_subsystem::Create_lock

STDMETHODIMP Lock_subsystem::Create_lock( spooler_com::Ilock** result )
{
    HRESULT hr = S_OK;

    try
    {
        ptr<Lock> lock = Z_NEW( Lock( this ) );
        *result = lock.take();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------------Lock_subsystem::Add_lock

STDMETHODIMP Lock_subsystem::Add_lock( spooler_com::Ilock* lock )
{
    HRESULT hr = S_OK;

    try
    {
        add_lock( dynamic_cast<Lock*>( lock ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace lock
} //namespace scheduler
} //namespace sos
