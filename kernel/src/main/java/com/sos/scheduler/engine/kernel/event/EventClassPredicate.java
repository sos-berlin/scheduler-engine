package com.sos.scheduler.engine.kernel.event;


public class EventClassPredicate implements EventPredicate {
    private final Class<? extends Event> eventClass;


    public EventClassPredicate(Class<? extends Event> eventClass) {
        this.eventClass = eventClass;
    }


    @Override public final boolean apply(Event e) {
        return eventClass.isAssignableFrom(e.getClass());
    }


    @Override public String toString() {
        return eventClass.toString();
    }
}
