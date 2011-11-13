package com.sos.scheduler.engine.kernelcpptest.schedulertest;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

/** Testet {@link com.sos.scheduler.engine.kernel.test.SchedulerTest} */
public final class FailingSchedulerReadyEventHandlerTest extends SchedulerTest {
    @Test(expected=MyError.class) public void quickTest() {
        controller().startScheduler();
        controller().terminateAndWait();
    }

    @EventHandler public void handleEvent(SchedulerReadyEvent e) {
        throw new MyError();
    }
}
