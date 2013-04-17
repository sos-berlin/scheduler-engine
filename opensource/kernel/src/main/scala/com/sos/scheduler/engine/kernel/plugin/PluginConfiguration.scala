package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.Module
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.XmlUtils._
import com.sos.scheduler.engine.kernel.plugin.ActivationMode.activateOnStart
import com.sos.scheduler.engine.kernel.plugin.ActivationMode.dontActivate
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import org.w3c.dom.Element
import scala.Predef._
import scala.collection.immutable

private [plugin] case class PluginConfiguration(className: String, activationMode: ActivationMode, configElementOption: Option[Element]) {

  val pluginClass = {
    val c = Class.forName(className)
    require(classOf[Plugin] isAssignableFrom c, s"Plugin $c does not implement ${classOf[Plugin]}")
    c.asInstanceOf[Class[_ <: Plugin]]
  }

  val guiceModuleOption: Option[Module] =
    Option(pluginClass.getAnnotation(classOf[UseGuiceModule])) map { _.value.newInstance() }
}

private [plugin] object PluginConfiguration {
  private val logger = Logger(getClass)

  private [plugin] def readXml(configurationXml: String): immutable.Seq[PluginConfiguration] = {
    val root = loadXml(configurationXml).getDocumentElement
    for (pluginsElement <- Option(elementXPathOrNull(root, "config/plugins")).to[immutable.Seq];
         e <- elementsXPath(pluginsElement, "plugin")) yield
      read(e)
  }

  private[plugin] def read(e: Element): PluginConfiguration = {
    val className = e.getAttribute("java_class")
    if (className.isEmpty) throw new SchedulerException("Missing attribute java_class in <plugin>")
    val result = PluginConfiguration(
      className,
      if (booleanXmlAttribute(e, "dont_activate", false)) dontActivate else activateOnStart,
      Option(elementXPathOrNull(e, "plugin.config")))
    logger.debug(s"Configuration for ${result.className} read")
    result
  }
}
