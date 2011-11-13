package com.sos.scheduler.engine.kernelcpptest.schedulertest;

import static com.google.common.collect.Lists.newArrayList;
import static org.hamcrest.Matchers.contains;
import static org.junit.Assert.assertThat;

import java.util.List;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

/** Testet {@link SchedulerTest} */
public final class SchedulerTestTest extends SchedulerTest {
    private final List<Class<? extends Event>> receivedEventClasses = newArrayList();

    @Test public void test() {
        controller().startScheduler();
        controller().close();
        Class<?>[] expected = {SchedulerReadyEvent.class, SchedulerCloseEvent.class, TerminatedEvent.class};
        assertThat(receivedEventClasses, contains(expected));
    }

    @EventHandler public void handleEvent(SchedulerReadyEvent e) {
        receivedEventClasses.add(SchedulerReadyEvent.class);
    }

    @EventHandler public void handleEvent(SchedulerCloseEvent e) {
        receivedEventClasses.add(SchedulerCloseEvent.class);
    }

    @EventHandler public void handleEvent(TerminatedEvent e) {
        receivedEventClasses.add(TerminatedEvent.class);
    }
}
