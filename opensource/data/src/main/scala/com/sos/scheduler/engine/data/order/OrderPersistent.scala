package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.base.HasKey
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import org.joda.time.ReadableInstant

case class OrderPersistent(
  jobChainPath: JobChainPath,
  orderId: OrderId,
  distributedNextTimeOption: Option[ReadableInstant],
  occupyingClusterIdOption: Option[ClusterMemberId],
  priority: Int,
  ordering: Int,
  stateOption: Option[OrderState],
  initialStateOption: Option[OrderState],
  title: String,
  creationTimestampOption: Option[ReadableInstant],
  modificationTimestampOption: Option[ReadableInstant],
  payloadXmlOption: Option[String],
  runtimeXmlOption: Option[String],
  xmlOption: Option[String])

extends HasKey[OrderKey] {

  def key = new OrderKey(jobChainPath, orderId)
}
