package com.sos.scheduler.engine.test;

import com.sos.scheduler.engine.eventbus.Event;

public class ScriptEventPredicate extends ClassEventPredicate {
    private final ScriptEventPredicateEngine context;
    private final String expression;


    ScriptEventPredicate(ScriptEventPredicateEngine c, String expression, Class<? extends Event> eventClass) {
        super(eventClass);
        this.context = c;
        this.expression = expression;
    }


    @Override public final boolean apply(Event e) {
        return super.apply(e)  &&  context.matches(e, expression);
    }


    @Override public final String toString() {
        return super.toString() + "(" + expression + ")";
    }
}
