package com.sos.scheduler.engine.test;

import javax.script.ScriptEngineManager;


public class JavascriptEventPredicateEngine  extends ScriptEventPredicateEngine {
    public JavascriptEventPredicateEngine() {
        super(new ScriptEngineManager().getEngineByName("JavaScript"));
    }
}