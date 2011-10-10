package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.NullSchedulerObject;
import com.sos.scheduler.engine.kernel.SchedulerObject;
import static com.sos.scheduler.engine.kernel.util.Util.*;

abstract public class ObjectEvent extends AbstractEvent {
    /** Nur gültig während des Events. 
     * Danach kann das Objekt ungültig geworden sein und es gibt den Fehler Z-JAVA-111. */
    abstract protected SchedulerObject getObject();

    @Override public String toString() {
        StringBuilder result = new StringBuilder(200);
        result.append(super.toString());
        SchedulerObject o = getObject();
        if (o != NullSchedulerObject.singleton)  result.append(" object=").append(stringOrException(o));
        return result.toString();
    }
}
