package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.scalautil.{Logger, SetOnce}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxyInvalidatedException, Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId, NodeKey}
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderHistoryId, OrderId, OrderKey, OrderObstacle, OrderOverview, OrderProcessingState, OrderSourceType, OrderView}
import com.sos.scheduler.engine.data.queries.QueryableOrder
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.async.CppCall
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.OrderC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.{JobSubsystem, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.Order._
import com.sos.scheduler.engine.kernel.order.jobchain.{JobChain, Node}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.{FileOrderAgentUriVariableName, FileOrderPathVariableName}
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerException}
import com.sos.scheduler.engine.kernel.time.CppTimeConversions.{eternalCppMillisToNoneInstant, zeroCppMillisToNoneInstant}
import java.lang.System.currentTimeMillis
import java.time.Instant
import scala.util.{Failure, Success}

@ForCpp
private[engine] final class Order private(
  protected[order] val cppProxy: OrderC,
  protected[kernel] val subsystem: StandingOrderSubsystem,
  orderSubsystem: OrderSubsystem,
  jobSubsystem: JobSubsystem,
  taskSubsystem: TaskSubsystem)
extends FileBased
with UnmodifiableOrder
with OrderPersistence {

  import subsystem.agentClientFactory

  protected type Self = Order
  type ThisPath = OrderKey

  private val idOnce = new SetOnce[OrderId]
  private val sourceTypeOnce = new SetOnce[OrderSourceType]
  private[kernel] val queryable = new QueryableOrder {
    private def flags = cppProxy.java_fast_flags
    def isSuspended = cppFastFlags.isSuspended(flags)
    def isSetback = cppFastFlags.isSetback(flags)
    def sourceType = Order.this.sourceType
    def orderKey = Order.this.orderKey
    def isBlacklisted = cppFastFlags.isBlacklisted(flags)
    def processingStateClass = processingState(cppProxy.java_fast_flags, nextStepAtOption = nextStepAtOption).getClass
  }

  def onCppProxyInvalidated(): Unit = {}

  private[kernel] def remove(): Unit = {
    cppProxy.java_remove()
  }

  @ForCpp
  private def agentFileExists(cppCall: CppCall): Unit = {
    import subsystem.schedulerThreadCallQueue.implicits.executionContext
    val orderId = id
    val p = cppProxy.params.getSister
    val file = p("scheduler_file_path")
    require(file.nonEmpty, "Order variable scheduler_file_path must not be empty")
    val agentUri = p(FileOrderAgentUriVariableName)
    require(agentUri.nonEmpty, s"Order variable $FileOrderAgentUriVariableName must not be empty")
    agentClientFactory.apply(agentUri).fileExists(file) onComplete {
      case Success(exists) ⇒
        try cppCall.call(exists: java.lang.Boolean)
        catch { case t: CppProxyInvalidatedException ⇒ logger.trace(s"Order '$orderId' has vanished while agentFileExists returns $exists") }
      case Failure(t) ⇒ log.error(t.toString)
    }
  }

  private[kernel] def view[V <: OrderView: OrderView.Companion]: V =
    implicitly[OrderView.Companion[V]] match {
      case OrderOverview ⇒ overview.asInstanceOf[V]
      case OrderDetailed ⇒ details.asInstanceOf[V]
    }

  private[kernel] def overview: OrderOverview = {
    val orderKey = this.pathOrKey
    val nodeId = this.nodeId
    val flags = cppProxy.java_fast_flags
    val nextStepAtOption = this.nextStepAtOption
    val processingState = this.processingState(flags, nextStepAtOption)
    OrderOverview(
      path = orderKey,  // key because this.path is valid only for permanent orders
      cppFastFlags.fileBasedState(flags),
      sourceType,
      nodeId = nodeId,
      processingState = processingState,
      historyIdOption,
      obstacles = obstacles(flags, processingState),
      startedAt = startedAtOption,
      nextStepAt = nextStepAtOption)
  }

  private[order] def processingState(flags: Long, nextStepAtOption: Option[Instant]): OrderProcessingState = {
    val isBlacklisted = cppFastFlags.isBlacklisted(flags)
    val isTouched = cppFastFlags.isTouched(flags)
    val isSetback = cppFastFlags.isSetback(flags)
    val taskId = this.taskId
    val currentSecond = currentTimeMillis / 1000
    import OrderProcessingState._
    (taskId, taskId flatMap taskSubsystem.taskOption) match {
      case (Some(taskId_), Some(task)) ⇒  // The task may be registered a little bit later.
        task.stepOrProcessStartedAt match {
          case None ⇒ WaitingInTask(taskId_, task.processClassPath)
          case Some(at) ⇒ InTaskProcess(taskId_, task.processClassPath, at, task.agentAddress)
        }
      case (_, _) ⇒
        occupyingClusterMemberId match {
          case Some(clusterMemberId) ⇒ OccupiedByClusterMember(clusterMemberId)
          case None ⇒
            if (isBlacklisted)
              Blacklisted
            else if (!isTouched)
              nextStepAtOption match {
                case None ⇒ NotPlanned
                case Some(at) if at.getEpochSecond >= currentSecond ⇒ Planned(at)
                case Some(at) ⇒ Pending(at)
              }
            else if (isSetback)
              Setback(setbackUntilOption getOrElse Instant.MAX)
            else
              WaitingForOther
        }
    }
  }

  private[order] def obstacles(flags: Long, processingState: OrderProcessingState) = {
    val isBlacklisted = cppFastFlags.isBlacklisted(flags)
    val isSuspended = cppFastFlags.isSuspended(flags)
    import OrderObstacle._
    val b = Set.newBuilder[OrderObstacle]
    for (o ← emptyToNone(fileBasedObstacles)) {
      b += FileBasedObstacles(o)
    }
    if (isSuspended) b += Suspended
    if (isBlacklisted) b += Blacklisted
    processingState match {
      case OrderProcessingState.Setback(at) ⇒ b += Setback(at)
      case _ ⇒
    }
    b.result
  }

  private[kernel] def details: OrderDetailed =
    OrderDetailed(
      overview = overview,
      priority = priority,
      initialNodeId = emptyToNone(cppProxy.initial_state_string) map NodeId.apply,
      endNodeId = emptyToNone(cppProxy.end_state_string) map NodeId.apply,
      title = title,
      variables = variables)

  def stringToPath(o: String) = OrderKey(o)

  def fileBasedType = FileBasedType.Order

  private[order] def sourceType: OrderSourceType =
    sourceTypeOnce getOrUpdate toOrderSourceType(isFileBased = cppProxy.is_file_based, isFileOrder = cppProxy.is_file_order)

  override protected def pathOrKey = if (hasBaseFile) path else orderKey

  def orderKey: OrderKey = inSchedulerThread { jobChainPath orderKey id }

  def id: OrderId =
    inSchedulerThread {
      idOnce getOrElse {
        if (cppProxy.id_locked) {
          idOnce.trySet(OrderId(cppProxy.string_id))
          idOnce()
        } else
          OrderId(cppProxy.string_id)
        }
    }

  private def historyIdOption: Option[OrderHistoryId] =
    cppProxy.history_id match {
      case 0 ⇒ None
      case o ⇒ Some(OrderHistoryId(o))
    }

  def nodeKey = inSchedulerThread { NodeKey(jobChainPath, nodeId) }

  def nodeId: NodeId =
    inSchedulerThread {
      cppProxy.java_job_chain_node match {
        case null ⇒ NodeId(cppProxy.string_state)
        case node ⇒ node.nodeId  // java_job_chain_node is somewhat faster then accessing string_state
      }
    }

  private def nodeOption: Option[Node] = Option(cppProxy.java_job_chain_node)

  private[order] def initialNodeId: NodeId =
    NodeId(cppProxy.initial_state_string)

//  def endNodeId: NodeId =
//    NodeId(cppProxy.end_state_string)
//
//  def endNodeId_=(s: NodeId): Unit = {
//    cppProxy.set_end_state(s.string)
//  }

  private def startedAtOption: Option[Instant] = zeroCppMillisToNoneInstant(cppProxy.startTimeMillis)

  private[order] def nextStepAtOption: Option[Instant] = eternalCppMillisToNoneInstant(cppProxy.next_step_at_millis)

  private def setbackUntilOption: Option[Instant] = zeroCppMillisToNoneInstant(cppProxy.setback_millis)

  private def occupyingClusterMemberId: Option[ClusterMemberId] =
    emptyToNone(cppProxy.java_occupying_cluster_member_id) map ClusterMemberId.apply

  private[kernel] def taskId: Option[TaskId] =
    cppProxy.task_id match {
      case 0 ⇒ None
      case o ⇒ Some(TaskId(o))
    }

  private[order] def priority: Int =
    cppProxy.priority

//  def priority_=(o: Int): Unit = {
//    cppProxy.set_priority(o)
//  }

  private[order] def isSuspended: Boolean =
    cppProxy.suspended

//  def isSuspended_=(b: Boolean): Unit = {
//    cppProxy.set_suspended(b)
//  }

  def title: String = inSchedulerThread { cppProxy.title }

//  def title_=(o: String): Unit = {
//    cppProxy.set_title(o)
//  }

  private[kernel] def jobChainPath: JobChainPath =
    cppProxy.job_chain match {
      case null ⇒
        emptyToNone(cppProxy.job_chain_path_string) map JobChainPath.apply getOrElse throwNotInAJobChain()
      case o ⇒ o.getSister.path   // Faster
    }

  private[kernel] def jobChain: JobChain =
    jobChainOption getOrElse throwNotInAJobChain()

  private def jobChainOption: Option[JobChain] =
    Option(cppProxy.job_chain) map { _.getSister }

  private[kernel] def filePath: String = cppProxy.params.get_string(FileOrderPathVariableName)

  //private def fileAgentUri: String = cppProxy.params.get_string(FileOrderAgentUriVariableName)

  def variables: Map[String, String] = inSchedulerThread { cppProxy.params.getSister.toMap }

  def nextInstantOption: Option[Instant] =
    inSchedulerThread { eternalCppMillisToNoneInstant(cppProxy.next_time_millis) }

  private[order] def createdAtOption: Option[Instant] = throw new UnsupportedOperationException

  override def toString =
    List(getClass.getSimpleName) ++ ((fixedPathOption orElse idOnce.toOption) map { o ⇒ s"('$o')" }) mkString " "

  private[kernel] def setOnBlacklist(): Unit = cppProxy.set_on_blacklist()

  private def throwNotInAJobChain() = throw new SchedulerException(s"Order is not in a job chain: $toString")

  private[kernel] def setEndStateReached() = cppProxy.set_end_state_reached()
}

object Order {
  object Type extends SisterType[Order, OrderC] {
    def sister(proxy: OrderC, context: Sister): Order = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Order(proxy,
        injector.instance[StandingOrderSubsystem],
        injector.instance[OrderSubsystem],
        injector.instance[JobSubsystem],
        injector.instance[TaskSubsystem])
    }
  }

  private val logger = Logger(getClass)

  object cppFastFlags {
    def isFileBased   (flags: Long) = (flags & 0x01) != 0
    def isSuspended   (flags: Long) = (flags & 0x02) != 0
    def isBlacklisted (flags: Long) = (flags & 0x04) != 0
    def isSetback     (flags: Long) = (flags & 0x08) != 0
    def fileBasedState(flags: Long) = FileBasedState.values()(((flags & 0xf0) >> 4).toInt)
    def isTouched     (flags: Long) = (flags & 0x100) != 0
    def isInProcess   (flags: Long) = (flags & 0x200) != 0
  }

  private def toOrderSourceType(isFileBased: Boolean, isFileOrder: Boolean) =
    if (isFileOrder)
      OrderSourceType.FileOrder
    else if (isFileBased)
      OrderSourceType.Permanent
    else
      OrderSourceType.AdHoc
}
