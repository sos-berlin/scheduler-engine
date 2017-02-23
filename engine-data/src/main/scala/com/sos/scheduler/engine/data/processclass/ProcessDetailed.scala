package com.sos.scheduler.engine.data.processclass

import com.sos.jobscheduler.base.sprayjson.JavaTimeJsonFormats.implicits._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.job.{JobPath, TaskKey}
import java.time.Instant
import spray.json.DefaultJsonProtocol._

/**
  * @author Joacim Zschimmer
  */
final case class ProcessDetailed(
  jobPath: JobPath,
  taskId: TaskId,
  startedAt: Instant,
  pid: Option[Int],
  agent: Option[AgentAddress])
{
  def taskKey = TaskKey(jobPath, taskId)
}

object ProcessDetailed {
  implicit val jsonFormat = jsonFormat5(apply)
}
