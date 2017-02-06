package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.Props
import com.google.inject.{AbstractModule, Injector, Provides}
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactory
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.WebLogDirectives.handleErrorAndLog
import com.sos.scheduler.engine.common.sprayutils.web.auth.GateKeeper
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.event.JocOrderStatisticsChangedSource
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.WebServiceActor._
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.routes.AllRoutes
import com.sos.scheduler.engine.plugins.newwebservice.routes.agent.AgentRouteSchedulerAdapter
import com.typesafe.config.Config
import javax.inject.{Inject, Provider, Singleton}
import scala.collection.JavaConversions._
import scala.concurrent.ExecutionContext
import spray.routing._

// An Actor must not be a @Singleton!
final class WebServiceActor @Inject private(
  gateKeeper: GateKeeper,
  extraRoutes: java.util.Set[ExtraRoute],
  protected val configuration: NewWebServicePluginConfiguration,
  protected val schedulerConfiguration: SchedulerConfiguration,
  protected val client: DirectSchedulerClient,
  protected val spoolerC: SpoolerC,
  protected val orderSubsystem: OrderSubsystemClient,
  protected val taskSubsystem: TaskSubsystemClient,
  protected val prefixLog: PrefixLog,
  protected val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val disposableCppProxyRegister: DisposableCppProxyRegister,
  protected val orderStatisticsChangedSource: JocOrderStatisticsChangedSource,
  config: Config,
  implicit protected val executionContext: ExecutionContext,
  toAgentClientProvider: Provider[SchedulerAgentClientFactory])
extends HttpServiceActor with AllRoutes with AgentRouteSchedulerAdapter {

  protected lazy val toAgentClient = toAgentClientProvider.get()

  private val completeRoute = {
    for (o ← extraRoutes) logger.info(s"Using ${o.getClass.getName}")
    val extras: Vector[Route] = extraRoutes.toVector map { _.route }
    (route +: extras) reduce { _ ~ _ }
  }

  override def postStop() = {
    logger.debug("Stopped")
    super.postStop()
  }

  override def receive = runRoute(
    handleErrorAndLog(subConfig = config.getConfig("jobscheduler.master.webserver")).apply {
      gateKeeper.restrict.apply { _ ⇒
        completeRoute
      }
    })
}

object WebServiceActor {
  private val logger = Logger(getClass)

  def props(gateKeeper: GateKeeper, injector: Injector) = Props {
    injector.createChildInjector(new AbstractModule {
      def configure() = {}

      @Provides @Singleton
      def provideGateKeeper(): GateKeeper = gateKeeper
    })
    .instance[WebServiceActor]
  }
}
