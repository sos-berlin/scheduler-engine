package com.sos.scheduler.engine.test.junit;

import org.junit.runners.model.Statement;

final class Statements {
    private Statements() {}

    static final Statement ignoringStatement = new Statement() {
        @Override public void evaluate() {}
    };
}
