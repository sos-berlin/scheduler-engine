package com.sos.scheduler.engine.kernel.plugin

import com.google.common.collect.ImmutableList
import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.eventbus.{EventBus, EventHandlerAnnotated}
import com.sos.scheduler.engine.kernel.command.{CommandHandler, HasCommandHandlers}
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerException, Subsystem}
import javax.inject.{Inject, Singleton}
import scala.collection.{immutable, mutable}

@Singleton
final class PluginSubsystem @Inject private(
  pluginConfigurations: immutable.Seq[PluginConfiguration],
  prefixLog: PrefixLog,
  injector: Injector,
  eventBus: EventBus)
extends Subsystem with HasCommandHandlers with AutoCloseable {

  private val pluginConfigurationMap: Map[String, PluginConfiguration] = (pluginConfigurations map { o ⇒ o.className → o }).toMap
  private val pluginAdapterMap = mutable.HashMap[PluginClass, PluginAdapter]()
  private var namespaceToPlugin: Map[String, NamespaceXmlPlugin] = null

  val commandHandlers = ImmutableList.of[CommandHandler](
    new PluginCommandExecutor(this),
    new PluginCommandCommandXmlParser(this),
    new PluginCommandResultXmlizer(this))

  def initialize(): Unit = {
    pluginAdapterMap ++= newPluginAdapterSeq()
    for (p ← pluginAdapterMap.values) {
      p.initialize(injector, prefixLog)
      tryRegisterEventHandler(p.pluginInstance)
    }
    for (p ← pluginAdapterMap.values) {
      p.prepare()
    }
    namespaceToPlugin = (for (p ← pluginAdapterMap.values) yield
      Some(p.pluginInstance) collect { case p: NamespaceXmlPlugin ⇒ p.xmlNamespace → p }).flatten.toDistinctMap
  }

  private def newPluginAdapterSeq() =
    pluginConfigurations requireDistinct { _.pluginClass } map { o ⇒ o.pluginClass → new PluginAdapter(o) }

  def close(): Unit =
    for (p ← pluginAdapterMap.values) {
      tryUnregisterEventHandler(p.pluginInstance)
      p.tryClose()
    }

  private def tryRegisterEventHandler(o: Plugin): Unit =
    o match {
      case e: EventHandlerAnnotated => eventBus.registerAnnotated(e)
      case _ ⇒
    }

  private def tryUnregisterEventHandler(o: Plugin): Unit =
    o match {
      case e: EventHandlerAnnotated => eventBus.unregisterAnnotated(e)
      case _ ⇒
    }

  def activate(): Unit =
    for (p ← pluginAdapters
         if pluginConfigurationMap(p.pluginClassName).activationMode eq ActivationMode.activateOnStart) {
      p.activate()
    }

  def activatePlugin(c: PluginClass): Unit = pluginAdapterByClassName(c.getName).activate()

  def pluginByClass[A <: Plugin](c: Class[A]): A = pluginAdapterByClassName(c.getName).pluginInstance.asInstanceOf[A]

  private[plugin] def pluginAdapterByClassName(className: String): PluginAdapter =
    pluginAdapterMap(pluginConfiguration(className).pluginClass)

  private[plugin] def pluginConfiguration(className: String) =
    pluginConfigurationMap.getOrElse(className, throw new SchedulerException(s"Unknown plugin '$className'"))

  def xmlNamespaceToPlugins(namespace: String): Option[NamespaceXmlPlugin] = namespaceToPlugin.get(namespace)

  def xmlNamespaceToPlugins: immutable.Iterable[NamespaceXmlPlugin] = namespaceToPlugin.values.toImmutableIterable

  private def pluginAdapters = pluginAdapterMap.values
}
