package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import java.time.Instant
import spray.json.DefaultJsonProtocol._
import spray.json.RootJsonFormat

/**
  * @author Joacim Zschimmer
  */
final case class JobOverview(
  path: JobPath,
  fileBasedState: FileBasedState,
  defaultProcessClassPath: Option[ProcessClassPath],
  isOrderJob: Boolean,
  title: String,
  enabled: Boolean,
  state: JobState,
  stateText: String,
  isInPeriod: Boolean,
  nextStartTime: Option[Instant],
  taskLimit: Int,
  usedTaskCount: Int,
  queuedTaskCount: Int,
  lateTaskCount: Int,
  obstacles: Set[JobObstacle],
  taskObstacles: Map[TaskId, Set[TaskObstacle]],
  error: Option[JobOverview.Error])
extends JobView
{
  def taskLimitReached = usedTaskCount >= taskLimit
}

object JobOverview extends JobView.Companion[JobOverview] {
  implicit val ordering: Ordering[JobOverview] = Ordering by { _.path }

  final case class Error(code: String, message: String)
  object Error {
    implicit val jsonFormat = jsonFormat2(Error.apply)
  }

  implicit val jsonFormat: RootJsonFormat[JobOverview] = {
    implicit val x = FileBasedState.MyJsonFormat
    implicit val y = JobState.MyJsonFormat
    jsonFormat17(apply)
  }
}
