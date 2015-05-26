package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.google.inject.Injector
import com.sos.scheduler.engine.agent.client.AgentClientFactory
import com.sos.scheduler.engine.agent.data.commands.RequestFileOrderSourceContent
import com.sos.scheduler.engine.client.agent.CppFileOrderSourceClient._
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.Tries._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import scala.collection.JavaConversions._
import scala.util.Try

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppFileOrderSourceClient private(
  agentClientFactory: AgentClientFactory,
  agentUri: String,
  directory: String,
  regex: String,
  durationMillis: Long,
  schedulerThreadCallQueue: SchedulerThreadCallQueue)
  (implicit actorSystem: ActorSystem) {

  import schedulerThreadCallQueue.implicits.executionContext

  private val agent = agentClientFactory.apply(agentUri = agentUri)

  @ForCpp
  def readFiles(knownFiles: java.util.List[String], resultCppCall: CppCall): Unit = {
    val command = RequestFileOrderSourceContent(directory = directory, regex = regex, durationMillis = durationMillis, knownFiles.toSet)
    agent.executeCommand(command) onComplete { completion ⇒
      val forCpp: Try[java.util.List[String]] = completion map { _.files map { _.path } }
      try resultCppCall.call(forCpp.withThisStackTrace)
      catch { case t: CppProxyInvalidatedException ⇒ logger.trace(s"Ignored: $t") } // Okay if C++ object (Directory_file_order_source) has been closed
    }
  }
}

object CppFileOrderSourceClient {
  private val logger = Logger(getClass)

  @ForCpp
  def apply(agentUri: String, directory: String, regex: String, durationMillis: Long)(injector: Injector) =
    new CppFileOrderSourceClient(
      injector.instance[AgentClientFactory],
      agentUri = agentUri,
      directory = directory,
      regex = regex,
      durationMillis = durationMillis,
      injector.instance[SchedulerThreadCallQueue])(
        injector.instance[ActorSystem])
}
