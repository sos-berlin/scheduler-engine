package com.sos.scheduler.engine.test;

import org.apache.log4j.Logger;
import org.junit.After;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    private static final Logger logger = Logger.getLogger(SchedulerTest.class);
    
//    public static final Time shortTimeout = TestSchedulerController.shortTimeout;
//  shortTimeout aus TestSchedulerController (10 sec) reicht für einige Test nicht
    public static final Time shortTimeout = Time.of(15);

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
