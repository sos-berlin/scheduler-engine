package com.sos.scheduler.engine.test;

import org.junit.After;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller = TestSchedulerController.of(getClass().getPackage());

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

    protected Settings temporaryDatabaseSettings() {
        return new TemporaryDatabaseSettings(controller.environment().directory());
    }
}
