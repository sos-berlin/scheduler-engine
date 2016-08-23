package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent}
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedAdded, FileBasedRemoved, FileBasedReplaced}
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode, TaskClosed, TaskEnded, TaskId, TaskKey, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.log.{LogEvent, SchedulerLogLevel}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.event.CppEventCode._
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.kernel.order.Order

@ForCpp object CppEventFactory {

  private[event] def newInstance(cppEventCode: CppEventCode, eventSource: EventSource): AnyKeyedEvent = {
    cppEventCode match {
      case `fileBasedActivatedEvent` ⇒
        KeyedEvent(FileBasedActivated)(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedAddedEvent` =>
        KeyedEvent(FileBasedAdded)(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedRemovedEvent` ⇒
        KeyedEvent(FileBasedRemoved)(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedReplacedEvent` =>
        KeyedEvent(FileBasedReplaced)(eventSource.asInstanceOf[FileBased].path)

      case `taskStartedEvent` ⇒
        val task = eventSource.asInstanceOf[Task]
        KeyedEvent(TaskStarted)(task.taskKey)

      case `taskClosedEvent` =>
        val task = eventSource.asInstanceOf[Task]
        KeyedEvent(TaskClosed)(task.taskKey)

      case `orderTouchedEvent` ⇒
        KeyedEvent(OrderStarted)(eventSource.asInstanceOf[Order].orderKey)

      case `orderFinishedEvent` ⇒
        val order: Order = eventSource.asInstanceOf[Order]
        KeyedEvent(OrderFinished(order.nodeId))(eventSource.asInstanceOf[Order].orderKey)

      case `orderNestedTouchedEvent` ⇒
        KeyedEvent(OrderNestedStarted)(eventSource.asInstanceOf[Order].orderKey)

      case `orderNestedFinishedEvent` ⇒
        KeyedEvent(OrderNestedFinished)(eventSource.asInstanceOf[Order].orderKey)

      case `orderSuspendedEvent` ⇒
        KeyedEvent(OrderSuspended)(eventSource.asInstanceOf[Order].orderKey)

      case `orderResumedEvent` ⇒
        KeyedEvent(OrderResumed)(eventSource.asInstanceOf[Order].orderKey)

      case `orderSetBackEvent` ⇒
        val order = eventSource.asInstanceOf[Order]
        KeyedEvent(OrderSetBack(order.nodeId))(order.orderKey)

      case `orderStepStartedEvent` ⇒
        val order: Order = eventSource.asInstanceOf[Order]
        KeyedEvent(OrderStepStarted(order.nodeId, order.taskId getOrElse TaskId.Null))(order.orderKey)

      case o ⇒
        sys.error(s"Not implemented cppEventCode=$o")
    }
  }

  @ForCpp def newLogEvent(cppLevel: Int, message: String): AnyKeyedEvent =
    KeyedEvent(LogEvent(SchedulerLogLevel.ofCpp(cppLevel), message))

  @ForCpp def newOrderStateChangedEvent(jobChainPath: String, orderId: String, previousNodeId: String, nodeId: String): AnyKeyedEvent =
    KeyedEvent(OrderNodeChanged(nodeId = NodeId(nodeId), fromNodeId = NodeId(previousNodeId)))(OrderKey(jobChainPath, orderId))

  @ForCpp def newOrderStepEndedEvent(jobChainPath: String, orderId: String, nodeTransitionCpp: Long): AnyKeyedEvent =
    KeyedEvent(OrderStepEnded(OrderNodeTransition.ofCppInternalValue(nodeTransitionCpp)))(OrderKey(jobChainPath, orderId))

  @ForCpp def newTaskEndedEvent(taskId: Int, jobPath: String, returnCode: Int): AnyKeyedEvent =
    KeyedEvent(TaskEnded(ReturnCode(returnCode)))(TaskKey(JobPath(jobPath), TaskId(taskId)))
}
