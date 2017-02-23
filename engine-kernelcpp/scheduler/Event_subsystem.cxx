// $Id: Event_subsystem.cxx 14167 2010-12-30 15:11:58Z jz $

#include "spooler.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__SpoolerC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__EventSubsystem.h"
#include "../javaproxy/com__sos__jobscheduler__data__event__KeyedEvent.h"
#include "../javaproxy/java__lang__String.h"

using namespace std;

namespace sos {
namespace scheduler {

typedef javaproxy::com::sos::jobscheduler::data::event::KeyedEvent KeyedEventJ;
typedef javaproxy::com::sos::scheduler::engine::kernel::event::EventSubsystem EventSubsystemJ;

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
    void                        report                      (const KeyedEventJ&);
    void                        report                      (const KeyedEventJ&, const ObjectJ&);
    virtual void                report_event_code           (Event_code, const ObjectJ& event_source);

    void report_logged(Log_level level, const string& prefix, const string& message) {
        if (_eventSubsystemJ) 
            _eventSubsystemJ.reportLogged((int)level, prefix, message);
    }

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

//-------------------------------------------------------Event_subsystem_impl::subsystem_initialize

bool Event_subsystem_impl::subsystem_initialize()
{
    _eventSubsystemJ.assign_( _spooler->schedulerJ().getEventSubsystem() );
    assert(_eventSubsystemJ != NULL);
    _eventSubsystemJ.checkNumberOfEventCodes((int)end_event_code);

    _subsystem_state = subsys_initialized;
    return true;
}

//------------------------------------------------------------Event_subsystem_impl::subsystem_load

bool Event_subsystem_impl::subsystem_load()
{
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

void Event_subsystem_impl::report(const KeyedEventJ& eventJ)
{
    if (_eventSubsystemJ) 
        _eventSubsystemJ.report(eventJ);
}

//----------------------------------------------------------Event_subsystem_impl::report_event_code

void Event_subsystem_impl::report_event_code(Event_code event_code, const ObjectJ& eventSourceJ) {
    if (_eventSubsystemJ) 
        _eventSubsystemJ.reportEventClass((int)event_code, eventSourceJ);
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
