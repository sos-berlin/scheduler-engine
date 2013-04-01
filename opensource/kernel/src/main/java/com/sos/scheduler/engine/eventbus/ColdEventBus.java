package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.eventbus.annotated.ColdMethodEventSubscriptionFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Collection;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import static com.google.common.base.Throwables.propagate;

public class ColdEventBus extends AbstractEventBus implements Runnable {
    private static final Logger logger = LoggerFactory.getLogger(ColdEventBus.class);

    private final LinkedBlockingQueue<Call> callQueue = new LinkedBlockingQueue<Call>();

    public ColdEventBus() {
        super(ColdMethodEventSubscriptionFactory.singleton);
    }

    @Override public final void publish(Event e) {
        publish(calls(e));
    }

    final void publish(Collection<Call> c) {
        callQueue.addAll(c);
    }

    public final void run() {
        try {
            dispatchEvents(true);
        } catch (InterruptedException ignored) {}
    }

    public final void dispatchEvents() {
        try {
            dispatchEvents(false);
        } catch (InterruptedException e) { throw propagate(e); }    // Kommt nicht vor
    }

    private void dispatchEvents(boolean wait) throws InterruptedException {
        while (true) {
            Call call = wait? callQueue.poll(Long.MAX_VALUE, TimeUnit.DAYS) : callQueue.poll();
            if (call == null) break;
            logger.trace("dispatch "+call);
            dispatchCall(call);
        }
    }
}
