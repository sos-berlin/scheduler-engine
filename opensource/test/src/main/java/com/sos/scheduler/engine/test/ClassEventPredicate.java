package com.sos.scheduler.engine.test;

import com.sos.scheduler.engine.eventbus.EventPredicate;
import com.sos.scheduler.engine.data.event.Event;

public class ClassEventPredicate implements EventPredicate {
    private final Class<? extends Event> eventClass;


    public ClassEventPredicate(Class<? extends Event> eventClass) {
        this.eventClass = eventClass;
    }


    @Override public boolean apply(Event e) {
        return eventClass.isAssignableFrom(e.getClass());
    }


    @Override public String toString() {
        return eventClass.getName();
    }
}
