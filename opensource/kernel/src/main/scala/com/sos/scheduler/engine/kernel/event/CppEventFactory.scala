package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.AbstractEvent
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.folder.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.folder.FileBasedRemovedEvent
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.data.log.LogEvent
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.event.CppEventCode._
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.kernel.order.Order

@ForCpp object CppEventFactory {

  private[event] def newInstance(cppEventCode: CppEventCode, eventSource: EventSource): Event = {
    cppEventCode match {
      case `fileBasedActivatedEvent` =>
        new FileBasedActivatedEvent(eventSource.asInstanceOf[FileBased].getTypedPath)

      case `fileBasedRemovedEvent` =>
        new FileBasedRemovedEvent(eventSource.asInstanceOf[FileBased].getTypedPath)

      case `taskStartedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskStartedEvent(task.id, task.job.getPath)

      case `taskEndedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskEndedEvent(task.id, task.job.getPath)

      case `taskClosedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskClosedEvent(task.id, task.job.getPath)

      case `orderTouchedEvent` =>
        new OrderTouchedEvent(eventSource.asInstanceOf[Order].getKey)

      case `orderFinishedEvent` =>
        new OrderFinishedEvent(eventSource.asInstanceOf[Order].getKey)

      case `orderSuspendedEvent` =>
        new OrderSuspendedEvent(eventSource.asInstanceOf[Order].getKey)

      case `orderResumedEvent` =>
        new OrderResumedEvent(eventSource.asInstanceOf[Order].getKey)

      case `orderSetBackEvent` =>
        val order = eventSource.asInstanceOf[Order]
        new OrderSetBackEvent(order.getKey, order.getState)

      case `orderStepStartedEvent` =>
        val order: Order = eventSource.asInstanceOf[Order]
        new OrderStepStartedEvent(order.getKey, order.getState)

      case o =>
        sys.error(s"Not implemented cppEventCode=$o")
    }
  }

  @ForCpp def newLogEvent(cppLevel: Int, message: String): AbstractEvent =
    LogEvent.of(SchedulerLogLevel.ofCpp(cppLevel), message)

  @ForCpp def newOrderStateChangedEvent(jobChainPath: String, orderId: String, previousState: String): AbstractEvent =
    new OrderStateChangedEvent(OrderKey(jobChainPath, orderId), new OrderState(previousState))

  @ForCpp def newOrderStepEndedEvent(jobChainPath: String, orderId: String, orderStateTransitionCpp: Int): AbstractEvent =
    new OrderStepEndedEvent(OrderKey(jobChainPath, orderId), OrderStateTransition.ofCppCode(orderStateTransitionCpp))
}
