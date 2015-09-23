package com.sos.scheduler.engine.agent.data.views

import com.sos.scheduler.engine.agent.data.AgentTaskId
import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits.InstantJsonFormat
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.tunnel.data.TunnelId
import java.time.Instant
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TaskOverview(
  id: AgentTaskId,
  tunnelId: TunnelId,
  masterAddress: String,
  startedAt: Instant,
  arguments: Option[TaskOverview.Arguments])

object TaskOverview {
  implicit val ArgumentsJsonFormat = jsonFormat5(Arguments)
  implicit val MyJsonFormat = jsonFormat5(apply)

  final case class Arguments(
    taskId: TaskId,
    jobName: String,
    language: String,
    javaClassName: Option[String],
    monitorCount: Int)
}
