package com.sos.scheduler.engine.kernel.event;

/** Gleicher Aufbau wie C++ enum Event_code. */
public enum CppEventCode {
    fileBasedActivatedEvent,
    fileBasedAddedEvent,
    fileBasedRemovedEvent,
    fileBasedReplacedEvent,

    jobChainNodeActionChanged,
    jobChainStateChanged,
    jobStateChanged,

    taskStartedEvent,
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
