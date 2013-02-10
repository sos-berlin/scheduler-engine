package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.event.AbstractEvent;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.inject.Inject;
import javax.inject.Singleton;

import static com.google.common.base.Preconditions.checkArgument;
import static com.sos.scheduler.engine.eventbus.HasUnmodifiableDelegates.tryUnmodifiableEventSource;

@ForCpp
@Singleton
public class EventSubsystem implements Subsystem {
    private static final Logger logger = LoggerFactory.getLogger(EventSubsystem.class);

    private final SchedulerEventBus eventBus;

    @Inject private EventSubsystem(SchedulerEventBus eventBus) {
        this.eventBus = eventBus;
    }

    /** @param e {@link AbstractEvent} statt {@link com.sos.scheduler.engine.data.event.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
    @ForCpp public final void report(AbstractEvent e) {
        eventBus.publish(e);
    }

    /** @param e {@link AbstractEvent} statt {@link com.sos.scheduler.engine.data.event.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig.
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
            eventBus.publish(e, tryUnmodifiableEventSource(e, o));
        } catch (Exception x) {
            logger.error("EventSubsystem.reportEventClass("+cppEventCode+"):", x);
        }
    }

    @Override public final String toString() {
        return getClass().getSimpleName();
    }
}
