package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
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
            if (e instanceof ErrorLogEvent) throw new AssertionError(((ErrorLogEvent)e).getMessage().toString());
            eventSubscriber.onEvent(e);
        }
        catch (Exception x) {
            schedulerController.terminateAfterException(x);
        }
        catch (Error x) {
            //logger.error(x);
            schedulerController.terminateAfterException(x);
        }
    }
}
