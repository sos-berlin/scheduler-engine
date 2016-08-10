package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.{AbstractEvent, Event}
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedAdded, FileBasedRemoved, FileBasedReplaced}
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode, TaskClosed, TaskEnded, TaskId, TaskStarted}
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
        new FileBasedActivated(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedAddedEvent` =>
        new FileBasedAdded(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedRemovedEvent` =>
        new FileBasedRemoved(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedReplacedEvent` =>
        new FileBasedReplaced(eventSource.asInstanceOf[FileBased].path)

      case `taskStartedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskStarted(task.id, task.job.path)

      case `taskClosedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        new TaskClosed(task.id, task.job.path)

      case `orderTouchedEvent` =>
        new OrderStarted(eventSource.asInstanceOf[Order].orderKey)

      case `orderFinishedEvent` =>
        val order: Order = eventSource.asInstanceOf[Order]
        new OrderFinished(eventSource.asInstanceOf[Order].orderKey, order.state)

      case `orderNestedTouchedEvent` =>
        new OrderNestedStarted(eventSource.asInstanceOf[Order].orderKey)

      case `orderNestedFinishedEvent` =>
        new OrderNestedFinished(eventSource.asInstanceOf[Order].orderKey)

      case `orderSuspendedEvent` =>
        new OrderSuspended(eventSource.asInstanceOf[Order].orderKey)

      case `orderResumedEvent` =>
        new OrderResumed(eventSource.asInstanceOf[Order].orderKey)

      case `orderSetBackEvent` =>
        val order = eventSource.asInstanceOf[Order]
        new OrderSetBack(order.orderKey, order.state)

      case `orderStepStartedEvent` =>
        val order: Order = eventSource.asInstanceOf[Order]
        new OrderStepStarted(order.orderKey, order.state, order.taskId getOrElse TaskId.Null)

      case o =>
        sys.error(s"Not implemented cppEventCode=$o")
    }
  }

  @ForCpp def newLogEvent(cppLevel: Int, message: String): AbstractEvent =
    LogEvent.of(SchedulerLogLevel.ofCpp(cppLevel), message)

  @ForCpp def newOrderStateChangedEvent(jobChainPath: String, orderId: String, previousState: String, state: String): AbstractEvent =
    OrderNodeChanged(OrderKey(jobChainPath, orderId), previousState = OrderState(previousState), state = OrderState(state))

  @ForCpp def newOrderStepEndedEvent(jobChainPath: String, orderId: String, orderStateTransitionCpp: Long): AbstractEvent =
    OrderStepEnded(OrderKey(jobChainPath, orderId), OrderNodeTransition.ofCppInternalValue(orderStateTransitionCpp))

  @ForCpp def newTaskEndedEvent(taskId: Int, jobPath: String, returnCode: Int): AbstractEvent =
    TaskEnded(TaskId(taskId), JobPath(jobPath), ReturnCode(returnCode))
}
