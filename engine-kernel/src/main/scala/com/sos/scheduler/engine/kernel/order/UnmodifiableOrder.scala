package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.order.{OrderId, OrderKey}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.log.PrefixLog
import java.time.Instant

@ForCpp trait UnmodifiableOrder
extends EventSource {

  def orderKey: OrderKey

  def id: OrderId

  def nodeId: NodeId

  def title: String

  def variables: Map[String, String]

  def log: PrefixLog

  def nextInstantOption: Option[Instant]
}
