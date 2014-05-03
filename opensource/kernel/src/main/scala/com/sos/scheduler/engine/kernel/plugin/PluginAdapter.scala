package com.sos.scheduler.engine.kernel.plugin

import PluginAdapter._
import com.google.common.base.Throwables.getStackTraceAsString
import com.google.inject.AbstractModule
import com.google.inject.Injector
import com.google.inject.name.Names
import com.sos.scheduler.engine.common.xml.XmlUtils.xmlQuoted
import com.sos.scheduler.engine.kernel.command.{HasCommandHandlers, CommandDispatcher}
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import javax.annotation.Nullable
import org.w3c.dom.Element
import scala.Option
import scala.collection.JavaConversions._
import scala.util.control.NonFatal

/** Die Engine spricht die Plugin über diesen Adapter an. */
class PluginAdapter(configuration: PluginConfiguration) {

  private var _log: PrefixLog = null
  @Nullable private var _pluginInstance: Plugin = null


  private[plugin] final def tryClose() {
    for (o <- Option(_pluginInstance)) {
      try o.close()
      catch {
        case NonFatal(t) => logThrowable(t)
      }
    }
  }

  private[plugin] final def initialize(injector: Injector, log: PrefixLog) {
    _log = log
    try {
      if(_pluginInstance != null) throw new IllegalStateException(s"$this is already initialized")
      _pluginInstance = newPluginInstance(injector)
    }
    catch {
      case x: Exception => throw new RuntimeException(s"$this cannot be initialized: $x", x)
    }
  }

  private[plugin] final def prepare() {
    try {
      if (pluginInstance.isPrepared) throw new IllegalStateException(s"$this is already prepared")
      _pluginInstance.prepare()
    }
    catch {
      case x: Exception ⇒ throw new RuntimeException(s"$this cannot be prepared: $x", x)
    }
  }

  private[plugin] final def tryActivate() {
    try activate()
    catch {
      case NonFatal(t) => logThrowable(t)
    }
  }

  private[plugin] final def activate() {
    _log = log
    try {
      if (pluginInstance.isActive) throw new IllegalStateException(s"$this is already active")
      pluginInstance.activate()
    }
    catch {
      case x: Exception => throw new RuntimeException(s"$this cannot be activated: $x", x)
    }
  }

  private def newPluginInstance(injector: Injector) =
    newPluginInstanceByDI(injector, configuration.pluginClass, configuration.configElement)

  private[plugin] final def xmlState: String = {
    try pluginInstance.xmlState
    catch {
      case x: Exception ⇒ "<ERROR text=" + xmlQuoted(x.toString) + "/>"
    }
  }

  private[plugin] final def commandDispatcher =
    pluginInstance match {
      case o: HasCommandHandlers ⇒ new CommandDispatcher(o.commandHandlers)
      case _ ⇒ throw new SchedulerException("Plugin is not a " + classOf[HasCommandHandlers].getSimpleName)
    }

  private def logThrowable(t: Throwable) {
    log.error(toString + ": " + t + "\n" + getStackTraceAsString(t))
  }

  private[plugin] def pluginInstance =
    _pluginInstance match {
      case null ⇒ throw new IllegalStateException("PluginAdapter not initialized")
      case o ⇒ o
    }

  private def log =
    _log match {
      case null ⇒ throw new IllegalStateException("PluginAdapter not initialized")
      case o ⇒ o
    }

  private[plugin] def isPrepared = pluginInstance.isPrepared

  private[plugin] def isActive = pluginInstance.isActive

  override def toString = s"Plugin $pluginClassName"

  private[plugin] final def pluginClassName: String = configuration.className
}

object PluginAdapter {
  private def newPluginInstanceByDI(injector: Injector, c: Class[_ <: Plugin], configElement: Element): Plugin = {
    val module = new AbstractModule {
      protected def configure() {
        bind(classOf[Element]) annotatedWith Names.named(Plugins.configurationXMLName) toInstance configElement
      }
    }
    injector.createChildInjector(module).getInstance(c)
  }
}
