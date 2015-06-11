package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.data.scheduler.SchedulerId
import java.time.Instant
import spray.json.DefaultJsonProtocol._
import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._

final case class SchedulerOverview(
  version: String,
  versionCommitHash: String,
  startInstant: Instant,
  instant: Instant,
  schedulerId: SchedulerId,
  tcpPort: Option[Int],
  udpPort: Option[Int],
  processId: Int,
  state: String)


object SchedulerOverview {
  implicit val MyJsonFormat = jsonFormat9(apply)
}
