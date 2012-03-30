package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.AbstractEvent;
import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import org.apache.log4j.Logger;

import static com.google.common.base.Preconditions.checkArgument;

@ForCpp
public class EventSubsystem extends AbstractHasPlatform implements Subsystem {
    private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final SchedulerEventBus eventBus;

    public EventSubsystem(Platform platform, SchedulerEventBus eventBus) {
        super(platform);
        this.eventBus = eventBus;
    }

    /** @param e {@link AbstractEvent} statt {@link com.sos.scheduler.engine.eventbus.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
    @ForCpp public final void report(AbstractEvent e) {
        eventBus.publish(e);
    }

    /** @param e {@link AbstractEvent} statt {@link com.sos.scheduler.engine.eventbus.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig.
     * @param eventSource {@link Object} statt {@link EventSource}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
    @ForCpp public final void report(AbstractEvent e, Object eventSource) {
        EventSource o = (EventSource)eventSource;
        eventBus.publish(e, o);
    }

    @ForCpp public final void checkNumberOfEventCodes(int count) {
        checkArgument(count == CppEventCode.values().length, "C++-Event_code does not match CppEventCode");
    }

    @ForCpp public final void reportEventClass(int cppEventCode, Object eventSource) {
        try {
            EventSource o = (EventSource)eventSource;
            Event e = CppEventFactory.newInstance(CppEventCode.values()[cppEventCode], o);
            eventBus.publish(e, o);
        } catch (Exception x) {
            logger.error("EventSubsystem.reportEventClass("+cppEventCode+"): "+x, x);
        }
    }

    @Override public final String toString() {
        return getClass().getSimpleName();
    }
}
