package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.google.inject.Injector
import com.sos.scheduler.engine.agent.client.FileOrderSourceClient
import com.sos.scheduler.engine.agent.data.RequestFileOrderSourceContent
import com.sos.scheduler.engine.client.agent.CppFileOrderSourceClient._
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.CppCall
import scala.collection.JavaConversions._
import scala.util.{Failure, Success}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppFileOrderSourceClient private(client: FileOrderSourceClient, agentUri: String, directory: String, regex: String, durationMillis: Long)
  (implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  @ForCpp
  def readFiles(knownFiles: java.util.List[String], resultCppCall: CppCall): Unit = {
    val request = RequestFileOrderSourceContent(directory = directory, regex = regex, durationMillis = durationMillis, knownFiles.toSet)
    client.readNewFiles(agentUri = agentUri, request) onComplete { completion ⇒
      try completion match {
        case Success(result) ⇒ resultCppCall.call(Success(result.files map {_.path }: java.util.List[String]))
        case Failure(t) ⇒ resultCppCall.call(Failure(t))
      }
      catch { case t: CppProxyInvalidatedException ⇒ logger.trace(s"Ignored: $t") }  // Okay if C++ object (Directory_file_order_source) has been closed
    }
  }
}

object CppFileOrderSourceClient {
  private val logger = Logger(getClass)

  @ForCpp
  def apply(agentUri: String, directory: String, regex: String, durationMillis: Long)(injector: Injector) =
    new CppFileOrderSourceClient(
      injector.instance[FileOrderSourceClient],
      agentUri = agentUri,
      directory = directory,
      regex = regex,
      durationMillis = durationMillis)(
        injector.instance[ActorSystem])
}
