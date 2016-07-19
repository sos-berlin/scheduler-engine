package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.{AbstractEvent, Event}
import com.sos.scheduler.engine.data.filebased.{FileBasedActivatedEvent, FileBasedAddedEvent, FileBasedRemovedEvent, FileBasedReplacedEvent}
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode, TaskClosedEvent, TaskEndedEvent, TaskId, TaskStartedEvent}
import com.sos.scheduler.engine.data.log.{LogEvent, SchedulerLogLevel}
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
        new OrderTouchedEvent(eventSource.asInstanceOf[Order].orderKey)

      case `orderFinishedEvent` =>
        val order: Order = eventSource.asInstanceOf[Order]
        new OrderFinishedEvent(eventSource.asInstanceOf[Order].orderKey, order.state)

      case `orderNestedTouchedEvent` =>
        new OrderNestedTouchedEvent(eventSource.asInstanceOf[Order].orderKey)

      case `orderNestedFinishedEvent` =>
        new OrderNestedFinishedEvent(eventSource.asInstanceOf[Order].orderKey)

      case `orderSuspendedEvent` =>
        new OrderSuspendedEvent(eventSource.asInstanceOf[Order].orderKey)

      case `orderResumedEvent` =>
        new OrderResumedEvent(eventSource.asInstanceOf[Order].orderKey)

      case `orderSetBackEvent` =>
        val order = eventSource.asInstanceOf[Order]
        new OrderSetBackEvent(order.orderKey, order.state)

      case `orderStepStartedEvent` =>
        val order: Order = eventSource.asInstanceOf[Order]
        new OrderStepStartedEvent(order.orderKey, order.state, order.taskId getOrElse TaskId.Null)

      case o =>
        sys.error(s"Not implemented cppEventCode=$o")
    }
  }

  @ForCpp def newLogEvent(cppLevel: Int, message: String): AbstractEvent =
    LogEvent.of(SchedulerLogLevel.ofCpp(cppLevel), message)

  @ForCpp def newOrderStateChangedEvent(jobChainPath: String, orderId: String, previousState: String, state: String): AbstractEvent =
    OrderStateChangedEvent(OrderKey(jobChainPath, orderId), previousState = OrderState(previousState), state = OrderState(state))

  @ForCpp def newOrderStepEndedEvent(jobChainPath: String, orderId: String, orderStateTransitionCpp: Long): AbstractEvent =
    OrderStepEndedEvent(OrderKey(jobChainPath, orderId), OrderStateTransition.ofCppInternalValue(orderStateTransitionCpp))

  @ForCpp def newTaskEndedEvent(taskId: Int, jobPath: String, returnCode: Int): AbstractEvent =
    TaskEndedEvent(TaskId(taskId), JobPath(jobPath), ReturnCode(returnCode))
}
