package com.sos.scheduler.engine.main.event;

import com.sos.scheduler.engine.kernel.util.Time;

public class TimeoutEvent extends ExceptionEvent {
    public TimeoutEvent(Time timeout) {
        super(new RuntimeException("Timeout after " + timeout + " while waiting for event"));
    }
}
