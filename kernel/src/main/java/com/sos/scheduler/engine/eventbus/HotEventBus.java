package com.sos.scheduler.engine.eventbus;

import java.util.Collection;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.event.EventSubsystem;

public class HotEventBus extends AbstractEventBus {
    private static final Logger logger = Logger.getLogger(HotEventBus.class);

    @Nullable private Event currentEvent = null;

    public HotEventBus() {
        super(HotEventHandler.class);
    }

    @Override public final void publish(Event e) {
        publish(e, calls(e));
    }

    final void publish(Event e, Collection<Call> calls) {
        if (currentEvent != null)
            handleRecursiveEvent(e);
        else
            dispatchNonrecursiveEvent(e, calls);
    }

    //@Override public final void dispatchEvents() {}

    private void handleRecursiveEvent(Event e) {
        try {
            // Kein log().error(), sonst gibt es wieder eine Rekursion
            throw new Exception(EventSubsystem.class.getSimpleName() + ".publish("+e+"): ignoring the event triggered by handling the event '"+currentEvent+"'");
        }
        catch (Exception x) {
            logger.error(x, x);
        }
    }

    private void dispatchNonrecursiveEvent(Event e, Collection<Call> calls) {
        currentEvent = e;
        try {
            for (Call c: calls)
                dispatchCall(c);
        }
        finally {
            currentEvent = null;
        }
    }
}
