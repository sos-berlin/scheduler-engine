package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.event.Message;


public class ExceptionEvent extends MainEvent
{
    private final Throwable throwable;
    private Message message = null;


    public ExceptionEvent(Throwable x) {
        throwable = x;
    }


    public Throwable exception() { return throwable; }

    
    @Override public Message getMessage() {
        if (message == null)  message = new Message(throwable.toString());
        return message;
    }
}
