package com.sos.scheduler.engine.plugins.newwebservice

import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.WebServiceActor._
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.routes.WebServices
import javax.inject.Inject
import scala.collection.JavaConversions._
import spray.routing._

final class WebServiceActor @Inject private(
  protected val configuration: NewWebServicePluginConfiguration,
  extraRoutes: java.util.Set[ExtraRoute],
  schedulerConfiguration: SchedulerConfiguration,
  protected val client: DirectSchedulerClient,
  injector: Injector)
extends HttpServiceActor with WebServices {

  override def receive = {
    for (o ← extraRoutes) logger.info(s"Using ${o.getClass.getName}")
    var extras: Vector[Route] = extraRoutes.toVector map { _.route }
    val routes = route +: extras
    runRoute(routes reduce { _ ~ _ })
  }
}

object WebServiceActor {
  private val logger = Logger(getClass)
}
