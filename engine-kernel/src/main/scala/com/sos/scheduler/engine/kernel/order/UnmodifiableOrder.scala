package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.order.{OrderId, OrderKey, OrderState}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.log.PrefixLog
import java.time.Instant

@ForCpp trait UnmodifiableOrder
extends EventSource {

  def orderKey: OrderKey

  def id: OrderId

  def state: OrderState

  def title: String

  def variables: Map[String, String]

  def log: PrefixLog

  def nextInstantOption: Option[Instant]
}
