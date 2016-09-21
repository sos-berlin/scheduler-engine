package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}
import com.sos.scheduler.engine.data.job.JobState
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemOverview
import spray.json.DefaultJsonProtocol._

final case class JobSubsystemOverview(
  fileBasedType: FileBasedType,
  count: Int,
  fileBasedStateCounts: Map[FileBasedState, Int],
  jobStateCounts: Map[JobState, Int],
  waitingForProcessClassCount: Int)
extends FileBasedSubsystemOverview


object JobSubsystemOverview {
  private implicit val fileBasedTypeJsonFormat = FileBasedType.MyJsonFormat
  private implicit val fileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  private implicit val jobStateJsonFormat = JobState.MyJsonFormat
  implicit val MyJsonFormat = jsonFormat5(apply)
}
