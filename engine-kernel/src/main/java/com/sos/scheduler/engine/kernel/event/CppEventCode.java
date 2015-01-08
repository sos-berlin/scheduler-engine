package com.sos.scheduler.engine.kernel.event;

/** Gleicher Aufbau wie C++ enum Event_code. */
public enum CppEventCode {
    fileBasedActivatedEvent,
    fileBasedAddedEvent,
    fileBasedRemovedEvent,
    fileBasedReplacedEvent,

    taskStartedEvent,
    taskEndedEvent_DELETED,
    taskClosedEvent,

    orderTouchedEvent,
    orderFinishedEvent,
    orderSuspendedEvent,
    orderResumedEvent,
    orderSetBackEvent,

    orderStepStartedEvent,
    orderNestedTouchedEvent,
    orderNestedFinishedEvent
}
