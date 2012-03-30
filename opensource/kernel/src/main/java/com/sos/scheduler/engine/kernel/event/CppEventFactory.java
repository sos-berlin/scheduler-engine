package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedRemovedEvent;
import com.sos.scheduler.engine.kernel.job.Task;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.job.events.TaskStartedEvent;
import com.sos.scheduler.engine.kernel.order.*;

class CppEventFactory {
    static Event newInstance(CppEventCode cppEventCode, EventSource o) {
        switch (cppEventCode) {
            case fileBasedActivatedEvent:
                return new FileBasedActivatedEvent();

            case fileBasedRemovedEvent:
                return new FileBasedRemovedEvent();

            case taskStartedEvent: {
                Task task = (Task)o;
                return new TaskStartedEvent(task.getId(), task.getJob().getPath());
            }

            case taskEndedEvent: {
                Task task = (Task)o;
                return new TaskEndedEvent(task.getId(), task.getJob().getPath());
            }

            case orderTouchedEvent:
                return new OrderTouchedEvent(((Order)o).getKey());

            case orderFinishedEvent:
                return new OrderFinishedEvent(((Order)o).getKey());

            case orderSuspendedEvent:
                return new OrderSuspendedEvent(((Order)o).getKey());

            case orderResumedEvent:
                return new OrderResumedEvent(((Order)o).getKey());

            case orderStepStartedEvent:
                return new OrderStepStartedEvent(((Order)o).getKey());
        }
        throw new RuntimeException("Not implemented cppEventCode="+cppEventCode);
    }

    private CppEventFactory() {}
}
