package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.kernel.event.Event;

public final class EventHandlerFailedEvent extends Event {
    private final Call call;
    private final Throwable throwable;

    public EventHandlerFailedEvent(Call call, Throwable throwable) {
        this.call = call;
        this.throwable = throwable;
    }

    public Call getCall() {
        return call;
    }

    public Throwable getThrowable() {
        return throwable;
    }

    @Override public String toString() {
        return call + "=>" + throwable;
    }
}
