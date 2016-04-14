package com.sos.scheduler.engine.agent.data

import com.sos.scheduler.engine.base.generic.IsString
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import java.nio.file.{Path, Paths}
import scala.collection.immutable

/**
  * @author Joacim Zschimmer
  */
final case class ProcessKillScript(file: Path) extends IsString {
  def string = file.toString

  def toCommandArguments(id: AgentTaskId, jobPath: JobPath, taskId: TaskId): immutable.Seq[String] =
    toCommandArguments(id) ++ List(s"-master-task-id=${taskId.string}", s"-job=${jobPath.string}")

  def toCommandArguments(id: AgentTaskId): immutable.Seq[String] = List(file.toString, s"-kill-agent-task-id=${id.string}")
}

object ProcessKillScript extends IsString.HasJsonFormat[ProcessKillScript] {
  override def apply(o: String) = new ProcessKillScript(Paths.get(o))
}
