package com.sos.scheduler.engine.kernel.log;

import static com.sos.scheduler.engine.kernel.util.Util.stringOrException;

import com.sos.scheduler.engine.eventbus.AbstractEvent;
import com.sos.scheduler.engine.kernel.event.Message;
import com.sos.scheduler.engine.kernel.event.SimpleMessage;

public abstract class LogEvent extends AbstractEvent {
    private final Message message;

    protected LogEvent(String message) {
        this.message = new SimpleMessage(message);
    }

    public final Message getMessage() {
        return message;
    }

    @Override public String toString() {
        return super.toString() + ", message=" + stringOrException(getMessage());
    }
}
