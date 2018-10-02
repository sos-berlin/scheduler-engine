package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.OrderId
import java.time.Instant
import scala.collection.immutable.Seq
import spray.json.DefaultJsonProtocol._
import spray.json.RootJsonFormat

/**
  * @author Joacim Zschimmer
  */
final case class JobDetailed(
  overview: JobOverview,
  defaultParameters: Map[String, String],
  queuedTasks: Seq[JobDetailed.QueuedTask],
  runningTasks: Seq[JobDetailed.RunningTask])
extends JobView {
  def path = overview.path
}

object JobDetailed extends JobView.Companion[JobDetailed]
{
  final case class TaskOrder(orderId: OrderId, jobChainPath: JobChainPath, nodeId: NodeId)

  final case class QueuedTask(
    taskId: TaskId,
    enqueuedAt: Option[Instant],
    startAt: Option[Instant])

  final case class RunningTask(
    taskId: TaskId,
    cause: String,
    enqueuedAt: Option[Instant],
    startAt: Option[Instant],
    startedAt: Instant,
    pid: Option[Int],
    stepCount: Int,
    order: Option[TaskOrder])

  implicit val jsonFormat: RootJsonFormat[JobDetailed] = {
    implicit val x = FileBasedState.MyJsonFormat
    implicit val y = jsonFormat3(QueuedTask.apply)
    implicit val z = jsonFormat3(TaskOrder.apply)
    implicit val ä = jsonFormat8(RunningTask.apply)
    jsonFormat4(apply)
  }
}
