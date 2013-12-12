package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.folder.JobChainPath
import scala.Predef.String

final case class OrderKey(jobChainPath: JobChainPath, id: OrderId) {

  @Deprecated
  def getJobChainPath: JobChainPath =
    jobChainPath

  @Deprecated
  def jobChainPathString: String =
    jobChainPath.string

  @Deprecated
  def getId: OrderId =
    id

  @Deprecated
  def idString: String =
    id.asString

  override def toString =
    s"${jobChainPath.string}:$id"
}

object OrderKey {
  def apply(jobChainPath: String, id: String): OrderKey =
    OrderKey(JobChainPath.of(jobChainPath), new OrderId(id))

  def of(jobChainPath: String, id: String): OrderKey =
    OrderKey(JobChainPath.of(jobChainPath), new OrderId(id))
}
