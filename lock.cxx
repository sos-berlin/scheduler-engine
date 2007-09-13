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
    COM_PROPERTY_GET( Lock_subsystem,  0, Lock                , VT_DISPATCH, 0, VT_BSTR ),
    COM_PROPERTY_GET( Lock_subsystem,  2, Lock_or_null        , VT_DISPATCH, 0, VT_BSTR ),
    COM_METHOD      ( Lock_subsystem,  3, Create_lock         , VT_DISPATCH, 0 ),
    COM_METHOD      ( Lock_subsystem,  4, Add_lock            , VT_EMPTY   , 0, VT_DISPATCH ),
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

//-------------------------------------------------------------------Lock_subsystem::Lock_subsystem

Lock_subsystem::Lock_subsystem( Scheduler* scheduler )
:
    Idispatch_implementation( &class_descriptor ),
    file_based_subsystem<Lock>( scheduler, static_cast<spooler_com::Ilocks*>( this ), type_lock_subsystem )
{
}

//----------------------------------------------------------------------------Lock_subsystem::close
    
void Lock_subsystem::close()
{
    set_subsystem_state( subsys_stopped );
    file_based_subsystem<Lock>::close();
}

//-------------------------------------------------------------Lock_subsystem::subsystem_initialize

bool Lock_subsystem::subsystem_initialize()
{
    file_based_subsystem<Lock>::subsystem_initialize();
    set_subsystem_state( subsys_initialized );
    return true;
}

//-------------------------------------------------------------------Lock_subsystem::subsystem_load

bool Lock_subsystem::subsystem_load()
{
    file_based_subsystem<Lock>::subsystem_load();
    set_subsystem_state( subsys_loaded );
    return true;
}

//---------------------------------------------------------------Lock_subsystem::subsystem_activate

bool Lock_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    file_based_subsystem<Lock>::subsystem_activate();
    return true;
}

//-------------------------------------------------------------Lock_subsystem<Lock>::new_file_based

ptr<Lock> Lock_subsystem::new_file_based()
{
    return Z_NEW( Lock( this ) );
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

STDMETHODIMP Lock_subsystem::Add_lock( spooler_com::Ilock* ilock )
{
    HRESULT hr = S_OK;

    try
    {
        Lock* lock = dynamic_cast<Lock*>( ilock );
        spooler()->root_folder()->lock_folder()->add_lock( lock );
        lock->activate();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------------Lock_folder::execute_xml

xml::Element_ptr Lock_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "lock" ) )  spooler()->root_folder()->lock_folder()->execute_xml_lock( element );
    else
    if( string_begins_with( element.nodeName(), "lock." ) ) 
    {
        lock( element.getAttribute( "lock" ) )->execute_xml( element, show_what );
    }
    else
        z::throw_xc( "SCHEDULER-113", element.nodeName() );

    return command_processor->_answer.createElement( "ok" );
}

//-------------------------------------------------------------------------Lock_folder::Lock_folder

Lock_folder::Lock_folder( Folder* folder )
:
    typed_folder<Lock>( folder->spooler()->lock_subsystem(), folder, type_lock_folder )
{
}

//------------------------------------------------------------------------Lock_folder::~Lock_folder
    
Lock_folder::~Lock_folder()
{
}

//-----------------------------------------------------------------------------Lock_folder::set_dom

void Lock_folder::set_dom( const xml::Element_ptr& locks_element )
{
    assert( locks_element.nodeName_is( "locks" ) );

    DOM_FOR_EACH_ELEMENT( locks_element, lock_element )
    {
        execute_xml_lock( lock_element );
    }
}

//--------------------------------------------------------------------Lock_folder::execute_xml_lock

