package com.sos.scheduler.engine.test.schedulertest;

import static org.hamcrest.Matchers.contains;
import static org.junit.Assert.assertThat;

import org.junit.Test;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.main.event.TerminatedEvent;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

/** Testet {@link SchedulerTest} */
public final class SchedulerTestTest extends SchedulerTest {
    private final ImmutableList.Builder<Class<? extends Event>> receivedEventClasses = ImmutableList.builder();

    @Test public void test() {
        controller().startScheduler();
        controller().close();
        Class<?>[] expected = {SchedulerReadyEvent.class, SchedulerCloseEvent.class, TerminatedEvent.class};
        assertThat(receivedEventClasses.build(), contains(expected));
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
