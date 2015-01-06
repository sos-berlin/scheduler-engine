package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HttpRemoteProcessStarter @Inject private(client: HttpSchedulerCommandClient)(implicit executionContext: ExecutionContext) {

  def startRemoteTask(schedulerApiTcpPort: Int, configuration: ApiProcessConfiguration, remoteUri: String): Future[HttpRemoteProcess] =
    (client.uncheckedExecute(remoteUri, configuration.startRemoteTaskXmlElem(schedulerApiTcpPort))
      map ProcessDescriptor.fromXml
      map { o ⇒ new HttpRemoteProcess(client, remoteUri, o) })
}
