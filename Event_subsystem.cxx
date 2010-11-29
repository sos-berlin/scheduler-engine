// $Id$

#include "spooler.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__SpoolerC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__event__EventSubsystem.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__event__Event.h"
#include "../javaproxy/java__lang__String.h"

using namespace std;

namespace sos {
namespace scheduler {

typedef javaproxy::com::sos::scheduler::kernel::core::event::Event EventJ;
typedef javaproxy::com::sos::scheduler::kernel::core::event::EventSubsystem EventSubsystemJ;

//-------------------------------------------------------------------------------------------static
//--------------------------------------------------------------------------------------------const

//-------------------------------------------------------------------------------------------------

struct Event_subsystem_impl : Event_subsystem
{
                                Event_subsystem_impl        ( Scheduler* scheduler );

    // Subsystem
    void                        close                       ()                                      {}
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();
    string                      name                        () const                                { return "event_subsystem"; }

    // Event_subsystem
    void                        report                      (const EventJ&);
    //void                        report                      ( const Scheduler_event2& );

private:
    Fill_zero                  _zero_;
    EventSubsystemJ            _eventSubsystemJ;
};

//-------------------------------------------------------Event_subsystem_impl::Event_subsystem_impl

Event_subsystem_impl::Event_subsystem_impl(Scheduler* scheduler)
:  
    Event_subsystem( scheduler, type_event_subsystem ),    
    _zero_(this+1)
{
}

//------------------------------------------------------------------------------new_event_subsystem

ptr<Event_subsystem> new_event_subsystem( Scheduler* scheduler )
{
    ptr<Event_subsystem_impl> result = Z_NEW( Event_subsystem_impl( scheduler ) );
    return +result;
}

//----------------------------------------------------------------------Event_subsystem_impl::close

//void Event_subsystem_impl::close()
//{
//    _eventSubsystemJ.close();
//}

//-------------------------------------------------------Event_subsystem_impl::subsystem_initialize

bool Event_subsystem_impl::subsystem_initialize()
{
    _subsystem_state = subsys_initialized;
    return true;
}

//------------------------------------------------------------Event_subsystem_impl::subsystem_load

bool Event_subsystem_impl::subsystem_load()
{
    _eventSubsystemJ.assign_( _spooler->schedulerJ().getEventSubsystem() );
    assert(_eventSubsystemJ != NULL);

    _subsystem_state = subsys_loaded;
    return true;
}

//---------------------------------------------------------Event_subsystem_impl::subsystem_activate

bool Event_subsystem_impl::subsystem_activate()
{
    _subsystem_state = subsys_active;
    return true;
}

//---------------------------------------------------------------------Event_subsystem_impl::report

void Event_subsystem_impl::report(const EventJ& eventJ)
{
    if (_spooler->_java_events_allowed && _eventSubsystemJ) 
        _eventSubsystemJ.report(eventJ);
}

//---------------------------------------------------------------------Event_subsystem_impl::report

//void Event_subsystem_impl::report( const Scheduler_event2& event )
//{
//    _eventSubsystemJ.report(event.j());
//}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
