package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import java.time.Instant
import spray.json.DefaultJsonProtocol._
import spray.json.RootJsonFormat

/**
  * @author Joacim Zschimmer
  */
final case class JobHistoryEntry(
  taskId: Option[TaskId],
  jobPath: JobPath,
  startedAt: Instant,
  endedAt: Option[Instant],
  cause: Option[String],
  clusterMemberId: Option[ClusterMemberId],
  stepCount: Option[Int],
  returnCode: Option[ReturnCode],
  pid: Option[Int],
  agentUri: Option[String],
  parameters: Option[Map[String, String]],
  error: Option[JobOverview.Error])

object JobHistoryEntry {
  implicit val jsonFormat: RootJsonFormat[JobHistoryEntry] = jsonFormat12(apply)
}
