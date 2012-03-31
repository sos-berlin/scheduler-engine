package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;

/** {@link EventSubscription} für eine Event-Klasse.*/
public abstract class GenericEventSubscription<E extends Event> implements EventSubscription {
    private final Class<E> eventClass;

    protected GenericEventSubscription(Class<E> eventClass) {
        this.eventClass = eventClass;
    }

    @Override public final Class<E> getEventClass() {
        return eventClass;
    }

    @SuppressWarnings("unchecked")
    @Override public final void handleEvent(Event e) {
        handleTypedEvent((E)e);
    }

    protected abstract void handleTypedEvent(E e);
}
