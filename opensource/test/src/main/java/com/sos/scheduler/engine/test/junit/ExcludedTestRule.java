package com.sos.scheduler.engine.test.junit;

import static com.sos.scheduler.engine.test.junit.Statements.ignoringStatement;
import static com.sos.scheduler.engine.kernel.util.Util.booleanOf;

import org.apache.log4j.Logger;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;

/** JUnit-{@link TestRule}, die einen Test als ausgeschlossen markiert und ihn nur ausführen lässt,
 * wenn die Property testExcluded gesetzt ist.
 *
 * Wird nicht benutzt und kann vielleicht raus.
 */
public final class ExcludedTestRule implements TestRule {
    private static final Logger log = Logger.getLogger(ExcludedTestRule.class);
    private static final boolean testExcluded = booleanOf(System.getProperty("testExcluded"), false, true);
    public static final ExcludedTestRule singleton = new ExcludedTestRule();

    private ExcludedTestRule() {}

    @Override public Statement apply(Statement stmt, Description description) {
        Class<?> c = description.getTestClass();
        if (isExcluded(c)) {
            log.warn("Test "+c.getName()+" is excluded");
            return ignoringStatement;
        } else
            return stmt;
    }

    private static boolean isExcluded(Class<?> testClass) {
        return !testExcluded || packagePathContains(testClass.getPackage(), "excluded");
    }

    private static boolean packagePathContains(Package p, String name) {
        return ImmutableList.copyOf(Splitter.on('.').split(p.getName())).contains(name);
    }
}
