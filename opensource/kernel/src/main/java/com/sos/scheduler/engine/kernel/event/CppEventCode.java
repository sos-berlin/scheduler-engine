package com.sos.scheduler.engine.kernel.event;

/** Gleicher Aufbau wie C++ enum Event_code. */
public enum CppEventCode {
    fileBasedActivatedEvent,
    fileBasedRemovedEvent,

    taskStartedEvent,
    taskEndedEvent,

    orderTouchedEvent,
    orderFinishedEvent,
    orderSuspendedEvent,
    orderResumedEvent,

    orderStepStartedEvent,
}
