package com.sos.scheduler.engine.tests.schedulertest;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

/** Testet {@link com.sos.scheduler.engine.test.SchedulerTest} */
public final class FailingTerminatedEventHandlerIT extends SchedulerTest {
    @Test(expected=TestError.class) public void activateTest() {
        controller().activateScheduler();
        controller().close();
    }

    @Test(expected=TestError.class) public void startTest() {
        controller().activateScheduler();
        controller().close();
    }

    @HotEventHandler public void handleEvent(KeyedEvent<Event> e) {
        if (TerminatedEvent.class.isAssignableFrom(e.event().getClass()))
            throw new TestError();
    }
}
