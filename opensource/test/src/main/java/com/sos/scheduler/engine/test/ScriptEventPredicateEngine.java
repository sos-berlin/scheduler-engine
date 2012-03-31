package com.sos.scheduler.engine.test;

import com.sos.scheduler.engine.eventbus.EventPredicate;
import com.sos.scheduler.engine.data.event.Event;

import javax.script.Bindings;
import javax.script.ScriptEngine;
import javax.script.ScriptException;


public class ScriptEventPredicateEngine {
    private final ScriptEngine scriptEngine;


    public ScriptEventPredicateEngine(ScriptEngine e) {
        this.scriptEngine = e;
    }


    public EventPredicate predicate(String expression) {
        return predicate(Event.class, expression);
    }


    public EventPredicate predicate(Class<? extends Event> eventClass, String expression) {
        return new ScriptEventPredicate(this, expression, eventClass);
    }


    public boolean matches(Event e, String expression) {
        try {
            Bindings bindings = scriptEngine.createBindings();
            bindings.put("event", e);
            return (Boolean)scriptEngine.eval(expression, bindings);
        }
        catch (ScriptException x) { throw new RuntimeException(x); }
    }
}
