package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.{ActorSystem, PoisonPill, Props}
import akka.util.Switch
import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.plugins.newwebservice.SprayServletContextInitializer._
import com.typesafe.config.Config
import javax.servlet.{ServletContextEvent, ServletContextListener}
import scala.language.reflectiveCalls
import spray.servlet.ConnectorSettings
import spray.servlet.Initializer.{ServiceActorAttrName, SettingsAttrName, SystemAttrName}

/** Like spray.servlet.Initializer. */
final class SprayServletContextInitializer(injector: Injector) extends ServletContextListener {

  private val started = new Switch
  private val actorSystem = injector.instance[ActorSystem]
  private lazy val actor = actorSystem actorOf WebServiceActor.props(injector)

  // TODO Bei ActorInitializationException Plugin abbrechen. Bisher wird die Exception nur protokolliert und der Scheduler setzt fort.

  def contextInitialized(servletContextEvent: ServletContextEvent): Unit =
    started switchOn {
      logger.debug("Starting Spray based web services ...")
      val servletContext = servletContextEvent.getServletContext
      servletContext.setAttribute(SettingsAttrName, ConnectorSettings(injector.instance[Config]))  // ???
      servletContext.setAttribute(SystemAttrName, actorSystem)
      servletContext.setAttribute(ServiceActorAttrName, actor)
    }

  def contextDestroyed(e: ServletContextEvent): Unit =
    started switchOff {
      logger.debug("Stopping Spray based web services ...")
      actor ! PoisonPill
    }
}

object SprayServletContextInitializer {
  private val logger = Logger(getClass)
}
