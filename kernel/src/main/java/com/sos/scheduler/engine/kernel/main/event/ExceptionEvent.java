package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.event.Message;
import com.sos.scheduler.engine.kernel.event.SimpleMessage;


public class ExceptionEvent extends MainEvent {
    private final Throwable throwable;
    private Message message = null;


    public ExceptionEvent(Throwable x) {
        throwable = x;
    }


    public final Throwable getException() {
        return throwable;
    }

    
    @Override public final Message getMessage() {
        if (message == null)  message = new SimpleMessage(throwable.toString());
        return message;
    }
}
