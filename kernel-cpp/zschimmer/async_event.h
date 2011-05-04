// $Id$

#ifndef __Z_ASYNC_EVENT_H
#define __Z_ASYNC_EVENT_H

#include "async.h"

//-------------------------------------------------------------------------------------------------

namespace zschimmer { 

struct Event_manager;

//-------------------------------------------------------------------------------------Socket_event
// Hat nicht so viel mit Sockets zu tun

#ifdef Z_WINDOWS
    typedef Event               Socket_event;
# else
    typedef Simple_event        Socket_event;
#endif

//-------------------------------------------------------------------------------------------------

struct Event_operation : Async_operation
{
                                Event_operation             ();// Event_manager* );
                               ~Event_operation             ();

    void                        add_to_event_manager        ( Event_manager* );
    void                        remove_from_event_manager   ();
    virtual Socket_event*       async_event                 ()                                      = 0;
    virtual bool                async_signaled_             ();

  private:
    Fill_zero                  _zero_;
    Event_manager*             _event_manager;
};

//------------------------------------------------------------------------------------Event_manager

struct Event_manager : Async_manager
{
                                Event_manager               ();
                               ~Event_manager               ();


    void                        get_events                  ( std::vector<Socket_event*>* );
    int                         wait                        ( double timeout_seconds );
    bool                        async_continue              ( double timeout = 0.0 )                  { return async_continue_selected( NULL, timeout ); }
    bool                        async_continue_selected     ( Operation_is_ok f, double timeout = 0.0 );


  protected:
    friend struct               Event_operation;

    // Benutze Event_operation::add_to_event_manager()!
    void                        add_event_operation         ( Event_operation* );
    void                        remove_event_operation      ( Event_operation* );

    bool                        async_continue_selected_    ( Operation_is_ok* );


    Fill_zero                  _zero_;

    typedef std::list< Event_operation* >  Event_operation_list;
    Event_operation_list       _event_operation_list;
    bool                       _event_operation_list_modified;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
