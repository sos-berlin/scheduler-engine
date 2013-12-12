package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.folder.JobChainPath
import scala.Predef.String

final case class OrderKey(jobChainPath: JobChainPath, id: OrderId) {

  def getJobChainPath: JobChainPath =
    jobChainPath

  def jobChainPathString: String =
    jobChainPath.asString

  def getId: OrderId =
    id

  def idString: String =
    id.asString

  override def toString =
    s"${jobChainPath.string}:$id"
}

object OrderKey {
  def of(jobChainPath: String, id: String): OrderKey =
    new OrderKey(JobChainPath.of(jobChainPath), new OrderId(id))
}

