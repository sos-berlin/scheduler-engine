// $Id: Event_subsystem.h 14011 2010-09-13 13:28:52Z jz $

#ifndef __SCHEDULER_EVENT_SUBSYSTEM_H
#define __SCHEDULER_EVENT_SUBSYSTEM_H


namespace sos {
namespace scheduler {

struct Scheduler_event2;

//----------------------------------------------------------------------------------Event_subsystem

struct Event_subsystem: Object, Subsystem
{
                                Event_subsystem             ( Scheduler* scheduler, Type_code t )  : Subsystem( scheduler, this, t ) {}

    virtual void                report                      (const AbstractEventJ&)                         = 0;
    virtual void                report                      (const AbstractEventJ&, const ObjectJ& event_source) = 0;
    virtual void                report_event_code           (Event_code, const ObjectJ& event_source) = 0;
};


ptr<Event_subsystem>            new_event_subsystem         ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
