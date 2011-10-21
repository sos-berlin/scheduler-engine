package com.sos.scheduler.engine.kernel.test;

import org.junit.After;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller = TestSchedulerController.of(getClass().getPackage());

    @After public final void schedulerTestClose() {
        controller.close();
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    /** Zur Bequemlichkeit; dasselbe wie {@link com.sos.scheduler.engine.kernel.test.TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }
}
