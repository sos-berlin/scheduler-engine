#ifndef __SCHEDULER_EVENT_CODE_H
#define __SCHEDULER_EVENT_CODE_H

namespace sos {
namespace scheduler {

// Gleicher Aufbau wie Java CppEventCode

enum Event_code {
    fileBasedActivatedEvent,
    fileBasedAddedEvent,
    fileBasedRemovedEvent,
    fileBasedReplacedEvent,

    taskStartedEvent,
    taskEndedEvent,
    taskClosedEvent,

    orderTouchedEvent,
    orderFinishedEvent,
    orderSuspendedEvent,
    orderResumedEvent,
    orderSetBackEvent,

    orderStepStartedEvent,
    orderNestedTouchedEvent,
    orderNestedFinishedEvent,

    // Endemarke:
    end_event_code
};

}} //namespace

#endif
