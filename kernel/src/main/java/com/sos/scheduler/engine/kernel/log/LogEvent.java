package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.event.AbstractEvent;
import com.sos.scheduler.engine.kernel.event.Message;
import com.sos.scheduler.engine.kernel.event.SimpleMessage;

//TODO Sollte ObjectEvent erweitern? In C++ dann das Objekt übergeben (spooler_log.cxx)

public class LogEvent extends AbstractEvent {
    private final Message message;


    public LogEvent(String message) {
        this.message = new SimpleMessage(message);
    }


    @Override public final Message getMessage() {
        return message;
    }
}
