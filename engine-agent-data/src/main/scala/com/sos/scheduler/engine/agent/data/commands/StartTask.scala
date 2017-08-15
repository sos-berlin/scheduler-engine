package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.commandresponses.StartTaskResponse
import com.sos.scheduler.engine.agent.data.commands.StartTask.{KeyLogon, Meta}
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
trait StartTask extends TaskCommand {
  type Response = StartTaskResponse

  def meta: Option[Meta]

  def logon: Option[KeyLogon]

  final def isLegacy = logon.isEmpty
}

object StartTask {
  final case class Meta(
    job: JobPath,
    taskId: TaskId)

  object Meta {
    /** For compatibility with a master before v1.10.4 **/
    val Default = Meta(JobPath("/(OLD-MASTER)"), TaskId(-1))
    implicit val MyJsonFormat = jsonFormat2(apply)
  }

  final case class KeyLogon(credentialsKey: String, withUserProfile: Boolean)

  object KeyLogon {
    implicit val MyJsonFormat = jsonFormat2(apply)
  }
}
