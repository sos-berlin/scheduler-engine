#ifndef __SCHEDULER_EVENT_CODE_H
#define __SCHEDULER_EVENT_CODE_H

namespace sos {
namespace scheduler {

// Gleicher Aufbau wie Java CppEventCode

enum Event_code {
    fileBasedActivatedEvent,
    fileBasedRemovedEvent,

    taskStartedEvent,
    taskEndedEvent,

    orderTouchedEvent,
    orderFinishedEvent,
    orderSuspendedEvent,
    orderResumedEvent,

    orderStepStartedEvent,

    // Endemarke:
    end_event_code
};

}} //namespace

#endif
