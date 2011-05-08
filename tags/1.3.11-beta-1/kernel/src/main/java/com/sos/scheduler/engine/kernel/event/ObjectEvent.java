package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.NullSchedulerObject;
import com.sos.scheduler.engine.kernel.SchedulerObject;
import static com.sos.scheduler.engine.kernel.util.Util.*;


public class ObjectEvent<T extends SchedulerObject> extends AbstractEvent {
    private final T object;


    public ObjectEvent(T o) {
        this.object = o;
    }


    @Override public String toString() {
        StringBuilder result = new StringBuilder(200);
        result.append(super.toString());
        if (object != NullSchedulerObject.singleton)  result.append(" object=").append(stringOrException(object));
        return result.toString();
    }


    /** Das Objekt kann bereits weg sein, dann gibt es den Fehler Z-JAVA-111. */
    @Override public T getObject() {
        return object;
    }
}
