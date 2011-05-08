package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.*;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import java.util.Collection;
import java.util.HashSet;
import org.apache.log4j.*;


@ForCpp
public class EventSubsystem extends AbstractHasPlatform
{
    private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final Collection<EventSubscriber> subscribers = new HashSet<EventSubscriber>();
    private int reportEventNesting = 0;


    public EventSubsystem(Platform platform) {
        super(platform);
    }


    public void report(Event e) {
        reportEventNesting++;   // Kann Rekursiv aufgerufen werden.
        try {
            publishEvent(e);
        } catch (Exception x) { log().error(getClass().getSimpleName() + ".report(" + e.getClass().getName() + "): " + x); }
        finally { reportEventNesting--; }
    }

    
    private void publishEvent(Event e) {
        if (reportEventNesting > 1)  throw new SchedulerException("Recursive reportEvent() " + e);
        for (EventSubscriber s: subscribers)
            try { s.onEvent(e); }
            catch (Exception x) { log().error(s + ": " + x.toString()); }
    }


    public void subscribe(EventSubscriber x) { subscribers.add(x); }
    public void unsubscribe(EventSubscriber x) { subscribers.remove(x); }
    
    @Override public String toString() { return getClass().getSimpleName(); }
}
