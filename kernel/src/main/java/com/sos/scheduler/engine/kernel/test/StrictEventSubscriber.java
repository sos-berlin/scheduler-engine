package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.eventbus.EventSubscriber2;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.main.SchedulerController;

/** Beendet bei einer Exception oder einem ErrorLogEvent den Scheduler. */
public class StrictEventSubscriber implements EventSubscriber2 {
    private final EventSubscriber2 eventSubscriber;
    private final SchedulerController schedulerController;

    public StrictEventSubscriber(EventSubscriber2 s, SchedulerController controller) {
        eventSubscriber = s;
        schedulerController = controller;
    }

    @Override public Class<? extends Event> getEventClass() {
        return eventSubscriber.getEventClass();
    }

    @Override public final void onEvent(Event e) {
        try {
            eventSubscriber.onEvent(e);
        } catch (Throwable x) {
            schedulerController.terminateAfterException(x);
        }
    }

    @Override public String toString() {
        return StrictEventSubscriber.class.getSimpleName()+":"+eventSubscriber;
    }
}
