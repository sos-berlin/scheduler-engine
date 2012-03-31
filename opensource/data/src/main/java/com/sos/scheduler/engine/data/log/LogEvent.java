package com.sos.scheduler.engine.data.log;

import com.sos.scheduler.engine.data.event.AbstractEvent;

public abstract class LogEvent extends AbstractEvent {
    private final String message;

    protected LogEvent(String message) {
        this.message = message;
    }

    public final String getMessage() {
        return message;
    }

    @Override public String toString() {
        return super.toString() +", message="+ message;
    }
}
