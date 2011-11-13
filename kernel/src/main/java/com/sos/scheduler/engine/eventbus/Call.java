package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.kernel.event.Event;

public class Call {
    private final Event event;
    private final EventSubscription subscription;

    Call(Event event, EventSubscription subscription) {
        this.event = event;
        this.subscription = subscription;
    }

    final void apply() {
        subscription.handleEvent(event);
    }

    public final Event getEvent() {
        return event;
    }

    public final EventSubscription getSubscription() {
        return subscription;
    }

    @Override public String toString() {
        return Call.class.getSimpleName()+":"+ subscription +"("+event+")";
    }
}
