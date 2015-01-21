package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey}
import com.sos.scheduler.engine.data.order.{OrderId, OrderKey, OrderState}
import com.sos.scheduler.engine.eventbus.HasUnmodifiableDelegate
import com.sos.scheduler.engine.kernel.cppproxy.OrderC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerException}
import com.sos.scheduler.engine.kernel.time.CppJodaConversions.eternalCppMillisToNoneInstant
import com.sos.scheduler.engine.kernel.variable.VariableSet
import org.joda.time.Instant

@ForCpp
final class Order private(
  protected val cppProxy: OrderC,
  protected val subsystem: StandingOrderSubsystem)
extends FileBased
with UnmodifiableOrder
with HasUnmodifiableDelegate[UnmodifiableOrder]
with OrderPersistence {

  type Path = OrderKey

  lazy val unmodifiableDelegate = new UnmodifiableOrderDelegate(this)

  def onCppProxyInvalidated(): Unit = {}

  def remove(): Unit = {
    cppProxy.java_remove()
  }

  def stringToPath(o: String) = OrderKey(o)

  def fileBasedType = FileBasedType.order

  def key: OrderKey =
    jobChainPath orderKey id

  def id: OrderId =
    OrderId(cppProxy.string_id)

  def nodeKey = NodeKey(jobChainPath, state)

  def state: OrderState =
    OrderState(cppProxy.string_state)

  def initialState: OrderState =
    OrderState(cppProxy.initial_state_string)

  def endState: OrderState =
    OrderState(cppProxy.end_state_string)

  def endState_=(s: OrderState): Unit = {
    cppProxy.set_end_state(s.string)
  }

  def priority: Int =
    cppProxy.priority

  def priority_=(o: Int): Unit = {
    cppProxy.set_priority(o)
  }

  def isSuspended: Boolean =
    cppProxy.suspended

  def isSuspended_=(b: Boolean): Unit = {
    cppProxy.set_suspended(b)
  }

  def title: String =
    cppProxy.title

  def title_=(o: String): Unit = {
    cppProxy.set_title(o)
  }

  def jobChainPath: JobChainPath =
    emptyToNone(cppProxy.job_chain_path_string) map JobChainPath.apply getOrElse throwNotInAJobChain()

  def jobChain: JobChain =
    jobChainOption getOrElse throwNotInAJobChain()

  def jobChainOption: Option[JobChain] =
    Option(cppProxy.job_chain) map { _.getSister }

  def parameters: VariableSet =
    cppProxy.params.getSister

  def nextInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_time_millis)

  def createdAtOption: Option[Instant] = ???

  override def toString = {
    val result = getClass.getSimpleName
    if (cppProxy.cppReferenceIsValid) s"$result ${cppProxy.job_chain_path_string}:$id"
    else result
  }

  private def throwNotInAJobChain() = throw new SchedulerException(s"Order is not in a job chain: $toString")
}


object Order {
  final class Type extends SisterType[Order, OrderC] {
    def sister(proxy: OrderC, context: Sister): Order = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Order(proxy, injector.apply[StandingOrderSubsystem])
    }
  }
}
