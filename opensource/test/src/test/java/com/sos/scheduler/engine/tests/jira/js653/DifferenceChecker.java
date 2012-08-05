package com.sos.scheduler.engine.tests.jira.js653;

import static org.junit.Assert.fail;

import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Joiner;
import com.google.common.collect.Sets;

class DifferenceChecker<T> {
    private static final Joiner commaJoiner = Joiner.on(", ");

    private final Set<T> expected;
    private final Set<T> actual;
    private final String adjective;

    private DifferenceChecker(Set<T> expected, Set<T> actual, String adjective) {
        this.expected = expected;
        this.actual = actual;
        this.adjective = adjective;
    }

    final void check() {
        if (!actual.equals(expected))
            fail(differenceMessage());
    }

    final String differenceMessage() {
        return Joiner.on("; ").skipNulls().join(wrongActualString(), missingActualString());
    }

    @Nullable private String wrongActualString() {
        Set<T> wrongActual = Sets.difference(actual, expected);
        return wrongActual.isEmpty()? null : "Unexpectedly "+adjective+" orders: "+ commaJoiner.join(wrongActual);
    }

    @Nullable private String missingActualString() {
        Set<T> missingActual = Sets.difference(expected, actual);
        return missingActual.isEmpty()? null : "Not "+adjective+" orders: "+ commaJoiner.join(missingActual);
    }

    static <T> void assertNoDifference(Set<T> expected, Set<T> actual, String adjective) {
        new DifferenceChecker<T>(expected, actual, adjective).check();
    }
}
