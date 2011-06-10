package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.event.SchedulerReadyEvent;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;


/** Beendet bei einer Exception oder einem ErrorLogEvent den Scheduler. */
public class StrictEventSubscriber implements EventSubscriber {
    private SchedulerController schedulerController = null;
    private final EventSubscriber eventSubscriber;


    public StrictEventSubscriber(EventSubscriber s) {
        eventSubscriber = s;
    }

    
    @Override public final void onEvent(Event e) {
        try {
            if (e instanceof SchedulerReadyEvent)     // The very first event
                schedulerController = ((SchedulerReadyEvent)e).getSchedulerController();
            else {
                assert schedulerController != null;
                if (e instanceof ErrorLogEvent) throw new AssertionError(((ErrorLogEvent)e).getMessage().toString());
            }
            
            eventSubscriber.onEvent(e);
        }
        catch (Exception x) {
            if (schedulerController != null)  schedulerController.terminateAfterException(x);
        }
        catch (Error x) {
            //logger.error(x);
            if (schedulerController != null)  schedulerController.terminateAfterException(x);
        }
    }
}
