package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventPredicate;



public class ScriptEventPredicate implements EventPredicate {
    private final ScriptEventPredicateEngine context;
    private final String expression;
    private final Class<? extends Event> eventClass;


    ScriptEventPredicate(ScriptEventPredicateEngine c, String expression, Class<? extends Event> eventClass) {
        this.context = c;
        this.expression = expression;
        this.eventClass = eventClass;
    }


    @Override public boolean apply(Event e) {
        return eventClass.isAssignableFrom(e.getClass())  &&  context.matches(e, expression);
    }


    @Override public String toString() {
        return eventClass.getSimpleName() + "(" + expression + ")";
    }
}
