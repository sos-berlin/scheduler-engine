package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.Props
import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.WebServiceActor._
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.routes.AllRoutes
import javax.inject.Inject
import scala.collection.JavaConversions._
import spray.routing._

final class WebServiceActor @Inject private(
  protected val configuration: NewWebServicePluginConfiguration,
  extraRoutes: java.util.Set[ExtraRoute],
  protected val schedulerConfiguration: SchedulerConfiguration,
  protected val client: DirectSchedulerClient,
  protected val spoolerC: SpoolerC,
  protected val prefixLog: PrefixLog,
  protected val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val disposableCppProxyRegister: DisposableCppProxyRegister)
extends HttpServiceActor with AllRoutes {

  override def postStop() = {
    logger.debug("Stopped")
    super.postStop()
  }

  override def receive = {
    for (o ← extraRoutes) logger.info(s"Using ${o.getClass.getName}")
    var extras: Vector[Route] = extraRoutes.toVector map { _.route }
    val routes = route +: extras
    runRoute(routes reduce { _ ~ _ })
  }
}

object WebServiceActor {
  private val logger = Logger(getClass)

  def props(injector: Injector) = Props { injector.instance[WebServiceActor] }
}
