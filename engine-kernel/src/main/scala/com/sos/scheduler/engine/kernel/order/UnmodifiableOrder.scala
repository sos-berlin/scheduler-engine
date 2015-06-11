package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.order.{OrderState, OrderId, OrderKey}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobChain
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet
import javax.annotation.Nullable
import java.time.Instant

@ForCpp trait UnmodifiableOrder
extends EventSource {

  def key: OrderKey

  def id: OrderId

  def state: OrderState

  def endState: OrderState

  def isSuspended: Boolean

  def title: String

  def jobChain: UnmodifiableJobChain

  @Nullable def jobChainOption: Option[UnmodifiableJobChain]

  def parameters: UnmodifiableVariableSet

  def log: PrefixLog

  def nextInstantOption: Option[Instant]
}
