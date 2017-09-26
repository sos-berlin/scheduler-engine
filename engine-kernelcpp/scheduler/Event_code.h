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

    jobChainNodeActionChanged,
    jobChainStateChanged,
    jobStateChanged,
    jobUnstopped,
    jobTaskQueueChanged,

    taskStartedEvent,
    taskClosedEvent,

    orderStartedEvent,
    orderFinishedEvent,
    orderSuspendedEvent,
    orderResumedEvent,
    orderSetBackEvent,

    orderWaitingInTask,
    orderStepStartedEvent,
    orderNestedStartedEvent,
    orderNestedFinishedEvent,

    // Endemarke:
    end_event_code
};

}} //namespace

#endif
