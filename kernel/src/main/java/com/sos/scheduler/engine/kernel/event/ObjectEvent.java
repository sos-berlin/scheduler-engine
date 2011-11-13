package com.sos.scheduler.engine.kernel.event;

import static com.sos.scheduler.engine.kernel.util.Util.stringOrException;

import com.sos.scheduler.engine.eventbus.AbstractEvent;
import com.sos.scheduler.engine.kernel.SchedulerObject;

public abstract class ObjectEvent extends AbstractEvent {
    /** Nur gültig während des Events. 
     * Danach kann das Objekt ungültig geworden sein und es gibt den Fehler Z-JAVA-111. */
    protected abstract SchedulerObject getObject();

    @Override public String toString() {
        StringBuilder result = new StringBuilder(200);
        result.append(super.toString());
        SchedulerObject o = getObject();
        result.append(" object=").append(stringOrException(o));
        return result.toString();
    }
}
