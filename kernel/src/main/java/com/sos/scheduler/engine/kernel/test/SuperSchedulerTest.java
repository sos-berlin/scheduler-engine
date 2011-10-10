package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.util.Time;

/** Ersetzt durch {@link SchedulerTest}, das mit {@link SchedulerTest#controller()} alle Funktionen bereitstellt. */
@Deprecated
public abstract class SuperSchedulerTest extends SchedulerTest {
    protected SuperSchedulerTest() {}

    protected final void strictSubscribeEvents() {
        controller().strictSubscribeEvents();
    }

    protected final void strictSubscribeEvents(EventSubscriber s) {
        controller().strictSubscribeEvents(s);
    }

    protected final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    protected final void startScheduler(String... args) {
        controller().startScheduler(args);
    }

    protected final void waitUntilSchedulerIsRunning() {
        controller().waitUntilSchedulerIsRunning();
    }

    protected final void waitForTermination(Time timeout) {
        controller().waitForTermination(timeout);
    }
}
