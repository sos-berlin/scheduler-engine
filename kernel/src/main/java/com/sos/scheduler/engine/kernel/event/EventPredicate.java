package com.sos.scheduler.engine.kernel.event;


public interface EventPredicate {
    boolean apply(Event e);


    EventPredicate alwaysTrue = new EventPredicate() {
        @Override public boolean apply(Event e) { return true; }
    };
}
