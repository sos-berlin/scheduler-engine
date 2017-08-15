package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import akka.util.Timeout
import com.sos.scheduler.engine.agent.data.commandresponses.{StartTaskFailed, StartTaskSucceeded}
import com.sos.scheduler.engine.agent.data.web.AgentUris
import com.sos.scheduler.engine.client.agent.HttpRemoteProcessStarter._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.http.client.heartbeat.{HeartbeatRequestor, HttpHeartbeatTiming}
import com.sos.scheduler.engine.tunnel.client.WebTunnelClient
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(
  agentClientFactory: SchedulerAgentClientFactory,
  newHeartbeatRequestor: HeartbeatRequestor.Factory)
  (implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  def startRemoteTask(
    schedulerApiTcpPort: Int,
    configuration: ApiProcessConfiguration,
    agentUri: AgentAddress,
    httpHeartbeatTiming: Option[HttpHeartbeatTiming]): Future[HttpRemoteProcess] =
  {
    val port = schedulerApiTcpPort
    val timing = httpHeartbeatTiming
    agentClientFactory.apply(agentUri).executeCommand(configuration.toUniversalAgentCommand) map {
      case response: StartTaskSucceeded ⇒
        val processDescriptor_ = ProcessDescriptor.fromStartProcessResponse(response)
        new TunnelledHttpRemoteProcess {
          def actorSystem = HttpRemoteProcessStarter.this.actorSystem
          val agentClient = agentClientFactory.apply(agentUri)
          val processDescriptor = processDescriptor_
          def schedulerApiTcpPort = port
          val tunnelClient = new WebTunnelClient(
            processDescriptor_.tunnelToken,
            AgentUris(agentUri).tunnel(processDescriptor_.tunnelToken.id),
            timing map newHeartbeatRequestor)(
            HttpRemoteProcessStarter.this.actorSystem)
          {
            def tunnelSendReceive(t: Timeout)(implicit ec: ExecutionContext) = agentClient.agentSendReceive(t)(ec)
          }
          .closeWithCloser
        }
      case StartTaskFailed(message) ⇒
        throw new AgentException(s"Agent start task command returned error: $message")
    }
  }
}

object HttpRemoteProcessStarter {
  final class AgentException private[HttpRemoteProcessStarter](message: String)
  extends RuntimeException(message)
}
