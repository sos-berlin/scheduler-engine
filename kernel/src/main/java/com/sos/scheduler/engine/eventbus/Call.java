package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.kernel.event.Event;

public class Call {
    private final Event event;
    private final EventSubscriber2 subscriber;

    Call(Event event, EventSubscriber2 subscriber) {
        this.event = event;
        this.subscriber = subscriber;
    }

    final void apply() {
        subscriber.onEvent(event);
    }

    public final Event getEvent() {
        return event;
    }

    public final EventSubscriber2 getSubscriber() {
        return subscriber;
    }

    @Override public String toString() {
        return Call.class.getSimpleName()+":"+subscriber+"("+event+")";
    }
}
