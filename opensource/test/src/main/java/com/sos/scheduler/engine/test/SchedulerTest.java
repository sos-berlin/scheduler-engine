package com.sos.scheduler.engine.test;

import org.apache.log4j.Logger;
import org.junit.After;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    private static final Logger logger = Logger.getLogger(SchedulerTest.class);
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;
    private static final Time terminationTimeout = shortTimeout;

    private final TestSchedulerController controller = TestSchedulerController.of(getClass().getPackage());

    protected SchedulerTest() {
        controller.getEventBus().registerAnnotated(this);
    }

    @After public final void schedulerTestClose() {
        boolean terminated = controller.tryWaitForTermination(terminationTimeout);
        if  (!terminated)  logger.error("Test "+ getClass().getSimpleName() +" had not terminated the JobScheduler");
        close();

    }

    private void close() {
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
