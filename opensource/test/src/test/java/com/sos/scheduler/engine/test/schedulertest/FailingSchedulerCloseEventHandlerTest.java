package com.sos.scheduler.engine.test.schedulertest;

import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.scheduler.events.SchedulerCloseEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

/** Testet {@link com.sos.scheduler.engine.test.SchedulerTest} */
public final class FailingSchedulerCloseEventHandlerTest extends SchedulerTest {
    @Test(expected=MyError.class) public void test() {
        controller().startScheduler();
        controller().close();
    }

    @HotEventHandler public void handleEvent(SchedulerCloseEvent e) {
        throw new MyError();
    }
}
