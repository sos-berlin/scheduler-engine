package com.sos.scheduler.engine.kernel.test.junit;

import static com.sos.scheduler.engine.kernel.test.junit.Statements.ignoringStatement;
import static com.sos.scheduler.engine.kernel.util.Util.booleanOf;

import org.apache.log4j.Logger;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

public class SlowTestRule implements TestRule {
    private static final Logger log = Logger.getLogger(SlowTestRule.class);
    private static final boolean testSlow = booleanOf(System.getProperty("slowTest"), false, true);
    public static final SlowTestRule singleton = new SlowTestRule();


    private SlowTestRule() {}

    @Override public Statement apply(Statement stmt, Description description) {
        if (!testSlow) {
            log.warn("Slow test "+description.getTestClass().getName()+" is suppressed");
            return ignoringStatement;
        } else
            return stmt;
    }
}
