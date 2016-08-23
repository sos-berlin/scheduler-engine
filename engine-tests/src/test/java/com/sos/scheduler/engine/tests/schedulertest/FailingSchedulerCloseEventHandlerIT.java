package com.sos.scheduler.engine.tests.schedulertest;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.main.event.SchedulerClosed$;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

/** Testet {@link com.sos.scheduler.engine.test.SchedulerTest} */
public final class FailingSchedulerCloseEventHandlerIT extends SchedulerTest {
    @Test(expected=TestError.class) public void activateTest() {
        controller().activateScheduler();
        controller().close();
    }

    @HotEventHandler public void handleEvent(KeyedEvent<Event> e) {
        if (e.event() == SchedulerClosed$.MODULE$)
            throw new TestError();
    }
}
