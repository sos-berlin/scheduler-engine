package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClientFactory
import com.sos.scheduler.engine.client.agent.HttpRemoteProcessStarter._
import com.sos.scheduler.engine.client.command.SchedulerClientFactory
import com.sos.scheduler.engine.common.scalautil.Logger
import javax.inject.{Inject, Singleton}
import scala.concurrent.{Future, Promise}
import scala.util.{Failure, Success, Try}
import spray.http.StatusCodes.NotFound

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(
  schedulerClientFactory: SchedulerClientFactory,
  agentClientFactory: AgentClientFactory,
  actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  def startRemoteTask(schedulerApiTcpPort: Int, configuration: ApiProcessConfiguration, agentUri: String): Future[HttpRemoteProcess] = {
    val universalClient = agentClientFactory.apply(agentUri)
    val classicClient = schedulerClientFactory.apply(agentUri)
    if (TryClassicAgentToo) {
      val promise = Promise[HttpRemoteProcess]()
      universalClient.executeCommand(configuration.toUniversalAgentCommand)
      .onComplete {
        case Success(response) ⇒
          promise.complete(Try {
            val processDescriptor = ProcessDescriptor.fromStartProcessResponse(response)
            val tunnelToken = response.tunnelTokenOption.getOrElse { sys.error("TunnelToken expected") }
            new TunnelledHttpRemoteProcess(actorSystem, classicClient, processDescriptor, schedulerApiTcpPort = schedulerApiTcpPort, universalClient, tunnelToken)
          })
        case Failure(t: spray.httpx.UnsuccessfulResponseException) if t.response.status == NotFound ⇒
          logger.info(s"$agentUri doesn't seem to be an Universal Agent: '${t.response.status}'. Switching to Classic Agent client")
          promise.completeWith(
            classicClient.uncheckedExecute(configuration.toClassicXmlElem(schedulerApiTcpPort))
            map ProcessDescriptor.fromXml
            map { pd ⇒ new HttpRemoteProcess.Standard(classicClient, pd, actorSystem.dispatcher) })
      }
      promise.future
    } else {
      universalClient.executeCommand(configuration.toUniversalAgentCommand) map { response ⇒
        val processDescriptor = ProcessDescriptor.fromStartProcessResponse(response)
        val tunnelToken = response.tunnelTokenOption.getOrElse { sys.error(s"Missing TunnelToken from agent $agentUri") }
        new TunnelledHttpRemoteProcess(actorSystem, classicClient, processDescriptor, schedulerApiTcpPort = schedulerApiTcpPort, universalClient, tunnelToken)
      }
    }
  }
}

object HttpRemoteProcessStarter {
  private val logger = Logger(getClass)
  val TryClassicAgentToo = !(sys.props.get("jobscheduler.tryClassicAgent") exists { _.toBoolean })
}
