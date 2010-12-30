package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.Message;


public class LogEvent extends Event {
    private final Message message;


    public LogEvent(String message) {
        this.message = new Message(message);
    }


    @Override public Message getMessage() {
        return message;
    }
}
