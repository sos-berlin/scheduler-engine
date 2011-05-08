package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventPredicate;


public class UnexpectedEventException extends RuntimeException {
    private static final long serialVersionUID = 2011040301;
    
    final Event event;
    final EventPredicate eventPredicate;

    public UnexpectedEventException(Event event, EventPredicate p, int expectEventNumber) {
        super("expectEvent #" + expectEventNumber + ": unexpected '" + event + "', expected was '" + p + "'");
        this.event = event;
        this.eventPredicate = p;
    }

    public static UnexpectedEventException of(Event event, EventPredicate p, int expectedEventNumber) {
        return event instanceof TerminatedEvent?
            new UnexpectedTerminatedEventException((TerminatedEvent)event, p, expectedEventNumber)
          : new UnexpectedEventException(event, p, expectedEventNumber);
    }
}
