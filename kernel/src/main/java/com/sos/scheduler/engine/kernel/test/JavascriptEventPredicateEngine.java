package com.sos.scheduler.engine.kernel.test;

import javax.script.ScriptEngineManager;


public class JavascriptEventPredicateEngine  extends ScriptEventPredicateEngine {
    public JavascriptEventPredicateEngine() {
        super(new ScriptEngineManager().getEngineByName("JavaScript"));
    }
}
