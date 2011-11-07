package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;

/** Beendet bei einer Exception oder einem ErrorLogEvent den Scheduler. */
public class StrictEventSubscriber implements EventSubscriber {
    private final EventSubscriber eventSubscriber;
    private final SchedulerController schedulerController;

    public StrictEventSubscriber(EventSubscriber s, SchedulerController controller) {
        eventSubscriber = s;
        schedulerController = controller;
    }

    @Override public final void onEvent(Event e) {
        try {
            eventSubscriber.onEvent(e);
        } catch (Throwable x) {
            schedulerController.terminateAfterException(x);
        }
    }
}