void Lock_folder::execute_xml_lock( const xml::Element_ptr& lock_element )
{
    if( !lock_element.nodeName_is( "lock" ) )  z::throw_xc( "SCHEDULER-409", "lock", lock_element.nodeName() );

    string lock_name = lock_element.getAttribute( "name" );
    
    ptr<Lock> lock = lock_or_null( lock_name );
    if( !lock )  
    {
        lock = Z_NEW( Lock( subsystem(), lock_name ) );
        lock->set_dom( lock_element );
        add_lock( lock );
        lock->activate();
    }
    else
        lock->set_dom( lock_element );

    lock->activate();
}

//-------------------------------------------------------------------------Lock_folder::dom_element

xml::Element_ptr Lock_folder::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "locks" );

    for( File_based_map::iterator it = _file_based_map.begin(); it != _file_based_map.end(); it++ )
    {
        Lock* lock = static_cast<Lock*>( +it->second );
        result.appendChild( lock->dom_element( dom_document, show_what ) );
    }

    return result;
}

//---------------------------------------------------------------------------------------Lock::Lock

Lock::Lock( Lock_subsystem* lock_subsystem, const string& name )
:
    Idispatch_implementation( &class_descriptor ),
    file_based<Lock,Lock_folder,Lock_subsystem>( lock_subsystem, static_cast<spooler_com::Ilock*>( this ), type_lock ),
    _zero_(this+1),
    _waiting_queues(2)                  // [lk_exclusive] und [lk_non_ecklusive]
{
    _config._max_non_exclusive = INT_MAX;

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


    //Z_FOR_EACH( Use_set, _use_set, it )
    //{
    //    Use* use = *it;
    //    use->disconnect();        Nicht nötig
    //}

    _use_set.clear();
}

//-----------------------------------------------------------------------Lock::on_base_file_changed

//Lock* Lock::on_base_file_changed( Lock* new_lock )
//{
//    set_max_non_exclusive( new_lock->_config._max_non_exclusive );
//
//    set_replacement( NULL );
//
//    return this;
//}

//bool Lock::can_be_replaced_now()
//{
//    return _replacement_is_valid();
//}

//------------------------------------------------------------------------------Lock::on_initialize

bool Lock::on_initialize()
{
    return true;
}

//------------------------------------------------------------------------------------Lock::on_load

bool Lock::on_load()
{
    return true;
}

//--------------------------------------------------------------------------------Lock::on_activate

bool Lock::on_activate()
{
    //FOR_EACH_JOB( job )
    //{
    //    if( Requestor* requestor = job->lock_requestor_or_null() )
    //    {
    //        requestor->on_new_lock( this );
    //    }
    //}

    return true;
}

//-------------------------------------------------------------------------Lock::can_be_removed_now

bool Lock::can_be_removed_now()
{
    return _remove  &&  _holder_set.empty();
}

//--------------------------------------------------------------------------Lock::prepare_to_remove

bool Lock::prepare_to_remove()
{
    if( !_remove )
    {
        _remove = true;

        //Z_FOR_EACH( Use_set, _use_set, it )
        //{
        //    Use* use = *it;
        //    use->requestor()->on_removing_lock( this );
        //}


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
    }

    bool result = My_file_based::prepare_to_remove();
    if( !result )  log()->info( message_string( "SCHEDULER-886", string_from_holders() ) );     int ON_REMOVE_DELAYED;
    return result;
}

//-------------------------------------------------------------------------Lock::prepare_to_replace

bool Lock::prepare_to_replace()
{
    _remove = false;

    return can_be_replaced_now();
}

//--------------------------------------------------------------------------------Lock::replace_now

Lock* Lock::replace_now()
{
    set_max_non_exclusive( replacement()->_config._max_non_exclusive );

    set_replacement( NULL );

    return this;
}

//----------------------------------------------------------------------Lock::set_max_non_exclusive

void Lock::set_max_non_exclusive( int max_non_exclusive )
{
    if( _config._max_non_exclusive != max_non_exclusive )
    {
        if( max_non_exclusive < count_non_exclusive_holders() )  
            log()->warn( message_string( "SCHEDULER-887", max_non_exclusive, string_from_holders() ) );

        _config._max_non_exclusive = max_non_exclusive;
    }
}

