package com.sos.scheduler.engine.kernel.test;

import org.junit.After;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller;

    protected SchedulerTest() {
        this(DefaultSettings.singleton);
    }

    protected SchedulerTest(Settings settings) {
        controller = TestSchedulerController.of(getClass().getPackage(), settings);
        controller.subscribeForAnnotatedEventHandlers(this);
        controller.setTerminateOnError(true);
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
