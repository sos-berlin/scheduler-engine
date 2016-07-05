package com.sos.scheduler.engine.client.command

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.scheduler.engine.client.command.SchedulerClientFactory.MyWebCommandClient
import com.sos.scheduler.engine.client.web.WebCommandClient
import javax.inject.{Inject, Singleton}
import spray.http.Uri

/**
 * HTTP client for JobScheduler command webservice.
 *
 * @author Joacim Zschimmer
 */
@Singleton
final class SchedulerClientFactory @Inject private(implicit actorRefFactory: ActorRefFactory) {

  def apply(schedulerUri: Uri): CommandClient = {
    val commandUri = schedulerUri.path match {
      case Uri.Path.Empty | Uri.Path.SingleSlash ⇒ schedulerUri.copy(path = Uri.Path("/jobscheduler/engine/command"))
      case _ ⇒ throw new IllegalArgumentException(s"Invalid JobScheduler URL: $schedulerUri")
    }
    new MyWebCommandClient(commandUri, actorRefFactory)
  }
}

object SchedulerClientFactory {
  private final class MyWebCommandClient(protected val commandUri: Uri, protected val actorRefFactory: ActorRefFactory)
  extends WebCommandClient
}
