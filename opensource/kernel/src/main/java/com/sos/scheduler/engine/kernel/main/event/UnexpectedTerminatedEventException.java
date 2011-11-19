package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.eventbus.EventPredicate;

public class UnexpectedTerminatedEventException extends UnexpectedEventException {
    public UnexpectedTerminatedEventException(TerminatedEvent event, EventPredicate p, int expectEventNumber) {
        super(event, p, expectEventNumber);
    }

    public final TerminatedEvent getTerminatedEvent() {
        return (TerminatedEvent)event();
    }
}
