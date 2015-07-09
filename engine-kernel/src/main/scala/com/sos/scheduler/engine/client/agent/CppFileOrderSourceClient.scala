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
import java.time.Duration
import org.scalactic.Requirements._
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
  duration: Duration,
  schedulerThreadCallQueue: SchedulerThreadCallQueue)
  (implicit actorSystem: ActorSystem) {

  import schedulerThreadCallQueue.implicits.executionContext

  private val agent = agentClientFactory.apply(agentUri = agentUri)
  private var isInCall = false
  private var closed = false

  @ForCpp
  def close(): Unit = closed = true

  @ForCpp
  def readFiles(knownFiles: java.util.List[String], resultCppCall: CppCall): Unit = {
    requireState(!isInCall)
    isInCall = true
    val command = RequestFileOrderSourceContent(directory = directory, regex = regex, duration = duration, knownFiles.toSet)
    agent.executeCommand(command) onComplete { completion ⇒
      isInCall = false
      if (closed) {
        logger.debug(s"Closed, response is ignored")
      } else {
        val forCpp: Try[java.util.List[String]] = completion map { _.files map { _.path } }
        try resultCppCall.call(forCpp.withThisStackTrace)
        catch { case t: CppProxyInvalidatedException ⇒ logger.trace(s"Ignored: $t") } // Okay if C++ object (Directory_file_order_source) has been closed
      }
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
      duration = Duration.ofMillis(durationMillis),
      injector.instance[SchedulerThreadCallQueue])(
        injector.instance[ActorSystem])
}
