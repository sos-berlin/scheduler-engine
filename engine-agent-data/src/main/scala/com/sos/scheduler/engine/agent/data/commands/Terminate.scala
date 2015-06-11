package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.responses.EmptyResponse
import com.sos.scheduler.engine.base.sprayjson.JavaTimeJsonFormats.implicits._
import java.time.Duration
import spray.json.DefaultJsonProtocol._
import com.sos.scheduler.engine.common.time.ScalaTime._

/**
 * @author Joacim Zschimmer
 */
final case class Terminate(
  sigtermProcesses: Boolean = false,
  sigkillProcessesAfter: Duration = Duration.ofSeconds(Long.MaxValue))
extends Command {
  type Response = EmptyResponse.type
}

object Terminate {
  val SerialTypeName = "Terminate"
  val XmlElementName = "agent.terminate"
  val MaxDuration = 31 * 24.h
  implicit val MyJsonFormat = jsonFormat2(apply)

  val AbortImmediately = Terminate(sigtermProcesses = false, sigkillProcessesAfter = 0.s)
}
