package com.sos.scheduler.engine.plugins.newwebservice

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.routes.WebServices
import javax.inject.Inject
import spray.routing._

final class WebServiceActor @Inject private(
  protected val configuration: NewWebServicePluginConfiguration,
  injector: Injector)
extends HttpServiceActor with WebServices {

  // Lazy, because these JobScheduler objects are available first after start (without lazy we get a deadlock due to different threads)
  protected lazy val schedulerConfiguration = injector.instance[SchedulerConfiguration]
  protected lazy val client = injector.instance[DirectSchedulerClient]
  protected lazy val fileBasedSubsystemRegister = injector.instance[FileBasedSubsystem.Register]

  override def receive = runRoute(route)
}
