package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.order.{OrderState, OrderId, OrderKey}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet
import javax.annotation.Nullable

@ForCpp trait UnmodifiableOrder extends EventSource {
  def getKey: OrderKey

  def getId: OrderId

  def getState: OrderState

  def getEndState: OrderState

  def suspended: Boolean

  def setSuspended(o: Boolean)

  def getTitle: String

  def getJobChain: UnmodifiableJobchain

  @Nullable def getJobChainOrNull: UnmodifiableJobchain

  def getParameters: UnmodifiableVariableSet

  def getLog: PrefixLog
}