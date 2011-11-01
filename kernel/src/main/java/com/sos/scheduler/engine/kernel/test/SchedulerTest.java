package com.sos.scheduler.engine.kernel.test;

import org.junit.After;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller = TestSchedulerController.of(getClass().getPackage());

    protected SchedulerTest() {
        controller.terminateOnErrorEvent();
        controller.subscribeForAnnotatedEventHandlers(this);
    }

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
