package com.sos.scheduler.engine.eventbus;

public final class HasUnmodifiableDelegates {
    public static EventSource tryUnmodifiableEventSource(Event e, EventSource o) {
        return !(e instanceof ModifiableSourceEvent) &&
                o instanceof HasUnmodifiableDelegate?
                    ((HasUnmodifiableDelegate<?>)o).unmodifiableDelegate()
                    : o;
    }

    private HasUnmodifiableDelegates() {}
}
