// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

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
        *result = lock( Absolute_path( root_path, string_from_bstr( path_bstr ) ) );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-----------------------------------------------------------------Lock_subsystem::get_Lock_or_null

STDMETHODIMP Lock_subsystem::get_Lock_or_null( BSTR path_bstr, spooler_com::Ilock** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = lock_or_null( Absolute_path( root_path, string_from_bstr( path_bstr ) ) );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------------------------Lock_subsystem::Create_lock

STDMETHODIMP Lock_subsystem::Create_lock( spooler_com::Ilock** result )
{
    HRESULT hr = S_OK;

    try
    {
        ptr<Lock> lock = Z_NEW( Lock( this ) );
        //nicht nötig  lock->set_folder_path( root_path );
        *result = lock.take();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------Lock_subsystem::Add_lock

STDMETHODIMP Lock_subsystem::Add_lock( spooler_com::Ilock* ilock )
{
    HRESULT hr = S_OK;

    try
    {
        // Das ist derselbe Algorithmus wie Add_job_chain(). Sollte zusammengefasst werden.

        Lock* lock = dynamic_cast<Lock*>( ilock );
        if( !lock)  return E_POINTER;

        //lock->set_defined();
        Folder* folder = spooler()->root_folder();
        lock->set_folder_path( folder->path() );
        lock->initialize();

        Lock* current_lock = lock_or_null( lock->path() );
        if( current_lock  &&  current_lock->is_to_be_removed() )
        {
            current_lock->replace_with( lock );
        }
        else
        {
            folder->lock_folder()->add_lock( lock );
            lock->activate();
        }
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------------------------Lock_subsystem::execute_xml

//xml::Element_ptr Lock_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
//{
//    xml::Element_ptr result;
//
//    if( element.nodeName_is( "lock" ) )  spooler()->root_folder()->lock_folder()->add_or_replace_file_based_xml( element );
//    else
//    if( string_begins_with( element.nodeName(), "lock." ) ) 
//    {
//        return lock( Absolute_path( root_path, element.getAttribute( "lock" ) ) )->execute_xml( command_processor, element, show_what );
//    }
//    else
//        z::throw_xc( "SCHEDULER-113", element.nodeName() );
//
//    return command_processor->_answer.createElement( "ok" );
//}

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
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << ": " << x.what() ); }
}

//--------------------------------------------------------------------------------------Lock::close

void Lock::close()
{
    for( Holder_map::iterator h = _holder_map.begin(); h != _holder_map.end(); )
    {
        Holder* holder = h->first;
        log()->error( message_string( "SCHEDULER-854", obj_name(), holder->obj_name() ) );
    }


    _waiting_queues.clear();


    //Z_FOR_EACH( Use_set, _use_set, u )
    //{
    //    Use* use = *u;
    //    use->disconnect();        Nicht nötig
    //}

    _use_set.clear();
}

//------------------------------------------------------------------------Lock::can_be_replaced_now

bool Lock::can_be_replaced_now()
{
    return replacement()  &&  
           replacement()->file_based_state() == File_based::s_initialized &&
           _holder_map.empty();
}

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
    return true;
}

//-------------------------------------------------------------------------Lock::can_be_removed_now

bool Lock::can_be_removed_now()
{
    return is_to_be_removed()  &&  _holder_map.empty();
}

//--------------------------------------------------------------------------Lock::prepare_to_remove

void Lock::prepare_to_remove( Remove_flags remove_flags )
{
    if( !is_to_be_removed() )
    {
        //_remove = true;

        //Z_FOR_EACH( Use_set, _use_set, u )
        //{
        //    Use* use = *u;
        //    use->requestor()->on_removing_lock( this );
        //}


        typedef stdext::hash_set< Requestor* >  Requestor_set;
        Requestor_set requestor_set;

        for( int lk = lk_exclusive; lk <= lk_non_exclusive; lk++ )
        {
            Z_FOR_EACH( Use_list, _waiting_queues[ lk ], u )
            {
                Use* lock_use = *u;
                requestor_set.insert( lock_use->requestor() );
            }
        }

        Z_FOR_EACH( Requestor_set, requestor_set, r )
        {
            Requestor* requestor = *r;
            requestor->dequeue_lock_requests();
        }
    }

    My_file_based::prepare_to_remove( remove_flags );
}

//-------------------------------------------------------------------------------Lock::remove_error

zschimmer::Xc Lock::remove_error()
{
    return zschimmer::Xc( "SCHEDULER-886", string_from_holders() );
}

//-------------------------------------------------------------------------Lock::prepare_to_replace

void Lock::prepare_to_replace()
{
    assert( replacement() );
}

//-----------------------------------------------------------------------------Lock::on_replace_now

Lock* Lock::on_replace_now()
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
           _holder_map.empty(); 
}

