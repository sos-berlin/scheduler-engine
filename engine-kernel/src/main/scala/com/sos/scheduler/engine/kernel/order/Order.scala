package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.scalautil.{Logger, SetOnce}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxyInvalidatedException, Sister, SisterType}
import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.EternalCppMillis
import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey}
import com.sos.scheduler.engine.data.order.{OrderId, OrderKey, OrderOverview, OrderSourceType, OrderState, QueryableOrder}
import com.sos.scheduler.engine.kernel.async.CppCall
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.OrderC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.order.Order._
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.{FileOrderAgentUriVariableName, FileOrderPathVariableName}
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerException}
import com.sos.scheduler.engine.kernel.time.CppTimeConversions.{eternalCppMillisToNoneInstant, zeroCppMillisToNoneInstant}
import com.sos.scheduler.engine.kernel.variable.VariableSet
import java.time.Instant
import scala.util.{Failure, Success}

@ForCpp
private[engine] final class Order private(
  protected[this] val cppProxy: OrderC,
  protected[kernel] val subsystem: StandingOrderSubsystem)
extends FileBased
with QueryableOrder
with UnmodifiableOrder
with OrderPersistence {

  import subsystem.agentClientFactory

  type ThisPath = OrderKey

  private val idOnce = new SetOnce[OrderId]
  private val sourceTypeOnce = new SetOnce[OrderSourceType]

  def onCppProxyInvalidated(): Unit = {}

  private[kernel] def remove(): Unit = {
    cppProxy.java_remove()
  }

  @ForCpp
  private def agentFileExists(cppCall: CppCall): Unit = {
    import subsystem.schedulerThreadCallQueue.implicits.executionContext
    val orderId = id
    val p = parameters
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

  private[kernel] override def overview: OrderOverview = {
    val flags = cppProxy.java_fast_flags
    OrderOverview(
      path = orderKey,  // key because this.path is valid only for permanent orders
      cppFastFlags.fileBasedState(flags),
      cppFastFlags.sourceType(flags),
      orderState = state,
      nextStepAt = nextStepAt,
      setbackUntil = if (cppFastFlags.isSetback(flags)) setbackUntil else None,
      taskId = taskId,
      isBlacklisted = cppFastFlags.isBlacklisted(flags),
      isSuspended = cppFastFlags.isSuspended(flags))
  }

  // Public for QueryableOrder
  def isSetback = setbackUntil.isDefined

  def stringToPath(o: String) = OrderKey(o)

  def fileBasedType = FileBasedType.order

  // Public for QueryableOrder
  def sourceType: OrderSourceType =
    sourceTypeOnce getOrUpdate toOrderSourceType(hasBaseFile = cppProxy.has_base_file, isFileOrder = cppProxy.is_file_order)

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

  def nodeKey = inSchedulerThread { NodeKey(jobChainPath, state) }

  def state: OrderState =
    inSchedulerThread {
      cppProxy.java_job_chain_node match {
        case null ⇒ OrderState(cppProxy.string_state)
        case node ⇒ node.orderState  // java_job_chain_node is somewhat faster then accessing string_state
      }
    }

  private[kernel] def initialState: OrderState =
    OrderState(cppProxy.initial_state_string)

//  def endState: OrderState =
//    OrderState(cppProxy.end_state_string)
//
//  def endState_=(s: OrderState): Unit = {
//    cppProxy.set_end_state(s.string)
//  }

  private def nextStepAt: Option[Instant] =
    cppProxy.next_step_at_millis match {
      case EternalCppMillis ⇒ None
      case millis ⇒ Some(Instant ofEpochMilli millis)
    }

  private def setbackUntil: Option[Instant] = zeroCppMillisToNoneInstant(cppProxy.setback_millis)

  private[kernel] def taskId: Option[TaskId] =
    cppProxy.task_id match {
      case 0 ⇒ None
      case o ⇒ Some(TaskId(o))
    }

  private[kernel] def priority: Int =
    cppProxy.priority

//  def priority_=(o: Int): Unit = {
//    cppProxy.set_priority(o)
//  }

  // Public for QueryableOrder
  def isSuspended: Boolean =
    cppProxy.suspended

//  def isSuspended_=(b: Boolean): Unit = {
//    cppProxy.set_suspended(b)
//  }

  // Public for QueryableOrder
  def isBlacklisted = cppProxy.is_on_blacklist

  def title: String = inSchedulerThread { cppProxy.title }

//  def title_=(o: String): Unit = {
//    cppProxy.set_title(o)
//  }

  private[kernel] def jobChainPath: JobChainPath =
    cppProxy.job_chain match {
      case null ⇒ emptyToNone(cppProxy.job_chain_path_string) map JobChainPath.apply getOrElse throwNotInAJobChain()
      case o ⇒ o.getSister.path   // Faster
    }

  private[kernel] def jobChain: JobChain =
    jobChainOption getOrElse throwNotInAJobChain()

  private def jobChainOption: Option[JobChain] =
    Option(cppProxy.job_chain) map { _.getSister }

  private[kernel] def filePath: String = cppProxy.params.get_string(FileOrderPathVariableName)

  //private def fileAgentUri: String = cppProxy.params.get_string(FileOrderAgentUriVariableName)

  def parameters: VariableSet = inSchedulerThread { cppProxy.params.getSister }

  def nextInstantOption: Option[Instant] =
    inSchedulerThread { eternalCppMillisToNoneInstant(cppProxy.next_time_millis) }

  private[kernel] def createdAtOption: Option[Instant] = throw new UnsupportedOperationException

  override def toString = getClass.getSimpleName + (idOnce.toOption map { o ⇒ s"('$o')" })

  private[kernel] def blacklist(): Unit = cppProxy.set_on_blacklist()

  private def throwNotInAJobChain() = throw new SchedulerException(s"Order is not in a job chain: $toString")

  private[kernel] def setEndStateReached() = cppProxy.set_end_state_reached()
}

object Order {
  object Type extends SisterType[Order, OrderC] {
    def sister(proxy: OrderC, context: Sister): Order = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Order(proxy, injector.instance[StandingOrderSubsystem])
    }
  }

  private val logger = Logger(getClass)

  object cppFastFlags {
    def hasBaseFile   (flags: Long) = (flags &  0x01) != 0
    def isSuspended   (flags: Long) = (flags &  0x02) != 0
    def isBlacklisted (flags: Long) = (flags &  0x04) != 0
    def isFileOrder   (flags: Long) = (flags &  0x08) != 0
    def fileBasedState(flags: Long) = FileBasedState.values()(((flags & 0xf0) >> 4).toInt)
    def isSetback     (flags: Long) = (flags & 0x100) != 0
    def sourceType    (flags: Long) = toOrderSourceType(hasBaseFile = hasBaseFile(flags), isFileOrder = isFileOrder(flags))
  }

  private def toOrderSourceType(hasBaseFile: Boolean, isFileOrder: Boolean) =
    if (isFileOrder)
      OrderSourceType.fileOrderSource
    else if (hasBaseFile)
      OrderSourceType.fileBased
    else
      OrderSourceType.adHoc
}
