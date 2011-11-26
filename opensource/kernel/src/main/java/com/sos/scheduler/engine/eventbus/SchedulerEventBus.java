package com.sos.scheduler.engine.eventbus;

import java.util.Collection;

public class SchedulerEventBus implements EventBus {
    private final HotEventBus hotEventBus = new HotEventBus();
    private final ColdEventBus coldEventBus = new ColdEventBus();

    @Override public final void registerAnnotated(EventHandlerAnnotated o) {
        coldEventBus.registerAnnotated(o);
        hotEventBus.registerAnnotated(o);
    }

    @Override public final void unregisterAnnotated(EventHandlerAnnotated o) {
        hotEventBus.unregisterAnnotated(o);
        coldEventBus.unregisterAnnotated(o);
    }

    public final void registerHot(EventSubscription s) {
        hotEventBus.register(s);
    }

    public final void unregisterHot(EventSubscription s) {
        hotEventBus.unregister(s);
    }

    public final void publish(Event e) {
        checkDuplicateCalls(e);
        //coldCalls.removeAll(hotCalls);
        //if (e instanceof HotEvent)
            hotEventBus.publish(e);
        coldEventBus.publish(e);
    }

    private void checkDuplicateCalls(Event e) {
        Collection<Call> hotCalls = hotEventBus.calls(e);
        Collection<Call> coldCalls = coldEventBus.calls(e);
        if (!hotCalls.isEmpty() && !coldCalls.isEmpty()) {
            int a = 1;
            a++;
        }
    }

    public final void dispatchEvents() {
        //hotEventBus.dispatchEvents();
        coldEventBus.dispatchEvents();
    }

    public final HotEventBus getHotEventBus() {
        return hotEventBus;
    }

    public final ColdEventBus getColdEventBus() {
        return coldEventBus;
    }
}
