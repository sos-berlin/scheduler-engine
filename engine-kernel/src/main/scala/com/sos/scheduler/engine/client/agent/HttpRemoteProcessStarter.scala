package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.command.SchedulerClientFactory
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(clientFactory: SchedulerClientFactory)(implicit executionContext: ExecutionContext) {

  def startRemoteTask(schedulerApiTcpPort: Int, configuration: ApiProcessConfiguration, remoteUri: String): Future[HttpRemoteProcess] = {
    val agent = clientFactory.apply(remoteUri)
    (agent.uncheckedExecute(configuration.startRemoteTaskXmlElem(schedulerApiTcpPort))
      map ProcessDescriptor.fromXml
      map { o ⇒ new HttpRemoteProcess(agent, o) })
  }
}
