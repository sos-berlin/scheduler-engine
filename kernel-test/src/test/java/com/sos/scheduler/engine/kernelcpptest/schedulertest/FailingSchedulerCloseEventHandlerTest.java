package com.sos.scheduler.engine.kernelcpptest.schedulertest;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

/** Testet {@link com.sos.scheduler.engine.kernel.test.SchedulerTest} */
public final class FailingSchedulerCloseEventHandlerTest extends SchedulerTest {
    @Test(expected=MyError.class) public void test() {
        controller().startScheduler();
        controller().terminateAndWait();
    }

    @EventHandler public void handleEvent(SchedulerCloseEvent e) {
        throw new MyError();
    }
}
