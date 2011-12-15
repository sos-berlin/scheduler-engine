package com.sos.scheduler.engine.test;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.util.Time;
import org.junit.After;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller = TestSchedulerController.of(getClass());

    protected SchedulerTest() {
        controller.getEventBus().registerAnnotated(this);
    }

    @After public final void schedulerTestClose() {
        controller.getEventBus().unregisterAnnotated(this);
        controller.close();
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    /** Zur Bequemlichkeit; dasselbe wie {@link com.sos.scheduler.engine.test.TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }
}
