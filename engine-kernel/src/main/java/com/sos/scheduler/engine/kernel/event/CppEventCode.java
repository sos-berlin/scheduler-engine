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
    jobUnstopped,
    jobTaskQueueChanged,

    taskStartedEvent,
    taskClosedEvent,

    orderRemovedEvent,
    orderStartedEvent,
    orderFinishedEvent,
    orderSuspendedEvent,
    orderResumedEvent,
    orderSetBackEvent,

    orderWaitingInTask,
    orderStepStartedEvent,
    orderNestedTouchedEvent,
    orderNestedFinishedEvent
}
