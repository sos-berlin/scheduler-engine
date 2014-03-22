package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.SisterType
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.JobChainPath
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
  protected val cppProxy: OrderC, injector: Injector)
extends FileBased
with UnmodifiableOrder
with HasUnmodifiableDelegate[UnmodifiableOrder]
with OrderPersistence {

  type Path = OrderKey

  lazy val unmodifiableDelegate = new UnmodifiableOrderDelegate(this)

  protected val orderSubsystem = injector.apply[OrderSubsystem]

  def onCppProxyInvalidated() {}

  def remove() {
    cppProxy.java_remove()
  }

  def stringToPath(o: String) = OrderKey(o)

  def fileBasedType = FileBasedType.order

  def key: OrderKey =
    jobChainPath orderKey id

  def id: OrderId =
    OrderId(cppProxy.string_id)

  def state: OrderState =
    OrderState(cppProxy.string_state)

  def initialState: OrderState =
    OrderState(cppProxy.initial_state_string)

  def endState: OrderState =
    OrderState(cppProxy.end_state_string)

  def endState_=(s: OrderState) {
    cppProxy.set_end_state(s.string)
  }

  def priority: Int =
    cppProxy.priority

  def priority_=(o: Int) {
    cppProxy.set_priority(o)
  }

  def isSuspended: Boolean =
    cppProxy.suspended

  def isSuspended_=(b: Boolean) {
    cppProxy.set_suspended(b)
  }

  def title: String =
    cppProxy.title

  def title_=(o: String) {
    cppProxy.set_title(o)
  }

  def jobChainPath: JobChainPath =
    JobChainPath(cppProxy.job_chain_path_string)

  def jobChain: JobChain =
    jobChainOption getOrElse { throw new SchedulerException(s"Order is not in a job chain: $toString") }

  def jobChainOption: Option[JobChain] =
    Option(cppProxy.job_chain) map { _.getSister }

  def parameters: VariableSet =
    cppProxy.params.getSister

  def nextInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_time_millis)

  def createdAtOption: Option[Instant] = ???

  override def toString = {
    val result = getClass.getSimpleName
    if (cppProxy.cppReferenceIsValid) s"$result $jobChainPath:$id"
    else result
  }
}


@ForCpp object Order {

  final class Type extends SisterType[Order, OrderC] {
    def sister(proxy: OrderC, context: Sister): Order =
      new Order(proxy, context.asInstanceOf[HasInjector].injector)
  }
}
