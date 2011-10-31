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
    private Event currentEvent = null;

    public EventSubsystem(Platform platform) {
        super(platform);
    }

    @ForCpp public final void report(Event e) {
        if (currentEvent != null) {
            onRecursiveEvent(e);
        } else {
            if (e instanceof SchedulerIsCallableEvent)
                publishEvent(e);
            else
                publishNonrecursiveEvent(e);
        }
    }

    private void onRecursiveEvent(Event e) {
        try {
            // Kein log().error(), sonst gibt es wieder eine Rekursion
            throw new Exception(EventSubsystem.class.getSimpleName() + ".report(" +e+ "): ignoring the event triggered by handling the event '"+currentEvent+"'");
        }
        catch (Exception x) {
            logger.error(x, x);
        }
    }

    private void publishNonrecursiveEvent(Event e) {
        currentEvent = e;
        try {
            publishEvent(e);
        }
        finally {
            currentEvent = null;
        }
    }

    private void publishEvent(Event e) {
        logger.trace(e);
        for (EventSubscriber s: subscribers) publishEvent(e, s);
    }

    private void publishEvent(Event e, EventSubscriber s) {
        try {
            s.onEvent(e);
        }
        catch (Exception x) {
            logger.error(s+": "+x, x);
            log().error(s+": "+x);
        }
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
}
