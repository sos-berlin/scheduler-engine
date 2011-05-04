// $Id$

#include "zschimmer.h"
#include "async_event.h"
#include "log.h"


using namespace std;

namespace zschimmer {

//-----------------------------------------------------------------Event_operation::Event_operation

Event_operation::Event_operation()
:
    _zero_(this+1)
{
}

//----------------------------------------------------------------Event_operation::~Event_operation
    
Event_operation::~Event_operation()
{
}

//-----------------------------------------------------------------Event_operation::async_signaled_

bool Event_operation::async_signaled_()
{
    if( Socket_event* event = async_event() )
    {
        return event->signaled();
    }

    return false;
}

//------------------------------------------------------------Event_operation::add_to_event_manager

void Event_operation::add_to_event_manager( Event_manager* event_manager )
{
    if( _event_manager )
    {
        if( _event_manager != event_manager )  throw_xc( Z_FUNCTION );
        return;
    }

    set_async_manager( event_manager );

    _event_manager = event_manager;
    _event_manager->add_event_operation( this );
}

//-------------------------------------------------------Event_operation::remove_from_event_manager

void Event_operation::remove_from_event_manager()
{
    if( _event_manager ) 
    {
        _event_manager->remove_event_operation( this );
        _event_manager = NULL;
    }

    set_async_manager( NULL );
}

//--------------------------------------------------------------------Event_manager::Event_manager

Event_manager::Event_manager()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------------Event_manager::~Event_manager

Event_manager::~Event_manager()
{
}

//---------------------------------------------------------------Event_manager::add_event_operation

void Event_manager::add_event_operation( Event_operation* op )
{
    _event_operation_list_modified = true;  // Für async_continue(), iteriert über _socket_operation_map.

    _event_operation_list.push_back( op );
    //if( op->_write_socket != SOCKET_ERROR )  _socket_operation_map[ op->_write_socket ] = op;
    //if( op->_read_socket  != SOCKET_ERROR )  _socket_operation_map[ op->_read_socket  ] = op;
}

//------------------------------------------------------------Event_manager::remove_event_operation

void Event_manager::remove_event_operation( Event_operation* op )
{
    _event_operation_list_modified = true;  // Für async_continue(), iteriert über _socket_operation_map.

    Z_FOR_EACH( Event_operation_list, _event_operation_list, o )
    {
        if( *o == op )  { _event_operation_list.erase( o );  break; }
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
