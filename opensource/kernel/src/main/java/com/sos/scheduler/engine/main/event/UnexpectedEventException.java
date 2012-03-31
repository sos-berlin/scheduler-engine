package com.sos.scheduler.engine.main.event;

import com.sos.scheduler.engine.eventbus.EventPredicate;
import com.sos.scheduler.engine.data.event.Event;

public class UnexpectedEventException extends RuntimeException {
    private static final long serialVersionUID = 2011040301;
    
    private final Event event;

    public UnexpectedEventException(Event event, EventPredicate p, int expectEventNumber) {
        super("expectEvent #" + expectEventNumber + ": unexpected '" + event + "', expected was '" + p + "'");
        this.event = event;
    }

    public static UnexpectedEventException of(Event event, EventPredicate p, int expectedEventNumber) {
        return event instanceof TerminatedEvent?
            new UnexpectedTerminatedEventException((TerminatedEvent)event, p, expectedEventNumber)
          : new UnexpectedEventException(event, p, expectedEventNumber);
    }

    public final Event event() {
        return event;
    }
}
