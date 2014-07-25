package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}

/**
 * @author Joacim Zschimmer
 */
final class HttpRemoteProcessStarter private(conf: ApiProcessConfiguration, client: HttpSchedulerCommandClient)(implicit executionContext: ExecutionContext) {

  def startRemoteTask(schedulerApiTcpPort: Int): Future[HttpRemoteProcess] =
    client.execute(conf.startRemoteTaskXmlElem(schedulerApiTcpPort)) map ProcessDescriptor.fromXml map { o ⇒ new HttpRemoteProcess(client, o) }
}

object HttpRemoteProcessStarter {
  @Singleton
  final class Factory @Inject private(clientFactory: HttpSchedulerCommandClient.Factory, executionContext: ExecutionContext) {
    def apply(conf: ApiProcessConfiguration) = new HttpRemoteProcessStarter(conf, clientFactory(conf.remoteSchedulerUri))(executionContext)
  }
}
