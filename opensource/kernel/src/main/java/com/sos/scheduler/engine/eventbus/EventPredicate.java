package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;

public interface EventPredicate {
    boolean apply(Event e);

    EventPredicate alwaysTrue = new EventPredicate() {
        @Override public boolean apply(Event e) { return true; }
    };
}
