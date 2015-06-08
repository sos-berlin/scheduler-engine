package com.sos.scheduler.engine.test.junit;

import static com.sos.scheduler.engine.test.junit.Statements.ignoringStatement;
import static com.sos.scheduler.engine.kernel.util.Util.booleanOf;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** JUnit-{@link TestRule}, die einen Test als langsam markiert und ihn nur ausführen lässt,
 * wenn die Property slowTest gesetzt ist.
 *
 * Verwendung: {@link org.junit.ClassRule} public static final TestRule slowTestRule = {@link SlowTestRule}.singleton;
 */
public final class SlowTestRule implements TestRule {
    private static final Logger logger = LoggerFactory.getLogger(SlowTestRule.class);
    private static final boolean testSlow = booleanOf(System.getProperty("slowTest"), false, true);
    public static final SlowTestRule singleton = new SlowTestRule();

    private SlowTestRule() {}

    @Override public Statement apply(Statement stmt, Description description) {
        if (!testSlow) {
            logger.warn("Slow test {} is suppressed", description.getTestClass().getName());
            return ignoringStatement;
        } else
            return stmt;
    }
}
