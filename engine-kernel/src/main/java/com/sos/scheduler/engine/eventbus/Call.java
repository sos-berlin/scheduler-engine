package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.KeyedEvent;

public class Call {
    private final KeyedEvent<Event> event;
    private final EventSubscription subscription;

    Call(KeyedEvent<Event> event, EventSubscription subscription) {
        this.event = event;
        this.subscription = subscription;
    }

    final void apply() {
        subscription.handleEvent(event);
    }

    public final KeyedEvent<Event> getEvent() {
        return event;
    }

    public final EventSubscription getSubscription() {
        return subscription;
    }

    @Override public String toString() {
        return Call.class.getSimpleName()+":"+ subscription +"("+event+")";
    }
}
