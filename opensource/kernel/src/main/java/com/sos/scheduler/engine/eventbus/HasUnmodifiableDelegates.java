package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.ModifiableSourceEvent;

public final class HasUnmodifiableDelegates {
    public static EventSource tryUnmodifiableEventSource(Event e, EventSource o) {
        return !(e instanceof ModifiableSourceEvent) &&
                o instanceof HasUnmodifiableDelegate?
                    ((HasUnmodifiableDelegate<?>)o).unmodifiableDelegate()
                    : o;
    }

    private HasUnmodifiableDelegates() {}
}
