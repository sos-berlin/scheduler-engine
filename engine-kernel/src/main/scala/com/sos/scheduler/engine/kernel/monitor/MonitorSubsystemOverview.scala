package com.sos.scheduler.engine.kernel.monitor

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}
import com.sos.scheduler.engine.data.job.JobState
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemOverview
import spray.json.DefaultJsonProtocol._

final case class MonitorSubsystemOverview(
  fileBasedType: FileBasedType,
  count: Int,
  fileBasedStateCounts: Map[FileBasedState, Int])
extends FileBasedSubsystemOverview

object MonitorSubsystemOverview
{
  private implicit val fileBasedTypeJsonFormat = FileBasedType.MyJsonFormat
  private implicit val fileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  private implicit val jobStateJsonFormat = JobState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat3(apply)
}