//------------------------------------------------------------------------------------Lock::is_free

bool Lock::is_free() const
{ 
    // Siehe auch can_be_removed_now()

    return file_based_state() == File_based::s_active  &&  
           _holder_set.empty(); 
}

//--------------------------------------------------------------------------------Lock::is_free_for

bool Lock::is_free_for( Lock_mode lock_mode ) const
{ 
    return lock_mode == lk_exclusive? is_free() 
                                    : ( _lock_mode != lk_exclusive || is_free() )  &&  
                                      count_non_exclusive_holders() < _config._max_non_exclusive;

    //if( lock_mode == lk_non_exclusive  &&  _config._max_non_exclusive == 0 )  return false;
    //return is_free()  ||   
    //       lock_mode == lk_non_exclusive  &&  _lock_mode == lk_non_exclusive  &&  count_non_exclusive_holders() < _config._max_non_exclusive;
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
    assert_is_active();

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

    set_max_non_exclusive( lock_element.int_getAttribute( "max_non_exclusive", _config._max_non_exclusive ) );
}

//--------------------------------------------------------------------------------Lock::dom_element

xml::Element_ptr Lock::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "lock" );

    if( has_base_file() )  result.appendChild_if( File_based::dom_element( dom_document, show_what ) );
    if( replacement()   )  result.append_new_element( "replacement" ).appendChild( replacement()->dom_element( dom_document, show_what ) );

    result.setAttribute( "name", name() );
    if( _config._max_non_exclusive < INT_MAX )  result.setAttribute( "max_non_exclusive", _config._max_non_exclusive );

    if( is_free() )  result.setAttribute( "is_free", "yes" );

    //result.setAttribute( "state", state_name() );

    if( !_holder_set.empty() )
    {
        xml::Element_ptr holders_element = result.append_new_element( "lock.holders" );
        holders_element.setAttribute( "exclusive", _lock_mode == lk_exclusive? "yes" : "no" );

        Z_FOR_EACH( Holder_set, _holder_set, it )
        {
            Holder* holder = *it;

            xml::Element_ptr holder_element = holders_element.append_new_element( "lock.holder" );
            holder->object()->write_element_attributes( holder_element );
        }
    }

    for( int lk = lk_exclusive; lk <= lk_non_exclusive; lk++ )
    {
        Use_list& queue = _waiting_queues[ lk ];

        if( !queue.empty() )
        {
            xml::Element_ptr queue_element = result.append_new_element( "lock.queue" );
            queue_element.setAttribute( "exclusive", lk == lk_exclusive? "yes" : "no" );

            Z_FOR_EACH( Use_list, queue, it )
            {
                Use* lock_use = *it;
                xml::Element_ptr entry_element = queue_element.append_new_element( "lock.queue.entry" );

                Scheduler_object* object = lock_use->requestor()->object();
                object->write_element_attributes( entry_element );
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------Lock::string_from_holders

string Lock::string_from_holders() const
{
    S result;

    if( _holder_set.empty() )
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
    Path path = lock_use_element.getAttribute( "lock" );

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

//----------------------------------------------------------------------------Requestor::initialize

void Requestor::initialize()
{
    Z_FOR_EACH( Use_list, _use_list, it )
    {
        Use* lock_use = *it;
        lock_use->initialize();
    }
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

//---------------------------------------------------------------------------Requestor::on_new_lock

//void Requestor::on_new_lock( Lock* lock )
//{
//    Z_FOR_EACH( Use_list, _use_list, it )
//    {
//        Use* use = *it;
//        
//        if( use->lock_or_null() == lock )
//        {
//            use->load();
//        }
//    }
//}

//-------------------------------------------------------------------Requestor::locks_are_available

bool Requestor::locks_are_available() const
{
    bool all_locks_are_free = true;
    bool its_my_turn        = false;

    Z_FOR_EACH_CONST( Use_list, _use_list, it )
    {
        Use*  lock_use = *it;
        Lock* lock     = lock_use->lock_or_null();

        if( !lock  ||  !lock->is_free_for( lock_use->lock_mode() ) )
        {
            all_locks_are_free = false;
            break;
        }

        if( !its_my_turn )  its_my_turn = lock->its_my_turn( lock_use );
    }

    return all_locks_are_free  &&  its_my_turn;   
}

//-----------------------------------------------------------------Requestor::enqueue_lock_requests

bool Requestor::enqueue_lock_requests()
{ 
    bool result = false;

    if( _is_enqueued )  z::throw_xc( __FUNCTION__ );


    bool all_locks_are_there = true;

    Z_FOR_EACH( Use_list, _use_list, it )
    {
        Use*  lock_use = *it;
        if( !lock_use->lock_or_null() )  { all_locks_are_there = false; break; }
    }

    if( all_locks_are_there )
    {
        _is_enqueued = true; 

        Z_FOR_EACH( Use_list, _use_list, it )
        {
            Use*  lock_use = *it;
            Lock* lock     = lock_use->lock();
            
            int place = lock->enqueue_lock_use( lock_use );    // Bei _use_list.size() > 1 kann die Sperre frei sein. Wir tragen uns trotzdem in die Warteschlange ein
                
            _log->info( message_string( "SCHEDULER-860", lock->obj_name(), lock_use->lock_mode() == Lock::lk_exclusive? "exclusive" : "non-exclusive", 
                                                         place, lock->string_from_holders() ) );
        }

        result = true;
    }

    return result;
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

    remove_dependant( spooler()->lock_subsystem(), _lock_path );
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

//----------------------------------------------------------------------------------Use::initialize

void Use::initialize()
{
    add_dependant( spooler()->lock_subsystem(), _lock_path );
}

//----------------------------------------------------------------------------------------Use::load

void Use::load()
{
    if( Lock* lock = this->lock_or_null() )
    {
        lock->register_lock_use( this );
    }
    //else
    //    add_dependant( spooler()->lock_subsystem(), _lock_path );
}

//---------------------------------------------------------------------Use::on_dependant_incarnated

bool Use::on_dependant_incarnated( File_based* file_based )
{
    Lock_subsystem* lock_subsystem = spooler()->lock_subsystem();

    assert( file_based->subsystem() == lock_subsystem );
    assert( file_based->normalized_path() == lock_subsystem->normalized_name( _lock_path ) );

    Lock* lock = dynamic_cast<Lock*>( file_based );
    assert( lock );

    load();

    assert( this->lock() );
    return true;
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

    if( _requestor->is_enqueued() )
    {
        if( Lock* lock = lock_or_null() )
            result.setAttribute( "is_available", lock->is_free_for( _lock_mode )? "yes" : "no" );
        else
            result.setAttribute( "is_missing", "yes" );
    }

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


    _is_holding = true;

    Z_FOR_EACH_CONST( Requestor::Use_list, _requestor->_use_list, it )
    {
        Use*  lock_use = *it;
        Lock* lock     = lock_use->lock();

        lock->require_lock_for( this, lock_use );

        log()->info( message_string( "SCHEDULER-855", lock->obj_name(), lock_use->lock_mode() == Lock::lk_exclusive? "exclusively" : "non-exclusively" ) );
    }
}

//----------------------------------------------------------------------------Holder::release_locks

void Holder::release_locks()
{
    if( _is_holding )
    {
        Z_FOR_EACH_CONST( Requestor::Use_list, _requestor->_use_list, it ) 
        {
            Use* lock_use = *it;
            
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

                lock->check_for_replacing_or_removing();
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

//-------------------------------------------------------------------------------------------------

} //namespace lock
} //namespace scheduler
} //namespace sos
