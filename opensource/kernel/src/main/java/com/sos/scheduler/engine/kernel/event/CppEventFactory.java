package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.event.AbstractEvent;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.folder.FileBasedActivatedEvent;
import com.sos.scheduler.engine.data.folder.FileBasedRemovedEvent;
import com.sos.scheduler.engine.data.job.TaskClosedEvent;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.data.log.LogEvent;
import com.sos.scheduler.engine.data.log.SchedulerLogLevel;
import com.sos.scheduler.engine.data.order.*;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.job.Task;
import com.sos.scheduler.engine.kernel.order.Order;

@ForCpp
public final class CppEventFactory {
    
    static Event newInstance(CppEventCode cppEventCode, EventSource o) {
        switch (cppEventCode) {
            case fileBasedActivatedEvent:
                return new FileBasedActivatedEvent(((FileBased)o).getTypedPath());

            case fileBasedRemovedEvent:
                return new FileBasedRemovedEvent(((FileBased)o).getTypedPath());

            case taskStartedEvent: {
                Task task = (Task)o;
                return new TaskStartedEvent(task.id(), task.job().getPath());
            }

            case taskEndedEvent: {
                Task task = (Task)o;
                return new TaskEndedEvent(task.id(), task.job().getPath());
            }

            case taskClosedEvent: {
                Task task = (Task)o;
                return new TaskClosedEvent(task.id(), task.job().getPath());
            }

            case orderTouchedEvent:
                return new OrderTouchedEvent(((Order)o).getKey());

            case orderFinishedEvent:
                return new OrderFinishedEvent(((Order)o).getKey());

            case orderSuspendedEvent:
                return new OrderSuspendedEvent(((Order)o).getKey());

            case orderResumedEvent:
                return new OrderResumedEvent(((Order)o).getKey());

            case orderStepStartedEvent: {
                Order order = (Order)o;
                return new OrderStepStartedEvent(order.getKey(), order.getState());
            }
        }
        throw new RuntimeException("Not implemented cppEventCode="+cppEventCode);
    }

    @ForCpp private static AbstractEvent newLogEvent(int cppLevel, String message) {
        return LogEvent.of(SchedulerLogLevel.ofCpp(cppLevel), message);
    }

    @ForCpp private static AbstractEvent newOrderStateChangedEvent(String jobChainPath, String orderId, String previousState) {
        return new OrderStateChangedEvent(OrderKey.of(jobChainPath, orderId), new OrderState(previousState));
    }

    @ForCpp private static AbstractEvent newOrderStepEndedEvent(String jobChainPath, String orderId, int orderStateTransitionCpp) {
        return new OrderStepEndedEvent(OrderKey.of(jobChainPath, orderId), OrderStateTransition.ofCppCode(orderStateTransitionCpp));
    }

    private CppEventFactory() {}
}