//--------------------------------------------------------------------------------Lock::is_free_for

bool Lock::is_free_for( const Use* lock_use, Holder* requesting_holder ) const
{ 
    bool free = lock_use->lock_mode() == lk_exclusive? is_free() 
                                                     : ( _lock_mode != lk_exclusive || is_free() )  &&  
                                                       count_non_exclusive_holders() < _config._max_non_exclusive;

    if( !free  &&  
        requesting_holder  &&  
        _holder_map.find( requesting_holder ) != _holder_map.end() )    // requesting_holder hält die Sperre bereits?
    {
        free = lock_use->lock_mode() == lk_non_exclusive  ||  _holder_map.size() == 1;
    }

    return free;
}

//--------------------------------------------------------------------------------Lock::its_my_turn

bool Lock::its_my_turn( const Use* lock_use, Holder* holder ) const
{
    bool result = false;

    if( is_free_for( lock_use, holder ) )
    {
        const Use_list&          queue = _waiting_queues[ lock_use->lock_mode() ];
        Use_list::const_iterator first = queue.begin();

        if( first == queue.end()  ||    // Warteschlange ist leer?
           *first == lock_use )         // Wir sind dran?
        {
            result = true;
        }
    }

    return result;
}

//---------------------------------------------------------------------------Lock::require_lock_for

bool Lock::require_lock_for( Holder* holder, Use* lock_use )
{
    assert_is_active();
    if( !holder->is_known_requestor( lock_use->requestor() ) )  z::throw_xc( Z_FUNCTION );
    if( !is_free_for( lock_use, holder ) )  z::throw_xc( Z_FUNCTION );

    bool is_freshly_required;

    Holder_map::iterator h = _holder_map.find( holder );

    if( h != _holder_map.end() )
    {
        if( _lock_mode == lk_non_exclusive  &&  lock_use->lock_mode() == lk_exclusive )
        {
            assert( _holder_map.size() == 1 );
            _lock_mode = lk_exclusive;   
            is_freshly_required = true;      // Holder hat seine Haltung von lk_non_exclusive auf lk_exclusive erhöht
        }
        else
            is_freshly_required = false;     // Holder hat nix verändert
    }
    else
    {
        _holder_map[ holder ] = Use_set();
        h = _holder_map.find( holder );
        _lock_mode = lock_use->lock_mode();
        is_freshly_required = true;      // Holder hält die Sperre neu oder mit strengerem Lock_mode
    }

    h->second.insert( lock_use );


    if( is_freshly_required )
    {
        holder->log()->info( message_string( "SCHEDULER-855", obj_name(), lock_use->lock_mode() == lk_exclusive? "exclusively" : "non-exclusively" ) );
    }

    return is_freshly_required;
}

//---------------------------------------------------------------------------Lock::release_lock_for

