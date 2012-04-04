package com.sos.scheduler.engine.data.log;

import com.sos.scheduler.engine.data.event.AbstractEvent;

public abstract class LogEvent extends AbstractEvent {
    private final String line;

    protected LogEvent(String message) {
        this.line = message;
    }

    public final String getLine() {
        return line;
    }

    @Override public String toString() {
        return super.toString() +", line="+ line;
    }
}
