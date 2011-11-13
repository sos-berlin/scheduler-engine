package com.sos.scheduler.engine.kernelcpptest.schedulertest;

import org.junit.Test;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

/** Testet {@link com.sos.scheduler.engine.kernel.test.SchedulerTest} */
public final class FailingTerminatedEventHandlerTest extends SchedulerTest {
    @Test(expected=MyError.class) public void quickTest() {
        controller().startScheduler();
        controller().close();
    }

    @EventHandler public void handleEvent(TerminatedEvent e) {
        throw new MyError();
    }
}
