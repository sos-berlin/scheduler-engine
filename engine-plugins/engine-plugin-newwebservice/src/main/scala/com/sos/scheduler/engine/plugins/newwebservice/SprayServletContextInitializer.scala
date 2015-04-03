package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.{ActorSystem, Props}
import akka.util.Switch
import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.plugins.newwebservice.SprayServletContextInitializer._
import com.typesafe.config.ConfigFactory
import javax.servlet.{ServletContextEvent, ServletContextListener}
import scala.language.reflectiveCalls
import spray.servlet.ConnectorSettings
import spray.servlet.Initializer.{ServiceActorAttrName, SettingsAttrName, SystemAttrName}

/** Like spray.servlet.Initializer. */
class SprayServletContextInitializer(injector: Injector) extends ServletContextListener {
  private val started = new Switch
  private[this] val actorSystem = injector.instance[ActorSystem]

  // TODO Bei ActorInitializationException Plugin abbrechen. Bisher wird die Exception nur protokolliert und der Scheduler setzt fort.

  def contextInitialized(servletContextEvent: ServletContextEvent): Unit =
    started switchOn {
      logger.debug("Starting Spray ...")
      val config = ConfigFactory.load(ReferenceConfResourcePath)
      val settings = ConnectorSettings(config)
      val actor = actorSystem actorOf Props { injector.getInstance(classOf[WebServiceActor]) }
      val servletContext = servletContextEvent.getServletContext
      servletContext.setAttribute(SettingsAttrName, settings)
      servletContext.setAttribute(SystemAttrName, actorSystem)
      servletContext.setAttribute(ServiceActorAttrName, actor)
    }

  def contextDestroyed(e: ServletContextEvent): Unit =
    started switchOff {
      // Need to shutdown Spray??? logger.debug("Shutting down Spray ...")
    }
}

object SprayServletContextInitializer {
  private val ReferenceConfResourcePath = "com/sos/scheduler/engine/plugins/newwebservice/configuration/reference.conf"
  private val logger = Logger(getClass)
}