bool Lock::release_lock_for( Holder* holder, Use* lock_use )
{
    bool is_released = false;

    Holder_map::iterator h = _holder_map.find( holder );
    if( h != _holder_map.end() )
    {
        Use_set::iterator u = h->second.find( lock_use );
        if( u != h->second.end() )
        {
            h->second.erase( u );

            if( h->second.empty() )  
            {
                _holder_map.erase( h );    // Das ist der Normalfall: Der Holder hält die Sperre nur einmal und wird jetzt entfernt
                is_released = true;
            }
            else
            if( lock_use->lock_mode() == lk_exclusive )         // Der letzte exklusive Use? (nur noch nicht-exklusives Use des Holder?)
            {
                assert( _lock_mode == lk_exclusive );

                bool has_exclusive_use = false;
                Z_FOR_EACH_CONST( Use_set, h->second, uu )  if( (*uu)->lock_mode() == lk_exclusive )  { has_exclusive_use = true;  break; }
                
                if( !has_exclusive_use )  
                {
                    _lock_mode = lk_non_exclusive;
                    is_released = true;                 // Wenigstens lk_non_exclusive statt vorher lk_exclusive
                }
            }

            if( is_released )
            {
                if( int remaining = count_non_exclusive_holders() )
                {
                    holder->log()->info( message_string( "SCHEDULER-857", obj_name(), remaining ) );
                }
                else
                {
                    holder->log()->info( message_string( "SCHEDULER-856", obj_name() ) );
                }


                // Wartenden Requestor benachrichtigen

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
                    try
                    {
                        next_requestor->on_locks_are_available();  
                    }
                    catch( exception& x ) 
                    { 
                        next_requestor->log()->error( S() << x << ", in on_lock_are_available()" );
                    }
                }


                check_for_replacing_or_removing();
            }
        }
        //else  
        //  Z_DEBUG_ONLY( assert( !"Unbekannter Use" ) );
    }
    //else  
    //  Z_DEBUG_ONLY( assert( !"Unbekannter Holder" ) );

    return is_released;
}

//---------------------------------------------------------------------------------Lock::is_held_by

//bool Lock::is_held_by( Holder* holder, Lock_mode lock_mode )
//{
//    bool result = false;
//
//    Holder_map::iterator h = _holder_map.find( holder );
//    if( h != _holder_map.end() )
//    {
//        result = _lock_mode == lk_exclusive  || 
//                 lock_mode == lk_non_exclusive; 
//    }
//
//    return result;
//}

//---------------------------------------------------------------------------------Lock::is_held_by

bool Lock::is_held_by( Holder* holder, Use* lock_use )
{
    bool result = false;

    Holder_map::iterator h = _holder_map.find( holder );
    if( h != _holder_map.end() )
    {
        result = h->second.find( lock_use ) != h->second.end();
    }

    return result;
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

    Z_FOR_EACH( Use_list, list, u )
    {
        if( *u == lock_use )  
        {
            list.erase( u );
            break;
        }
    }
}

//----------------------------------------------------------------Lock::count_non_exclusive_holders

int Lock::count_non_exclusive_holders() const
{ 
    // Mehrfaches kl_exclusive wird toleriert, mehrfaches lk_non_exclusive zählt aber? Das ist vielleicht inkonsequent. 
    // Alse zählen wir doppelte Haltungen nicht.
    // Das erfordert weitere Behandlung in is_free_for() und assert() in require_lock_for():
    //int result = 0;

    //if( _lock_mode == lk_non_exclusive )
    //{
    //    Z_FOR_EACH_CONST( Holder_map, _holder_map, it )  result += it->second;      // Mehrfache Haltungen desselben Holder zählen mehrfach
    //}

    //return result;

    return _lock_mode == lk_non_exclusive? _holder_map.size() : 0; 
}

//------------------------------------------------------------------------------------Lock::set_dom

void Lock::set_dom( const xml::Element_ptr& lock_element )
{
    if( !lock_element.nodeName_is( "lock" ) )  z::throw_xc( "SCHEDULER-409", "lock", lock_element.nodeName() );

    clear_source_xml();
    set_max_non_exclusive( lock_element.int_getAttribute( "max_non_exclusive", _config._max_non_exclusive ) );
}

//--------------------------------------------------------------------------------Lock::dom_element

