package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.order.{OrderState, OrderId, OrderKey}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobChain
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet
import javax.annotation.Nullable
import org.joda.time.Instant

@ForCpp trait UnmodifiableOrder
extends EventSource {

  @Deprecated final def getKey = key
  @Deprecated final def getId = id
  @Deprecated final def getState = state
  @Deprecated final def getEndState = endState
  @Deprecated final def getTitle = title
  @Deprecated final def getJobChain = jobChain
  @Deprecated final def getParameters = parameters
  @Deprecated final def getLog = log

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