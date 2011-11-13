package com.sos.scheduler.engine.eventbus;

import static com.google.common.base.Throwables.propagate;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;

public class EventSubscriber2Adapter implements EventSubscriber2 {
    private final EventSubscriber oldEventSubscriber;

    public EventSubscriber2Adapter(EventSubscriber oldEventSubscriber) {
        this.oldEventSubscriber = oldEventSubscriber;
    }

    @Override public Class<? extends Event> getEventClass() {
        return Event.class;
    }

    @Override public void onEvent(Event e) {
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
        return o instanceof EventSubscriber2Adapter && oldEventSubscriber.equals(o);
    }
}
