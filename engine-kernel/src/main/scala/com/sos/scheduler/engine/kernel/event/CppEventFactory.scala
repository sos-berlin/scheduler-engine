package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.AbstractEvent
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.filebased.{FileBasedReplacedEvent, FileBasedRemovedEvent, FileBasedAddedEvent, FileBasedActivatedEvent}
import com.sos.scheduler.engine.data.job.{TaskId, JobPath, ResultCode, TaskClosedEvent, TaskEndedEvent, TaskStartedEvent}
import com.sos.scheduler.engine.data.log.LogEvent
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.event.CppEventCode._
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.kernel.order.Order

@ForCpp object CppEventFactory {

  private[event] def newInstance(cppEventCode: CppEventCode, eventSource: EventSource): Event = {
    cppEventCode match {
      case `fileBasedActivatedEvent` =>
        new FileBasedActivatedEvent(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedAddedEvent` =>
        new FileBasedAddedEvent(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedRemovedEvent` =>
        new FileBasedRemovedEvent(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedReplacedEvent` =>
        new FileBasedReplacedEvent(eventSource.asInstanceOf[FileBased].path)

      case `taskStartedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskStartedEvent(task.id, task.job.path)

      case `taskClosedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskClosedEvent(task.id, task.job.path)

      case `orderTouchedEvent` =>
        new OrderTouchedEvent(eventSource.asInstanceOf[Order].key)

      case `orderFinishedEvent` =>
        new OrderFinishedEvent(eventSource.asInstanceOf[Order].key)

      case `orderNestedTouchedEvent` =>
        new OrderNestedTouchedEvent(eventSource.asInstanceOf[Order].key)

      case `orderNestedFinishedEvent` =>
        new OrderNestedFinishedEvent(eventSource.asInstanceOf[Order].key)

      case `orderSuspendedEvent` =>
        new OrderSuspendedEvent(eventSource.asInstanceOf[Order].key)

      case `orderResumedEvent` =>
        new OrderResumedEvent(eventSource.asInstanceOf[Order].key)

      case `orderSetBackEvent` =>
        val order = eventSource.asInstanceOf[Order]
        new OrderSetBackEvent(order.key, order.state)

      case `orderStepStartedEvent` =>
        val order: Order = eventSource.asInstanceOf[Order]
        new OrderStepStartedEvent(order.key, order.state)

      case o =>
        sys.error(s"Not implemented cppEventCode=$o")
    }
  }

  @ForCpp def newLogEvent(cppLevel: Int, message: String): AbstractEvent =
    LogEvent.of(SchedulerLogLevel.ofCpp(cppLevel), message)

  @ForCpp def newOrderStateChangedEvent(jobChainPath: String, orderId: String, previousState: String, state: String): AbstractEvent =
    OrderStateChangedEvent(OrderKey(jobChainPath, orderId), previousState = OrderState(previousState), state = OrderState(state))

  @ForCpp def newOrderStepEndedEvent(jobChainPath: String, orderId: String, orderStateTransitionCpp: Int): AbstractEvent =
    OrderStepEndedEvent(OrderKey(jobChainPath, orderId), OrderStateTransition.ofCppCode(orderStateTransitionCpp))

  @ForCpp def newTaskEndedEvent(taskId: Int, jobPath: String, resultCode: Int): AbstractEvent =
    TaskEndedEvent(TaskId(taskId), JobPath(jobPath), ResultCode(resultCode))
}
