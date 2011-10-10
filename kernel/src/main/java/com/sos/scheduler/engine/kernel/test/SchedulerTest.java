package com.sos.scheduler.engine.kernel.test;

import org.junit.After;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest {
    public static final Time shortTimeout = Time.of(10);

    private final TestSchedulerController controller = new TestSchedulerController(getClass().getPackage());

    @After public void schedulerTestClose() {
        controller.close();
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    /** Zur Bequemlichkeit; dasselbe wie {@link TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }
}
