package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.name.Names
import com.google.inject.{AbstractModule, Injector}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.XmlUtils.xmlQuoted
import com.sos.scheduler.engine.kernel.command.{CommandDispatcher, HasCommandHandlers}
import com.sos.scheduler.engine.kernel.plugin.PluginAdapter._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import javax.annotation.Nullable
import org.w3c.dom.Element
import scala.util.control.NonFatal

/** Die Engine spricht die Plugin über diesen Adapter an. */
final class PluginAdapter(val configuration: PluginConfiguration) {

  @Nullable private var _pluginInstance: Plugin = null

  private[plugin] def tryClose(): Unit = {
    for (o ← Option(_pluginInstance)) {
      try o.close()
      catch {
        case NonFatal(t) ⇒ logger.warn(s"Close $this: $t", t)
      }
    }
  }

  private[plugin] def initialize(injector: Injector): Unit = {
    try {
      if(_pluginInstance != null) throw new IllegalStateException(s"$this is already initialized")
      _pluginInstance = newPluginInstance(injector)
    }
    catch {
      case x: Exception => throw new RuntimeException(s"$this cannot be initialized: $x", x)
    }
  }

  private[plugin] def prepare(): Unit =
    try {
      if (pluginInstance.isPrepared) throw new IllegalStateException(s"$this is already prepared")
      _pluginInstance.prepare()
      if (!_pluginInstance.isPrepared) throw new IllegalStateException(s"$this: prepare() did not result in isPrepared")
    }
    catch {
      case x: Exception ⇒ throw new RuntimeException(s"$this cannot be prepared: $x", x)
    }

  private[plugin] def tryActivate(): Unit =
    try activate()
    catch {
      case NonFatal(t) ⇒ logger.warn(s"Activate $this: $t", t)
    }

  private[plugin] def activate(): Unit =
    try {
      if (pluginInstance.isActive) throw new IllegalStateException(s"$this is already active")
      pluginInstance.activate()
      if (!_pluginInstance.isActive) throw new IllegalStateException(s"$this: prepare() did not result in isPrepared")
    }
    catch {
      case x: Exception => throw new RuntimeException(s"$this cannot be activated: $x", x)
    }

  private def newPluginInstance(injector: Injector) =
    newPluginInstanceByDI(injector, configuration.pluginClass, configuration.configElement)

  private[plugin] def xmlState: String =
    try pluginInstance.xmlState
    catch {
      case x: Exception ⇒ "<ERROR text=" + xmlQuoted(x.toString) + "/>"
    }

  private[plugin] def commandDispatcher =
    pluginInstance match {
      case o: HasCommandHandlers ⇒ new CommandDispatcher(o.commandHandlers)
      case _ ⇒ throw new SchedulerException("Plugin is not a " + classOf[HasCommandHandlers].getSimpleName)
    }

  private[plugin] def pluginInstanceOption: Option[Plugin] =
    Option(_pluginInstance)

  private[plugin] def pluginInstance =
    _pluginInstance match {
      case null ⇒ throw new IllegalStateException("PluginAdapter not initialized")
      case o ⇒ o
    }

  private[plugin] def isPrepared = pluginInstance.isPrepared

  private[plugin] def isActive = pluginInstance.isActive

  override def toString = s"Plugin $pluginClassName"

  private[plugin] def pluginClassName: String = configuration.className
}

object PluginAdapter {
  private val logger = Logger(getClass)

  private def newPluginInstanceByDI(injector: Injector, c: Class[_ <: Plugin], configElement: Element): Plugin = {
    val module = new AbstractModule {
      protected def configure(): Unit = {
        bind(classOf[Element]) annotatedWith Names.named(Plugins.configurationXMLName) toInstance configElement
      }
    }
    injector.createChildInjector(module).getInstance(c)
  }
}
