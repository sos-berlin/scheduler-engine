package com.sos.scheduler.engine.plugins.newwebservice

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, StandingOrderSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import javax.inject.Inject
import spray.routing._

final class WebServiceActor @Inject private(
  protected val configuration: NewWebServicePluginConfiguration,
  injector: Injector)
extends HttpServiceActor with WebServices{

  // Lazy, because these JobScheduler objects are available first after start (without lazy we get a deadlock due to different threads)
  protected lazy val schedulerConfiguration = injector.instance[SchedulerConfiguration]
  protected lazy val scheduler = injector.instance[Scheduler]
  protected lazy val fileBasedSubsystemRegister = injector.instance[FileBasedSubsystem.Register]
  protected lazy val orderSubsystem = injector.instance[OrderSubsystem]
  protected lazy val standingOrderSubsystem = injector.instance[StandingOrderSubsystem]

  override def receive = runRoute(route)
}