xml::Element_ptr Lock::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = dom_document.createElement( "lock" );

    fill_file_based_dom_element( result, show_what );

    if( _config._max_non_exclusive < INT_MAX )  result.setAttribute( "max_non_exclusive", _config._max_non_exclusive );

    if( is_free() )  result.setAttribute( "is_free", "yes" );

    if( !_holder_map.empty() )
    {
        xml::Element_ptr holders_element = result.append_new_element( "lock.holders" );
        holders_element.setAttribute( "exclusive", _lock_mode == lk_exclusive? "yes" : "no" );

        Z_FOR_EACH( Holder_map, _holder_map, h )
        {
            Holder* holder = h->first;

            xml::Element_ptr holder_element = holders_element.append_new_element( "lock.holder" );
            holder->object()->write_element_attributes( holder_element );
            if( h->second.size() > 1 )  holder_element.setAttribute( "count", (int64)h->second.size() );
        }
    }

    for( int lk = lk_exclusive; lk <= lk_non_exclusive; lk++ )
    {
        Use_list& queue = _waiting_queues[ lk ];

        if( !queue.empty() )
        {
            xml::Element_ptr queue_element = result.append_new_element( "lock.queue" );
            queue_element.setAttribute( "exclusive", lk == lk_exclusive? "yes" : "no" );

            Z_FOR_EACH( Use_list, queue, u )
            {
                Use* lock_use = *u;
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

    if( _holder_map.empty() )
    {
        result << "free";
    }
    else
    {
        result << ( _lock_mode == lk_exclusive? "exclusive holder " : "non-exclusive holder " );

        Z_FOR_EACH_CONST( Holder_map, _holder_map, h )
        {
            Holder* holder = h->first;

            if( h != _holder_map.begin() )  result << ", "; 
            result << holder->object()->obj_name();

            if( h->second.size() > 1 )  result << "(" << h->second.size() << "x)";       // Anzahl der Haltungen
        }
    }

    return result;
}

//---------------------------------------------------------------------------Lock::string_from_uses

string Lock::string_from_uses() const
{
    S result;

    Z_FOR_EACH_CONST( Use_set, _use_set, u )
    {
        Use* lock_use = *u;

        if( !result.empty() )  result << ", ";
        result << lock_use->requestor()->object()->obj_name();
    }

    return result;
}

//-----------------------------------------------------------------------------------Lock::obj_name

//string Lock::obj_name() const
//{
//    S result;
//
//    result << "Lock " << path().without_slash();
//
//    return result;
//}

//-----------------------------------------------------------------------------------Lock::put_Name

STDMETHODIMP Lock::put_Name( BSTR name_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_name( string_from_bstr( name_bstr ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

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
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------Lock::Remove

STDMETHODIMP Lock::Remove()
{
    HRESULT hr = S_OK;

    try
    {
        remove( rm_base_file_too );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//--------------------------------------------------------------------------------Lock::execute_xml

//void Lock::execute_xml( const xml::Element_ptr& element, const Show_what& )
//{
//    if( element.nodeName_is( "lock.remove" ) ) 
//    {
//        remove( File_based::rm_base_file_too );
//    }
//    else
//        z::throw_xc( "SCHEDULER-409", "lock.remove", element.nodeName() );
//}
//
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
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  ERROR " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------------------Requestor::close

void Requestor::close()
{ 
    Z_FOR_EACH( Use_list, _use_list, u )
    {
        Use* lock_use = *u;
        
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
    string normalized_path = Absolute_path( _folder_path, spooler()->lock_subsystem()->normalized_path( lock_use_element.getAttribute( "lock" ) ) );

    ptr<Use> lock_use;

    Z_FOR_EACH( Use_list, _use_list, u )
    {
        if( (*u)->lock_normalized_path() == normalized_path )
        {
            lock_use = *u;
            break;
        }
    }

    if( !lock_use )  
    {
        lock_use = Z_NEW( Use( this ) );
      //lock_use->set_folder_path( _folder_path );
        _use_list.push_back( lock_use );
    }

    lock_use->set_dom( lock_use_element );
}

//--------------------------------------------------------------------------Requestor::add_lock_use

Use* Requestor::add_lock_use( const Absolute_path& lock_path, Lock::Lock_mode lock_mode )
{
    ptr<Use> lock_use = lock_use_or_null( lock_path, lock_mode );

    if( !lock_use )
    {
        lock_use = Z_NEW( Use( this, lock_path, lock_mode ) );
        _use_list.push_back( lock_use );
    }

    return lock_use;
}

//----------------------------------------------------------------------Requestor::lock_use_or_null

Use* Requestor::lock_use_or_null( const Absolute_path& lock_path, Lock::Lock_mode lock_mode )
{
    ptr<Use>      lock_use             = NULL;
    Absolute_path normalized_lock_path ( _spooler->lock_subsystem()->normalized_path( lock_path ) );

    Z_FOR_EACH( Use_list, _use_list, u )
    {
        if( _spooler->lock_subsystem()->normalized_path( (*u)->lock_path() ) == normalized_lock_path  &&
            (*u)->lock_mode() == lock_mode )
        {
            lock_use = *u;
        }
    }

    return lock_use;
}

//-----------------------------------------------------------------------Requestor::set_folder_path

//void Requestor::set_folder_path( const Absolute_path& folder_path )
//{ 
//    Z_FOR_EACH( Use_list, _use_list, u )
//    {
//        Use* use = *u;
//        use->set_folder_path( folder_path );
//    }
//}

//----------------------------------------------------------------------------Requestor::initialize

void Requestor::initialize()
{
    Z_FOR_EACH( Use_list, _use_list, u )
    {
        Use* lock_use = *u;
        lock_use->initialize();
    }
}

//----------------------------------------------------------------------------------Requestor::load

void Requestor::load()
{
    Z_FOR_EACH( Use_list, _use_list, u )
    {
        Use* lock_use = *u;
        lock_use->load();
    }
}

//---------------------------------------------------------------------------Requestor::on_new_lock

//void Requestor::on_new_lock( Lock* lock )
//{
//    Z_FOR_EACH( Use_list, _use_list, u )
//    {
//        Use* use = *u;
//        
//        if( use->lock_or_null() == lock )
//        {
//            use->load();
//        }
//    }
//}

//-------------------------------------------------------------------Requestor::locks_are_available

bool Requestor::locks_are_available_for_holder( Holder* holder ) const
{
    bool all_locks_are_free = true;
    bool its_my_turn        = false;

    Z_FOR_EACH_CONST( Use_list, _use_list, u )
    {
        Use*  lock_use = *u;
        Lock* lock     = lock_use->lock_or_null();

        if( !lock  ||  !lock->is_free_for( lock_use, holder ) )
        {
            all_locks_are_free = false;
            break;
        }

        if( !its_my_turn )  its_my_turn = lock->its_my_turn( lock_use, holder );
    }

    return all_locks_are_free  &&  its_my_turn;   
}

//-----------------------------------------------------------------------Requestor::locks_are_known

bool Requestor::locks_are_known() const
{
    bool result = true;

    Z_FOR_EACH_CONST( Use_list, _use_list, u )
    {
        Use* lock_use = *u;
        
        if( !lock_use->lock_or_null() )  
        { 
            result = false; 
            break; 
        }
    }

    return result;
}

//-----------------------------------------------------------------Requestor::enqueue_lock_requests

bool Requestor::enqueue_lock_requests( Holder* holder )
{ 
    bool result = false;

    if( _is_enqueued )  assert(0), z::throw_xc( Z_FUNCTION );


    if( locks_are_known() )
    {
        _is_enqueued = true; 

        Z_FOR_EACH( Use_list, _use_list, u )
        {
            Use*  lock_use = *u;
            Lock* lock     = lock_use->lock();
            
            if( !holder  ||  !lock->is_held_by( holder, lock_use ) )
            {
                int place = lock->enqueue_lock_use( lock_use );    // Bei _use_list.size() > 1 kann die Sperre frei sein. Wir tragen uns trotzdem in die Warteschlange ein

                _log->info( message_string( "SCHEDULER-860", lock->obj_name(), lock_use->lock_mode() == Lock::lk_exclusive? "exclusive" : "non-exclusive", 
                                                             place, lock->string_from_holders() ) );
            }
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

        Z_FOR_EACH( Use_list, _use_list, u )
        {
            Use* lock_use = *u;
            
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

    Z_FOR_EACH( Use_list, _use_list, u )
    {
        Use* lock_use = *u;
        result.appendChild( lock_use->dom_element( dom_document, show_what ) );
    }

    return result;
}

//------------------------------------------------------------------------------Requestor::obj_name

string Requestor::obj_name() const
{
    S result;
    result << "Lock.Requestor(";
    
    Z_FOR_EACH_CONST( Use_list, _use_list, u )
    {
        if( u != _use_list.begin() )  result << ", ";
        Use* lock_use = *u;
        result << lock_use->obj_name();
    }

    result << ")";

    return result;
}

//-----------------------------------------------------------------------------------------Use::Use

Use::Use( Requestor* requestor, const Absolute_path& lock_path, Lock::Lock_mode lock_mode ) 
: 
    Scheduler_object( requestor->_spooler, this, type_lock_requestor ),
    _zero_(this+1), 
    _requestor(requestor),
    _lock_path( lock_path ),
    _lock_mode( lock_mode )
{
    _log = requestor->_log;
    set_folder_path( requestor->folder_path() );
}

//----------------------------------------------------------------------------------------Use::~Use
    
Use::~Use()
{ 
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  ERROR " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------------------------Use::close

void Use::close()
{ 
    if( Lock* lock = lock_or_null() )
    {
        lock->dequeue_lock_use( this );
        lock->unregister_lock_use( this );
    }

    remove_requisite( Requisite_path( spooler()->lock_subsystem(), _lock_path ) );
}

//-------------------------------------------------------------------------------------Use::set_dom

void Use::set_dom( const xml::Element_ptr& lock_use_element )
{
    if( !lock_use_element.nodeName_is( "lock.use" ) )  z::throw_xc( "SCHEDULER-409", "lock.use", lock_use_element.nodeName() );

    Absolute_path lock_path ( _folder_path, lock_use_element.getAttribute( "lock" ) );
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
        //if( _folder_path != "" )  _lock_path.set_absolute_if_relative( _folder_path );
        if( lock_normalized_path() != spooler()->lock_subsystem()->normalized_path( lock_path ) )  assert(0), z::throw_xc( Z_FUNCTION );
        if( _lock_mode != lock_mode )  z::throw_xc( "SCHEDULER-408", "lock.use", "exclusive" );
    }
}

//----------------------------------------------------------------------------------Use::initialize

void Use::initialize()
{
    add_requisite( Requisite_path( spooler()->lock_subsystem(), _lock_path ) );
}

//----------------------------------------------------------------------------------------Use::load

void Use::load()
{
    if( Lock* lock = this->lock_or_null() )
    {
        lock->register_lock_use( this );
    }
    //else
    //    add_requisite( spooler()->lock_subsystem(), _lock_path );
}

//-------------------------------------------------------------------------Use::on_requisite_loaded

bool Use::on_requisite_loaded( File_based* file_based )
{
    Lock_subsystem* lock_subsystem = spooler()->lock_subsystem();

    assert( file_based->subsystem() == lock_subsystem );
    assert( file_based->normalized_path() == lock_subsystem->normalized_path( _lock_path ) );

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

//-----------------------------------------------------------------------Lock::lock_normalized_path

string Use::lock_normalized_path() const
{ 
    return spooler()->lock_subsystem()->normalized_path( _lock_path ); 
}

//---------------------------------------------------------------------------------Use::dom_element

xml::Element_ptr Use::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    xml::Element_ptr result = dom_document.createElement( "lock.use" );

    result.setAttribute( "lock"     , _lock_path );
    result.setAttribute( "exclusive", _lock_mode == Lock::lk_exclusive? "yes" : "no" );

    if( Lock* lock = lock_or_null() )
    {
        if( _requestor->is_enqueued() )  result.setAttribute( "is_available", lock->is_free_for( this, (Holder*)NULL )? "yes" : "no" );
    }
    else
        result.setAttribute( "is_missing", "yes" );


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

Holder::Holder( Scheduler_object* o ) 
: 
    Scheduler_object( o->_spooler, this, type_lock_holder ),
    _zero_(this+1), 
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
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
}

//------------------------------------------------------------------------------------Holder::close

void Holder::close()
{
    //if( _is_holding )
    {
        release_locks();
    }
}

//----------------------------------------------------------------------------Holder::add_requestor

void Holder::add_requestor( const Requestor* requestor )
{
    assert( !is_known_requestor( requestor ) );

    _requestor_set.insert( requestor );
}

//----------------------------------------------------------------------------Holder::add_requestor

void Holder::remove_requestor( const Requestor* requestor )
{
    assert( is_known_requestor( requestor ) );
    assert( is_holding_none_of( requestor ) );

    release_locks( requestor );         // Sollte bereits passiert sein
    _requestor_set.erase( requestor );
}

//-------------------------------------------------------------------------------Holder::hold_locks

void Holder::hold_locks( const Requestor* requestor )
{
  //assert( is_holding_requestor( requestor ) );
    assert( requestor->locks_are_available_for_holder( this ) );
    assert( is_known_requestor( requestor ) );

  //_holding_requestor_set.insert( requestor );

    Z_FOR_EACH_CONST( Requestor::Use_list, requestor->_use_list, u )
    {
        Use* lock_use = *u;

        hold_lock( lock_use );
    }

    assert( is_holding_all_of( requestor ) );
}

//----------------------------------------------------------------------------Holder::release_locks

void Holder::release_locks()
{
    Z_FOR_EACH( set<const Requestor*>, _requestor_set, r )
    {
        release_locks( *r );
    }
}

//----------------------------------------------------------------------------Holder::release_locks

void Holder::release_locks( const Requestor* requestor )
{
    assert( is_known_requestor( requestor ) );

  //if( is_holding_requestor( requestor ) )
    {
        Z_FOR_EACH_CONST( Requestor::Use_list, requestor->_use_list, u ) 
        {
            Use* lock_use = *u;
            
            try
            {
                lock_use->lock()->release_lock_for( this, lock_use );
            }
            catch( exception& x )
            {
                log()->error( S() << x.what() << ", in " << Z_FUNCTION );
            }
        }

      //_holding_requestor_set.erase( requestor );
    }

    assert( is_holding_none_of( requestor ) );
}

//---------------------------------------------------------------------------------Holder::try_hold

bool Holder::try_hold( Use* lock_use )
{
    bool result = false;

  //_is_holding = XXXX;     //?? Die per API angeforderten Sperren sind gehalten oder auch nicht


    Lock* lock = lock_use->lock();
    
    if( lock->is_free_for( lock_use, this ) ) 
    {
        //if( lock->its_my_turn( lock_use ) ) ...   Reihenfolge der Warteanforderungen beachten wir hier nicht
        hold_lock( lock_use );
        result = true;
    }

    return result;
}

//--------------------------------------------------------------------------------Holder::hold_lock

void Holder::hold_lock( Use* lock_use )
{
    lock_use->lock()->require_lock_for( this, lock_use );      // false, wenn Holder bereits die Sperre mit gleichen oder schwächeren Lock_mode hält
}

//------------------------------------------------------------------------------Holder::dom_element

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

//-----------------------------------------------------------------------Holder::is_known_requestor

bool Holder::is_known_requestor( const Requestor* requestor )
{
    return _requestor_set.find( requestor ) != _requestor_set.end();
}

//------------------------------------------------------------------------Holder::is_holding_all_of

bool Holder::is_holding_all_of( const Requestor* requestor )
{
    assert( is_known_requestor( requestor ) );

    bool result = !requestor->_use_list.empty();

    Z_FOR_EACH_CONST( Requestor::Use_list, requestor->_use_list, u )
    {
        Use*  lock_use = *u;
        Lock* lock     = _spooler->lock_subsystem()->lock_or_null( lock_use->lock_path() );

        result &= lock && lock->is_held_by( this, lock_use );
        if( !result )  break;
    }

    return result;
}

//-----------------------------------------------------------------------Holder::is_holding_none_of

bool Holder::is_holding_none_of( const Requestor* requestor )
{
    assert( is_known_requestor( requestor ) );

    bool is_holding_some = false;

    Z_FOR_EACH_CONST( Requestor::Use_list, requestor->_use_list, u )
    {
        Use*  lock_use = *u;
        Lock* lock     = _spooler->lock_subsystem()->lock_or_null( lock_use->lock_path() );

        is_holding_some |= lock && lock->is_held_by( this, lock_use );
        if( is_holding_some )  break;
    }

    return !is_holding_some;
}

//---------------------------------------------------------------------Holder::is_holding_requestor

//bool Holder::is_holding_requestor( const Requestor* requestor )
//{
//    return _holding_requestor_set.find( requestor ) != _holding_requestor_set.end();
//}

//---------------------------------------------------------------------------------Holder::obj_name

string Holder::obj_name() const
{
    S result;

    result << "Holder(";
    
    Z_FOR_EACH_CONST( set<const Requestor*>, _requestor_set, r )  
    {
        if( r != _requestor_set.begin() )  result << " ";
        result << (*r)->obj_name();
    }

    result << ")";

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace lock
} //namespace scheduler
} //namespace sos
