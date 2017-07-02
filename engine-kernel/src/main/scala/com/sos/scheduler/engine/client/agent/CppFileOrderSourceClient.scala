package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.google.inject.Injector
import com.sos.jobscheduler.agent.data.commands.AgentCommand.RequestFileOrderSourceContent
import com.sos.jobscheduler.base.utils.StackTraces._
import com.sos.jobscheduler.common.guice.GuiceImplicits.RichInjector
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.scheduler.engine.client.agent.CppFileOrderSourceClient._
import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
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
  agentClientFactory: SchedulerAgentClientFactory,
  agentUri: AgentAddress,
  directory: String,
  regex: String,
  duration: Duration)
  (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue,
    actorSystem: ActorSystem) {

  //import schedulerThreadCallQueue.JsonFormats.executionContext
  import actorSystem.dispatcher

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
        try inSchedulerThread { resultCppCall.call(forCpp.appendCurrentStackTrace) }
        catch {
          case t: CppProxyInvalidatedException ⇒ logger.trace(s"Ignored: $t")  // Okay if C++ object (Directory_file_order_source) has been closed
          case t: CallQueue.ClosedException ⇒ logger.trace(s"Ignored: $t")  // Okay if JobScheduler has been closed
        }
      }
    }
  }
}

object CppFileOrderSourceClient {
  private val logger = Logger(getClass)

  @ForCpp
  def apply(agentUri: String, directory: String, regex: String, durationMillis: Long)(injector: Injector) =
    new CppFileOrderSourceClient(
      injector.instance[SchedulerAgentClientFactory],
      agentUri = AgentAddress.normalized(agentUri),
      directory = directory,
      regex = regex,
      duration = Duration.ofMillis(durationMillis))(
        injector.instance[SchedulerThreadCallQueue],
        injector.instance[ActorSystem])
}
