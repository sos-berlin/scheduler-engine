package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.eventbus.AbstractEvent;

import static com.sos.scheduler.engine.kernel.util.Util.stringOrException;

public abstract class LogEvent extends AbstractEvent {
    private final String message;

    protected LogEvent(String message) {
        this.message = message;
    }

    public final String getMessage() {
        return message;
    }

    @Override public String toString() {
        return super.toString() + ", message=" + stringOrException(getMessage());
    }
}
