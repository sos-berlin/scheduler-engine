package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.*;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import java.util.Collection;
import java.util.HashSet;
import org.apache.log4j.*;


@ForCpp
public class EventSubsystem extends AbstractHasPlatform implements Subsystem {
    private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final Collection<EventSubscriber> subscribers = new HashSet<EventSubscriber>();
    private int reportEventNesting = 0;


    public EventSubsystem(Platform platform) {
        super(platform);
    }


    public final void report(Event e) {
        reportEventNesting++;   // Kann Rekursiv aufgerufen werden.
        try {
            publishEvent(e);
        } catch(RecursiveEventException x) {
            // Kein log().error(), sonst gibt es wieder eine Rekursion
            logger.error(getClass().getSimpleName() + ".report(" + e.getClass().getName() + "): " + x);
        }
        finally { reportEventNesting--; }
    }

    
    private void publishEvent(Event e) {
        if (reportEventNesting > 1)  throw new RecursiveEventException(e);  // Das sollte natürlich nicht vorkommen ...
        for (EventSubscriber s: subscribers)
            try { s.onEvent(e); }
            catch (Exception x) { log().error(s + ": " + x); }
    }


    public final void subscribe(EventSubscriber x) {
        subscribers.add(x);
    }

    
    public final void unsubscribe(EventSubscriber x) { 
        subscribers.remove(x);
    }


    @Override public final String toString() {
        return getClass().getSimpleName();
    }


    private static final class RecursiveEventException extends SchedulerException {
        private RecursiveEventException(Event e) {
            super("Recursive publishing of events: " + e);
        }
    }
}
