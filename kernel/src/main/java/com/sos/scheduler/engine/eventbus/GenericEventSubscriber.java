package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.kernel.event.Event;

/** {@link EventSubscriber2} für eine Event-Klasse.*/
public abstract class GenericEventSubscriber<E extends Event> implements EventSubscriber2 {
    private final Class<E> eventClass;

    protected GenericEventSubscriber(Class<E> eventClass) {
        this.eventClass = eventClass;
    }

    @Override public final Class<E> getEventClass() {
        return eventClass;
    }

    @SuppressWarnings("unchecked")
    @Override public final void onEvent(Event e) {
        handleTypedEvent((E)e);
    }

    protected abstract void handleTypedEvent(E e);
}
