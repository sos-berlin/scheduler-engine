package com.sos.scheduler.engine.kernel.scheduler.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.SchedulerIsCallableEvent;

@Deprecated
@ForCpp
public class SchedulerEntersSleepStateEvent extends Event implements SchedulerIsCallableEvent {
    @ForCpp public SchedulerEntersSleepStateEvent() {}
}
