package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.SisterType
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.eventbus.HasUnmodifiableDelegate
import com.sos.scheduler.engine.kernel.cppproxy.OrderC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import com.sos.scheduler.engine.kernel.time.CppJodaConversions.eternalCppMillisToNoneInstant
import com.sos.scheduler.engine.kernel.variable.VariableSet
import org.joda.time.Instant

@ForCpp final class Order(cppProxy: OrderC)
extends FileBased
with UnmodifiableOrder
with HasUnmodifiableDelegate[UnmodifiableOrder]
with Sister {

  lazy val unmodifiableDelegate = new UnmodifiableOrderDelegate(this)

  def onCppProxyInvalidated() {}

  def remove() {
    cppProxy.java_remove()
  }

  def fileBasedType: FileBasedType =
    FileBasedType.order

  def path: OrderKey =
    OrderKey(cppProxy.path)

  def key: OrderKey =
    jobChainPath orderKey id

  def id: OrderId =
    OrderId(cppProxy.string_id)

  def state: OrderState =
    OrderState(cppProxy.string_state)

  def endState: OrderState =
    OrderState(cppProxy.string_end_state)

  def endState_=(s: OrderState) {
    cppProxy.set_end_state(s.string)
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

  def log: PrefixLog =
    cppProxy.log.getSister

  def nextInstantOption: Option[Instant] =
    eternalCppMillisToNoneInstant(cppProxy.next_time_millis)

  override def toString = {
    val result = getClass.getSimpleName
    if (cppProxy.cppReferenceIsValid) s"$result $id"
    else result
  }
}


@ForCpp object Order {

  final class Type extends SisterType[Order, OrderC] {
    def sister(proxy: OrderC, context: Sister): Order =
      new Order(proxy)
  }
}
