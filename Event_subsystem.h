// $Id$

#ifndef __SCHEDULER_EVENT_SUBSYSTEM_H
#define __SCHEDULER_EVENT_SUBSYSTEM_H


namespace sos {
namespace scheduler {

struct Scheduler_event2;

//----------------------------------------------------------------------------------Event_subsystem

struct Event_subsystem: Object, Subsystem
{
                                Event_subsystem             ( Scheduler* scheduler, Type_code t )  : Subsystem( scheduler, this, t ) {}

    virtual void                report                      ( const Scheduler_event2& )             = 0;
};


ptr<Event_subsystem>            new_event_subsystem         ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
