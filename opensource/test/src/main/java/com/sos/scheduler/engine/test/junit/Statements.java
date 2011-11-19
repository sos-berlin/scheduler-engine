package com.sos.scheduler.engine.test.junit;

import org.junit.runners.model.Statement;

public final class Statements {
    private Statements() {}

    public static final Statement ignoringStatement = new Statement() {
        @Override public void evaluate() {}
    };
}
