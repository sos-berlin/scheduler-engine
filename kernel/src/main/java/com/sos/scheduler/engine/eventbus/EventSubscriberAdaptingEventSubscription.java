package com.sos.scheduler.engine.eventbus;

import static com.google.common.base.Throwables.propagate;

import com.sos.scheduler.engine.kernel.event.EventSubscriber;

public class EventSubscriberAdaptingEventSubscription implements EventSubscription {
    private final EventSubscriber oldEventSubscriber;

    public EventSubscriberAdaptingEventSubscription(EventSubscriber oldEventSubscriber) {
        this.oldEventSubscriber = oldEventSubscriber;
    }

    @Override public Class<? extends Event> getEventClass() {
        return Event.class;
    }

    @Override public void handleEvent(Event e) {
        try {
            oldEventSubscriber.onEvent(e);
        } catch (Exception x) {
            throw propagate(x);
        }
    }

    @Override public int hashCode() {
        return oldEventSubscriber.hashCode();
    }

    @Override public boolean equals(Object o) {
        return o instanceof EventSubscriberAdaptingEventSubscription && oldEventSubscriber.equals(o);
    }
}
