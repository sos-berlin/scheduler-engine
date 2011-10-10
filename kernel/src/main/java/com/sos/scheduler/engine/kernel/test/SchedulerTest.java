package com.sos.scheduler.engine.kernel.test;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest {
    public static final Time shortTimeout = Time.of(10);

    @Rule public final TemporaryFolder folder = new TemporaryFolder();
    private TestSchedulerController controller = null;

    protected SchedulerTest() {}

    @Before public final void schedulerTestBefore() {
        controller = TestSchedulerController.of(getClass().getPackage(), folder);
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        controller.terminateAndCleanUp();
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    /** Zur Bequemlichkeit; dasselbe wie {@link TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }
}
