package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.event.EventPredicate;
import com.sos.scheduler.engine.kernel.util.Time;


public class EventExpectation {
    private final Time timeout;
    private final EventPredicate predicate;

    
    public EventExpectation(Time timeout, EventPredicate predicate) {
        this.timeout = timeout;
        this.predicate = predicate;
    }


    public Time getTimeout() {
        return timeout;
    }


    public EventPredicate getPredicate() {
        return predicate;
    }
}
