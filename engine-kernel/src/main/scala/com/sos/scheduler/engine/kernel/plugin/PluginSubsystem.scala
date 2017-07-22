package com.sos.scheduler.engine.kernel.plugin

import com.google.common.collect.ImmutableList
import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.AssignableFrom.assignableFrom
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.filebaseds.TypedPathRegister
import com.sos.scheduler.engine.eventbus.{EventBus, EventHandlerAnnotated}
import com.sos.scheduler.engine.kernel.command.{CommandHandler, HasCommandHandlers}
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly
import scala.collection.immutable
import scala.reflect.ClassTag

@ForCpp
@Singleton
final class PluginSubsystem @Inject private(
  pluginConfigurations: immutable.Seq[PluginConfiguration],
  injector: Injector,
  eventBus: EventBus)
extends Subsystem with HasCommandHandlers with AutoCloseable {

  private val classNameToConfiguration: Map[String, PluginConfiguration] =
    pluginConfigurations toKeyedMap { _.className } withDefault { o ⇒ throw new NoSuchElementException(s"Unknown plugin '$o'") }
  private var classToPluginAdapter: Map[Class[Plugin], PluginAdapter] = Map()
  private var namespaceToPlugin: Map[String, NamespaceXmlPlugin] = Map()
  private lazy val typeToSourceChangingPlugins: Map[FileBasedType, immutable.Seq[XmlConfigurationChangingPlugin]] =
    (for (p ← plugins[XmlConfigurationChangingPlugin]; t ← p.fileBasedTypes) yield t → p).toSeqMultiMap withDefaultValue Nil

  private[kernel] val commandHandlers = ImmutableList.of[CommandHandler](
    new PluginCommandExecutor(this),
    new PluginCommandCommandXmlParser(this),
    new PluginCommandResultXmlizer(this))

  private[kernel] def initialize(): Unit = {
    classToPluginAdapter = (pluginConfigurations map { o ⇒ o.pluginClass → new PluginAdapter(o) }).uniqueToMap
    for (p ← pluginAdapters) p.initialize(injector)
    plugins[EventHandlerAnnotated] foreach eventBus.registerAnnotated
    for (p ← pluginAdapters) p.prepare()
    namespaceToPlugin = plugins[NamespaceXmlPlugin] toKeyedMap { _.xmlNamespace } withDefault { o ⇒ throw new NoSuchElementException(s"Unknown XML namespace: $o")}
  }

  def close(): Unit = {
    plugins[EventHandlerAnnotated] foreach eventBus.unregisterAnnotated
    for (p ← pluginAdapters) p.tryClose()
  }

  private[kernel] def activate(): Unit =
    for (p ← pluginAdapters if !p.configuration.testInhibitsActivateOnStart) {
      p.activate()
    }

  @ForCpp
  private[kernel] def changeObjectXmlBytes(typeName: String, pathString: String, xmlBytes: Array[Byte]): Array[Byte] = {
    val typ = FileBasedType.fromInternalCppName(typeName)
    val path = TypedPathRegister.fileBasedTypedToCompanion(typ).apply(pathString)
    var result = xmlBytes
    for (p ← typeToSourceChangingPlugins(typ)) {
      result = p.changeXmlConfiguration(typ, path, xmlBytes)
    }
    result
  }

  @TestOnly
  def activatePlugin(c: Class[_ <: Plugin]): Unit = classNameToPluginAdapter(c.getName).activate()

  def pluginByClass[A <: Plugin](c: Class[A]): A = classNameToPluginAdapter(c.getName).pluginInstance.asInstanceOf[A]

  def pluginOptionByClass[A <: Plugin](c: Class[A]): Option[A] =
    for (pluginConfiguration ← classNameToConfiguration.get(c.getName)) yield
      classToPluginAdapter(pluginConfiguration.pluginClass).pluginInstance.asInstanceOf[A]

  private[plugin] def classNameToPluginAdapter(className: String): PluginAdapter =
    classToPluginAdapter(classNameToConfiguration(className).pluginClass)

  private[kernel] def xmlNamespaceToPlugins[A : ClassTag](namespace: String): Option[A] = namespaceToPlugin.get(namespace) collect assignableFrom[A]

  def stateOption(name: String): Option[String] = {
    val values = for (plugin ← plugins[Plugin]; value ← plugin.state.lift(name)) yield value
    if (values.size > 1) throw new IllegalStateException(s"Plugin state name '$name' is not unique")
    values.headOption
  }

  private[plugin] def plugins[A : ClassTag]: immutable.Iterable[A] =
    (classToPluginAdapter.valuesIterator map { _.pluginInstance } collect assignableFrom[A]).toImmutableIterable

  private def pluginAdapters = classToPluginAdapter.values
}
