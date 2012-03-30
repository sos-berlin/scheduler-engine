package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.AbstractEvent;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;

@ForCpp
public class EventSubsystem extends AbstractHasPlatform implements Subsystem {
    //private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final SchedulerEventBus eventBus;

    public EventSubsystem(Platform platform, SchedulerEventBus eventBus) {
        super(platform);
        this.eventBus = eventBus;
    }

    /** @param e {@link AbstractEvent} statt {@link com.sos.scheduler.engine.eventbus.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
    @ForCpp public final void report(AbstractEvent e) {
        if (e instanceof ObjectEvent)
            report(e, ((ObjectEvent)e).getObject());
        else
            eventBus.publish(e);
    }

    /** @param e {@link AbstractEvent} statt {@link com.sos.scheduler.engine.eventbus.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig.
     * @param eventSource {@link Object} statt {@liink EventSource}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
    @ForCpp public final void report(AbstractEvent e, Object eventSource) {
        EventSource o = (EventSource)eventSource;
        eventBus.publish(e, o);
    }

    @Override public final String toString() {
        return getClass().getSimpleName();
    }
}
