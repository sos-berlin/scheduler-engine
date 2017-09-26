package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent}
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedAdded, FileBasedRemoved, FileBasedReplaced}
import com.sos.scheduler.engine.data.job.{JobPath, JobStateChanged, JobTaskQueueChanged, JobUnstopped, ReturnCode, TaskClosed, TaskEnded, TaskId, TaskKey, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.scheduler.{SchedulerState, SchedulerStateChanged}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.event.CppEventCode._
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.{Job, Task}
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain

@ForCpp object CppEventFactory {

  private[event] def newInstance(cppEventCode: CppEventCode, eventSource: EventSource): AnyKeyedEvent = {
    cppEventCode match {
      case `fileBasedActivatedEvent` ⇒
        KeyedEvent(FileBasedActivated)(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedAddedEvent` ⇒
        KeyedEvent(FileBasedAdded)(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedRemovedEvent` ⇒
        KeyedEvent(FileBasedRemoved)(eventSource.asInstanceOf[FileBased].path)

      case `fileBasedReplacedEvent` ⇒
        KeyedEvent(FileBasedReplaced)(eventSource.asInstanceOf[FileBased].path)

      case `jobChainStateChanged` ⇒
        val jobChain = eventSource.asInstanceOf[JobChain]
        KeyedEvent(JobChainStateChanged(jobChain.state))(jobChain.path)

      case `jobStateChanged` ⇒
        val job = eventSource.asInstanceOf[Job]
        KeyedEvent(JobStateChanged(job.state))(job.path)

      case `jobUnstopped` ⇒
        val job = eventSource.asInstanceOf[Job]
        KeyedEvent(JobUnstopped)(job.path)

      case `jobTaskQueueChanged` ⇒
        val job = eventSource.asInstanceOf[Job]
        KeyedEvent(JobTaskQueueChanged)(job.path)

      case `taskStartedEvent` ⇒
        val task = eventSource.asInstanceOf[Task]
        KeyedEvent(TaskStarted)(task.taskKey)

      case `taskClosedEvent` ⇒
        val task = eventSource.asInstanceOf[Task]
        KeyedEvent(TaskClosed)(task.taskKey)

      case `orderStartedEvent` ⇒
        KeyedEvent(OrderStarted)(eventSource.asInstanceOf[Order].orderKey)

      case `orderFinishedEvent` ⇒
        val order = eventSource.asInstanceOf[Order]
        KeyedEvent(OrderFinished(order.nodeId))(eventSource.asInstanceOf[Order].orderKey)

      case `orderWaitingInTask` ⇒
        KeyedEvent(OrderWaitingInTask)(eventSource.asInstanceOf[Order].orderKey)

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
        val order = eventSource.asInstanceOf[Order]
        KeyedEvent(OrderStepStarted(order.nodeId, order.taskIdOption getOrElse TaskId.Null))(order.orderKey)
    }
  }

  @ForCpp def newJobChainNodeActionChangedEvent(jobChainPath: String, nodeId: String, action: Int): AnyKeyedEvent =
    KeyedEvent(JobChainNodeActionChanged(NodeId(nodeId), JobChainNodeAction.values()(action)))(JobChainPath(jobChainPath))

  @ForCpp def newOrderStateChangedEvent(jobChainPath: String, orderId: String, previousNodeId: String, nodeId: String): AnyKeyedEvent =
    KeyedEvent(OrderNodeChanged(nodeId = NodeId(nodeId), fromNodeId = NodeId(previousNodeId)))(OrderKey(jobChainPath, orderId))

  @ForCpp def newOrderStepEndedEvent(jobChainPath: String, orderId: String, nodeTransitionCpp: Long): AnyKeyedEvent =
    KeyedEvent(OrderStepEnded(OrderNodeTransition.ofCppInternalValue(nodeTransitionCpp)))(OrderKey(jobChainPath, orderId))

  @ForCpp def newTaskEndedEvent(taskId: Int, jobPath: String, returnCode: Int): AnyKeyedEvent =
    KeyedEvent(TaskEnded(ReturnCode(returnCode)))(TaskKey(JobPath(jobPath), TaskId(taskId)))

  @ForCpp def newSchedulerStateChanged(statusCode: Int): AnyKeyedEvent =
    SchedulerStateChanged(SchedulerState.values()(statusCode))
}
