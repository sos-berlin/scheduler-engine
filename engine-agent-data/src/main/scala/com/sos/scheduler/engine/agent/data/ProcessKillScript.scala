package com.sos.scheduler.engine.agent.data

import com.sos.scheduler.engine.base.generic.IsString
import com.sos.scheduler.engine.common.process.Processes.Pid
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import java.nio.file.{Path, Paths}
import scala.collection.immutable

/**
  * @author Joacim Zschimmer
  */
final case class ProcessKillScript(file: Path) extends IsString {
  def string = file.toString

  def toCommandArguments(id: AgentTaskId, pid: Option[Pid], jobPath: JobPath, taskId: TaskId): immutable.Seq[String] =
    toCommandArguments(id, pid) :+ s"-master-task-id=${taskId.string}" :+ s"-job=${jobPath.string}"

  def toCommandArguments(id: AgentTaskId, pid: Option[Pid]): immutable.Seq[String] =
    Vector(file.toString, s"-kill-agent-task-id=${id.string}") ++ (pid map { o ⇒ s"-pid=${o.string}" })
}

object ProcessKillScript extends IsString.HasJsonFormat[ProcessKillScript] {
  override def apply(o: String) = new ProcessKillScript(Paths.get(o))
}
