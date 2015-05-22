package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId
import scala.collection.immutable
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
sealed trait Command


/**
 * @author Joacim Zschimmer
 */
sealed trait Response


/**
 * @author Joacim Zschimmer
 */
trait ProcessCommand extends Command


trait StartProcess extends ProcessCommand {
  val controllerAddress: String
}

final case class StartThread(controllerAddress: String)
extends StartProcess

final case class StartSeparateProcess(controllerAddress: String, javaOptions: String, javaClasspath: String)
extends StartProcess


/**
 * @author Joacim Zschimmer
 */
final case class StartProcessResponse(processId: AgentProcessId) extends Response


/**
 * @author Joacim Zschimmer
 */
final case class CloseProcess(processId: AgentProcessId, kill: Boolean)
extends ProcessCommand


/**
 * @author Joacim Zschimmer
 */
object CloseProcessResponse extends Response


final case class RequestFileOrderSourceContent(
  directory: String,
  regex: String,
  durationMillis: Long,
  knownFiles: immutable.Set[String])
  extends Command

object RequestFileOrderSourceContent {
  implicit val MyJsonFormat = jsonFormat4(apply)
}

final case class FileOrderSourceContent(files: immutable.Seq[FileOrderSourceContent.Entry])

object FileOrderSourceContent {
  final case class Entry(path: String, lastModifiedTime: Long)

  object Entry {
    implicit val MyJsonFormat = jsonFormat2(apply)
  }

  implicit val MyJsonFormat = jsonFormat1(apply)
}
